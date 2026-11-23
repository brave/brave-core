// SIMD encoding includes work derived from stdsimd under the MIT license.
// See LICENSE-THIRD-PARTY/Rust Project Developers.

#[cfg(target_arch = "x86")]
use core::arch::x86::*;
#[cfg(target_arch = "x86_64")]
use core::arch::x86_64::*;

#[cfg(target_arch = "aarch64")]
use core::arch::aarch64::*;

#[cfg(feature = "alloc")]
use alloc::{string::String, vec::Vec};

use core::mem::MaybeUninit;

use crate::error::Error;

// SIMD callers establish CPU/OS support and exactly twice the source length
// in destination space. Kernels initialize every output byte as ASCII; only
// encode() exposes the initialized prefix as a string.
const TABLE_LOWER: &[u8; 16] = b"0123456789abcdef";
const TABLE_UPPER: &[u8; 16] = b"0123456789ABCDEF";

#[cfg(feature = "alloc")]
#[inline]
fn hex_string_custom_case(src: &[u8], upper_case: bool) -> String {
    let len = src.len().checked_mul(2).expect("encoded length overflow");
    let mut buffer = Vec::with_capacity(len);
    encode(src, buffer.spare_capacity_mut(), upper_case).expect("capacity reserved for encoding");
    // SAFETY: encode initialized the first len spare bytes with ASCII. The length
    // is committed only after encoding returns, so unwinding never exposes it.
    unsafe {
        buffer.set_len(len);
        String::from_utf8_unchecked(buffer)
    }
}

/// Encodes `src` as an owned lowercase hexadecimal string.
///
/// Each byte produces two ASCII digits, including leading zeroes. The result has
/// length `src.len() * 2`, contains no prefix or separators, and owns its storage
/// independently of `src`. Empty input produces an empty string.
///
/// Available with the `alloc` feature. To reuse a string's capacity, use
/// [`hex_append`]. For caller-provided storage, use [`hex_encode`].
///
/// # Panics
///
/// Panics if the encoded length cannot be represented or exceeds `isize::MAX`
/// bytes. Allocation failure follows the allocator's error handling; it is not
/// returned as an [`Error`].
///
/// # Examples
///
/// ```
/// use faster_hex::hex_string;
///
/// assert_eq!(hex_string(&[0, 0xab, 0xff]), "00abff");
/// assert_eq!(hex_string(&[]), "");
/// ```
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
#[inline]
pub fn hex_string(src: &[u8]) -> String {
    hex_string_custom_case(src, false)
}

/// Encodes `src` as an owned uppercase hexadecimal string.
///
/// This is the uppercase counterpart of [`hex_string`]: each byte produces two
/// ASCII digits without a prefix or separators. Available with `alloc`.
///
/// # Panics
///
/// Panics on the same capacity overflows as [`hex_string`]. Allocation failure
/// follows the allocator's error handling.
///
/// # Examples
///
/// ```
/// assert_eq!(faster_hex::hex_string_upper(&[0, 0xab, 0xff]), "00ABFF");
/// ```
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
#[inline]
pub fn hex_string_upper(src: &[u8]) -> String {
    hex_string_custom_case(src, true)
}

#[inline]
pub(crate) fn hex_encode_custom<'a>(
    src: &[u8],
    dst: &'a mut [u8],
    upper_case: bool,
) -> Result<&'a mut str, Error> {
    // SAFETY: MaybeUninit has the same layout as u8. encode only writes initialized
    // ASCII bytes; it never makes an existing byte uninitialized, including on error.
    let output = unsafe { core::slice::from_raw_parts_mut(dst.as_mut_ptr().cast(), dst.len()) };
    encode(src, output, upper_case)
}

