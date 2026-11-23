//! NEON checking and decoding.
//!
//! Callers establish NEON support. Checkers accept any source length. Bounded
//! decoders require the documented input range and exactly half as much output
//! space; they validate the complete input before writing. The raw decoder
//! additionally requires valid hex input.

// SIMD decoding includes work derived from fast-hex under the MIT license.
// See https://github.com/zbjornson/fast-hex and LICENSE-THIRD-PARTY/fast-hex.
// Saturating nibble mapping is adapted from const-hex under the MIT license;
// see LICENSE-THIRD-PARTY/const-hex.

use core::arch::aarch64::*;

use super::{hex_check_fallback_with_case, hex_decode_fallback, CheckCase};

// Wrapping subtraction turns each ASCII range into one unsigned comparison.
// Only the either-case policy folds the ASCII case bit.
#[inline]
#[target_feature(enable = "neon")]
unsafe fn valid_neon(bytes: uint8x16_t, case: CheckCase) -> uint8x16_t {
    let digit = vcleq_u8(vsubq_u8(bytes, vdupq_n_u8(b'0')), vdupq_n_u8(9));
    let fold = if case == CheckCase::None { 0x20 } else { 0 };
    let letters = vorrq_u8(bytes, vdupq_n_u8(fold));
    let first = if case == CheckCase::Upper { b'A' } else { b'a' };
    let letter = vcleq_u8(vsubq_u8(letters, vdupq_n_u8(first)), vdupq_n_u8(5));
    vorrq_u8(digit, letter)
}

#[inline]
#[target_feature(enable = "neon")]
unsafe fn decode_neon_nibbles(bytes: uint8x16_t, case: CheckCase) -> uint8x16_t {
    let digit = vsubq_u8(
        vqsubq_u8(vaddq_u8(bytes, vdupq_n_u8(198)), vdupq_n_u8(6)),
        vdupq_n_u8(240),
    );
    let fold = if case == CheckCase::None { 0x20 } else { 0 };
    let first = if case == CheckCase::Upper { b'A' } else { b'a' };
    let letter = vsubq_u8(vorrq_u8(bytes, vdupq_n_u8(fold)), vdupq_n_u8(first));
    vminq_u8(digit, vqaddq_u8(letter, vdupq_n_u8(10)))
}

#[inline]
#[target_feature(enable = "neon")]
pub(crate) unsafe fn hex_check_neon_with_case(src: &[u8], check_case: CheckCase) -> bool {
    if src.len() < 64 {
        return hex_check_neon_short(src, check_case);
    }
    let (batches, rest) = src.as_chunks::<64>();
    for batch in batches {
        let a = valid_neon(vld1q_u8(batch.as_ptr()), check_case);
        let b = valid_neon(vld1q_u8(batch.as_ptr().add(16)), check_case);
        let c = valid_neon(vld1q_u8(batch.as_ptr().add(32)), check_case);
        let d = valid_neon(vld1q_u8(batch.as_ptr().add(48)), check_case);
        if vminvq_u8(vandq_u8(vandq_u8(a, b), vandq_u8(c, d))) == 0 {
            return false;
        }
    }
    rest.is_empty() || hex_check_neon_short(&src[src.len() - rest.len().max(16)..], check_case)
}

// At most 64 bytes. Overlapping blocks cover the complete input, with only
// one horizontal reduction. The caller handles longer inputs in 64-byte batches.
#[inline]
#[target_feature(enable = "neon")]
unsafe fn hex_check_neon_short(src: &[u8], check_case: CheckCase) -> bool {
    if src.len() < 16 {
        return hex_check_fallback_with_case(src, check_case);
    }
    let a = valid_neon(vld1q_u8(src.as_ptr()), check_case);
    if src.len() == 16 {
        return vminvq_u8(a) != 0;
    }
    let b = valid_neon(vld1q_u8(src.as_ptr().add(src.len() - 16)), check_case);
    let valid = if src.len() <= 32 {
        vandq_u8(a, b)
    } else {
        let c = valid_neon(vld1q_u8(src.as_ptr().add(16)), check_case);
        let valid = vandq_u8(vandq_u8(a, b), c);
        if src.len() <= 48 {
            valid
        } else {
            let d = valid_neon(vld1q_u8(src.as_ptr().add(src.len() - 32)), check_case);
            vandq_u8(valid, d)
        }
    };
    vminvq_u8(valid) != 0
}

#[inline]
#[target_feature(enable = "neon")]
unsafe fn decode_neon_block(high: uint8x16_t, low: uint8x16_t) -> uint8x16_t {
    // Valid ASCII hex letters need nine added to their low nibble. SLI packs
    // just those nibbles, discarding the ASCII high bits without separate masks.
    let adjust = |bytes| vmlaq_u8(bytes, vshrq_n_u8::<6>(bytes), vdupq_n_u8(9));
    vsliq_n_u8::<4>(adjust(low), adjust(high))
}

