// +build rocksdb_inner

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include "rocksdb/c.h"

    typedef struct rocksdb_internal_iterator_t rocksdb_internal_iterator_t;
    typedef struct rocksdb_memtable_t rocksdb_memtable_t;
    typedef struct rocksdb_column_family_data_t rocksdb_column_family_data_t;

    extern rocksdb_options_t *rocksdb_options_create_default();

    extern void rocksdb_internal_iter_destroy(rocksdb_internal_iterator_t *);
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

    extern rocksdb_memtable_t *rocksdb_memtable_create(const rocksdb_options_t *opt, uint64_t earliest_seq, uint32_t column_family_id);
    extern void rocksdb_memtable_add(rocksdb_memtable_t *t, uint64_t seq, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr);
    extern rocksdb_internal_iterator_t *rocksdb_memtable_iterator(rocksdb_memtable_t *t, const rocksdb_readoptions_t *ropt);
    extern void rocksdb_memtable_destroy(rocksdb_memtable_t *t);

    extern rocksdb_column_family_data_t *rocksdb_column_family_handle_get_cfd(rocksdb_column_family_handle_t *cf);
    extern rocksdb_memtable_t *rocksdb_column_family_data_get_memtable(rocksdb_column_family_data_t *cfd);
    extern rocksdb_internal_iterator_t *rocksdb_internal_iterator_create(rocksdb_t *db, rocksdb_column_family_handle_t *cfh, const rocksdb_readoptions_t *ropt);
    extern rocksdb_iterator_t *rocksdb_wrapped_internal_iterator_create(rocksdb_t *db, rocksdb_column_family_handle_t *cfh, const rocksdb_readoptions_t *ropt);

#ifdef __cplusplus
} /* end extern "C" */
#endif