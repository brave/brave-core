//! SSE4.1, AVX2 and AVX-512 checking and decoding.
//!
//! Callers establish CPU/OS support. Checkers accept any source length.
//! Checked decoders require even input and exactly half as much output space;
//! they validate the complete input before writing. Raw decoders additionally
//! require valid hex input.

// SIMD decoding includes work derived from fast-hex under the MIT license.
// See https://github.com/zbjornson/fast-hex and LICENSE-THIRD-PARTY/fast-hex.
// ALSW lookup is adapted from vsimd; saturating nibble mapping from const-hex.
// See LICENSE-THIRD-PARTY/vsimd and LICENSE-THIRD-PARTY/const-hex.

#[cfg(target_arch = "x86")]
use core::arch::x86::*;
#[cfg(target_arch = "x86_64")]
use core::arch::x86_64::*;

use super::{hex_check_fallback_with_case, hex_decode_fallback, CheckCase};

// Both AVX widths repeat these 16-entry hash and decode tables in every lane.
#[inline]
#[target_feature(enable = "sse2")]
fn alsw_tables() -> (__m128i, __m128i) {
    let hash = _mm_setr_epi8(1, 1, 1, 1, 1, 1, 1, 11, 11, 11, 15, 15, 15, 15, 15, 15);
    let decode = _mm_setr_epi8(
        -128, -128, -128, -128, -48, -55, -128, -87, -128, -48, -128, -128, -128, -128, -128, -128,
    );
    (hash, decode)
}

// The ALSW hash separates digits, uppercase and lowercase ASCII ranges.
// Saturating addition with each range's negative start marks invalid bytes
// with a sign bit. Slots 5 and 7 select the uppercase and lowercase ranges.
// Non-ASCII bytes remain negative under signed saturating addition.
#[inline]
fn decode_check_offsets(case: CheckCase) -> &'static [i8; 16] {
    const MIXED: [i8; 16] = [
        -128, -128, -128, -128, -48, -65, -128, -97, -128, -55, -128, -128, -128, -128, -128, -128,
    ];
    const LOWER: [i8; 16] = {
        let mut offsets = MIXED;
        offsets[5] = -128;
        offsets
    };
    const UPPER: [i8; 16] = {
        let mut offsets = MIXED;
        offsets[7] = -128;
        offsets
    };
    match case {
        CheckCase::None => &MIXED,
        CheckCase::Lower => &LOWER,
        CheckCase::Upper => &UPPER,
    }
}

// Add a wrapping bias so the accepted unsigned ASCII interval starts at -128.
// A signed comparison then rejects both bytes below the interval and above it.
#[inline]
#[target_feature(enable = "sse4.1")]
unsafe fn valid_sse(bytes: __m128i, case: CheckCase) -> __m128i {
    let digit = _mm_cmpgt_epi8(_mm_set1_epi8(-118), _mm_add_epi8(bytes, _mm_set1_epi8(80)));
    let fold = if case == CheckCase::None { 0x20 } else { 0 };
    let first = if case == CheckCase::Upper { b'A' } else { b'a' };
    let letters = _mm_or_si128(bytes, _mm_set1_epi8(fold));
    let letter = _mm_cmpgt_epi8(
        _mm_set1_epi8(-122),
        _mm_add_epi8(letters, _mm_set1_epi8(128u8.wrapping_sub(first) as i8)),
    );
    _mm_or_si128(digit, letter)
}

#[target_feature(enable = "sse4.1")]
pub(crate) unsafe fn hex_check_sse_with_case(src: &[u8], case: CheckCase) -> bool {
    if src.len() < 16 {
        return hex_check_fallback_with_case(src, case);
    }
    let (blocks, tail) = src.as_chunks::<16>();
    for block in blocks {
        if _mm_movemask_epi8(valid_sse(_mm_loadu_si128(block.as_ptr().cast()), case)) != 0xffff {
            return false;
        }
    }
    if !tail.is_empty() {
        if let Some(last) = src.last_chunk::<16>() {
            return _mm_movemask_epi8(valid_sse(_mm_loadu_si128(last.as_ptr().cast()), case))
                == 0xffff;
        }
    }
    true
}

