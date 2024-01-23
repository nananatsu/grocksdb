
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include "rocksdb/c.h"

    extern ROCKSDB_LIBRARY_API void rocksdb_backup_engine_create_new_backup_with_meta(rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, char **errptr);
    extern ROCKSDB_LIBRARY_API void rocksdb_backup_engine_create_new_backup_with_meta_flush(rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, unsigned char flush_before_backup, char **errptr);
    extern ROCKSDB_LIBRARY_API const char *rocksdb_backup_engine_info_app_metadata(const rocksdb_backup_engine_info_t *info, int index);

    extern ROCKSDB_LIBRARY_API void rocksdb_options_set_comparator_uint64ts(rocksdb_options_t *);

    extern ROCKSDB_LIBRARY_API void rocksdb_uint64ts_encode(uint64_t ts, char **tsSlice, size_t *tsle);
    extern ROCKSDB_LIBRARY_API uint64_t rocksdb_uint64ts_decode(const char *ts, size_t tslen, char **errptr);
    extern ROCKSDB_LIBRARY_API void rocksdb_readoptions_set_timestamp_uint64(rocksdb_readoptions_t *opt, uint64_t ts, char *tsSlice);
    extern ROCKSDB_LIBRARY_API uint64_t rocksdb_readoptions_get_timestamp_uint64(rocksdb_readoptions_t *opt, char **errptr);
    extern ROCKSDB_LIBRARY_API void rocksdb_readoptions_set_iter_start_ts_uint64(rocksdb_readoptions_t *opt, uint64_t ts, char *tsSlice);

    extern ROCKSDB_LIBRARY_API void rocksdb_put_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr);
    extern ROCKSDB_LIBRARY_API void rocksdb_put_with_ts_uint64(rocksdb_t *db, const char *key, size_t keylen, uint64_t ts, const char *val, size_t vallen, char **errptr);
    extern ROCKSDB_LIBRARY_API void rocksdb_put_with_ts_slice(rocksdb_t *db, const char *key, size_t keylen, const char *ts, size_t tslen, const char *val, size_t vallen, char **errptr);

    extern ROCKSDB_LIBRARY_API void rocksdb_delete_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, char **errptr);

    extern ROCKSDB_LIBRARY_API char *rocksdb_get_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, size_t *vallen, char **errptr);
    extern ROCKSDB_LIBRARY_API char *rocksdb_get_with_ts_uint64(rocksdb_t *db, const char *key, size_t keylen, uint64_t ts, size_t *vallen, char **errptr);
    extern ROCKSDB_LIBRARY_API char *rocksdb_get_with_ts_slice(rocksdb_t *db, const char *key, size_t keylen, const char *ts, size_t tslen, size_t *vallen, char **errptr);

#ifdef __cplusplus
} /* end extern "C" */
#endif