#include "extend.h"
#include "rocksdb/status.h"
#include "rocksdb/comparator.h"
#include "rocksdb/utilities/backup_engine.h"

using ROCKSDB_NAMESPACE::BackupEngine;
using ROCKSDB_NAMESPACE::BackupInfo;
using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::Options;

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

    const char *rocksdb_backup_engine_info_app_metadata(
        const rocksdb_backup_engine_info_t *info, int index)
    {
        std::string meta = info->rep[index].app_metadata;
        char *cstr = new char[meta.length() + 1];
        strcpy(cstr, meta.c_str());
        return cstr;
    }

    void rocksdb_options_set_comparator_with_uint64ts(rocksdb_options_t *opt)
    {
         opt->rep.comparator = rocksdb::BytewiseComparatorWithU64Ts();
    }
}