// Add a wrapping bias so the accepted unsigned ASCII interval starts at -128.
// A signed comparison then rejects both bytes below the interval and above it.
#[inline]
#[target_feature(enable = "avx2")]
unsafe fn valid_avx2(bytes: __m256i, case: CheckCase) -> __m256i {
    let digit = _mm256_cmpgt_epi8(
        _mm256_set1_epi8(-118),
        _mm256_add_epi8(bytes, _mm256_set1_epi8(80)),
    );
    let fold = if case == CheckCase::None { 0x20 } else { 0 };
    let first = if case == CheckCase::Upper { b'A' } else { b'a' };
    let letters = _mm256_or_si256(bytes, _mm256_set1_epi8(fold));
    let letter = _mm256_cmpgt_epi8(
        _mm256_set1_epi8(-122),
        _mm256_add_epi8(letters, _mm256_set1_epi8(128u8.wrapping_sub(first) as i8)),
    );
    _mm256_or_si256(digit, letter)
}

#[target_feature(enable = "avx2")]
#[inline]
pub(crate) unsafe fn hex_check_avx2_with_case(src: &[u8], case: CheckCase) -> bool {
    if src.len() < 32 {
        return hex_check_sse_with_case(src, case);
    }
    // Reduce four vectors together so long inputs need fewer mask tests.
    let (batches, rest) = src.as_chunks::<128>();
    for batch in batches {
        let mut valid = _mm256_set1_epi8(-1);
        for block in batch.as_chunks::<32>().0 {
            valid = _mm256_and_si256(
                valid,
                valid_avx2(_mm256_loadu_si256(block.as_ptr().cast()), case),
            );
        }
        if _mm256_movemask_epi8(valid) != -1 {
            return false;
        }
    }
    let (blocks, tail) = rest.as_chunks::<32>();
    let mut valid = _mm256_set1_epi8(-1);
    for block in blocks {
        valid = _mm256_and_si256(
            valid,
            valid_avx2(_mm256_loadu_si256(block.as_ptr().cast()), case),
        );
    }
    if !tail.is_empty() {
        if let Some(last) = src.last_chunk::<32>() {
            valid = _mm256_and_si256(
                valid,
                valid_avx2(_mm256_loadu_si256(last.as_ptr().cast()), case),
            );
        }
    }
    _mm256_movemask_epi8(valid) == -1
}

#[inline]
#[target_feature(enable = "avx512f,avx512bw")]
unsafe fn valid_avx512(bytes: __m512i, case: CheckCase) -> u64 {
    let digit = _mm512_cmple_epu8_mask(
        _mm512_sub_epi8(bytes, _mm512_set1_epi8(b'0' as i8)),
        _mm512_set1_epi8(9),
    );
    let fold = if case == CheckCase::None { 0x20 } else { 0 };
    let first = if case == CheckCase::Upper { b'A' } else { b'a' };
    let letters = _mm512_or_si512(bytes, _mm512_set1_epi8(fold));
    let letter = _mm512_cmple_epu8_mask(
        _mm512_sub_epi8(letters, _mm512_set1_epi8(first as i8)),
        _mm512_set1_epi8(5),
    );
    digit | letter
}

#[target_feature(enable = "avx512f,avx512bw")]
#[inline]
pub(crate) unsafe fn hex_check_avx512_with_case(src: &[u8], case: CheckCase) -> bool {
    if src.len() < 64 {
        return hex_check_avx2_with_case(src, case);
    }
    let (blocks, tail) = src.as_chunks::<64>();
    for block in blocks {
        if valid_avx512(_mm512_loadu_si512(block.as_ptr().cast()), case) != u64::MAX {
            return false;
        }
    }
    if !tail.is_empty() {
        if let Some(last) = src.last_chunk::<64>() {
            return valid_avx512(_mm512_loadu_si512(last.as_ptr().cast()), case) == u64::MAX;
        }
    }
    true
}

