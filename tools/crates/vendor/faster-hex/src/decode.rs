use crate::error::Error;

#[cfg(target_arch = "aarch64")]
pub(crate) mod aarch64;
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
pub(crate) mod x86;

const NIL: u8 = u8::MAX;

const fn init_unhex_array(check_case: CheckCase) -> [u8; 256] {
    let mut arr = [0; 256];
    let mut i = 0;
    while i < 256 {
        arr[i] = match i as u8 {
            b'0'..=b'9' => i as u8 - b'0',
            b'a'..=b'f' => match check_case {
                CheckCase::Lower | CheckCase::None => i as u8 - b'a' + 10,
                _ => NIL,
            },
            b'A'..=b'F' => match check_case {
                CheckCase::Upper | CheckCase::None => i as u8 - b'A' + 10,
                _ => NIL,
            },
            _ => NIL,
        };
        i += 1;
    }
    arr
}

const fn init_unhex4_array(check_case: CheckCase) -> [u8; 256] {
    let unhex_arr = init_unhex_array(check_case);

    let mut unhex4_arr = [NIL; 256];
    let mut i = 0;
    while i < 256 {
        if unhex_arr[i] != NIL {
            unhex4_arr[i] = unhex_arr[i] << 4;
        }
        i += 1;
    }
    unhex4_arr
}

// ASCII -> hex
static UNHEX: [u8; 256] = init_unhex_array(CheckCase::None);

// ASCII -> hex, lower case
static UNHEX_LOWER: [u8; 256] = init_unhex_array(CheckCase::Lower);

// ASCII -> hex, upper case
static UNHEX_UPPER: [u8; 256] = init_unhex_array(CheckCase::Upper);

// ASCII -> hex << 4
static UNHEX4: [u8; 256] = init_unhex4_array(CheckCase::None);

/// Returns whether every byte is an ASCII hex digit, accepting either letter case.
///
/// Accepts `0` through `9`, `a` through `f`, and `A` through `F`, including mixed
/// case. Prefixes, whitespace and non-ASCII bytes are rejected. This is a
/// character-only check: empty and odd-length inputs can pass. It does not
/// allocate or modify the input.
///
/// Use [`hex_decode`] to also require complete byte pairs. There is no need to
/// call this function before a checked decoder: decoding already validates input.
///
/// # Examples
///
/// ```
/// use faster_hex::hex_check;
///
/// assert!(hex_check(b"00aBcD"));
/// assert!(hex_check(b"a")); // Valid characters, but not a complete byte pair.
/// assert!(hex_check(b""));
/// assert!(!hex_check(b"0x01"));
/// assert!(!hex_check(b"00 01"));
/// ```
#[inline]
pub fn hex_check(src: &[u8]) -> bool {
    hex_check_with_case(src, CheckCase::None)
}

/// Checks ASCII hex digits against an explicit letter-case policy.
///
/// Digits are accepted under every [`CheckCase`] policy. Empty input succeeds;
/// odd lengths are allowed. Like [`hex_check`], this checks characters only and
/// neither allocates nor modifies the input.
///
/// # Examples
///
/// ```
/// use faster_hex::{hex_check_with_case, CheckCase};
///
/// assert!(hex_check_with_case(b"0123", CheckCase::Upper));
/// assert!(hex_check_with_case(b"ab01", CheckCase::Lower));
/// assert!(!hex_check_with_case(b"AB01", CheckCase::Lower));
/// assert!(hex_check_with_case(b"aB01", CheckCase::None));
/// ```
#[inline]
pub fn hex_check_with_case(src: &[u8], check_case: CheckCase) -> bool {
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    {
        match crate::vectorization_support() {
            crate::Vectorization::AVX512 => {
                // SAFETY: Dispatch checks AVX-512BW and its OS state.
                unsafe { x86::hex_check_avx512_with_case(src, check_case) }
            }
            crate::Vectorization::AVX2 => {
                // SAFETY: Dispatch guarantees AVX2; the checker bounds every load.
                unsafe { x86::hex_check_avx2_with_case(src, check_case) }
            }
            crate::Vectorization::SSE41 => {
                // SAFETY: Dispatch checks SSE4.1; the checker bounds every load.
                unsafe { x86::hex_check_sse_with_case(src, check_case) }
            }
            crate::Vectorization::None => hex_check_fallback_with_case(src, check_case),
        }
    }

    #[cfg(target_arch = "aarch64")]
    {
        match crate::vectorization_support() {
            crate::Vectorization::Neon => {
                // SAFETY: Dispatch guarantees NEON; the checker bounds every load.
                unsafe { aarch64::hex_check_neon_with_case(src, check_case) }
            }
            crate::Vectorization::None => hex_check_fallback_with_case(src, check_case),
        }
    }

    #[cfg(not(any(target_arch = "x86", target_arch = "x86_64", target_arch = "aarch64")))]
    hex_check_fallback_with_case(src, check_case)
}

