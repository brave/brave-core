//! Hash functions and hash utilities

/// Compute a CRC32 hash from the given `bytes`, returning the CRC32 hash.
///
/// When calling this function for the first time, `previous_value` should be `0`.
/// Otherwise, it should be the previous return value of this function to provide a hash
/// of multiple sequential chunks of `bytes`.
#[cfg(feature = "crc32")]
pub fn crc32_update(previous_value: u32, bytes: &[u8]) -> u32 {
    let mut h = crc32fast::Hasher::new_with_initial(previous_value);
    h.update(bytes);
    h.finalize()
}

/// Compute a CRC32 value of the given input `bytes`.
///
/// In case multiple chunks of `bytes` are present, one should use [`crc32_update()`] instead.
#[cfg(feature = "crc32")]
pub fn crc32(bytes: &[u8]) -> u32 {
    let mut h = crc32fast::Hasher::new();
    h.update(bytes);
    h.finalize()
}