// Map valid ASCII to 0..=15 and every invalid byte to >=16. This is the
// saturating-arithmetic idea in Muła and Langdale's hex parser, with a strict
// case policy and validation completed before any output is written:
// http://0x80.pl/notesen/2022-01-17-validating-hex-parse.html
// Wrapping (x - 58), saturating subtraction of 6, then wrapping +16 maps
// '0'..='9' to 0..=9 and everything else to >=16. The letter candidate maps
// its first six values to 10..=15; unsigned min chooses the valid candidate.
#[inline]
#[target_feature(enable = "sse4.1")]
unsafe fn decode_sse41_nibbles(bytes: __m128i, case: CheckCase) -> __m128i {
    let digit = _mm_sub_epi8(
        _mm_subs_epu8(_mm_add_epi8(bytes, _mm_set1_epi8(-58)), _mm_set1_epi8(6)),
        _mm_set1_epi8(-16),
    );
    let fold = if case == CheckCase::None { 0x20 } else { 0 };
    let first = if case == CheckCase::Upper { b'A' } else { b'a' };
    let letter = _mm_sub_epi8(
        _mm_or_si128(bytes, _mm_set1_epi8(fold)),
        _mm_set1_epi8(first as i8),
    );
    _mm_min_epu8(digit, _mm_adds_epu8(letter, _mm_set1_epi8(10)))
}

// Returns (nibbles, validity bytes). Nibbles are valid only when no validity
// byte has its sign bit set. Check the complete input before storing output.
// The word-sized shift leaves neighbor bits in the hash, but for ASCII they
// only affect index bits ignored by the 16-entry byte shuffles.
#[inline]
#[target_feature(enable = "avx2")]
unsafe fn decode_avx2_nibbles(bytes: __m256i, case: CheckCase) -> (__m256i, __m256i) {
    let (hash_table, decode_offsets) = alsw_tables();
    let hash_table = _mm256_broadcastsi128_si256(hash_table);
    let check_offsets =
        _mm256_broadcastsi128_si256(_mm_loadu_si128(decode_check_offsets(case).as_ptr().cast()));
    let decode_offsets = _mm256_broadcastsi128_si256(decode_offsets);
    let hash = _mm256_avg_epu8(
        _mm256_srli_epi32::<3>(bytes),
        _mm256_shuffle_epi8(hash_table, bytes),
    );
    let checked = _mm256_adds_epi8(bytes, _mm256_shuffle_epi8(check_offsets, hash));
    let nibbles = _mm256_add_epi8(bytes, _mm256_shuffle_epi8(decode_offsets, hash));
    (nibbles, checked)
}

// Same nibble/validity contract as decode_avx2_nibbles, for 64 input bytes.
#[inline]
#[target_feature(enable = "avx512f,avx512bw")]
unsafe fn decode_avx512_nibbles(bytes: __m512i, case: CheckCase) -> (__m512i, __m512i) {
    let (hash_table, decode_offsets) = alsw_tables();
    let hash_table = _mm512_broadcast_i32x4(hash_table);
    let check_offsets =
        _mm512_broadcast_i32x4(_mm_loadu_si128(decode_check_offsets(case).as_ptr().cast()));
    let decode_offsets = _mm512_broadcast_i32x4(decode_offsets);
    let hash = _mm512_avg_epu8(
        _mm512_srli_epi32::<3>(bytes),
        _mm512_shuffle_epi8(hash_table, bytes),
    );
    let checked = _mm512_adds_epi8(bytes, _mm512_shuffle_epi8(check_offsets, hash));
    let nibbles = _mm512_add_epi8(bytes, _mm512_shuffle_epi8(decode_offsets, hash));
    (nibbles, checked)
}

