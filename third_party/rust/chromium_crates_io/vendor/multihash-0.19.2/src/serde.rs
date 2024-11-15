//! Multihash Serde (de)serialization

use core::{fmt, mem, slice};

use serde::{
    de::{self, SeqAccess, Visitor},
    ser, Deserialize, Deserializer, Serialize, Serializer,
};

use crate::Multihash;

/// The maximum serialization size of `code` is 9 bytes (a large varint encoded u64) and for `size`
/// is 2 bytes  (a large varint encoded u8), this makes a total of 11 bytes.
const MAXIMUM_PREFIX_SIZE: usize = 11;

/// The is currently no way to allocate an array that is some constant size bigger then a given
/// const generic. Once `generic_const_exprs` are a thing, this struct will no longer be needed.
/// Until then we introduce a hack. We allocate a struct, which contains two independent arrays,
/// which can be specified with const generics. We then treat the whole struct as a slice of
/// continuous memory.
#[repr(C, packed)]
struct Buffer<const SIZE_FIRST: usize, const SIZE_SECOND: usize> {
    first: [u8; SIZE_FIRST],
    second: [u8; SIZE_SECOND],
}

#[allow(unsafe_code)]
impl<const SIZE_FIRST: usize, const SIZE_SECOND: usize> Buffer<SIZE_FIRST, SIZE_SECOND> {
    fn new() -> Self {
        Self {
            first: [0; SIZE_FIRST],
            second: [0; SIZE_SECOND],
        }
    }

    fn as_slice(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self as *const _ as _, mem::size_of::<Self>()) }
    }

    fn as_mut_slice(&mut self) -> &mut [u8] {
        unsafe { slice::from_raw_parts_mut(self as *mut _ as _, mem::size_of::<Self>()) }
    }
}

impl<const SIZE: usize> Serialize for Multihash<SIZE> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let mut buffer = Buffer::<MAXIMUM_PREFIX_SIZE, SIZE>::new();
        let bytes_written = self
            .write(buffer.as_mut_slice())
            .map_err(|_| ser::Error::custom("Failed to serialize Multihash"))?;
        serializer.serialize_bytes(&buffer.as_slice()[..bytes_written])
    }
}

struct BytesVisitor<const SIZE: usize>;

impl<'de, const SIZE: usize> Visitor<'de> for BytesVisitor<SIZE> {
    type Value = Multihash<SIZE>;

    fn expecting(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        write!(fmt, "a valid Multihash in bytes")
    }

    fn visit_bytes<E>(self, bytes: &[u8]) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Multihash::<SIZE>::from_bytes(bytes)
            .map_err(|_| de::Error::custom("Failed to deserialize Multihash"))
    }

    // Some Serde data formats interpret a byte stream as a sequence of bytes (e.g. `serde_json`).
    fn visit_seq<A>(self, mut seq: A) -> Result<Self::Value, A::Error>
    where
        A: SeqAccess<'de>,
    {
        let mut buffer = Buffer::<MAXIMUM_PREFIX_SIZE, SIZE>::new();
        let bytes = buffer.as_mut_slice();

        // Fill the bytes slices with the given sequence
        let mut pos = 0;
        while let Some(byte) = seq.next_element()? {
            bytes[pos] = byte;
            pos += 1;
            if pos >= bytes.len() {
                return Err(de::Error::custom("Failed to deserialize Multihash"));
            }
        }

        Multihash::<SIZE>::from_bytes(&bytes[..pos])
            .map_err(|_| de::Error::custom("Failed to deserialize Multihash"))
    }
}

