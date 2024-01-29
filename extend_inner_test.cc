// +build cgo_ignore

#include <gtest/gtest.h>
#include "rocksdb/rocksdb_namespace.h"
#include "extend.h"
#include "extend_inner.h"

TEST(ExtenInnerTest, TestMemtableIterator)
{
    auto *opt = rocksdb_options_create_default();
    auto *ropt = rocksdb_readoptions_create();
    auto mem = rocksdb_memtable_create(opt, 1, 1);
    auto ite = rocksdb_memtable_iterator(mem, ropt);

    rocksdb_internal_iter_destroy(ite);
    rocksdb_readoptions_destroy(ropt);
    rocksdb_memtable_destroy(mem);
    rocksdb_options_destroy(opt);
}

TEST(ExtenInnerTest, TestDBIterator)
{
    auto *opt = rocksdb_options_create_default();
    auto *ropt = rocksdb_readoptions_create();

    auto name = "../test";
    auto cfNum = 1;
    const char *handles[] = {"default"};
    rocksdb_options_t *cfopts[] = {opt};
    auto cfhs = new rocksdb_column_family_handle_t *[cfNum];
    char *err = nullptr;

    auto db = rocksdb_open_column_families(opt, name, cfNum, handles, cfopts, cfhs, &err);
    if (err != nullptr)
    {
        std::cerr << err << std::endl;
    }

    rocksdb_put_with_uint64_ts(db, "1", 1, 1, "1", 1, &err);
    if (err != nullptr)
    {
        std::cerr << err << std::endl;
    }

    auto ite = rocksdb_wrapped_internal_iterator_create(db, cfhs[0], ropt);

    rocksdb_iter_destroy(ite);
    rocksdb_readoptions_destroy(ropt);
    rocksdb_options_destroy(opt);
}