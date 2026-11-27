use alloc::string::String;
use alloc::vec::Vec;
use core::hint;

use simdutf8::basic::from_utf8;

use crate::error::DecodingError;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum Flavor {
    Cesu8,
    Mutf8,
}

#[inline]
pub(crate) fn decode(bytes: &[u8], options: DecodeOptions) -> Result<String, DecodingError> {
    let capacity = if options.lossy {
        // NOTE: This is the worst-case scenario where *every* byte is invalid,
        // and we have to replace it with the "U+FFFD REPLACEMENT CHARACTER".
        bytes.len().checked_mul(3).unwrap_or(ISIZE_MAX_USIZE)
    } else {
        // NOTE: Valid CESU-8 and MUTF-8 strings will always be longer than
        // their UTF-8 counterparts, so we can use the length of the input
        // bytes as a speculative capacity.
        bytes.len()
    };

    let mut decoded = Vec::<u8>::with_capacity(capacity);
    let mut index = 0;
    let mut processed = 0;

    macro_rules! err {
        () => {{
            if options.lossy {
                // NOTE: This is the "U+FFFD REPLACEMENT CHARACTER" in UTF-8.
                // Because CESU-8 and MUTF-8 only differ in how they encode
                // 4-byte characters, and null, this UTF-8 is valid as-is in
                // both encodings.
                decoded.extend_from_slice(&[0xef, 0xbf, 0xbd]);
                // SAFETY: We know that `processed` will only ever be less than
                // or equal to `bytes.len()`, so this is safe. We increment
                // `processed` here to ensure that we don't get stuck in an
                // infinite loop.
                processed = unsafe { processed.unchecked_add(1) };
                // NOTE: We unwind `index` to the new start.
                index = processed;
                continue;
            }

            return Err(DecodingError(()));
        }};
    }

    macro_rules! next {
        () => {{
            if index >= bytes.len() {
                err!();
            }

            // SAFETY: We know that `index` is less than `bytes.len()`.
            let byte = unsafe { *bytes.get_unchecked(index) };
            // SAFETY: We know that `index` is less than `bytes.len()`, so at
            // most, `index + 1` will be equal to `isize::MAX + 1`, which will
            // never overflow a `usize`.
            index = unsafe { index.unchecked_add(1) };
            byte
        }};
    }

    macro_rules! next_continue {
        () => {{
            let byte = next!();

            if byte & 0b1100_0000 != 0b1000_0000 {
                err!();
            }

            byte
        }};
    }

    while processed < bytes.len() {
        // NOTE: `processed` should be equal to `index` at the start of each
        // iteration.
        debug_assert_eq!(index, processed);
        // SAFETY: We know that `index` is less than `bytes.len()` due to the
        // loop condition.
        let first = unsafe { *bytes.get_unchecked(processed) };
        // SAFETY: We know that `index` is less than `bytes.len()`, so at most,
        // `index + 1` will be equal to `isize::MAX + 1`, which will never
        // overflow a `usize`.
        index = unsafe { index.unchecked_add(1) };

        match first {
            0x00 if options.flavor == Flavor::Mutf8 => err!(),
            0x00..=0x7f => {
                decoded.push(first);
            }
            0xc0 if options.flavor == Flavor::Mutf8 => {
                if next!() != 0x80 {
                    err!();
                }

                decoded.push(0x00);
            }
            0xc2..=0xdf => {
                let second = next_continue!();
                decoded.extend_from_slice(&[first, second]);
            }
            0xe0..=0xef => {
                let second = next!();

                match (first, second) {
                    (0xe0, 0xa0..=0xbf)
                    | (0xe1..=0xec | 0xee..=0xef, 0x80..=0xbf)
                    | (0xed, 0x80..=0x9f) => {
                        let third = next_continue!();
                        decoded.extend_from_slice(&[first, second, third]);
                    }
                    (0xed, 0xa0..=0xaf) => {
                        if index + 4 > bytes.len() {
                            err!();
                        }

                        // SAFETY: We know that `index + 4` is less than or
                        // equal to `bytes.len()`, so this is safe.
                        let slice = unsafe { bytes.get_unchecked(index..index + 4) };

                        let &[third, fourth, fifth, sixth] = slice else {
                            // SAFETY: We know that the slice is exactly four
                            // bytes.
                            unsafe { hint::unreachable_unchecked() };
                        };

                        // PERF: There was a lot of branching here before, so
                        // this is some magic. Basically, we're checking if the
                        // first byte is a continuation byte, the second byte is
                        // equal to 0xed, the third byte checks if the value is
                        // in the range 0xb0..=0xbf, and the fourth byte is a
                        // continuation byte.
                        let value = u32::from_be_bytes([third, fourth, fifth, sixth]);
                        let validation_mask = 0b1100_0000_1111_1111_1111_0000_1100_0000u32;
                        let desired = 0b1000_0000_1110_1101_1011_0000_1000_0000u32;

                        if value & validation_mask != desired {
                            err!();
                        }

                        index += 4;
                        let c = decode_surrogate_pair(second, third, fifth, sixth);
                        decoded.extend_from_slice(&c);
                    }
                    _ => err!(),
                }
            }
            _ => err!(),
        }

        processed = index;
    }

    // NOTE: We do a sanity check that the decoded string is valid UTF-8. We
    // have to do this because `String::from_utf8_unchecked` doesn't have a
    // sanity check in debug mode.
    debug_assert!(from_utf8(&decoded).is_ok());
    // SAFETY: We know that `decoded` is a valid UTF-8 string because we only
    // ever push valid UTF-8 bytes to it.
    let decoded = unsafe { String::from_utf8_unchecked(decoded) };
    Ok(decoded)
}

