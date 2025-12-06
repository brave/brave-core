#![cfg_attr(feature = "nightly", feature(avx512_target_feature, is_sorted))]

use std::iter::zip;

use criterion::{criterion_group, criterion_main, Criterion};
use pulp::*;

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[cfg(feature = "nightly")]
mod nightly {
    #![allow(
        clippy::identity_op,
        clippy::too_many_arguments,
        clippy::type_complexity
    )]

    use pulp::x86::V4;
    use pulp::{b8, cast, u64x8, NullaryFnOnce};
    const NETWORK_64BIT_1: u64x8 = u64x8(3, 2, 1, 0, 7, 6, 5, 4);
    const NETWORK_64BIT_2: u64x8 = u64x8(7, 6, 5, 4, 3, 2, 1, 0);
    const NETWORK_64BIT_3: u64x8 = u64x8(2, 3, 0, 1, 6, 7, 4, 5);
    const NETWORK_64BIT_4: u64x8 = u64x8(4, 5, 6, 7, 0, 1, 2, 3);

    #[inline(always)]
    fn get_pivot_64bit(simd: V4, a: &[u64]) -> u64 {
        assert!(!a.is_empty());
        let stride = ((a.len() - 1) / 8) as u64;
        let rand_index = u64x8(
            8 * stride,
            7 * stride,
            6 * stride,
            5 * stride,
            4 * stride,
            3 * stride,
            2 * stride,
            1 * stride,
        );
        let avx = simd.avx512f;
        let rand_vec =
            unsafe { avx._mm512_i64gather_epi64::<8>(cast(rand_index), a.as_ptr() as *const u8) };
        let sort = sort_zmm_64bit(simd, cast(rand_vec));
        sort.4
    }

    #[must_use]
    #[inline(always)]
    fn cmp_merge(simd: V4, in1: u64x8, in2: u64x8, mask: b8) -> u64x8 {
        let min = simd.min_u64x8(in1, in2);
        let max = simd.max_u64x8(in1, in2);
        simd.select_u64x8(mask, max, min)
    }

    #[must_use]
    #[inline(always)]
    fn permute(simd: V4, indices: u64x8, zmm: u64x8) -> u64x8 {
        cast(simd.avx512f._mm512_permutexvar_pd(cast(indices), cast(zmm)))
    }

    #[must_use]
    #[inline(always)]
    fn shuffle(simd: V4, zmm: u64x8) -> u64x8 {
        let avx = simd.avx512f;
        cast(avx._mm512_shuffle_pd::<0b01010101>(cast(zmm), cast(zmm)))
    }

    #[must_use]
    #[inline(always)]
    fn coex(simd: V4, a: u64x8, b: u64x8) -> (u64x8, u64x8) {
        (simd.min_u64x8(a, b), simd.max_u64x8(a, b))
    }

    /// Assumes zmm is random and performs a full sorting network defined in
    /// https://en.wikipedia.org/wiki/Bitonic_sorter#/media/File:BitonicSort.svg
    #[must_use]
    #[inline(always)]
    fn sort_zmm_64bit(simd: V4, zmm: u64x8) -> u64x8 {
        let zmm = cmp_merge(simd, zmm, shuffle(simd, zmm), b8(0xAA));
        let zmm = cmp_merge(simd, zmm, permute(simd, NETWORK_64BIT_1, zmm), b8(0xCC));
        let zmm = cmp_merge(simd, zmm, shuffle(simd, zmm), b8(0xAA));
        let zmm = cmp_merge(simd, zmm, permute(simd, NETWORK_64BIT_2, zmm), b8(0xF0));
        let zmm = cmp_merge(simd, zmm, permute(simd, NETWORK_64BIT_3, zmm), b8(0xCC));
        cmp_merge(simd, zmm, shuffle(simd, zmm), b8(0xAA))
    }

    /// Assumes zmm is bitonic and performs a recursive half cleaner
    #[must_use]
    #[inline(always)]
    fn bitonic_merge_zmm_64bit(simd: V4, zmm: u64x8) -> u64x8 {
        // 1) half_cleaner[8]: compare 0-4, 1-5, 2-6, 3-7
        let zmm = cmp_merge(simd, zmm, permute(simd, NETWORK_64BIT_4, zmm), b8(0xF0));
        // 2) half_cleaner[4]
        let zmm = cmp_merge(simd, zmm, permute(simd, NETWORK_64BIT_3, zmm), b8(0xCC));
        // 3) half_cleaner[1]
        cmp_merge(simd, zmm, shuffle(simd, zmm), b8(0xAA))
    }

    /// Assumes zmm0 and zmm1 are sorted and performs a recursive half cleaner
    #[must_use]
    #[inline(always)]
    fn bitonic_merge_two_zmm_64bit(simd: V4, zmm0: u64x8, zmm1: u64x8) -> (u64x8, u64x8) {
        // 1) First step of a merging network: coex of zmm0 and zmm1 reversed
        let zmm1 = permute(simd, NETWORK_64BIT_2, zmm1);
        let zmm2 = simd.min_u64x8(zmm0, zmm1);
        let zmm3 = simd.max_u64x8(zmm0, zmm1);
        // 2) Recursive half cleaner for each
        (
            bitonic_merge_zmm_64bit(simd, zmm2),
            bitonic_merge_zmm_64bit(simd, zmm3),
        )
    }

    #[must_use]
    #[inline(always)]
    fn bitonic_merge_four_zmm_64bit(
        simd: V4,
        zmm0: u64x8,
        zmm1: u64x8,
        zmm2: u64x8,
        zmm3: u64x8,
    ) -> (u64x8, u64x8, u64x8, u64x8) {
        // 1) First step of a merging network
        let zmm2r = permute(simd, NETWORK_64BIT_2, zmm2);
        let zmm3r = permute(simd, NETWORK_64BIT_2, zmm3);
        let zmm_t1 = simd.min_u64x8(zmm0, zmm3r);
        let zmm_t2 = simd.min_u64x8(zmm1, zmm2r);
        // 2) Recursive half clearer: 16
        let zmm_t3 = permute(simd, NETWORK_64BIT_2, simd.max_u64x8(zmm1, zmm2r));
        let zmm_t4 = permute(simd, NETWORK_64BIT_2, simd.max_u64x8(zmm0, zmm3r));
        let zmm0 = simd.min_u64x8(zmm_t1, zmm_t2);
        let zmm1 = simd.max_u64x8(zmm_t1, zmm_t2);
        let zmm2 = simd.min_u64x8(zmm_t3, zmm_t4);
        let zmm3 = simd.max_u64x8(zmm_t3, zmm_t4);

        (
            bitonic_merge_zmm_64bit(simd, zmm0),
            bitonic_merge_zmm_64bit(simd, zmm1),
            bitonic_merge_zmm_64bit(simd, zmm2),
            bitonic_merge_zmm_64bit(simd, zmm3),
        )
    }

    #[must_use]
    #[inline(always)]
    fn bitonic_merge_eight_zmm_64bit(
        simd: V4,
        zmm0: u64x8,
        zmm1: u64x8,
        zmm2: u64x8,
        zmm3: u64x8,
        zmm4: u64x8,
        zmm5: u64x8,
        zmm6: u64x8,
        zmm7: u64x8,
    ) -> (u64x8, u64x8, u64x8, u64x8, u64x8, u64x8, u64x8, u64x8) {
        let rev_index = NETWORK_64BIT_2;
        let zmm4r = permute(simd, rev_index, zmm4);
        let zmm5r = permute(simd, rev_index, zmm5);
        let zmm6r = permute(simd, rev_index, zmm6);
        let zmm7r = permute(simd, rev_index, zmm7);
        let zmm_t1 = simd.min_u64x8(zmm0, zmm7r);
        let zmm_t2 = simd.min_u64x8(zmm1, zmm6r);
        let zmm_t3 = simd.min_u64x8(zmm2, zmm5r);
        let zmm_t4 = simd.min_u64x8(zmm3, zmm4r);
        let zmm_t5 = permute(simd, rev_index, simd.max_u64x8(zmm3, zmm4r));
        let zmm_t6 = permute(simd, rev_index, simd.max_u64x8(zmm2, zmm5r));
        let zmm_t7 = permute(simd, rev_index, simd.max_u64x8(zmm1, zmm6r));
        let zmm_t8 = permute(simd, rev_index, simd.max_u64x8(zmm0, zmm7r));
        let (zmm_t1, zmm_t3) = coex(simd, zmm_t1, zmm_t3);
        let (zmm_t2, zmm_t4) = coex(simd, zmm_t2, zmm_t4);
        let (zmm_t5, zmm_t7) = coex(simd, zmm_t5, zmm_t7);
        let (zmm_t6, zmm_t8) = coex(simd, zmm_t6, zmm_t8);
        let (zmm_t1, zmm_t2) = coex(simd, zmm_t1, zmm_t2);
        let (zmm_t3, zmm_t4) = coex(simd, zmm_t3, zmm_t4);
        let (zmm_t5, zmm_t6) = coex(simd, zmm_t5, zmm_t6);
        let (zmm_t7, zmm_t8) = coex(simd, zmm_t7, zmm_t8);
        (
            bitonic_merge_zmm_64bit(simd, zmm_t1),
            bitonic_merge_zmm_64bit(simd, zmm_t2),
            bitonic_merge_zmm_64bit(simd, zmm_t3),
            bitonic_merge_zmm_64bit(simd, zmm_t4),
            bitonic_merge_zmm_64bit(simd, zmm_t5),
            bitonic_merge_zmm_64bit(simd, zmm_t6),
            bitonic_merge_zmm_64bit(simd, zmm_t7),
            bitonic_merge_zmm_64bit(simd, zmm_t8),
        )
    }

    #[must_use]
    #[inline(always)]
    fn bitonic_merge_sixteen_zmm_64bit(
        simd: V4,
        zmm0: u64x8,
        zmm1: u64x8,
        zmm2: u64x8,
        zmm3: u64x8,
        zmm4: u64x8,
        zmm5: u64x8,
        zmm6: u64x8,
        zmm7: u64x8,
        zmm8: u64x8,
        zmm9: u64x8,
        zmm10: u64x8,
        zmm11: u64x8,
        zmm12: u64x8,
        zmm13: u64x8,
        zmm14: u64x8,
        zmm15: u64x8,
    ) -> (
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
        u64x8,
    ) {
        let rev_index = NETWORK_64BIT_2;
        let zmm8r = permute(simd, rev_index, zmm8);
        let zmm9r = permute(simd, rev_index, zmm9);
        let zmm10r = permute(simd, rev_index, zmm10);
        let zmm11r = permute(simd, rev_index, zmm11);
        let zmm12r = permute(simd, rev_index, zmm12);
        let zmm13r = permute(simd, rev_index, zmm13);
        let zmm14r = permute(simd, rev_index, zmm14);
        let zmm15r = permute(simd, rev_index, zmm15);
        let zmm_t1 = simd.min_u64x8(zmm0, zmm15r);
        let zmm_t2 = simd.min_u64x8(zmm1, zmm14r);
        let zmm_t3 = simd.min_u64x8(zmm2, zmm13r);
        let zmm_t4 = simd.min_u64x8(zmm3, zmm12r);
        let zmm_t5 = simd.min_u64x8(zmm4, zmm11r);
        let zmm_t6 = simd.min_u64x8(zmm5, zmm10r);
        let zmm_t7 = simd.min_u64x8(zmm6, zmm9r);
        let zmm_t8 = simd.min_u64x8(zmm7, zmm8r);
        let zmm_t9 = permute(simd, rev_index, simd.max_u64x8(zmm7, zmm8r));
        let zmm_t10 = permute(simd, rev_index, simd.max_u64x8(zmm6, zmm9r));
        let zmm_t11 = permute(simd, rev_index, simd.max_u64x8(zmm5, zmm10r));
        let zmm_t12 = permute(simd, rev_index, simd.max_u64x8(zmm4, zmm11r));
        let zmm_t13 = permute(simd, rev_index, simd.max_u64x8(zmm3, zmm12r));
        let zmm_t14 = permute(simd, rev_index, simd.max_u64x8(zmm2, zmm13r));
        let zmm_t15 = permute(simd, rev_index, simd.max_u64x8(zmm1, zmm14r));
        let zmm_t16 = permute(simd, rev_index, simd.max_u64x8(zmm0, zmm15r));
        // Recusive half clear 16 zmm regs
        let (zmm_t1, zmm_t5) = coex(simd, zmm_t1, zmm_t5);
        let (zmm_t2, zmm_t6) = coex(simd, zmm_t2, zmm_t6);
        let (zmm_t3, zmm_t7) = coex(simd, zmm_t3, zmm_t7);
        let (zmm_t4, zmm_t8) = coex(simd, zmm_t4, zmm_t8);
        let (zmm_t9, zmm_t13) = coex(simd, zmm_t9, zmm_t13);
        let (zmm_t10, zmm_t14) = coex(simd, zmm_t10, zmm_t14);
        let (zmm_t11, zmm_t15) = coex(simd, zmm_t11, zmm_t15);
        let (zmm_t12, zmm_t16) = coex(simd, zmm_t12, zmm_t16);

        let (zmm_t1, zmm_t3) = coex(simd, zmm_t1, zmm_t3);
        let (zmm_t2, zmm_t4) = coex(simd, zmm_t2, zmm_t4);
        let (zmm_t5, zmm_t7) = coex(simd, zmm_t5, zmm_t7);
        let (zmm_t6, zmm_t8) = coex(simd, zmm_t6, zmm_t8);
        let (zmm_t9, zmm_t11) = coex(simd, zmm_t9, zmm_t11);
        let (zmm_t10, zmm_t12) = coex(simd, zmm_t10, zmm_t12);
        let (zmm_t13, zmm_t15) = coex(simd, zmm_t13, zmm_t15);
        let (zmm_t14, zmm_t16) = coex(simd, zmm_t14, zmm_t16);

        let (zmm_t1, zmm_t2) = coex(simd, zmm_t1, zmm_t2);
        let (zmm_t3, zmm_t4) = coex(simd, zmm_t3, zmm_t4);
        let (zmm_t5, zmm_t6) = coex(simd, zmm_t5, zmm_t6);
        let (zmm_t7, zmm_t8) = coex(simd, zmm_t7, zmm_t8);
        let (zmm_t9, zmm_t10) = coex(simd, zmm_t9, zmm_t10);
        let (zmm_t11, zmm_t12) = coex(simd, zmm_t11, zmm_t12);
        let (zmm_t13, zmm_t14) = coex(simd, zmm_t13, zmm_t14);
        let (zmm_t15, zmm_t16) = coex(simd, zmm_t15, zmm_t16);
        //
        (
            bitonic_merge_zmm_64bit(simd, zmm_t1),
            bitonic_merge_zmm_64bit(simd, zmm_t2),
            bitonic_merge_zmm_64bit(simd, zmm_t3),
            bitonic_merge_zmm_64bit(simd, zmm_t4),
            bitonic_merge_zmm_64bit(simd, zmm_t5),
            bitonic_merge_zmm_64bit(simd, zmm_t6),
            bitonic_merge_zmm_64bit(simd, zmm_t7),
            bitonic_merge_zmm_64bit(simd, zmm_t8),
            bitonic_merge_zmm_64bit(simd, zmm_t9),
            bitonic_merge_zmm_64bit(simd, zmm_t10),
            bitonic_merge_zmm_64bit(simd, zmm_t11),
            bitonic_merge_zmm_64bit(simd, zmm_t12),
            bitonic_merge_zmm_64bit(simd, zmm_t13),
            bitonic_merge_zmm_64bit(simd, zmm_t14),
            bitonic_merge_zmm_64bit(simd, zmm_t15),
            bitonic_merge_zmm_64bit(simd, zmm_t16),
        )
    }

    #[inline(always)]
    fn zeroing_shl(x: u64, shift: usize) -> u64 {
        x.checked_shl(shift as u32).unwrap_or(0)
    }

    #[inline(always)]
    pub fn load(simd: V4, a: &[u64], fill: u64) -> u64x8 {
        let n = a.len();
        let mask = b8(zeroing_shl(1, n).wrapping_sub(1) as u8);
        let avx = simd.avx512f;
        let fill = cast(simd.splat_u64x8(fill));
        unsafe { cast(avx._mm512_mask_loadu_epi64(fill, mask.0, a.as_ptr() as *const i64)) }
    }

    #[inline(always)]
    pub fn store(simd: V4, a: &mut [u64], v: u64x8) {
        let n = a.len();
        let mask = b8(zeroing_shl(1, n).wrapping_sub(1) as u8);
        let avx = simd.avx512f;
        unsafe { avx._mm512_mask_storeu_epi64(a.as_mut_ptr() as *mut i64, mask.0, cast(v)) };
    }

    #[inline(always)]
    pub fn load2(simd: V4, a: &[u64], fill: u64) -> (u64x8, u64x8) {
        let n = a.len();
        let combined_mask = zeroing_shl(1, n).wrapping_sub(1);
        let mask0 = combined_mask as u8;
        let mask1 = (combined_mask >> 8) as u8;

        let a = a.as_ptr() as *const i64;
        let avx = simd.avx512f;
        let fill = cast(simd.splat_u64x8(fill));
        unsafe {
            (
                cast(avx._mm512_mask_loadu_epi64(fill, mask0, a)),
                cast(avx._mm512_mask_loadu_epi64(fill, mask1, a.wrapping_add(8))),
            )
        }
    }

    #[inline(always)]
    pub fn store2(simd: V4, a: &mut [u64], v0: u64x8, v1: u64x8) {
        let n = a.len();
        let combined_mask = zeroing_shl(1, n).wrapping_sub(1);
        let mask0 = combined_mask as u8;
        let mask1 = (combined_mask >> 8) as u8;

        let a = a.as_mut_ptr() as *mut i64;
        let avx = simd.avx512f;
        unsafe {
            avx._mm512_mask_storeu_epi64(a, mask0, cast(v0));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(8), mask1, cast(v1));
        }
    }

    #[inline(always)]
    pub fn load4(simd: V4, a: &[u64], fill: u64) -> (u64x8, u64x8, u64x8, u64x8) {
        let n = a.len();
        let combined_mask = zeroing_shl(1, n).wrapping_sub(1);
        let mask0 = combined_mask as u8;
        let mask1 = (combined_mask >> 8) as u8;
        let mask2 = (combined_mask >> 16) as u8;
        let mask3 = (combined_mask >> 24) as u8;

        let a = a.as_ptr() as *const i64;

        let avx = simd.avx512f;
        let fill = cast(simd.splat_u64x8(fill));
        unsafe {
            (
                cast(avx._mm512_mask_loadu_epi64(fill, mask0, a)),
                cast(avx._mm512_mask_loadu_epi64(fill, mask1, a.wrapping_add(8))),
                cast(avx._mm512_mask_loadu_epi64(fill, mask2, a.wrapping_add(16))),
                cast(avx._mm512_mask_loadu_epi64(fill, mask3, a.wrapping_add(24))),
            )
        }
    }

    #[inline(always)]
    pub fn store4(simd: V4, a: &mut [u64], v0: u64x8, v1: u64x8, v2: u64x8, v3: u64x8) {
        let n = a.len();
        let combined_mask = zeroing_shl(1, n).wrapping_sub(1);
        let mask0 = combined_mask as u8;
        let mask1 = (combined_mask >> 8) as u8;
        let mask2 = (combined_mask >> 16) as u8;
        let mask3 = (combined_mask >> 24) as u8;

        let a = a.as_mut_ptr() as *mut i64;
        let avx = simd.avx512f;
        unsafe {
            avx._mm512_mask_storeu_epi64(a, mask0, cast(v0));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(8), mask1, cast(v1));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(16), mask2, cast(v2));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(24), mask3, cast(v3));
        }
    }

    #[inline(always)]
    pub fn load8(
        simd: V4,
        a: &[u64],
        fill: u64,
    ) -> (u64x8, u64x8, u64x8, u64x8, u64x8, u64x8, u64x8, u64x8) {
        let n = a.len();
        let combined_mask = zeroing_shl(1, n).wrapping_sub(1);
        let mask0 = combined_mask as u8;
        let mask1 = (combined_mask >> 8) as u8;
        let mask2 = (combined_mask >> 16) as u8;
        let mask3 = (combined_mask >> 24) as u8;
        let mask4 = (combined_mask >> 32) as u8;
        let mask5 = (combined_mask >> 40) as u8;
        let mask6 = (combined_mask >> 48) as u8;
        let mask7 = (combined_mask >> 56) as u8;

        let a = a.as_ptr() as *const i64;

        let avx = simd.avx512f;
        let fill = cast(simd.splat_u64x8(fill));
        unsafe {
            (
                cast(avx._mm512_mask_loadu_epi64(fill, mask0, a)),
                cast(avx._mm512_mask_loadu_epi64(fill, mask1, a.wrapping_add(8))),
                cast(avx._mm512_mask_loadu_epi64(fill, mask2, a.wrapping_add(16))),
                cast(avx._mm512_mask_loadu_epi64(fill, mask3, a.wrapping_add(24))),
                cast(avx._mm512_mask_loadu_epi64(fill, mask4, a.wrapping_add(32))),
                cast(avx._mm512_mask_loadu_epi64(fill, mask5, a.wrapping_add(40))),
                cast(avx._mm512_mask_loadu_epi64(fill, mask6, a.wrapping_add(48))),
                cast(avx._mm512_mask_loadu_epi64(fill, mask7, a.wrapping_add(56))),
            )
        }
    }

    #[inline(always)]
    pub fn store8(
        simd: V4,
        a: &mut [u64],
        v0: u64x8,
        v1: u64x8,
        v2: u64x8,
        v3: u64x8,
        v4: u64x8,
        v5: u64x8,
        v6: u64x8,
        v7: u64x8,
    ) {
        let n = a.len();
        let combined_mask = zeroing_shl(1, n).wrapping_sub(1);
        let mask0 = combined_mask as u8;
        let mask1 = (combined_mask >> 8) as u8;
        let mask2 = (combined_mask >> 16) as u8;
        let mask3 = (combined_mask >> 24) as u8;
        let mask4 = (combined_mask >> 32) as u8;
        let mask5 = (combined_mask >> 40) as u8;
        let mask6 = (combined_mask >> 48) as u8;
        let mask7 = (combined_mask >> 56) as u8;

        let a = a.as_mut_ptr() as *mut i64;
        let avx = simd.avx512f;
        unsafe {
            avx._mm512_mask_storeu_epi64(a, mask0, cast(v0));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(8), mask1, cast(v1));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(16), mask2, cast(v2));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(24), mask3, cast(v3));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(32), mask4, cast(v4));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(40), mask5, cast(v5));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(48), mask6, cast(v6));
            avx._mm512_mask_storeu_epi64(a.wrapping_add(56), mask7, cast(v7));
        }
    }

    #[inline(always)]
    pub fn sort_8_64bit(simd: V4, a: &mut [u64]) {
        let zmm = load(simd, a, u64::MAX);
        let zmm = sort_zmm_64bit(simd, zmm);
        store(simd, a, zmm);
    }

    #[inline(always)]
    pub fn sort_16_64bit(simd: V4, a: &mut [u64]) {
        let n = a.len();
        if n <= 8 {
            return sort_8_64bit(simd, a);
        }
        let (a0, a) = a.split_at_mut(8);
        let a0: &mut [u64; 8] = a0.try_into().unwrap();
        let zmm0 = cast(*a0);
        let zmm1 = load(simd, a, u64::MAX);

        let zmm0 = sort_zmm_64bit(simd, zmm0);
        let zmm1 = sort_zmm_64bit(simd, zmm1);

        let (zmm0, zmm1) = bitonic_merge_two_zmm_64bit(simd, zmm0, zmm1);

        *a0 = cast(zmm0);
        store(simd, a, zmm1);
    }

    #[inline(always)]
    pub fn sort_32_64bit(simd: V4, a: &mut [u64]) {
        let n = a.len();
        if n <= 16 {
            return sort_16_64bit(simd, a);
        }
        let (a0, a) = a.split_at_mut(8);
        let (a1, a) = a.split_at_mut(8);
        let a0: &mut [u64; 8] = a0.try_into().unwrap();
        let a1: &mut [u64; 8] = a1.try_into().unwrap();

        let zmm0 = cast(*a0);
        let zmm1 = cast(*a1);
        let (zmm2, zmm3) = load2(simd, a, u64::MAX);

        let zmm0 = sort_zmm_64bit(simd, zmm0);
        let zmm1 = sort_zmm_64bit(simd, zmm1);
        let zmm2 = sort_zmm_64bit(simd, zmm2);
        let zmm3 = sort_zmm_64bit(simd, zmm3);

        let (zmm0, zmm1) = bitonic_merge_two_zmm_64bit(simd, zmm0, zmm1);
        let (zmm2, zmm3) = bitonic_merge_two_zmm_64bit(simd, zmm2, zmm3);

        let (zmm0, zmm1, zmm2, zmm3) = bitonic_merge_four_zmm_64bit(simd, zmm0, zmm1, zmm2, zmm3);

        *a0 = cast(zmm0);
        *a1 = cast(zmm1);
        store2(simd, a, zmm2, zmm3);
    }

    #[inline(always)]
    pub fn sort_64_64bit(simd: V4, a: &mut [u64]) {
        let n = a.len();
        if n <= 32 {
            return sort_32_64bit(simd, a);
        }
        let (a0, a) = a.split_at_mut(8);
        let (a1, a) = a.split_at_mut(8);
        let (a2, a) = a.split_at_mut(8);
        let (a3, a) = a.split_at_mut(8);
        let a0: &mut [u64; 8] = a0.try_into().unwrap();
        let a1: &mut [u64; 8] = a1.try_into().unwrap();
        let a2: &mut [u64; 8] = a2.try_into().unwrap();
        let a3: &mut [u64; 8] = a3.try_into().unwrap();

        let zmm0 = cast(*a0);
        let zmm1 = cast(*a1);
        let zmm2 = cast(*a2);
        let zmm3 = cast(*a3);
        let (zmm4, zmm5, zmm6, zmm7) = load4(simd, a, u64::MAX);

        let zmm0 = sort_zmm_64bit(simd, zmm0);
        let zmm1 = sort_zmm_64bit(simd, zmm1);
        let zmm2 = sort_zmm_64bit(simd, zmm2);
        let zmm3 = sort_zmm_64bit(simd, zmm3);
        let zmm4 = sort_zmm_64bit(simd, zmm4);
        let zmm5 = sort_zmm_64bit(simd, zmm5);
        let zmm6 = sort_zmm_64bit(simd, zmm6);
        let zmm7 = sort_zmm_64bit(simd, zmm7);

        let (zmm0, zmm1) = bitonic_merge_two_zmm_64bit(simd, zmm0, zmm1);
        let (zmm2, zmm3) = bitonic_merge_two_zmm_64bit(simd, zmm2, zmm3);
        let (zmm4, zmm5) = bitonic_merge_two_zmm_64bit(simd, zmm4, zmm5);
        let (zmm6, zmm7) = bitonic_merge_two_zmm_64bit(simd, zmm6, zmm7);

        let (zmm0, zmm1, zmm2, zmm3) = bitonic_merge_four_zmm_64bit(simd, zmm0, zmm1, zmm2, zmm3);
        let (zmm4, zmm5, zmm6, zmm7) = bitonic_merge_four_zmm_64bit(simd, zmm4, zmm5, zmm6, zmm7);

        let (zmm0, zmm1, zmm2, zmm3, zmm4, zmm5, zmm6, zmm7) =
            bitonic_merge_eight_zmm_64bit(simd, zmm0, zmm1, zmm2, zmm3, zmm4, zmm5, zmm6, zmm7);

        *a0 = cast(zmm0);
        *a1 = cast(zmm1);
        *a2 = cast(zmm2);
        *a3 = cast(zmm3);
        store4(simd, a, zmm4, zmm5, zmm6, zmm7);
    }

    #[inline(always)]
    fn sort_128_64bit(simd: V4, a: &mut [u64]) {
        let n = a.len();
        if n <= 64 {
            return sort_64_64bit(simd, a);
        }
        let (a0, a) = a.split_at_mut(8);
        let (a1, a) = a.split_at_mut(8);
        let (a2, a) = a.split_at_mut(8);
        let (a3, a) = a.split_at_mut(8);
        let (a4, a) = a.split_at_mut(8);
        let (a5, a) = a.split_at_mut(8);
        let (a6, a) = a.split_at_mut(8);
        let (a7, a) = a.split_at_mut(8);
        let a0: &mut [u64; 8] = a0.try_into().unwrap();
        let a1: &mut [u64; 8] = a1.try_into().unwrap();
        let a2: &mut [u64; 8] = a2.try_into().unwrap();
        let a3: &mut [u64; 8] = a3.try_into().unwrap();
        let a4: &mut [u64; 8] = a4.try_into().unwrap();
        let a5: &mut [u64; 8] = a5.try_into().unwrap();
        let a6: &mut [u64; 8] = a6.try_into().unwrap();
        let a7: &mut [u64; 8] = a7.try_into().unwrap();

        let zmm0 = cast(*a0);
        let zmm1 = cast(*a1);
        let zmm2 = cast(*a2);
        let zmm3 = cast(*a3);
        let zmm4 = cast(*a4);
        let zmm5 = cast(*a5);
        let zmm6 = cast(*a6);
        let zmm7 = cast(*a7);
        let (zmm8, zmm9, zmm10, zmm11, zmm12, zmm13, zmm14, zmm15) = load8(simd, a, u64::MAX);

        let zmm0 = sort_zmm_64bit(simd, zmm0);
        let zmm1 = sort_zmm_64bit(simd, zmm1);
        let zmm2 = sort_zmm_64bit(simd, zmm2);
        let zmm3 = sort_zmm_64bit(simd, zmm3);
        let zmm4 = sort_zmm_64bit(simd, zmm4);
        let zmm5 = sort_zmm_64bit(simd, zmm5);
        let zmm6 = sort_zmm_64bit(simd, zmm6);
        let zmm7 = sort_zmm_64bit(simd, zmm7);
        let zmm8 = sort_zmm_64bit(simd, zmm8);
        let zmm9 = sort_zmm_64bit(simd, zmm9);
        let zmm10 = sort_zmm_64bit(simd, zmm10);
        let zmm11 = sort_zmm_64bit(simd, zmm11);
        let zmm12 = sort_zmm_64bit(simd, zmm12);
        let zmm13 = sort_zmm_64bit(simd, zmm13);
        let zmm14 = sort_zmm_64bit(simd, zmm14);
        let zmm15 = sort_zmm_64bit(simd, zmm15);

        let (zmm0, zmm1) = bitonic_merge_two_zmm_64bit(simd, zmm0, zmm1);
        let (zmm2, zmm3) = bitonic_merge_two_zmm_64bit(simd, zmm2, zmm3);
        let (zmm4, zmm5) = bitonic_merge_two_zmm_64bit(simd, zmm4, zmm5);
        let (zmm6, zmm7) = bitonic_merge_two_zmm_64bit(simd, zmm6, zmm7);
        let (zmm8, zmm9) = bitonic_merge_two_zmm_64bit(simd, zmm8, zmm9);
        let (zmm10, zmm11) = bitonic_merge_two_zmm_64bit(simd, zmm10, zmm11);
        let (zmm12, zmm13) = bitonic_merge_two_zmm_64bit(simd, zmm12, zmm13);
        let (zmm14, zmm15) = bitonic_merge_two_zmm_64bit(simd, zmm14, zmm15);

        let (zmm0, zmm1, zmm2, zmm3) = bitonic_merge_four_zmm_64bit(simd, zmm0, zmm1, zmm2, zmm3);
        let (zmm4, zmm5, zmm6, zmm7) = bitonic_merge_four_zmm_64bit(simd, zmm4, zmm5, zmm6, zmm7);
        let (zmm8, zmm9, zmm10, zmm11) =
            bitonic_merge_four_zmm_64bit(simd, zmm8, zmm9, zmm10, zmm11);
        let (zmm12, zmm13, zmm14, zmm15) =
            bitonic_merge_four_zmm_64bit(simd, zmm12, zmm13, zmm14, zmm15);

        let (zmm0, zmm1, zmm2, zmm3, zmm4, zmm5, zmm6, zmm7) =
            bitonic_merge_eight_zmm_64bit(simd, zmm0, zmm1, zmm2, zmm3, zmm4, zmm5, zmm6, zmm7);
        let (zmm8, zmm9, zmm10, zmm11, zmm12, zmm13, zmm14, zmm15) = bitonic_merge_eight_zmm_64bit(
            simd, zmm8, zmm9, zmm10, zmm11, zmm12, zmm13, zmm14, zmm15,
        );

        let (
            zmm0,
            zmm1,
            zmm2,
            zmm3,
            zmm4,
            zmm5,
            zmm6,
            zmm7,
            zmm8,
            zmm9,
            zmm10,
            zmm11,
            zmm12,
            zmm13,
            zmm14,
            zmm15,
        ) = bitonic_merge_sixteen_zmm_64bit(
            simd, zmm0, zmm1, zmm2, zmm3, zmm4, zmm5, zmm6, zmm7, zmm8, zmm9, zmm10, zmm11, zmm12,
            zmm13, zmm14, zmm15,
        );

        *a0 = cast(zmm0);
        *a1 = cast(zmm1);
        *a2 = cast(zmm2);
        *a3 = cast(zmm3);
        *a4 = cast(zmm4);
        *a5 = cast(zmm5);
        *a6 = cast(zmm6);
        *a7 = cast(zmm7);
        store8(
            simd, a, zmm8, zmm9, zmm10, zmm11, zmm12, zmm13, zmm14, zmm15,
        );
    }

    fn sort_u64_impl(simd: V4, a: &mut [u64], max_iters: u32) {
        struct Impl<'a> {
            simd: V4,
            a: &'a mut [u64],
            max_iters: u32,
        }

        impl NullaryFnOnce for Impl<'_> {
            type Output = ();

            #[inline(always)]
            fn call(self) -> Self::Output {
                let Self { simd, a, max_iters } = self;
                if max_iters == 0 {
                    a.sort_unstable();
                    return;
                }
                if a.len() <= 128 {
                    sort_128_64bit(simd, a);
                    return;
                }

                let pivot = get_pivot_64bit(simd, a);

                let (pivot_index, smallest, biggest) = partition_avx512(simd, a, pivot);
                let (left, right) = a.split_at_mut(pivot_index);

                if pivot != smallest {
                    sort_u64_impl(simd, left, max_iters - 1);
                }
                if pivot != biggest {
                    sort_u64_impl(simd, right, max_iters - 1);
                }
            }
        }
        simd.vectorize(Impl { simd, a, max_iters })
    }

    #[inline(always)]
    fn partition_avx512(simd: V4, arr: &mut [u64], pivot: u64) -> (usize, u64, u64) {
        let mut smallest = u64::MAX;
        let mut biggest = u64::MIN;

        let mut left = 0;
        let mut right = arr.len();

        let n = arr.len();

        for _ in 0..n % 8 {
            smallest = smallest.min(arr[left]);
            biggest = biggest.max(arr[left]);
            if arr[left] < pivot {
                left += 1;
            } else {
                right -= 1;
                arr.swap(left, right);
            }
        }

        if left == right {
            return (left, smallest, biggest);
        }

        let pivot_vec = simd.splat_u64x8(pivot);
        let mut min_vec = simd.splat_u64x8(smallest);
        let mut max_vec = simd.splat_u64x8(biggest);

        if right - left == 8 {
            unreachable!();
        }

        let vec_left: u64x8 = cast((<[u64; 8]>::try_from(&arr[left..left + 8])).unwrap());
        let vec_right: u64x8 = cast((<[u64; 8]>::try_from(&arr[right - 8..right])).unwrap());

        let mut l_store = left;
        let mut r_store = right - 8;

        left += 8;
        right -= 8;

        while right - left != 0 {
            let curr_vec: u64x8 = if r_store + 8 - right < left - l_store {
                right -= 8;
                cast((<[u64; 8]>::try_from(&arr[right - 8..right])).unwrap())
            } else {
                let v = cast((<[u64; 8]>::try_from(&arr[left..left + 8])).unwrap());
                left += 8;
                v
            };

            let amount_gt_pivot;
            (amount_gt_pivot, min_vec, max_vec) = unsafe {
                partition_vec(
                    simd,
                    arr,
                    l_store,
                    r_store + 8,
                    curr_vec,
                    pivot_vec,
                    min_vec,
                    max_vec,
                )
            };

            r_store -= amount_gt_pivot;
            l_store += 8 - amount_gt_pivot;
        }

        let amount_gt_pivot;
        (amount_gt_pivot, min_vec, max_vec) = unsafe {
            partition_vec(
                simd,
                arr,
                l_store,
                r_store + 8,
                vec_left,
                pivot_vec,
                min_vec,
                max_vec,
            )
        };
        l_store += 8 - amount_gt_pivot;

        let amount_gt_pivot;
        (amount_gt_pivot, min_vec, max_vec) = unsafe {
            partition_vec(
                simd,
                arr,
                l_store,
                l_store + 8,
                vec_right,
                pivot_vec,
                min_vec,
                max_vec,
            )
        };
        l_store += 8 - amount_gt_pivot;
        smallest = simd.avx512f._mm512_reduce_min_epu64(cast(min_vec));
        biggest = simd.avx512f._mm512_reduce_max_epu64(cast(max_vec));

        (l_store, smallest, biggest)
    }

    #[inline(always)]
    unsafe fn partition_vec(
        simd: V4,
        arr: &mut [u64],
        left: usize,
        right: usize,
        curr_vec: u64x8,
        pivot_vec: u64x8,
        min_vec: u64x8,
        max_vec: u64x8,
    ) -> (usize, u64x8, u64x8) {
        let gt_mask = simd.cmp_ge_u64x8(curr_vec, pivot_vec);
        let amount_gt_pivot = gt_mask.0.count_ones() as usize;
        let avx = simd.avx512f;
        let arr = arr.as_mut_ptr();
        let left = arr.wrapping_add(left) as _;
        let right = arr.wrapping_add(right - amount_gt_pivot) as _;
        unsafe {
            avx._mm512_mask_compressstoreu_epi64(left, !gt_mask.0, cast(curr_vec));
            avx._mm512_mask_compressstoreu_epi64(right, gt_mask.0, cast(curr_vec));
        };

        (
            amount_gt_pivot,
            simd.min_u64x8(curr_vec, min_vec),
            simd.max_u64x8(curr_vec, max_vec),
        )
    }

    pub fn sort_u64(simd: V4, data: &mut [u64]) {
        if data.len() > 1 {
            sort_u64_impl(simd, data, 2 * data.len().ilog2())
        }
    }
}