// 17..=32 decoded bytes fit in four input registers. Overlapping end
// blocks cover the complete input; validate all registers before either store.
#[inline]
#[target_feature(enable = "neon")]
pub(super) unsafe fn hex_decode_bounded_neon(
    src: &[u8],
    dst: &mut [u8],
    case: CheckCase,
) -> Result<(), ()> {
    let a = decode_neon_nibbles(vld1q_u8(src.as_ptr()), case);
    let b = decode_neon_nibbles(vld1q_u8(src.as_ptr().add(16)), case);
    let c = decode_neon_nibbles(vld1q_u8(src.as_ptr().add(src.len() - 32)), case);
    let d = decode_neon_nibbles(vld1q_u8(src.as_ptr().add(src.len() - 16)), case);
    if vmaxvq_u8(vorrq_u8(vorrq_u8(a, b), vorrq_u8(c, d))) > 15 {
        return Err(());
    }
    let pack = |hi, lo| vsliq_n_u8::<4>(vuzp2q_u8(hi, lo), vuzp1q_u8(hi, lo));
    vst1q_u8(dst.as_mut_ptr(), pack(a, b));
    vst1q_u8(dst.as_mut_ptr().add(dst.len() - 16), pack(c, d));
    Ok(())
}

#[inline]
#[target_feature(enable = "neon")]
pub(super) unsafe fn hex_decode_neon(src: &[u8], dst: &mut [u8]) {
    if src.len() < 32 {
        return hex_decode_fallback(src, dst);
    }
    let (batches, rest) = src.as_chunks::<128>();
    let (outputs, remaining) = dst.as_chunks_mut::<64>();
    for (input, output) in batches.iter().zip(outputs) {
        for (input, output) in input
            .as_chunks::<32>()
            .0
            .iter()
            .zip(output.as_chunks_mut::<16>().0)
        {
            let uint8x16x2_t(a, b) = vld2q_u8(input.as_ptr());
            vst1q_u8(output.as_mut_ptr(), decode_neon_block(a, b));
        }
    }
    let (blocks, tail) = rest.as_chunks::<32>();
    for (input, output) in blocks.iter().zip(remaining.as_chunks_mut::<16>().0) {
        let uint8x16x2_t(a, b) = vld2q_u8(input.as_ptr());
        vst1q_u8(output.as_mut_ptr(), decode_neon_block(a, b));
    }
    if !tail.is_empty() {
        // Input is already validated. An overlapping final block avoids scalar
        // tails while keeping every load and store within the original slices.
        match (src.last_chunk::<32>(), dst.last_chunk_mut::<16>()) {
            (Some(input), Some(output)) => {
                let uint8x16x2_t(a, b) = vld2q_u8(input.as_ptr());
                vst1q_u8(output.as_mut_ptr(), decode_neon_block(a, b));
            }
            _ => hex_decode_fallback(src, dst),
        }
    }
}

// 8..=16 decoded bytes fit in two input registers, possibly overlapping.
#[inline]
#[target_feature(enable = "neon")]
pub(super) unsafe fn hex_decode_short_neon(
    src: &[u8],
    dst: &mut [u8],
    case: CheckCase,
) -> Result<(), ()> {
    let pack = |a, b| vsliq_n_u8::<4>(vuzp2q_u8(a, b), vuzp1q_u8(a, b));
    let a = decode_neon_nibbles(vld1q_u8(src.as_ptr()), case);
    if src.len() == 16 {
        if vmaxvq_u8(a) > 15 {
            return Err(());
        }
        vst1_u8(dst.as_mut_ptr(), vget_low_u8(pack(a, a)));
        return Ok(());
    }
    let b = decode_neon_nibbles(vld1q_u8(src.as_ptr().add(src.len() - 16)), case);
    if vmaxvq_u8(vorrq_u8(a, b)) > 15 {
        return Err(());
    }
    let decoded = pack(a, b);
    vst1_u8(dst.as_mut_ptr(), vget_low_u8(decoded));
    vst1_u8(dst.as_mut_ptr().add(dst.len() - 8), vget_high_u8(decoded));
    Ok(())
}

// Earlier valid blocks may be committed: the owning caller discards output
// if any later block fails. Lengths have the exact 2:1 input/output ratio.
#[target_feature(enable = "neon")]
pub(crate) unsafe fn hex_decode_neon_owned(
    src: &[u8],
    dst: &mut [u8],
    case: CheckCase,
) -> Result<(), ()> {
    let (blocks, tail) = src.as_chunks::<64>();
    let (outputs, rest) = dst.split_at_mut(blocks.len() * 32);
    for (input, output) in blocks.iter().zip(outputs.as_chunks_mut::<32>().0) {
        hex_decode_bounded_neon(input, output, case)?;
    }
    super::decode_checked(tail, rest, case)
}
