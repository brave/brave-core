#[cfg(target_arch = "x86")]
use core::arch::x86::*;
#[cfg(target_arch = "x86_64")]
use core::arch::x86_64::*;

use crate::guts::{
    assemble_count, count_high, count_low, final_block, flag_word, input_debug_asserts, Finalize,
    Job, LastNode, Stride,
};
use crate::{Count, Word, BLOCKBYTES, IV, SIGMA};
use arrayref::{array_refs, mut_array_refs};
use core::cmp;
use core::mem;

pub const DEGREE: usize = 4;

#[inline(always)]
unsafe fn loadu(src: *const [Word; DEGREE]) -> __m128i {
    // This is an unaligned load, so the pointer cast is allowed.
    _mm_loadu_si128(src as *const __m128i)
}

#[inline(always)]
unsafe fn storeu(src: __m128i, dest: *mut [Word; DEGREE]) {
    // This is an unaligned store, so the pointer cast is allowed.
    _mm_storeu_si128(dest as *mut __m128i, src)
}

#[inline(always)]
unsafe fn add(a: __m128i, b: __m128i) -> __m128i {
    _mm_add_epi32(a, b)
}

#[inline(always)]
unsafe fn eq(a: __m128i, b: __m128i) -> __m128i {
    _mm_cmpeq_epi32(a, b)
}

#[inline(always)]
unsafe fn and(a: __m128i, b: __m128i) -> __m128i {
    _mm_and_si128(a, b)
}

#[inline(always)]
unsafe fn negate_and(a: __m128i, b: __m128i) -> __m128i {
    // Note that "and not" implies the reverse of the actual arg order.
    _mm_andnot_si128(a, b)
}

#[inline(always)]
unsafe fn xor(a: __m128i, b: __m128i) -> __m128i {
    _mm_xor_si128(a, b)
}

#[inline(always)]
unsafe fn set1(x: u32) -> __m128i {
    _mm_set1_epi32(x as i32)
}

#[inline(always)]
unsafe fn set4(a: u32, b: u32, c: u32, d: u32) -> __m128i {
    _mm_setr_epi32(a as i32, b as i32, c as i32, d as i32)
}

// These rotations are the "simple version". For the "complicated version", see
// https://github.com/sneves/blake2-avx2/blob/b3723921f668df09ece52dcd225a36d4a4eea1d9/blake2s-common.h#L63-L66.
// For a discussion of the tradeoffs, see
// https://github.com/sneves/blake2-avx2/pull/5. In short:
// - Due to an LLVM bug (https://bugs.llvm.org/show_bug.cgi?id=44379), this
//   version performs better on recent x86 chips.
// - LLVM is able to optimize this version to AVX-512 rotation instructions
//   when those are enabled.

#[inline(always)]
unsafe fn rot7(a: __m128i) -> __m128i {
    _mm_or_si128(_mm_srli_epi32(a, 7), _mm_slli_epi32(a, 32 - 7))
}

#[inline(always)]
unsafe fn rot8(a: __m128i) -> __m128i {
    _mm_or_si128(_mm_srli_epi32(a, 8), _mm_slli_epi32(a, 32 - 8))
}

#[inline(always)]
unsafe fn rot12(a: __m128i) -> __m128i {
    _mm_or_si128(_mm_srli_epi32(a, 12), _mm_slli_epi32(a, 32 - 12))
}

#[inline(always)]
unsafe fn rot16(a: __m128i) -> __m128i {
    _mm_or_si128(_mm_srli_epi32(a, 16), _mm_slli_epi32(a, 32 - 16))
}

#[inline(always)]
unsafe fn g1(
    row1: &mut __m128i,
    row2: &mut __m128i,
    row3: &mut __m128i,
    row4: &mut __m128i,
    m: __m128i,
) {
    *row1 = add(add(*row1, m), *row2);
    *row4 = xor(*row4, *row1);
    *row4 = rot16(*row4);
    *row3 = add(*row3, *row4);
    *row2 = xor(*row2, *row3);
    *row2 = rot12(*row2);
}

#[inline(always)]
unsafe fn g2(
    row1: &mut __m128i,
    row2: &mut __m128i,
    row3: &mut __m128i,
    row4: &mut __m128i,
    m: __m128i,
) {
    *row1 = add(add(*row1, m), *row2);
    *row4 = xor(*row4, *row1);
    *row4 = rot8(*row4);
    *row3 = add(*row3, *row4);
    *row2 = xor(*row2, *row3);
    *row2 = rot7(*row2);
}

// Adapted from https://github.com/rust-lang-nursery/stdsimd/pull/479.
macro_rules! _MM_SHUFFLE {
    ($z:expr, $y:expr, $x:expr, $w:expr) => {
        ($z << 6) | ($y << 4) | ($x << 2) | $w
    };
}