/// Check if the input is valid hex bytes slice with case check
pub(crate) fn hex_check_fallback_with_case(src: &[u8], check_case: CheckCase) -> bool {
    match check_case {
        CheckCase::None => src.iter().all(|&x| UNHEX[x as usize] != NIL),
        CheckCase::Lower => src.iter().all(|&x| UNHEX_LOWER[x as usize] != NIL),
        CheckCase::Upper => src.iter().all(|&x| UNHEX_UPPER[x as usize] != NIL),
    }
}

/// Which ASCII letter cases are accepted when checking or decoding hex.
///
/// [`None`](Self::None) is the default and accepts mixed case; it does not
/// disable character validation. All policies accept ASCII digits `0` through
/// `9` and reject prefixes, whitespace and non-ASCII characters. They do not
/// change the length rules of the operation using them.
///
/// Encoding selects lowercase or uppercase through separate functions, so an
/// encoding operation never needs this policy.
///
/// # Examples
///
/// ```
/// use faster_hex::{hex_decode_with_case, CheckCase};
///
/// let mut bytes = [0; 2];
/// assert_eq!(hex_decode_with_case(b"AB01", &mut bytes, CheckCase::Upper)?,
///            &[0xab, 1]);
/// assert!(hex_decode_with_case(b"ab01", &mut bytes, CheckCase::Upper).is_err());
/// # Ok::<(), faster_hex::Error>(())
/// ```
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Hash)]
#[cfg_attr(feature = "defmt-03", derive(defmt::Format))]
pub enum CheckCase {
    /// Accept uppercase and lowercase, including a mixture of both.
    #[default]
    None,
    /// Accept digits and lowercase `a` through `f` only.
    Lower,
    /// Accept digits and uppercase `A` through `F` only.
    Upper,
}

/// Decodes all of `src` into `dst` without allocation, accepting either letter case.
///
/// `src` must contain an even number of ASCII hex digits, without a prefix,
/// whitespace or separators. Uppercase and lowercase digits may be mixed. Leading
/// zeroes are preserved as bytes; this converts a byte sequence, not an integer.
///
/// `dst` must have at least `src.len() / 2` bytes. The returned mutable slice
/// covers exactly the written prefix and borrows only `dst`. Spare destination
/// bytes remain unchanged. Empty input returns an empty slice and changes nothing.
///
/// Use [`hex_decode_with_case`] to restrict letter case, or [`hex_decode_array`]
/// when the decoded size must match a fixed array exactly.
///
/// # Errors
///
/// Errors are checked in this order:
///
/// 1. [`Error::OddLength`] if the input has an odd number of bytes.
/// 2. [`Error::OutputTooSmall`] if the destination is too small. `required` counts
///    the full decoded output size in bytes.
/// 3. [`Error::InvalidChar`] for the first invalid input byte. `index` is a
///    zero-based byte offset in `src`, not a Unicode character position.
///
/// Every error leaves the **entire destination unchanged**, including when an
/// invalid byte occurs after a long valid prefix. A short destination never
/// causes silent prefix decoding; slice `src` explicitly if that is intended.
///
/// # Examples
///
/// ```
/// use faster_hex::hex_decode;
///
/// let mut destination = [0xa5; 5];
/// let bytes = {
///     let source = *b"00aBcD";
///     hex_decode(&source, &mut destination)?
/// }; // The source is no longer needed.
/// assert_eq!(bytes, &[0, 0xab, 0xcd]);
/// bytes[0] = 0xff;
/// assert_eq!(destination, [0xff, 0xab, 0xcd, 0xa5, 0xa5]);
/// # Ok::<(), faster_hex::Error>(())
/// ```
///
/// Invalid input does not commit a partial result:
///
/// ```
/// use faster_hex::{hex_decode, Error};
///
/// let mut destination = [0xa5; 3];
/// assert!(matches!(hex_decode(b"00ff0g", &mut destination),
///     Err(Error::InvalidChar { index: 5, byte: b'g', .. })));
/// assert_eq!(destination, [0xa5; 3]);
/// ```
#[inline]
pub fn hex_decode<'a>(src: &[u8], dst: &'a mut [u8]) -> Result<&'a mut [u8], Error> {
    hex_decode_with_case(src, dst, CheckCase::None)
}

