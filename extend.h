#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include "rocksdb/c.h"

    typedef struct rocksdb_sstfilereader_t rocksdb_sstfilereader_t;

    extern void rocksdb_backup_engine_create_new_backup_with_metadata(rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, char **errptr);
    extern void rocksdb_backup_engine_create_new_backup_flush_with_metadata(rocksdb_backup_engine_t *be, rocksdb_t *db, const char *meta, unsigned char flush_before_backup, char **errptr);
    extern const char *rocksdb_backup_engine_info_app_metadata(const rocksdb_backup_engine_info_t *info, int index);

    extern void rocksdb_options_set_comparator_built_in_uint64ts(rocksdb_options_t *);

    extern void rocksdb_uint64ts_encode(uint64_t ts, char **tsSlice, size_t *tsle);
    extern uint64_t rocksdb_uint64ts_decode(const char *ts, size_t tslen, char **errptr);

    extern void rocksdb_readoptions_set_timestamp_uint64(rocksdb_readoptions_t *opt, uint64_t ts, char *tsSlice);
    extern uint64_t rocksdb_readoptions_get_timestamp_uint64(rocksdb_readoptions_t *opt, char **errptr);
    extern void rocksdb_readoptions_set_iter_start_ts_uint64(rocksdb_readoptions_t *opt, uint64_t ts, char *tsSlice);

    extern void rocksdb_put_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr);
    extern void rocksdb_put_with_uint64_ts(rocksdb_t *db, const char *key, size_t keylen, uint64_t ts, const char *val, size_t vallen, char **errptr);
    extern void rocksdb_put_with_fixed64_ts(rocksdb_t *db, const char *key, size_t keylen, const char *ts, size_t tslen, const char *val, size_t vallen, char **errptr);

    extern void rocksdb_delete_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, char **errptr);
    extern void rocksdb_delete_with_uint64_ts(rocksdb_t *db, const char *key, size_t keylen, uint64_t ts, char **errptr);
    extern void rocksdb_delete_with_fixed64_ts(rocksdb_t *db, const char *key, size_t keylen, const char *ts, size_t tslen, char **errptr);

    extern void rocksdb_delete_range_cf_current_ts(rocksdb_t *db, rocksdb_column_family_handle_t *column_family, const char *start_key, size_t start_key_len, const char *end_key, size_t end_key_len, char **errptr);
    extern void rocksdb_delete_range_cf_uint64_ts(rocksdb_t *db, rocksdb_column_family_handle_t *column_family, const char *start_key, size_t start_key_len, const char *end_key, size_t end_key_len, uint64_t ts, char **errptr);
    extern void rocksdb_delete_range_cf_fixed64_ts(rocksdb_t *db, rocksdb_column_family_handle_t *column_family, const char *start_key, size_t start_key_len, const char *end_key, size_t end_key_len, const char *ts, size_t tslen, char **errptr);

    extern char *rocksdb_get_with_current_ts(rocksdb_t *db, const char *key, size_t keylen, size_t *vallen, char **errptr);
    extern char *rocksdb_get_with_uint64_ts(rocksdb_t *db, const char *key, size_t keylen, uint64_t ts, size_t *vallen, char **errptr);
    extern char *rocksdb_get_with_fixed64_ts(rocksdb_t *db, const char *key, size_t keylen, const char *ts, size_t tslen, size_t *vallen, char **errptr);

    extern void rocksdb_sstfilewriter_delete_range_with_ts(rocksdb_sstfilewriter_t *writer, const char *begin_key, size_t begin_keylen, const char *end_key, size_t end_keylen, const char *ts, size_t tslen, char **errptr);
    extern rocksdb_sstfilereader_t *rocksdb_sstfilereader_create(rocksdb_options_t *opt);
    extern void rocksdb_sstfilereader_open(rocksdb_sstfilereader_t *reader, const char *file_path, char **errptr);
    extern rocksdb_iterator_t *rocksdb_sstfilereader_iterator(rocksdb_sstfilereader_t *reader, rocksdb_readoptions_t *opt);
    extern void rocksdb_sstfilereader_destroy(rocksdb_sstfilereader_t *reader);

#ifdef __cplusplus
} /* end extern "C" */
#endif