// Note the optimization here of leaving row2 as the unrotated row, rather than
// row1. All the message loads below are adjusted to compensate for this. See
// discussion at https://github.com/sneves/blake2-avx2/pull/4
#[inline(always)]
unsafe fn diagonalize(row1: &mut __m128i, row3: &mut __m128i, row4: &mut __m128i) {
    *row1 = _mm_shuffle_epi32(*row1, _MM_SHUFFLE!(2, 1, 0, 3));
    *row4 = _mm_shuffle_epi32(*row4, _MM_SHUFFLE!(1, 0, 3, 2));
    *row3 = _mm_shuffle_epi32(*row3, _MM_SHUFFLE!(0, 3, 2, 1));
}

#[inline(always)]
unsafe fn undiagonalize(row1: &mut __m128i, row3: &mut __m128i, row4: &mut __m128i) {
    *row1 = _mm_shuffle_epi32(*row1, _MM_SHUFFLE!(0, 3, 2, 1));
    *row4 = _mm_shuffle_epi32(*row4, _MM_SHUFFLE!(1, 0, 3, 2));
    *row3 = _mm_shuffle_epi32(*row3, _MM_SHUFFLE!(2, 1, 0, 3));
}

#[inline(always)]
pub unsafe fn compress_block(
    block: &[u8; BLOCKBYTES],
    words: &mut [Word; 8],
    count: Count,
    last_block: Word,
    last_node: Word,
) {
    let (words_low, words_high) = mut_array_refs!(words, DEGREE, DEGREE);
    let (iv_low, iv_high) = array_refs!(&IV, DEGREE, DEGREE);

    let row1 = &mut loadu(words_low);
    let row2 = &mut loadu(words_high);
    let row3 = &mut loadu(iv_low);
    let row4 = &mut xor(
        loadu(iv_high),
        set4(count_low(count), count_high(count), last_block, last_node),
    );

    let msg_ptr = block.as_ptr() as *const [Word; DEGREE];
    let m0 = loadu(msg_ptr.add(0));
    let m1 = loadu(msg_ptr.add(1));
    let m2 = loadu(msg_ptr.add(2));
    let m3 = loadu(msg_ptr.add(3));

    // round 1
    let buf = _mm_castps_si128(_mm_shuffle_ps(
        _mm_castsi128_ps(m0),
        _mm_castsi128_ps(m1),
        _MM_SHUFFLE!(2, 0, 2, 0),
    ));
    g1(row1, row2, row3, row4, buf);
    let buf = _mm_castps_si128(_mm_shuffle_ps(
        _mm_castsi128_ps(m0),
        _mm_castsi128_ps(m1),
        _MM_SHUFFLE!(3, 1, 3, 1),
    ));
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_shuffle_epi32(m2, _MM_SHUFFLE!(3, 2, 0, 1));
    let t1 = _mm_shuffle_epi32(m3, _MM_SHUFFLE!(0, 1, 3, 2));
    let buf = _mm_blend_epi16(t0, t1, 0xC3);
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_blend_epi16(t0, t1, 0x3C);
    let buf = _mm_shuffle_epi32(t0, _MM_SHUFFLE!(2, 3, 0, 1));
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    // round 2
    let t0 = _mm_blend_epi16(m1, m2, 0x0C);
    let t1 = _mm_slli_si128(m3, 4);
    let t2 = _mm_blend_epi16(t0, t1, 0xF0);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 1, 0, 3));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_shuffle_epi32(m2, _MM_SHUFFLE!(0, 0, 2, 0));
    let t1 = _mm_blend_epi16(m1, m3, 0xC0);
    let t2 = _mm_blend_epi16(t0, t1, 0xF0);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 3, 0, 1));
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_slli_si128(m1, 4);
    let t1 = _mm_blend_epi16(m2, t0, 0x30);
    let t2 = _mm_blend_epi16(m0, t1, 0xF0);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(3, 0, 1, 2));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_unpackhi_epi32(m0, m1);
    let t1 = _mm_slli_si128(m3, 4);
    let t2 = _mm_blend_epi16(t0, t1, 0x0C);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(3, 0, 1, 2));
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    // round 3
    let t0 = _mm_unpackhi_epi32(m2, m3);
    let t1 = _mm_blend_epi16(m3, m1, 0x0C);
    let t2 = _mm_blend_epi16(t0, t1, 0x0F);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(3, 1, 0, 2));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_unpacklo_epi32(m2, m0);
    let t1 = _mm_blend_epi16(t0, m0, 0xF0);
    let t2 = _mm_slli_si128(m3, 8);
    let buf = _mm_blend_epi16(t1, t2, 0xC0);
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_blend_epi16(m0, m2, 0x3C);
    let t1 = _mm_srli_si128(m1, 12);
    let t2 = _mm_blend_epi16(t0, t1, 0x03);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(0, 3, 2, 1));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_slli_si128(m3, 4);
    let t1 = _mm_blend_epi16(m0, m1, 0x33);
    let t2 = _mm_blend_epi16(t1, t0, 0xC0);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(1, 2, 3, 0));
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    // round 4
    let t0 = _mm_unpackhi_epi32(m0, m1);
    let t1 = _mm_unpackhi_epi32(t0, m2);
    let t2 = _mm_blend_epi16(t1, m3, 0x0C);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(3, 1, 0, 2));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_slli_si128(m2, 8);
    let t1 = _mm_blend_epi16(m3, m0, 0x0C);
    let t2 = _mm_blend_epi16(t1, t0, 0xC0);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 0, 1, 3));
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_blend_epi16(m0, m1, 0x0F);
    let t1 = _mm_blend_epi16(t0, m3, 0xC0);
    let buf = _mm_shuffle_epi32(t1, _MM_SHUFFLE!(0, 1, 2, 3));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_alignr_epi8(m0, m1, 4);
    let buf = _mm_blend_epi16(t0, m2, 0x33);
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    // round 5
    let t0 = _mm_unpacklo_epi64(m1, m2);
    let t1 = _mm_unpackhi_epi64(m0, m2);
    let t2 = _mm_blend_epi16(t0, t1, 0x33);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 0, 1, 3));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_unpackhi_epi64(m1, m3);
    let t1 = _mm_unpacklo_epi64(m0, m1);
    let buf = _mm_blend_epi16(t0, t1, 0x33);
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_unpackhi_epi64(m3, m1);
    let t1 = _mm_unpackhi_epi64(m2, m0);
    let t2 = _mm_blend_epi16(t1, t0, 0x33);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 1, 0, 3));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_blend_epi16(m0, m2, 0x03);
    let t1 = _mm_slli_si128(t0, 8);
    let t2 = _mm_blend_epi16(t1, m3, 0x0F);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 0, 3, 1));
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    // round 6
    let t0 = _mm_unpackhi_epi32(m0, m1);
    let t1 = _mm_unpacklo_epi32(m0, m2);
    let buf = _mm_unpacklo_epi64(t0, t1);
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_srli_si128(m2, 4);
    let t1 = _mm_blend_epi16(m0, m3, 0x03);
    let buf = _mm_blend_epi16(t1, t0, 0x3C);
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_blend_epi16(m1, m0, 0x0C);
    let t1 = _mm_srli_si128(m3, 4);
    let t2 = _mm_blend_epi16(t0, t1, 0x30);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 3, 0, 1));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_unpacklo_epi64(m2, m1);
    let t1 = _mm_shuffle_epi32(m3, _MM_SHUFFLE!(2, 0, 1, 0));
    let t2 = _mm_srli_si128(t0, 4);
    let buf = _mm_blend_epi16(t1, t2, 0x33);
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    // round 7
    let t0 = _mm_slli_si128(m1, 12);
    let t1 = _mm_blend_epi16(m0, m3, 0x33);
    let buf = _mm_blend_epi16(t1, t0, 0xC0);
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_blend_epi16(m3, m2, 0x30);
    let t1 = _mm_srli_si128(m1, 4);
    let t2 = _mm_blend_epi16(t0, t1, 0x03);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 1, 3, 0));
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_unpacklo_epi64(m0, m2);
    let t1 = _mm_srli_si128(m1, 4);
    let buf = _mm_shuffle_epi32(_mm_blend_epi16(t0, t1, 0x0C), _MM_SHUFFLE!(3, 1, 0, 2));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_unpackhi_epi32(m1, m2);
    let t1 = _mm_unpackhi_epi64(m0, t0);
    let buf = _mm_shuffle_epi32(t1, _MM_SHUFFLE!(0, 1, 2, 3));
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    // round 8
    let t0 = _mm_unpackhi_epi32(m0, m1);
    let t1 = _mm_blend_epi16(t0, m3, 0x0F);
    let buf = _mm_shuffle_epi32(t1, _MM_SHUFFLE!(2, 0, 3, 1));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_blend_epi16(m2, m3, 0x30);
    let t1 = _mm_srli_si128(m0, 4);
    let t2 = _mm_blend_epi16(t0, t1, 0x03);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(1, 0, 2, 3));
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_unpackhi_epi64(m0, m3);
    let t1 = _mm_unpacklo_epi64(m1, m2);
    let t2 = _mm_blend_epi16(t0, t1, 0x3C);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 3, 1, 0));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_unpacklo_epi32(m0, m1);
    let t1 = _mm_unpackhi_epi32(m1, m2);
    let t2 = _mm_unpacklo_epi64(t0, t1);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(2, 1, 0, 3));
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    // round 9
    let t0 = _mm_unpackhi_epi32(m1, m3);
    let t1 = _mm_unpacklo_epi64(t0, m0);
    let t2 = _mm_blend_epi16(t1, m2, 0xC0);
    let buf = _mm_shufflehi_epi16(t2, _MM_SHUFFLE!(1, 0, 3, 2));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_unpackhi_epi32(m0, m3);
    let t1 = _mm_blend_epi16(m2, t0, 0xF0);
    let buf = _mm_shuffle_epi32(t1, _MM_SHUFFLE!(0, 2, 1, 3));
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_unpacklo_epi64(m0, m3);
    let t1 = _mm_srli_si128(m2, 8);
    let t2 = _mm_blend_epi16(t0, t1, 0x03);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(1, 3, 2, 0));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_blend_epi16(m1, m0, 0x30);
    let buf = _mm_shuffle_epi32(t0, _MM_SHUFFLE!(0, 3, 2, 1));
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    // round 10
    let t0 = _mm_blend_epi16(m0, m2, 0x03);
    let t1 = _mm_blend_epi16(m1, m2, 0x30);
    let t2 = _mm_blend_epi16(t1, t0, 0x0F);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(1, 3, 0, 2));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_slli_si128(m0, 4);
    let t1 = _mm_blend_epi16(m1, t0, 0xC0);
    let buf = _mm_shuffle_epi32(t1, _MM_SHUFFLE!(1, 2, 0, 3));
    g2(row1, row2, row3, row4, buf);
    diagonalize(row1, row3, row4);
    let t0 = _mm_unpackhi_epi32(m0, m3);
    let t1 = _mm_unpacklo_epi32(m2, m3);
    let t2 = _mm_unpackhi_epi64(t0, t1);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(0, 2, 1, 3));
    g1(row1, row2, row3, row4, buf);
    let t0 = _mm_blend_epi16(m3, m2, 0xC0);
    let t1 = _mm_unpacklo_epi32(m0, m3);
    let t2 = _mm_blend_epi16(t0, t1, 0x0F);
    let buf = _mm_shuffle_epi32(t2, _MM_SHUFFLE!(1, 2, 3, 0));
    g2(row1, row2, row3, row4, buf);
    undiagonalize(row1, row3, row4);

    storeu(xor(loadu(words_low), xor(*row1, *row3)), words_low);
    storeu(xor(loadu(words_high), xor(*row2, *row4)), words_high);
}