/// Decodes all of `src` into `dst` using an explicit letter-case policy.
///
/// The size, borrowing and destination-preservation guarantees are identical to
/// [`hex_decode`]. Digits are accepted under every policy; [`CheckCase::None`]
/// also accepts mixed-case letters. No allocation is performed.
///
/// # Errors
///
/// Returns the same errors in the same order as [`hex_decode`]. A letter rejected
/// by `check_case` is an [`Error::InvalidChar`], with its byte and zero-based
/// position in `src`. The first invalid byte wins, including when it is a case
/// violation before another non-hex byte. The entire destination remains unchanged.
///
/// # Examples
///
/// ```
/// use faster_hex::{hex_decode_with_case, CheckCase, Error};
///
/// let mut bytes = [0; 2];
/// hex_decode_with_case(b"ab01", &mut bytes, CheckCase::Lower)?;
/// assert_eq!(bytes, [0xab, 1]);
/// assert!(matches!(hex_decode_with_case(b"Ab01", &mut bytes, CheckCase::Lower),
///     Err(Error::InvalidChar { index: 0, byte: b'A', .. })));
/// assert_eq!(bytes, [0xab, 1]);
/// # Ok::<(), Error>(())
/// ```
#[inline(always)]
pub fn hex_decode_with_case<'a>(
    src: &[u8],
    dst: &'a mut [u8],
    check_case: CheckCase,
) -> Result<&'a mut [u8], Error> {
    if !src.len().is_multiple_of(2) {
        return Err(Error::OddLength);
    }
    let len = src.len() / 2;
    let dst = dst
        .get_mut(..len)
        .ok_or(Error::OutputTooSmall { required: len })?;
    if decode_checked(src, dst, check_case).is_err() {
        decode_diagnosed(src, dst, check_case)?;
    }
    Ok(dst)
}

/// Decodes exactly `N` bytes into an array without allocation, accepting either case.
///
/// The input grammar is the same as [`hex_decode`], but the decoded length must
/// equal `N`: both shorter and longer inputs are rejected. The result owns its
/// bytes independently of the input. Only `N == 0` accepts empty input.
///
/// Use [`hex_decode_array_with_case`] for a strict letter-case policy, or
/// [`hex_decode`] to write into a slice with spare capacity.
///
/// # Errors
///
/// Checks [`Error::OddLength`] first, [`Error::LengthMismatch`] second, and
/// [`Error::InvalidChar`] last. The mismatch's `expected` and `actual` fields
/// both count decoded bytes; character positions count source bytes. Prefixes,
/// whitespace and separators are rejected as invalid characters.
///
/// # Examples
///
/// ```
/// use faster_hex::hex_decode_array;
/// let bytes = hex_decode_array::<4>(b"0001aBff")?;
/// assert_eq!(bytes, [0, 1, 0xab, 0xff]);
/// assert!(hex_decode_array::<4>(b"0001").is_err());
/// assert_eq!(hex_decode_array::<0>(b"")?, []);
/// # Ok::<(), faster_hex::Error>(())
/// ```
#[inline]
pub fn hex_decode_array<const N: usize>(src: &[u8]) -> Result<[u8; N], Error> {
    hex_decode_array_with_case(src, CheckCase::None)
}