// One boundary for caller-owned bytes, allocation spare capacity and formatting:
// validate the size, initialize the exact ASCII prefix, then return its string view.
#[inline]
pub(crate) fn encode<'a>(
    src: &[u8],
    dst: &'a mut [MaybeUninit<u8>],
    upper_case: bool,
) -> Result<&'a mut str, Error> {
    let len = src.len().checked_mul(2).ok_or(Error::Overflow)?;
    if dst.len() < len {
        return Err(Error::OutputTooSmall { required: len });
    }
    let dst = &mut dst[..len];
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    {
        match crate::vectorization_support() {
            crate::Vectorization::AVX2 | crate::Vectorization::AVX512 => {
                // SAFETY: Dispatch checked AVX2 and the OS register state.
                unsafe { hex_encode_avx2(src, dst, upper_case) }
            }
            crate::Vectorization::SSE41 => {
                // SAFETY: Dispatch checks SSE4.1; dst has twice src.len() bytes.
                unsafe { hex_encode_sse41(src, dst, upper_case) }
            }
            crate::Vectorization::None => hex_encode_custom_case_fallback(src, dst, upper_case),
        }
    }
    #[cfg(target_arch = "aarch64")]
    {
        if src.len() < 8 {
            hex_encode_pairs(src, dst, upper_case);
        } else {
            match crate::vectorization_support() {
                crate::Vectorization::Neon => {
                    // SAFETY: Dispatch checks NEON; dst has twice src.len() bytes.
                    unsafe { hex_encode_neon(src, dst, upper_case) }
                }
                crate::Vectorization::None => hex_encode_custom_case_fallback(src, dst, upper_case),
            }
        }
    }
    #[cfg(not(any(target_arch = "x86", target_arch = "x86_64", target_arch = "aarch64")))]
    {
        hex_encode_custom_case_fallback(src, dst, upper_case);
    }
    // SAFETY: Every backend initialized all len elements with ASCII, using the
    // exact 1:2 length ratio established above. MaybeUninit<u8> has u8's layout,
    // and ASCII is valid UTF-8. Spare capacity is excluded from this string.
    Ok(unsafe {
        core::str::from_utf8_unchecked_mut(core::slice::from_raw_parts_mut(
            dst.as_mut_ptr().cast(),
            len,
        ))
    })
}

/// Encodes all of `src` as lowercase hex into `dst` without allocation.
///
/// Every input byte produces two ASCII digits. `dst` must have room for at least
/// `src.len() * 2` bytes; extra capacity is allowed and remains unchanged. The
/// returned string covers only the written prefix and borrows only `dst`, so
/// `src` can be dropped or reused while the result is still in use.
///
/// No prefix or separators are written. Empty input returns an empty string and
/// leaves `dst` unchanged. For uppercase digits, use [`hex_encode_upper`].
///
/// # Errors
///
/// Returns [`Error::Overflow`] if the encoded length cannot be represented as a
/// `usize`, or [`Error::OutputTooSmall`] if `dst` is too short. Its `required`
/// field is the full output size in bytes. The entire destination is unchanged
/// on either error; a short buffer is never partially filled.
///
/// # Examples
///
/// ```
/// use faster_hex::hex_encode;
///
/// let mut destination = [0xff; 8];
/// let text = hex_encode(&[0, 0xab, 0xcd], &mut destination)?;
/// assert_eq!(text, "00abcd");
/// // Only the returned prefix is modified.
/// text.make_ascii_uppercase();
/// assert_eq!(&destination[..6], b"00ABCD");
/// assert_eq!(&destination[6..], &[0xff; 2]);
/// # Ok::<(), faster_hex::Error>(())
/// ```
///
/// An insufficient destination is reported without modifying it:
///
/// ```
/// use faster_hex::{hex_encode, Error};
///
/// let mut destination = [0xa5; 3];
/// assert!(matches!(hex_encode(&[0xab, 0xcd], &mut destination),
///     Err(Error::OutputTooSmall { required: 4, .. })));
/// assert_eq!(destination, [0xa5; 3]);
/// ```
#[inline]
pub fn hex_encode<'a>(src: &[u8], dst: &'a mut [u8]) -> Result<&'a mut str, Error> {
    hex_encode_custom(src, dst, false)
}

/// Encodes all of `src` as uppercase hex into `dst` without allocation.
///
/// This has the same length, borrowing and destination-preservation guarantees
/// as [`hex_encode`], using `A` through `F` instead of `a` through `f`.
///
/// # Errors
///
/// Returns [`Error::Overflow`] or [`Error::OutputTooSmall`] under the same
/// conditions as [`hex_encode`]. An error leaves the entire destination unchanged.
///
/// # Examples
///
/// ```
/// let mut destination = [0; 6];
/// assert_eq!(faster_hex::hex_encode_upper(&[0, 0xab, 0xff], &mut destination)?,
///            "00ABFF");
/// # Ok::<(), faster_hex::Error>(())
/// ```
#[inline]
pub fn hex_encode_upper<'a>(src: &[u8], dst: &'a mut [u8]) -> Result<&'a mut str, Error> {
    hex_encode_custom(src, dst, true)
}