#[target_feature(enable = "sse4.1")]
pub unsafe fn compress1_loop(
    input: &[u8],
    words: &mut [Word; 8],
    mut count: Count,
    last_node: LastNode,
    finalize: Finalize,
    stride: Stride,
) {
    input_debug_asserts(input, finalize);

    let mut local_words = *words;

    let mut fin_offset = input.len().saturating_sub(1);
    fin_offset -= fin_offset % stride.padded_blockbytes();
    let mut buf = [0; BLOCKBYTES];
    let (fin_block, fin_len, _) = final_block(input, fin_offset, &mut buf, stride);
    let fin_last_block = flag_word(finalize.yes());
    let fin_last_node = flag_word(finalize.yes() && last_node.yes());

    let mut offset = 0;
    loop {
        let block;
        let count_delta;
        let last_block;
        let last_node;
        if offset == fin_offset {
            block = fin_block;
            count_delta = fin_len;
            last_block = fin_last_block;
            last_node = fin_last_node;
        } else {
            // This unsafe cast avoids bounds checks. There's guaranteed to be
            // enough input because `offset < fin_offset`.
            block = &*(input.as_ptr().add(offset) as *const [u8; BLOCKBYTES]);
            count_delta = BLOCKBYTES;
            last_block = flag_word(false);
            last_node = flag_word(false);
        };

        count = count.wrapping_add(count_delta as Count);
        compress_block(block, &mut local_words, count, last_block, last_node);

        // Check for termination before bumping the offset, to avoid overflow.
        if offset == fin_offset {
            break;
        }

        offset += stride.padded_blockbytes();
    }

    *words = local_words;
}