/// Decodes exactly `N` bytes into an array using an explicit letter-case policy.
///
/// This has the ownership, exact-length and allocation-free guarantees of
/// [`hex_decode_array`]. Only `N == 0` accepts empty input.
///
/// # Errors
///
/// Returns [`Error::OddLength`], [`Error::LengthMismatch`], then
/// [`Error::InvalidChar`] in that order. A disallowed letter case is an invalid
/// character. Mismatched lengths count decoded bytes; invalid-character indexes
/// count source bytes.
///
/// # Examples
///
/// ```
/// use faster_hex::{hex_decode_array_with_case, CheckCase};
///
/// let id = hex_decode_array_with_case::<2>(b"AB01", CheckCase::Upper)?;
/// assert_eq!(id, [0xab, 1]);
/// assert!(hex_decode_array_with_case::<2>(b"ab01", CheckCase::Upper).is_err());
/// # Ok::<(), faster_hex::Error>(())
/// ```
#[inline]
pub fn hex_decode_array_with_case<const N: usize>(
    src: &[u8],
    check_case: CheckCase,
) -> Result<[u8; N], Error> {
    if !src.len().is_multiple_of(2) {
        return Err(Error::OddLength);
    }
    let actual = src.len() / 2;
    if actual != N {
        return Err(Error::LengthMismatch {
            expected: N,
            actual,
        });
    }
    let mut bytes = [0; N];
    #[cfg(target_arch = "aarch64")]
    if N == 4 && crate::vectorization_support() == crate::Vectorization::Neon {
        // A fixed short array can inline the existing NEON block by padding
        // with valid digits, without reading beyond the source slice.
        let mut input = [b'0'; 16];
        input[..src.len()].copy_from_slice(src);
        let mut output = [0; 8];
        hex_decode_with_case(&input, &mut output, check_case)?;
        bytes.copy_from_slice(&output[..N]);
        return Ok(bytes);
    }
    if N > OWNED_DECODE_THRESHOLD {
        decode_owned_large(src, &mut bytes, check_case)?;
    } else {
        hex_decode_with_case(src, &mut bytes, check_case)?;
    }
    Ok(bytes)
}

/// Decodes the complete input into a new vector, accepting either letter case.
///
/// Available with `alloc`. The result owns exactly `src.len() / 2` decoded bytes,
/// independently of the input. Empty input produces an empty vector. The same
/// strict ASCII grammar as [`hex_decode`] applies: no prefixes, whitespace or
/// separators. Use [`hex_decode_vec_with_case`] to restrict letter case.
///
/// # Errors
///
/// Returns [`Error::OddLength`] for odd input, before allocating output. Even
/// input is checked for [`Error::InvalidChar`] after allocation, so malformed
/// even-length input can allocate before its first invalid byte is reported.
///
/// There is no input-size limit or recoverable allocation-error result. The
/// vector uses the allocator's normal error handling. Apply an application limit
/// before calling when necessary, or use [`hex_decode`] with reusable storage.
///
/// # Examples
///
/// ```
/// assert_eq!(faster_hex::hex_decode_vec(b"00aBff")?, [0, 0xab, 0xff]);
/// assert!(faster_hex::hex_decode_vec(b"0x01").is_err());
/// # Ok::<(), faster_hex::Error>(())
/// ```
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
#[inline]
pub fn hex_decode_vec(src: &[u8]) -> Result<alloc::vec::Vec<u8>, Error> {
    hex_decode_vec_with_case(src, CheckCase::None)
}

/// Decodes the complete input into a new vector using a letter-case policy.
///
/// Available with `alloc`. Ownership, grammar and allocation behavior are the
/// same as [`hex_decode_vec`]. Digits are accepted under every policy.
///
/// # Errors
///
/// Returns [`Error::OddLength`] before allocation, then [`Error::InvalidChar`]
/// for the first invalid byte or disallowed letter. As with [`hex_decode_vec`],
/// allocation failure is not returned as a codec error and no size limit is imposed.
///
/// # Examples
///
/// ```
/// use faster_hex::{hex_decode_vec_with_case, CheckCase};
///
/// assert_eq!(hex_decode_vec_with_case(b"AB01", CheckCase::Upper)?, [0xab, 1]);
/// assert!(hex_decode_vec_with_case(b"ab01", CheckCase::Upper).is_err());
/// # Ok::<(), faster_hex::Error>(())
/// ```
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
#[inline]
pub fn hex_decode_vec_with_case(
    src: &[u8],
    check_case: CheckCase,
) -> Result<alloc::vec::Vec<u8>, Error> {
    if !src.len().is_multiple_of(2) {
        return Err(Error::OddLength);
    }
    let mut bytes = alloc::vec![0; src.len() / 2];
    if bytes.len() > OWNED_DECODE_THRESHOLD {
        decode_owned_large(src, &mut bytes, check_case)?;
    } else {
        hex_decode_with_case(src, &mut bytes, check_case)?;
    }
    Ok(bytes)
}