#[derive(Debug, Clone, Copy)]
pub(crate) struct DecodeOptions {
    pub(crate) flavor: Flavor,
    pub(crate) lossy: bool,
}

#[inline]
fn decode_surrogate_pair(second: u8, third: u8, fifth: u8, sixth: u8) -> [u8; 4] {
    let high = decode_surrogate(second, third);
    let low = decode_surrogate(fifth, sixth);
    let code_point = 0x10000 + ((high - 0xd800) << 10 | (low - 0xdc00));
    decode_code_point(code_point)
}

#[inline]
fn decode_surrogate(second: u8, third: u8) -> u32 {
    0xd000 | u32::from(second & 0b0011_1111) << 6 | u32::from(third & 0b0011_1111)
}

#[inline]
fn decode_code_point(code_point: u32) -> [u8; 4] {
    [
        0b1111_0000 | ((code_point & 0b1_1100_0000_0000_0000_0000) >> 18) as u8,
        0b1000_0000 | ((code_point & 0b0_0011_1111_0000_0000_0000) >> 12) as u8,
        0b1000_0000 | ((code_point & 0b0_0000_0000_1111_1100_0000) >> 6) as u8,
        0b1000_0000 | ((code_point & 0b0_0000_0000_0000_0011_1111) as u8),
    ]
}

/// Encodes a string into a new vector of bytes using the given flavor of
/// encoding: CESU-8 or MUTF-8.
///
/// NOTE: This function is inlined. It is expected that the call site of this
/// function is **not** inlined. This is to ensure that LLVM elides the flavor
/// logic when the flavor is known at compile time.
///
/// # Panics
///
/// If `value` is greater than <code>[isize::MAX] / 2</code> bytes long, this
/// function might panic by trying to allocate a vector with a capacity greater
/// than [`isize::MAX`] bytes.
#[must_use]
#[inline]
pub(crate) fn encode(value: &str, flavor: Flavor) -> Vec<u8> {
    let mut buf = Vec::new();
    encode_into(value, flavor, &mut buf);
    buf
}

/// Encodes a string into an existing vector of bytes using the given flavor of
/// encoding: CESU-8 or MUTF-8.
///
/// NOTE: This function is inlined. It is expected that the call site of this
/// function is **not** inlined. This is to ensure that LLVM elides the flavor
/// logic when the flavor is known at compile time.
///
/// # Panics
///
/// If `value` is greater than <code>[isize::MAX] / 2</code> bytes long, this
/// function might panic by trying to allocate a vector with a capacity greater
/// than [`isize::MAX`] bytes.
#[inline]
pub(crate) fn encode_into(value: &str, flavor: Flavor, buf: &mut Vec<u8>) {
    let capacity = value.len().checked_mul(2).unwrap_or(ISIZE_MAX_USIZE);
    buf.reserve(capacity);

    let bytes = value.as_bytes();
    let mut index = 0;

    while index < bytes.len() {
        // SAFETY: We know that `index` is less than `bytes.len()`. This is
        // surprisingly significantly faster than a while let loop.
        let first = unsafe { *bytes.get_unchecked(index) };

        // PERF: While it's tempting to use a match statement here, converting
        // this logic to the if statement you see here (without merging 2 and 3)
        // improved performance by over 80%.
        if first <= 0x7f {
            if flavor == Flavor::Mutf8 && first == 0x00 {
                buf.extend_from_slice(&[0xc0, 0x80]);
            } else {
                buf.push(first);
            }

            index += 1;
        } else if first <= 0xdf {
            // SAFETY: We know that `bytes` is a valid UTF-8 string, so the
            // slice is guaranteed to be valid.
            let slice = unsafe { bytes.get_unchecked(index..index + 2) };
            buf.extend_from_slice(slice);
            index += 2;
        } else if first <= 0xef {
            // SAFETY: We know that `bytes` is a valid UTF-8 string, so the
            // slice is guaranteed to be valid.
            let slice = unsafe { bytes.get_unchecked(index..index + 3) };
            buf.extend_from_slice(slice);
            index += 3;
        } else {
            // SAFETY: We know that `bytes` is a valid UTF-8 string, so the
            // slice is guaranteed to be valid.
            let slice = unsafe { bytes.get_unchecked(index..index + 4) };

            let &[first, second, third, fourth] = slice else {
                // SAFETY: We know that the slice is exactly four bytes.
                unsafe { hint::unreachable_unchecked() };
            };

            let code_point = (u32::from(first & 0b0000_0111) << 18)
                | (u32::from(second & 0b0011_1111) << 12)
                | (u32::from(third & 0b0011_1111) << 6)
                | u32::from(fourth & 0b0011_1111);

            let [s1, s2] = to_surrogate_pair(code_point);
            buf.extend_from_slice(&encode_surrogate(s1));
            buf.extend_from_slice(&encode_surrogate(s2));
            index += 4;
        }
    }
}

#[must_use]
#[inline]
fn to_surrogate_pair(code_point: u32) -> [u16; 2] {
    let code_point = code_point - 0x10000;
    #[allow(clippy::cast_possible_truncation)]
    let high = ((code_point >> 10) as u16) | 0xd800;
    let low = ((code_point & 0x3ff) as u16) | 0xdc00;
    [high, low]
}

#[must_use]
#[inline]
fn encode_surrogate(surrogate: u16) -> [u8; 3] {
    [
        0b1110_0000 | ((surrogate & 0b1111_0000_0000_0000) >> 12) as u8,
        0b1000_0000 | ((surrogate & 0b0000_1111_1100_0000) >> 6) as u8,
        0b1000_0000 | ((surrogate & 0b0000_0000_0011_1111) as u8),
    ]
}

const ISIZE_MAX_USIZE: usize = isize::MAX as usize;