fn criterion_bench(criterion: &mut Criterion) {
    let _ = &mut *criterion;
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    #[cfg(feature = "nightly")]
    if let Some(simd) = pulp::x86::V4::try_new() {
        for n in [10000, 100000, 1000000] {
            let mut orig = vec![0; n];
            for x in &mut orig {
                *x = rand::random();
            }
            let mut buf = orig.clone();
            criterion.bench_function(&format!("avx512-sort-{n}"), |bencher| {
                bencher.iter(|| {
                    buf.copy_from_slice(&orig);
                    nightly::sort_u64(simd, &mut buf);
                });
            });
            criterion.bench_function(&format!("std-sort-{n}"), |bencher| {
                bencher.iter(|| {
                    buf.copy_from_slice(&orig);
                    buf.sort_unstable();
                });
            });
        }
    }
    let _ = criterion;
}

fn aligned_sum_vertical_bench(criterion: &mut Criterion) {
    #[repr(align(128))]
    #[derive(Copy, Clone, Debug)]
    struct Aligned<T>(T);

    use rand::{Rng, SeedableRng};

    let mut rng = rand::rngs::StdRng::seed_from_u64(0);

    let nan = f64::NAN;
    const N: usize = 32190;
    let data = core::array::from_fn::<f64, N, _>(|_| rng.gen());
    let unaligned_data = Aligned(core::array::from_fn::<f64, { N + 3 }, _>(|i| {
        if i < 3 {
            nan
        } else {
            data[i - 3]
        }
    }));

    let mut unaligned_dst = Aligned(core::array::from_fn::<f64, { N + 3 }, _>(|i| {
        if i < 3 {
            nan
        } else {
            data[i - 3]
        }
    }));
    let lhs = &unaligned_data.0[3..];
    let rhs = &unaligned_data.0[3..];
    let dst = &mut unaligned_dst.0[3..];

    let arch = Arch::new();

    struct Sum<'a> {
        lhs: &'a [f64],
        rhs: &'a [f64],
        dst: &'a mut [f64],
    }
    struct AlignedSum<'a> {
        lhs: &'a [f64],
        rhs: &'a [f64],
        dst: &'a mut [f64],
    }

    impl WithSimd for Sum<'_> {
        type Output = ();

        #[inline(always)]
        fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
            let (lhs_head, lhs_tail) = S::f64s_as_simd(self.lhs);
            let (rhs_head, rhs_tail) = S::f64s_as_simd(self.rhs);
            let (dst_head, dst_tail) = S::f64s_as_mut_simd(self.dst);

            for (dst, (lhs, rhs)) in zip(dst_head, zip(lhs_head, rhs_head)) {
                *dst = simd.f64s_add(*lhs, *rhs);
            }
            simd.f64s_partial_store(
                dst_tail,
                simd.f64s_add(
                    simd.f64s_partial_load(lhs_tail),
                    simd.f64s_partial_load(rhs_tail),
                ),
            );
        }
    }

    impl WithSimd for AlignedSum<'_> {
        type Output = ();

        #[inline(always)]
        fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
            let offset = simd.f64s_align_offset(self.dst.as_ptr(), self.dst.len());
            let (lhs_head, lhs_body, lhs_tail) = simd.f64s_as_aligned_simd(self.lhs, offset);
            let (rhs_head, rhs_body, rhs_tail) = simd.f64s_as_aligned_simd(self.rhs, offset);
            let (mut dst_head, dst_body, mut dst_tail) =
                simd.f64s_as_aligned_mut_simd(self.dst, offset);

            let zero = simd.f64s_splat(0.0);
            dst_head.write(simd.f64s_add(lhs_head.read_or(zero), rhs_head.read_or(zero)));
            for (dst, (lhs, rhs)) in zip(dst_body, zip(lhs_body, rhs_body)) {
                *dst = simd.f64s_add(*lhs, *rhs);
            }
            dst_tail.write(simd.f64s_add(lhs_tail.read_or(zero), rhs_tail.read_or(zero)));
        }
    }

    criterion.bench_function("sum-vertical-unaligned", |bencher| {
        bencher.iter(|| arch.dispatch(Sum { lhs, rhs, dst }));
    });
    criterion.bench_function("sum-vertical-aligned", |bencher| {
        bencher.iter(|| arch.dispatch(AlignedSum { lhs, rhs, dst }));
    });
}

