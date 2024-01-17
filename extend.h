
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include "rocksdb/c.h"

    extern ROCKSDB_LIBRARY_API void rocksdb_backup_engine_create_new_backup_with_meta(
        rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, char **errptr);

    extern ROCKSDB_LIBRARY_API void rocksdb_backup_engine_create_new_backup_with_meta_flush(
        rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, unsigned char flush_before_backup, char **errptr);

    extern ROCKSDB_LIBRARY_API const char *rocksdb_backup_engine_info_app_metadata(
        const rocksdb_backup_engine_info_t *info, int index);

    extern ROCKSDB_LIBRARY_API void rocksdb_options_set_comparator_with_uint64ts(rocksdb_options_t*);

#ifdef __cplusplus
} /* end extern "C" */
#endif