#[inline(always)]
unsafe fn round(v: &mut [__m128i; 16], m: &[__m128i; 16], r: usize) {
    v[0] = add(v[0], m[SIGMA[r][0] as usize]);
    v[1] = add(v[1], m[SIGMA[r][2] as usize]);
    v[2] = add(v[2], m[SIGMA[r][4] as usize]);
    v[3] = add(v[3], m[SIGMA[r][6] as usize]);
    v[0] = add(v[0], v[4]);
    v[1] = add(v[1], v[5]);
    v[2] = add(v[2], v[6]);
    v[3] = add(v[3], v[7]);
    v[12] = xor(v[12], v[0]);
    v[13] = xor(v[13], v[1]);
    v[14] = xor(v[14], v[2]);
    v[15] = xor(v[15], v[3]);
    v[12] = rot16(v[12]);
    v[13] = rot16(v[13]);
    v[14] = rot16(v[14]);
    v[15] = rot16(v[15]);
    v[8] = add(v[8], v[12]);
    v[9] = add(v[9], v[13]);
    v[10] = add(v[10], v[14]);
    v[11] = add(v[11], v[15]);
    v[4] = xor(v[4], v[8]);
    v[5] = xor(v[5], v[9]);
    v[6] = xor(v[6], v[10]);
    v[7] = xor(v[7], v[11]);
    v[4] = rot12(v[4]);
    v[5] = rot12(v[5]);
    v[6] = rot12(v[6]);
    v[7] = rot12(v[7]);
    v[0] = add(v[0], m[SIGMA[r][1] as usize]);
    v[1] = add(v[1], m[SIGMA[r][3] as usize]);
    v[2] = add(v[2], m[SIGMA[r][5] as usize]);
    v[3] = add(v[3], m[SIGMA[r][7] as usize]);
    v[0] = add(v[0], v[4]);
    v[1] = add(v[1], v[5]);
    v[2] = add(v[2], v[6]);
    v[3] = add(v[3], v[7]);
    v[12] = xor(v[12], v[0]);
    v[13] = xor(v[13], v[1]);
    v[14] = xor(v[14], v[2]);
    v[15] = xor(v[15], v[3]);
    v[12] = rot8(v[12]);
    v[13] = rot8(v[13]);
    v[14] = rot8(v[14]);
    v[15] = rot8(v[15]);
    v[8] = add(v[8], v[12]);
    v[9] = add(v[9], v[13]);
    v[10] = add(v[10], v[14]);
    v[11] = add(v[11], v[15]);
    v[4] = xor(v[4], v[8]);
    v[5] = xor(v[5], v[9]);
    v[6] = xor(v[6], v[10]);
    v[7] = xor(v[7], v[11]);
    v[4] = rot7(v[4]);
    v[5] = rot7(v[5]);
    v[6] = rot7(v[6]);
    v[7] = rot7(v[7]);

    v[0] = add(v[0], m[SIGMA[r][8] as usize]);
    v[1] = add(v[1], m[SIGMA[r][10] as usize]);
    v[2] = add(v[2], m[SIGMA[r][12] as usize]);
    v[3] = add(v[3], m[SIGMA[r][14] as usize]);
    v[0] = add(v[0], v[5]);
    v[1] = add(v[1], v[6]);
    v[2] = add(v[2], v[7]);
    v[3] = add(v[3], v[4]);
    v[15] = xor(v[15], v[0]);
    v[12] = xor(v[12], v[1]);
    v[13] = xor(v[13], v[2]);
    v[14] = xor(v[14], v[3]);
    v[15] = rot16(v[15]);
    v[12] = rot16(v[12]);
    v[13] = rot16(v[13]);
    v[14] = rot16(v[14]);
    v[10] = add(v[10], v[15]);
    v[11] = add(v[11], v[12]);
    v[8] = add(v[8], v[13]);
    v[9] = add(v[9], v[14]);
    v[5] = xor(v[5], v[10]);
    v[6] = xor(v[6], v[11]);
    v[7] = xor(v[7], v[8]);
    v[4] = xor(v[4], v[9]);
    v[5] = rot12(v[5]);
    v[6] = rot12(v[6]);
    v[7] = rot12(v[7]);
    v[4] = rot12(v[4]);
    v[0] = add(v[0], m[SIGMA[r][9] as usize]);
    v[1] = add(v[1], m[SIGMA[r][11] as usize]);
    v[2] = add(v[2], m[SIGMA[r][13] as usize]);
    v[3] = add(v[3], m[SIGMA[r][15] as usize]);
    v[0] = add(v[0], v[5]);
    v[1] = add(v[1], v[6]);
    v[2] = add(v[2], v[7]);
    v[3] = add(v[3], v[4]);
    v[15] = xor(v[15], v[0]);
    v[12] = xor(v[12], v[1]);
    v[13] = xor(v[13], v[2]);
    v[14] = xor(v[14], v[3]);
    v[15] = rot8(v[15]);
    v[12] = rot8(v[12]);
    v[13] = rot8(v[13]);
    v[14] = rot8(v[14]);
    v[10] = add(v[10], v[15]);
    v[11] = add(v[11], v[12]);
    v[8] = add(v[8], v[13]);
    v[9] = add(v[9], v[14]);
    v[5] = xor(v[5], v[10]);
    v[6] = xor(v[6], v[11]);
    v[7] = xor(v[7], v[8]);
    v[4] = xor(v[4], v[9]);
    v[5] = rot7(v[5]);
    v[6] = rot7(v[6]);
    v[7] = rot7(v[7]);
    v[4] = rot7(v[4]);
}