// Checked SIMD decoders require even input of at least 16 bytes and the exact
// 2:1 input/output ratio. Dispatch handles shorter input with the scalar kernel.
// Bounded inputs stay in registers until every byte passes validation; the
// general path validates the complete input before writing.
// 8..=16 output bytes fit in two input registers, possibly overlapping.
#[inline]
#[target_feature(enable = "sse4.1")]
unsafe fn hex_decode_short_sse41(src: &[u8], dst: &mut [u8], case: CheckCase) -> Result<(), ()> {
    debug_assert!((16..=32).contains(&src.len()));
    let a = decode_sse41_nibbles(_mm_loadu_si128(src.as_ptr().cast()), case);
    // An eight-byte output fits in this one load; no overlapping tail is needed.
    if src.len() == 16 {
        if _mm_testz_si128(a, _mm_set1_epi8(-16)) == 0 {
            return Err(());
        }
        _mm_storel_epi64(dst.as_mut_ptr().cast(), pack_sse41(a, a));
        return Ok(());
    }
    let b = decode_sse41_nibbles(
        _mm_loadu_si128(src.as_ptr().add(src.len() - 16).cast()),
        case,
    );
    if _mm_testz_si128(_mm_or_si128(a, b), _mm_set1_epi8(-16)) == 0 {
        return Err(());
    }
    let decoded = pack_sse41(a, b);
    _mm_storel_epi64(dst.as_mut_ptr().cast(), decoded);
    _mm_storel_epi64(
        dst.as_mut_ptr().add(dst.len() - 8).cast(),
        _mm_srli_si128::<8>(decoded),
    );
    Ok(())
}

#[target_feature(enable = "sse4.1")]
pub(crate) unsafe fn hex_decode_sse41_checked(
    src: &[u8],
    dst: &mut [u8],
    case: CheckCase,
) -> Result<(), ()> {
    debug_assert!(src.len() >= 16);
    if src.len() <= 32 {
        return hex_decode_short_sse41(src, dst, case);
    }
    if src.len() <= 64 {
        let a = decode_sse41_nibbles(_mm_loadu_si128(src.as_ptr().cast()), case);
        let b = decode_sse41_nibbles(_mm_loadu_si128(src.as_ptr().add(16).cast()), case);
        let c = decode_sse41_nibbles(
            _mm_loadu_si128(src.as_ptr().add(src.len() - 32).cast()),
            case,
        );
        let d = decode_sse41_nibbles(
            _mm_loadu_si128(src.as_ptr().add(src.len() - 16).cast()),
            case,
        );
        let combined = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(c, d));
        if _mm_testz_si128(combined, _mm_set1_epi8(-16)) == 0 {
            return Err(());
        }
        _mm_storeu_si128(dst.as_mut_ptr().cast(), pack_sse41(a, b));
        _mm_storeu_si128(
            dst.as_mut_ptr().add(dst.len() - 16).cast(),
            pack_sse41(c, d),
        );
    } else {
        if !hex_check_sse_with_case(src, case) {
            return Err(());
        }
        hex_decode_sse41(src, dst);
    }
    Ok(())
}