impl<'de, const SIZE: usize> Deserialize<'de> for Multihash<SIZE> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_bytes(BytesVisitor)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::ptr;

    use serde_test::{assert_tokens, Token};

    const SHA2_256_CODE: u64 = 0x12;
    const DIGEST: [u8; 32] = [
        159, 228, 204, 198, 222, 22, 114, 79, 58, 48, 199, 232, 242, 84, 243, 198, 71, 25, 134,
        172, 177, 248, 216, 207, 142, 150, 206, 42, 215, 219, 231, 251,
    ];

    #[test]
    fn test_serde_json() {
        // This is a concatenation of `SHA2_256_CODE + DIGEST_LENGTH + DIGEST`.
        let expected_json = format!("[{},{},159,228,204,198,222,22,114,79,58,48,199,232,242,84,243,198,71,25,134,172,177,248,216,207,142,150,206,42,215,219,231,251]", SHA2_256_CODE as u8, DIGEST.len() as u8);

        let mh = Multihash::<32>::wrap(SHA2_256_CODE, &DIGEST).unwrap();

        let json = serde_json::to_string(&mh).unwrap();
        assert_eq!(json, expected_json);

        let mh_decoded: Multihash<32> = serde_json::from_str(&json).unwrap();
        assert_eq!(mh, mh_decoded);
    }

    #[test]
    fn test_serde_test() {
        // This is a concatenation of `SHA2_256_CODE + DIGEST_LENGTH + DIGEST`.
        const ENCODED_MULTIHASH_BYTES: [u8; 34] = [
            SHA2_256_CODE as u8,
            DIGEST.len() as u8,
            159,
            228,
            204,
            198,
            222,
            22,
            114,
            79,
            58,
            48,
            199,
            232,
            242,
            84,
            243,
            198,
            71,
            25,
            134,
            172,
            177,
            248,
            216,
            207,
            142,
            150,
            206,
            42,
            215,
            219,
            231,
            251,
        ];

        let mh = Multihash::<32>::wrap(SHA2_256_CODE, &DIGEST).unwrap();

        // As bytes.
        assert_tokens(&mh, &[Token::Bytes(&ENCODED_MULTIHASH_BYTES)]);

        // As sequence.
        serde_test::assert_de_tokens(
            &mh,
            &[
                Token::Seq { len: Some(34) },
                Token::U8(SHA2_256_CODE as u8),
                Token::U8(DIGEST.len() as u8),
                Token::U8(159),
                Token::U8(228),
                Token::U8(204),
                Token::U8(198),
                Token::U8(222),
                Token::U8(22),
                Token::U8(114),
                Token::U8(79),
                Token::U8(58),
                Token::U8(48),
                Token::U8(199),
                Token::U8(232),
                Token::U8(242),
                Token::U8(84),
                Token::U8(243),
                Token::U8(198),
                Token::U8(71),
                Token::U8(25),
                Token::U8(134),
                Token::U8(172),
                Token::U8(177),
                Token::U8(248),
                Token::U8(216),
                Token::U8(207),
                Token::U8(142),
                Token::U8(150),
                Token::U8(206),
                Token::U8(42),
                Token::U8(215),
                Token::U8(219),
                Token::U8(231),
                Token::U8(251),
                Token::SeqEnd,
            ],
        );
    }

    #[test]
    fn test_buffer_alignment() {
        const SIZE_FIRST: usize = 11;
        const SIZE_SECOND: usize = 13;
        let buffer = Buffer::<SIZE_FIRST, SIZE_SECOND>::new();

        // Make sure that the struct allocated continuous memory, as we exploit that fact with the
        // `as_slice` and `as_mut_slice()` methods.
        let start_first = ptr::addr_of!(buffer.first) as *const u8;
        let start_second = ptr::addr_of!(buffer.second) as *const u8;
        #[allow(unsafe_code)]
        unsafe {
            assert_eq!(start_second.offset_from(start_first), SIZE_FIRST as isize);
        };
    }

    #[test]
    fn test_buffer() {
        const SIZE_FIRST: usize = 3;
        const SIZE_SECOND: usize = 8;
        let mut buffer = Buffer::<SIZE_FIRST, SIZE_SECOND>::new();

        let data: [u8; SIZE_FIRST + SIZE_SECOND] = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
        buffer.as_mut_slice().copy_from_slice(&data);
        assert_eq!(buffer.as_slice(), data);
    }
}