// Small outputs are sensitive to streaming setup and call-site inlining.
const OWNED_DECODE_THRESHOLD: usize = 1024;

// Owned callers discard this initialized, exact-size buffer on error. Large
// inputs can commit one checked block at a time without re-reading valid input.
// Inline dispatch to preserve array return/copy code generation. The SIMD block
// loops remain in their target-feature functions.
#[inline(always)]
fn decode_owned_large(src: &[u8], dst: &mut [u8], case: CheckCase) -> Result<(), Error> {
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    let decoded = match crate::vectorization_support() {
        crate::Vectorization::AVX512 => {
            // SAFETY: Dispatch establishes AVX-512BW and OS support; the slices
            // have the exact 2:1 ratio, and output is private until success.
            unsafe { x86::hex_decode_avx512_owned(src, dst, case) }
        }
        crate::Vectorization::AVX2 => {
            // SAFETY: Dispatch establishes AVX2 and the same slice invariant.
            unsafe { x86::hex_decode_avx2_owned(src, dst, case) }
        }
        _ => decode_checked(src, dst, case),
    };
    #[cfg(target_arch = "aarch64")]
    let decoded = if crate::vectorization_support() == crate::Vectorization::Neon {
        // SAFETY: NEON is available and the slices have the exact 2:1 ratio.
        unsafe { aarch64::hex_decode_neon_owned(src, dst, case) }
    } else {
        decode_checked(src, dst, case)
    };
    #[cfg(not(any(target_arch = "x86", target_arch = "x86_64", target_arch = "aarch64")))]
    let decoded = decode_checked(src, dst, case);
    if decoded.is_err() {
        decode_diagnosed(src, dst, case)?;
    }
    Ok(())
}

// Call after establishing even input and the exact 2:1 source/output ratio.
// Every path validates the complete input before writing; diagnostics stay at
// the public boundary. Tests use the same operation after asserting its lengths.
// Inline the boundary and dispatch together so short decodes can keep their
// result and SIMD constants in the caller instead of a separate stack frame.
#[inline(always)]
pub(crate) fn decode_checked(src: &[u8], dst: &mut [u8], check_case: CheckCase) -> Result<(), ()> {
    if dst.len() < 8 {
        return hex_decode_short_scalar(src, dst, check_case);
    }
    #[cfg(target_arch = "aarch64")]
    let len = dst.len();
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    {
        match crate::vectorization_support() {
            crate::Vectorization::AVX512 => {
                // SAFETY: Dispatch checked AVX2 and AVX-512BW with its OS state;
                // dst has exactly src.len() / 2 bytes.
                unsafe { x86::hex_decode_avx512_checked(src, dst, check_case) }
            }
            crate::Vectorization::AVX2 => {
                // SAFETY: AVX2 is available and the slices have the exact 2:1 ratio.
                unsafe { x86::hex_decode_avx2_checked(src, dst, check_case) }
            }
            crate::Vectorization::SSE41 => {
                // SAFETY: SSE4.1 is available and the slice lengths have the exact ratio.
                unsafe { x86::hex_decode_sse41_checked(src, dst, check_case) }
            }
            crate::Vectorization::None => {
                if !hex_check_fallback_with_case(src, check_case) {
                    return Err(());
                }
                hex_decode_fallback(src, dst);
                Ok(())
            }
        }
    }

    #[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
    {
        #[cfg(target_arch = "aarch64")]
        if (17..=32).contains(&len) && crate::vectorization_support() == crate::Vectorization::Neon
        {
            // SAFETY: NEON is available; src has 34..=64 bytes and dst is its exact half.
            return unsafe { aarch64::hex_decode_bounded_neon(src, dst, check_case) };
        }

        #[cfg(target_arch = "aarch64")]
        if (8..=16).contains(&len) && crate::vectorization_support() == crate::Vectorization::Neon {
            // SAFETY: Each complete 16-byte load and 8-byte store stays in its slice.
            return unsafe { aarch64::hex_decode_short_neon(src, dst, check_case) };
        }
        if !hex_check_with_case(src, check_case) {
            return Err(());
        }
        hex_decode_unchecked(src, dst);
        Ok(())
    }
}

