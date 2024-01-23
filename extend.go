package grocksdb

// #include <stdlib.h>
// #include "rocksdb/c.h"
// #include "extend.h"
import "C"

func EncodeUint64Ts(ts uint64) *Slice {
	var (
		cTs    *C.char
		cTsLen C.size_t
	)
	C.rocksdb_uint64ts_encode(C.uint64_t(ts), &cTs, &cTsLen)
	return NewSlice(cTs, cTsLen)
}

func DecodeUint64Ts(tsSlice []byte) (ts uint64, err error) {
	var (
		cErr     *C.char
		cTsSlice = refGoBytes(tsSlice)
	)

	cTs := C.rocksdb_uint64ts_decode(cTsSlice, C.size_t(len(tsSlice)), &cErr)
	if err = fromCError(cErr); err == nil {
		ts = uint64(cTs)
	}
	return
}

func SetStartTimestampUint64(opts *ReadOptions, ts uint64) {
	tsSlice := make([]byte, 8)
	opts.timestampStart = tsSlice

	cTsSlice := refGoBytes(tsSlice)
	C.rocksdb_readoptions_set_iter_start_ts_uint64(opts.c, C.uint64_t(ts), cTsSlice)
}

func SetTimestampUint64(opts *ReadOptions, ts uint64) {
	tsSlice := make([]byte, 8)
	opts.timestamp = tsSlice

	cTsSlice := refGoBytes(tsSlice)
	C.rocksdb_readoptions_set_timestamp_uint64(opts.c, C.uint64_t(ts), cTsSlice)
}

func GetTimestampUint64(opts *ReadOptions) (ts uint64, err error) {
	var cErr *C.char
	cTs := C.rocksdb_readoptions_get_timestamp_uint64(opts.c, &cErr)
	if err := fromCError(cErr); err == nil {
		ts = uint64(cTs)
	}
	return
}
