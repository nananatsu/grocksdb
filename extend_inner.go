//go:build rocksdb_inner
// +build rocksdb_inner

package grocksdb

// #include <stdlib.h>
// #include "rocksdb/c.h"
// #include "extend_inner.h"
import "C"

import (
	"bytes"
	"sync"
	"unsafe"
)

func toGoBytes(cChar *C.char, cLen C.size_t) []byte {
	cCharPtr := unsafe.Pointer(cChar)
	ret := C.GoBytes(cCharPtr, C.int(cLen))
	C.free(cCharPtr)
	return ret
}

type InternalIterator struct {
	c *C.rocksdb_internal_iterator_t
}

// Valid returns false only when an Iterator has iterated past either the
// first or the last key in the database.
func (iter *InternalIterator) Valid() bool {
	return C.rocksdb_internal_iter_valid(iter.c) != 0
}

// ValidForPrefix returns false only when an Iterator has iterated past the
// first or the last key in the database or the specified prefix.
func (iter *InternalIterator) ValidForPrefix(prefix []byte) bool {
	if C.rocksdb_internal_iter_valid(iter.c) == 0 {
		return false
	}

	key := iter.Key()
	result := bytes.HasPrefix(key.Data(), prefix)
	key.Free()
	return result
}

// Key returns the key the iterator currently holds.
func (iter *InternalIterator) Key() *Slice {
	var cLen C.size_t
	cKey := C.rocksdb_internal_iter_key(iter.c, &cLen)
	if cKey == nil {
		return nil
	}
	return &Slice{data: cKey, size: cLen, freed: true}
}

// Value returns the value in the database the iterator currently holds.
func (iter *InternalIterator) Value() *Slice {
	var cLen C.size_t
	cVal := C.rocksdb_internal_iter_value(iter.c, &cLen)
	if cVal == nil {
		return nil
	}
	return &Slice{data: cVal, size: cLen, freed: true}
}

// Next moves the iterator to the next sequential key in the database.
func (iter *InternalIterator) Next() {
	C.rocksdb_internal_iter_next(iter.c)
}

// Prev moves the iterator to the previous sequential key in the database.
func (iter *InternalIterator) Prev() {
	C.rocksdb_internal_iter_prev(iter.c)
}

// SeekToFirst moves the iterator to the first key in the database.
func (iter *InternalIterator) SeekToFirst() {
	C.rocksdb_internal_iter_seek_to_first(iter.c)
}

// SeekToLast moves the iterator to the last key in the database.
func (iter *InternalIterator) SeekToLast() {
	C.rocksdb_internal_iter_seek_to_last(iter.c)
}

// Seek moves the iterator to the position greater than or equal to the key.
func (iter *InternalIterator) Seek(key []byte) {
	cKey := refGoBytes(key)
	C.rocksdb_internal_iter_seek(iter.c, cKey, C.size_t(len(key)))
}

// SeekForPrev moves the iterator to the last key that less than or equal
// to the target key, in contrast with Seek.
func (iter *InternalIterator) SeekForPrev(key []byte) {
	cKey := refGoBytes(key)
	C.rocksdb_internal_iter_seek_for_prev(iter.c, cKey, C.size_t(len(key)))
}

// Err returns nil if no errors happened during iteration, or the actual
// error otherwise.
func (iter *InternalIterator) Err() (err error) {
	var cErr *C.char
	C.rocksdb_internal_iter_get_error(iter.c, &cErr)
	err = fromCError(cErr)
	return
}

// Close closes the iterator.
func (iter *InternalIterator) Close() {
	C.rocksdb_internal_iter_destroy(iter.c)
	iter.c = nil
}

type Memtable struct {
	c *C.rocksdb_memtable_t
}

func (m *Memtable) Add(seq uint64, key []byte, value []byte) (err error) {
	var (
		cErr   *C.char
		cKey   = refGoBytes(key)
		cValue = refGoBytes(value)
	)

	C.rocksdb_memtable_add(m.c, C.uint64_t(seq), cKey, C.size_t(len(key)), cValue, C.size_t(len(value)), &cErr)
	err = fromCError(cErr)
	return
}

