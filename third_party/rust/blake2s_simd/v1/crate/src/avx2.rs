#[cfg(target_arch = "x86")]
use core::arch::x86::*;
#[cfg(target_arch = "x86_64")]
use core::arch::x86_64::*;

use crate::guts::{
    assemble_count, count_high, count_low, final_block, flag_word, input_debug_asserts, Finalize,
    Job, Stride,
};
use crate::{Word, BLOCKBYTES, IV, SIGMA};
use core::cmp;
use core::mem;

pub const DEGREE: usize = 8;

#[inline(always)]
unsafe fn loadu(src: *const [Word; DEGREE]) -> __m256i {
    // This is an unaligned load, so the pointer cast is allowed.
    _mm256_loadu_si256(src as *const __m256i)
}

#[inline(always)]
unsafe fn storeu(src: __m256i, dest: *mut [Word; DEGREE]) {
    // This is an unaligned store, so the pointer cast is allowed.
    _mm256_storeu_si256(dest as *mut __m256i, src)
}

#[inline(always)]
unsafe fn add(a: __m256i, b: __m256i) -> __m256i {
    _mm256_add_epi32(a, b)
}

#[inline(always)]
unsafe fn eq(a: __m256i, b: __m256i) -> __m256i {
    _mm256_cmpeq_epi32(a, b)
}

#[inline(always)]
unsafe fn and(a: __m256i, b: __m256i) -> __m256i {
    _mm256_and_si256(a, b)
}

#[inline(always)]
unsafe fn negate_and(a: __m256i, b: __m256i) -> __m256i {
    // Note that "and not" implies the reverse of the actual arg order.
    _mm256_andnot_si256(a, b)
}

#[inline(always)]
unsafe fn xor(a: __m256i, b: __m256i) -> __m256i {
    _mm256_xor_si256(a, b)
}

#[inline(always)]
unsafe fn set1(x: u32) -> __m256i {
    _mm256_set1_epi32(x as i32)
}