/// Appends lowercase hexadecimal digits to `dst` and returns the appended suffix.
///
/// Existing text is preserved, including non-ASCII text. The returned mutable
/// string borrows only `dst` and contains exactly `src.len() * 2` new ASCII
/// digits. Empty input returns an empty suffix without changing the string.
///
/// Available with `alloc`. Existing capacity is reused without allocation when
/// sufficient; otherwise the string grows. Call [`String::clear`] first to
/// replace its contents while retaining the allocation. For a new string, use
/// [`hex_string`].
///
/// # Panics
///
/// Panics if the resulting length cannot be represented or exceeds `isize::MAX`
/// bytes. Allocation failure follows the allocator's error handling.
///
/// # Examples
///
/// ```
/// use faster_hex::hex_append;
///
/// let mut text = String::with_capacity(64);
/// text.push_str("hash: ");
/// assert_eq!(hex_append(&[0xab, 0xcd], &mut text), "abcd");
/// assert_eq!(text, "hash: abcd");
///
/// text.clear();
/// hex_append(&[0, 1], &mut text);
/// assert_eq!(text, "0001");
/// ```
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub fn hex_append<'a>(src: &[u8], dst: &'a mut String) -> &'a mut str {
    hex_append_custom(src, dst, false)
}

/// Appends uppercase hexadecimal digits to `dst` and returns the appended suffix.
///
/// Available with `alloc`. Existing content and capacity are handled as in
/// [`hex_append`]; only the new hex digits use uppercase letters.
///
/// # Panics
///
/// Panics on the same capacity overflows as [`hex_append`]. Allocation failure
/// follows the allocator's error handling.
///
/// # Examples
///
/// ```
/// let mut text = String::from("hash: ");
/// assert_eq!(faster_hex::hex_append_upper(&[0xab, 0xcd], &mut text), "ABCD");
/// assert_eq!(text, "hash: ABCD");
/// ```
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub fn hex_append_upper<'a>(src: &[u8], dst: &'a mut String) -> &'a mut str {
    hex_append_custom(src, dst, true)
}

#[cfg(feature = "alloc")]
fn hex_append_custom<'a>(src: &[u8], dst: &'a mut String, upper: bool) -> &'a mut str {
    let base = dst.len();
    let additional = src.len().checked_mul(2).expect("encoded length overflow");
    let len = base
        .checked_add(additional)
        .expect("encoded length overflow");
    dst.reserve(additional);
    // SAFETY: Only spare capacity beyond the original string is written.
    // encode fully initializes that suffix as ASCII before set_len commits it;
    // a panic before the commit leaves the original string valid and unchanged.
    unsafe {
        let bytes = dst.as_mut_vec();
        encode(src, bytes.spare_capacity_mut(), upper).expect("capacity reserved for encoding");
        bytes.set_len(len);
        core::str::from_utf8_unchecked_mut(&mut bytes[base..])
    }
}

// Each backend receives the exact output prefix sized by encode.
#[target_feature(enable = "avx2")]
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[inline]
pub(crate) unsafe fn hex_encode_avx2(src: &[u8], dst: &mut [MaybeUninit<u8>], upper_case: bool) {
    if src.len() < 32 {
        return hex_encode_sse41(src, dst, upper_case);
    }
    let table = if upper_case { TABLE_UPPER } else { TABLE_LOWER };
    let table = _mm256_broadcastsi128_si256(_mm_loadu_si128(table.as_ptr().cast()));
    let (blocks, tail) = src.as_chunks::<32>();
    for (input, output) in blocks.iter().zip(dst.as_chunks_mut::<64>().0) {
        encode_avx2_32(input, output, table);
    }
    if !tail.is_empty() {
        if let (Some(input), Some(output)) = (src.last_chunk::<32>(), dst.last_chunk_mut::<64>()) {
            encode_avx2_32(input, output, table);
        }
    }
}

