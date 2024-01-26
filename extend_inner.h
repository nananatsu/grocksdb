// +build rocksdb_inner

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include "rocksdb/c.h"

    typedef struct rocksdb_memtable_iterator_t rocksdb_memtable_iterator_t;
    typedef struct rocksdb_memtable_skiplist_t rocksdb_memtable_skiplist_t;

    extern void rocksdb_memtable_iter_destroy(rocksdb_memtable_iterator_t *);
    extern unsigned char rocksdb_memtable_iter_valid(const rocksdb_memtable_iterator_t *);
    extern void rocksdb_memtable_iter_seek_to_first(rocksdb_memtable_iterator_t *);
    extern void rocksdb_memtable_iter_seek_to_last(rocksdb_memtable_iterator_t *);
    extern void rocksdb_memtable_iter_seek(rocksdb_memtable_iterator_t *, const char *k, size_t klen);
    extern void rocksdb_memtable_iter_seek_for_prev(rocksdb_memtable_iterator_t *, const char *k, size_t klen);
    extern void rocksdb_memtable_iter_next(rocksdb_memtable_iterator_t *);
    extern void rocksdb_memtable_iter_prev(rocksdb_memtable_iterator_t *);
    extern const char *rocksdb_memtable_iter_key(const rocksdb_memtable_iterator_t *, size_t *klen);
    extern const char *rocksdb_memtable_iter_value(const rocksdb_memtable_iterator_t *, size_t *vlen);
    extern const char *rocksdb_memtable_iter_timestamp(const rocksdb_memtable_iterator_t *, size_t *tslen);
    extern void rocksdb_memtable_iter_get_error(const rocksdb_memtable_iterator_t *, char **errptr);

    extern rocksdb_memtable_skiplist_t *rocksdb_memtable_skiplist_create(rocksdb_options_t *opt);
    extern void rocksdb_memtable_skiplist_add(rocksdb_memtable_skiplist_t *t, uint64_t seq, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr);
    extern rocksdb_memtable_iterator_t *rocksdb_memtable_skiplist_iterator(rocksdb_memtable_skiplist_t *t, rocksdb_readoptions_t *ropt);
    extern void rocksdb_memtable_skiplist_destroy(rocksdb_memtable_skiplist_t *t);

#ifdef __cplusplus
} /* end extern "C" */
#endif