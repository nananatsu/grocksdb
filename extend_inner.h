// +build rocksdb_inner

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include "rocksdb/c.h"

    typedef struct rocksdb_memtable_t rocksdb_memtable_t;
    typedef struct rocksdb_internal_iterator_t rocksdb_internal_iterator_t;
    typedef struct range_tombstone_list_t range_tombstone_list_t;

    typedef void (*kv_cb)(const char *key, size_t keylen, const char *val, size_t vallen);

    extern rocksdb_options_t *rocksdb_options_create_default();

    extern rocksdb_internal_iterator_t *rocksdb_internal_iterator_create(rocksdb_t *db, rocksdb_column_family_handle_t *cfh, const rocksdb_readoptions_t *ropt);
    extern rocksdb_iterator_t *rocksdb_wrapped_internal_iterator_create(rocksdb_t *db, rocksdb_column_family_handle_t *cfh, const rocksdb_readoptions_t *ropt);
    extern unsigned char rocksdb_internal_iter_valid(const rocksdb_internal_iterator_t *);
    extern void rocksdb_internal_iter_seek_to_first(rocksdb_internal_iterator_t *);
    extern void rocksdb_internal_iter_seek_to_last(rocksdb_internal_iterator_t *);
    extern void rocksdb_internal_iter_seek(rocksdb_internal_iterator_t *, const char *k, size_t klen);
    extern void rocksdb_internal_iter_seek_for_prev(rocksdb_internal_iterator_t *, const char *k, size_t klen);
    extern void rocksdb_internal_iter_next(rocksdb_internal_iterator_t *);
    extern void rocksdb_internal_iter_prev(rocksdb_internal_iterator_t *);
    extern const char *rocksdb_internal_iter_key(const rocksdb_internal_iterator_t *, size_t *klen);
    extern const char *rocksdb_internal_iter_value(const rocksdb_internal_iterator_t *, size_t *vlen);
    extern const char *rocksdb_internal_iter_timestamp(const rocksdb_internal_iterator_t *, size_t *tslen);
    extern void rocksdb_internal_iter_get_error(const rocksdb_internal_iterator_t *, char **errptr);
    void rocksdb_internal_iter_foreach(rocksdb_internal_iterator_t *ite, kv_cb cb, char **errptr);
    extern void rocksdb_internal_iter_destroy(rocksdb_internal_iterator_t *);

    extern rocksdb_memtable_t *rocksdb_memtable_create(const rocksdb_options_t *opt, uint64_t earliest_seq, uint32_t column_family_id);
    extern void rocksdb_memtable_add(rocksdb_memtable_t *t, uint64_t seq, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr);
    extern rocksdb_internal_iterator_t *rocksdb_memtable_iterator(rocksdb_memtable_t *t, const rocksdb_readoptions_t *ropt);
    extern rocksdb_internal_iterator_t *rocksdb_memtable_range_tombstone_iterator(rocksdb_memtable_t *t, const rocksdb_readoptions_t *ropt, char **errptr);
    extern void rocksdb_memtable_destroy(rocksdb_memtable_t *t);

    extern rocksdb_internal_iterator_t *rocksdb_column_family_handle_memtable_iterator_create(rocksdb_column_family_handle_t *cf, rocksdb_readoptions_t *ropt);
    extern rocksdb_internal_iterator_t *rocksdb_column_family_handle_memtable_range_tombstone_iterator_create(rocksdb_column_family_handle_t *cf, rocksdb_readoptions_t *ropt, char **errptr);
    extern range_tombstone_list_t *rocksdb_column_family_handle_range_tombstone_list(rocksdb_t *db, rocksdb_column_family_handle_t *cf, const rocksdb_readoptions_t *ropt, char **errptr);

    extern size_t rocksdb_range_tombstone_list_size(range_tombstone_list_t *list);
    extern const char *rocksdb_range_tombstone_list_get_start_key(range_tombstone_list_t *list, int index, size_t *keylen);
    extern const char *rocksdb_range_tombstone_list_get_ts(range_tombstone_list_t *list, int index, size_t *tslen);
    extern uint64_t rocksdb_range_tombstone_list_get_seq(range_tombstone_list_t *list, int index);
    extern const char *rocksdb_range_tombstone_list_get_end_key(range_tombstone_list_t *list, int index, size_t *keylen);
    extern void rocksdb_range_tombstone_list_destory(range_tombstone_list_t *list);

    extern void rocksdb_tables_range_tombstone_summary(rocksdb_t *db);

#ifdef __cplusplus
} /* end extern "C" */
#endif