// We'd rather make this a regular function with #[inline(always)], but for
// some reason that blows up compile times by about 10 seconds, at least in
// some cases (BLAKE2b avx2.rs). This macro seems to get the same performance
// result, without the compile time issue.
macro_rules! compress4_transposed {
    (
        $h_vecs:expr,
        $msg_vecs:expr,
        $count_low:expr,
        $count_high:expr,
        $lastblock:expr,
        $lastnode:expr,
    ) => {
        let h_vecs: &mut [__m128i; 8] = $h_vecs;
        let msg_vecs: &[__m128i; 16] = $msg_vecs;
        let count_low: __m128i = $count_low;
        let count_high: __m128i = $count_high;
        let lastblock: __m128i = $lastblock;
        let lastnode: __m128i = $lastnode;
        let mut v = [
            h_vecs[0],
            h_vecs[1],
            h_vecs[2],
            h_vecs[3],
            h_vecs[4],
            h_vecs[5],
            h_vecs[6],
            h_vecs[7],
            set1(IV[0]),
            set1(IV[1]),
            set1(IV[2]),
            set1(IV[3]),
            xor(set1(IV[4]), count_low),
            xor(set1(IV[5]), count_high),
            xor(set1(IV[6]), lastblock),
            xor(set1(IV[7]), lastnode),
        ];

        round(&mut v, &msg_vecs, 0);
        round(&mut v, &msg_vecs, 1);
        round(&mut v, &msg_vecs, 2);
        round(&mut v, &msg_vecs, 3);
        round(&mut v, &msg_vecs, 4);
        round(&mut v, &msg_vecs, 5);
        round(&mut v, &msg_vecs, 6);
        round(&mut v, &msg_vecs, 7);
        round(&mut v, &msg_vecs, 8);
        round(&mut v, &msg_vecs, 9);

        h_vecs[0] = xor(xor(h_vecs[0], v[0]), v[8]);
        h_vecs[1] = xor(xor(h_vecs[1], v[1]), v[9]);
        h_vecs[2] = xor(xor(h_vecs[2], v[2]), v[10]);
        h_vecs[3] = xor(xor(h_vecs[3], v[3]), v[11]);
        h_vecs[4] = xor(xor(h_vecs[4], v[4]), v[12]);
        h_vecs[5] = xor(xor(h_vecs[5], v[5]), v[13]);
        h_vecs[6] = xor(xor(h_vecs[6], v[6]), v[14]);
        h_vecs[7] = xor(xor(h_vecs[7], v[7]), v[15]);
    };
}

