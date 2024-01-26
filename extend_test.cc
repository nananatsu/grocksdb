// +build cgo_ignore

#include <gtest/gtest.h>
#include "extend.h"

TEST(ExtenTest, TestReadOptionTS)
{
    rocksdb_readoptions_t *opt = rocksdb_readoptions_create();
    char *tsSlice = new char[8];
    rocksdb_readoptions_set_timestamp_uint64(opt, 123, tsSlice);

    char *err = reinterpret_cast<char *>(malloc(sizeof(char) * 256));
    uint64_t u_ts = rocksdb_readoptions_get_timestamp_uint64(opt, &err);
    EXPECT_EQ(u_ts, 123);

    rocksdb_readoptions_destroy(opt);
    delete tsSlice;
}