#[inline(always)]
unsafe fn set8(a: u32, b: u32, c: u32, d: u32, e: u32, f: u32, g: u32, h: u32) -> __m256i {
    _mm256_setr_epi32(
        a as i32, b as i32, c as i32, d as i32, e as i32, f as i32, g as i32, h as i32,
    )
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
unsafe fn rot16(x: __m256i) -> __m256i {
    _mm256_or_si256(_mm256_srli_epi32(x, 16), _mm256_slli_epi32(x, 32 - 16))
}

#[inline(always)]
unsafe fn rot12(x: __m256i) -> __m256i {
    _mm256_or_si256(_mm256_srli_epi32(x, 12), _mm256_slli_epi32(x, 32 - 12))
}

#[inline(always)]
unsafe fn rot8(x: __m256i) -> __m256i {
    _mm256_or_si256(_mm256_srli_epi32(x, 8), _mm256_slli_epi32(x, 32 - 8))
}

#[inline(always)]
unsafe fn rot7(x: __m256i) -> __m256i {
    _mm256_or_si256(_mm256_srli_epi32(x, 7), _mm256_slli_epi32(x, 32 - 7))
}

#[inline(always)]
unsafe fn round(v: &mut [__m256i; 16], m: &[__m256i; 16], r: usize) {
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
macro_rules! compress8_transposed {
    (
        $h_vecs:expr,
        $msg_vecs:expr,
        $count_low:expr,
        $count_high:expr,
        $lastblock:expr,
        $lastnode:expr,
    ) => {
        let h_vecs: &mut [__m256i; 8] = $h_vecs;
        let msg_vecs: &[__m256i; 16] = $msg_vecs;
        let count_low: __m256i = $count_low;
        let count_high: __m256i = $count_high;
        let lastblock: __m256i = $lastblock;
        let lastnode: __m256i = $lastnode;

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
unsafe fn interleave128(a: __m256i, b: __m256i) -> (__m256i, __m256i) {
    (
        _mm256_permute2x128_si256(a, b, 0x20),
        _mm256_permute2x128_si256(a, b, 0x31),
    )
}

// There are several ways to do a transposition. We could do it naively, with 8 separate
// _mm256_set_epi32 instructions, referencing each of the 32 words explicitly. Or we could copy
// the vecs into contiguous storage and then use gather instructions. This third approach is to use
// a series of unpack instructions to interleave the vectors. In my benchmarks, interleaving is the
// fastest approach. To test this, run `cargo +nightly bench --bench libtest load_8` in the
// https://github.com/oconnor663/bao_experiments repo.
#[inline(always)]
unsafe fn transpose_vecs(
    vec_a: __m256i,
    vec_b: __m256i,
    vec_c: __m256i,
    vec_d: __m256i,
    vec_e: __m256i,
    vec_f: __m256i,
    vec_g: __m256i,
    vec_h: __m256i,
) -> [__m256i; 8] {
    // Interleave 32-bit lanes. The low unpack is lanes 00/11/44/55, and the high is 22/33/66/77.
    let ab_0145 = _mm256_unpacklo_epi32(vec_a, vec_b);
    let ab_2367 = _mm256_unpackhi_epi32(vec_a, vec_b);
    let cd_0145 = _mm256_unpacklo_epi32(vec_c, vec_d);
    let cd_2367 = _mm256_unpackhi_epi32(vec_c, vec_d);
    let ef_0145 = _mm256_unpacklo_epi32(vec_e, vec_f);
    let ef_2367 = _mm256_unpackhi_epi32(vec_e, vec_f);
    let gh_0145 = _mm256_unpacklo_epi32(vec_g, vec_h);
    let gh_2367 = _mm256_unpackhi_epi32(vec_g, vec_h);

    // Interleave 64-bit lates. The low unpack is lanes 00/22 and the high is 11/33.
    let abcd_04 = _mm256_unpacklo_epi64(ab_0145, cd_0145);
    let abcd_15 = _mm256_unpackhi_epi64(ab_0145, cd_0145);
    let abcd_26 = _mm256_unpacklo_epi64(ab_2367, cd_2367);
    let abcd_37 = _mm256_unpackhi_epi64(ab_2367, cd_2367);
    let efgh_04 = _mm256_unpacklo_epi64(ef_0145, gh_0145);
    let efgh_15 = _mm256_unpackhi_epi64(ef_0145, gh_0145);
    let efgh_26 = _mm256_unpacklo_epi64(ef_2367, gh_2367);
    let efgh_37 = _mm256_unpackhi_epi64(ef_2367, gh_2367);

    // Interleave 128-bit lanes.
    let (abcdefgh_0, abcdefgh_4) = interleave128(abcd_04, efgh_04);
    let (abcdefgh_1, abcdefgh_5) = interleave128(abcd_15, efgh_15);
    let (abcdefgh_2, abcdefgh_6) = interleave128(abcd_26, efgh_26);
    let (abcdefgh_3, abcdefgh_7) = interleave128(abcd_37, efgh_37);

    [
        abcdefgh_0, abcdefgh_1, abcdefgh_2, abcdefgh_3, abcdefgh_4, abcdefgh_5, abcdefgh_6,
        abcdefgh_7,
    ]
}

#[inline(always)]
unsafe fn transpose_state_vecs(jobs: &[Job; DEGREE]) -> [__m256i; 8] {
    // Load all the state words into transposed vectors, where the first vector
    // has the first word of each state, etc. Transposing once at the beginning
    // and once at the end is more efficient that repeating it for each block.
    transpose_vecs(
        loadu(jobs[0].words),
        loadu(jobs[1].words),
        loadu(jobs[2].words),
        loadu(jobs[3].words),
        loadu(jobs[4].words),
        loadu(jobs[5].words),
        loadu(jobs[6].words),
        loadu(jobs[7].words),
    )
}

#[inline(always)]
unsafe fn untranspose_state_vecs(h_vecs: &[__m256i; 8], jobs: &mut [Job; DEGREE]) {
    // Un-transpose the updated state vectors back into the caller's arrays.
    let out = transpose_vecs(
        h_vecs[0], h_vecs[1], h_vecs[2], h_vecs[3], h_vecs[4], h_vecs[5], h_vecs[6], h_vecs[7],
    );
    storeu(out[0], jobs[0].words);
    storeu(out[1], jobs[1].words);
    storeu(out[2], jobs[2].words);
    storeu(out[3], jobs[3].words);
    storeu(out[4], jobs[4].words);
    storeu(out[5], jobs[5].words);
    storeu(out[6], jobs[6].words);
    storeu(out[7], jobs[7].words);
}

#[inline(always)]
unsafe fn transpose_msg_vecs(blocks: [*const [u8; BLOCKBYTES]; DEGREE]) -> [__m256i; 16] {
    // These input arrays have no particular alignment, so we use unaligned
    // loads to read from them.
    let block0 = blocks[0] as *const [Word; DEGREE];
    let block1 = blocks[1] as *const [Word; DEGREE];
    let block2 = blocks[2] as *const [Word; DEGREE];
    let block3 = blocks[3] as *const [Word; DEGREE];
    let block4 = blocks[4] as *const [Word; DEGREE];
    let block5 = blocks[5] as *const [Word; DEGREE];
    let block6 = blocks[6] as *const [Word; DEGREE];
    let block7 = blocks[7] as *const [Word; DEGREE];
    let [m0, m1, m2, m3, m4, m5, m6, m7] = transpose_vecs(
        loadu(block0.add(0)),
        loadu(block1.add(0)),
        loadu(block2.add(0)),
        loadu(block3.add(0)),
        loadu(block4.add(0)),
        loadu(block5.add(0)),
        loadu(block6.add(0)),
        loadu(block7.add(0)),
    );
    let [m8, m9, m10, m11, m12, m13, m14, m15] = transpose_vecs(
        loadu(block0.add(1)),
        loadu(block1.add(1)),
        loadu(block2.add(1)),
        loadu(block3.add(1)),
        loadu(block4.add(1)),
        loadu(block5.add(1)),
        loadu(block6.add(1)),
        loadu(block7.add(1)),
    );
    [
        m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15,
    ]
}

#[inline(always)]
unsafe fn load_counts(jobs: &[Job; DEGREE]) -> (__m256i, __m256i) {
    (
        set8(
            count_low(jobs[0].count),
            count_low(jobs[1].count),
            count_low(jobs[2].count),
            count_low(jobs[3].count),
            count_low(jobs[4].count),
            count_low(jobs[5].count),
            count_low(jobs[6].count),
            count_low(jobs[7].count),
        ),
        set8(
            count_high(jobs[0].count),
            count_high(jobs[1].count),
            count_high(jobs[2].count),
            count_high(jobs[3].count),
            count_high(jobs[4].count),
            count_high(jobs[5].count),
            count_high(jobs[6].count),
            count_high(jobs[7].count),
        ),
    )
}

#[inline(always)]
unsafe fn store_counts(jobs: &mut [Job; DEGREE], low: __m256i, high: __m256i) {
    let low_ints: [Word; DEGREE] = mem::transmute(low);
    let high_ints: [Word; DEGREE] = mem::transmute(high);
    for i in 0..DEGREE {
        jobs[i].count = assemble_count(low_ints[i], high_ints[i]);
    }
}

#[inline(always)]
unsafe fn add_to_counts(lo: &mut __m256i, hi: &mut __m256i, delta: __m256i) {
    // If the low counts reach zero, that means they wrapped, unless the delta
    // was also zero.
    *lo = add(*lo, delta);
    let lo_reached_zero = eq(*lo, set1(0));
    let delta_was_zero = eq(delta, set1(0));
    let hi_inc = and(set1(1), negate_and(delta_was_zero, lo_reached_zero));
    *hi = add(*hi, hi_inc);
}

#[inline(always)]
unsafe fn flags_vec(flags: [bool; DEGREE]) -> __m256i {
    set8(
        flag_word(flags[0]),
        flag_word(flags[1]),
        flag_word(flags[2]),
        flag_word(flags[3]),
        flag_word(flags[4]),
        flag_word(flags[5]),
        flag_word(flags[6]),
        flag_word(flags[7]),
    )
}

#[target_feature(enable = "avx2")]
pub unsafe fn compress8_loop(jobs: &mut [Job; DEGREE], finalize: Finalize, stride: Stride) {
    // If we're not finalizing, there can't be a partial block at the end.
    for job in jobs.iter() {
        input_debug_asserts(job.input, finalize);
    }

    let msg_ptrs = [
        jobs[0].input.as_ptr(),
        jobs[1].input.as_ptr(),
        jobs[2].input.as_ptr(),
        jobs[3].input.as_ptr(),
        jobs[4].input.as_ptr(),
        jobs[5].input.as_ptr(),
        jobs[6].input.as_ptr(),
        jobs[7].input.as_ptr(),
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
    let mut buf4: [u8; BLOCKBYTES] = [0; BLOCKBYTES];
    let mut buf5: [u8; BLOCKBYTES] = [0; BLOCKBYTES];
    let mut buf6: [u8; BLOCKBYTES] = [0; BLOCKBYTES];
    let mut buf7: [u8; BLOCKBYTES] = [0; BLOCKBYTES];
    let (block0, len0, finalize0) = final_block(jobs[0].input, fin_offset, &mut buf0, stride);
    let (block1, len1, finalize1) = final_block(jobs[1].input, fin_offset, &mut buf1, stride);
    let (block2, len2, finalize2) = final_block(jobs[2].input, fin_offset, &mut buf2, stride);
    let (block3, len3, finalize3) = final_block(jobs[3].input, fin_offset, &mut buf3, stride);
    let (block4, len4, finalize4) = final_block(jobs[4].input, fin_offset, &mut buf4, stride);
    let (block5, len5, finalize5) = final_block(jobs[5].input, fin_offset, &mut buf5, stride);
    let (block6, len6, finalize6) = final_block(jobs[6].input, fin_offset, &mut buf6, stride);
    let (block7, len7, finalize7) = final_block(jobs[7].input, fin_offset, &mut buf7, stride);
    let fin_blocks: [*const [u8; BLOCKBYTES]; DEGREE] = [
        block0, block1, block2, block3, block4, block5, block6, block7,
    ];
    let fin_counts_delta = set8(
        len0 as Word,
        len1 as Word,
        len2 as Word,
        len3 as Word,
        len4 as Word,
        len5 as Word,
        len6 as Word,
        len7 as Word,
    );
    let fin_last_block;
    let fin_last_node;
    if finalize.yes() {
        fin_last_block = flags_vec([
            finalize0, finalize1, finalize2, finalize3, finalize4, finalize5, finalize6, finalize7,
        ]);
        fin_last_node = flags_vec([
            finalize0 && jobs[0].last_node.yes(),
            finalize1 && jobs[1].last_node.yes(),
            finalize2 && jobs[2].last_node.yes(),
            finalize3 && jobs[3].last_node.yes(),
            finalize4 && jobs[4].last_node.yes(),
            finalize5 && jobs[5].last_node.yes(),
            finalize6 && jobs[6].last_node.yes(),
            finalize7 && jobs[7].last_node.yes(),
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
                msg_ptrs[4].add(offset) as *const [u8; BLOCKBYTES],
                msg_ptrs[5].add(offset) as *const [u8; BLOCKBYTES],
                msg_ptrs[6].add(offset) as *const [u8; BLOCKBYTES],
                msg_ptrs[7].add(offset) as *const [u8; BLOCKBYTES],
            ];
            counts_delta = set1(BLOCKBYTES as Word);
            last_block = set1(0);
            last_node = set1(0);
        };

        let m_vecs = transpose_msg_vecs(blocks);
        add_to_counts(&mut counts_lo, &mut counts_hi, counts_delta);
        compress8_transposed!(
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
