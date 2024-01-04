#include "backup.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/backup_engine.h"

using ROCKSDB_NAMESPACE::BackupEngine;
using ROCKSDB_NAMESPACE::BackupInfo;
using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Status;

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

    void rocksdb_backup_engine_create_new_backup_with_meta(rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, char **errptr)
    {
        Status s = be->rep->CreateNewBackupWithMetadata(db->rep, meta);

        if (!s.ok())
        {
            if (*errptr == nullptr)
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
        }
    }

    const char *rocksdb_backup_engine_info_app_metadata(
        const rocksdb_backup_engine_info_t *info, int index)
    {
        std::string meta = info->rep[index].app_metadata;
        char *cstr = new char[meta.length()];
        strcpy(cstr, meta.c_str());
        return cstr;
    }
}
