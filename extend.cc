#include <chrono>
#include <iostream>
#include "extend.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/iterator.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/status.h"
#include "rocksdb/sst_file_reader.h"
#include "rocksdb/utilities/backup_engine.h"

using namespace std::chrono;
using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::Status;

extern "C"
{
    struct rocksdb_t
    {
        rocksdb::DB *rep;
    };

    struct rocksdb_backup_engine_t
    {
        rocksdb::BackupEngine *rep;
    };

    struct rocksdb_backup_engine_info_t
    {
        std::vector<rocksdb::BackupInfo> rep;
    };

    struct rocksdb_options_t
    {
        rocksdb::Options rep;
    };

    struct rocksdb_readoptions_t
    {
        rocksdb::ReadOptions rep;
        // stack variables to set pointers to in ReadOptions
        Slice upper_bound;
        Slice lower_bound;
        Slice timestamp;
        Slice iter_start_ts;
    };

    struct rocksdb_column_family_handle_t
    {
        rocksdb::ColumnFamilyHandle *rep;
    };

    struct rocksdb_iterator_t
    {
        rocksdb::Iterator *rep;
    };

    struct rocksdb_sstfilewriter_t
    {
        rocksdb::SstFileWriter *rep;
    };

    struct rocksdb_sstfilereader_t
    {
        rocksdb::SstFileReader *rep;
    };

    static rocksdb::WriteOptions &default_rocksdb_write_option = *new rocksdb::WriteOptions;

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

    void rocksdb_backup_engine_create_new_backup_with_metadata(rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, char **errptr)
    {
        SaveError(errptr, be->rep->CreateNewBackupWithMetadata(db->rep, meta));
    }

    void rocksdb_backup_engine_create_new_backup_flush_with_metadata(rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, unsigned char flush_before_backup, char **errptr)
    {
        SaveError(errptr, be->rep->CreateNewBackupWithMetadata(db->rep, meta, flush_before_backup));
    }

    const char *rocksdb_backup_engine_info_app_metadata(const rocksdb_backup_engine_info_t *info, int index)
    {
        return CopyString(info->rep[index].app_metadata);
    }

    void rocksdb_options_set_comparator_built_in_uint64ts(rocksdb_options_t *opt)
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
        auto s = rocksdb::DecodeU64Ts(Slice(tsSlice, tslen), &ts);
        if (!s.ok())
        {
            SaveError(errptr, s);
        }
        return ts;
    }

    void rocksdb_readoptions_set_timestamp_uint64(rocksdb_readoptions_t *opt, uint64_t ts, char *tsSlice)
    {
        std::string ts_buf;
        rocksdb::EncodeU64Ts(ts, &ts_buf);
        memcpy(tsSlice, ts_buf.data(), sizeof(char) * ts_buf.size());
        opt->timestamp = Slice(tsSlice, ts_buf.size());
        opt->rep.timestamp = &opt->timestamp;
    }

    uint64_t rocksdb_readoptions_get_timestamp_uint64(rocksdb_readoptions_t *opt, char **errptr)
    {
        uint64_t u_ts;
        auto s = rocksdb::DecodeU64Ts(opt->timestamp, &u_ts);
        if (!s.ok())
        {
            SaveError(errptr, s);
        }
        return u_ts;
    }

    void rocksdb_readoptions_set_iter_start_ts_uint64(rocksdb_readoptions_t *opt, uint64_t ts, char *tsSlice)
    {
        std::string ts_buf;
        rocksdb::EncodeU64Ts(ts, &ts_buf);
        memcpy(tsSlice, ts_buf.data(), sizeof(char) * ts_buf.size());
        opt->iter_start_ts = Slice(tsSlice, ts_buf.size());
        opt->rep.iter_start_ts = &opt->iter_start_ts;
    }

    void rocksdb_put_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr)
    {
        auto u_ts = static_cast<uint64_t>(time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count());
        std::string ts_buf;
        SaveError(errptr, db->rep->Put(default_rocksdb_write_option, Slice(key, keylen), rocksdb::EncodeU64Ts(u_ts, &ts_buf), Slice(val, vallen)));
    }

    void rocksdb_put_with_uint64_ts(rocksdb_t *db, const char *key, size_t keylen, uint64_t ts, const char *val, size_t vallen, char **errptr)
    {
        std::string ts_buf;
        SaveError(errptr, db->rep->Put(default_rocksdb_write_option, Slice(key, keylen), rocksdb::EncodeU64Ts(ts, &ts_buf), Slice(val, vallen)));
    }

    void rocksdb_put_with_fixed64_ts(rocksdb_t *db, const char *key, size_t keylen, const char *ts, size_t tslen, const char *val, size_t vallen, char **errptr)
    {
        SaveError(errptr, db->rep->Put(default_rocksdb_write_option, Slice(key, keylen), Slice(ts, tslen), Slice(val, vallen)));
    }

    void rocksdb_delete_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, char **errptr)
    {
        std::string ts_buf;
        auto u_ts = static_cast<uint64_t>(time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count());
        SaveError(errptr, db->rep->Delete(default_rocksdb_write_option, Slice(key, keylen), rocksdb::EncodeU64Ts(u_ts, &ts_buf)));
    }

    void rocksdb_delete_with_uint64_ts(rocksdb_t *db, const char *key, size_t keylen, uint64_t ts, char **errptr)
    {
        std::string ts_buf;
        SaveError(errptr, db->rep->Delete(default_rocksdb_write_option, Slice(key, keylen), rocksdb::EncodeU64Ts(ts, &ts_buf)));
    }

    void rocksdb_delete_with_fixed64_ts(rocksdb_t *db, const char *key, size_t keylen, const char *ts, size_t tslen, char **errptr)
    {
        SaveError(errptr, db->rep->Delete(default_rocksdb_write_option, Slice(key, keylen), Slice(ts, tslen)));
    }

    void rocksdb_delete_range_cf_current_ts(rocksdb_t *db, rocksdb_column_family_handle_t *column_family, const char *start_key, size_t start_key_len, const char *end_key, size_t end_key_len, char **errptr)
    {
        std::string ts_buf;
        auto u_ts = static_cast<uint64_t>(time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count());
        SaveError(errptr, db->rep->DeleteRange(default_rocksdb_write_option, column_family->rep,
                                               Slice(start_key, start_key_len),
                                               Slice(end_key, end_key_len),
                                               rocksdb::EncodeU64Ts(u_ts, &ts_buf)));
    }

    void rocksdb_delete_range_cf_uint64_ts(rocksdb_t *db, rocksdb_column_family_handle_t *column_family, const char *start_key, size_t start_key_len, const char *end_key, size_t end_key_len, uint64_t ts, char **errptr)
    {
        std::string ts_buf;
        SaveError(errptr, db->rep->DeleteRange(default_rocksdb_write_option, column_family->rep,
                                               Slice(start_key, start_key_len),
                                               Slice(end_key, end_key_len),
                                               rocksdb::EncodeU64Ts(ts, &ts_buf)));
    }

    void rocksdb_delete_range_cf_fixed64_ts(rocksdb_t *db, rocksdb_column_family_handle_t *column_family, const char *start_key, size_t start_key_len, const char *end_key, size_t end_key_len, const char *ts, size_t tslen, char **errptr)
    {
        SaveError(errptr, db->rep->DeleteRange(default_rocksdb_write_option, column_family->rep,
                                               Slice(start_key, start_key_len),
                                               Slice(end_key, end_key_len),
                                               Slice(ts, tslen)));
    }

    char *rocksdb_get_with_fixed_ts(rocksdb_t *db, const char *key, size_t keylen, Slice *ts_slice, size_t *vallen, char **errptr)
    {
        char *result = nullptr;
        std::string tmp;

        auto &opt = *new rocksdb::ReadOptions;
        opt.timestamp = ts_slice;

        auto s = db->rep->Get(opt, Slice(key, keylen), &tmp);
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
        delete &opt;
        return result;
    }

    char *rocksdb_get_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, size_t *vallen, char **errptr)
    {
        std::string ts_buf;
        auto ts = rocksdb::EncodeU64Ts(static_cast<uint64_t>(time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count()), &ts_buf);
        return rocksdb_get_with_fixed_ts(db, key, keylen, &ts, vallen, errptr);
    }

    char *rocksdb_get_with_uint64_ts(rocksdb_t *db, const char *key, size_t keylen, uint64_t ts, size_t *vallen, char **errptr)
    {
        std::string ts_buf;
        auto ts_slice = rocksdb::EncodeU64Ts(ts, &ts_buf);
        return rocksdb_get_with_fixed_ts(db, key, keylen, &ts_slice, vallen, errptr);
    }

    char *rocksdb_get_with_fixed64_ts(rocksdb_t *db, const char *key, size_t keylen, const char *ts, size_t tslen, size_t *vallen, char **errptr)
    {
        auto ts_slice = Slice(ts, tslen);
        return rocksdb_get_with_fixed_ts(db, key, keylen, &ts_slice, vallen, errptr);
    }

    void rocksdb_sstfilewriter_delete_range_with_ts(rocksdb_sstfilewriter_t *writer, const char *begin_key, size_t begin_keylen, const char *end_key, size_t end_keylen, const char *ts, size_t tslen, char **errptr)
    {
        SaveError(errptr, writer->rep->DeleteRange(Slice(begin_key, begin_keylen), Slice(end_key, end_keylen), Slice(ts, tslen)));
    }

    rocksdb_sstfilereader_t *rocksdb_sstfilereader_create(rocksdb_options_t *opt)
    {
        auto reader = new rocksdb_sstfilereader_t;
        reader->rep = new rocksdb::SstFileReader(opt->rep);
        return reader;
    }

    void rocksdb_sstfilereader_open(rocksdb_sstfilereader_t *reader, const char *file_path, char **errptr)
    {
        SaveError(errptr, reader->rep->Open(std::string(file_path)));
    }

    rocksdb_iterator_t *rocksdb_sstfilereader_iterator(rocksdb_sstfilereader_t *reader, rocksdb_readoptions_t *opt)
    {
        auto ite = new rocksdb_iterator_t;
        ite->rep = reader->rep->NewIterator(opt->rep);
        return ite;
    }

    void rocksdb_sstfilereader_destroy(rocksdb_sstfilereader_t *reader)
    {
        delete reader->rep;
        delete reader;
    }
}