func (m *Memtable) NewIterator(opts *ReadOptions) *InternalIterator {
	cIter := C.rocksdb_memtable_iterator(m.c, opts.c)
	return &InternalIterator{c: cIter}
}

func (m *Memtable) NewRangeTombstoneIterator(opts *ReadOptions) (iter *InternalIterator, err error) {
	var cErr *C.char
	cIter := C.rocksdb_memtable_range_tombstone_iterator(m.c, opts.c, &cErr)
	err = fromCError(cErr)
	iter = &InternalIterator{c: cIter}
	return
}

func (m *Memtable) Destroy() {
	C.rocksdb_memtable_destroy(m.c)
	m.c = nil
}

func NewMemtable(dbOpts *Options, earliestSeq uint64, columnFamilyId uint32) *Memtable {
	c := C.rocksdb_memtable_create(dbOpts.c, C.uint64_t(earliestSeq), C.uint32_t(columnFamilyId))
	return &Memtable{c: c}
}

type RangeTombstone struct {
	StartKey []byte
	EndKey   []byte
	Ts       []byte
	Seq      uint64
}

type RangeTombstoneIterator struct {
	c    *C.range_tombstone_list_t
	mu   sync.Mutex
	cur  int
	size int
}

func (iter *RangeTombstoneIterator) Next() (tombstone *RangeTombstone, ok bool) {
	iter.mu.Lock()
	cur := iter.cur
	if cur < iter.size {
		ok = true
		iter.cur++
	}
	iter.mu.Unlock()

	if !ok {
		return
	}
	cCur := C.int(cur)
	tombstone = &RangeTombstone{}

	var cLen C.size_t
	tombstone.StartKey = toGoBytes(C.rocksdb_range_tombstone_list_get_start_key(iter.c, cCur, &cLen), cLen)
	tombstone.EndKey = toGoBytes(C.rocksdb_range_tombstone_list_get_end_key(iter.c, cCur, &cLen), cLen)
	tombstone.Ts = toGoBytes(C.rocksdb_range_tombstone_list_get_ts(iter.c, cCur, &cLen), cLen)
	tombstone.Seq = uint64(C.rocksdb_range_tombstone_list_get_seq(iter.c, cCur))
	return tombstone, true
}

func (iter *RangeTombstoneIterator) Close() {
	C.rocksdb_range_tombstone_list_destory(iter.c)
	iter.c = nil
}

func NewRangeTombstoneIterator(cList *C.range_tombstone_list_t) *RangeTombstoneIterator {
	return &RangeTombstoneIterator{
		c:    cList,
		size: int(C.rocksdb_range_tombstone_list_size(cList)),
	}
}

func (cfh *ColumnFamilyHandle) NewMemtableIterator(opts *ReadOptions) (iter *InternalIterator) {
	cIter := C.rocksdb_column_family_handle_memtable_iterator_create(cfh.c, opts.c)
	iter = &InternalIterator{c: cIter}
	return
}

func (cfh *ColumnFamilyHandle) NewMemtableRangeTombstoneIterator(opts *ReadOptions) (iter *InternalIterator, err error) {
	var cErr *C.char
	cIter := C.rocksdb_column_family_handle_memtable_range_tombstone_iterator_create(cfh.c, opts.c, &cErr)
	err = fromCError(cErr)
	iter = &InternalIterator{c: cIter}
	return
}

func (cfh *ColumnFamilyHandle) NewRangeTombstoneIterator(db *DB, opts *ReadOptions) (ite *RangeTombstoneIterator, err error) {
	var cErr *C.char
	cList := C.rocksdb_column_family_handle_range_tombstone_list(db.c, cfh.c, opts.c, &cErr)
	err = fromCError(cErr)
	ite = NewRangeTombstoneIterator(cList)
	return
}

func (db *DB) NewInternalIterator(cfh *ColumnFamilyHandle, opts *ReadOptions) *InternalIterator {
	return &InternalIterator{c: C.rocksdb_internal_iterator_create(db.c, cfh.c, opts.c)}
}

func (db *DB) NewWrappedInternalIterator(cfh *ColumnFamilyHandle, opts *ReadOptions) *Iterator {
	return &Iterator{c: C.rocksdb_wrapped_internal_iterator_create(db.c, cfh.c, opts.c)}
}
