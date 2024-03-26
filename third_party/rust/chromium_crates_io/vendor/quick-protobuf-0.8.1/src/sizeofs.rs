//! A module to compute the binary size of data once encoded
//!
//! This module is used primilarly when implementing the `MessageWrite::get_size`

/// Computes the binary size of the varint encoded u64
///
/// https://developers.google.com/protocol-buffers/docs/encoding
pub fn sizeof_varint(v: u64) -> usize {
    match v {
        0x0..=0x7F => 1,
        0x80..=0x3FFF => 2,
        0x4000..=0x1FFFFF => 3,
        0x200000..=0xFFFFFFF => 4,
        0x10000000..=0x7FFFFFFFF => 5,
        0x0800000000..=0x3FFFFFFFFFF => 6,
        0x040000000000..=0x1FFFFFFFFFFFF => 7,
        0x02000000000000..=0xFFFFFFFFFFFFFF => 8,
        0x0100000000000000..=0x7FFFFFFFFFFFFFFF => 9,
        _ => 10,
    }
}

/// Computes the binary size of a variable length chunk of data (wire type 2)
///
/// The total size is the varint encoded length size plus the length itself
/// https://developers.google.com/protocol-buffers/docs/encoding
pub fn sizeof_len(len: usize) -> usize {
    sizeof_varint(len as u64) + len
}

/// Computes the binary size of the varint encoded i32
pub fn sizeof_int32(v: i32) -> usize {
    sizeof_varint(v as u64)
}

/// Computes the binary size of the varint encoded i64
pub fn sizeof_int64(v: i64) -> usize {
    sizeof_varint(v as u64)
}

/// Computes the binary size of the varint encoded uint32
pub fn sizeof_uint32(v: u32) -> usize {
    sizeof_varint(v as u64)
}

/// Computes the binary size of the varint encoded uint64
pub fn sizeof_uint64(v: u64) -> usize {
    sizeof_varint(v)
}

/// Computes the binary size of the varint encoded sint32
pub fn sizeof_sint32(v: i32) -> usize {
    sizeof_varint(((v << 1) ^ (v >> 31)) as u64)
}

/// Computes the binary size of the varint encoded sint64
pub fn sizeof_sint64(v: i64) -> usize {
    sizeof_varint(((v << 1) ^ (v >> 63)) as u64)
}

/// Computes the binary size of the varint encoded bool (always = 1)
pub fn sizeof_bool(_: bool) -> usize {
    1
}

/// Computes the binary size of the varint encoded enum
pub fn sizeof_enum(v: i32) -> usize {
    sizeof_int32(v)
}
