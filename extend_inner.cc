// +build rocksdb_inner

#include <iostream>
#include "extend_inner.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/iterator.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/status.h"
#include "rocksdb/sst_file_reader.h"
#include "rocksdb/utilities/backup_engine.h"

#include "db/arena_wrapped_db_iter.h"
#include "db/column_family.h"
#include "db/memtable.h"
#include "db/db_impl/db_impl.h"
#include "memory/arena.h"
#include "util/cast_util.h"

using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::Status;

extern "C"
{
    struct rocksdb_t
    {
        rocksdb::DB *rep;
    };

    struct rocksdb_env_t
    {
        rocksdb::Env *rep;
        bool is_default;
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

    struct rocksdb_column_family_data_t
    {
        rocksdb::ColumnFamilyData *rep;
    };

    struct rocksdb_memtable_t
    {
        rocksdb::MemTable *rep;
    };

    struct rocksdb_iterator_t
    {
        rocksdb::Iterator *rep;
    };

    struct rocksdb_internal_iterator_t
    {
        rocksdb::InternalIterator *rep;
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

    void rocksdb_internal_iter_destroy(rocksdb_internal_iterator_t *iter)
    {
        if (iter->rep != nullptr)
        {
            iter->rep->~InternalIteratorBase();
        }
        delete iter;
    }

    unsigned char rocksdb_internal_iter_valid(const rocksdb_internal_iterator_t *iter)
    {
        return iter->rep->Valid();
    }

    void rocksdb_internal_iter_seek_to_first(rocksdb_internal_iterator_t *iter)
    {
        iter->rep->SeekToFirst();
    }

    void rocksdb_internal_iter_seek_to_last(rocksdb_internal_iterator_t *iter)
    {
        iter->rep->SeekToLast();
    }

    void rocksdb_internal_iter_seek(rocksdb_internal_iterator_t *iter, const char *k, size_t klen)
    {
        iter->rep->Seek(Slice(k, klen));
    }

    void rocksdb_internal_iter_seek_for_prev(rocksdb_internal_iterator_t *iter, const char *k,
                                             size_t klen)
    {
        iter->rep->SeekForPrev(Slice(k, klen));
    }

    void rocksdb_internal_iter_next(rocksdb_internal_iterator_t *iter) { iter->rep->Next(); }

    void rocksdb_internal_iter_prev(rocksdb_internal_iterator_t *iter) { iter->rep->Prev(); }

    const char *rocksdb_internal_iter_key(const rocksdb_internal_iterator_t *iter, size_t *klen)
    {
        Slice s = iter->rep->key();
        *klen = s.size();
        return s.data();
    }

    const char *rocksdb_internal_iter_value(const rocksdb_internal_iterator_t *iter, size_t *vlen)
    {
        Slice s = iter->rep->value();
        *vlen = s.size();
        return s.data();
    }

    void rocksdb_internal_iter_get_error(const rocksdb_internal_iterator_t *iter, char **errptr)
    {
        SaveError(errptr, iter->rep->status());
    }

    rocksdb_internal_iterator_t *rocksdb_internal_iterator_create(rocksdb_t *db, rocksdb_column_family_handle_t *cfh, const rocksdb_readoptions_t *ropt)
    {
        rocksdb::Arena arena_;
        auto snap = (ropt->rep.snapshot != nullptr) ? ropt->rep.snapshot->GetSequenceNumber() : db->rep->GetLatestSequenceNumber();
        rocksdb_internal_iterator_t *ite = new rocksdb_internal_iterator_t;

        rocksdb::DBImpl *dbi = rocksdb::static_cast_with_check<rocksdb::DBImpl>(db->rep);
        ite->rep = dbi->NewInternalIterator(ropt->rep, &arena_, snap, cfh->rep, true);
        return ite;
    }

    rocksdb_options_t *rocksdb_options_create_default()
    {
        auto opt = rocksdb_options_create();
        opt->rep.create_if_missing = true;
        opt->rep.comparator = rocksdb::BytewiseComparatorWithU64Ts();
        return opt;
    }

    rocksdb_iterator_t *rocksdb_wrapped_internal_iterator_create(rocksdb_t *db, rocksdb_column_family_handle_t *cfh, const rocksdb_readoptions_t *ropt)
    {
        rocksdb::DBImpl *dbi = rocksdb::static_cast_with_check<rocksdb::DBImpl>(db->rep);

        auto env = rocksdb_create_default_env();
        rocksdb::ReadOptions read_options(ropt->rep);

        auto snap = (read_options.snapshot != nullptr) ? read_options.snapshot->GetSequenceNumber() : dbi->GetLatestSequenceNumber();
        // auto snap = (read_options.snapshot != nullptr) ? read_options.snapshot->GetSequenceNumber() : rocksdb::kMaxSequenceNumber;
        auto cfhi = rocksdb::static_cast_with_check<rocksdb::ColumnFamilyHandleImpl>(cfh->rep);
        rocksdb::ColumnFamilyData *cfd = cfhi->cfd();
        rocksdb::ReadCallback *read_callback = nullptr;
        rocksdb::SuperVersion *sv = cfd->GetReferencedSuperVersion(dbi);

        auto *db_iter = rocksdb::NewArenaWrappedDbIterator(
            env->rep, read_options, *cfd->ioptions(), sv->mutable_cf_options, sv->current,
            snap, sv->mutable_cf_options.max_sequential_skip_in_iterations,
            sv->version_number, read_callback, dbi, cfd, false, true);

        auto internal_iter = dbi->NewInternalIterator(db_iter->GetReadOptions(), cfd, sv, db_iter->GetArena(), snap, true, db_iter);
        db_iter->SetIterUnderDBIter(internal_iter);

        rocksdb_iterator_t *result = new rocksdb_iterator_t;
        result->rep = db_iter;
        return result;
    }

    rocksdb_memtable_t *rocksdb_memtable_create(const rocksdb_options_t *opt, uint64_t earliest_seq, uint32_t column_family_id)
    {
        rocksdb::ColumnFamilyOptions cf_options(opt->rep);
        rocksdb::ImmutableOptions ioptions_(opt->rep, opt->rep);
        rocksdb::MutableCFOptions mutable_cf_options_(opt->rep);
        rocksdb::InternalKeyComparator internal_comparator_(cf_options.comparator);

        rocksdb_memtable_t *memtable = new rocksdb_memtable_t;
        memtable->rep = new rocksdb::MemTable(internal_comparator_, ioptions_, mutable_cf_options_, opt->rep.write_buffer_manager.get(), earliest_seq, column_family_id);

        return memtable;
    }

    void rocksdb_memtable_add(rocksdb_memtable_t *t, uint64_t seq, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr)
    {
        SaveError(errptr, t->rep->Add(rocksdb::SequenceNumber(seq), rocksdb::kTypeValue, Slice(key, keylen), Slice(val, vallen), nullptr));
    }

    rocksdb_internal_iterator_t *rocksdb_memtable_iterator(rocksdb_memtable_t *t, const rocksdb_readoptions_t *ropt)
    {
        rocksdb::Arena &arena_ = *new rocksdb::Arena;
        rocksdb_internal_iterator_t *ite = new rocksdb_internal_iterator_t;
        ite->rep = t->rep->NewIterator(ropt->rep, &arena_);
        return ite;
    }

    void rocksdb_memtable_destroy(rocksdb_memtable_t *t)
    {
        delete t->rep;
        delete t;
    }

    rocksdb_column_family_data_t *rocksdb_column_family_handle_get_cfd(rocksdb_column_family_handle_t *cf)
    {
        rocksdb_column_family_data_t *cfd = nullptr;
        rocksdb::ColumnFamilyHandleImpl *cfi = rocksdb::static_cast_with_check<rocksdb::ColumnFamilyHandleImpl>(cf->rep);
        cfd = new rocksdb_column_family_data_t;
        cfd->rep = cfi->cfd();
        return cfd;
    }

    rocksdb_memtable_t *rocksdb_column_family_data_get_memtable(rocksdb_column_family_data_t *cfd)
    {
        rocksdb_memtable_t *memtable = new rocksdb_memtable_t;
        memtable->rep = cfd->rep->mem();
        return memtable;
    }
}