#[target_feature(enable = "avx2")]
#[inline]
pub(crate) unsafe fn hex_decode_avx2_checked(
    src: &[u8],
    dst: &mut [u8],
    case: CheckCase,
) -> Result<(), ()> {
    debug_assert!(src.len() >= 16);
    // Common fixed sizes use constant offsets and avoid duplicate tail loads.
    // Every branch validates the complete input before its first store.
    if src.len() == 64 {
        let (a, a_checked) = decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().cast()), case);
        let (b, b_checked) =
            decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().add(32).cast()), case);
        if _mm256_movemask_epi8(_mm256_or_si256(a_checked, b_checked)) != 0 {
            return Err(());
        }
        _mm256_storeu_si256(dst.as_mut_ptr().cast(), pack_avx2(a, b));
        return Ok(());
    }
    if src.len() == 32 {
        let (nibbles, checked) = decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().cast()), case);
        if _mm256_movemask_epi8(checked) != 0 {
            return Err(());
        }
        _mm_storeu_si128(
            dst.as_mut_ptr().cast(),
            _mm256_castsi256_si128(pack_avx2(nibbles, nibbles)),
        );
        return Ok(());
    }
    if src.len() < 32 {
        return hex_decode_short_sse41(src, dst, case);
    }
    if src.len() < 64 {
        let (a, a_checked) = decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().cast()), case);
        let (b, b_checked) = decode_avx2_nibbles(
            _mm256_loadu_si256(src.as_ptr().add(src.len() - 32).cast()),
            case,
        );
        if _mm256_movemask_epi8(_mm256_or_si256(a_checked, b_checked)) != 0 {
            return Err(());
        }
        let packed = pack_avx2(a, b);
        _mm_storeu_si128(dst.as_mut_ptr().cast(), _mm256_castsi256_si128(packed));
        _mm_storeu_si128(
            dst.as_mut_ptr().add(dst.len() - 16).cast(),
            _mm256_extracti128_si256::<1>(packed),
        );
    } else if src.len() <= 128 {
        let (a, a_checked) = decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().cast()), case);
        let (b, b_checked) =
            decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().add(32).cast()), case);
        let (c, c_checked) = decode_avx2_nibbles(
            _mm256_loadu_si256(src.as_ptr().add(src.len() - 64).cast()),
            case,
        );
        let (d, d_checked) = decode_avx2_nibbles(
            _mm256_loadu_si256(src.as_ptr().add(src.len() - 32).cast()),
            case,
        );
        let checked = _mm256_or_si256(
            _mm256_or_si256(a_checked, b_checked),
            _mm256_or_si256(c_checked, d_checked),
        );
        if _mm256_movemask_epi8(checked) != 0 {
            return Err(());
        }
        _mm256_storeu_si256(dst.as_mut_ptr().cast(), pack_avx2(a, b));
        _mm256_storeu_si256(dst.as_mut_ptr().add(dst.len() - 32).cast(), pack_avx2(c, d));
    } else if src.len() <= 192 {
        let (a, a_checked) = decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().cast()), case);
        let (b, b_checked) =
            decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().add(32).cast()), case);
        let (c, c_checked) =
            decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().add(64).cast()), case);
        let (d, d_checked) =
            decode_avx2_nibbles(_mm256_loadu_si256(src.as_ptr().add(96).cast()), case);
        let (e, e_checked) = decode_avx2_nibbles(
            _mm256_loadu_si256(src.as_ptr().add(src.len() - 64).cast()),
            case,
        );
        let (f, f_checked) = decode_avx2_nibbles(
            _mm256_loadu_si256(src.as_ptr().add(src.len() - 32).cast()),
            case,
        );
        let checked = _mm256_or_si256(
            _mm256_or_si256(a_checked, b_checked),
            _mm256_or_si256(
                _mm256_or_si256(c_checked, d_checked),
                _mm256_or_si256(e_checked, f_checked),
            ),
        );
        if _mm256_movemask_epi8(checked) != 0 {
            return Err(());
        }
        _mm256_storeu_si256(dst.as_mut_ptr().cast(), pack_avx2(a, b));
        _mm256_storeu_si256(dst.as_mut_ptr().add(32).cast(), pack_avx2(c, d));
        _mm256_storeu_si256(dst.as_mut_ptr().add(dst.len() - 32).cast(), pack_avx2(e, f));
    } else {
        if !hex_check_avx2_with_case(src, case) {
            return Err(());
        }
        hex_decode_avx2(src, dst);
    }
    Ok(())
}

