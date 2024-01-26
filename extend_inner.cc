// +build rocksdb_inner

#include <iostream>
#include "extend.h"
#include "extend_inner.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/iterator.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/status.h"
#include "rocksdb/sst_file_reader.h"
#include "rocksdb/utilities/backup_engine.h"

#include "db/memtable.h"
#include "memory/arena.h"

using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::Status;

extern "C"
{
    struct rocksdb_t
    {
        rocksdb::DB *rep;
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

    struct rocksdb_memtable_skiplist_t
    {
        rocksdb::MemTable *rep;
    };

    struct rocksdb_memtable_iterator_t
    {
        rocksdb::InternalIterator *rep;
    };

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

    void rocksdb_memtable_iter_destroy(rocksdb_memtable_iterator_t *iter)
    {
        delete iter->rep;
        delete iter;
    }

    unsigned char rocksdb_memtable_iter_valid(const rocksdb_memtable_iterator_t *iter)
    {
        return iter->rep->Valid();
    }

    void rocksdb_memtable_iter_seek_to_first(rocksdb_memtable_iterator_t *iter)
    {
        iter->rep->SeekToFirst();
    }

    void rocksdb_memtable_iter_seek_to_last(rocksdb_memtable_iterator_t *iter)
    {
        iter->rep->SeekToLast();
    }

    void rocksdb_memtable_iter_seek(rocksdb_memtable_iterator_t *iter, const char *k, size_t klen)
    {
        iter->rep->Seek(Slice(k, klen));
    }

    void rocksdb_memtable_iter_seek_for_prev(rocksdb_memtable_iterator_t *iter, const char *k,
                                             size_t klen)
    {
        iter->rep->SeekForPrev(Slice(k, klen));
    }

    void rocksdb_memtable_iter_next(rocksdb_memtable_iterator_t *iter) { iter->rep->Next(); }

    void rocksdb_memtable_iter_prev(rocksdb_memtable_iterator_t *iter) { iter->rep->Prev(); }

    const char *rocksdb_memtable_iter_key(const rocksdb_memtable_iterator_t *iter, size_t *klen)
    {
        Slice s = iter->rep->key();
        *klen = s.size();
        return s.data();
    }

    const char *rocksdb_memtable_iter_value(const rocksdb_memtable_iterator_t *iter, size_t *vlen)
    {
        Slice s = iter->rep->value();
        *vlen = s.size();
        return s.data();
    }

    void rocksdb_memtable_iter_get_error(const rocksdb_memtable_iterator_t *iter, char **errptr)
    {
        SaveError(errptr, iter->rep->status());
    }

    rocksdb_memtable_skiplist_t *rocksdb_memtable_skiplist_create(rocksdb_options_t *opt)
    {
        rocksdb::ColumnFamilyOptions cf_options(opt->rep);
        rocksdb::ImmutableOptions ioptions_(opt->rep, opt->rep);
        rocksdb::MutableCFOptions mutable_cf_options_(opt->rep);
        rocksdb::InternalKeyComparator internal_comparator_(cf_options.comparator);

        rocksdb_memtable_skiplist_t *memtable = new rocksdb_memtable_skiplist_t;
        memtable->rep = new rocksdb::MemTable(internal_comparator_, ioptions_, mutable_cf_options_, opt->rep.write_buffer_manager.get(), 1, 1);

        return memtable;
    }

    void rocksdb_memtable_skiplist_add(rocksdb_memtable_skiplist_t *t, uint64_t seq, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr)
    {
        SaveError(errptr, t->rep->Add(rocksdb::SequenceNumber(seq), rocksdb::kTypeValue, Slice(key, keylen), Slice(val, vallen), nullptr));
    }

    rocksdb_memtable_iterator_t *rocksdb_memtable_skiplist_iterator(rocksdb_memtable_skiplist_t *t, rocksdb_readoptions_t *ropt)
    {
        rocksdb::Arena arena_;
        rocksdb_memtable_iterator_t *ite = new rocksdb_memtable_iterator_t;
        ite->rep = t->rep->NewIterator(ropt->rep, &arena_);
        return ite;
    }

    void rocksdb_memtable_skiplist_destroy(rocksdb_memtable_skiplist_t *t)
    {
        delete t->rep;
        delete t;
    }
}