#[inline(always)]
unsafe fn transpose_vecs(
    vec_a: __m128i,
    vec_b: __m128i,
    vec_c: __m128i,
    vec_d: __m128i,
) -> [__m128i; 4] {
    // Interleave 32-bit lates. The low unpack is lanes 00/11 and the high is
    // 22/33. Note that this doesn't split the vector into two lanes, as the
    // AVX2 counterparts do.
    let ab_01 = _mm_unpacklo_epi32(vec_a, vec_b);
    let ab_23 = _mm_unpackhi_epi32(vec_a, vec_b);
    let cd_01 = _mm_unpacklo_epi32(vec_c, vec_d);
    let cd_23 = _mm_unpackhi_epi32(vec_c, vec_d);

    // Interleave 64-bit lanes.
    let abcd_0 = _mm_unpacklo_epi64(ab_01, cd_01);
    let abcd_1 = _mm_unpackhi_epi64(ab_01, cd_01);
    let abcd_2 = _mm_unpacklo_epi64(ab_23, cd_23);
    let abcd_3 = _mm_unpackhi_epi64(ab_23, cd_23);

    [abcd_0, abcd_1, abcd_2, abcd_3]
}

#[inline(always)]
unsafe fn transpose_state_vecs(jobs: &[Job; DEGREE]) -> [__m128i; 8] {
    // Load all the state words into transposed vectors, where the first vector
    // has the first word of each state, etc. Transposing once at the beginning
    // and once at the end is more efficient that repeating it for each block.
    let words0 = array_refs!(&jobs[0].words, DEGREE, DEGREE);
    let words1 = array_refs!(&jobs[1].words, DEGREE, DEGREE);
    let words2 = array_refs!(&jobs[2].words, DEGREE, DEGREE);
    let words3 = array_refs!(&jobs[3].words, DEGREE, DEGREE);
    let [h0, h1, h2, h3] = transpose_vecs(
        loadu(words0.0),
        loadu(words1.0),
        loadu(words2.0),
        loadu(words3.0),
    );
    let [h4, h5, h6, h7] = transpose_vecs(
        loadu(words0.1),
        loadu(words1.1),
        loadu(words2.1),
        loadu(words3.1),
    );
    [h0, h1, h2, h3, h4, h5, h6, h7]
}

