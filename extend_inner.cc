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

    struct RangeTombstone
    {
        RangeTombstone(Slice &start_key, Slice &ts_slice, rocksdb::SequenceNumber seq, Slice &end_key)
            : start_key_(start_key.ToString()),
              ts_(ts_slice.ToString()),
              seq_(seq),
              end_key_(end_key.ToString()){};
        std::string start_key_;
        std::string ts_;
        uint64_t seq_;
        std::string end_key_;
    };

    struct range_tombstone_list_t
    {
        std::vector<RangeTombstone> rep;
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

    static bool SaveErrorMsg(char **errptr, const char *errstr)
    {
        assert(errptr != nullptr);
        if (*errptr == nullptr)
        {
            *errptr = strdup(errstr);
        }
        else
        {
            // TODO(sanjay): Merge with existing error?
            // This is a bug if *errptr is not created by malloc()
            free(*errptr);
            *errptr = strdup(errstr);
        }
        return true;
    }

    rocksdb_options_t *rocksdb_options_create_default()
    {
        auto opt = rocksdb_options_create();
        opt->rep.create_if_missing = true;
        opt->rep.comparator = rocksdb::BytewiseComparatorWithU64Ts();
        return opt;
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

    void rocksdb_internal_iter_seek_for_prev(rocksdb_internal_iterator_t *iter, const char *k, size_t klen)
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

    void rocksdb_internal_iter_foreach(rocksdb_internal_iterator_t *ite, kv_cb cb, char **errptr)
    {
        auto iterator = ite->rep;
        iterator->SeekToFirst();

        for (; iterator->Valid(); iterator->Next())
        {
            auto k = iterator->key();
            auto v = iterator->value();
            cb(k.data(), k.size(), v.data(), v.size());
        }
        SaveError(errptr, iterator->status());
    }

    rocksdb_internal_iterator_t *rocksdb_internal_iterator_create(rocksdb_t *db, rocksdb_column_family_handle_t *cfh, const rocksdb_readoptions_t *ropt)
    {
        rocksdb::Arena arena_;
        auto snap = (ropt->rep.snapshot != nullptr) ? ropt->rep.snapshot->GetSequenceNumber() : db->rep->GetLatestSequenceNumber();
        auto ite = new rocksdb_internal_iterator_t;
        auto *dbi = rocksdb::static_cast_with_check<rocksdb::DBImpl>(db->rep);
        ite->rep = dbi->NewInternalIterator(ropt->rep, &arena_, snap, cfh->rep, true);
        return ite;
    }

    rocksdb_iterator_t *rocksdb_wrapped_internal_iterator_create(rocksdb_t *db, rocksdb_column_family_handle_t *cfh, const rocksdb_readoptions_t *ropt)
    {
        rocksdb::ReadOptions read_options(ropt->rep);
        auto dbi = rocksdb::static_cast_with_check<rocksdb::DBImpl>(db->rep);
        auto env = rocksdb_create_default_env();
        auto snap = (read_options.snapshot != nullptr) ? read_options.snapshot->GetSequenceNumber() : dbi->GetLatestSequenceNumber();
        auto cfhi = rocksdb::static_cast_with_check<rocksdb::ColumnFamilyHandleImpl>(cfh->rep);
        auto cfd = cfhi->cfd();
        auto read_callback = nullptr;
        auto sv = cfd->GetReferencedSuperVersion(dbi);

        auto *db_iter = rocksdb::NewArenaWrappedDbIterator(
            env->rep, read_options, *cfd->ioptions(), sv->mutable_cf_options, sv->current,
            snap, sv->mutable_cf_options.max_sequential_skip_in_iterations,
            sv->version_number, read_callback, dbi, cfd, false, true);

        auto internal_iter = dbi->NewInternalIterator(db_iter->GetReadOptions(), cfd, sv, db_iter->GetArena(), snap, true, db_iter);
        db_iter->SetIterUnderDBIter(internal_iter);

        auto iter = new rocksdb_iterator_t;
        iter->rep = db_iter;
        return iter;
    }

    rocksdb_memtable_t *rocksdb_memtable_create(const rocksdb_options_t *opt, uint64_t earliest_seq, uint32_t column_family_id)
    {
        rocksdb::ColumnFamilyOptions cf_options(opt->rep);
        rocksdb::ImmutableOptions ioptions_(opt->rep, opt->rep);
        rocksdb::MutableCFOptions mutable_cf_options_(opt->rep);
        rocksdb::InternalKeyComparator internal_comparator_(cf_options.comparator);

        auto memtable = new rocksdb_memtable_t;
        memtable->rep = new rocksdb::MemTable(internal_comparator_, ioptions_, mutable_cf_options_, opt->rep.write_buffer_manager.get(), earliest_seq, column_family_id);
        return memtable;
    }

    void rocksdb_memtable_add(rocksdb_memtable_t *t, uint64_t seq, const char *key, size_t keylen, const char *val, size_t vallen, char **errptr)
    {
        SaveError(errptr, t->rep->Add(rocksdb::SequenceNumber(seq), rocksdb::kTypeValue, Slice(key, keylen), Slice(val, vallen), nullptr));
    }

    rocksdb_internal_iterator_t *rocksdb_memtable_iterator(rocksdb_memtable_t *t, const rocksdb_readoptions_t *ropt)
    {
        rocksdb::Arena arena_;
        auto ite = new rocksdb_internal_iterator_t;
        ite->rep = t->rep->NewIterator(ropt->rep, &arena_);
        return ite;
    }

    rocksdb_internal_iterator_t *rocksdb_memtable_range_tombstone_iterator(rocksdb_memtable_t *t, const rocksdb_readoptions_t *ropt, char **errptr)
    {
        rocksdb_internal_iterator_t *ite = nullptr;
        auto tombstone_iter = t->rep->NewRangeTombstoneIterator(ropt->rep, 0, false);
        if (tombstone_iter != nullptr)
        {
            ite = new rocksdb_internal_iterator_t;
            ite->rep = tombstone_iter;
        }
        else
        {
            SaveErrorMsg(errptr, "empty tombstone list");
        }
        return ite;
    }

    void rocksdb_memtable_destroy(rocksdb_memtable_t *t)
    {
        delete t->rep;
        delete t;
    }

    rocksdb_internal_iterator_t *rocksdb_column_family_handle_memtable_iterator_create(rocksdb_column_family_handle_t *cf, rocksdb_readoptions_t *ropt)
    {
        rocksdb::Arena arena_;
        auto cfi = rocksdb::static_cast_with_check<rocksdb::ColumnFamilyHandleImpl>(cf->rep);
        auto ite = new rocksdb_internal_iterator_t;
        ite->rep = cfi->cfd()->mem()->NewIterator(ropt->rep, &arena_);
        return ite;
    }

    rocksdb_internal_iterator_t *rocksdb_column_family_handle_memtable_range_tombstone_iterator_create(rocksdb_column_family_handle_t *cf, rocksdb_readoptions_t *ropt, char **errptr)
    {
        rocksdb_internal_iterator_t *ite = nullptr;
        rocksdb::Arena arena_;
        auto cfi = rocksdb::static_cast_with_check<rocksdb::ColumnFamilyHandleImpl>(cf->rep);
        auto tombstone_iter = cfi->cfd()->mem()->NewRangeTombstoneIterator(ropt->rep, 0, false);
        if (tombstone_iter != nullptr)
        {
            ite = new rocksdb_internal_iterator_t;
            ite->rep = tombstone_iter;
        }
        else
        {
            SaveErrorMsg(errptr, "empty tombstone list");
        }
        return ite;
    }

    range_tombstone_list_t *rocksdb_column_family_handle_range_tombstone_list(rocksdb_t *db, rocksdb_column_family_handle_t *cf, const rocksdb_readoptions_t *ropt, char **errptr)
    {
        auto dbi = rocksdb::static_cast_with_check<rocksdb::DBImpl>(db->rep);
        auto cfi = rocksdb::static_cast_with_check<rocksdb::ColumnFamilyHandleImpl>(cf->rep);
        auto cfd = cfi->cfd();

        auto cache = cfd->table_cache();
        auto super_version = cfd->GetReferencedSuperVersion(dbi);
        auto version = super_version->current;
        auto storage_info = version->storage_info();

        std::vector<RangeTombstone> tombstones;
        for (int level = 0; level < storage_info->num_levels(); level++)
        {
            for (const auto &file_meta : storage_info->LevelFiles(level))
            {
                std::unique_ptr<rocksdb::FragmentedRangeTombstoneIterator> tombstone_iter;
                auto s = cache->GetRangeTombstoneIterator(ropt->rep, cfd->internal_comparator(), *file_meta, cfd->GetLatestMutableCFOptions()->block_protection_bytes_per_key, &tombstone_iter);
                if (!s.ok())
                {
                    SaveError(errptr, s);
                    return nullptr;
                }

                if (tombstone_iter)
                {
                    tombstone_iter->SeekToFirst();
                    while (tombstone_iter->Valid())
                    {
                        auto start_key = tombstone_iter->start_key();
                        auto end_key = tombstone_iter->end_key();
                        auto ts_slice = tombstone_iter->timestamp();
                        auto seq_no = tombstone_iter->seq();
                        auto tombstone = RangeTombstone(start_key, ts_slice, seq_no, end_key);
                        tombstones.push_back(tombstone);
                        tombstone_iter->Next();
                    }
                }
            }
        }
        dbi->CleanupSuperVersion(super_version);

        auto list = new range_tombstone_list_t;
        list->rep = tombstones;
        return list;
    }

    size_t rocksdb_range_tombstone_list_size(range_tombstone_list_t *list)
    {
        return list->rep.size();
    }

    const char *rocksdb_range_tombstone_list_get_start_key(range_tombstone_list_t *list, int index, size_t *keylen)
    {
        auto key = list->rep[index].start_key_;
        *keylen = key.size();
        return CopyString(key);
    }

    const char *rocksdb_range_tombstone_list_get_ts(range_tombstone_list_t *list, int index, size_t *tslen)
    {
        auto ts = list->rep[index].ts_;
        *tslen = ts.size();
        return CopyString(ts);
    }

    uint64_t rocksdb_range_tombstone_list_get_seq(range_tombstone_list_t *list, int index)
    {
        return list->rep[index].seq_;
    }

    const char *rocksdb_range_tombstone_list_get_end_key(range_tombstone_list_t *list, int index, size_t *keylen)
    {
        auto key = list->rep[index].end_key_;
        *keylen = key.size();
        return CopyString(key);
    }

    void rocksdb_range_tombstone_list_destory(range_tombstone_list_t *list)
    {
        delete list;
    }

    void rocksdb_tables_range_tombstone_summary(rocksdb_t *db)
    {
        std::string out;
        auto *dbi = rocksdb::static_cast_with_check<rocksdb::DBImpl>(db->rep);
        dbi->TablesRangeTombstoneSummary(dbi->DefaultColumnFamily(), 65535, &out);
        std::cout << out << std::endl;
    }
}