// Fewer than eight output bytes fit in one word. Decode each pair once
// and commit only after the complete input has passed validation.
#[inline]
pub(crate) fn hex_decode_short_scalar(
    src: &[u8],
    dst: &mut [u8],
    case: CheckCase,
) -> Result<(), ()> {
    let table = match case {
        CheckCase::None => &UNHEX,
        CheckCase::Lower => &UNHEX_LOWER,
        CheckCase::Upper => &UNHEX_UPPER,
    };
    let mut decoded = 0u64;
    for pair in src.as_chunks::<2>().0 {
        let high = table[usize::from(pair[0])];
        let low = table[usize::from(pair[1])];
        if high | low == NIL {
            return Err(());
        }
        decoded = decoded << 8 | u64::from(high << 4 | low);
    }
    // The word is built in input order, so its low byte belongs in the last slot.
    for slot in dst.iter_mut().rev() {
        *slot = decoded as u8;
        decoded >>= 8;
    }
    Ok(())
}

// Backends report validity; diagnostics belong to the public boundary. The
// successful SIMD path does not track byte positions or construct an error.
#[cold]
fn decode_diagnosed(src: &[u8], dst: &mut [u8], case: CheckCase) -> Result<(), Error> {
    let table = match case {
        CheckCase::None => &UNHEX,
        CheckCase::Lower => &UNHEX_LOWER,
        CheckCase::Upper => &UNHEX_UPPER,
    };
    // Skip valid blocks so locating a late error retains SIMD throughput.
    // Short inputs go directly to the byte scan.
    let skipped = if src.len() > 64 {
        src.chunks_exact(64)
            .take_while(|chunk| hex_check_with_case(chunk, case))
            .count()
            * 64
    } else {
        0
    };
    for (offset, &byte) in src[skipped..].iter().enumerate() {
        let index = skipped + offset;
        if table[usize::from(byte)] == NIL {
            return Err(Error::InvalidChar { index, byte });
        }
    }
    // Every byte was validated. A scalar conversion also handles a backend
    // rejecting valid input; never return an unwritten output prefix.
    hex_decode_fallback(src, dst);
    Ok(())
}

// Internal conversion only. Even if miscalled with short input, never pass an
// undersized source to a SIMD kernel. Public callers must use checked decode.
#[cfg(any(test, not(any(target_arch = "x86", target_arch = "x86_64"))))]
pub(crate) fn hex_decode_unchecked(src: &[u8], dst: &mut [u8]) {
    let len = core::cmp::min(src.len() / 2, dst.len());
    let src = &src[..len * 2];
    let dst = &mut dst[..len];
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    {
        match crate::vectorization_support() {
            crate::Vectorization::AVX512 => {
                // SAFETY: Dispatch checks AVX-512BW; the slices have a 2:1 length ratio.
                unsafe { x86::hex_decode_avx512(src, dst) }
            }
            crate::Vectorization::AVX2 => {
                // SAFETY: Dispatch guarantees AVX2 and the slices have a 2:1 length ratio.
                unsafe { x86::hex_decode_avx2(src, dst) }
            }
            crate::Vectorization::SSE41 => {
                // SAFETY: Dispatch guarantees SSE4.1 and the slices have a 2:1 length ratio.
                unsafe { x86::hex_decode_sse41(src, dst) }
            }
            crate::Vectorization::None => hex_decode_fallback(src, dst),
        }
    }
    #[cfg(target_arch = "aarch64")]
    match crate::vectorization_support() {
        crate::Vectorization::Neon => {
            // SAFETY: Dispatch guarantees NEON; both slices have the exact 2:1 ratio.
            unsafe { aarch64::hex_decode_neon(src, dst) }
        }
        crate::Vectorization::None => hex_decode_fallback(src, dst),
    }
    #[cfg(not(any(target_arch = "x86", target_arch = "x86_64", target_arch = "aarch64")))]
    hex_decode_fallback(src, dst);
}

#[inline]
pub(crate) fn hex_decode_fallback(src: &[u8], dst: &mut [u8]) {
    for (slot, bytes) in dst.iter_mut().zip(src.chunks_exact(2)) {
        let a = UNHEX4[bytes[0] as usize];
        let b = UNHEX[bytes[1] as usize];
        *slot = a | b;
    }
}