#[inline(always)]
unsafe fn untranspose_state_vecs(h_vecs: &[__m128i; 8], jobs: &mut [Job; DEGREE]) {
    // Un-transpose the updated state vectors back into the caller's arrays.
    let [job0, job1, job2, job3] = jobs;
    let words0 = mut_array_refs!(&mut job0.words, DEGREE, DEGREE);
    let words1 = mut_array_refs!(&mut job1.words, DEGREE, DEGREE);
    let words2 = mut_array_refs!(&mut job2.words, DEGREE, DEGREE);
    let words3 = mut_array_refs!(&mut job3.words, DEGREE, DEGREE);

    let out = transpose_vecs(h_vecs[0], h_vecs[1], h_vecs[2], h_vecs[3]);
    storeu(out[0], words0.0);
    storeu(out[1], words1.0);
    storeu(out[2], words2.0);
    storeu(out[3], words3.0);
    let out = transpose_vecs(h_vecs[4], h_vecs[5], h_vecs[6], h_vecs[7]);
    storeu(out[0], words0.1);
    storeu(out[1], words1.1);
    storeu(out[2], words2.1);
    storeu(out[3], words3.1);
}

#[inline(always)]
unsafe fn transpose_msg_vecs(blocks: [*const [u8; BLOCKBYTES]; DEGREE]) -> [__m128i; 16] {
    // These input arrays have no particular alignment, so we use unaligned
    // loads to read from them.
    let block0 = blocks[0] as *const [Word; DEGREE];
    let block1 = blocks[1] as *const [Word; DEGREE];
    let block2 = blocks[2] as *const [Word; DEGREE];
    let block3 = blocks[3] as *const [Word; DEGREE];
    let [m0, m1, m2, m3] = transpose_vecs(
        loadu(block0.add(0)),
        loadu(block1.add(0)),
        loadu(block2.add(0)),
        loadu(block3.add(0)),
    );
    let [m4, m5, m6, m7] = transpose_vecs(
        loadu(block0.add(1)),
        loadu(block1.add(1)),
        loadu(block2.add(1)),
        loadu(block3.add(1)),
    );
    let [m8, m9, m10, m11] = transpose_vecs(
        loadu(block0.add(2)),
        loadu(block1.add(2)),
        loadu(block2.add(2)),
        loadu(block3.add(2)),
    );
    let [m12, m13, m14, m15] = transpose_vecs(
        loadu(block0.add(3)),
        loadu(block1.add(3)),
        loadu(block2.add(3)),
        loadu(block3.add(3)),
    );
    [
        m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15,
    ]
}

#[inline(always)]
unsafe fn load_counts(jobs: &[Job; DEGREE]) -> (__m128i, __m128i) {
    (
        set4(
            count_low(jobs[0].count),
            count_low(jobs[1].count),
            count_low(jobs[2].count),
            count_low(jobs[3].count),
        ),
        set4(
            count_high(jobs[0].count),
            count_high(jobs[1].count),
            count_high(jobs[2].count),
            count_high(jobs[3].count),
        ),
    )
}

#[inline(always)]
unsafe fn store_counts(jobs: &mut [Job; DEGREE], low: __m128i, high: __m128i) {
    let low_ints: [Word; DEGREE] = mem::transmute(low);
    let high_ints: [Word; DEGREE] = mem::transmute(high);
    for i in 0..DEGREE {
        jobs[i].count = assemble_count(low_ints[i], high_ints[i]);
    }
}

#[inline(always)]
unsafe fn add_to_counts(lo: &mut __m128i, hi: &mut __m128i, delta: __m128i) {
    // If the low counts reach zero, that means they wrapped, unless the delta
    // was also zero.
    *lo = add(*lo, delta);
    let lo_reached_zero = eq(*lo, set1(0));
    let delta_was_zero = eq(delta, set1(0));
    let hi_inc = and(set1(1), negate_and(delta_was_zero, lo_reached_zero));
    *hi = add(*hi, hi_inc);
}