fn aligned_sum_reduce_bench(criterion: &mut Criterion) {
    #[repr(align(128))]
    #[derive(Copy, Clone, Debug)]
    struct Aligned<T>(T);

    use rand::{Rng, SeedableRng};

    let mut rng = rand::rngs::StdRng::seed_from_u64(0);

    let nan = f64::NAN;
    const N: usize = 32190;
    let data = core::array::from_fn::<f64, N, _>(|_| rng.gen());
    let unaligned_data = Aligned(core::array::from_fn::<f64, { N + 3 }, _>(|i| {
        if i < 3 {
            nan
        } else {
            data[i - 3]
        }
    }));
    let data = &unaligned_data.0[3..];

    let arch = Arch::new();

    struct Sum<'a> {
        slice: &'a [f64],
    }
    struct AlignedSum<'a> {
        slice: &'a [f64],
    }
    struct WrongAlignedSum<'a> {
        slice: &'a [f64],
    }

    impl WithSimd for Sum<'_> {
        type Output = f64;

        #[inline(always)]
        fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
            let mut sum0 = simd.f64s_splat(0.0);
            let mut sum1 = simd.f64s_splat(0.0);
            let mut sum2 = simd.f64s_splat(0.0);
            let mut sum3 = simd.f64s_splat(0.0);

            let (head, tail) = S::f64s_as_simd(self.slice);
            let (head4, head1) = as_arrays::<4, _>(head);
            for &[x0, x1, x2, x3] in head4 {
                sum0 = simd.f64s_add(sum0, x0);
                sum1 = simd.f64s_add(sum1, x1);
                sum2 = simd.f64s_add(sum2, x2);
                sum3 = simd.f64s_add(sum3, x3);
            }
            for &x0 in head1 {
                sum0 = simd.f64s_add(sum0, x0);
            }
            sum0 = simd.f64s_add(sum0, simd.f64s_partial_load(tail));
            sum0 = simd.f64s_add(simd.f64s_add(sum0, sum1), simd.f64s_add(sum2, sum3));

            simd.f64s_reduce_sum(sum0)
        }
    }

    impl WithSimd for AlignedSum<'_> {
        type Output = f64;

        #[inline(always)]
        fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
            let offset = simd.f64s_align_offset(self.slice.as_ptr(), self.slice.len());
            let (prefix, body, suffix) = simd.f64s_as_aligned_simd(self.slice, offset);

            let mut sum0 = prefix.read_or(simd.f64s_splat(0.0));
            let mut sum1 = simd.f64s_splat(0.0);
            let mut sum2 = simd.f64s_splat(0.0);
            let mut sum3 = simd.f64s_splat(0.0);
            let (body4, body1) = as_arrays::<4, _>(body);
            for &[x0, x1, x2, x3] in body4 {
                sum0 = simd.f64s_add(sum0, x0);
                sum1 = simd.f64s_add(sum1, x1);
                sum2 = simd.f64s_add(sum2, x2);
                sum3 = simd.f64s_add(sum3, x3);
            }
            for &x0 in body1 {
                sum0 = simd.f64s_add(sum0, x0);
            }
            sum0 = simd.f64s_add(sum0, suffix.read_or(simd.f64s_splat(0.0)));
            sum0 = simd.f64s_add(simd.f64s_add(sum0, sum1), simd.f64s_add(sum2, sum3));

            simd.f64s_reduce_sum(simd.f64s_rotate_left(sum0, offset.rotate_left_amount()))
        }
    }

    impl WithSimd for WrongAlignedSum<'_> {
        type Output = f64;

        #[inline(always)]
        fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
            let offset = simd.f64s_align_offset(self.slice.as_ptr(), self.slice.len());
            let (prefix, body, suffix) = simd.f64s_as_aligned_simd(self.slice, offset);

            let mut sum0 = prefix.read_or(simd.f64s_splat(0.0));
            let mut sum1 = simd.f64s_splat(0.0);
            let mut sum2 = simd.f64s_splat(0.0);
            let mut sum3 = simd.f64s_splat(0.0);
            let (body4, body1) = as_arrays::<4, _>(body);
            for &[x0, x1, x2, x3] in body4 {
                sum0 = simd.f64s_add(sum0, x0);
                sum1 = simd.f64s_add(sum1, x1);
                sum2 = simd.f64s_add(sum2, x2);
                sum3 = simd.f64s_add(sum3, x3);
            }
            for &x0 in body1 {
                sum0 = simd.f64s_add(sum0, x0);
            }
            sum0 = simd.f64s_add(sum0, suffix.read_or(simd.f64s_splat(0.0)));
            sum0 = simd.f64s_add(simd.f64s_add(sum0, sum1), simd.f64s_add(sum2, sum3));

            simd.f64s_reduce_sum(sum0)
        }
    }

    criterion.bench_function("sum-reduce-unaligned", |bencher| {
        bencher.iter(|| arch.dispatch(Sum { slice: data }));
    });
    criterion.bench_function("sum-reduce-aligned", |bencher| {
        bencher.iter(|| arch.dispatch(AlignedSum { slice: data }));
    });
    criterion.bench_function("sum-reduce-aligned-wrong", |bencher| {
        bencher.iter(|| arch.dispatch(WrongAlignedSum { slice: data }));
    });
}

criterion_group!(
    benches,
    criterion_bench,
    aligned_sum_reduce_bench,
    aligned_sum_vertical_bench
);
criterion_main!(benches);