#[target_feature(enable = "avx512f,avx512bw")]
#[inline]
pub(crate) unsafe fn hex_decode_avx512_checked(
    src: &[u8],
    dst: &mut [u8],
    case: CheckCase,
) -> Result<(), ()> {
    debug_assert!(src.len() >= 16);
    if src.len() < 64 {
        return hex_decode_avx2_checked(src, dst, case);
    }
    if src.len() == 64 {
        let (nibbles, checked) =
            decode_avx512_nibbles(_mm512_loadu_si512(src.as_ptr().cast()), case);
        if _mm512_movepi8_mask(checked) != 0 {
            return Err(());
        }
        _mm256_storeu_si256(dst.as_mut_ptr().cast(), pack_avx512(nibbles));
    } else if src.len() <= 128 {
        let (a, a_checked) = decode_avx512_nibbles(_mm512_loadu_si512(src.as_ptr().cast()), case);
        let (b, b_checked) = decode_avx512_nibbles(
            _mm512_loadu_si512(src.as_ptr().add(src.len() - 64).cast()),
            case,
        );
        if _mm512_movepi8_mask(_mm512_or_si512(a_checked, b_checked)) != 0 {
            return Err(());
        }
        _mm256_storeu_si256(dst.as_mut_ptr().cast(), pack_avx512(a));
        _mm256_storeu_si256(dst.as_mut_ptr().add(dst.len() - 32).cast(), pack_avx512(b));
    } else if src.len() <= 192 {
        let (a, a_checked) = decode_avx512_nibbles(_mm512_loadu_si512(src.as_ptr().cast()), case);
        let (b, b_checked) =
            decode_avx512_nibbles(_mm512_loadu_si512(src.as_ptr().add(64).cast()), case);
        let (c, c_checked) = decode_avx512_nibbles(
            _mm512_loadu_si512(src.as_ptr().add(src.len() - 64).cast()),
            case,
        );
        if _mm512_movepi8_mask(_mm512_or_si512(
            _mm512_or_si512(a_checked, b_checked),
            c_checked,
        )) != 0
        {
            return Err(());
        }
        _mm256_storeu_si256(dst.as_mut_ptr().cast(), pack_avx512(a));
        _mm256_storeu_si256(dst.as_mut_ptr().add(32).cast(), pack_avx512(b));
        _mm256_storeu_si256(dst.as_mut_ptr().add(dst.len() - 32).cast(), pack_avx512(c));
    } else {
        if !hex_check_avx512_with_case(src, case) {
            return Err(());
        }
        hex_decode_avx512(src, dst);
    }
    Ok(())
}

#[inline]
#[target_feature(enable = "avx512f,avx512bw")]
unsafe fn pack_avx512(nibbles: __m512i) -> __m256i {
    _mm512_cvtepi16_epi8(_mm512_maddubs_epi16(nibbles, _mm512_set1_epi16(0x0110)))
}

#[inline]
#[target_feature(enable = "avx512f,avx512bw")]
pub(super) unsafe fn hex_decode_avx512(src: &[u8], dst: &mut [u8]) {
    if src.len() < 64 {
        return hex_decode_avx2(src, dst);
    }
    let convert = |input: &[u8; 64], output: &mut [u8; 32]| {
        let bytes = _mm512_loadu_si512(input.as_ptr().cast());
        let low = _mm512_and_si512(bytes, _mm512_set1_epi8(15));
        let letters = _mm512_cmpgt_epu8_mask(bytes, _mm512_set1_epi8(b'9' as i8));
        let nibbles = _mm512_mask_add_epi8(low, letters, low, _mm512_set1_epi8(9));
        _mm256_storeu_si256(output.as_mut_ptr().cast(), pack_avx512(nibbles));
    };
    let (blocks, tail) = src.as_chunks::<64>();
    for (input, output) in blocks.iter().zip(dst.as_chunks_mut::<32>().0) {
        convert(input, output);
    }
    if !tail.is_empty() {
        if let (Some(input), Some(output)) = (src.last_chunk::<64>(), dst.last_chunk_mut::<32>()) {
            convert(input, output);
        }
    }
}

#[inline]
#[target_feature(enable = "sse4.1")]
unsafe fn decode_sse41_block(a: __m128i, b: __m128i) -> __m128i {
    let nibble = |bytes| {
        // Valid ASCII letters add nine to their low nibble.
        _mm_add_epi8(
            _mm_and_si128(bytes, _mm_set1_epi8(15)),
            _mm_and_si128(
                _mm_cmpgt_epi8(bytes, _mm_set1_epi8(b'9' as i8)),
                _mm_set1_epi8(9),
            ),
        )
    };
    pack_sse41(nibble(a), nibble(b))
}

#[inline]
#[target_feature(enable = "avx2")]
unsafe fn decode_avx2_block(a: __m256i, b: __m256i) -> __m256i {
    let nibble = |bytes| {
        _mm256_add_epi8(
            _mm256_and_si256(bytes, _mm256_set1_epi8(15)),
            _mm256_and_si256(
                _mm256_cmpgt_epi8(bytes, _mm256_set1_epi8(b'9' as i8)),
                _mm256_set1_epi8(9),
            ),
        )
    };
    pack_avx2(nibble(a), nibble(b))
}