#[inline]
#[target_feature(enable = "avx2")]
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
unsafe fn encode_avx2_32(src: &[u8; 32], dst: &mut [MaybeUninit<u8>; 64], table: __m256i) {
    let bytes = _mm256_loadu_si256(src.as_ptr().cast());
    let mask = _mm256_set1_epi8(15);
    let high = _mm256_and_si256(_mm256_srli_epi16::<4>(bytes), mask);
    let low = _mm256_and_si256(bytes, mask);
    let a = _mm256_unpacklo_epi8(high, low);
    let b = _mm256_unpackhi_epi8(high, low);
    _mm256_storeu_si256(
        dst.as_mut_ptr().cast(),
        _mm256_shuffle_epi8(table, _mm256_permute2x128_si256::<0x20>(a, b)),
    );
    _mm256_storeu_si256(
        dst.as_mut_ptr().add(32).cast(),
        _mm256_shuffle_epi8(table, _mm256_permute2x128_si256::<0x31>(a, b)),
    );
}

#[target_feature(enable = "sse4.1")]
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
pub(crate) unsafe fn hex_encode_sse41(src: &[u8], dst: &mut [MaybeUninit<u8>], upper_case: bool) {
    if src.len() < 16 {
        return hex_encode_custom_case_fallback(src, dst, upper_case);
    }
    let table = if upper_case { TABLE_UPPER } else { TABLE_LOWER };
    let table = _mm_loadu_si128(table.as_ptr().cast());
    let (blocks, tail) = src.as_chunks::<16>();
    for (input, output) in blocks.iter().zip(dst.as_chunks_mut::<32>().0) {
        encode_sse41_16(input, output, table);
    }
    if !tail.is_empty() {
        if let (Some(input), Some(output)) = (src.last_chunk::<16>(), dst.last_chunk_mut::<32>()) {
            encode_sse41_16(input, output, table);
        }
    }
}

#[inline]
#[target_feature(enable = "sse4.1")]
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
unsafe fn encode_sse41_16(src: &[u8; 16], dst: &mut [MaybeUninit<u8>; 32], table: __m128i) {
    let bytes = _mm_loadu_si128(src.as_ptr().cast());
    let mask = _mm_set1_epi8(15);
    let high = _mm_shuffle_epi8(table, _mm_and_si128(_mm_srli_epi16::<4>(bytes), mask));
    let low = _mm_shuffle_epi8(table, _mm_and_si128(bytes, mask));
    _mm_storeu_si128(dst.as_mut_ptr().cast(), _mm_unpacklo_epi8(high, low));
    _mm_storeu_si128(
        dst.as_mut_ptr().add(16).cast(),
        _mm_unpackhi_epi8(high, low),
    );
}

#[inline]
#[target_feature(enable = "neon")]
#[cfg(target_arch = "aarch64")]
pub(crate) unsafe fn hex_encode_neon(src: &[u8], dst: &mut [MaybeUninit<u8>], upper_case: bool) {
    if src.len() < 8 {
        return hex_encode_custom_case_fallback(src, dst, upper_case);
    }
    let table = if upper_case { TABLE_UPPER } else { TABLE_LOWER };
    let table = vld1q_u8(table.as_ptr());
    if src.len() < 16 {
        if let (Some(input), Some(output)) = (src.first_chunk::<8>(), dst.first_chunk_mut::<16>()) {
            encode_neon_8(input, output, table);
        }
        if src.len() > 8 {
            if let (Some(input), Some(output)) = (src.last_chunk::<8>(), dst.last_chunk_mut::<16>())
            {
                encode_neon_8(input, output, table);
            }
        }
        return;
    }
    let (batches, rest) = src.as_chunks::<64>();
    let (outputs, remaining) = dst.as_chunks_mut::<128>();
    for (input, output) in batches.iter().zip(outputs) {
        for (input, output) in input
            .as_chunks::<16>()
            .0
            .iter()
            .zip(output.as_chunks_mut::<32>().0)
        {
            encode_neon_16(input, output, table);
        }
    }
    let (blocks, tail) = rest.as_chunks::<16>();
    for (input, output) in blocks.iter().zip(remaining.as_chunks_mut::<32>().0) {
        encode_neon_16(input, output, table);
    }
    if !tail.is_empty() {
        // Re-encode the last complete block to cover a short tail. The overlap
        // writes the same bytes and never accesses outside either slice.
        match (src.last_chunk::<16>(), dst.last_chunk_mut::<32>()) {
            (Some(input), Some(output)) => encode_neon_16(input, output, table),
            _ => hex_encode_custom_case_fallback(src, dst, upper_case),
        }
    }
}

