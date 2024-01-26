// +build rocksdb_inner

package grocksdb

// #include <stdlib.h>
// #include "rocksdb/c.h"
// #include "extend_inner.h"
import "C"

import "bytes"

type MemtableIterator struct {
	c *C.rocksdb_memtable_iterator_t
}

// Valid returns false only when an Iterator has iterated past either the
// first or the last key in the database.
func (iter *MemtableIterator) Valid() bool {
	return C.rocksdb_memtable_iter_valid(iter.c) != 0
}

// ValidForPrefix returns false only when an Iterator has iterated past the
// first or the last key in the database or the specified prefix.
func (iter *MemtableIterator) ValidForPrefix(prefix []byte) bool {
	if C.rocksdb_memtable_iter_valid(iter.c) == 0 {
		return false
	}

	key := iter.Key()
	result := bytes.HasPrefix(key.Data(), prefix)
	key.Free()
	return result
}

// Key returns the key the iterator currently holds.
func (iter *MemtableIterator) Key() *Slice {
	var cLen C.size_t
	cKey := C.rocksdb_memtable_iter_key(iter.c, &cLen)
	if cKey == nil {
		return nil
	}
	return &Slice{data: cKey, size: cLen, freed: true}
}

// Value returns the value in the database the iterator currently holds.
func (iter *MemtableIterator) Value() *Slice {
	var cLen C.size_t
	cVal := C.rocksdb_memtable_iter_value(iter.c, &cLen)
	if cVal == nil {
		return nil
	}
	return &Slice{data: cVal, size: cLen, freed: true}
}

// Next moves the iterator to the next sequential key in the database.
func (iter *MemtableIterator) Next() {
	C.rocksdb_memtable_iter_next(iter.c)
}

// Prev moves the iterator to the previous sequential key in the database.
func (iter *MemtableIterator) Prev() {
	C.rocksdb_memtable_iter_prev(iter.c)
}

// SeekToFirst moves the iterator to the first key in the database.
func (iter *MemtableIterator) SeekToFirst() {
	C.rocksdb_memtable_iter_seek_to_first(iter.c)
}

// SeekToLast moves the iterator to the last key in the database.
func (iter *MemtableIterator) SeekToLast() {
	C.rocksdb_memtable_iter_seek_to_last(iter.c)
}

// Seek moves the iterator to the position greater than or equal to the key.
func (iter *MemtableIterator) Seek(key []byte) {
	cKey := refGoBytes(key)
	C.rocksdb_memtable_iter_seek(iter.c, cKey, C.size_t(len(key)))
}

// SeekForPrev moves the iterator to the last key that less than or equal
// to the target key, in contrast with Seek.
func (iter *MemtableIterator) SeekForPrev(key []byte) {
	cKey := refGoBytes(key)
	C.rocksdb_memtable_iter_seek_for_prev(iter.c, cKey, C.size_t(len(key)))
}

// Err returns nil if no errors happened during iteration, or the actual
// error otherwise.
func (iter *MemtableIterator) Err() (err error) {
	var cErr *C.char
	C.rocksdb_memtable_iter_get_error(iter.c, &cErr)
	err = fromCError(cErr)
	return
}

// Close closes the iterator.
func (iter *MemtableIterator) Close() {
	C.rocksdb_memtable_iter_destroy(iter.c)
	iter.c = nil
}

type Memtable struct {
	c *C.rocksdb_memtable_skiplist_t
}

func (m *Memtable) Add(seq uint64, key []byte, value []byte) (err error) {
	var (
		cErr   *C.char
		cKey   = refGoBytes(key)
		cValue = refGoBytes(value)
	)

	C.rocksdb_memtable_skiplist_add(m.c, C.uint64_t(seq), cKey, C.size_t(len(key)), cValue, C.size_t(len(value)), &cErr)
	err = fromCError(cErr)

	return
}

func (m *Memtable) NewIterator(opts *ReadOptions) *MemtableIterator {
	cIter := C.rocksdb_memtable_skiplist_iterator(m.c, opts.c)
	return &MemtableIterator{c: cIter}
}

func (m *Memtable) Destroy() {
	C.rocksdb_memtable_skiplist_destroy(m.c)
	m.c = nil
}

func NewMemtable(dbOpts *Options) *Memtable {
	c := C.rocksdb_memtable_skiplist_create(dbOpts.c)
	return &Memtable{c: c}
}
