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
    ASSERT_EQ(nullptr, err);

    rocksdb_put_with_current_ts(db, "1", 1, "1", 1, &err);
    if (err != nullptr)
    {
        std::cerr << err << std::endl;
    }
    ASSERT_EQ(nullptr, err);

    rocksdb_put_with_current_ts(db, "3", 1, "3", 1, &err);
    if (err != nullptr)
    {
        std::cerr << err << std::endl;
    }
    ASSERT_EQ(nullptr, err);

    rocksdb_delete_range_cf_current_ts(db, cfhs[0], "1", 1, "3", 1, &err);
    if (err != nullptr)
    {
        std::cerr << err << std::endl;
    }
    ASSERT_EQ(nullptr, err);

    // rocksdb_flush(db, rocksdb_flushoptions_create(), &err);
    // if (err != nullptr)
    // {
    //     std::cerr << err << std::endl;
    // }
    // ASSERT_EQ(nullptr, err);

    char *ts = new char[8];
    size_t tslen = 8;
    auto u_ts = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
    rocksdb_uint64ts_encode(u_ts, &ts, &tslen);

    auto ts_decode = rocksdb_uint64ts_decode(ts, tslen, &err);
    if (err != nullptr)
    {
        std::cerr << err << std::endl;
    }
    ASSERT_EQ(nullptr, err);
    ASSERT_EQ(u_ts, ts_decode);
    rocksdb_readoptions_set_timestamp(ropt, ts, tslen);

    size_t vallen = 0;
    auto val = rocksdb_get_with_current_ts(db, "3", 1, &vallen, &err);
    if (vallen > 0)
    {
        std::cout << "value: " << std::string(val).substr(0, vallen) << std::endl;
    }

    auto ite = rocksdb_column_family_handle_memtable_range_tombstone_iterator_create(cfhs[0], ropt, &err);
    if (err != nullptr)
    {
        std::cerr << err << std::endl;
    }
    ASSERT_EQ(nullptr, err);
    ASSERT_NE(nullptr, ite);

    rocksdb_flush(db, rocksdb_flushoptions_create(), &err);
    if (err != nullptr)
    {
        std::cerr << err << std::endl;
    }
    ASSERT_EQ(nullptr, err);

    auto func = [](const char *key, size_t keylen, const char *val, size_t vallen)
    {
        std::cout << "start: " << std::string(key).substr(0, keylen - 16) << " ,end: " << std::string(val).substr(0, vallen - 8) << std::endl;
    };
    rocksdb_internal_iter_foreach(ite, func, &err);
    rocksdb_internal_iter_destroy(ite);

    rocksdb_tables_range_tombstone_summary(db);

    auto list = rocksdb_column_family_handle_range_tombstone_list(db, cfhs[0], ropt, &err);
    if (err != nullptr)
    {
        std::cerr << err << std::endl;
    }
    ASSERT_EQ(nullptr, err);

    for (size_t i = 0; i < rocksdb_range_tombstone_list_size(list); i++)
    {
        size_t start_key_len;
        size_t end_key_len;
        size_t ts_key_len;
        auto start_key = rocksdb_range_tombstone_list_get_start_key(list, i, &start_key_len);
        auto end_key = rocksdb_range_tombstone_list_get_end_key(list, i, &end_key_len);
        auto seq = rocksdb_range_tombstone_list_get_seq(list, i);
        auto ts = rocksdb_range_tombstone_list_get_ts(list, i, &ts_key_len);

        std::cout << "start: " << std::string(start_key, start_key_len).substr(0, 1) << " ,end: " << std::string(val, end_key_len).substr(0, 1) << ", ts: " << rocksdb_uint64ts_decode(ts, ts_key_len, &err) << ", seq: " << seq << std::endl;
        free((char *)start_key);
        free((char *)end_key);
        free((char *)ts);
    }
    rocksdb_range_tombstone_list_destory(list);

    rocksdb_close(db);
    delete cfhs;
    rocksdb_readoptions_destroy(ropt);
    rocksdb_options_destroy(opt);
}