#[inline(always)]
unsafe fn flags_vec(flags: [bool; DEGREE]) -> __m128i {
    set4(
        flag_word(flags[0]),
        flag_word(flags[1]),
        flag_word(flags[2]),
        flag_word(flags[3]),
    )
}

#[target_feature(enable = "sse4.1")]
pub unsafe fn compress4_loop(jobs: &mut [Job; DEGREE], finalize: Finalize, stride: Stride) {
    // If we're not finalizing, there can't be a partial block at the end.
    for job in jobs.iter() {
        input_debug_asserts(job.input, finalize);
    }

    let msg_ptrs = [
        jobs[0].input.as_ptr(),
        jobs[1].input.as_ptr(),
        jobs[2].input.as_ptr(),
        jobs[3].input.as_ptr(),
    ];
    let mut h_vecs = transpose_state_vecs(&jobs);
    let (mut counts_lo, mut counts_hi) = load_counts(&jobs);

    // Prepare the final blocks (note, which could be empty if the input is
    // empty). Do all this before entering the main loop.
    let min_len = jobs.iter().map(|job| job.input.len()).min().unwrap();
    let mut fin_offset = min_len.saturating_sub(1);
    fin_offset -= fin_offset % stride.padded_blockbytes();
    // Performance note, making these buffers mem::uninitialized() seems to
    // cause problems in the optimizer.
    let mut buf0: [u8; BLOCKBYTES] = [0; BLOCKBYTES];
    let mut buf1: [u8; BLOCKBYTES] = [0; BLOCKBYTES];
    let mut buf2: [u8; BLOCKBYTES] = [0; BLOCKBYTES];
    let mut buf3: [u8; BLOCKBYTES] = [0; BLOCKBYTES];
    let (block0, len0, finalize0) = final_block(jobs[0].input, fin_offset, &mut buf0, stride);
    let (block1, len1, finalize1) = final_block(jobs[1].input, fin_offset, &mut buf1, stride);
    let (block2, len2, finalize2) = final_block(jobs[2].input, fin_offset, &mut buf2, stride);
    let (block3, len3, finalize3) = final_block(jobs[3].input, fin_offset, &mut buf3, stride);
    let fin_blocks: [*const [u8; BLOCKBYTES]; DEGREE] = [block0, block1, block2, block3];
    let fin_counts_delta = set4(len0 as Word, len1 as Word, len2 as Word, len3 as Word);
    let fin_last_block;
    let fin_last_node;
    if finalize.yes() {
        fin_last_block = flags_vec([finalize0, finalize1, finalize2, finalize3]);
        fin_last_node = flags_vec([
            finalize0 && jobs[0].last_node.yes(),
            finalize1 && jobs[1].last_node.yes(),
            finalize2 && jobs[2].last_node.yes(),
            finalize3 && jobs[3].last_node.yes(),
        ]);
    } else {
        fin_last_block = set1(0);
        fin_last_node = set1(0);
    }

    // The main loop.
    let mut offset = 0;
    loop {
        let blocks;
        let counts_delta;
        let last_block;
        let last_node;
        if offset == fin_offset {
            blocks = fin_blocks;
            counts_delta = fin_counts_delta;
            last_block = fin_last_block;
            last_node = fin_last_node;
        } else {
            blocks = [
                msg_ptrs[0].add(offset) as *const [u8; BLOCKBYTES],
                msg_ptrs[1].add(offset) as *const [u8; BLOCKBYTES],
                msg_ptrs[2].add(offset) as *const [u8; BLOCKBYTES],
                msg_ptrs[3].add(offset) as *const [u8; BLOCKBYTES],
            ];
            counts_delta = set1(BLOCKBYTES as Word);
            last_block = set1(0);
            last_node = set1(0);
        };

        let m_vecs = transpose_msg_vecs(blocks);
        add_to_counts(&mut counts_lo, &mut counts_hi, counts_delta);
        compress4_transposed!(
            &mut h_vecs,
            &m_vecs,
            counts_lo,
            counts_hi,
            last_block,
            last_node,
        );

        // Check for termination before bumping the offset, to avoid overflow.
        if offset == fin_offset {
            break;
        }

        offset += stride.padded_blockbytes();
    }

    // Write out the results.
    untranspose_state_vecs(&h_vecs, &mut *jobs);
    store_counts(&mut *jobs, counts_lo, counts_hi);
    let max_consumed = offset.saturating_add(stride.padded_blockbytes());
    for job in jobs.iter_mut() {
        let consumed = cmp::min(max_consumed, job.input.len());
        job.input = &job.input[consumed..];
    }
}