#[inline]
#[target_feature(enable = "sse4.1")]
unsafe fn pack_sse41(a: __m128i, b: __m128i) -> __m128i {
    // PMADDUBSW combines adjacent nibbles with [16, 1]; no result exceeds 255.
    let weights = _mm_set1_epi16(0x0110);
    _mm_packus_epi16(_mm_maddubs_epi16(a, weights), _mm_maddubs_epi16(b, weights))
}

#[inline]
#[target_feature(enable = "avx2")]
unsafe fn pack_avx2(a: __m256i, b: __m256i) -> __m256i {
    let weights = _mm256_set1_epi16(0x0110);
    // Packing is lane-local; restore the original order of the four 8-byte groups.
    _mm256_permute4x64_epi64::<0xd8>(_mm256_packus_epi16(
        _mm256_maddubs_epi16(a, weights),
        _mm256_maddubs_epi16(b, weights),
    ))
}

#[target_feature(enable = "sse4.1")]
pub(super) unsafe fn hex_decode_sse41(src: &[u8], dst: &mut [u8]) {
    if src.len() < 32 {
        return hex_decode_fallback(src, dst);
    }
    let convert = |input: &[u8; 32], output: &mut [u8; 16]| {
        let a = _mm_loadu_si128(input.as_ptr().cast());
        let b = _mm_loadu_si128(input.as_ptr().add(16).cast());
        _mm_storeu_si128(output.as_mut_ptr().cast(), decode_sse41_block(a, b));
    };
    let (blocks, tail) = src.as_chunks::<32>();
    for (input, output) in blocks.iter().zip(dst.as_chunks_mut::<16>().0) {
        convert(input, output);
    }
    if !tail.is_empty() {
        if let (Some(input), Some(output)) = (src.last_chunk::<32>(), dst.last_chunk_mut::<16>()) {
            convert(input, output);
        }
    }
}

#[target_feature(enable = "avx2")]
pub(super) unsafe fn hex_decode_avx2(src: &[u8], dst: &mut [u8]) {
    if src.len() < 64 {
        return hex_decode_sse41(src, dst);
    }
    let convert = |input: &[u8; 64], output: &mut [u8; 32]| {
        let a = _mm256_loadu_si256(input.as_ptr().cast());
        let b = _mm256_loadu_si256(input.as_ptr().add(32).cast());
        _mm256_storeu_si256(output.as_mut_ptr().cast(), decode_avx2_block(a, b));
    };
    let (blocks, tail) = src.as_chunks::<64>();
    for (input, output) in blocks.iter().zip(dst.as_chunks_mut::<32>().0) {
        convert(input, output);
    }
    if !tail.is_empty() {
        if let (Some(input), Some(output)) = (src.last_chunk::<64>(), dst.last_chunk_mut::<32>()) {
            convert(input, output);
        }
    }
}

// The exact-size output is private to an owning caller: earlier valid blocks
// may be written before a later block fails. The feature boundary surrounds
// the loop so each constant-size checked kernel can inline into it.
#[target_feature(enable = "avx2")]
pub(crate) unsafe fn hex_decode_avx2_owned(
    src: &[u8],
    dst: &mut [u8],
    case: CheckCase,
) -> Result<(), ()> {
    let (blocks, tail) = src.as_chunks::<64>();
    let (outputs, rest) = dst.split_at_mut(blocks.len() * 32);
    for (input, output) in blocks.iter().zip(outputs.as_chunks_mut::<32>().0) {
        hex_decode_avx2_checked(input, output, case)?;
    }
    super::decode_checked(tail, rest, case)
}

#[target_feature(enable = "avx512f,avx512bw")]
pub(crate) unsafe fn hex_decode_avx512_owned(
    src: &[u8],
    dst: &mut [u8],
    case: CheckCase,
) -> Result<(), ()> {
    let (blocks, tail) = src.as_chunks::<128>();
    let (outputs, rest) = dst.split_at_mut(blocks.len() * 64);
    for (input, output) in blocks.iter().zip(outputs.as_chunks_mut::<64>().0) {
        hex_decode_avx512_checked(input, output, case)?;
    }
    super::decode_checked(tail, rest, case)
}
