//go:build rocksdb_inner
// +build rocksdb_inner

package grocksdb

// #include <stdlib.h>
// #include "rocksdb/c.h"
// #include "extend_inner.h"
import "C"

import "bytes"

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

func (m *Memtable) Destroy() {
	C.rocksdb_memtable_destroy(m.c)
	m.c = nil
}

func NewMemtable(dbOpts *Options, earliestSeq uint64, columnFamilyId uint32) *Memtable {
	c := C.rocksdb_memtable_create(dbOpts.c, C.uint64_t(earliestSeq), C.uint32_t(columnFamilyId))
	return &Memtable{c: c}
}

type ColumnFamilyData struct {
	c *C.rocksdb_column_family_data_t
}

func (cfd *ColumnFamilyData) GetMemtable() *Memtable {
	c := C.rocksdb_column_family_data_get_memtable(cfd.c)
	return &Memtable{c: c}
}

func (h *ColumnFamilyHandle) GetColumnFamilyData() *ColumnFamilyData {
	return &ColumnFamilyData{c: C.rocksdb_column_family_handle_get_cfd(h.c)}
}

func (db *DB) NewInternalIterator(opts *ReadOptions, cfh *ColumnFamilyHandle) *InternalIterator {
	return &InternalIterator{c: C.rocksdb_internal_iterator_create(db.c, cfh.c, opts.c)}
}

func (db *DB) NewWrappedInternalIterator(opts *ReadOptions, cfh *ColumnFamilyHandle) *Iterator {
	return &Iterator{c: C.rocksdb_wrapped_internal_iterator_create(db.c, cfh.c, opts.c)}
}