#[inline]
#[target_feature(enable = "neon")]
#[cfg(target_arch = "aarch64")]
unsafe fn encode_neon_16(src: &[u8; 16], dst: &mut [MaybeUninit<u8>; 32], table: uint8x16_t) {
    let bytes = vld1q_u8(src.as_ptr());
    let high = vqtbl1q_u8(table, vshrq_n_u8::<4>(bytes));
    let low = vqtbl1q_u8(table, vandq_u8(bytes, vdupq_n_u8(15)));
    vst2q_u8(dst.as_mut_ptr().cast(), uint8x16x2_t(high, low));
}

const fn encode_pairs(alphabet: &[u8; 16]) -> [[u8; 2]; 256] {
    let mut pairs = [[0; 2]; 256];
    let mut byte = 0;
    while byte < pairs.len() {
        pairs[byte] = [alphabet[byte >> 4], alphabet[byte & 15]];
        byte += 1;
    }
    pairs
}

static PAIRS_LOWER: [[u8; 2]; 256] = encode_pairs(TABLE_LOWER);
static PAIRS_UPPER: [[u8; 2]; 256] = encode_pairs(TABLE_UPPER);

pub(crate) fn hex_encode_custom_case_fallback(
    src: &[u8],
    dst: &mut [MaybeUninit<u8>],
    upper_case: bool,
) {
    // Arithmetic lets LLVM vectorize longer inputs on baseline SIMD targets.
    // Pair lookup avoids per-nibble branches for short or non-SIMD inputs.
    if cfg!(any(
        target_feature = "neon",
        target_feature = "sse2",
        target_feature = "simd128"
    )) && src.len() >= 32
    {
        let letter = if upper_case { b'A' - 10 } else { b'a' - 10 };
        let ascii = |nibble| nibble + if nibble < 10 { b'0' } else { letter };
        for (&byte, pair) in src.iter().zip(dst.chunks_exact_mut(2)) {
            pair[0].write(ascii(byte >> 4));
            pair[1].write(ascii(byte & 15));
        }
    } else {
        hex_encode_pairs(src, dst, upper_case);
    }
}

#[inline]
fn hex_encode_pairs(src: &[u8], dst: &mut [MaybeUninit<u8>], upper_case: bool) {
    let table = if upper_case {
        &PAIRS_UPPER
    } else {
        &PAIRS_LOWER
    };
    for (&byte, pair) in src.iter().zip(dst.as_chunks_mut::<2>().0) {
        *pair = table[byte as usize].map(MaybeUninit::new);
    }
}

#[cfg(test)]
pub(crate) fn hex_encode_fallback(src: &[u8], dst: &mut [u8], upper: bool) {
    // SAFETY: The scalar encoder only writes initialized ASCII to complete pairs;
    // untouched bytes retain their previous initialized values, even for short dst.
    let output = unsafe { core::slice::from_raw_parts_mut(dst.as_mut_ptr().cast(), dst.len()) };
    hex_encode_custom_case_fallback(src, output, upper);
}

#[inline]
#[target_feature(enable = "neon")]
#[cfg(target_arch = "aarch64")]
unsafe fn encode_neon_8(src: &[u8; 8], dst: &mut [MaybeUninit<u8>; 16], table: uint8x16_t) {
    let bytes = vcombine_u8(vld1_u8(src.as_ptr()), vdup_n_u8(0));
    let high = vqtbl1q_u8(table, vshrq_n_u8::<4>(bytes));
    let low = vqtbl1q_u8(table, vandq_u8(bytes, vdupq_n_u8(15)));
    vst1q_u8(dst.as_mut_ptr().cast(), vzip1q_u8(high, low));
}
