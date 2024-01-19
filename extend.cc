#include <chrono>
#include <iostream>
#include "extend.h"
#include "rocksdb/db.h"
#include "rocksdb/status.h"
#include "rocksdb/comparator.h"
#include "rocksdb/utilities/backup_engine.h"

using namespace std::chrono;
using ROCKSDB_NAMESPACE::BackupEngine;
using ROCKSDB_NAMESPACE::BackupInfo;
using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteOptions;

extern "C"
{
    struct rocksdb_t
    {
        DB *rep;
    };

    struct rocksdb_backup_engine_t
    {
        BackupEngine *rep;
    };

    struct rocksdb_backup_engine_info_t
    {
        std::vector<BackupInfo> rep;
    };

    struct rocksdb_options_t
    {
        Options rep;
    };

    struct rocksdb_readoptions_t
    {
        ReadOptions rep;
        // stack variables to set pointers to in ReadOptions
        Slice upper_bound;
        Slice lower_bound;
        Slice timestamp;
        Slice iter_start_ts;
    };

    using time_stamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

    static WriteOptions &default_rocksdb_write_option = *new WriteOptions;

    static char *CopyString(const std::string &str)
    {
        char *result = reinterpret_cast<char *>(malloc(sizeof(char) * str.size()));
        memcpy(result, str.data(), sizeof(char) * str.size());
        return result;
    }

    static bool SaveError(char **errptr, const Status &s)
    {
        assert(errptr != nullptr);
        if (s.ok())
        {
            return false;
        }
        else if (*errptr == nullptr)
        {
            *errptr = strdup(s.ToString().c_str());
        }
        else
        {
            // TODO(sanjay): Merge with existing error?
            // This is a bug if *errptr is not created by malloc()
            free(*errptr);
            *errptr = strdup(s.ToString().c_str());
        }
        return true;
    }

    void rocksdb_backup_engine_create_new_backup_with_meta(rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, char **errptr)
    {
        SaveError(errptr, be->rep->CreateNewBackupWithMetadata(db->rep, meta));
    }

    void rocksdb_backup_engine_create_new_backup_with_meta_flush(rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, unsigned char flush_before_backup, char **errptr)
    {
        SaveError(errptr, be->rep->CreateNewBackupWithMetadata(db->rep, meta, flush_before_backup));
    }

    const char *rocksdb_backup_engine_info_app_metadata(const rocksdb_backup_engine_info_t *info, int index)
    {
        return CopyString(info->rep[index].app_metadata);
    }

    void rocksdb_options_set_comparator_uint64ts(rocksdb_options_t *opt)
    {
        opt->rep.comparator = rocksdb::BytewiseComparatorWithU64Ts();
    }

    void rocksdb_uint64ts_encode(uint64_t ts, char **tsSlice, size_t *tslen)
    {
        std::string ts_buf;
        rocksdb::EncodeU64Ts(ts, &ts_buf);
        *tslen = ts_buf.size();
        *tsSlice = CopyString(ts_buf);
    }

    uint64_t rocksdb_uint64ts_decode(const char *tsSlice, size_t tslen, char **errptr)
    {
        uint64_t ts;
        Status s = rocksdb::DecodeU64Ts(Slice(tsSlice, tslen), &ts);
        if (!s.ok())
        {
            SaveError(errptr, s);
        }
        return ts;
    }

    void rocksdb_readoptions_set_current_timestamp(rocksdb_readoptions_t *opt)
    {
        std::string ts_buf;
        uint64_t u_ts = static_cast<uint64_t>(time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count());

        opt->timestamp = rocksdb::EncodeU64Ts(u_ts, &ts_buf);
        opt->rep.timestamp = &opt->timestamp;
    }

    void rocksdb_put_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr)
    {
        uint64_t u_ts = static_cast<uint64_t>(time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count());
        std::string ts_buf;
        SaveError(errptr, db->rep->Put(default_rocksdb_write_option, Slice(key, keylen), rocksdb::EncodeU64Ts(u_ts, &ts_buf), Slice(val, vallen)));
    }

    void rocksdb_delete_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, char **errptr)
    {
        std::string ts_buf;
        uint64_t u_ts = static_cast<uint64_t>(time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count());
        SaveError(errptr, db->rep->Delete(default_rocksdb_write_option, Slice(key, keylen), rocksdb::EncodeU64Ts(u_ts, &ts_buf)));
    }

    char *rocksdb_get_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, size_t *vallen, char **errptr)
    {
        char *result = nullptr;
        std::string tmp;

        // rocksdb_readoptions_t *opt = rocksdb_readoptions_create();
        // rocksdb_readoptions_t *opt = new rocksdb_readoptions_t;
        // rocksdb_readoptions_set_current_timestamp(opt);
        std::string ts_buf;
        uint64_t u_ts = static_cast<uint64_t>(time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count());
        ReadOptions &opt = *new ReadOptions;
        Slice ts = rocksdb::EncodeU64Ts(u_ts, &ts_buf);
        opt.timestamp = &ts;

        // Status s = db->rep->Get(opt->rep, Slice(key, keylen), &tmp);
        Status s = db->rep->Get(opt, Slice(key, keylen), &tmp);
        if (s.ok())
        {
            *vallen = tmp.size();
            result = CopyString(tmp);
        }
        else
        {
            *vallen = 0;
            if (!s.IsNotFound())
            {
                SaveError(errptr, s);
            }
        }

        // rocksdb_readoptions_destroy(opt);
        delete &opt;
        return result;
    }
}
