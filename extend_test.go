package grocksdb

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestUint64Ts(t *testing.T) {

	ts := uint64(time.Now().UnixMilli())
	tsSlice := EncodeUint64TS(ts)
	defer tsSlice.Free()

	cts, err := DecodeUint64TS(tsSlice.Data())
	if err != nil {
		t.Error(err)
		return
	}
	assert.Equal(t, ts, cts)
}

func TestSetReadOptionsUinit64Ts(t *testing.T) {

	ts := uint64(time.Now().UnixMilli())
	opt := NewDefaultReadOptions()

	SetTimestampUint64(opt, ts)
	ots, err := GetTimestampUint64(opt)
	if err != nil {
		t.Error(err)
		return
	}

	assert.Equal(t, ts, ots)

}
