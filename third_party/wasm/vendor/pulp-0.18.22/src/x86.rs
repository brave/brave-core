use super::*;
use crate::core_arch::internal_simd_type;
#[cfg(feature = "nightly")]
use crate::core_arch::x86::{Avx512bw_Avx512vl, Avx512f_Avx512vl};
#[cfg(target_arch = "x86")]
use core::arch::x86::*;
#[cfg(target_arch = "x86_64")]
use core::arch::x86_64::*;

use core::mem::transmute;

// x86-32 wants to use a 32-bit address size, but asm! defaults to using the full
// register name (e.g. rax). We have to explicitly override the placeholder to
// use the 32-bit register name in that case.

#[cfg(feature = "nightly")]
#[cfg(target_pointer_width = "32")]
macro_rules! vpl {
    ($inst:expr) => {
        concat!($inst, ", [{p:e}]")
    };
}
#[cfg(feature = "nightly")]
#[cfg(target_pointer_width = "64")]
macro_rules! vpl {
    ($inst:expr) => {
        concat!($inst, ", [{p}]")
    };
}
#[cfg(feature = "nightly")]
#[cfg(target_pointer_width = "32")]
macro_rules! vps {
    ($inst1:expr, $inst2:expr) => {
        concat!($inst1, " [{p:e}]", $inst2)
    };
}
#[cfg(feature = "nightly")]
#[cfg(target_pointer_width = "64")]
macro_rules! vps {
    ($inst1:expr, $inst2:expr) => {
        concat!($inst1, " [{p}]", $inst2)
    };
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_maskz_loadu_pd(k: __mmask8, mem_addr: *const f64) -> __m512i {
    // this is copied from the standard library with added flags from V4.
    // if the full flags are not provided, the function doesn't get inlined properly
    let mut dst: __m512i;
    core::arch::asm!(
        vpl!("vmovupd {dst}{{{k}}} {{z}}"),
        p = in(reg) mem_addr,
        k = in(kreg) k,
        dst = out(zmm_reg) dst,
        options(pure, readonly, nostack)
    );
    dst
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_maskz_loadu_epi64(k: __mmask8, mem_addr: *const i64) -> __m512i {
    // this is copied from the standard library with added flags from V4.
    // if the full flags are not provided, the function doesn't get inlined properly
    let mut dst: __m512i;
    core::arch::asm!(
        vpl!("vmovdqu64 {dst}{{{k}}} {{z}}"),
        p = in(reg) mem_addr,
        k = in(kreg) k,
        dst = out(zmm_reg) dst,
        options(pure, readonly, nostack)
    );
    dst
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_maskz_loadu_ps(k: __mmask16, mem_addr: *const f32) -> __m512i {
    // this is copied from the standard library with added flags from V4.
    // if the full flags are not provided, the function doesn't get inlined properly
    let mut dst: __m512i;
    core::arch::asm!(
        vpl!("vmovups {dst}{{{k}}} {{z}}"),
        p = in(reg) mem_addr,
        k = in(kreg) k,
        dst = out(zmm_reg) dst,
        options(pure, readonly, nostack)
    );
    dst
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_maskz_loadu_epi32(k: __mmask16, mem_addr: *const i32) -> __m512i {
    // this is copied from the standard library with added flags from V4.
    // if the full flags are not provided, the function doesn't get inlined properly
    let mut dst: __m512i;
    core::arch::asm!(
        vpl!("vmovdqu32 {dst}{{{k}}} {{z}}"),
        p = in(reg) mem_addr,
        k = in(kreg) k,
        dst = out(zmm_reg) dst,
        options(pure, readonly, nostack)
    );
    dst
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_mask_loadu_pd(mut src: __m512d, k: __mmask8, mem_addr: *const f64) -> __m512d {
    // this is copied from the standard library with added flags from V4.
    // if the full flags are not provided, the function doesn't get inlined properly
    core::arch::asm!(
        vpl!("vmovupd {dst}{{{k}}}"),
        p = in(reg) mem_addr,
        k = in(kreg) k,
        dst = inout(zmm_reg) src,
        options(pure, readonly, nostack)
    );
    src
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_mask_loadu_epi64(
    mut src: __m512i,
    k: __mmask8,
    mem_addr: *const i64,
) -> __m512i {
    // this is copied from the standard library with added flags from V4.
    // if the full flags are not provided, the function doesn't get inlined properly
    core::arch::asm!(
        vpl!("vmovdqu64 {dst}{{{k}}}"),
        p = in(reg) mem_addr,
        k = in(kreg) k,
        dst = inout(zmm_reg) src,
        options(pure, readonly, nostack)
    );
    src
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_mask_loadu_ps(mut src: __m512, k: __mmask16, mem_addr: *const f32) -> __m512 {
    // this is copied from the standard library with added flags from V4.
    // if the full flags are not provided, the function doesn't get inlined properly
    core::arch::asm!(
        vpl!("vmovups {dst}{{{k}}}"),
        p = in(reg) mem_addr,
        k = in(kreg) k,
        dst = inout(zmm_reg) src,
        options(pure, readonly, nostack)
    );
    src
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_mask_loadu_epi32(
    mut src: __m512i,
    k: __mmask16,
    mem_addr: *const i32,
) -> __m512i {
    // this is copied from the standard library with added flags from V4.
    // if the full flags are not provided, the function doesn't get inlined properly
    core::arch::asm!(
        vpl!("vmovdqu32 {dst}{{{k}}}"),
        p = in(reg) mem_addr,
        k = in(kreg) k,
        dst = inout(zmm_reg) src,
        options(pure, readonly, nostack)
    );
    src
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_mask_storeu_pd(mem_addr: *mut f64, mask: __mmask8, a: __m512i) {
    core::arch::asm!(
        vps!("vmovupd", "{{{mask}}}, {a}"),
        p = in(reg) mem_addr,
        mask = in(kreg) mask,
        a = in(zmm_reg) a,
        options(nostack)
    );
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_mask_storeu_epi64(mem_addr: *mut i64, mask: __mmask8, a: __m512i) {
    core::arch::asm!(
        vps!("vmovdqu64", "{{{mask}}}, {a}"),
        p = in(reg) mem_addr,
        mask = in(kreg) mask,
        a = in(zmm_reg) a,
        options(nostack)
    );
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_mask_storeu_ps(mem_addr: *mut f32, mask: __mmask16, a: __m512i) {
    core::arch::asm!(
        vps!("vmovups", "{{{mask}}}, {a}"),
        p = in(reg) mem_addr,
        mask = in(kreg) mask,
        a = in(zmm_reg) a,
        options(nostack)
    );
}

/// # Safety
/// Same preconditions as the one in `std::arch`
#[cfg(feature = "nightly")]
#[inline]
#[target_feature(enable = "sse")]
#[target_feature(enable = "sse2")]
#[target_feature(enable = "fxsr")]
#[target_feature(enable = "sse3")]
#[target_feature(enable = "ssse3")]
#[target_feature(enable = "sse4.1")]
#[target_feature(enable = "sse4.2")]
#[target_feature(enable = "popcnt")]
#[target_feature(enable = "avx")]
#[target_feature(enable = "avx2")]
#[target_feature(enable = "bmi1")]
#[target_feature(enable = "bmi2")]
#[target_feature(enable = "fma")]
#[target_feature(enable = "lzcnt")]
#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512bw")]
#[target_feature(enable = "avx512cd")]
#[target_feature(enable = "avx512dq")]
#[target_feature(enable = "avx512vl")]
pub unsafe fn _mm512_mask_storeu_epi32(mem_addr: *mut i32, mask: __mmask16, a: __m512i) {
    core::arch::asm!(
        vps!("vmovdqu32", "{{{mask}}}, {a}"),
        p = in(reg) mem_addr,
        mask = in(kreg) mask,
        a = in(zmm_reg) a,
        options(nostack)
    );
}

// copied from the standard library
#[inline(always)]
unsafe fn avx2_pshufb(bytes: __m256i, idxs: __m256i) -> __m256i {
    let mid = _mm256_set1_epi8(16i8);
    let high = _mm256_set1_epi8(32i8);
    // This is ordering sensitive, and LLVM will order these how you put them.
    // Most AVX2 impls use ~5 "ports", and only 1 or 2 are capable of permutes.
    // But the "compose" step will lower to ops that can also use at least 1 other port.
    // So this tries to break up permutes so composition flows through "open" ports.
    // Comparative benches should be done on multiple AVX2 CPUs before reordering this

    let hihi = _mm256_permute2x128_si256::<0x11>(bytes, bytes);
    let hi_shuf = _mm256_shuffle_epi8(
        hihi, // duplicate the vector's top half
        idxs, // so that using only 4 bits of an index still picks bytes 16-31
    );
    // A zero-fill during the compose step gives the "all-Neon-like" OOB-is-0 semantics
    let compose = _mm256_blendv_epi8(_mm256_set1_epi8(0), hi_shuf, _mm256_cmpgt_epi8(high, idxs));
    let lolo = _mm256_permute2x128_si256::<0x00>(bytes, bytes);
    let lo_shuf = _mm256_shuffle_epi8(lolo, idxs);
    // Repeat, then pick indices < 16, overwriting indices 0-15 from previous compose step
    _mm256_blendv_epi8(compose, lo_shuf, _mm256_cmpgt_epi8(mid, idxs))
}

static AVX2_ROTATE_IDX: [u8x32; 32] = [
    u8x32(
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31,
    ),
    u8x32(
        31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30,
    ),
    u8x32(
        30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
        23, 24, 25, 26, 27, 28, 29,
    ),
    u8x32(
        29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
        22, 23, 24, 25, 26, 27, 28,
    ),
    u8x32(
        28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27,
    ),
    u8x32(
        27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26,
    ),
    u8x32(
        26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25,
    ),
    u8x32(
        25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24,
    ),
    u8x32(
        24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23,
    ),
    u8x32(
        23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22,
    ),
    u8x32(
        22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21,
    ),
    u8x32(
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
        14, 15, 16, 17, 18, 19, 20,
    ),
    u8x32(
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
        13, 14, 15, 16, 17, 18, 19,
    ),
    u8x32(
        19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 16, 17, 18,
    ),
    u8x32(
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17,
    ),
    u8x32(
        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 16,
    ),
    u8x32(
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15,
    ),
    u8x32(
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14,
    ),
    u8x32(
        14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5,
        6, 7, 8, 9, 10, 11, 12, 13,
    ),
    u8x32(
        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4,
        5, 6, 7, 8, 9, 10, 11, 12,
    ),
    u8x32(
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3,
        4, 5, 6, 7, 8, 9, 10, 11,
    ),
    u8x32(
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1,
        2, 3, 4, 5, 6, 7, 8, 9, 10,
    ),
    u8x32(
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0,
        1, 2, 3, 4, 5, 6, 7, 8, 9,
    ),
    u8x32(
        9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        0, 1, 2, 3, 4, 5, 6, 7, 8,
    ),
    u8x32(
        8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 0, 1, 2, 3, 4, 5, 6, 7,
    ),
    u8x32(
        7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 0, 1, 2, 3, 4, 5, 6,
    ),
    u8x32(
        6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 0, 1, 2, 3, 4, 5,
    ),
    u8x32(
        5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
        29, 30, 31, 0, 1, 2, 3, 4,
    ),
    u8x32(
        4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        28, 29, 30, 31, 0, 1, 2, 3,
    ),
    u8x32(
        3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
        27, 28, 29, 30, 31, 0, 1, 2,
    ),
    u8x32(
        2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
        27, 28, 29, 30, 31, 0, 1,
    ),
    u8x32(
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
        26, 27, 28, 29, 30, 31, 0,
    ),
];

#[cfg(feature = "nightly")]
static AVX512_ROTATE_IDX: [u32x16; 16] = [
    u32x16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    u32x16(15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14),
    u32x16(14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13),
    u32x16(13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12),
    u32x16(12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11),
    u32x16(11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10),
    u32x16(10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9),
    u32x16(9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8),
    u32x16(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7),
    u32x16(7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6),
    u32x16(6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5),
    u32x16(5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4),
    u32x16(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3),
    u32x16(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2),
    u32x16(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1),
    u32x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0),
];

#[cfg(feature = "nightly")]
static AVX512_256_ROTATE_IDX: [u32x8; 8] = [
    u32x8(0, 1, 2, 3, 4, 5, 6, 7),
    u32x8(7, 0, 1, 2, 3, 4, 5, 6),
    u32x8(6, 7, 0, 1, 2, 3, 4, 5),
    u32x8(5, 6, 7, 0, 1, 2, 3, 4),
    u32x8(4, 5, 6, 7, 0, 1, 2, 3),
    u32x8(3, 4, 5, 6, 7, 0, 1, 2),
    u32x8(2, 3, 4, 5, 6, 7, 0, 1),
    u32x8(1, 2, 3, 4, 5, 6, 7, 0),
];

impl Seal for V2 {}
impl Seal for V3 {}
impl Seal for V3Scalar {}
#[cfg(feature = "nightly")]
impl Seal for V4 {}
#[cfg(feature = "nightly")]
impl Seal for V4_256 {}

#[cfg(feature = "nightly")]
impl V4 {
    #[target_feature(enable = "avx512f")]
    #[inline]
    unsafe fn fmsubadd_ps(a: __m512, b: __m512, c: __m512) -> __m512 {
        _mm512_fmaddsub_ps(a, b, _mm512_sub_ps(_mm512_set1_ps(-0.0), c))
    }
    #[target_feature(enable = "avx512f")]
    #[inline]
    unsafe fn fmsubadd_pd(a: __m512d, b: __m512d, c: __m512d) -> __m512d {
        _mm512_fmaddsub_pd(a, b, _mm512_sub_pd(_mm512_set1_pd(-0.0), c))
    }
}

impl f32x8 {
    #[inline(always)]
    fn as_vec(self) -> __m256 {
        unsafe { transmute(self) }
    }
}

impl f64x4 {
    #[inline(always)]
    fn as_vec(self) -> __m256d {
        unsafe { transmute(self) }
    }
}

#[cfg(feature = "nightly")]
impl f32x16 {
    #[inline(always)]
    fn as_vec(self) -> __m512 {
        unsafe { transmute(self) }
    }
}

#[cfg(feature = "nightly")]
impl f64x8 {
    #[inline(always)]
    fn as_vec(self) -> __m512d {
        unsafe { transmute(self) }
    }
}

// https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels
internal_simd_type! {
    /// SSE instruction set.
    #[allow(missing_docs)]
    pub struct V2 {
        pub sse: "sse",
        pub sse2: "sse2",
        pub fxsr: "fxsr",
        pub sse3: "sse3",
        pub ssse3: "ssse3",
        pub sse4_1: "sse4.1",
        pub sse4_2: "sse4.2",
        pub popcnt: "popcnt",
    }

    /// AVX instruction set.
    ///
    /// Notable additions over [`V2`] include:
    ///  - Instructions operating on 256-bit SIMD vectors.
    ///  - Shift functions with a separate shift per lane, such as [`V3::shl_dyn_u32x4`].
    ///  - Fused multiply-accumulate instructions, such as [`V3::mul_add_f32x4`].
    #[allow(missing_docs)]
    pub struct V3 {
        pub sse: "sse",
        pub sse2: "sse2",
        pub fxsr: "fxsr",
        pub sse3: "sse3",
        pub ssse3: "ssse3",
        pub sse4_1: "sse4.1",
        pub sse4_2: "sse4.2",
        pub popcnt: "popcnt",
        pub avx: "avx",
        pub avx2: "avx2",
        pub bmi1: "bmi1",
        pub bmi2: "bmi2",
        pub fma: "fma",
        pub lzcnt: "lzcnt",
    }

    /// AVX instruction set using only scalar instructions.
    ///
    /// Notable additions over [`V2`] include:
    ///  - Instructions operating on 256-bit SIMD vectors.
    ///  - Shift functions with a separate shift per lane, such as [`V3::shl_dyn_u32x4`].
    ///  - Fused multiply-accumulate instructions, such as [`V3::mul_add_f32x4`].
    #[allow(missing_docs)]
    pub struct V3Scalar {
        pub sse: "sse",
        pub sse2: "sse2",
        pub fxsr: "fxsr",
        pub sse3: "sse3",
        pub ssse3: "ssse3",
        pub sse4_1: "sse4.1",
        pub sse4_2: "sse4.2",
        pub popcnt: "popcnt",
        pub avx: "avx",
        pub avx2: "avx2",
        pub bmi1: "bmi1",
        pub bmi2: "bmi2",
        pub fma: "fma",
        pub lzcnt: "lzcnt",
    }

    /// AVX512 instruction set.
    ///
    /// Notable additions over [`V3`] include:
    ///  - Instructions operating on 512-bit SIMD vectors.
    ///  - Masks are now composed of bits rather than vector lanes.
    #[cfg(feature = "nightly")]
    #[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
    #[allow(missing_docs)]
    pub struct V4 {
        pub sse: "sse",
        pub sse2: "sse2",
        pub fxsr: "fxsr",
        pub sse3: "sse3",
        pub ssse3: "ssse3",
        pub sse4_1: "sse4.1",
        pub sse4_2: "sse4.2",
        pub popcnt: "popcnt",
        pub avx: "avx",
        pub avx2: "avx2",
        pub bmi1: "bmi1",
        pub bmi2: "bmi2",
        pub fma: "fma",
        pub lzcnt: "lzcnt",
        pub avx512f: "avx512f",
        pub avx512bw: "avx512bw",
        pub avx512cd: "avx512cd",
        pub avx512dq: "avx512dq",
        pub avx512vl: "avx512vl",
    }
}

#[cfg(feature = "nightly")]
#[derive(Copy, Clone, Debug)]
#[repr(transparent)]
pub struct V4_256(pub V4);

impl core::ops::Deref for V3 {
    type Target = V2;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        static_assert_same_size!((), V2);
        unsafe { &*(self as *const V3 as *const V2) }
    }
}

#[cfg(feature = "nightly")]
impl core::ops::Deref for V4 {
    type Target = V3;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        static_assert_same_size!((), V3);
        unsafe { &*(self as *const V4 as *const V3) }
    }
}

#[cfg(feature = "nightly")]
impl core::ops::Deref for V4_256 {
    type Target = V4;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

static V3_U32_MASKS: [u32x8; 9] = [
    u32x8(0, 0, 0, 0, 0, 0, 0, 0),
    u32x8(!0, 0, 0, 0, 0, 0, 0, 0),
    u32x8(!0, !0, 0, 0, 0, 0, 0, 0),
    u32x8(!0, !0, !0, 0, 0, 0, 0, 0),
    u32x8(!0, !0, !0, !0, 0, 0, 0, 0),
    u32x8(!0, !0, !0, !0, !0, 0, 0, 0),
    u32x8(!0, !0, !0, !0, !0, !0, 0, 0),
    u32x8(!0, !0, !0, !0, !0, !0, !0, 0),
    u32x8(!0, !0, !0, !0, !0, !0, !0, !0),
];

static V3_U32_LAST_MASKS: [u32x8; 9] = [
    u32x8(0, 0, 0, 0, 0, 0, 0, 0),
    u32x8(0, 0, 0, 0, 0, 0, 0, !0),
    u32x8(0, 0, 0, 0, 0, 0, !0, !0),
    u32x8(0, 0, 0, 0, 0, !0, !0, !0),
    u32x8(0, 0, 0, 0, !0, !0, !0, !0),
    u32x8(0, 0, 0, !0, !0, !0, !0, !0),
    u32x8(0, 0, !0, !0, !0, !0, !0, !0),
    u32x8(0, !0, !0, !0, !0, !0, !0, !0),
    u32x8(!0, !0, !0, !0, !0, !0, !0, !0),
];

#[cfg(feature = "nightly")]
static V4_U32_MASKS: [u16; 17] = [
    0b0000000000000000,
    0b0000000000000001,
    0b0000000000000011,
    0b0000000000000111,
    0b0000000000001111,
    0b0000000000011111,
    0b0000000000111111,
    0b0000000001111111,
    0b0000000011111111,
    0b0000000111111111,
    0b0000001111111111,
    0b0000011111111111,
    0b0000111111111111,
    0b0001111111111111,
    0b0011111111111111,
    0b0111111111111111,
    0b1111111111111111,
];

#[cfg(feature = "nightly")]
static V4_U32_LAST_MASKS: [u16; 17] = [
    0b0000000000000000,
    0b1000000000000000,
    0b1100000000000000,
    0b1110000000000000,
    0b1111000000000000,
    0b1111100000000000,
    0b1111110000000000,
    0b1111111000000000,
    0b1111111100000000,
    0b1111111110000000,
    0b1111111111000000,
    0b1111111111100000,
    0b1111111111110000,
    0b1111111111111000,
    0b1111111111111100,
    0b1111111111111110,
    0b1111111111111111,
];

#[cfg(feature = "nightly")]
static V4_256_U32_MASKS: [u8; 9] = [
    0b00000000, 0b00000001, 0b00000011, 0b00000111, 0b00001111, 0b00011111, 0b00111111, 0b01111111,
    0b11111111,
];

#[cfg(feature = "nightly")]
static V4_256_U32_LAST_MASKS: [u8; 9] = [
    0b00000000, 0b10000000, 0b11000000, 0b11100000, 0b11110000, 0b11111000, 0b11111100, 0b11111110,
    0b11111111,
];

impl V2 {
    #[inline(always)]
    fn f32s_reduce_sum(self, a: f32x4) -> f32 {
        unsafe {
            // a0 a1 a2 a3
            let a: __m128 = transmute(a);
            // a2 a3 a2 a3
            let hi = _mm_movehl_ps(a, a);

            // a0+a2 a1+a3 _ _
            let r0 = _mm_add_ps(a, hi);
            // a1+a3 a2+a1 _ _
            let r0_shuffled = _mm_shuffle_ps::<0b0001>(r0, r0);

            let r = _mm_add_ss(r0, r0_shuffled);

            _mm_cvtss_f32(r)
        }
    }
    #[inline(always)]
    fn f32s_reduce_product(self, a: f32x4) -> f32 {
        unsafe {
            let a: __m128 = transmute(a);
            let hi = _mm_movehl_ps(a, a);
            let r0 = _mm_mul_ps(a, hi);
            let r0_shuffled = _mm_shuffle_ps::<0b0001>(r0, r0);
            let r = _mm_mul_ss(r0, r0_shuffled);
            _mm_cvtss_f32(r)
        }
    }
    #[inline(always)]
    fn f32s_reduce_min(self, a: f32x4) -> f32 {
        unsafe {
            let a: __m128 = transmute(a);
            let hi = _mm_movehl_ps(a, a);
            let r0 = _mm_min_ps(a, hi);
            let r0_shuffled = _mm_shuffle_ps::<0b0001>(r0, r0);
            let r = _mm_min_ss(r0, r0_shuffled);
            _mm_cvtss_f32(r)
        }
    }
    #[inline(always)]
    fn f32s_reduce_max(self, a: f32x4) -> f32 {
        unsafe {
            let a: __m128 = transmute(a);
            let hi = _mm_movehl_ps(a, a);
            let r0 = _mm_max_ps(a, hi);
            let r0_shuffled = _mm_shuffle_ps::<0b0001>(r0, r0);
            let r = _mm_max_ss(r0, r0_shuffled);
            _mm_cvtss_f32(r)
        }
    }
    #[inline(always)]
    fn f64s_reduce_sum(self, a: f64x2) -> f64 {
        unsafe {
            let a: __m128d = transmute(a);
            let hi = transmute(_mm_movehl_ps(transmute(a), transmute(a)));
            let r = _mm_add_sd(a, hi);
            _mm_cvtsd_f64(r)
        }
    }
    #[inline(always)]
    fn f64s_reduce_product(self, a: f64x2) -> f64 {
        unsafe {
            let a: __m128d = transmute(a);
            let hi = transmute(_mm_movehl_ps(transmute(a), transmute(a)));
            let r = _mm_mul_sd(a, hi);
            _mm_cvtsd_f64(r)
        }
    }
    #[inline(always)]
    fn f64s_reduce_min(self, a: f64x2) -> f64 {
        unsafe {
            let a: __m128d = transmute(a);
            let hi = transmute(_mm_movehl_ps(transmute(a), transmute(a)));
            let r = _mm_min_sd(a, hi);
            _mm_cvtsd_f64(r)
        }
    }
    #[inline(always)]
    fn f64s_reduce_max(self, a: f64x2) -> f64 {
        unsafe {
            let a: __m128d = transmute(a);
            let hi = transmute(_mm_movehl_ps(transmute(a), transmute(a)));
            let r = _mm_max_sd(a, hi);
            _mm_cvtsd_f64(r)
        }
    }

    #[inline(always)]
    fn c32s_reduce_sum(self, a: f32x4) -> c32 {
        unsafe {
            // a0 a1 a2 a3
            let a: __m128 = transmute(a);
            // a2 a3 a2 a3
            let hi = _mm_movehl_ps(a, a);

            // a0+a2 a1+a3 _ _
            let r0 = _mm_add_ps(a, hi);

            cast(_mm_cvtsd_f64(cast(r0)))
        }
    }

    #[inline(always)]
    fn c64s_reduce_sum(self, a: f64x2) -> c64 {
        cast(a)
    }
}

impl Simd for V3 {
    type m32s = m32x8;
    type f32s = f32x8;
    type i32s = i32x8;
    type u32s = u32x8;

    type m64s = m64x4;
    type f64s = f64x4;
    type i64s = i64x4;
    type u64s = u64x4;

    #[inline(always)]
    fn m32s_not(self, a: Self::m32s) -> Self::m32s {
        unsafe {
            transmute(_mm256_xor_pd(
                transmute(_mm256_set1_epi32(-1)),
                transmute(a),
            ))
        }
    }
    #[inline(always)]
    fn m32s_and(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        unsafe { transmute(_mm256_and_pd(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn m32s_or(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        unsafe { transmute(_mm256_or_pd(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn m32s_xor(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        unsafe { transmute(_mm256_xor_pd(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn m64s_not(self, a: Self::m64s) -> Self::m64s {
        unsafe {
            transmute(_mm256_xor_pd(
                transmute(_mm256_set1_epi32(-1)),
                transmute(a),
            ))
        }
    }
    #[inline(always)]
    fn m64s_and(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        unsafe { transmute(_mm256_and_pd(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn m64s_or(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        unsafe { transmute(_mm256_or_pd(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn m64s_xor(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        unsafe { transmute(_mm256_xor_pd(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn u32s_not(self, a: Self::u32s) -> Self::u32s {
        unsafe {
            transmute(_mm256_xor_pd(
                transmute(_mm256_set1_epi32(-1)),
                transmute(a),
            ))
        }
    }
    #[inline(always)]
    fn u32s_and(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_and_pd(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u32s_or(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_or_pd(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u32s_xor(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_xor_pd(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn u64s_not(self, a: Self::u64s) -> Self::u64s {
        unsafe {
            transmute(_mm256_xor_pd(
                transmute(_mm256_set1_epi32(-1)),
                transmute(a),
            ))
        }
    }
    #[inline(always)]
    fn u64s_and(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_and_pd(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_or(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_or_pd(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_xor(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_xor_pd(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn f32s_splat(self, value: f32) -> Self::f32s {
        unsafe { transmute(_mm256_set1_ps(value)) }
    }
    #[inline(always)]
    fn f32s_add(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_add_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_sub(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_sub_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_mul(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_mul_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_div(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_div_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        unsafe { transmute(_mm256_cmp_ps::<_CMP_EQ_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_less_than(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        unsafe { transmute(_mm256_cmp_ps::<_CMP_LT_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_less_than_or_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        unsafe { transmute(_mm256_cmp_ps::<_CMP_LE_OQ>(a.as_vec(), b.as_vec())) }
    }

    #[inline(always)]
    fn f64s_splat(self, value: f64) -> Self::f64s {
        unsafe { transmute(_mm256_set1_pd(value)) }
    }
    #[inline(always)]
    fn f64s_add(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_add_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_sub(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_sub_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_mul(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_mul_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_div(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_div_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        unsafe { transmute(_mm256_cmp_pd::<_CMP_EQ_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_less_than(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        unsafe { transmute(_mm256_cmp_pd::<_CMP_LT_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_less_than_or_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        unsafe { transmute(_mm256_cmp_pd::<_CMP_LE_OQ>(a.as_vec(), b.as_vec())) }
    }

    #[inline(always)]
    fn m32s_select_u32s(
        self,
        mask: Self::m32s,
        if_true: Self::u32s,
        if_false: Self::u32s,
    ) -> Self::u32s {
        unsafe {
            let mask: __m256 = transmute(mask);
            let if_true: __m256 = transmute(if_true);
            let if_false: __m256 = transmute(if_false);

            transmute(_mm256_blendv_ps(if_false, if_true, mask))
        }
    }
    #[inline(always)]
    fn m64s_select_u64s(
        self,
        mask: Self::m64s,
        if_true: Self::u64s,
        if_false: Self::u64s,
    ) -> Self::u64s {
        unsafe {
            let mask: __m256d = transmute(mask);
            let if_true: __m256d = transmute(if_true);
            let if_false: __m256d = transmute(if_false);

            transmute(_mm256_blendv_pd(if_false, if_true, mask))
        }
    }

    #[inline(always)]
    fn f32s_min(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_min_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_max(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_max_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_min(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_min_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_max(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_max_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn u32s_splat(self, value: u32) -> Self::u32s {
        unsafe { transmute(_mm256_set1_epi32(value as i32)) }
    }
    #[inline(always)]
    fn u64s_splat(self, value: u64) -> Self::u64s {
        unsafe { transmute(_mm256_set1_epi64x(value as i64)) }
    }

    #[inline(always)]
    fn u32s_add(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_add_epi32(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u32s_sub(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_sub_epi32(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_add(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_add_epi64(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_sub(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_sub_epi64(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn f64s_mul_add_e(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_fmadd_pd(a.as_vec(), b.as_vec(), c.as_vec())) }
    }
    #[inline(always)]
    fn f64_scalar_mul_add_e(self, a: f64, b: f64, c: f64) -> f64 {
        unsafe {
            crate::cast_lossy(_mm_fmadd_sd(
                _mm_load_sd(&a),
                _mm_load_sd(&b),
                _mm_load_sd(&c),
            ))
        }
    }

    #[inline(always)]
    fn f32s_mul_add_e(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_fmadd_ps(a.as_vec(), b.as_vec(), c.as_vec())) }
    }
    #[inline(always)]
    fn f32_scalar_mul_add_e(self, a: f32, b: f32, c: f32) -> f32 {
        unsafe {
            crate::cast_lossy(_mm_fmadd_ss(
                _mm_load_ss(&a),
                _mm_load_ss(&b),
                _mm_load_ss(&c),
            ))
        }
    }

    #[inline(always)]
    fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
        struct Impl<Op> {
            this: V3,
            op: Op,
        }
        impl<Op: WithSimd> crate::NullaryFnOnce for Impl<Op> {
            type Output = Op::Output;

            #[inline(always)]
            fn call(self) -> Self::Output {
                self.op.with_simd(self.this)
            }
        }
        self.vectorize(Impl { this: self, op })
    }

    #[inline(always)]
    fn f32s_reduce_sum(self, a: Self::f32s) -> f32 {
        unsafe {
            let a: __m256 = transmute(a);
            let r = _mm_add_ps(_mm256_castps256_ps128(a), _mm256_extractf128_ps::<1>(a));
            (*self).f32s_reduce_sum(transmute(r))
        }
    }

    #[inline(always)]
    fn f32s_reduce_product(self, a: Self::f32s) -> f32 {
        unsafe {
            let a: __m256 = transmute(a);
            let r = _mm_mul_ps(_mm256_castps256_ps128(a), _mm256_extractf128_ps::<1>(a));
            (*self).f32s_reduce_product(transmute(r))
        }
    }

    #[inline(always)]
    fn f32s_reduce_min(self, a: Self::f32s) -> f32 {
        unsafe {
            let a: __m256 = transmute(a);
            let r = _mm_min_ps(_mm256_castps256_ps128(a), _mm256_extractf128_ps::<1>(a));
            (*self).f32s_reduce_min(transmute(r))
        }
    }

    #[inline(always)]
    fn f32s_reduce_max(self, a: Self::f32s) -> f32 {
        unsafe {
            let a: __m256 = transmute(a);
            let r = _mm_max_ps(_mm256_castps256_ps128(a), _mm256_extractf128_ps::<1>(a));
            (*self).f32s_reduce_max(transmute(r))
        }
    }

    #[inline(always)]
    fn f64s_reduce_sum(self, a: Self::f64s) -> f64 {
        unsafe {
            let a: __m256d = transmute(a);
            let r = _mm_add_pd(_mm256_castpd256_pd128(a), _mm256_extractf128_pd::<1>(a));
            (*self).f64s_reduce_sum(transmute(r))
        }
    }

    #[inline(always)]
    fn f64s_reduce_product(self, a: Self::f64s) -> f64 {
        unsafe {
            let a: __m256d = transmute(a);
            let r = _mm_mul_pd(_mm256_castpd256_pd128(a), _mm256_extractf128_pd::<1>(a));
            (*self).f64s_reduce_product(transmute(r))
        }
    }

    #[inline(always)]
    fn f64s_reduce_min(self, a: Self::f64s) -> f64 {
        unsafe {
            let a: __m256d = transmute(a);
            let r = _mm_min_pd(_mm256_castpd256_pd128(a), _mm256_extractf128_pd::<1>(a));
            (*self).f64s_reduce_min(transmute(r))
        }
    }

    #[inline(always)]
    fn f64s_reduce_max(self, a: Self::f64s) -> f64 {
        unsafe {
            let a: __m256d = transmute(a);
            let r = _mm_max_pd(_mm256_castpd256_pd128(a), _mm256_extractf128_pd::<1>(a));
            (*self).f64s_reduce_max(transmute(r))
        }
    }

    type c32s = f32x8;
    type c64s = f64x4;

    #[inline(always)]
    fn c32s_splat(self, value: c32) -> Self::c32s {
        cast(self.f64s_splat(cast(value)))
    }
    #[inline(always)]
    fn c32s_add(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.f32s_add(a, b)
    }
    #[inline(always)]
    fn c32s_sub(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.f32s_sub(a, b)
    }

    #[inline(always)]
    fn c32s_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm256_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm256_moveldup_ps(ab);
            let bb = _mm256_movehdup_ps(ab);

            cast(_mm256_fmaddsub_ps(aa, xy, _mm256_mul_ps(bb, yx)))
        }
    }
    #[inline(always)]
    fn c32_scalar_mul(self, a: c32, b: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, -a_im * b_im);
        let im = self.f32_scalar_mul_add(a_re, b_im, a_im * b_re);

        c32 { re, im }
    }
    #[inline(always)]
    fn c32_scalar_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;
        let c_re = c.re;
        let c_im = c.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, self.f32_scalar_mul_add(-a_im, b_im, c_re));
        let im = self.f32_scalar_mul_add(a_re, b_im, self.f32_scalar_mul_add(a_im, b_re, c_im));

        c32 { re, im }
    }
    #[inline(always)]
    fn c32_scalar_conj_mul(self, a: c32, b: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, a_im * b_im);
        let im = self.f32_scalar_mul_add(a_re, b_im, -a_im * b_re);

        c32 { re, im }
    }
    #[inline(always)]
    fn c32_scalar_conj_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;
        let c_re = c.re;
        let c_im = c.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, self.f32_scalar_mul_add(a_im, b_im, c_re));
        let im = self.f32_scalar_mul_add(a_re, b_im, self.f32_scalar_mul_add(-a_im, b_re, c_im));

        c32 { re, im }
    }

    #[inline(always)]
    fn c64_scalar_mul(self, a: c64, b: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, -a_im * b_im);
        let im = self.f64_scalar_mul_add(a_re, b_im, a_im * b_re);

        c64 { re, im }
    }
    #[inline(always)]
    fn c64_scalar_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;
        let c_re = c.re;
        let c_im = c.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, self.f64_scalar_mul_add(-a_im, b_im, c_re));
        let im = self.f64_scalar_mul_add(a_re, b_im, self.f64_scalar_mul_add(a_im, b_re, c_im));

        c64 { re, im }
    }
    #[inline(always)]
    fn c64_scalar_conj_mul(self, a: c64, b: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, a_im * b_im);
        let im = self.f64_scalar_mul_add(a_re, b_im, -a_im * b_re);

        c64 { re, im }
    }
    #[inline(always)]
    fn c64_scalar_conj_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;
        let c_re = c.re;
        let c_im = c.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, self.f64_scalar_mul_add(a_im, b_im, c_re));
        let im = self.f64_scalar_mul_add(a_re, b_im, self.f64_scalar_mul_add(-a_im, b_re, c_im));

        c64 { re, im }
    }

    #[inline(always)]
    fn f32s_mul_add(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        self.f32s_mul_add_e(a, b, c)
    }
    #[inline(always)]
    fn f64s_mul_add(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        self.f64s_mul_add_e(a, b, c)
    }

    #[inline(always)]
    fn c64s_splat(self, value: c64) -> Self::c64s {
        unsafe { cast(_mm256_broadcast_pd(&*(&value as *const _ as *const _))) }
    }
    #[inline(always)]
    fn c64s_add(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.f64s_add(a, b)
    }
    #[inline(always)]
    fn c64s_sub(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.f64s_sub(a, b)
    }

    #[inline(always)]
    fn c64s_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm256_permute_pd::<0b0101>(xy);
            let aa = _mm256_unpacklo_pd(ab, ab);
            let bb = _mm256_unpackhi_pd(ab, ab);

            cast(_mm256_fmaddsub_pd(aa, xy, _mm256_mul_pd(bb, yx)))
        }
    }

    #[inline(always)]
    fn c32s_abs2(self, a: Self::c32s) -> Self::c32s {
        unsafe {
            let sqr = self.f32s_mul(a, a);
            let sqr_rev = _mm256_shuffle_ps::<0b10_11_00_01>(cast(sqr), cast(sqr));
            self.f32s_add(sqr, cast(sqr_rev))
        }
    }

    #[inline(always)]
    fn c64s_abs2(self, a: Self::c64s) -> Self::c64s {
        unsafe {
            let sqr = self.f64s_mul(a, a);
            let sqr_rev = _mm256_shuffle_pd::<0b0101>(cast(sqr), cast(sqr));
            self.f64s_add(sqr, cast(sqr_rev))
        }
    }

    #[inline(always)]
    fn u32s_partial_load(self, slice: &[u32]) -> Self::u32s {
        unsafe {
            let mask = cast(V3_U32_MASKS[slice.len().min(8)]);
            cast(_mm256_maskload_epi32(slice.as_ptr() as _, mask))
        }
    }

    #[inline(always)]
    fn u32s_partial_store(self, slice: &mut [u32], values: Self::u32s) {
        unsafe {
            let mask = cast(V3_U32_MASKS[slice.len().min(8)]);
            _mm256_maskstore_epi32(slice.as_mut_ptr() as _, mask, cast(values))
        }
    }

    #[inline(always)]
    fn u64s_partial_load(self, slice: &[u64]) -> Self::u64s {
        unsafe {
            let mask = cast(V3_U32_MASKS[(2 * slice.len()).min(8)]);
            cast(_mm256_maskload_epi64(slice.as_ptr() as _, mask))
        }
    }

    #[inline(always)]
    fn u64s_partial_store(self, slice: &mut [u64], values: Self::u64s) {
        unsafe {
            let mask = cast(V3_U32_MASKS[(slice.len() * 2).min(8)]);
            _mm256_maskstore_epi32(slice.as_mut_ptr() as _, mask, cast(values))
        }
    }

    #[inline(always)]
    fn c64s_partial_load(self, slice: &[c64]) -> Self::c64s {
        unsafe {
            let mask = cast(V3_U32_MASKS[(4 * slice.len()).min(8)]);
            cast(_mm256_maskload_epi64(slice.as_ptr() as _, mask))
        }
    }

    #[inline(always)]
    fn c64s_partial_store(self, slice: &mut [c64], values: Self::c64s) {
        unsafe {
            let mask = cast(V3_U32_MASKS[(slice.len() * 4).min(8)]);
            _mm256_maskstore_epi32(slice.as_mut_ptr() as _, mask, cast(values))
        }
    }

    #[inline(always)]
    fn u32s_partial_load_last(self, slice: &[u32]) -> Self::u32s {
        unsafe {
            let len = slice.len();
            let mask = cast(V3_U32_LAST_MASKS[len.min(8)]);
            cast(_mm256_maskload_epi32(
                slice.as_ptr().add(len).wrapping_sub(8) as _,
                mask,
            ))
        }
    }

    #[inline(always)]
    fn u32s_partial_store_last(self, slice: &mut [u32], values: Self::u32s) {
        unsafe {
            let len = slice.len();
            let mask = cast(V3_U32_LAST_MASKS[len.min(8)]);
            _mm256_maskstore_epi32(
                slice.as_mut_ptr().add(len).wrapping_sub(8) as _,
                mask,
                cast(values),
            )
        }
    }

    #[inline(always)]
    fn u64s_partial_load_last(self, slice: &[u64]) -> Self::u64s {
        unsafe {
            let len = slice.len();
            let mask = cast(V3_U32_LAST_MASKS[(2 * len).min(8)]);
            cast(_mm256_maskload_epi64(
                slice.as_ptr().add(len).wrapping_sub(4) as _,
                mask,
            ))
        }
    }

    #[inline(always)]
    fn u64s_partial_store_last(self, slice: &mut [u64], values: Self::u64s) {
        unsafe {
            let len = slice.len();
            let mask = cast(V3_U32_LAST_MASKS[(len * 2).min(8)]);
            _mm256_maskstore_epi32(
                slice.as_mut_ptr().add(len).wrapping_sub(4) as _,
                mask,
                cast(values),
            )
        }
    }

    #[inline(always)]
    fn c64s_partial_load_last(self, slice: &[c64]) -> Self::c64s {
        unsafe {
            let len = slice.len();
            let mask = cast(V3_U32_LAST_MASKS[(4 * len).min(8)]);
            cast(_mm256_maskload_epi64(
                slice.as_ptr().add(len).wrapping_sub(2) as _,
                mask,
            ))
        }
    }

    #[inline(always)]
    fn c64s_partial_store_last(self, slice: &mut [c64], values: Self::c64s) {
        unsafe {
            let len = slice.len();
            let mask = cast(V3_U32_LAST_MASKS[(len * 4).min(8)]);
            _mm256_maskstore_epi32(
                slice.as_mut_ptr().add(len).wrapping_sub(2) as _,
                mask,
                cast(values),
            )
        }
    }

    #[inline(always)]
    fn c32s_conj(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.c32s_splat(c32 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c32s_conj_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm256_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm256_moveldup_ps(ab);
            let bb = _mm256_movehdup_ps(ab);

            cast(_mm256_fmsubadd_ps(aa, xy, _mm256_mul_ps(bb, yx)))
        }
    }

    #[inline(always)]
    fn c32s_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm256_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm256_moveldup_ps(ab);
            let bb = _mm256_movehdup_ps(ab);

            cast(_mm256_fmaddsub_ps(
                aa,
                xy,
                _mm256_fmaddsub_ps(bb, yx, cast(c)),
            ))
        }
    }

    #[inline(always)]
    fn c32s_conj_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm256_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm256_moveldup_ps(ab);
            let bb = _mm256_movehdup_ps(ab);

            cast(_mm256_fmsubadd_ps(
                aa,
                xy,
                _mm256_fmsubadd_ps(bb, yx, cast(c)),
            ))
        }
    }

    #[inline(always)]
    fn c64s_conj(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.c64s_splat(c64 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c64s_conj_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm256_permute_pd::<0b0101>(xy);
            let aa = _mm256_unpacklo_pd(ab, ab);
            let bb = _mm256_unpackhi_pd(ab, ab);

            cast(_mm256_fmsubadd_pd(aa, xy, _mm256_mul_pd(bb, yx)))
        }
    }

    #[inline(always)]
    fn c64s_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm256_permute_pd::<0b0101>(xy);
            let aa = _mm256_unpacklo_pd(ab, ab);
            let bb = _mm256_unpackhi_pd(ab, ab);

            cast(_mm256_fmaddsub_pd(
                aa,
                xy,
                _mm256_fmaddsub_pd(bb, yx, cast(c)),
            ))
        }
    }

    #[inline(always)]
    fn c64s_conj_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm256_permute_pd::<0b0101>(xy);
            let aa = _mm256_unpacklo_pd(ab, ab);
            let bb = _mm256_unpackhi_pd(ab, ab);

            cast(_mm256_fmsubadd_pd(
                aa,
                xy,
                _mm256_fmsubadd_pd(bb, yx, cast(c)),
            ))
        }
    }

    #[inline(always)]
    fn c32s_neg(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.f32s_splat(-0.0))
    }

    #[inline(always)]
    fn c32s_reduce_sum(self, a: Self::c32s) -> c32 {
        unsafe {
            let a: __m256 = transmute(a);
            let r = _mm_add_ps(_mm256_castps256_ps128(a), _mm256_extractf128_ps::<1>(a));
            (*self).c32s_reduce_sum(transmute(r))
        }
    }

    #[inline(always)]
    fn c64s_neg(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.f64s_splat(-0.0))
    }

    #[inline(always)]
    fn c64s_reduce_sum(self, a: Self::c64s) -> c64 {
        unsafe {
            let a: __m256d = transmute(a);
            let r = _mm_add_pd(_mm256_castpd256_pd128(a), _mm256_extractf128_pd::<1>(a));
            (*self).c64s_reduce_sum(transmute(r))
        }
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shl(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        self.shl_dyn_u32x8(a, self.and_u32x8(amount, self.splat_u32x8(32 - 1)))
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shr(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        self.shr_dyn_u32x8(a, self.and_u32x8(amount, self.splat_u32x8(32 - 1)))
    }

    #[inline(always)]
    fn u32s_widening_mul(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
        self.widening_mul_u32x8(a, b)
    }

    #[inline(always)]
    fn u32s_less_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_lt_u32x8(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_gt_u32x8(a, b)
    }

    #[inline(always)]
    fn u32s_less_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_le_u32x8(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_ge_u32x8(a, b)
    }

    #[inline(always)]
    fn u64s_less_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_lt_u64x4(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_gt_u64x4(a, b)
    }

    #[inline(always)]
    fn u64s_less_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_le_u64x4(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_ge_u64x4(a, b)
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const u32,
        or: Self::u32s,
    ) -> Self::u32s {
        self.m32s_select_u32s(
            mask,
            transmute(_mm256_maskload_epi32(ptr as _, transmute(mask))),
            or,
        )
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const f32,
        or: Self::f32s,
    ) -> Self::f32s {
        self.m32s_select_f32s(
            mask,
            transmute(_mm256_maskload_ps(ptr as _, transmute(mask))),
            or,
        )
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const c32,
        or: Self::c32s,
    ) -> Self::c32s {
        self.m32s_select_f32s(
            mask,
            transmute(_mm256_maskload_ps(ptr as _, transmute(mask))),
            or,
        )
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const u64,
        or: Self::u64s,
    ) -> Self::u64s {
        self.m64s_select_u64s(
            mask,
            transmute(_mm256_maskload_epi64(ptr as _, transmute(mask))),
            or,
        )
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const f64,
        or: Self::f64s,
    ) -> Self::f64s {
        self.m64s_select_f64s(
            mask,
            transmute(_mm256_maskload_pd(ptr as _, transmute(mask))),
            or,
        )
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const c64,
        or: Self::c64s,
    ) -> Self::c64s {
        self.m64s_select_f64s(
            mask,
            transmute(_mm256_maskload_pd(ptr as _, transmute(mask))),
            or,
        )
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut u32, values: Self::u32s) {
        _mm256_maskstore_epi32(ptr as *mut i32, transmute(mask), transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut f32, values: Self::f32s) {
        _mm256_maskstore_ps(ptr, transmute(mask), transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut c32, values: Self::c32s) {
        _mm256_maskstore_ps(ptr as *mut f32, transmute(mask), transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut u64, values: Self::u64s) {
        _mm256_maskstore_epi64(ptr as *mut i64, transmute(mask), transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut f64, values: Self::f64s) {
        _mm256_maskstore_pd(ptr, transmute(mask), transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut c64, values: Self::c64s) {
        _mm256_maskstore_pd(ptr as *mut f64, transmute(mask), transmute(values));
    }

    #[inline(always)]
    fn u32s_rotate_right(self, a: Self::u32s, amount: usize) -> Self::u32s {
        unsafe {
            transmute(avx2_pshufb(
                transmute(a),
                transmute(AVX2_ROTATE_IDX[4 * (amount % 8)]),
            ))
        }
    }

    #[inline(always)]
    fn c32s_rotate_right(self, a: Self::c32s, amount: usize) -> Self::c32s {
        unsafe {
            transmute(avx2_pshufb(
                transmute(a),
                transmute(AVX2_ROTATE_IDX[8 * (amount % 4)]),
            ))
        }
    }

    #[inline(always)]
    fn u64s_rotate_right(self, a: Self::u64s, amount: usize) -> Self::u64s {
        unsafe {
            transmute(avx2_pshufb(
                transmute(a),
                transmute(AVX2_ROTATE_IDX[8 * (amount % 4)]),
            ))
        }
    }

    #[inline(always)]
    fn c64s_rotate_right(self, a: Self::c64s, amount: usize) -> Self::c64s {
        unsafe {
            transmute(avx2_pshufb(
                transmute(a),
                transmute(AVX2_ROTATE_IDX[16 * (amount % 2)]),
            ))
        }
    }

    #[inline(always)]
    fn c32s_swap_re_im(self, a: Self::c32s) -> Self::c32s {
        unsafe { cast(_mm256_permute_ps::<0b10_11_00_01>(cast(a))) }
    }

    #[inline(always)]
    fn c64s_swap_re_im(self, a: Self::c64s) -> Self::c64s {
        unsafe { cast(_mm256_permute_pd::<0b0101>(cast(a))) }
    }
}

impl Simd for V3Scalar {
    type m32s = bool;
    type f32s = f32;
    type i32s = i32;
    type u32s = u32;

    type m64s = bool;
    type f64s = f64;
    type i64s = i64;
    type u64s = u64;

    type c32s = crate::c32;
    type c64s = crate::c64;

    #[inline(always)]
    fn m32s_not(self, a: Self::m32s) -> Self::m32s {
        !a
    }
    #[inline(always)]
    fn m32s_and(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        a && b
    }
    #[inline(always)]
    fn m32s_or(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        a || b
    }
    #[inline(always)]
    fn m32s_xor(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        a ^ b
    }

    #[inline(always)]
    fn m64s_not(self, a: Self::m64s) -> Self::m64s {
        !a
    }
    #[inline(always)]
    fn m64s_and(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        a && b
    }
    #[inline(always)]
    fn m64s_or(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        a || b
    }
    #[inline(always)]
    fn m64s_xor(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        a ^ b
    }

    #[inline(always)]
    fn u32s_not(self, a: Self::u32s) -> Self::u32s {
        !a
    }
    #[inline(always)]
    fn u32s_and(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a & b
    }
    #[inline(always)]
    fn u32s_or(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a | b
    }
    #[inline(always)]
    fn u32s_xor(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a ^ b
    }

    #[inline(always)]
    fn u64s_not(self, a: Self::u64s) -> Self::u64s {
        !a
    }
    #[inline(always)]
    fn u64s_and(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a & b
    }
    #[inline(always)]
    fn u64s_or(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a | b
    }
    #[inline(always)]
    fn u64s_xor(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a ^ b
    }

    #[inline(always)]
    fn f32s_splat(self, value: f32) -> Self::f32s {
        value
    }
    #[inline(always)]
    fn f32s_add(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a + b
    }
    #[inline(always)]
    fn f32s_sub(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a - b
    }
    #[inline(always)]
    fn f32s_mul(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a * b
    }
    #[inline(always)]
    fn f32s_div(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a / b
    }
    #[inline(always)]
    fn f32s_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        a == b
    }
    #[inline(always)]
    fn f32s_less_than(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        a < b
    }
    #[inline(always)]
    fn f32s_less_than_or_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        a <= b
    }

    #[inline(always)]
    fn f64s_splat(self, value: f64) -> Self::f64s {
        value
    }
    #[inline(always)]
    fn f64s_add(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a + b
    }
    #[inline(always)]
    fn f64s_sub(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a - b
    }
    #[inline(always)]
    fn f64s_mul(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a * b
    }
    #[inline(always)]
    fn f64s_div(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a / b
    }
    #[inline(always)]
    fn f64s_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        a == b
    }
    #[inline(always)]
    fn f64s_less_than(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        a < b
    }
    #[inline(always)]
    fn f64s_less_than_or_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        a <= b
    }

    #[inline(always)]
    fn m32s_select_u32s(
        self,
        mask: Self::m32s,
        if_true: Self::u32s,
        if_false: Self::u32s,
    ) -> Self::u32s {
        if mask {
            if_true
        } else {
            if_false
        }
    }
    #[inline(always)]
    fn m64s_select_u64s(
        self,
        mask: Self::m64s,
        if_true: Self::u64s,
        if_false: Self::u64s,
    ) -> Self::u64s {
        if mask {
            if_true
        } else {
            if_false
        }
    }

    #[inline(always)]
    fn f32s_min(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        f32::min(a, b)
    }
    #[inline(always)]
    fn f32s_max(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        f32::max(a, b)
    }
    #[inline(always)]
    fn f64s_min(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        f64::min(a, b)
    }
    #[inline(always)]
    fn f64s_max(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        f64::max(a, b)
    }
    #[inline(always)]
    fn u32s_splat(self, value: u32) -> Self::u32s {
        value
    }
    #[inline(always)]
    fn u64s_splat(self, value: u64) -> Self::u64s {
        value
    }

    #[inline(always)]
    fn u32s_add(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a + b
    }
    #[inline(always)]
    fn u32s_sub(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a - b
    }
    #[inline(always)]
    fn u64s_add(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a + b
    }
    #[inline(always)]
    fn u64s_sub(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a - b
    }

    #[inline(always)]
    fn f64s_mul_add_e(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        self.f64_scalar_mul_add_e(a, b, c)
    }
    #[inline(always)]
    fn f64_scalar_mul_add_e(self, a: f64, b: f64, c: f64) -> f64 {
        #[cfg(feature = "std")]
        {
            f64::mul_add(a, b, c)
        }
        #[cfg(not(feature = "std"))]
        {
            libm::fma(a, b, c)
        }
    }

    #[inline(always)]
    fn f32s_mul_add_e(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        self.f32_scalar_mul_add_e(a, b, c)
    }
    #[inline(always)]
    fn f32_scalar_mul_add_e(self, a: f32, b: f32, c: f32) -> f32 {
        #[cfg(feature = "std")]
        {
            f32::mul_add(a, b, c)
        }
        #[cfg(not(feature = "std"))]
        {
            libm::fmaf(a, b, c)
        }
    }

    #[inline(always)]
    fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
        struct Impl<Op> {
            this: V3Scalar,
            op: Op,
        }
        impl<Op: WithSimd> crate::NullaryFnOnce for Impl<Op> {
            type Output = Op::Output;

            #[inline(always)]
            fn call(self) -> Self::Output {
                self.op.with_simd(self.this)
            }
        }
        self.vectorize(Impl { this: self, op })
    }

    #[inline(always)]
    fn f32s_reduce_sum(self, a: Self::f32s) -> f32 {
        a
    }

    #[inline(always)]
    fn f32s_reduce_product(self, a: Self::f32s) -> f32 {
        a
    }

    #[inline(always)]
    fn f32s_reduce_min(self, a: Self::f32s) -> f32 {
        a
    }

    #[inline(always)]
    fn f32s_reduce_max(self, a: Self::f32s) -> f32 {
        a
    }

    #[inline(always)]
    fn f64s_reduce_sum(self, a: Self::f64s) -> f64 {
        a
    }

    #[inline(always)]
    fn f64s_reduce_product(self, a: Self::f64s) -> f64 {
        a
    }

    #[inline(always)]
    fn f64s_reduce_min(self, a: Self::f64s) -> f64 {
        a
    }

    #[inline(always)]
    fn f64s_reduce_max(self, a: Self::f64s) -> f64 {
        a
    }

    #[inline(always)]
    fn c32s_splat(self, value: c32) -> Self::c32s {
        value
    }
    #[inline(always)]
    fn c32s_add(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        a + b
    }
    #[inline(always)]
    fn c32s_sub(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        a - b
    }

    #[inline(always)]
    fn c32s_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.c32_scalar_mul(a, b)
    }
    #[inline(always)]
    fn c32_scalar_mul(self, a: c32, b: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, -a_im * b_im);
        let im = self.f32_scalar_mul_add(a_re, b_im, a_im * b_re);

        c32 { re, im }
    }
    #[inline(always)]
    fn c32_scalar_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;
        let c_re = c.re;
        let c_im = c.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, self.f32_scalar_mul_add(-a_im, b_im, c_re));
        let im = self.f32_scalar_mul_add(a_re, b_im, self.f32_scalar_mul_add(a_im, b_re, c_im));

        c32 { re, im }
    }
    #[inline(always)]
    fn c32_scalar_conj_mul(self, a: c32, b: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, a_im * b_im);
        let im = self.f32_scalar_mul_add(a_re, b_im, -a_im * b_re);

        c32 { re, im }
    }
    #[inline(always)]
    fn c32_scalar_conj_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;
        let c_re = c.re;
        let c_im = c.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, self.f32_scalar_mul_add(a_im, b_im, c_re));
        let im = self.f32_scalar_mul_add(a_re, b_im, self.f32_scalar_mul_add(-a_im, b_re, c_im));

        c32 { re, im }
    }

    #[inline(always)]
    fn c64_scalar_mul(self, a: c64, b: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, -a_im * b_im);
        let im = self.f64_scalar_mul_add(a_re, b_im, a_im * b_re);

        c64 { re, im }
    }
    #[inline(always)]
    fn c64_scalar_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;
        let c_re = c.re;
        let c_im = c.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, self.f64_scalar_mul_add(-a_im, b_im, c_re));
        let im = self.f64_scalar_mul_add(a_re, b_im, self.f64_scalar_mul_add(a_im, b_re, c_im));

        c64 { re, im }
    }
    #[inline(always)]
    fn c64_scalar_conj_mul(self, a: c64, b: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, a_im * b_im);
        let im = self.f64_scalar_mul_add(a_re, b_im, -a_im * b_re);

        c64 { re, im }
    }
    #[inline(always)]
    fn c64_scalar_conj_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;
        let c_re = c.re;
        let c_im = c.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, self.f64_scalar_mul_add(a_im, b_im, c_re));
        let im = self.f64_scalar_mul_add(a_re, b_im, self.f64_scalar_mul_add(-a_im, b_re, c_im));

        c64 { re, im }
    }

    #[inline(always)]
    fn f32s_mul_add(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        self.f32s_mul_add_e(a, b, c)
    }
    #[inline(always)]
    fn f64s_mul_add(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        self.f64s_mul_add_e(a, b, c)
    }

    #[inline(always)]
    fn c64s_splat(self, value: c64) -> Self::c64s {
        unsafe { cast(_mm256_broadcast_pd(&*(&value as *const _ as *const _))) }
    }
    #[inline(always)]
    fn c64s_add(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        a + b
    }
    #[inline(always)]
    fn c64s_sub(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        a - b
    }

    #[inline(always)]
    fn c64s_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.c64_scalar_mul(a, b)
    }

    #[inline(always)]
    fn c32s_abs2(self, a: Self::c32s) -> Self::c32s {
        let val = a.re * a.re + a.im * a.im;
        c32 { re: val, im: val }
    }

    #[inline(always)]
    fn c64s_abs2(self, a: Self::c64s) -> Self::c64s {
        let val = a.re * a.re + a.im * a.im;
        c64 { re: val, im: val }
    }

    #[inline(always)]
    fn u32s_partial_load(self, slice: &[u32]) -> Self::u32s {
        slice.first().copied().unwrap_or(0)
    }

    #[inline(always)]
    fn u32s_partial_store(self, slice: &mut [u32], values: Self::u32s) {
        if let Some(first) = slice.first_mut() {
            *first = values
        }
    }

    #[inline(always)]
    fn u64s_partial_load(self, slice: &[u64]) -> Self::u64s {
        slice.first().copied().unwrap_or(0)
    }

    #[inline(always)]
    fn u64s_partial_store(self, slice: &mut [u64], values: Self::u64s) {
        if let Some(first) = slice.first_mut() {
            *first = values
        }
    }

    #[inline(always)]
    fn c64s_partial_load(self, slice: &[c64]) -> Self::c64s {
        slice.first().copied().unwrap_or(c64 { re: 0.0, im: 0.0 })
    }

    #[inline(always)]
    fn c64s_partial_store(self, slice: &mut [c64], values: Self::c64s) {
        if let Some(first) = slice.first_mut() {
            *first = values
        }
    }

    #[inline(always)]
    fn u32s_partial_load_last(self, slice: &[u32]) -> Self::u32s {
        slice.first().copied().unwrap_or(0)
    }

    #[inline(always)]
    fn u32s_partial_store_last(self, slice: &mut [u32], values: Self::u32s) {
        if let Some(first) = slice.first_mut() {
            *first = values
        }
    }

    #[inline(always)]
    fn u64s_partial_load_last(self, slice: &[u64]) -> Self::u64s {
        slice.first().copied().unwrap_or(0)
    }

    #[inline(always)]
    fn u64s_partial_store_last(self, slice: &mut [u64], values: Self::u64s) {
        if let Some(first) = slice.first_mut() {
            *first = values
        }
    }

    #[inline(always)]
    fn c64s_partial_load_last(self, slice: &[c64]) -> Self::c64s {
        slice.first().copied().unwrap_or(c64 { re: 0.0, im: 0.0 })
    }

    #[inline(always)]
    fn c64s_partial_store_last(self, slice: &mut [c64], values: Self::c64s) {
        if let Some(first) = slice.first_mut() {
            *first = values
        }
    }

    #[inline(always)]
    fn c32s_conj(self, a: Self::c32s) -> Self::c32s {
        c32 {
            re: a.re,
            im: -a.im,
        }
    }

    #[inline(always)]
    fn c32s_conj_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.c32_scalar_conj_mul(a, b)
    }

    #[inline(always)]
    fn c32s_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        self.c32_scalar_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c32s_conj_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        self.c32_scalar_conj_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c64s_conj(self, a: Self::c64s) -> Self::c64s {
        c64 {
            re: a.re,
            im: -a.im,
        }
    }

    #[inline(always)]
    fn c64s_conj_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.c64_scalar_conj_mul(a, b)
    }

    #[inline(always)]
    fn c64s_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        self.c64_scalar_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c64s_conj_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        self.c64_scalar_conj_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c32s_neg(self, a: Self::c32s) -> Self::c32s {
        -a
    }

    #[inline(always)]
    fn c32s_reduce_sum(self, a: Self::c32s) -> c32 {
        a
    }

    #[inline(always)]
    fn c64s_neg(self, a: Self::c64s) -> Self::c64s {
        -a
    }

    #[inline(always)]
    fn c64s_reduce_sum(self, a: Self::c64s) -> c64 {
        a
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shl(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        a.wrapping_shl(amount)
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shr(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        a.wrapping_shr(amount)
    }

    #[inline(always)]
    fn u32s_widening_mul(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
        let c = a as u64 * b as u64;
        let lo = c as u32;
        let hi = (c >> 32) as u32;
        (lo, hi)
    }

    #[inline(always)]
    fn u32s_less_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        a < b
    }

    #[inline(always)]
    fn u32s_greater_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        a > b
    }

    #[inline(always)]
    fn u32s_less_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        a <= b
    }

    #[inline(always)]
    fn u32s_greater_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        a >= b
    }

    #[inline(always)]
    fn u64s_less_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        a < b
    }

    #[inline(always)]
    fn u64s_greater_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        a > b
    }

    #[inline(always)]
    fn u64s_less_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        a <= b
    }

    #[inline(always)]
    fn u64s_greater_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        a >= b
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const u32,
        or: Self::u32s,
    ) -> Self::u32s {
        if mask {
            *ptr
        } else {
            or
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const f32,
        or: Self::f32s,
    ) -> Self::f32s {
        if mask {
            *ptr
        } else {
            or
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const c32,
        or: Self::c32s,
    ) -> Self::c32s {
        if mask {
            *ptr
        } else {
            or
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const u64,
        or: Self::u64s,
    ) -> Self::u64s {
        if mask {
            *ptr
        } else {
            or
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const f64,
        or: Self::f64s,
    ) -> Self::f64s {
        if mask {
            *ptr
        } else {
            or
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const c64,
        or: Self::c64s,
    ) -> Self::c64s {
        if mask {
            *ptr
        } else {
            or
        }
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut u32, values: Self::u32s) {
        if mask {
            *ptr = values
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut f32, values: Self::f32s) {
        if mask {
            *ptr = values
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut c32, values: Self::c32s) {
        if mask {
            *ptr = values
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut u64, values: Self::u64s) {
        if mask {
            *ptr = values
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut f64, values: Self::f64s) {
        if mask {
            *ptr = values
        }
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut c64, values: Self::c64s) {
        if mask {
            *ptr = values
        }
    }

    #[inline(always)]
    fn u32s_rotate_right(self, a: Self::u32s, _amount: usize) -> Self::u32s {
        a
    }

    #[inline(always)]
    fn c32s_rotate_right(self, a: Self::c32s, _amount: usize) -> Self::c32s {
        a
    }

    #[inline(always)]
    fn u64s_rotate_right(self, a: Self::u64s, _amount: usize) -> Self::u64s {
        a
    }

    #[inline(always)]
    fn c64s_rotate_right(self, a: Self::c64s, _amount: usize) -> Self::c64s {
        a
    }

    #[inline(always)]
    fn c32s_swap_re_im(self, a: Self::c32s) -> Self::c32s {
        c32 { re: a.im, im: a.re }
    }

    #[inline(always)]
    fn c64s_swap_re_im(self, a: Self::c64s) -> Self::c64s {
        c64 { re: a.im, im: a.re }
    }
}

#[cfg(feature = "nightly")]
impl Simd for V4 {
    type m32s = b16;
    type f32s = f32x16;
    type i32s = i32x16;
    type u32s = u32x16;

    type m64s = b8;
    type f64s = f64x8;
    type i64s = i64x8;
    type u64s = u64x8;

    #[inline(always)]
    fn m32s_not(self, a: Self::m32s) -> Self::m32s {
        b16(!a.0)
    }
    #[inline(always)]
    fn m32s_and(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        b16(a.0 & b.0)
    }
    #[inline(always)]
    fn m32s_or(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        b16(a.0 | b.0)
    }
    #[inline(always)]
    fn m32s_xor(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        b16(a.0 ^ b.0)
    }

    #[inline(always)]
    fn m64s_not(self, a: Self::m64s) -> Self::m64s {
        b8(!a.0)
    }
    #[inline(always)]
    fn m64s_and(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        b8(a.0 & b.0)
    }
    #[inline(always)]
    fn m64s_or(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        b8(a.0 | b.0)
    }
    #[inline(always)]
    fn m64s_xor(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        b8(a.0 ^ b.0)
    }

    #[inline(always)]
    fn u32s_not(self, a: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm512_xor_si512(_mm512_set1_epi32(-1), transmute(a))) }
    }
    #[inline(always)]
    fn u32s_and(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm512_and_si512(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u32s_or(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm512_or_si512(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u32s_xor(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm512_xor_si512(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn u64s_not(self, a: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm512_xor_si512(_mm512_set1_epi32(-1), transmute(a))) }
    }
    #[inline(always)]
    fn u64s_and(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm512_and_si512(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_or(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm512_or_si512(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_xor(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm512_xor_si512(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn f32s_splat(self, value: f32) -> Self::f32s {
        unsafe { transmute(_mm512_set1_ps(value)) }
    }
    #[inline(always)]
    fn f32s_add(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm512_add_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_sub(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm512_sub_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_mul(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm512_mul_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_div(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm512_div_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        unsafe { transmute(_mm512_cmp_ps_mask::<_CMP_EQ_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_less_than(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        unsafe { transmute(_mm512_cmp_ps_mask::<_CMP_LT_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_less_than_or_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        unsafe { transmute(_mm512_cmp_ps_mask::<_CMP_LE_OQ>(a.as_vec(), b.as_vec())) }
    }

    #[inline(always)]
    fn f64s_splat(self, value: f64) -> Self::f64s {
        unsafe { transmute(_mm512_set1_pd(value)) }
    }
    #[inline(always)]
    fn f64s_add(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm512_add_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_sub(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm512_sub_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_mul(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm512_mul_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_div(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm512_div_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        unsafe { transmute(_mm512_cmp_pd_mask::<_CMP_EQ_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_less_than(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        unsafe { transmute(_mm512_cmp_pd_mask::<_CMP_LT_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_less_than_or_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        unsafe { transmute(_mm512_cmp_pd_mask::<_CMP_LE_OQ>(a.as_vec(), b.as_vec())) }
    }

    #[inline(always)]
    fn f64s_mul_add_e(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm512_fmadd_pd(a.as_vec(), b.as_vec(), c.as_vec())) }
    }
    #[inline(always)]
    fn f32s_mul_add_e(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm512_fmadd_ps(a.as_vec(), b.as_vec(), c.as_vec())) }
    }
    #[inline(always)]
    fn f32_scalar_mul_add_e(self, a: f32, b: f32, c: f32) -> f32 {
        (*self).f32_scalar_mul_add_e(a, b, c)
    }

    #[inline(always)]
    fn m32s_select_u32s(
        self,
        mask: Self::m32s,
        if_true: Self::u32s,
        if_false: Self::u32s,
    ) -> Self::u32s {
        unsafe {
            let mask: __mmask16 = mask.0;
            let if_true: __m512 = transmute(if_true);
            let if_false: __m512 = transmute(if_false);

            transmute(_mm512_mask_blend_ps(mask, if_false, if_true))
        }
    }

    #[inline(always)]
    fn m64s_select_u64s(
        self,
        mask: Self::m64s,
        if_true: Self::u64s,
        if_false: Self::u64s,
    ) -> Self::u64s {
        unsafe {
            let mask: __mmask8 = mask.0;
            let if_true: __m512d = transmute(if_true);
            let if_false: __m512d = transmute(if_false);

            transmute(_mm512_mask_blend_pd(mask, if_false, if_true))
        }
    }

    #[inline(always)]
    fn f32s_min(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm512_min_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_max(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm512_max_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_min(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm512_min_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_max(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm512_max_pd(a.as_vec(), b.as_vec())) }
    }

    #[inline(always)]
    fn u32s_add(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm512_add_epi32(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u32s_sub(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm512_sub_epi32(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_add(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm512_add_epi64(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_sub(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm512_sub_epi64(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
        struct Impl<Op> {
            this: V4,
            op: Op,
        }
        impl<Op: WithSimd> crate::NullaryFnOnce for Impl<Op> {
            type Output = Op::Output;

            #[inline(always)]
            fn call(self) -> Self::Output {
                self.op.with_simd(self.this)
            }
        }
        self.vectorize(Impl { this: self, op })
    }

    #[inline(always)]
    fn u32s_splat(self, value: u32) -> Self::u32s {
        unsafe { transmute(_mm512_set1_epi32(value as i32)) }
    }
    #[inline(always)]
    fn u64s_splat(self, value: u64) -> Self::u64s {
        unsafe { transmute(_mm512_set1_epi64(value as i64)) }
    }

    #[inline(always)]
    fn f32s_reduce_sum(self, a: Self::f32s) -> f32 {
        unsafe {
            let a: __m512 = transmute(a);
            let r = _mm256_add_ps(
                _mm512_castps512_ps256(a),
                transmute(_mm512_extractf64x4_pd::<1>(transmute(a))),
            );
            (*self).f32s_reduce_sum(transmute(r))
        }
    }

    #[inline(always)]
    fn f32s_reduce_product(self, a: Self::f32s) -> f32 {
        unsafe {
            let a: __m512 = transmute(a);
            let r = _mm256_mul_ps(
                _mm512_castps512_ps256(a),
                transmute(_mm512_extractf64x4_pd::<1>(transmute(a))),
            );
            (*self).f32s_reduce_product(transmute(r))
        }
    }

    #[inline(always)]
    fn f32s_reduce_min(self, a: Self::f32s) -> f32 {
        unsafe {
            let a: __m512 = transmute(a);
            let r = _mm256_min_ps(
                _mm512_castps512_ps256(a),
                transmute(_mm512_extractf64x4_pd::<1>(transmute(a))),
            );
            (*self).f32s_reduce_min(transmute(r))
        }
    }

    #[inline(always)]
    fn f32s_reduce_max(self, a: Self::f32s) -> f32 {
        unsafe {
            let a: __m512 = transmute(a);
            let r = _mm256_max_ps(
                _mm512_castps512_ps256(a),
                transmute(_mm512_extractf64x4_pd::<1>(transmute(a))),
            );
            (*self).f32s_reduce_max(transmute(r))
        }
    }

    #[inline(always)]
    fn f64s_reduce_sum(self, a: Self::f64s) -> f64 {
        unsafe {
            let a: __m512d = transmute(a);
            let r = _mm256_add_pd(_mm512_castpd512_pd256(a), _mm512_extractf64x4_pd::<1>(a));
            (*self).f64s_reduce_sum(transmute(r))
        }
    }

    #[inline(always)]
    fn f64s_reduce_product(self, a: Self::f64s) -> f64 {
        unsafe {
            let a: __m512d = transmute(a);
            let r = _mm256_mul_pd(_mm512_castpd512_pd256(a), _mm512_extractf64x4_pd::<1>(a));
            (*self).f64s_reduce_product(transmute(r))
        }
    }

    #[inline(always)]
    fn f64s_reduce_min(self, a: Self::f64s) -> f64 {
        unsafe {
            let a: __m512d = transmute(a);
            let r = _mm256_min_pd(_mm512_castpd512_pd256(a), _mm512_extractf64x4_pd::<1>(a));
            (*self).f64s_reduce_min(transmute(r))
        }
    }

    #[inline(always)]
    fn f64s_reduce_max(self, a: Self::f64s) -> f64 {
        unsafe {
            let a: __m512d = transmute(a);
            let r = _mm256_max_pd(_mm512_castpd512_pd256(a), _mm512_extractf64x4_pd::<1>(a));
            (*self).f64s_reduce_max(transmute(r))
        }
    }

    type c32s = f32x16;
    type c64s = f64x8;

    #[inline(always)]
    fn c32s_splat(self, value: c32) -> Self::c32s {
        cast(self.f64s_splat(cast(value)))
    }
    #[inline(always)]
    fn c32s_add(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.f32s_add(a, b)
    }
    #[inline(always)]
    fn c32s_sub(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.f32s_sub(a, b)
    }

    #[inline(always)]
    fn c32s_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm512_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm512_moveldup_ps(ab);
            let bb = _mm512_movehdup_ps(ab);

            cast(_mm512_fmaddsub_ps(aa, xy, _mm512_mul_ps(bb, yx)))
        }
    }

    #[inline(always)]
    fn f32s_mul_add(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        self.f32s_mul_add_e(a, b, c)
    }
    #[inline(always)]
    fn f64s_mul_add(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        self.f64s_mul_add_e(a, b, c)
    }

    #[inline(always)]
    fn c64s_splat(self, value: c64) -> Self::c64s {
        unsafe { cast(_mm512_broadcast_f32x4(cast(value))) }
    }
    #[inline(always)]
    fn c64s_add(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.f64s_add(a, b)
    }
    #[inline(always)]
    fn c64s_sub(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.f64s_sub(a, b)
    }

    #[inline(always)]
    fn c64s_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm512_permute_pd::<0b01010101>(xy);
            let aa = _mm512_unpacklo_pd(ab, ab);
            let bb = _mm512_unpackhi_pd(ab, ab);

            cast(_mm512_fmaddsub_pd(aa, xy, _mm512_mul_pd(bb, yx)))
        }
    }

    #[inline(always)]
    fn c32s_abs2(self, a: Self::c32s) -> Self::c32s {
        unsafe {
            let sqr = self.f32s_mul(a, a);
            let sqr_rev = _mm512_shuffle_ps::<0b10_11_00_01>(cast(sqr), cast(sqr));
            self.f32s_add(sqr, cast(sqr_rev))
        }
    }

    #[inline(always)]
    fn c64s_abs2(self, a: Self::c64s) -> Self::c64s {
        unsafe {
            let sqr = self.f64s_mul(a, a);
            let sqr_rev = _mm512_shuffle_pd::<0b01010101>(cast(sqr), cast(sqr));
            self.f64s_add(sqr, cast(sqr_rev))
        }
    }

    #[inline(always)]
    fn u32s_partial_load(self, slice: &[u32]) -> Self::u32s {
        unsafe {
            let mask = cast(V4_U32_MASKS[slice.len().min(16)]);
            cast(_mm512_maskz_loadu_epi32(mask, slice.as_ptr() as _))
        }
    }

    #[inline(always)]
    fn u32s_partial_store(self, slice: &mut [u32], values: Self::u32s) {
        unsafe {
            let mask = cast(V4_U32_MASKS[slice.len().min(16)]);
            _mm512_mask_storeu_epi32(slice.as_mut_ptr() as _, mask, cast(values));
        }
    }

    #[inline(always)]
    fn u64s_partial_load(self, slice: &[u64]) -> Self::u64s {
        unsafe {
            let mask = cast(V4_U32_MASKS[(2 * slice.len()).min(16)]);
            cast(_mm512_maskz_loadu_epi32(mask, slice.as_ptr() as _))
        }
    }

    #[inline(always)]
    fn u64s_partial_store(self, slice: &mut [u64], values: Self::u64s) {
        unsafe {
            let mask = cast(V4_U32_MASKS[(2 * slice.len()).min(16)]);
            _mm512_mask_storeu_epi32(slice.as_mut_ptr() as _, mask, cast(values));
        }
    }

    #[inline(always)]
    fn c64s_partial_load(self, slice: &[c64]) -> Self::c64s {
        unsafe {
            let mask = cast(V4_U32_MASKS[(4 * slice.len()).min(16)]);
            cast(_mm512_maskz_loadu_epi32(mask, slice.as_ptr() as _))
        }
    }

    #[inline(always)]
    fn c64s_partial_store(self, slice: &mut [c64], values: Self::c64s) {
        unsafe {
            let mask = cast(V4_U32_MASKS[(4 * slice.len()).min(16)]);
            _mm512_mask_storeu_epi32(slice.as_mut_ptr() as _, mask, cast(values));
        }
    }

    #[inline(always)]
    fn u32s_partial_load_last(self, slice: &[u32]) -> Self::u32s {
        unsafe {
            let len = slice.len();
            let mask = cast(V4_U32_LAST_MASKS[slice.len().min(16)]);
            cast(_mm512_maskz_loadu_epi32(
                mask,
                slice.as_ptr().add(len).wrapping_sub(16) as _,
            ))
        }
    }

    #[inline(always)]
    fn u32s_partial_store_last(self, slice: &mut [u32], values: Self::u32s) {
        unsafe {
            let len = slice.len();
            let mask = cast(V4_U32_LAST_MASKS[slice.len().min(16)]);
            _mm512_mask_storeu_epi32(
                slice.as_mut_ptr().add(len).wrapping_sub(16) as _,
                mask,
                cast(values),
            );
        }
    }

    #[inline(always)]
    fn u64s_partial_load_last(self, slice: &[u64]) -> Self::u64s {
        unsafe {
            let len = slice.len();
            let mask = cast(V4_U32_LAST_MASKS[(2 * slice.len()).min(16)]);
            cast(_mm512_maskz_loadu_epi32(
                mask,
                slice.as_ptr().add(len).wrapping_sub(8) as _,
            ))
        }
    }

    #[inline(always)]
    fn u64s_partial_store_last(self, slice: &mut [u64], values: Self::u64s) {
        unsafe {
            let len = slice.len();
            let mask = cast(V4_U32_LAST_MASKS[(2 * slice.len()).min(16)]);
            _mm512_mask_storeu_epi32(
                slice.as_mut_ptr().add(len).wrapping_sub(8) as _,
                mask,
                cast(values),
            );
        }
    }

    #[inline(always)]
    fn c64s_partial_load_last(self, slice: &[c64]) -> Self::c64s {
        unsafe {
            let len = slice.len();
            let mask = cast(V4_U32_LAST_MASKS[(4 * slice.len()).min(16)]);
            cast(_mm512_maskz_loadu_epi32(
                mask,
                slice.as_ptr().add(len).wrapping_sub(4) as _,
            ))
        }
    }

    #[inline(always)]
    fn c64s_partial_store_last(self, slice: &mut [c64], values: Self::c64s) {
        unsafe {
            let len = slice.len();
            let mask = cast(V4_U32_LAST_MASKS[(4 * slice.len()).min(16)]);
            _mm512_mask_storeu_epi32(
                slice.as_mut_ptr().add(len).wrapping_sub(4) as _,
                mask,
                cast(values),
            );
        }
    }

    #[inline(always)]
    fn c32s_conj(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.c32s_splat(c32 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c32s_conj_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm512_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm512_moveldup_ps(ab);
            let bb = _mm512_movehdup_ps(ab);

            cast(Self::fmsubadd_ps(aa, xy, _mm512_mul_ps(bb, yx)))
        }
    }

    #[inline(always)]
    fn c32s_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm512_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm512_moveldup_ps(ab);
            let bb = _mm512_movehdup_ps(ab);

            cast(_mm512_fmaddsub_ps(
                aa,
                xy,
                _mm512_fmaddsub_ps(bb, yx, cast(c)),
            ))
        }
    }

    #[inline(always)]
    fn c32s_conj_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm512_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm512_moveldup_ps(ab);
            let bb = _mm512_movehdup_ps(ab);

            cast(Self::fmsubadd_ps(
                aa,
                xy,
                Self::fmsubadd_ps(bb, yx, cast(c)),
            ))
        }
    }

    #[inline(always)]
    fn c64s_conj(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.c64s_splat(c64 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c64s_conj_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm512_permute_pd::<0b01010101>(xy);
            let aa = _mm512_unpacklo_pd(ab, ab);
            let bb = _mm512_unpackhi_pd(ab, ab);

            cast(Self::fmsubadd_pd(aa, xy, _mm512_mul_pd(bb, yx)))
        }
    }

    #[inline(always)]
    fn c64s_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm512_permute_pd::<0b01010101>(xy);
            let aa = _mm512_unpacklo_pd(ab, ab);
            let bb = _mm512_unpackhi_pd(ab, ab);

            cast(_mm512_fmaddsub_pd(
                aa,
                xy,
                _mm512_fmaddsub_pd(bb, yx, cast(c)),
            ))
        }
    }

    #[inline(always)]
    fn c64s_conj_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = cast(a);
            let xy = cast(b);

            let yx = _mm512_permute_pd::<0b01010101>(xy);
            let aa = _mm512_unpacklo_pd(ab, ab);
            let bb = _mm512_unpackhi_pd(ab, ab);

            cast(Self::fmsubadd_pd(
                aa,
                xy,
                Self::fmsubadd_pd(bb, yx, cast(c)),
            ))
        }
    }

    #[inline(always)]
    fn c32s_neg(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.f32s_splat(-0.0))
    }

    #[inline(always)]
    fn c32s_reduce_sum(self, a: Self::c32s) -> c32 {
        unsafe {
            let a: __m512 = transmute(a);
            let r = _mm256_add_ps(
                _mm512_castps512_ps256(a),
                transmute(_mm512_extractf64x4_pd::<1>(transmute(a))),
            );
            (*self).c32s_reduce_sum(transmute(r))
        }
    }

    #[inline(always)]
    fn c64s_neg(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.f64s_splat(-0.0))
    }

    #[inline(always)]
    fn c64s_reduce_sum(self, a: Self::c64s) -> c64 {
        unsafe {
            let a: __m512d = transmute(a);
            let r = _mm256_add_pd(_mm512_castpd512_pd256(a), _mm512_extractf64x4_pd::<1>(a));
            (*self).c64s_reduce_sum(transmute(r))
        }
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shl(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        self.shl_dyn_u32x16(a, self.and_u32x16(amount, self.splat_u32x16(32 - 1)))
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shr(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        self.shr_dyn_u32x16(a, self.and_u32x16(amount, self.splat_u32x16(32 - 1)))
    }

    #[inline(always)]
    fn u32s_widening_mul(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
        self.widening_mul_u32x16(a, b)
    }

    #[inline(always)]
    fn u64s_less_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_lt_u64x8(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_gt_u64x8(a, b)
    }

    #[inline(always)]
    fn u64s_less_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_le_u64x8(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_ge_u64x8(a, b)
    }

    #[inline(always)]
    fn u32s_less_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_lt_u32x16(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_gt_u32x16(a, b)
    }

    #[inline(always)]
    fn u32s_less_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_le_u32x16(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_ge_u32x16(a, b)
    }

    #[inline(always)]
    fn c32_scalar_mul(self, a: c32, b: c32) -> c32 {
        (*self).c32_scalar_mul(a, b)
    }
    #[inline(always)]
    fn c32_scalar_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        (*self).c32_scalar_mul_add(a, b, c)
    }
    #[inline(always)]
    fn c32_scalar_conj_mul(self, a: c32, b: c32) -> c32 {
        (*self).c32_scalar_conj_mul(a, b)
    }
    #[inline(always)]
    fn c32_scalar_conj_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        (*self).c32_scalar_conj_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c64_scalar_mul(self, a: c64, b: c64) -> c64 {
        (*self).c64_scalar_mul(a, b)
    }
    #[inline(always)]
    fn c64_scalar_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        (*self).c64_scalar_mul_add(a, b, c)
    }
    #[inline(always)]
    fn c64_scalar_conj_mul(self, a: c64, b: c64) -> c64 {
        (*self).c64_scalar_conj_mul(a, b)
    }
    #[inline(always)]
    fn c64_scalar_conj_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        (*self).c64_scalar_conj_mul_add(a, b, c)
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const u32,
        or: Self::u32s,
    ) -> Self::u32s {
        transmute(_mm512_mask_loadu_epi32(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const f32,
        or: Self::f32s,
    ) -> Self::f32s {
        transmute(_mm512_mask_loadu_ps(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const c32,
        or: Self::c32s,
    ) -> Self::c32s {
        transmute(_mm512_mask_loadu_ps(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const u64,
        or: Self::u64s,
    ) -> Self::u64s {
        transmute(_mm512_mask_loadu_epi64(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const f64,
        or: Self::f64s,
    ) -> Self::f64s {
        transmute(_mm512_mask_loadu_pd(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const c64,
        or: Self::c64s,
    ) -> Self::c64s {
        transmute(_mm512_mask_loadu_pd(transmute(or), mask.0, ptr as _))
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut u32, values: Self::u32s) {
        _mm512_mask_storeu_epi32(ptr as *mut i32, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut f32, values: Self::f32s) {
        _mm512_mask_storeu_ps(ptr, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut c32, values: Self::c32s) {
        _mm512_mask_storeu_ps(ptr as *mut f32, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut u64, values: Self::u64s) {
        _mm512_mask_storeu_epi64(ptr as *mut i64, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut f64, values: Self::f64s) {
        _mm512_mask_storeu_pd(ptr, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut c64, values: Self::c64s) {
        _mm512_mask_storeu_pd(ptr as *mut f64, mask.0, transmute(values));
    }

    #[inline(always)]
    fn u32s_rotate_right(self, a: Self::u32s, amount: usize) -> Self::u32s {
        unsafe {
            transmute(_mm512_permutexvar_epi32(
                transmute(AVX512_ROTATE_IDX[amount % 16]),
                transmute(a),
            ))
        }
    }

    #[inline(always)]
    fn c32s_rotate_right(self, a: Self::c32s, amount: usize) -> Self::c32s {
        unsafe {
            transmute(_mm512_permutexvar_epi32(
                transmute(AVX512_ROTATE_IDX[2 * (amount % 8)]),
                transmute(a),
            ))
        }
    }

    #[inline(always)]
    fn u64s_rotate_right(self, a: Self::u64s, amount: usize) -> Self::u64s {
        unsafe {
            transmute(_mm512_permutexvar_epi32(
                transmute(AVX512_ROTATE_IDX[2 * (amount % 8)]),
                transmute(a),
            ))
        }
    }

    #[inline(always)]
    fn c64s_rotate_right(self, a: Self::c64s, amount: usize) -> Self::c64s {
        unsafe {
            transmute(_mm512_permutexvar_epi32(
                transmute(AVX512_ROTATE_IDX[4 * (amount % 4)]),
                transmute(a),
            ))
        }
    }

    #[inline(always)]
    fn c32s_swap_re_im(self, a: Self::c32s) -> Self::c32s {
        unsafe { cast(_mm512_permute_ps::<0b10_11_00_01>(cast(a))) }
    }

    #[inline(always)]
    fn c64s_swap_re_im(self, a: Self::c64s) -> Self::c64s {
        unsafe { cast(_mm512_permute_pd::<0b01010101>(cast(a))) }
    }
}

#[cfg(feature = "nightly")]
impl Simd for V4_256 {
    type m32s = b8;
    type f32s = f32x8;
    type i32s = i32x8;
    type u32s = u32x8;

    type m64s = b8;
    type f64s = f64x4;
    type i64s = i64x4;
    type u64s = u64x4;

    #[inline(always)]
    fn m32s_not(self, a: Self::m32s) -> Self::m32s {
        b8(!a.0)
    }
    #[inline(always)]
    fn m32s_and(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        b8(a.0 & b.0)
    }
    #[inline(always)]
    fn m32s_or(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        b8(a.0 | b.0)
    }
    #[inline(always)]
    fn m32s_xor(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        b8(a.0 ^ b.0)
    }

    #[inline(always)]
    fn m64s_not(self, a: Self::m64s) -> Self::m64s {
        b8(!a.0)
    }
    #[inline(always)]
    fn m64s_and(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        b8(a.0 & b.0)
    }
    #[inline(always)]
    fn m64s_or(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        b8(a.0 | b.0)
    }
    #[inline(always)]
    fn m64s_xor(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        b8(a.0 ^ b.0)
    }

    #[inline(always)]
    fn u32s_not(self, a: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_xor_si256(_mm256_set1_epi32(-1), transmute(a))) }
    }
    #[inline(always)]
    fn u32s_and(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_and_si256(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u32s_or(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_or_si256(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u32s_xor(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_xor_si256(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn u64s_not(self, a: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_xor_si256(_mm256_set1_epi32(-1), transmute(a))) }
    }
    #[inline(always)]
    fn u64s_and(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_and_si256(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_or(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_or_si256(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_xor(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_xor_si256(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn f32s_splat(self, value: f32) -> Self::f32s {
        unsafe { transmute(_mm256_set1_ps(value)) }
    }
    #[inline(always)]
    fn f32s_add(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_add_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_sub(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_sub_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_mul(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_mul_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_div(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_div_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        unsafe { transmute(_mm256_cmp_ps_mask::<_CMP_EQ_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_less_than(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        unsafe { transmute(_mm256_cmp_ps_mask::<_CMP_LT_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_less_than_or_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        unsafe { transmute(_mm256_cmp_ps_mask::<_CMP_LE_OQ>(a.as_vec(), b.as_vec())) }
    }

    #[inline(always)]
    fn f64s_splat(self, value: f64) -> Self::f64s {
        unsafe { transmute(_mm256_set1_pd(value)) }
    }
    #[inline(always)]
    fn f64s_add(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_add_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_sub(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_sub_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_mul(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_mul_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_div(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_div_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        unsafe { transmute(_mm256_cmp_pd_mask::<_CMP_EQ_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_less_than(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        unsafe { transmute(_mm256_cmp_pd_mask::<_CMP_LT_OQ>(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_less_than_or_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        unsafe { transmute(_mm256_cmp_pd_mask::<_CMP_LE_OQ>(a.as_vec(), b.as_vec())) }
    }

    #[inline(always)]
    fn f64s_mul_add_e(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_fmadd_pd(a.as_vec(), b.as_vec(), c.as_vec())) }
    }
    #[inline(always)]
    fn f32s_mul_add_e(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_fmadd_ps(a.as_vec(), b.as_vec(), c.as_vec())) }
    }
    #[inline(always)]
    fn f32_scalar_mul_add_e(self, a: f32, b: f32, c: f32) -> f32 {
        (*self).f32_scalar_mul_add_e(a, b, c)
    }

    #[inline(always)]
    fn m32s_select_u32s(
        self,
        mask: Self::m32s,
        if_true: Self::u32s,
        if_false: Self::u32s,
    ) -> Self::u32s {
        unsafe {
            let mask: __mmask8 = mask.0;
            let if_true: __m256 = transmute(if_true);
            let if_false: __m256 = transmute(if_false);

            transmute(_mm256_mask_blend_ps(mask, if_false, if_true))
        }
    }

    #[inline(always)]
    fn m64s_select_u64s(
        self,
        mask: Self::m64s,
        if_true: Self::u64s,
        if_false: Self::u64s,
    ) -> Self::u64s {
        unsafe {
            let mask: __mmask8 = mask.0;
            let if_true: __m256d = transmute(if_true);
            let if_false: __m256d = transmute(if_false);

            transmute(_mm256_mask_blend_pd(mask, if_false, if_true))
        }
    }

    #[inline(always)]
    fn f32s_min(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_min_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f32s_max(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        unsafe { transmute(_mm256_max_ps(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_min(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_min_pd(a.as_vec(), b.as_vec())) }
    }
    #[inline(always)]
    fn f64s_max(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        unsafe { transmute(_mm256_max_pd(a.as_vec(), b.as_vec())) }
    }

    #[inline(always)]
    fn u32s_add(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_add_epi32(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u32s_sub(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        unsafe { transmute(_mm256_sub_epi32(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_add(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_add_epi64(transmute(a), transmute(b))) }
    }
    #[inline(always)]
    fn u64s_sub(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        unsafe { transmute(_mm256_sub_epi64(transmute(a), transmute(b))) }
    }

    #[inline(always)]
    fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
        struct Impl<Op> {
            this: V4_256,
            op: Op,
        }
        impl<Op: WithSimd> crate::NullaryFnOnce for Impl<Op> {
            type Output = Op::Output;

            #[inline(always)]
            fn call(self) -> Self::Output {
                self.op.with_simd(self.this)
            }
        }
        (*self).vectorize(Impl { this: self, op })
    }

    #[inline(always)]
    fn u32s_splat(self, value: u32) -> Self::u32s {
        unsafe { transmute(_mm256_set1_epi32(value as i32)) }
    }
    #[inline(always)]
    fn u64s_splat(self, value: u64) -> Self::u64s {
        unsafe { transmute(_mm256_set1_epi64x(value as i64)) }
    }

    #[inline(always)]
    fn f32s_reduce_sum(self, a: Self::f32s) -> f32 {
        (**self).f32s_reduce_sum(a)
    }

    #[inline(always)]
    fn f32s_reduce_product(self, a: Self::f32s) -> f32 {
        (**self).f32s_reduce_product(a)
    }

    #[inline(always)]
    fn f32s_reduce_min(self, a: Self::f32s) -> f32 {
        (**self).f32s_reduce_min(a)
    }

    #[inline(always)]
    fn f32s_reduce_max(self, a: Self::f32s) -> f32 {
        (**self).f32s_reduce_max(a)
    }

    #[inline(always)]
    fn f64s_reduce_sum(self, a: Self::f64s) -> f64 {
        (**self).f64s_reduce_sum(a)
    }

    #[inline(always)]
    fn f64s_reduce_product(self, a: Self::f64s) -> f64 {
        (**self).f64s_reduce_product(a)
    }

    #[inline(always)]
    fn f64s_reduce_min(self, a: Self::f64s) -> f64 {
        (**self).f64s_reduce_min(a)
    }

    #[inline(always)]
    fn f64s_reduce_max(self, a: Self::f64s) -> f64 {
        (**self).f64s_reduce_max(a)
    }

    type c32s = f32x8;
    type c64s = f64x4;

    #[inline(always)]
    fn c32s_splat(self, value: c32) -> Self::c32s {
        unsafe { transmute(self.f64s_splat(transmute(value))) }
    }
    #[inline(always)]
    fn c32s_add(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.f32s_add(a, b)
    }
    #[inline(always)]
    fn c32s_sub(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.f32s_sub(a, b)
    }

    #[inline(always)]
    fn c32s_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = _mm256_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm256_moveldup_ps(ab);
            let bb = _mm256_movehdup_ps(ab);

            transmute(_mm256_fmaddsub_ps(aa, xy, _mm256_mul_ps(bb, yx)))
        }
    }

    #[inline(always)]
    fn f32s_mul_add(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        self.f32s_mul_add_e(a, b, c)
    }
    #[inline(always)]
    fn f64s_mul_add(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        self.f64s_mul_add_e(a, b, c)
    }

    #[inline(always)]
    fn c64s_splat(self, value: c64) -> Self::c64s {
        unsafe { transmute(_mm256_broadcast_f32x4(transmute(value))) }
    }
    #[inline(always)]
    fn c64s_add(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.f64s_add(a, b)
    }
    #[inline(always)]
    fn c64s_sub(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.f64s_sub(a, b)
    }

    #[inline(always)]
    fn c64s_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = _mm256_permute_pd::<0b0101>(xy);
            let aa = _mm256_unpacklo_pd(ab, ab);
            let bb = _mm256_unpackhi_pd(ab, ab);

            transmute(_mm256_fmaddsub_pd(aa, xy, _mm256_mul_pd(bb, yx)))
        }
    }

    #[inline(always)]
    fn c32s_abs2(self, a: Self::c32s) -> Self::c32s {
        unsafe {
            let sqr = self.f32s_mul(a, a);
            let sqr_rev = _mm256_shuffle_ps::<0b10_11_00_01>(transmute(sqr), transmute(sqr));
            self.f32s_add(sqr, transmute(sqr_rev))
        }
    }

    #[inline(always)]
    fn c64s_abs2(self, a: Self::c64s) -> Self::c64s {
        unsafe {
            let sqr = self.f64s_mul(a, a);
            let sqr_rev = _mm256_shuffle_pd::<0b01010101>(transmute(sqr), transmute(sqr));
            self.f64s_add(sqr, transmute(sqr_rev))
        }
    }

    #[inline(always)]
    fn u32s_partial_load(self, slice: &[u32]) -> Self::u32s {
        unsafe {
            let mask = V4_256_U32_MASKS[slice.len().min(16)];
            transmute(_mm256_maskz_loadu_epi32(mask, slice.as_ptr() as _))
        }
    }

    #[inline(always)]
    fn u32s_partial_store(self, slice: &mut [u32], values: Self::u32s) {
        unsafe {
            let mask = V4_256_U32_MASKS[slice.len().min(16)];
            _mm256_mask_storeu_epi32(slice.as_mut_ptr() as _, mask, transmute(values));
        }
    }

    #[inline(always)]
    fn u64s_partial_load(self, slice: &[u64]) -> Self::u64s {
        unsafe {
            let mask = V4_256_U32_MASKS[(2 * slice.len()).min(16)];
            transmute(_mm256_maskz_loadu_epi32(mask, slice.as_ptr() as _))
        }
    }

    #[inline(always)]
    fn u64s_partial_store(self, slice: &mut [u64], values: Self::u64s) {
        unsafe {
            let mask = V4_256_U32_MASKS[(2 * slice.len()).min(16)];
            _mm256_mask_storeu_epi32(slice.as_mut_ptr() as _, mask, transmute(values));
        }
    }

    #[inline(always)]
    fn c64s_partial_load(self, slice: &[c64]) -> Self::c64s {
        unsafe {
            let mask = V4_256_U32_MASKS[(4 * slice.len()).min(16)];
            transmute(_mm256_maskz_loadu_epi32(mask, slice.as_ptr() as _))
        }
    }

    #[inline(always)]
    fn c64s_partial_store(self, slice: &mut [c64], values: Self::c64s) {
        unsafe {
            let mask = V4_256_U32_MASKS[(4 * slice.len()).min(16)];
            _mm256_mask_storeu_epi32(slice.as_mut_ptr() as _, mask, transmute(values));
        }
    }

    #[inline(always)]
    fn u32s_partial_load_last(self, slice: &[u32]) -> Self::u32s {
        unsafe {
            let len = slice.len();
            let mask = V4_256_U32_LAST_MASKS[slice.len().min(16)];
            transmute(_mm256_maskz_loadu_epi32(
                mask,
                slice.as_ptr().add(len).wrapping_sub(16) as _,
            ))
        }
    }

    #[inline(always)]
    fn u32s_partial_store_last(self, slice: &mut [u32], values: Self::u32s) {
        unsafe {
            let len = slice.len();
            let mask = V4_256_U32_LAST_MASKS[slice.len().min(16)];
            _mm256_mask_storeu_epi32(
                slice.as_mut_ptr().add(len).wrapping_sub(16) as _,
                mask,
                transmute(values),
            );
        }
    }

    #[inline(always)]
    fn u64s_partial_load_last(self, slice: &[u64]) -> Self::u64s {
        unsafe {
            let len = slice.len();
            let mask = V4_256_U32_LAST_MASKS[(2 * slice.len()).min(16)];
            transmute(_mm256_maskz_loadu_epi32(
                mask,
                slice.as_ptr().add(len).wrapping_sub(8) as _,
            ))
        }
    }

    #[inline(always)]
    fn u64s_partial_store_last(self, slice: &mut [u64], values: Self::u64s) {
        unsafe {
            let len = slice.len();
            let mask = V4_256_U32_LAST_MASKS[(2 * slice.len()).min(16)];
            _mm256_mask_storeu_epi32(
                slice.as_mut_ptr().add(len).wrapping_sub(8) as _,
                mask,
                transmute(values),
            );
        }
    }

    #[inline(always)]
    fn c64s_partial_load_last(self, slice: &[c64]) -> Self::c64s {
        unsafe {
            let len = slice.len();
            let mask = V4_256_U32_LAST_MASKS[(4 * slice.len()).min(16)];
            transmute(_mm256_maskz_loadu_epi32(
                mask,
                slice.as_ptr().add(len).wrapping_sub(4) as _,
            ))
        }
    }

    #[inline(always)]
    fn c64s_partial_store_last(self, slice: &mut [c64], values: Self::c64s) {
        unsafe {
            let len = slice.len();
            let mask = V4_256_U32_LAST_MASKS[(4 * slice.len()).min(16)];
            _mm256_mask_storeu_epi32(
                slice.as_mut_ptr().add(len).wrapping_sub(4) as _,
                mask,
                transmute(values),
            );
        }
    }

    #[inline(always)]
    fn c32s_conj(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.c32s_splat(c32 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c32s_conj_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        (**self).c32s_conj_mul(a, b)
    }

    #[inline(always)]
    fn c32s_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        (**self).c32s_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c32s_conj_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        (**self).c32s_conj_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c64s_conj(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.c64s_splat(c64 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c64s_conj_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        (**self).c64s_mul(a, b)
    }

    #[inline(always)]
    fn c64s_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        (**self).c64s_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c64s_conj_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        (**self).c64s_conj_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c32s_neg(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.f32s_splat(-0.0))
    }

    #[inline(always)]
    fn c32s_reduce_sum(self, a: Self::c32s) -> c32 {
        (**self).c32s_reduce_sum(a)
    }

    #[inline(always)]
    fn c64s_neg(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.f64s_splat(-0.0))
    }

    #[inline(always)]
    fn c64s_reduce_sum(self, a: Self::c64s) -> c64 {
        (**self).c64s_reduce_sum(a)
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shl(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        self.shl_dyn_u32x8(a, self.and_u32x8(amount, self.splat_u32x8(32 - 1)))
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shr(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        self.shr_dyn_u32x8(a, self.and_u32x8(amount, self.splat_u32x8(32 - 1)))
    }

    #[inline(always)]
    fn u32s_widening_mul(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
        self.widening_mul_u32x8(a, b)
    }

    #[inline(always)]
    fn u64s_less_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_lt_u64x4(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_gt_u64x4(a, b)
    }

    #[inline(always)]
    fn u64s_less_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_le_u64x4(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_ge_u64x4(a, b)
    }

    #[inline(always)]
    fn u32s_less_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_lt_u32x8(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_gt_u32x8(a, b)
    }

    #[inline(always)]
    fn u32s_less_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_le_u32x8(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_ge_u32x8(a, b)
    }

    #[inline(always)]
    fn c32_scalar_mul(self, a: c32, b: c32) -> c32 {
        (*self).c32_scalar_mul(a, b)
    }
    #[inline(always)]
    fn c32_scalar_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        (*self).c32_scalar_mul_add(a, b, c)
    }
    #[inline(always)]
    fn c32_scalar_conj_mul(self, a: c32, b: c32) -> c32 {
        (*self).c32_scalar_conj_mul(a, b)
    }
    #[inline(always)]
    fn c32_scalar_conj_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        (*self).c32_scalar_conj_mul_add(a, b, c)
    }

    #[inline(always)]
    fn c64_scalar_mul(self, a: c64, b: c64) -> c64 {
        (*self).c64_scalar_mul(a, b)
    }
    #[inline(always)]
    fn c64_scalar_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        (*self).c64_scalar_mul_add(a, b, c)
    }
    #[inline(always)]
    fn c64_scalar_conj_mul(self, a: c64, b: c64) -> c64 {
        (*self).c64_scalar_conj_mul(a, b)
    }
    #[inline(always)]
    fn c64_scalar_conj_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        (*self).c64_scalar_conj_mul_add(a, b, c)
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const u32,
        or: Self::u32s,
    ) -> Self::u32s {
        transmute(_mm256_mask_loadu_epi32(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const f32,
        or: Self::f32s,
    ) -> Self::f32s {
        transmute(_mm256_mask_loadu_ps(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const c32,
        or: Self::c32s,
    ) -> Self::c32s {
        transmute(_mm256_mask_loadu_ps(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const u64,
        or: Self::u64s,
    ) -> Self::u64s {
        transmute(_mm256_mask_loadu_epi64(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const f64,
        or: Self::f64s,
    ) -> Self::f64s {
        transmute(_mm256_mask_loadu_pd(transmute(or), mask.0, ptr as _))
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const c64,
        or: Self::c64s,
    ) -> Self::c64s {
        transmute(_mm256_mask_loadu_pd(transmute(or), mask.0, ptr as _))
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut u32, values: Self::u32s) {
        _mm256_mask_storeu_epi32(ptr as *mut i32, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut f32, values: Self::f32s) {
        _mm256_mask_storeu_ps(ptr, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut c32, values: Self::c32s) {
        _mm256_mask_storeu_ps(ptr as *mut f32, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut u64, values: Self::u64s) {
        _mm256_mask_storeu_epi64(ptr as *mut i64, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn f64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut f64, values: Self::f64s) {
        _mm256_mask_storeu_pd(ptr, mask.0, transmute(values));
    }
    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut c64, values: Self::c64s) {
        _mm256_mask_storeu_pd(ptr as *mut f64, mask.0, transmute(values));
    }

    #[inline(always)]
    fn u32s_rotate_right(self, a: Self::u32s, amount: usize) -> Self::u32s {
        unsafe {
            transmute(_mm256_permutexvar_epi32(
                transmute(AVX512_256_ROTATE_IDX[amount % 16]),
                transmute(a),
            ))
        }
    }

    #[inline(always)]
    fn c32s_rotate_right(self, a: Self::c32s, amount: usize) -> Self::c32s {
        unsafe {
            transmute(_mm256_permutexvar_epi32(
                transmute(AVX512_256_ROTATE_IDX[2 * (amount % 8)]),
                transmute(a),
            ))
        }
    }

    #[inline(always)]
    fn u64s_rotate_right(self, a: Self::u64s, amount: usize) -> Self::u64s {
        unsafe {
            transmute(_mm256_permutexvar_epi32(
                transmute(AVX512_256_ROTATE_IDX[2 * (amount % 8)]),
                transmute(a),
            ))
        }
    }

    #[inline(always)]
    fn c64s_rotate_right(self, a: Self::c64s, amount: usize) -> Self::c64s {
        unsafe {
            transmute(_mm256_permutexvar_epi32(
                transmute(AVX512_256_ROTATE_IDX[4 * (amount % 4)]),
                transmute(a),
            ))
        }
    }

    #[inline(always)]
    fn c32s_swap_re_im(self, a: Self::c32s) -> Self::c32s {
        unsafe { cast(_mm256_permute_ps::<0b10_11_00_01>(cast(a))) }
    }

    #[inline(always)]
    fn c64s_swap_re_im(self, a: Self::c64s) -> Self::c64s {
        unsafe { cast(_mm256_permute_pd::<0b0101>(cast(a))) }
    }
}

impl V2 {
    //-------------------------------------------------------------------------------
    // splat
    //-------------------------------------------------------------------------------

    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u8x16(self, value: u8) -> u8x16 {
        cast(self.sse2._mm_set1_epi8(value as i8))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i8x16(self, value: i8) -> i8x16 {
        cast(self.sse2._mm_set1_epi8(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m8x16(self, value: m8) -> m8x16 {
        unsafe { transmute(self.sse2._mm_set1_epi8(value.0 as i8)) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u16x8(self, value: u16) -> u16x8 {
        cast(self.sse2._mm_set1_epi16(value as i16))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i16x8(self, value: i16) -> i16x8 {
        cast(self.sse2._mm_set1_epi16(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m16x8(self, value: m16) -> m16x8 {
        unsafe { transmute(self.sse2._mm_set1_epi16(value.0 as i16)) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u32x4(self, value: u32) -> u32x4 {
        cast(self.sse2._mm_set1_epi32(value as i32))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i32x4(self, value: i32) -> i32x4 {
        cast(self.sse2._mm_set1_epi32(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m32x4(self, value: m32) -> m32x4 {
        unsafe { transmute(self.sse2._mm_set1_epi32(value.0 as i32)) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_f32x4(self, value: f32) -> f32x4 {
        cast(self.sse._mm_set1_ps(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u64x2(self, value: u64) -> u64x2 {
        cast(self.sse2._mm_set1_epi64x(value as i64))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i64x2(self, value: i64) -> i64x2 {
        cast(self.sse2._mm_set1_epi64x(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m64x2(self, value: m64) -> m64x2 {
        unsafe { transmute(self.sse2._mm_set1_epi64x(value.0 as i64)) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_f64x2(self, value: f64) -> f64x2 {
        cast(self.sse2._mm_set1_pd(value))
    }

    //-------------------------------------------------------------------------------
    // bitwise
    //-------------------------------------------------------------------------------

    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_and_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse2._mm_and_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
        unsafe { transmute(self.sse2._mm_and_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_and_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_and_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
        unsafe { transmute(self.sse2._mm_and_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        cast(self.sse2._mm_and_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.sse2._mm_and_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
        unsafe { transmute(self.sse2._mm_and_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_and_ps(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        cast(self.sse2._mm_and_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        cast(self.sse2._mm_and_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_and_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_and_pd(cast(a), cast(b)))
    }

    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_or_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse2._mm_or_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
        unsafe { transmute(self.sse2._mm_or_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_or_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_or_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
        unsafe { transmute(self.sse2._mm_or_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        cast(self.sse2._mm_or_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.sse2._mm_or_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
        unsafe { transmute(self.sse2._mm_or_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_or_ps(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        cast(self.sse2._mm_or_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        cast(self.sse2._mm_or_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_or_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_or_pd(cast(a), cast(b)))
    }

    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_xor_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse2._mm_xor_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
        unsafe { transmute(self.sse2._mm_xor_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_xor_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_xor_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
        unsafe { transmute(self.sse2._mm_xor_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        cast(self.sse2._mm_xor_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.sse2._mm_xor_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
        unsafe { transmute(self.sse2._mm_xor_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_xor_ps(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        cast(self.sse2._mm_xor_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        cast(self.sse2._mm_xor_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_xor_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_xor_pd(cast(a), cast(b)))
    }

    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u8x16(self, a: u8x16) -> u8x16 {
        self.xor_u8x16(a, self.splat_u8x16(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i8x16(self, a: i8x16) -> i8x16 {
        self.xor_i8x16(a, self.splat_i8x16(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m8x16(self, a: m8x16) -> m8x16 {
        self.xor_m8x16(a, self.splat_m8x16(m8::new(true)))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u16x8(self, a: u16x8) -> u16x8 {
        self.xor_u16x8(a, self.splat_u16x8(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i16x8(self, a: i16x8) -> i16x8 {
        self.xor_i16x8(a, self.splat_i16x8(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m16x8(self, a: m16x8) -> m16x8 {
        self.xor_m16x8(a, self.splat_m16x8(m16::new(true)))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u32x4(self, a: u32x4) -> u32x4 {
        self.xor_u32x4(a, self.splat_u32x4(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i32x4(self, a: i32x4) -> i32x4 {
        self.xor_i32x4(a, self.splat_i32x4(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m32x4(self, a: m32x4) -> m32x4 {
        self.xor_m32x4(a, self.splat_m32x4(m32::new(true)))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u64x2(self, a: u64x2) -> u64x2 {
        self.xor_u64x2(a, self.splat_u64x2(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i64x2(self, a: i64x2) -> i64x2 {
        self.xor_i64x2(a, self.splat_i64x2(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m64x2(self, a: m64x2) -> m64x2 {
        self.xor_m64x2(a, self.splat_m64x2(m64::new(true)))
    }

    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_andnot_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse2._mm_andnot_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
        unsafe { transmute(self.sse2._mm_andnot_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_andnot_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_andnot_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
        unsafe { transmute(self.sse2._mm_andnot_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        cast(self.sse2._mm_andnot_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.sse2._mm_andnot_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
        unsafe { transmute(self.sse2._mm_andnot_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_andnot_ps(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        cast(self.sse2._mm_andnot_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        cast(self.sse2._mm_andnot_si128(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_andnot_si128(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_andnot_pd(cast(a), cast(b)))
    }

    //-------------------------------------------------------------------------------
    // bit shifts
    //-------------------------------------------------------------------------------

    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_u16x8<const AMOUNT: i32>(self, a: u16x8) -> u16x8 {
        cast(self.sse2._mm_slli_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_i16x8<const AMOUNT: i32>(self, a: i16x8) -> i16x8 {
        cast(self.sse2._mm_slli_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_u32x4<const AMOUNT: i32>(self, a: u32x4) -> u32x4 {
        cast(self.sse2._mm_slli_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_i32x4<const AMOUNT: i32>(self, a: i32x4) -> i32x4 {
        cast(self.sse2._mm_slli_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_u64x2<const AMOUNT: i32>(self, a: u64x2) -> u64x2 {
        cast(self.sse2._mm_slli_epi64::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_i64x2<const AMOUNT: i32>(self, a: i64x2) -> i64x2 {
        cast(self.sse2._mm_slli_epi64::<AMOUNT>(cast(a)))
    }

    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_const_u16x8<const AMOUNT: i32>(self, a: u16x8) -> u16x8 {
        cast(self.sse2._mm_srli_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_const_i16x8<const AMOUNT: i32>(self, a: i16x8) -> i16x8 {
        cast(self.sse2._mm_srai_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_const_u32x4<const AMOUNT: i32>(self, a: u32x4) -> u32x4 {
        cast(self.sse2._mm_srli_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_const_i32x4<const AMOUNT: i32>(self, a: i32x4) -> i32x4 {
        cast(self.sse2._mm_srai_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_const_u64x2<const AMOUNT: i32>(self, a: u64x2) -> u64x2 {
        cast(self.sse2._mm_srli_epi64::<AMOUNT>(cast(a)))
    }

    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_u16x8(self, a: u16x8, amount: u64x2) -> u16x8 {
        cast(self.sse2._mm_sll_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_i16x8(self, a: i16x8, amount: u64x2) -> i16x8 {
        cast(self.sse2._mm_sll_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_u32x4(self, a: u32x4, amount: u64x2) -> u32x4 {
        cast(self.sse2._mm_sll_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_i32x4(self, a: i32x4, amount: u64x2) -> i32x4 {
        cast(self.sse2._mm_sll_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_u64x2(self, a: u64x2, amount: u64x2) -> u64x2 {
        cast(self.sse2._mm_sll_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_i64x2(self, a: i64x2, amount: u64x2) -> u64x2 {
        cast(self.sse2._mm_sll_epi64(cast(a), cast(amount)))
    }

    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_u16x8(self, a: u16x8, amount: u64x2) -> u16x8 {
        cast(self.sse2._mm_srl_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_i16x8(self, a: i16x8, amount: u64x2) -> i16x8 {
        cast(self.sse2._mm_sra_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_u32x4(self, a: u32x4, amount: u64x2) -> u32x4 {
        cast(self.sse2._mm_srl_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_i32x4(self, a: i32x4, amount: u64x2) -> i32x4 {
        cast(self.sse2._mm_sra_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_u64x2(self, a: u64x2, amount: u64x2) -> u64x2 {
        cast(self.sse2._mm_srl_epi64(cast(a), cast(amount)))
    }

    //-------------------------------------------------------------------------------
    // arithmetic
    //-------------------------------------------------------------------------------

    /// Adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn add_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_add_ps(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn add_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_add_pd(cast(a), cast(b)))
    }

    /// Subtracts the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn sub_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_sub_ps(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn sub_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_sub_pd(cast(a), cast(b)))
    }

    /// Alternatively subtracts and adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn subadd_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse3._mm_addsub_ps(cast(a), cast(b)))
    }
    /// Alternatively subtracts and adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn subadd_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse3._mm_addsub_pd(cast(a), cast(b)))
    }

    /// Multiplies the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn mul_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_mul_ps(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn mul_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_mul_pd(cast(a), cast(b)))
    }

    /// Divides the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn div_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_div_ps(cast(a), cast(b)))
    }
    /// Divides the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn div_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_div_pd(cast(a), cast(b)))
    }

    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_add_epi8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse2._mm_add_epi8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_add_epi16(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_add_epi16(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        cast(self.sse2._mm_add_epi32(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.sse2._mm_add_epi32(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        cast(self.sse2._mm_add_epi64(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        cast(self.sse2._mm_add_epi64(cast(a), cast(b)))
    }

    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_adds_epu8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse2._mm_adds_epi8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_adds_epu16(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_adds_epi16(cast(a), cast(b)))
    }

    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_sub_epi8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse2._mm_sub_epi8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_sub_epi16(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_sub_epi16(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        cast(self.sse2._mm_sub_epi32(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.sse2._mm_sub_epi32(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        cast(self.sse2._mm_sub_epi64(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        cast(self.sse2._mm_sub_epi64(cast(a), cast(b)))
    }

    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_subs_epu8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse2._mm_subs_epi8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_subs_epu16(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_subs_epi16(cast(a), cast(b)))
    }

    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_mullo_epi16(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_mullo_epi16(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        cast(self.sse4_1._mm_mullo_epi32(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.sse4_1._mm_mullo_epi32(cast(a), cast(b)))
    }

    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_u16x8(self, a: u16x8, b: u16x8) -> (u16x8, u16x8) {
        (
            cast(self.sse2._mm_mullo_epi16(cast(a), cast(b))),
            cast(self.sse2._mm_mulhi_epu16(cast(a), cast(b))),
        )
    }
    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_i16x8(self, a: i16x8, b: i16x8) -> (i16x8, i16x8) {
        (
            cast(self.sse2._mm_mullo_epi16(cast(a), cast(b))),
            cast(self.sse2._mm_mulhi_epi16(cast(a), cast(b))),
        )
    }
    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_u32x4(self, a: u32x4, b: u32x4) -> (u32x4, u32x4) {
        let a = cast(a);
        let b = cast(b);
        let sse = self.sse2;

        // a0b0_lo a0b0_hi a2b2_lo a2b2_hi
        let ab_evens = sse._mm_mul_epu32(a, b);
        // a1b1_lo a1b1_hi a3b3_lo a3b3_hi
        let ab_odds = sse._mm_mul_epu32(sse._mm_srli_epi64::<32>(a), sse._mm_srli_epi64::<32>(b));

        let ab_lo = self.sse4_1._mm_blend_ps::<0b1010>(
            // a0b0_lo xxxxxxx a2b2_lo xxxxxxx
            cast(ab_evens),
            // xxxxxxx a1b1_lo xxxxxxx a3b3_lo
            cast(sse._mm_slli_epi64::<32>(ab_odds)),
        );
        let ab_hi = self.sse4_1._mm_blend_ps::<0b1010>(
            // a0b0_hi xxxxxxx a2b2_hi xxxxxxx
            cast(sse._mm_srli_epi64::<32>(ab_evens)),
            // xxxxxxx a1b1_hi xxxxxxx a3b3_hi
            cast(ab_odds),
        );

        (cast(ab_lo), cast(ab_hi))
    }
    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_i32x4(self, a: i32x4, b: i32x4) -> (i32x4, i32x4) {
        let a = cast(a);
        let b = cast(b);
        let sse = self.sse2;

        // a0b0_lo a0b0_hi a2b2_lo a2b2_hi
        let ab_evens = self.sse4_1._mm_mul_epi32(a, b);
        // a1b1_lo a1b1_hi a3b3_lo a3b3_hi
        let ab_odds = self
            .sse4_1
            ._mm_mul_epi32(sse._mm_srli_epi64::<32>(a), sse._mm_srli_epi64::<32>(b));

        let ab_lo = self.sse4_1._mm_blend_ps::<0b1010>(
            // a0b0_lo xxxxxxx a2b2_lo xxxxxxx
            cast(ab_evens),
            // xxxxxxx a1b1_lo xxxxxxx a3b3_lo
            cast(sse._mm_slli_epi64::<32>(ab_odds)),
        );
        let ab_hi = self.sse4_1._mm_blend_ps::<0b1010>(
            // a0b0_hi xxxxxxx a2b2_hi xxxxxxx
            cast(sse._mm_srli_epi64::<32>(ab_evens)),
            // xxxxxxx a1b1_hi xxxxxxx a3b3_hi
            cast(ab_odds),
        );

        (cast(ab_lo), cast(ab_hi))
    }

    //-------------------------------------------------------------------------------
    // math
    //-------------------------------------------------------------------------------

    /// Averages the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn average_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_avg_epu8(cast(a), cast(b)))
    }
    /// Averages the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn average_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse2._mm_avg_epu16(cast(a), cast(b)))
    }

    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_min_epu8(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse4_1._mm_min_epi8(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse4_1._mm_min_epu16(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_min_epi16(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        cast(self.sse4_1._mm_min_epu32(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.sse4_1._mm_min_epi32(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_min_ps(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_min_pd(cast(a), cast(b)))
    }

    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        cast(self.sse2._mm_max_epu8(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        cast(self.sse4_1._mm_max_epi8(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        cast(self.sse4_1._mm_max_epu16(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.sse2._mm_max_epi16(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        cast(self.sse4_1._mm_max_epu32(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.sse4_1._mm_max_epi32(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse._mm_max_ps(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse2._mm_max_pd(cast(a), cast(b)))
    }

    /// Computes the absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn abs_f32x4(self, a: f32x4) -> f32x4 {
        self.and_f32x4(a, cast(self.splat_u32x4((1 << 31) - 1)))
    }
    /// Computes the absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn abs_f64x2(self, a: f64x2) -> f64x2 {
        self.and_f64x2(a, cast(self.splat_u64x2((1 << 63) - 1)))
    }

    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i8x16(self, a: i8x16) -> u8x16 {
        cast(self.ssse3._mm_abs_epi8(cast(a)))
    }
    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i16x8(self, a: i16x8) -> u16x8 {
        cast(self.ssse3._mm_abs_epi16(cast(a)))
    }
    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i32x4(self, a: i32x4) -> u32x4 {
        cast(self.ssse3._mm_abs_epi32(cast(a)))
    }

    /// Applies the sign of each element of `sign` to the corresponding lane in `a`.
    /// - If `sign` is zero, the corresponding element is zeroed.
    /// - If `sign` is positive, the corresponding element is returned as is.
    /// - If `sign` is negative, the corresponding element is negated.
    #[inline(always)]
    pub fn apply_sign_i8x16(self, sign: i8x16, a: i8x16) -> i8x16 {
        cast(self.ssse3._mm_sign_epi8(cast(a), cast(sign)))
    }
    /// Applies the sign of each element of `sign` to the corresponding lane in `a`.
    /// - If `sign` is zero, the corresponding element is zeroed.
    /// - If `sign` is positive, the corresponding element is returned as is.
    /// - If `sign` is negative, the corresponding element is negated.
    #[inline(always)]
    pub fn apply_sign_i16x8(self, sign: i16x8, a: i16x8) -> i16x8 {
        cast(self.ssse3._mm_sign_epi16(cast(a), cast(sign)))
    }
    /// Applies the sign of each element of `sign` to the corresponding lane in `a`.
    /// - If `sign` is zero, the corresponding element is zeroed.
    /// - If `sign` is positive, the corresponding element is returned as is.
    /// - If `sign` is negative, the corresponding element is negated.
    #[inline(always)]
    pub fn apply_sign_i32x4(self, sign: i32x4, a: i32x4) -> i32x4 {
        cast(self.ssse3._mm_sign_epi32(cast(a), cast(sign)))
    }

    /// Computes the square roots of the elements of each lane of `a`.
    #[inline(always)]
    pub fn sqrt_f32x4(self, a: f32x4) -> f32x4 {
        cast(self.sse._mm_sqrt_ps(cast(a)))
    }
    /// Computes the square roots of the elements of each lane of `a`.
    #[inline(always)]
    pub fn sqrt_f64x2(self, a: f64x2) -> f64x2 {
        cast(self.sse2._mm_sqrt_pd(cast(a)))
    }

    /// Computes the approximate reciprocal of the elements of each lane of `a`.
    #[inline(always)]
    pub fn approx_reciprocal_f32x4(self, a: f32x4) -> f32x4 {
        cast(self.sse._mm_rcp_ps(cast(a)))
    }
    /// Computes the approximate reciprocal of the square roots of the elements of each lane of `a`.
    #[inline(always)]
    pub fn approx_reciprocal_sqrt_f32x4(self, a: f32x4) -> f32x4 {
        cast(self.sse._mm_rsqrt_ps(cast(a)))
    }

    /// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
    #[inline(always)]
    pub fn floor_f32x4(self, a: f32x4) -> f32x4 {
        cast(self.sse4_1._mm_floor_ps(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
    #[inline(always)]
    pub fn floor_f64x2(self, a: f64x2) -> f64x2 {
        cast(self.sse4_1._mm_floor_pd(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards positive infinity.
    #[inline(always)]
    pub fn ceil_f32x4(self, a: f32x4) -> f32x4 {
        cast(self.sse4_1._mm_ceil_ps(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards positive infinity.
    #[inline(always)]
    pub fn ceil_f64x2(self, a: f64x2) -> f64x2 {
        cast(self.sse4_1._mm_ceil_pd(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
    /// close, the even value is returned.
    #[inline(always)]
    pub fn round_f32x4(self, a: f32x4) -> f32x4 {
        const ROUNDING: i32 = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC;
        cast(self.sse4_1._mm_round_ps::<ROUNDING>(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
    /// close, the even value is returned.
    #[inline(always)]
    pub fn round_f64x2(self, a: f64x2) -> f64x2 {
        const ROUNDING: i32 = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC;
        cast(self.sse4_1._mm_round_pd::<ROUNDING>(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards zero.
    #[inline(always)]
    pub fn truncate_f32x4(self, a: f32x4) -> f32x4 {
        const ROUNDING: i32 = _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC;
        cast(self.sse4_1._mm_round_ps::<ROUNDING>(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards zero.
    #[inline(always)]
    pub fn truncate_f64x2(self, a: f64x2) -> f64x2 {
        const ROUNDING: i32 = _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC;
        cast(self.sse4_1._mm_round_pd::<ROUNDING>(cast(a)))
    }

    /// See `_mm_hadd_epi16`.
    #[inline(always)]
    pub fn horizontal_add_pack_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.ssse3._mm_hadd_epi16(cast(a), cast(b)))
    }
    /// See `_mm_hadd_epi32`.
    #[inline(always)]
    pub fn horizontal_add_pack_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.ssse3._mm_hadd_epi32(cast(a), cast(b)))
    }
    /// See `_mm_hadd_ps`.
    #[inline(always)]
    pub fn horizontal_add_pack_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse3._mm_hadd_ps(cast(a), cast(b)))
    }
    /// See `_mm_hadd_pd`.
    #[inline(always)]
    pub fn horizontal_add_pack_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse3._mm_hadd_pd(cast(a), cast(b)))
    }

    /// See `_mm_hsub_epi16`
    #[inline(always)]
    pub fn horizontal_sub_pack_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.ssse3._mm_hsub_epi16(cast(a), cast(b)))
    }
    /// See `_mm_hsub_epi32`
    #[inline(always)]
    pub fn horizontal_sub_pack_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        cast(self.ssse3._mm_hsub_epi32(cast(a), cast(b)))
    }
    /// See `_mm_hsub_ps`
    #[inline(always)]
    pub fn horizontal_sub_pack_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        cast(self.sse3._mm_hsub_ps(cast(a), cast(b)))
    }
    /// See `_mm_hsub_pd`
    #[inline(always)]
    pub fn horizontal_sub_pack_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        cast(self.sse3._mm_hsub_pd(cast(a), cast(b)))
    }

    /// See `_mm_hadds_epi16`
    #[inline(always)]
    pub fn horizontal_saturating_add_pack_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.ssse3._mm_hadds_epi16(cast(a), cast(b)))
    }
    /// See `_mm_hsubs_epi16`
    #[inline(always)]
    pub fn horizontal_saturating_sub_pack_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        cast(self.ssse3._mm_hsubs_epi16(cast(a), cast(b)))
    }

    /// See `_mm_madd_epi16`
    #[inline(always)]
    pub fn multiply_wrapping_add_adjacent_i16x8(self, a: i16x8, b: i16x8) -> i32x4 {
        cast(self.sse2._mm_madd_epi16(cast(a), cast(b)))
    }
    /// See `_mm_maddubs_epi16`
    #[inline(always)]
    pub fn multiply_saturating_add_adjacent_i8x16(self, a: i8x16, b: i8x16) -> i16x8 {
        cast(self.ssse3._mm_maddubs_epi16(cast(a), cast(b)))
    }

    /// See `_mm_mpsadbw_epu8`.
    #[inline(always)]
    pub fn multisum_of_absolute_differences_u8x16<const OFFSETS: i32>(
        self,
        a: u8x16,
        b: u8x16,
    ) -> u16x8 {
        cast(self.sse4_1._mm_mpsadbw_epu8::<OFFSETS>(cast(a), cast(b)))
    }

    /// See `_mm_packs_epi16`
    #[inline(always)]
    pub fn pack_with_signed_saturation_i16x8(self, a: i16x8, b: i16x8) -> i8x16 {
        cast(self.sse2._mm_packs_epi16(cast(a), cast(b)))
    }
    /// See `_mm_packs_epi32`
    #[inline(always)]
    pub fn pack_with_signed_saturation_i32x4(self, a: i32x4, b: i32x4) -> i16x8 {
        cast(self.sse2._mm_packs_epi32(cast(a), cast(b)))
    }

    /// See `_mm_packus_epi16`
    #[inline(always)]
    pub fn pack_with_unsigned_saturation_i16x8(self, a: i16x8, b: i16x8) -> u8x16 {
        cast(self.sse2._mm_packus_epi16(cast(a), cast(b)))
    }
    /// See `_mm_packus_epi32`
    #[inline(always)]
    pub fn pack_with_unsigned_saturation_i32x4(self, a: i32x4, b: i32x4) -> u16x8 {
        cast(self.sse4_1._mm_packus_epi32(cast(a), cast(b)))
    }

    /// See `_mm_sad_epu8`
    #[inline(always)]
    pub fn sum_of_absolute_differences_u8x16(self, a: u8x16, b: u8x16) -> u64x2 {
        cast(self.sse2._mm_sad_epu8(cast(a), cast(b)))
    }

    //-------------------------------------------------------------------------------
    // conversions
    //-------------------------------------------------------------------------------

    /// Converts a `u8x16` to `i8x16`, elementwise.
    #[inline(always)]
    pub fn convert_u8x16_to_i8x16(self, a: u8x16) -> i8x16 {
        cast(a)
    }
    /// Converts a `u8x16` to `u16x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_u16x8(self, a: u8x16) -> u16x8 {
        cast(self.sse4_1._mm_cvtepu8_epi16(cast(a)))
    }
    /// Converts a `u8x16` to `i16x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_i16x8(self, a: u8x16) -> i16x8 {
        cast(self.sse4_1._mm_cvtepu8_epi16(cast(a)))
    }
    /// Converts a `u8x16` to `u32x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_u32x4(self, a: u8x16) -> u32x4 {
        cast(self.sse4_1._mm_cvtepu8_epi32(cast(a)))
    }
    /// Converts a `u8x16` to `i32x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_i32x4(self, a: u8x16) -> i32x4 {
        cast(self.sse4_1._mm_cvtepu8_epi32(cast(a)))
    }
    /// Converts a `u8x16` to `u64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_u64x2(self, a: u8x16) -> u64x2 {
        cast(self.sse4_1._mm_cvtepu8_epi64(cast(a)))
    }
    /// Converts a `u8x16` to `i64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_i64x2(self, a: u8x16) -> i64x2 {
        cast(self.sse4_1._mm_cvtepu8_epi64(cast(a)))
    }

    /// Converts a `i8x16` to `u8x16`, elementwise.
    #[inline(always)]
    pub fn convert_i8x16_to_u8x16(self, a: i8x16) -> u8x16 {
        cast(a)
    }
    /// Converts a `i8x16` to `u16x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_u16x8(self, a: i8x16) -> u16x8 {
        cast(self.sse4_1._mm_cvtepi8_epi16(cast(a)))
    }
    /// Converts a `i8x16` to `i16x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_i16x8(self, a: i8x16) -> i16x8 {
        cast(self.sse4_1._mm_cvtepi8_epi16(cast(a)))
    }
    /// Converts a `i8x16` to `u32x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_u32x4(self, a: i8x16) -> u32x4 {
        cast(self.sse4_1._mm_cvtepi8_epi32(cast(a)))
    }
    /// Converts a `i8x16` to `i32x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_i32x4(self, a: i8x16) -> i32x4 {
        cast(self.sse4_1._mm_cvtepi8_epi32(cast(a)))
    }
    /// Converts a `i8x16` to `u64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_u64x2(self, a: i8x16) -> u64x2 {
        cast(self.sse4_1._mm_cvtepi8_epi64(cast(a)))
    }
    /// Converts a `i8x16` to `i64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_i64x2(self, a: i8x16) -> i64x2 {
        cast(self.sse4_1._mm_cvtepi8_epi64(cast(a)))
    }

    /// Converts a `u16x8` to `i16x8`, elementwise.
    #[inline(always)]
    pub fn convert_u16x8_to_i16x8(self, a: u16x8) -> i16x8 {
        cast(a)
    }
    /// Converts a `u16x8` to `u32x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u16x8_to_u32x4(self, a: u16x8) -> u32x4 {
        cast(self.sse4_1._mm_cvtepu16_epi32(cast(a)))
    }
    /// Converts a `u16x8` to `i32x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u16x8_to_i32x4(self, a: u16x8) -> i32x4 {
        cast(self.sse4_1._mm_cvtepu16_epi32(cast(a)))
    }
    /// Converts a `u16x8` to `u64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u16x8_to_u64x2(self, a: u16x8) -> u64x2 {
        cast(self.sse4_1._mm_cvtepu16_epi64(cast(a)))
    }
    /// Converts a `u16x8` to `i64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u16x8_to_i64x2(self, a: u16x8) -> i64x2 {
        cast(self.sse4_1._mm_cvtepu16_epi64(cast(a)))
    }

    /// Converts a `i16x8` to `u16x8`, elementwise.
    #[inline(always)]
    pub fn convert_i16x8_to_u16x8(self, a: i16x8) -> u16x8 {
        cast(a)
    }
    /// Converts a `i16x8` to `u32x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i16x8_to_u32x4(self, a: i16x8) -> u32x4 {
        cast(self.sse4_1._mm_cvtepi16_epi32(cast(a)))
    }
    /// Converts a `i16x8` to `i32x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i16x8_to_i32x4(self, a: i16x8) -> i32x4 {
        cast(self.sse4_1._mm_cvtepi16_epi32(cast(a)))
    }
    /// Converts a `i16x8` to `u64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i16x8_to_u64x2(self, a: i16x8) -> u64x2 {
        cast(self.sse4_1._mm_cvtepi16_epi64(cast(a)))
    }
    /// Converts a `i16x8` to `i64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i16x8_to_i64x2(self, a: i16x8) -> i64x2 {
        cast(self.sse4_1._mm_cvtepi16_epi64(cast(a)))
    }

    /// Converts a `u32x4` to `i32x4`, elementwise.
    #[inline(always)]
    pub fn convert_u32x4_to_i32x4(self, a: u32x4) -> i32x4 {
        cast(a)
    }
    /// Converts a `u32x4` to `u64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u32x4_to_u64x2(self, a: u32x4) -> u64x2 {
        cast(self.sse4_1._mm_cvtepu32_epi64(cast(a)))
    }
    /// Converts a `u32x4` to `i64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u32x4_to_i64x2(self, a: u32x4) -> i64x2 {
        cast(self.sse4_1._mm_cvtepu32_epi64(cast(a)))
    }

    /// Converts a `i32x4` to `u32x4`, elementwise.
    #[inline(always)]
    pub fn convert_i32x4_to_u32x4(self, a: i32x4) -> u32x4 {
        cast(a)
    }
    /// Converts a `i32x4` to `f32x4`, elementwise.
    #[inline(always)]
    pub fn convert_i32x4_to_f32x4(self, a: i32x4) -> f32x4 {
        cast(self.sse2._mm_cvtepi32_ps(cast(a)))
    }
    /// Converts a `i32x4` to `u64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i32x4_to_u64x2(self, a: i32x4) -> u64x2 {
        cast(self.sse4_1._mm_cvtepi32_epi64(cast(a)))
    }
    /// Converts a `i32x4` to `i64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i32x4_to_i64x2(self, a: i32x4) -> i64x2 {
        cast(self.sse4_1._mm_cvtepi32_epi64(cast(a)))
    }
    /// Converts a `i32x4` to `f64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i32x4_to_f64x2(self, a: i32x4) -> f64x2 {
        cast(self.sse2._mm_cvtepi32_pd(cast(a)))
    }

    /// Converts a `f32x4` to `i32x4`, elementwise.
    #[inline(always)]
    pub fn convert_f32x4_to_i32x4(self, a: f32x4) -> i32x4 {
        cast(self.sse2._mm_cvttps_epi32(cast(a)))
    }
    /// Converts a `f32x4` to `f64x2`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_f32x4_to_f64x2(self, a: f32x4) -> f64x2 {
        cast(self.sse2._mm_cvtps_pd(cast(a)))
    }

    /// Converts a `f64x2` to `i32x4`, elementwise.
    #[inline(always)]
    pub fn convert_f64x2_to_i32x4(self, a: f64x2) -> i32x4 {
        cast(self.sse2._mm_cvttpd_epi32(cast(a)))
    }
    /// Converts a `f64x2` to `f32x4`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_f64x2_to_f32x4(self, a: f64x2) -> f32x4 {
        cast(self.sse2._mm_cvtpd_ps(cast(a)))
    }

    //-------------------------------------------------------------------------------
    // comparisons
    //-------------------------------------------------------------------------------

    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        unsafe { transmute(self.sse2._mm_cmpeq_epi8(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        unsafe { transmute(self.sse2._mm_cmpeq_epi8(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        unsafe { transmute(self.sse2._mm_cmpeq_epi16(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        unsafe { transmute(self.sse2._mm_cmpeq_epi16(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        unsafe { transmute(self.sse2._mm_cmpeq_epi32(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        unsafe { transmute(self.sse2._mm_cmpeq_epi32(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        unsafe { transmute(self.sse4_1._mm_cmpeq_epi64(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        unsafe { transmute(self.sse4_1._mm_cmpeq_epi64(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        let k = self.splat_u8x16(0x80);
        self.cmp_gt_i8x16(cast(self.xor_u8x16(a, k)), cast(self.xor_u8x16(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        unsafe { transmute(self.sse2._mm_cmpgt_epi8(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        let k = self.splat_u16x8(0x8000);
        self.cmp_gt_i16x8(cast(self.xor_u16x8(a, k)), cast(self.xor_u16x8(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        unsafe { transmute(self.sse2._mm_cmpgt_epi16(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        let k = self.splat_u32x4(0x80000000);
        self.cmp_gt_i32x4(cast(self.xor_u32x4(a, k)), cast(self.xor_u32x4(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        unsafe { transmute(self.sse2._mm_cmpgt_epi32(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        let k = self.splat_u64x2(0x8000000000000000);
        self.cmp_gt_i64x2(cast(self.xor_u64x2(a, k)), cast(self.xor_u64x2(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        unsafe { transmute(self.sse4_2._mm_cmpgt_epi64(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        self.not_m8x16(self.cmp_lt_u8x16(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        self.not_m8x16(self.cmp_lt_i8x16(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        self.not_m16x8(self.cmp_lt_u16x8(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        self.not_m16x8(self.cmp_lt_i16x8(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        self.not_m32x4(self.cmp_lt_u32x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        self.not_m32x4(self.cmp_lt_i32x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        self.not_m64x2(self.cmp_lt_u64x2(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        self.not_m64x2(self.cmp_lt_i64x2(a, b))
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        let k = self.splat_u8x16(0x80);
        self.cmp_lt_i8x16(cast(self.xor_u8x16(a, k)), cast(self.xor_u8x16(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        unsafe { transmute(self.sse2._mm_cmplt_epi8(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        let k = self.splat_u16x8(0x8000);
        self.cmp_lt_i16x8(cast(self.xor_u16x8(a, k)), cast(self.xor_u16x8(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        unsafe { transmute(self.sse2._mm_cmplt_epi16(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        let k = self.splat_u32x4(0x80000000);
        self.cmp_lt_i32x4(cast(self.xor_u32x4(a, k)), cast(self.xor_u32x4(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        unsafe { transmute(self.sse2._mm_cmplt_epi32(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        let k = self.splat_u64x2(0x8000000000000000);
        self.cmp_lt_i64x2(cast(self.xor_u64x2(a, k)), cast(self.xor_u64x2(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        unsafe { transmute(self.sse4_2._mm_cmpgt_epi64(cast(b), cast(a))) }
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        self.not_m8x16(self.cmp_gt_u8x16(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        self.not_m8x16(self.cmp_gt_i8x16(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        self.not_m16x8(self.cmp_gt_u16x8(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        self.not_m16x8(self.cmp_gt_i16x8(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        self.not_m32x4(self.cmp_gt_u32x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        self.not_m32x4(self.cmp_gt_i32x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        self.not_m64x2(self.cmp_gt_u64x2(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        self.not_m64x2(self.cmp_gt_i64x2(a, b))
    }

    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpeq_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpeq_pd(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpneq_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpneq_pd(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpgt_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpgt_pd(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpge_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpge_pd(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpngt_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpngt_pd(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpnge_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpnge_pd(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmplt_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmplt_pd(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmple_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmple_pd(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpnlt_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpnlt_pd(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpnle_ps(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpnle_pd(cast(a), cast(b))) }
    }

    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f32x4(self, a: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpunord_ps(cast(a), cast(a))) }
    }
    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f64x2(self, a: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpunord_pd(cast(a), cast(a))) }
    }

    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f32x4(self, a: f32x4) -> m32x4 {
        unsafe { transmute(self.sse._mm_cmpord_ps(cast(a), cast(a))) }
    }
    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f64x2(self, a: f64x2) -> m64x2 {
        unsafe { transmute(self.sse2._mm_cmpord_pd(cast(a), cast(a))) }
    }

    //-------------------------------------------------------------------------------
    // select
    //-------------------------------------------------------------------------------

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_u32x4<const MASK4: i32>(self, if_true: u32x4, if_false: u32x4) -> u32x4 {
        cast(
            self.sse4_1
                ._mm_blend_ps::<MASK4>(cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_i32x4<const MASK4: i32>(self, if_true: i32x4, if_false: i32x4) -> i32x4 {
        cast(self.select_const_u32x4::<MASK4>(cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_f32x4<const MASK4: i32>(self, if_true: f32x4, if_false: f32x4) -> f32x4 {
        cast(self.select_const_u32x4::<MASK4>(cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_u64x2<const MASK2: i32>(self, if_true: u64x2, if_false: u64x2) -> u64x2 {
        cast(
            self.sse4_1
                ._mm_blend_pd::<MASK2>(cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_i64x2<const MASK2: i32>(self, if_true: i64x2, if_false: i64x2) -> i64x2 {
        cast(self.select_const_u64x2::<MASK2>(cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_f64x2<const MASK2: i32>(self, if_true: f64x2, if_false: f64x2) -> f64x2 {
        cast(self.select_const_u64x2::<MASK2>(cast(if_true), cast(if_false)))
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u8x16(self, mask: m8x16, if_true: u8x16, if_false: u8x16) -> u8x16 {
        cast(
            self.sse4_1
                ._mm_blendv_epi8(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i8x16(self, mask: m8x16, if_true: i8x16, if_false: i8x16) -> i8x16 {
        cast(self.select_u8x16(mask, cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u16x8(self, mask: m16x8, if_true: u16x8, if_false: u16x8) -> u16x8 {
        cast(
            self.sse4_1
                ._mm_blendv_epi8(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i16x8(self, mask: m16x8, if_true: i16x8, if_false: i16x8) -> i16x8 {
        cast(self.select_u16x8(mask, cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u32x4(self, mask: m32x4, if_true: u32x4, if_false: u32x4) -> u32x4 {
        cast(
            self.sse4_1
                ._mm_blendv_epi8(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i32x4(self, mask: m32x4, if_true: i32x4, if_false: i32x4) -> i32x4 {
        cast(self.select_u32x4(mask, cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f32x4(self, mask: m32x4, if_true: f32x4, if_false: f32x4) -> f32x4 {
        cast(
            self.sse4_1
                ._mm_blendv_ps(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u64x2(self, mask: m64x2, if_true: u64x2, if_false: u64x2) -> u64x2 {
        cast(
            self.sse4_1
                ._mm_blendv_epi8(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i64x2(self, mask: m64x2, if_true: i64x2, if_false: i64x2) -> i64x2 {
        cast(self.select_u64x2(mask, cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f64x2(self, mask: m64x2, if_true: f64x2, if_false: f64x2) -> f64x2 {
        cast(
            self.sse4_1
                ._mm_blendv_pd(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
}

impl V3 {
    //-------------------------------------------------------------------------------
    // splat
    //-------------------------------------------------------------------------------

    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u8x32(self, value: u8) -> u8x32 {
        cast(self.avx._mm256_set1_epi8(value as i8))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i8x32(self, value: i8) -> i8x32 {
        cast(self.avx._mm256_set1_epi8(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m8x32(self, value: m8) -> m8x32 {
        unsafe { transmute(self.avx._mm256_set1_epi8(value.0 as i8)) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u16x16(self, value: u16) -> u16x16 {
        cast(self.avx._mm256_set1_epi16(value as i16))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i16x16(self, value: i16) -> i16x16 {
        cast(self.avx._mm256_set1_epi16(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m16x16(self, value: m16) -> m16x16 {
        unsafe { transmute(self.avx._mm256_set1_epi16(value.0 as i16)) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u32x8(self, value: u32) -> u32x8 {
        cast(self.avx._mm256_set1_epi32(value as i32))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i32x8(self, value: i32) -> i32x8 {
        cast(self.avx._mm256_set1_epi32(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m32x8(self, value: m32) -> m32x8 {
        unsafe { transmute(self.avx._mm256_set1_epi32(value.0 as i32)) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_f32x8(self, value: f32) -> f32x8 {
        cast(self.avx._mm256_set1_ps(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u64x4(self, value: u64) -> u64x4 {
        cast(self.avx._mm256_set1_epi64x(value as i64))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i64x4(self, value: i64) -> i64x4 {
        cast(self.avx._mm256_set1_epi64x(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m64x4(self, value: m64) -> m64x4 {
        unsafe { transmute(self.avx._mm256_set1_epi64x(value.0 as i64)) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_f64x4(self, value: f64) -> f64x4 {
        cast(self.avx._mm256_set1_pd(value))
    }

    //-------------------------------------------------------------------------------
    // bitwise
    //-------------------------------------------------------------------------------

    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_and_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_and_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m8x32(self, a: m8x32, b: m8x32) -> m8x32 {
        unsafe { transmute(self.avx2._mm256_and_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_and_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_and_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m16x16(self, a: m16x16, b: m16x16) -> m16x16 {
        unsafe { transmute(self.avx2._mm256_and_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u32x8(self, a: u32x8, b: u32x8) -> u32x8 {
        cast(self.avx2._mm256_and_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_and_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m32x8(self, a: m32x8, b: m32x8) -> m32x8 {
        unsafe { transmute(self.avx2._mm256_and_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_and_ps(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
        cast(self.avx2._mm256_and_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
        cast(self.avx2._mm256_and_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m64x4(self, a: m64x4, b: m64x4) -> m64x4 {
        unsafe { transmute(self.avx2._mm256_and_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_and_pd(cast(a), cast(b)))
    }

    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_or_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_or_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m8x32(self, a: m8x32, b: m8x32) -> m8x32 {
        unsafe { transmute(self.avx2._mm256_or_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_or_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_or_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m16x16(self, a: m16x16, b: m16x16) -> m16x16 {
        unsafe { transmute(self.avx2._mm256_or_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u32x8(self, a: u32x8, b: u32x8) -> u32x8 {
        cast(self.avx2._mm256_or_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_or_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m32x8(self, a: m32x8, b: m32x8) -> m32x8 {
        unsafe { transmute(self.avx2._mm256_or_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_or_ps(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
        cast(self.avx2._mm256_or_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
        cast(self.avx2._mm256_or_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m64x4(self, a: m64x4, b: m64x4) -> m64x4 {
        unsafe { transmute(self.avx2._mm256_or_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_or_pd(cast(a), cast(b)))
    }

    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_xor_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_xor_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m8x32(self, a: m8x32, b: m8x32) -> m8x32 {
        unsafe { transmute(self.avx2._mm256_xor_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_xor_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_xor_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m16x16(self, a: m16x16, b: m16x16) -> m16x16 {
        unsafe { transmute(self.avx2._mm256_xor_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u32x8(self, a: u32x8, b: u32x8) -> u32x8 {
        cast(self.avx2._mm256_xor_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_xor_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m32x8(self, a: m32x8, b: m32x8) -> m32x8 {
        unsafe { transmute(self.avx2._mm256_xor_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_xor_ps(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
        cast(self.avx2._mm256_xor_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
        cast(self.avx2._mm256_xor_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m64x4(self, a: m64x4, b: m64x4) -> m64x4 {
        unsafe { transmute(self.avx2._mm256_xor_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_xor_pd(cast(a), cast(b)))
    }

    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u8x32(self, a: u8x32) -> u8x32 {
        self.xor_u8x32(a, self.splat_u8x32(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i8x32(self, a: i8x32) -> i8x32 {
        self.xor_i8x32(a, self.splat_i8x32(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m8x32(self, a: m8x32) -> m8x32 {
        self.xor_m8x32(a, self.splat_m8x32(m8::new(true)))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u16x16(self, a: u16x16) -> u16x16 {
        self.xor_u16x16(a, self.splat_u16x16(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i16x16(self, a: i16x16) -> i16x16 {
        self.xor_i16x16(a, self.splat_i16x16(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m16x16(self, a: m16x16) -> m16x16 {
        self.xor_m16x16(a, self.splat_m16x16(m16::new(true)))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u32x8(self, a: u32x8) -> u32x8 {
        self.xor_u32x8(a, self.splat_u32x8(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i32x8(self, a: i32x8) -> i32x8 {
        self.xor_i32x8(a, self.splat_i32x8(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m32x8(self, a: m32x8) -> m32x8 {
        self.xor_m32x8(a, self.splat_m32x8(m32::new(true)))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u64x4(self, a: u64x4) -> u64x4 {
        self.xor_u64x4(a, self.splat_u64x4(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i64x4(self, a: i64x4) -> i64x4 {
        self.xor_i64x4(a, self.splat_i64x4(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m64x4(self, a: m64x4) -> m64x4 {
        self.xor_m64x4(a, self.splat_m64x4(m64::new(true)))
    }

    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_andnot_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_andnot_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m8x32(self, a: m8x32, b: m8x32) -> m8x32 {
        unsafe { transmute(self.avx2._mm256_andnot_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_andnot_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_andnot_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m16x16(self, a: m16x16, b: m16x16) -> m16x16 {
        unsafe { transmute(self.avx2._mm256_andnot_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u32x8(self, a: u32x8, b: u32x8) -> u32x8 {
        cast(self.avx2._mm256_andnot_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_andnot_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m32x8(self, a: m32x8, b: m32x8) -> m32x8 {
        unsafe { transmute(self.avx2._mm256_andnot_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_andnot_ps(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
        cast(self.avx2._mm256_andnot_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
        cast(self.avx2._mm256_andnot_si256(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m64x4(self, a: m64x4, b: m64x4) -> m64x4 {
        unsafe { transmute(self.avx2._mm256_andnot_si256(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_andnot_pd(cast(a), cast(b)))
    }

    //-------------------------------------------------------------------------------
    // bit shifts
    //-------------------------------------------------------------------------------

    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_u16x16<const AMOUNT: i32>(self, a: u16x16) -> u16x16 {
        cast(self.avx2._mm256_slli_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_i16x16<const AMOUNT: i32>(self, a: i16x16) -> i16x16 {
        cast(self.avx2._mm256_slli_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_u32x8<const AMOUNT: i32>(self, a: u32x8) -> u32x8 {
        cast(self.avx2._mm256_slli_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_i32x8<const AMOUNT: i32>(self, a: i32x8) -> i32x8 {
        cast(self.avx2._mm256_slli_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_u64x4<const AMOUNT: i32>(self, a: u64x4) -> u64x4 {
        cast(self.avx2._mm256_slli_epi64::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_i64x4<const AMOUNT: i32>(self, a: i64x4) -> i64x4 {
        cast(self.avx2._mm256_slli_epi64::<AMOUNT>(cast(a)))
    }

    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_const_u16x16<const AMOUNT: i32>(self, a: u16x16) -> u16x16 {
        cast(self.avx2._mm256_srli_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_const_i16x16<const AMOUNT: i32>(self, a: i16x16) -> i16x16 {
        cast(self.avx2._mm256_srai_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_const_u32x8<const AMOUNT: i32>(self, a: u32x8) -> u32x8 {
        cast(self.avx2._mm256_srli_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_const_i32x8<const AMOUNT: i32>(self, a: i32x8) -> i32x8 {
        cast(self.avx2._mm256_srai_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_const_u64x4<const AMOUNT: i32>(self, a: u64x4) -> u64x4 {
        cast(self.avx2._mm256_srli_epi64::<AMOUNT>(cast(a)))
    }

    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_u16x16(self, a: u16x16, amount: u64x2) -> u16x16 {
        cast(self.avx2._mm256_sll_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_i16x16(self, a: i16x16, amount: u64x2) -> i16x16 {
        cast(self.avx2._mm256_sll_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_u32x8(self, a: u32x8, amount: u64x2) -> u32x8 {
        cast(self.avx2._mm256_sll_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_i32x8(self, a: i32x8, amount: u64x2) -> i32x8 {
        cast(self.avx2._mm256_sll_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_u64x4(self, a: u64x4, amount: u64x2) -> u64x4 {
        cast(self.avx2._mm256_sll_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_i64x4(self, a: i64x4, amount: u64x2) -> i64x4 {
        cast(self.avx2._mm256_sll_epi64(cast(a), cast(amount)))
    }

    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_u16x16(self, a: u16x16, amount: u64x2) -> u16x16 {
        cast(self.avx2._mm256_srl_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_i16x16(self, a: i16x16, amount: u64x2) -> i16x16 {
        cast(self.avx2._mm256_sra_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_u32x8(self, a: u32x8, amount: u64x2) -> u32x8 {
        cast(self.avx2._mm256_srl_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_i32x8(self, a: i32x8, amount: u64x2) -> i32x8 {
        cast(self.avx2._mm256_sra_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_u64x4(self, a: u64x4, amount: u64x2) -> u64x4 {
        cast(self.avx2._mm256_srl_epi64(cast(a), cast(amount)))
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_u32x4(self, a: u32x4, amount: u32x4) -> u32x4 {
        cast(self.avx2._mm_sllv_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_i32x4(self, a: i32x4, amount: u32x4) -> i32x4 {
        cast(self.avx2._mm_sllv_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_u32x8(self, a: u32x8, amount: u32x8) -> u32x8 {
        cast(self.avx2._mm256_sllv_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_i32x8(self, a: i32x8, amount: u32x8) -> i32x8 {
        cast(self.avx2._mm256_sllv_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_u64x2(self, a: u64x2, amount: u64x2) -> u64x2 {
        cast(self.avx2._mm_sllv_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_i64x2(self, a: i64x2, amount: u64x2) -> i64x2 {
        cast(self.avx2._mm_sllv_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_u64x4(self, a: u64x4, amount: u64x4) -> u64x4 {
        cast(self.avx2._mm256_sllv_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_i64x4(self, a: i64x4, amount: u64x4) -> i64x4 {
        cast(self.avx2._mm256_sllv_epi64(cast(a), cast(amount)))
    }

    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_dyn_u32x4(self, a: u32x4, amount: u32x4) -> u32x4 {
        cast(self.avx2._mm_srlv_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_dyn_u32x8(self, a: u32x8, amount: u32x8) -> u32x8 {
        cast(self.avx2._mm256_srlv_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_dyn_i32x4(self, a: i32x4, amount: i32x4) -> i32x4 {
        cast(self.avx2._mm_srav_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_dyn_i32x8(self, a: i32x8, amount: i32x8) -> i32x8 {
        cast(self.avx2._mm256_srav_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_dyn_u64x2(self, a: u64x2, amount: u64x2) -> u64x2 {
        cast(self.avx2._mm_srlv_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_dyn_u64x4(self, a: u64x4, amount: u64x4) -> u64x4 {
        cast(self.avx2._mm256_srlv_epi64(cast(a), cast(amount)))
    }

    //-------------------------------------------------------------------------------
    // arithmetic
    //-------------------------------------------------------------------------------

    /// Adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn add_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_add_ps(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn add_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_add_pd(cast(a), cast(b)))
    }

    /// Subtracts the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn sub_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_sub_ps(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn sub_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_sub_pd(cast(a), cast(b)))
    }

    /// Alternatively subtracts and adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn subadd_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_addsub_ps(cast(a), cast(b)))
    }
    /// Alternatively subtracts and adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn subadd_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_addsub_pd(cast(a), cast(b)))
    }

    /// Multiplies the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn mul_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_mul_ps(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn mul_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_mul_pd(cast(a), cast(b)))
    }

    /// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
    /// `c`.
    #[inline(always)]
    pub fn mul_add_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
        cast(self.fma._mm_fmadd_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
    /// `c`.
    #[inline(always)]
    pub fn mul_add_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
        cast(self.fma._mm256_fmadd_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
    /// `c`.
    #[inline(always)]
    pub fn mul_add_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
        cast(self.fma._mm_fmadd_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
    /// `c`.
    #[inline(always)]
    pub fn mul_add_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
        cast(self.fma._mm256_fmadd_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the results.
    #[inline(always)]
    pub fn mul_sub_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
        cast(self.fma._mm_fmsub_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the results.
    #[inline(always)]
    pub fn mul_sub_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
        cast(self.fma._mm256_fmsub_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the results.
    #[inline(always)]
    pub fn mul_sub_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
        cast(self.fma._mm_fmsub_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the results.
    #[inline(always)]
    pub fn mul_sub_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
        cast(self.fma._mm256_fmsub_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
    /// each lane of `c`.
    #[inline(always)]
    pub fn negate_mul_add_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
        cast(self.fma._mm_fnmadd_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
    /// each lane of `c`.
    #[inline(always)]
    pub fn negate_mul_add_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
        cast(self.fma._mm256_fnmadd_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
    /// each lane of `c`.
    #[inline(always)]
    pub fn negate_mul_add_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
        cast(self.fma._mm_fnmadd_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
    /// each lane of `c`.
    #[inline(always)]
    pub fn negate_mul_add_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
        cast(self.fma._mm256_fnmadd_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the negation of the results.
    #[inline(always)]
    pub fn negate_mul_sub_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
        cast(self.fma._mm_fnmsub_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the negation of the results.
    #[inline(always)]
    pub fn negate_mul_sub_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
        cast(self.fma._mm256_fnmsub_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the negation of the results.
    #[inline(always)]
    pub fn negate_mul_sub_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
        cast(self.fma._mm_fnmsub_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the negation of the results.
    #[inline(always)]
    pub fn negate_mul_sub_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
        cast(self.fma._mm256_fnmsub_pd(cast(a), cast(b), cast(c)))
    }

    /// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_addsub_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
        cast(self.fma._mm_fmsubadd_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_addsub_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
        cast(self.fma._mm256_fmsubadd_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_addsub_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
        cast(self.fma._mm_fmsubadd_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_addsub_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
        cast(self.fma._mm256_fmsubadd_pd(cast(a), cast(b), cast(c)))
    }

    /// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_subadd_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
        cast(self.fma._mm_fmaddsub_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_subadd_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
        cast(self.fma._mm256_fmaddsub_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_subadd_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
        cast(self.fma._mm_fmaddsub_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_subadd_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
        cast(self.fma._mm256_fmaddsub_pd(cast(a), cast(b), cast(c)))
    }

    /// Divides the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn div_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_div_ps(cast(a), cast(b)))
    }
    /// Divides the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn div_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_div_pd(cast(a), cast(b)))
    }

    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_add_epi8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_add_epi8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_add_epi16(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_add_epi16(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u32x8(self, a: u32x8, b: u32x8) -> u32x8 {
        cast(self.avx2._mm256_add_epi32(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_add_epi32(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
        cast(self.avx2._mm256_add_epi64(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
        cast(self.avx2._mm256_add_epi64(cast(a), cast(b)))
    }

    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_adds_epu8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_adds_epi8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_adds_epu16(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_adds_epi16(cast(a), cast(b)))
    }

    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_sub_epi8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_sub_epi8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_sub_epi16(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_sub_epi16(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u32x8(self, a: u32x8, b: u32x8) -> u32x8 {
        cast(self.avx2._mm256_sub_epi32(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_sub_epi32(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
        cast(self.avx2._mm256_sub_epi64(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
        cast(self.avx2._mm256_sub_epi64(cast(a), cast(b)))
    }

    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_subs_epu8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_subs_epi8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_subs_epu16(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_subs_epi16(cast(a), cast(b)))
    }

    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_mullo_epi16(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_mullo_epi16(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_u32x8(self, a: u32x8, b: u32x8) -> u32x8 {
        cast(self.avx2._mm256_mullo_epi32(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_mullo_epi32(cast(a), cast(b)))
    }

    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_u16x16(self, a: u16x16, b: u16x16) -> (u16x16, u16x16) {
        (
            cast(self.avx2._mm256_mullo_epi16(cast(a), cast(b))),
            cast(self.avx2._mm256_mulhi_epu16(cast(a), cast(b))),
        )
    }
    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_i16x16(self, a: i16x16, b: i16x16) -> (i16x16, i16x16) {
        (
            cast(self.avx2._mm256_mullo_epi16(cast(a), cast(b))),
            cast(self.avx2._mm256_mulhi_epi16(cast(a), cast(b))),
        )
    }
    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_u32x8(self, a: u32x8, b: u32x8) -> (u32x8, u32x8) {
        let a = cast(a);
        let b = cast(b);
        let avx2 = self.avx2;

        // a0b0_lo a0b0_hi a2b2_lo a2b2_hi
        let ab_evens = avx2._mm256_mul_epu32(a, b);
        // a1b1_lo a1b1_hi a3b3_lo a3b3_hi
        let ab_odds = avx2._mm256_mul_epu32(
            avx2._mm256_srli_epi64::<32>(a),
            avx2._mm256_srli_epi64::<32>(b),
        );

        let ab_lo = self.avx2._mm256_blend_epi32::<0b10101010>(
            // a0b0_lo xxxxxxx a2b2_lo xxxxxxx
            cast(ab_evens),
            // xxxxxxx a1b1_lo xxxxxxx a3b3_lo
            cast(avx2._mm256_slli_epi64::<32>(ab_odds)),
        );
        let ab_hi = self.avx2._mm256_blend_epi32::<0b10101010>(
            // a0b0_hi xxxxxxx a2b2_hi xxxxxxx
            cast(avx2._mm256_srli_epi64::<32>(ab_evens)),
            // xxxxxxx a1b1_hi xxxxxxx a3b3_hi
            cast(ab_odds),
        );

        (cast(ab_lo), cast(ab_hi))
    }
    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_i32x8(self, a: i32x8, b: i32x8) -> (i32x8, i32x8) {
        let a = cast(a);
        let b = cast(b);
        let avx2 = self.avx2;

        // a0b0_lo a0b0_hi a2b2_lo a2b2_hi
        let ab_evens = self.avx2._mm256_mul_epi32(a, b);
        // a1b1_lo a1b1_hi a3b3_lo a3b3_hi
        let ab_odds = self.avx2._mm256_mul_epi32(
            avx2._mm256_srli_epi64::<32>(a),
            avx2._mm256_srli_epi64::<32>(b),
        );

        let ab_lo = self.avx2._mm256_blend_epi32::<0b10101010>(
            // a0b0_lo xxxxxxx a2b2_lo xxxxxxx
            cast(ab_evens),
            // xxxxxxx a1b1_lo xxxxxxx a3b3_lo
            cast(avx2._mm256_slli_epi64::<32>(ab_odds)),
        );
        let ab_hi = self.avx2._mm256_blend_epi32::<0b10101010>(
            // a0b0_hi xxxxxxx a2b2_hi xxxxxxx
            cast(avx2._mm256_srli_epi64::<32>(ab_evens)),
            // xxxxxxx a1b1_hi xxxxxxx a3b3_hi
            cast(ab_odds),
        );

        (cast(ab_lo), cast(ab_hi))
    }

    //-------------------------------------------------------------------------------
    // math
    //-------------------------------------------------------------------------------

    /// Averages the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn average_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_avg_epu8(cast(a), cast(b)))
    }
    /// Averages the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn average_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_avg_epu16(cast(a), cast(b)))
    }

    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_min_epu8(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_min_epi8(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_min_epu16(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_min_epi16(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u32x8(self, a: u32x8, b: u32x8) -> u32x8 {
        cast(self.avx2._mm256_min_epu32(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_min_epi32(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_min_ps(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_min_pd(cast(a), cast(b)))
    }

    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u8x32(self, a: u8x32, b: u8x32) -> u8x32 {
        cast(self.avx2._mm256_max_epu8(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i8x32(self, a: i8x32, b: i8x32) -> i8x32 {
        cast(self.avx2._mm256_max_epi8(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u16x16(self, a: u16x16, b: u16x16) -> u16x16 {
        cast(self.avx2._mm256_max_epu16(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_max_epi16(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u32x8(self, a: u32x8, b: u32x8) -> u32x8 {
        cast(self.avx2._mm256_max_epu32(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_max_epi32(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_max_ps(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_max_pd(cast(a), cast(b)))
    }

    /// Computes the absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn abs_f32x8(self, a: f32x8) -> f32x8 {
        self.and_f32x8(a, cast(self.splat_u32x8((1 << 31) - 1)))
    }
    /// Computes the absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn abs_f64x4(self, a: f64x4) -> f64x4 {
        self.and_f64x4(a, cast(self.splat_u64x4((1 << 63) - 1)))
    }

    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i8x32(self, a: i8x32) -> u8x32 {
        cast(self.avx2._mm256_abs_epi8(cast(a)))
    }
    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i16x16(self, a: i16x16) -> u16x16 {
        cast(self.avx2._mm256_abs_epi16(cast(a)))
    }
    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i32x8(self, a: i32x8) -> u32x8 {
        cast(self.avx2._mm256_abs_epi32(cast(a)))
    }

    /// Applies the sign of each element of `sign` to the corresponding lane in `a`.
    /// - If `sign` is zero, the corresponding element is zeroed.
    /// - If `sign` is positive, the corresponding element is returned as is.
    /// - If `sign` is negative, the corresponding element is negated.
    #[inline(always)]
    pub fn apply_sign_i8x32(self, sign: i8x32, a: i8x32) -> i8x32 {
        cast(self.avx2._mm256_sign_epi8(cast(a), cast(sign)))
    }
    /// Applies the sign of each element of `sign` to the corresponding lane in `a`.
    /// - If `sign` is zero, the corresponding element is zeroed.
    /// - If `sign` is positive, the corresponding element is returned as is.
    /// - If `sign` is negative, the corresponding element is negated.
    #[inline(always)]
    pub fn apply_sign_i16x16(self, sign: i16x16, a: i16x16) -> i16x16 {
        cast(self.avx2._mm256_sign_epi16(cast(a), cast(sign)))
    }
    /// Applies the sign of each element of `sign` to the corresponding lane in `a`.
    /// - If `sign` is zero, the corresponding element is zeroed.
    /// - If `sign` is positive, the corresponding element is returned as is.
    /// - If `sign` is negative, the corresponding element is negated.
    #[inline(always)]
    pub fn apply_sign_i32x8(self, sign: i32x8, a: i32x8) -> i32x8 {
        cast(self.avx2._mm256_sign_epi32(cast(a), cast(sign)))
    }

    /// Computes the square roots of the elements of each lane of `a`.
    #[inline(always)]
    pub fn sqrt_f32x8(self, a: f32x8) -> f32x8 {
        cast(self.avx._mm256_sqrt_ps(cast(a)))
    }
    /// Computes the square roots of the elements of each lane of `a`.
    #[inline(always)]
    pub fn sqrt_f64x4(self, a: f64x4) -> f64x4 {
        cast(self.avx._mm256_sqrt_pd(cast(a)))
    }

    /// Computes the approximate reciprocal of the elements of each lane of `a`.
    #[inline(always)]
    pub fn approx_reciprocal_f32x8(self, a: f32x8) -> f32x8 {
        cast(self.avx._mm256_rcp_ps(cast(a)))
    }
    /// Computes the approximate reciprocal of the square roots of the elements of each lane of `a`.
    #[inline(always)]
    pub fn approx_reciprocal_sqrt_f32x8(self, a: f32x8) -> f32x8 {
        cast(self.avx._mm256_rsqrt_ps(cast(a)))
    }

    /// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
    #[inline(always)]
    pub fn floor_f32x8(self, a: f32x8) -> f32x8 {
        cast(self.avx._mm256_floor_ps(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
    #[inline(always)]
    pub fn floor_f64x4(self, a: f64x4) -> f64x4 {
        cast(self.avx._mm256_floor_pd(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards positive infinity.
    #[inline(always)]
    pub fn ceil_f32x8(self, a: f32x8) -> f32x8 {
        cast(self.avx._mm256_ceil_ps(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards positive infinity.
    #[inline(always)]
    pub fn ceil_f64x4(self, a: f64x4) -> f64x4 {
        cast(self.avx._mm256_ceil_pd(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
    /// close, the even value is returned.
    #[inline(always)]
    pub fn round_f32x8(self, a: f32x8) -> f32x8 {
        const ROUNDING: i32 = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC;
        cast(self.avx._mm256_round_ps::<ROUNDING>(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
    /// close, the even value is returned.
    #[inline(always)]
    pub fn round_f64x4(self, a: f64x4) -> f64x4 {
        const ROUNDING: i32 = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC;
        cast(self.avx._mm256_round_pd::<ROUNDING>(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards zero.
    #[inline(always)]
    pub fn truncate_f32x8(self, a: f32x8) -> f32x8 {
        const ROUNDING: i32 = _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC;
        cast(self.avx._mm256_round_ps::<ROUNDING>(cast(a)))
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards zero.
    #[inline(always)]
    pub fn truncate_f64x4(self, a: f64x4) -> f64x4 {
        const ROUNDING: i32 = _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC;
        cast(self.avx._mm256_round_pd::<ROUNDING>(cast(a)))
    }

    /// See `_mm256_hadd_epi16`.
    #[inline(always)]
    pub fn horizontal_add_pack_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_hadd_epi16(cast(a), cast(b)))
    }
    /// See `_mm256_hadd_epi32`.
    #[inline(always)]
    pub fn horizontal_add_pack_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_hadd_epi32(cast(a), cast(b)))
    }
    /// See `_mm256_hadd_ps`.
    #[inline(always)]
    pub fn horizontal_add_pack_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_hadd_ps(cast(a), cast(b)))
    }
    /// See `_mm256_hadd_pd`.
    #[inline(always)]
    pub fn horizontal_add_pack_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_hadd_pd(cast(a), cast(b)))
    }

    /// See `_mm256_hsub_epi16`
    #[inline(always)]
    pub fn horizontal_sub_pack_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_hsub_epi16(cast(a), cast(b)))
    }
    /// See `_mm256_hsub_epi32`
    #[inline(always)]
    pub fn horizontal_sub_pack_i32x8(self, a: i32x8, b: i32x8) -> i32x8 {
        cast(self.avx2._mm256_hsub_epi32(cast(a), cast(b)))
    }
    /// See `_mm256_hsub_ps`
    #[inline(always)]
    pub fn horizontal_sub_pack_f32x8(self, a: f32x8, b: f32x8) -> f32x8 {
        cast(self.avx._mm256_hsub_ps(cast(a), cast(b)))
    }
    /// See `_mm256_hsub_pd`
    #[inline(always)]
    pub fn horizontal_sub_pack_f64x4(self, a: f64x4, b: f64x4) -> f64x4 {
        cast(self.avx._mm256_hsub_pd(cast(a), cast(b)))
    }

    /// See `_mm256_hadds_epi16`
    #[inline(always)]
    pub fn horizontal_saturating_add_pack_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_hadds_epi16(cast(a), cast(b)))
    }
    /// See `_mm256_hsubs_epi16`
    #[inline(always)]
    pub fn horizontal_saturating_sub_pack_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
        cast(self.avx2._mm256_hsubs_epi16(cast(a), cast(b)))
    }

    /// See `_mm256_madd_epi16`
    #[inline(always)]
    pub fn multiply_wrapping_add_adjacent_i16x16(self, a: i16x16, b: i16x16) -> i32x8 {
        cast(self.avx2._mm256_madd_epi16(cast(a), cast(b)))
    }
    /// See `_mm256_maddubs_epi16`
    #[inline(always)]
    pub fn multiply_saturating_add_adjacent_i8x32(self, a: i8x32, b: i8x32) -> i16x16 {
        cast(self.avx2._mm256_maddubs_epi16(cast(a), cast(b)))
    }

    /// See `_mm256_mpsadbw_epu8`.
    #[inline(always)]
    pub fn multisum_of_absolute_differences_u8x32<const OFFSETS: i32>(
        self,
        a: u8x32,
        b: u8x32,
    ) -> u16x16 {
        cast(self.avx2._mm256_mpsadbw_epu8::<OFFSETS>(cast(a), cast(b)))
    }

    /// See `_mm256_packs_epi16`
    #[inline(always)]
    pub fn pack_with_signed_saturation_i16x16(self, a: i16x16, b: i16x16) -> i8x32 {
        cast(self.avx2._mm256_packs_epi16(cast(a), cast(b)))
    }
    /// See `_mm256_packs_epi32`
    #[inline(always)]
    pub fn pack_with_signed_saturation_i32x8(self, a: i32x8, b: i32x8) -> i16x16 {
        cast(self.avx2._mm256_packs_epi32(cast(a), cast(b)))
    }

    /// See `_mm256_packus_epi16`
    #[inline(always)]
    pub fn pack_with_unsigned_saturation_i16x16(self, a: i16x16, b: i16x16) -> u8x32 {
        cast(self.avx2._mm256_packus_epi16(cast(a), cast(b)))
    }
    /// See `_mm256_packus_epi32`
    #[inline(always)]
    pub fn pack_with_unsigned_saturation_i32x8(self, a: i32x8, b: i32x8) -> u16x16 {
        cast(self.avx2._mm256_packus_epi32(cast(a), cast(b)))
    }

    /// See `_mm256_sad_epu8`
    #[inline(always)]
    pub fn sum_of_absolute_differences_u8x32(self, a: u8x32, b: u8x32) -> u64x4 {
        cast(self.avx2._mm256_sad_epu8(cast(a), cast(b)))
    }

    //-------------------------------------------------------------------------------
    // conversions
    //-------------------------------------------------------------------------------

    /// Converts a `u8x32` to `i8x32`, elementwise.
    #[inline(always)]
    pub fn convert_u8x32_to_i8x32(self, a: u8x32) -> i8x32 {
        cast(a)
    }
    /// Converts a `u8x16` to `u16x16`, elementwise.
    #[inline(always)]
    pub fn convert_u8x16_to_u16x16(self, a: u8x16) -> u16x16 {
        cast(self.avx2._mm256_cvtepu8_epi16(cast(a)))
    }
    /// Converts a `u8x16` to `i16x16`, elementwise.
    #[inline(always)]
    pub fn convert_u8x16_to_i16x16(self, a: u8x16) -> i16x16 {
        cast(self.avx2._mm256_cvtepu8_epi16(cast(a)))
    }
    /// Converts a `u8x16` to `u32x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_u32x8(self, a: u8x16) -> u32x8 {
        cast(self.avx2._mm256_cvtepu8_epi32(cast(a)))
    }
    /// Converts a `u8x16` to `i32x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_i32x8(self, a: u8x16) -> i32x8 {
        cast(self.avx2._mm256_cvtepu8_epi32(cast(a)))
    }
    /// Converts a `u8x16` to `u64x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_u64x4(self, a: u8x16) -> u64x4 {
        cast(self.avx2._mm256_cvtepu8_epi64(cast(a)))
    }
    /// Converts a `u8x16` to `i64x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_i64x4(self, a: u8x16) -> i64x4 {
        cast(self.avx2._mm256_cvtepu8_epi64(cast(a)))
    }

    /// Converts a `i8x32` to `u8x32`, elementwise.
    #[inline(always)]
    pub fn convert_i8x32_to_u8x32(self, a: i8x32) -> u8x32 {
        cast(a)
    }
    /// Converts a `i8x16` to `u16x16`, elementwise.
    #[inline(always)]
    pub fn convert_i8x16_to_u16x16(self, a: i8x16) -> u16x16 {
        cast(self.avx2._mm256_cvtepi8_epi16(cast(a)))
    }
    /// Converts a `i8x16` to `i16x16`, elementwise.
    #[inline(always)]
    pub fn convert_i8x16_to_i16x16(self, a: i8x16) -> i16x16 {
        cast(self.avx2._mm256_cvtepi8_epi16(cast(a)))
    }
    /// Converts a `i8x16` to `u32x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_u32x8(self, a: i8x16) -> u32x8 {
        cast(self.avx2._mm256_cvtepi8_epi32(cast(a)))
    }
    /// Converts a `i8x16` to `i32x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_i32x8(self, a: i8x16) -> i32x8 {
        cast(self.avx2._mm256_cvtepi8_epi32(cast(a)))
    }
    /// Converts a `i8x16` to `u64x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_u64x4(self, a: i8x16) -> u64x4 {
        cast(self.avx2._mm256_cvtepi8_epi64(cast(a)))
    }
    /// Converts a `i8x16` to `i64x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_i64x4(self, a: i8x16) -> i64x4 {
        cast(self.avx2._mm256_cvtepi8_epi64(cast(a)))
    }

    /// Converts a `u16x16` to `i16x16`, elementwise.
    #[inline(always)]
    pub fn convert_u16x16_to_i16x16(self, a: u16x16) -> i16x16 {
        cast(a)
    }
    /// Converts a `u16x8` to `u32x8`, elementwise.
    #[inline(always)]
    pub fn convert_u16x8_to_u32x8(self, a: u16x8) -> u32x8 {
        cast(self.avx2._mm256_cvtepu16_epi32(cast(a)))
    }
    /// Converts a `u16x8` to `i32x8`, elementwise.
    #[inline(always)]
    pub fn convert_u16x8_to_i32x8(self, a: u16x8) -> i32x8 {
        cast(self.avx2._mm256_cvtepu16_epi32(cast(a)))
    }
    /// Converts a `u16x8` to `u64x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u16x8_to_u64x4(self, a: u16x8) -> u64x4 {
        cast(self.avx2._mm256_cvtepu16_epi64(cast(a)))
    }
    /// Converts a `u16x8` to `i64x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u16x8_to_i64x4(self, a: u16x8) -> i64x4 {
        cast(self.avx2._mm256_cvtepu16_epi64(cast(a)))
    }

    /// Converts a `i16x16` to `u16x16`, elementwise.
    #[inline(always)]
    pub fn convert_i16x16_to_u16x16(self, a: i16x16) -> u16x16 {
        cast(a)
    }
    /// Converts a `i16x8` to `u32x8`, elementwise.
    #[inline(always)]
    pub fn convert_i16x8_to_u32x8(self, a: i16x8) -> u32x8 {
        cast(self.avx2._mm256_cvtepi16_epi32(cast(a)))
    }
    /// Converts a `i16x8` to `i32x8`, elementwise.
    #[inline(always)]
    pub fn convert_i16x8_to_i32x8(self, a: i16x8) -> i32x8 {
        cast(self.avx2._mm256_cvtepi16_epi32(cast(a)))
    }
    /// Converts a `i16x8` to `u64x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i16x8_to_u64x4(self, a: i16x8) -> u64x4 {
        cast(self.avx2._mm256_cvtepi16_epi64(cast(a)))
    }
    /// Converts a `i16x8` to `i64x4`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i16x8_to_i64x4(self, a: i16x8) -> i64x4 {
        cast(self.avx2._mm256_cvtepi16_epi64(cast(a)))
    }

    /// Converts a `u32x8` to `i32x8`, elementwise.
    #[inline(always)]
    pub fn convert_u32x8_to_i32x8(self, a: u32x8) -> i32x8 {
        cast(a)
    }
    /// Converts a `u32x4` to `u64x4`, elementwise.
    #[inline(always)]
    pub fn convert_u32x4_to_u64x4(self, a: u32x4) -> u64x4 {
        cast(self.avx2._mm256_cvtepu32_epi64(cast(a)))
    }
    /// Converts a `u32x4` to `i64x4`, elementwise.
    #[inline(always)]
    pub fn convert_u32x4_to_i64x4(self, a: u32x4) -> i64x4 {
        cast(self.avx2._mm256_cvtepu32_epi64(cast(a)))
    }

    /// Converts a `i32x8` to `u32x8`, elementwise.
    #[inline(always)]
    pub fn convert_i32x8_to_u32x8(self, a: i32x8) -> u32x8 {
        cast(a)
    }
    /// Converts a `i32x8` to `f32x8`, elementwise.
    #[inline(always)]
    pub fn convert_i32x8_to_f32x8(self, a: i32x8) -> f32x8 {
        cast(self.avx._mm256_cvtepi32_ps(cast(a)))
    }
    /// Converts a `i32x4` to `u64x4`, elementwise.
    #[inline(always)]
    pub fn convert_i32x4_to_u64x4(self, a: i32x4) -> u64x4 {
        cast(self.avx2._mm256_cvtepi32_epi64(cast(a)))
    }
    /// Converts a `i32x4` to `i64x4`, elementwise.
    #[inline(always)]
    pub fn convert_i32x4_to_i64x4(self, a: i32x4) -> i64x4 {
        cast(self.avx2._mm256_cvtepi32_epi64(cast(a)))
    }
    /// Converts a `i32x4` to `f64x4`, elementwise.
    #[inline(always)]
    pub fn convert_i32x4_to_f64x4(self, a: i32x4) -> f64x4 {
        cast(self.avx._mm256_cvtepi32_pd(cast(a)))
    }

    /// Converts a `f32x8` to `i32x8`, elementwise.
    #[inline(always)]
    pub fn convert_f32x8_to_i32x8(self, a: f32x8) -> i32x8 {
        cast(self.avx._mm256_cvttps_epi32(cast(a)))
    }
    /// Converts a `f32x4` to `f64x4`, elementwise.
    #[inline(always)]
    pub fn convert_f32x4_to_f64x4(self, a: f32x4) -> f64x4 {
        cast(self.avx._mm256_cvtps_pd(cast(a)))
    }

    /// Converts a `f64x4` to `i32x4`, elementwise.
    #[inline(always)]
    pub fn convert_f64x4_to_i32x4(self, a: f64x4) -> i32x4 {
        cast(self.avx._mm256_cvttpd_epi32(cast(a)))
    }
    /// Converts a `f64x4` to `f32x4`, elementwise.
    #[inline(always)]
    pub fn convert_f64x4_to_f32x4(self, a: f64x4) -> f32x4 {
        cast(self.avx._mm256_cvtpd_ps(cast(a)))
    }

    //-------------------------------------------------------------------------------
    // comparisons
    //-------------------------------------------------------------------------------

    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u8x32(self, a: u8x32, b: u8x32) -> m8x32 {
        unsafe { transmute(self.avx2._mm256_cmpeq_epi8(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i8x32(self, a: i8x32, b: i8x32) -> m8x32 {
        unsafe { transmute(self.avx2._mm256_cmpeq_epi8(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u16x16(self, a: u16x16, b: u16x16) -> m16x16 {
        unsafe { transmute(self.avx2._mm256_cmpeq_epi16(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i16x16(self, a: i16x16, b: i16x16) -> m16x16 {
        unsafe { transmute(self.avx2._mm256_cmpeq_epi16(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u32x8(self, a: u32x8, b: u32x8) -> m32x8 {
        unsafe { transmute(self.avx2._mm256_cmpeq_epi32(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i32x8(self, a: i32x8, b: i32x8) -> m32x8 {
        unsafe { transmute(self.avx2._mm256_cmpeq_epi32(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u64x4(self, a: u64x4, b: u64x4) -> m64x4 {
        unsafe { transmute(self.avx2._mm256_cmpeq_epi64(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i64x4(self, a: i64x4, b: i64x4) -> m64x4 {
        unsafe { transmute(self.avx2._mm256_cmpeq_epi64(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u8x32(self, a: u8x32, b: u8x32) -> m8x32 {
        let k = self.splat_u8x32(0x80);
        self.cmp_gt_i8x32(cast(self.xor_u8x32(a, k)), cast(self.xor_u8x32(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i8x32(self, a: i8x32, b: i8x32) -> m8x32 {
        unsafe { transmute(self.avx2._mm256_cmpgt_epi8(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u16x16(self, a: u16x16, b: u16x16) -> m16x16 {
        let k = self.splat_u16x16(0x8000);
        self.cmp_gt_i16x16(cast(self.xor_u16x16(a, k)), cast(self.xor_u16x16(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i16x16(self, a: i16x16, b: i16x16) -> m16x16 {
        unsafe { transmute(self.avx2._mm256_cmpgt_epi16(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u32x8(self, a: u32x8, b: u32x8) -> m32x8 {
        let k = self.splat_u32x8(0x80000000);
        self.cmp_gt_i32x8(cast(self.xor_u32x8(a, k)), cast(self.xor_u32x8(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i32x8(self, a: i32x8, b: i32x8) -> m32x8 {
        unsafe { transmute(self.avx2._mm256_cmpgt_epi32(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u64x4(self, a: u64x4, b: u64x4) -> m64x4 {
        let k = self.splat_u64x4(0x8000000000000000);
        self.cmp_gt_i64x4(cast(self.xor_u64x4(a, k)), cast(self.xor_u64x4(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i64x4(self, a: i64x4, b: i64x4) -> m64x4 {
        unsafe { transmute(self.avx2._mm256_cmpgt_epi64(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u8x32(self, a: u8x32, b: u8x32) -> m8x32 {
        self.not_m8x32(self.cmp_lt_u8x32(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i8x32(self, a: i8x32, b: i8x32) -> m8x32 {
        self.not_m8x32(self.cmp_lt_i8x32(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u16x16(self, a: u16x16, b: u16x16) -> m16x16 {
        self.not_m16x16(self.cmp_lt_u16x16(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i16x16(self, a: i16x16, b: i16x16) -> m16x16 {
        self.not_m16x16(self.cmp_lt_i16x16(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u32x8(self, a: u32x8, b: u32x8) -> m32x8 {
        self.not_m32x8(self.cmp_lt_u32x8(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i32x8(self, a: i32x8, b: i32x8) -> m32x8 {
        self.not_m32x8(self.cmp_lt_i32x8(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u64x4(self, a: u64x4, b: u64x4) -> m64x4 {
        self.not_m64x4(self.cmp_lt_u64x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i64x4(self, a: i64x4, b: i64x4) -> m64x4 {
        self.not_m64x4(self.cmp_lt_i64x4(a, b))
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u8x32(self, a: u8x32, b: u8x32) -> m8x32 {
        let k = self.splat_u8x32(0x80);
        self.cmp_lt_i8x32(cast(self.xor_u8x32(a, k)), cast(self.xor_u8x32(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i8x32(self, a: i8x32, b: i8x32) -> m8x32 {
        unsafe { transmute(self.avx2._mm256_cmpgt_epi8(cast(b), cast(a))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u16x16(self, a: u16x16, b: u16x16) -> m16x16 {
        let k = self.splat_u16x16(0x8000);
        self.cmp_lt_i16x16(cast(self.xor_u16x16(a, k)), cast(self.xor_u16x16(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i16x16(self, a: i16x16, b: i16x16) -> m16x16 {
        unsafe { transmute(self.avx2._mm256_cmpgt_epi16(cast(b), cast(a))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u32x8(self, a: u32x8, b: u32x8) -> m32x8 {
        let k = self.splat_u32x8(0x80000000);
        self.cmp_lt_i32x8(cast(self.xor_u32x8(a, k)), cast(self.xor_u32x8(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i32x8(self, a: i32x8, b: i32x8) -> m32x8 {
        unsafe { transmute(self.avx2._mm256_cmpgt_epi32(cast(b), cast(a))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u64x4(self, a: u64x4, b: u64x4) -> m64x4 {
        let k = self.splat_u64x4(0x8000000000000000);
        self.cmp_lt_i64x4(cast(self.xor_u64x4(a, k)), cast(self.xor_u64x4(b, k)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i64x4(self, a: i64x4, b: i64x4) -> m64x4 {
        unsafe { transmute(self.avx2._mm256_cmpgt_epi64(cast(b), cast(a))) }
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u8x32(self, a: u8x32, b: u8x32) -> m8x32 {
        self.not_m8x32(self.cmp_gt_u8x32(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i8x32(self, a: i8x32, b: i8x32) -> m8x32 {
        self.not_m8x32(self.cmp_gt_i8x32(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u16x16(self, a: u16x16, b: u16x16) -> m16x16 {
        self.not_m16x16(self.cmp_gt_u16x16(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i16x16(self, a: i16x16, b: i16x16) -> m16x16 {
        self.not_m16x16(self.cmp_gt_i16x16(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u32x8(self, a: u32x8, b: u32x8) -> m32x8 {
        self.not_m32x8(self.cmp_gt_u32x8(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i32x8(self, a: i32x8, b: i32x8) -> m32x8 {
        self.not_m32x8(self.cmp_gt_i32x8(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u64x4(self, a: u64x4, b: u64x4) -> m64x4 {
        self.not_m64x4(self.cmp_gt_u64x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i64x4(self, a: i64x4, b: i64x4) -> m64x4 {
        self.not_m64x4(self.cmp_gt_i64x4(a, b))
    }

    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_EQ_OQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_EQ_OQ>(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_NEQ_UQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_NEQ_UQ>(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_GT_OQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_GT_OQ>(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_GE_OQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_GE_OQ>(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_NGT_UQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_NGT_UQ>(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_NGE_UQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_NGE_UQ>(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_LT_OQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_LT_OQ>(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_LE_OQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_LE_OQ>(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_NLT_UQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_NLT_UQ>(cast(a), cast(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_NLE_UQ>(cast(a), cast(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_NLE_UQ>(cast(a), cast(b))) }
    }

    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f32x8(self, a: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_UNORD_Q>(cast(a), cast(a))) }
    }
    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f64x4(self, a: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_UNORD_Q>(cast(a), cast(a))) }
    }

    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f32x8(self, a: f32x8) -> m32x8 {
        unsafe { transmute(self.avx._mm256_cmp_ps::<_CMP_ORD_Q>(cast(a), cast(a))) }
    }
    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f64x4(self, a: f64x4) -> m64x4 {
        unsafe { transmute(self.avx._mm256_cmp_pd::<_CMP_ORD_Q>(cast(a), cast(a))) }
    }

    //-------------------------------------------------------------------------------
    // select
    //-------------------------------------------------------------------------------

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_u32x8<const MASK8: i32>(self, if_true: u32x8, if_false: u32x8) -> u32x8 {
        cast(
            self.avx2
                ._mm256_blend_epi32::<MASK8>(cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_i32x8<const MASK8: i32>(self, if_true: i32x8, if_false: i32x8) -> i32x8 {
        cast(self.select_const_u32x8::<MASK8>(cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_f32x8<const MASK8: i32>(self, if_true: f32x8, if_false: f32x8) -> f32x8 {
        cast(
            self.avx
                ._mm256_blend_ps::<MASK8>(cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_u64x4<const MASK4: i32>(self, if_true: u64x4, if_false: u64x4) -> u64x4 {
        cast(
            self.avx
                ._mm256_blend_pd::<MASK4>(cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_i64x4<const MASK4: i32>(self, if_true: i64x4, if_false: i64x4) -> i64x4 {
        cast(self.select_const_u64x4::<MASK4>(cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in the mask is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_const_f64x4<const MASK4: i32>(self, if_true: f64x4, if_false: f64x4) -> f64x4 {
        cast(self.select_const_u64x4::<MASK4>(cast(if_true), cast(if_false)))
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u8x32(self, mask: m8x32, if_true: u8x32, if_false: u8x32) -> u8x32 {
        cast(
            self.avx2
                ._mm256_blendv_epi8(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i8x32(self, mask: m8x32, if_true: i8x32, if_false: i8x32) -> i8x32 {
        cast(self.select_u8x32(mask, cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u16x16(self, mask: m16x16, if_true: u16x16, if_false: u16x16) -> u16x16 {
        cast(
            self.avx2
                ._mm256_blendv_epi8(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i16x16(self, mask: m16x16, if_true: i16x16, if_false: i16x16) -> i16x16 {
        cast(self.select_u16x16(mask, cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u32x8(self, mask: m32x8, if_true: u32x8, if_false: u32x8) -> u32x8 {
        cast(
            self.avx2
                ._mm256_blendv_epi8(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i32x8(self, mask: m32x8, if_true: i32x8, if_false: i32x8) -> i32x8 {
        cast(self.select_u32x8(mask, cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f32x8(self, mask: m32x8, if_true: f32x8, if_false: f32x8) -> f32x8 {
        cast(
            self.avx
                ._mm256_blendv_ps(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u64x4(self, mask: m64x4, if_true: u64x4, if_false: u64x4) -> u64x4 {
        cast(
            self.avx2
                ._mm256_blendv_epi8(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i64x4(self, mask: m64x4, if_true: i64x4, if_false: i64x4) -> i64x4 {
        cast(self.select_u64x4(mask, cast(if_true), cast(if_false)))
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f64x4(self, mask: m64x4, if_true: f64x4, if_false: f64x4) -> f64x4 {
        cast(
            self.avx
                ._mm256_blendv_pd(cast(if_false), cast(if_true), unsafe { transmute(mask) }),
        )
    }
}

#[cfg(feature = "nightly")]
#[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
impl V4 {
    #[inline(always)]
    fn fvl(self) -> Avx512f_Avx512vl {
        Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        }
    }
    #[inline(always)]
    fn bwvl(self) -> Avx512bw_Avx512vl {
        Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        }
    }
    //-------------------------------------------------------------------------------
    // splat
    //-------------------------------------------------------------------------------

    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u8x64(self, value: u8) -> u8x64 {
        cast(self.avx512f._mm512_set1_epi8(value as i8))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i8x64(self, value: i8) -> i8x64 {
        cast(self.avx512f._mm512_set1_epi8(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u16x32(self, value: u16) -> u16x32 {
        cast(self.avx512f._mm512_set1_epi16(value as i16))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i16x32(self, value: i16) -> i16x32 {
        cast(self.avx512f._mm512_set1_epi16(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u32x16(self, value: u32) -> u32x16 {
        cast(self.avx512f._mm512_set1_epi32(value as i32))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i32x16(self, value: i32) -> i32x16 {
        cast(self.avx512f._mm512_set1_epi32(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_f32x16(self, value: f32) -> f32x16 {
        cast(self.avx512f._mm512_set1_ps(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u64x8(self, value: u64) -> u64x8 {
        cast(self.avx512f._mm512_set1_epi64(value as i64))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i64x8(self, value: i64) -> i64x8 {
        cast(self.avx512f._mm512_set1_epi64(value))
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_f64x8(self, value: f64) -> f64x8 {
        cast(self.avx512f._mm512_set1_pd(value))
    }

    //-------------------------------------------------------------------------------
    // bitwise
    //-------------------------------------------------------------------------------

    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_and_si512(cast(a), cast(b)))
    }

    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_or_si512(cast(a), cast(b)))
    }

    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_xor_si512(cast(a), cast(b)))
    }

    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u8x64(self, a: u8x64) -> u8x64 {
        self.xor_u8x64(a, self.splat_u8x64(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i8x64(self, a: i8x64) -> i8x64 {
        self.xor_i8x64(a, self.splat_i8x64(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u16x32(self, a: u16x32) -> u16x32 {
        self.xor_u16x32(a, self.splat_u16x32(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i16x32(self, a: i16x32) -> i16x32 {
        self.xor_i16x32(a, self.splat_i16x32(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u32x16(self, a: u32x16) -> u32x16 {
        self.xor_u32x16(a, self.splat_u32x16(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i32x16(self, a: i32x16) -> i32x16 {
        self.xor_i32x16(a, self.splat_i32x16(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u64x8(self, a: u64x8) -> u64x8 {
        self.xor_u64x8(a, self.splat_u64x8(!0))
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i64x8(self, a: i64x8) -> i64x8 {
        self.xor_i64x8(a, self.splat_i64x8(!0))
    }

    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_andnot_si512(cast(a), cast(b)))
    }

    //-------------------------------------------------------------------------------
    // bit shifts
    //-------------------------------------------------------------------------------

    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_u16x32<const AMOUNT: u32>(self, a: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_slli_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_i16x32<const AMOUNT: u32>(self, a: i16x32) -> i16x32 {
        cast(self.avx512bw._mm512_slli_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_u32x16<const AMOUNT: u32>(self, a: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_slli_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_i32x16<const AMOUNT: u32>(self, a: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_slli_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_u64x8<const AMOUNT: u32>(self, a: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_slli_epi64::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_const_i64x8<const AMOUNT: u32>(self, a: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_slli_epi64::<AMOUNT>(cast(a)))
    }

    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_const_u16x32<const AMOUNT: u32>(self, a: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_srli_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_const_i16x32<const AMOUNT: u32>(self, a: i16x32) -> i16x32 {
        cast(self.avx512bw._mm512_srai_epi16::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_const_u32x16<const AMOUNT: u32>(self, a: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_srli_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_const_i32x16<const AMOUNT: u32>(self, a: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_srai_epi32::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_const_u64x8<const AMOUNT: u32>(self, a: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_srli_epi64::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_const_i64x2<const AMOUNT: u32>(self, a: i64x2) -> i64x2 {
        cast(self.fvl()._mm_srai_epi64::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_const_i64x4<const AMOUNT: u32>(self, a: i64x4) -> i64x4 {
        cast(self.fvl()._mm256_srai_epi64::<AMOUNT>(cast(a)))
    }
    /// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_const_i64x8<const AMOUNT: u32>(self, a: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_srai_epi64::<AMOUNT>(cast(a)))
    }

    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_u16x32(self, a: u16x32, amount: u64x2) -> u16x32 {
        cast(self.avx512bw._mm512_sll_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_i16x32(self, a: i16x32, amount: u64x2) -> i16x32 {
        cast(self.avx512bw._mm512_sll_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_u32x16(self, a: u32x16, amount: u64x2) -> u32x16 {
        cast(self.avx512f._mm512_sll_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_i32x16(self, a: i32x16, amount: u64x2) -> i32x16 {
        cast(self.avx512f._mm512_sll_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_u64x8(self, a: u64x8, amount: u64x2) -> u64x8 {
        cast(self.avx512f._mm512_sll_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_i64x8(self, a: i64x8, amount: u64x2) -> i64x8 {
        cast(self.avx512f._mm512_sll_epi64(cast(a), cast(amount)))
    }

    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_u16x32(self, a: u16x32, amount: u64x2) -> u16x32 {
        cast(self.avx512bw._mm512_srl_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_i16x32(self, a: i16x32, amount: u64x2) -> i16x32 {
        cast(self.avx512bw._mm512_sra_epi16(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_u32x16(self, a: u32x16, amount: u64x2) -> u32x16 {
        cast(self.avx512f._mm512_srl_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_i32x16(self, a: i32x16, amount: u64x2) -> i32x16 {
        cast(self.avx512f._mm512_sra_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_u64x8(self, a: u64x8, amount: u64x2) -> u64x8 {
        cast(self.avx512f._mm512_srl_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_i64x2(self, a: i64x2, amount: u64x2) -> i64x2 {
        cast(self.fvl()._mm_sra_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_i64x4(self, a: i64x4, amount: u64x2) -> i64x4 {
        cast(self.fvl()._mm256_sra_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
    /// shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_i64x8(self, a: i64x8, amount: u64x2) -> i64x8 {
        cast(self.avx512f._mm512_sra_epi64(cast(a), cast(amount)))
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_u32x16(self, a: u32x16, amount: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_sllv_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_i32x16(self, a: i32x16, amount: u32x16) -> i32x16 {
        cast(self.avx512f._mm512_sllv_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_u64x8(self, a: u64x8, amount: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_sllv_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shl_dyn_i64x8(self, a: i64x8, amount: u64x8) -> i64x8 {
        cast(self.avx512f._mm512_sllv_epi64(cast(a), cast(amount)))
    }

    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_dyn_u32x16(self, a: u32x16, amount: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_srlv_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_dyn_i32x16(self, a: i32x16, amount: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_srav_epi32(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero.
    #[inline(always)]
    pub fn shr_dyn_u64x8(self, a: u64x8, amount: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_srlv_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_dyn_i64x2(self, a: i64x2, amount: u64x2) -> i64x2 {
        cast(self.fvl()._mm_srav_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_dyn_i64x4(self, a: i64x4, amount: u64x4) -> i64x4 {
        cast(self.fvl()._mm256_srav_epi64(cast(a), cast(amount)))
    }
    /// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
    /// `amount`, while shifting in sign bits.  
    /// Shifting by a value greater than the bit width of the type sets the result to zero if the
    /// sign bit is not set, and to `-1` if the sign bit is set.
    #[inline(always)]
    pub fn shr_dyn_i64x8(self, a: i64x8, amount: u64x8) -> i64x8 {
        cast(self.avx512f._mm512_srav_epi64(cast(a), cast(amount)))
    }

    //-------------------------------------------------------------------------------
    // arithmetic
    //-------------------------------------------------------------------------------

    /// Adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn add_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_add_ps(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn add_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_add_pd(cast(a), cast(b)))
    }

    /// Subtracts the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn sub_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_sub_ps(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn sub_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_sub_pd(cast(a), cast(b)))
    }

    /// Multiplies the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn mul_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_mul_ps(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn mul_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_mul_pd(cast(a), cast(b)))
    }

    /// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
    /// `c`.
    #[inline(always)]
    pub fn mul_add_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_fmadd_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
    /// `c`.
    #[inline(always)]
    pub fn mul_add_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_fmadd_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the results.
    #[inline(always)]
    pub fn mul_sub_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_fmsub_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the results.
    #[inline(always)]
    pub fn mul_sub_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_fmsub_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
    /// each lane of `c`.
    #[inline(always)]
    pub fn negate_mul_add_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_fnmadd_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
    /// each lane of `c`.
    #[inline(always)]
    pub fn negate_mul_add_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_fnmadd_pd(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the negation of the results.
    #[inline(always)]
    pub fn negate_mul_sub_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_fnmsub_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
    /// the negation of the results.
    #[inline(always)]
    pub fn negate_mul_sub_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_fnmsub_pd(cast(a), cast(b), cast(c)))
    }

    /// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_addsub_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_fmaddsub_ps(
            cast(a),
            cast(b),
            cast(self.sub_f32x16(self.splat_f32x16(-0.0), c)),
        ))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_addsub_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_fmaddsub_pd(
            cast(a),
            cast(b),
            cast(self.sub_f64x8(self.splat_f64x8(-0.0), c)),
        ))
    }

    /// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_subadd_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_fmaddsub_ps(cast(a), cast(b), cast(c)))
    }
    /// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
    /// to/from the results.
    #[inline(always)]
    pub fn mul_subadd_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_fmaddsub_pd(cast(a), cast(b), cast(c)))
    }

    /// Divides the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn div_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_div_ps(cast(a), cast(b)))
    }
    /// Divides the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn div_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_div_pd(cast(a), cast(b)))
    }

    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512bw._mm512_add_epi8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512bw._mm512_add_epi8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_add_epi16(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512bw._mm512_add_epi16(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_add_epi32(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_add_epi32(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_add_epi64(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_add_epi64(cast(a), cast(b)))
    }

    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512bw._mm512_adds_epu8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512bw._mm512_adds_epi8(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_adds_epu16(cast(a), cast(b)))
    }
    /// Adds the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_add_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512bw._mm512_adds_epi16(cast(a), cast(b)))
    }

    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512bw._mm512_sub_epi8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512bw._mm512_sub_epi8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_sub_epi16(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512bw._mm512_sub_epi16(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_sub_epi32(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_sub_epi32(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_sub_epi64(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_sub_epi64(cast(a), cast(b)))
    }

    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512bw._mm512_subs_epu8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512bw._mm512_subs_epi8(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_subs_epu16(cast(a), cast(b)))
    }
    /// Subtracts the elements of each lane of `a` and `b`, with saturation.
    #[inline(always)]
    pub fn saturating_sub_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512bw._mm512_subs_epi16(cast(a), cast(b)))
    }

    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_mullo_epi16(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512bw._mm512_mullo_epi16(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_mullo_epi32(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_mullo_epi32(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_mullox_epi64(cast(a), cast(b)))
    }
    /// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_mul_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_mullox_epi64(cast(a), cast(b)))
    }

    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_u16x32(self, a: u16x32, b: u16x32) -> (u16x32, u16x32) {
        (
            cast(self.avx512bw._mm512_mullo_epi16(cast(a), cast(b))),
            cast(self.avx512bw._mm512_mulhi_epu16(cast(a), cast(b))),
        )
    }
    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_i16x32(self, a: i16x32, b: i16x32) -> (i16x32, i16x32) {
        (
            cast(self.avx512bw._mm512_mullo_epi16(cast(a), cast(b))),
            cast(self.avx512bw._mm512_mulhi_epi16(cast(a), cast(b))),
        )
    }
    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_u32x16(self, a: u32x16, b: u32x16) -> (u32x16, u32x16) {
        let a = cast(a);
        let b = cast(b);
        let avx512f = self.avx512f;

        // a0b0_lo a0b0_hi a2b2_lo a2b2_hi
        let ab_evens = avx512f._mm512_mul_epu32(a, b);
        // a1b1_lo a1b1_hi a3b3_lo a3b3_hi
        let ab_odds = avx512f._mm512_mul_epu32(
            avx512f._mm512_srli_epi64::<32>(a),
            avx512f._mm512_srli_epi64::<32>(b),
        );

        let ab_lo = self.avx512f._mm512_mask_blend_epi32(
            0b1010101010101010,
            // a0b0_lo xxxxxxx a2b2_lo xxxxxxx
            cast(ab_evens),
            // xxxxxxx a1b1_lo xxxxxxx a3b3_lo
            cast(avx512f._mm512_slli_epi64::<32>(ab_odds)),
        );
        let ab_hi = self.avx512f._mm512_mask_blend_epi32(
            0b1010101010101010,
            // a0b0_hi xxxxxxx a2b2_hi xxxxxxx
            cast(avx512f._mm512_srli_epi64::<32>(ab_evens)),
            // xxxxxxx a1b1_hi xxxxxxx a3b3_hi
            cast(ab_odds),
        );

        (cast(ab_lo), cast(ab_hi))
    }
    /// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
    /// high bits of the result.
    #[inline(always)]
    pub fn widening_mul_i32x16(self, a: i32x16, b: i32x16) -> (i32x16, i32x16) {
        let a = cast(a);
        let b = cast(b);
        let avx512f = self.avx512f;

        // a0b0_lo a0b0_hi a2b2_lo a2b2_hi
        let ab_evens = self.avx512f._mm512_mul_epi32(a, b);
        // a1b1_lo a1b1_hi a3b3_lo a3b3_hi
        let ab_odds = self.avx512f._mm512_mul_epi32(
            avx512f._mm512_srli_epi64::<32>(a),
            avx512f._mm512_srli_epi64::<32>(b),
        );

        let ab_lo = self.avx512f._mm512_mask_blend_epi32(
            0b1010101010101010,
            // a0b0_lo xxxxxxx a2b2_lo xxxxxxx
            cast(ab_evens),
            // xxxxxxx a1b1_lo xxxxxxx a3b3_lo
            cast(avx512f._mm512_slli_epi64::<32>(ab_odds)),
        );
        let ab_hi = self.avx512f._mm512_mask_blend_epi32(
            0b1010101010101010,
            // a0b0_hi xxxxxxx a2b2_hi xxxxxxx
            cast(avx512f._mm512_srli_epi64::<32>(ab_evens)),
            // xxxxxxx a1b1_hi xxxxxxx a3b3_hi
            cast(ab_odds),
        );

        (cast(ab_lo), cast(ab_hi))
    }

    //-------------------------------------------------------------------------------
    // math
    //-------------------------------------------------------------------------------

    /// Averages the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn average_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512bw._mm512_avg_epu8(cast(a), cast(b)))
    }
    /// Averages the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn average_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_avg_epu16(cast(a), cast(b)))
    }

    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512bw._mm512_min_epu8(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512bw._mm512_min_epi8(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_min_epu16(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512bw._mm512_min_epi16(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_min_epu32(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_min_epi32(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_min_ps(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        cast(self.fvl()._mm_min_epu64(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
        cast(self.fvl()._mm256_min_epu64(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_min_epu64(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
        cast(self.fvl()._mm256_min_epi64(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_min_epi64(cast(a), cast(b)))
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_min_pd(cast(a), cast(b)))
    }

    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
        cast(self.avx512bw._mm512_max_epu8(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
        cast(self.avx512bw._mm512_max_epi8(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
        cast(self.avx512bw._mm512_max_epu16(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
        cast(self.avx512bw._mm512_max_epi16(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
        cast(self.avx512f._mm512_max_epu32(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
        cast(self.avx512f._mm512_max_epi32(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_max_ps(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        cast(self.fvl()._mm_max_epu64(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
        cast(self.fvl()._mm256_max_epu64(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
        cast(self.avx512f._mm512_max_epu64(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        cast(self.fvl()._mm_max_epi64(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
        cast(self.fvl()._mm256_max_epi64(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
        cast(self.avx512f._mm512_max_epi64(cast(a), cast(b)))
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_max_pd(cast(a), cast(b)))
    }

    /// Computes the absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn abs_f32x16(self, a: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_abs_ps(cast(a)))
    }
    /// Computes the absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn abs_f64x8(self, a: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_abs_pd(cast(a)))
    }

    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i8x64(self, a: i8x64) -> u8x64 {
        cast(self.avx512bw._mm512_abs_epi8(cast(a)))
    }
    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i16x32(self, a: i16x32) -> u16x32 {
        cast(self.avx512bw._mm512_abs_epi16(cast(a)))
    }
    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i32x16(self, a: i32x16) -> u32x16 {
        cast(self.avx512f._mm512_abs_epi32(cast(a)))
    }
    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i64x4(self, a: i64x4) -> u64x4 {
        cast(self.fvl()._mm256_abs_epi64(cast(a)))
    }
    /// Computes the unsigned absolute value of the elements of each lane of `a`.
    #[inline(always)]
    pub fn unsigned_abs_i64x8(self, a: i64x8) -> u64x8 {
        cast(self.avx512f._mm512_abs_epi64(cast(a)))
    }

    /// Computes the square roots of the elements of each lane of `a`.
    #[inline(always)]
    pub fn sqrt_f32x16(self, a: f32x16) -> f32x16 {
        cast(self.avx512f._mm512_sqrt_ps(cast(a)))
    }
    /// Computes the square roots of the elements of each lane of `a`.
    #[inline(always)]
    pub fn sqrt_f64x8(self, a: f64x8) -> f64x8 {
        cast(self.avx512f._mm512_sqrt_pd(cast(a)))
    }

    /// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
    #[inline(always)]
    pub fn floor_f32x16(self, a: f32x16) -> f32x16 {
        cast(
            self.avx512f
                ._mm512_roundscale_ps::<_MM_FROUND_TO_NEG_INF>(cast(a)),
        )
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
    #[inline(always)]
    pub fn floor_f64x8(self, a: f64x8) -> f64x8 {
        cast(
            self.avx512f
                ._mm512_roundscale_pd::<_MM_FROUND_TO_NEG_INF>(cast(a)),
        )
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards positive infinity.
    #[inline(always)]
    pub fn ceil_f32x16(self, a: f32x16) -> f32x16 {
        cast(
            self.avx512f
                ._mm512_roundscale_ps::<_MM_FROUND_TO_POS_INF>(cast(a)),
        )
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards positive infinity.
    #[inline(always)]
    pub fn ceil_f64x8(self, a: f64x8) -> f64x8 {
        cast(
            self.avx512f
                ._mm512_roundscale_pd::<_MM_FROUND_TO_POS_INF>(cast(a)),
        )
    }
    /// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
    /// close, the even value is returned.
    #[inline(always)]
    pub fn round_f32x16(self, a: f32x16) -> f32x16 {
        cast(
            self.avx512f
                ._mm512_roundscale_pd::<_MM_FROUND_TO_NEAREST_INT>(cast(a)),
        )
    }
    /// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
    /// close, the even value is returned.
    #[inline(always)]
    pub fn round_f64x8(self, a: f64x8) -> f64x8 {
        cast(
            self.avx512f
                ._mm512_roundscale_pd::<_MM_FROUND_TO_NEAREST_INT>(cast(a)),
        )
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards zero.
    #[inline(always)]
    pub fn truncate_f32x16(self, a: f32x16) -> f32x16 {
        cast(
            self.avx512f
                ._mm512_roundscale_pd::<_MM_FROUND_TO_ZERO>(cast(a)),
        )
    }
    /// Rounds the elements of each lane of `a` to the nearest integer towards zero.
    #[inline(always)]
    pub fn truncate_f64x8(self, a: f64x8) -> f64x8 {
        cast(
            self.avx512f
                ._mm512_roundscale_pd::<_MM_FROUND_TO_ZERO>(cast(a)),
        )
    }

    /// See `_mm512_madd_epi16`
    #[inline(always)]
    pub fn multiply_wrapping_add_adjacent_i16x32(self, a: i16x32, b: i16x32) -> i32x16 {
        cast(self.avx512bw._mm512_madd_epi16(cast(a), cast(b)))
    }
    /// See `_mm512_maddubs_epi16`
    #[inline(always)]
    pub fn multiply_saturating_add_adjacent_i8x64(self, a: i8x64, b: i8x64) -> i16x32 {
        cast(self.avx512bw._mm512_maddubs_epi16(cast(a), cast(b)))
    }

    /// See `_mm512_packs_epi16`
    #[inline(always)]
    pub fn pack_with_signed_saturation_i16x32(self, a: i16x32, b: i16x32) -> i8x64 {
        cast(self.avx512bw._mm512_packs_epi16(cast(a), cast(b)))
    }
    /// See `_mm512_packs_epi32`
    #[inline(always)]
    pub fn pack_with_signed_saturation_i32x16(self, a: i32x16, b: i32x16) -> i16x32 {
        cast(self.avx512bw._mm512_packs_epi32(cast(a), cast(b)))
    }

    /// See `_mm512_packus_epi16`
    #[inline(always)]
    pub fn pack_with_unsigned_saturation_i16x32(self, a: i16x32, b: i16x32) -> u8x64 {
        cast(self.avx512bw._mm512_packus_epi16(cast(a), cast(b)))
    }
    /// See `_mm512_packus_epi32`
    #[inline(always)]
    pub fn pack_with_unsigned_saturation_i32x16(self, a: i32x16, b: i32x16) -> u16x32 {
        cast(self.avx512bw._mm512_packus_epi32(cast(a), cast(b)))
    }

    /// See `_mm512_sad_epu8`
    #[inline(always)]
    pub fn sum_of_absolute_differences_u8x64(self, a: u8x64, b: u8x64) -> u64x8 {
        cast(self.avx512bw._mm512_sad_epu8(cast(a), cast(b)))
    }

    //-------------------------------------------------------------------------------
    // conversions
    //-------------------------------------------------------------------------------

    /// Converts a `u8x64` to `i8x64`, elementwise.
    #[inline(always)]
    pub fn convert_u8x64_to_i8x64(self, a: u8x64) -> i8x64 {
        cast(a)
    }
    /// Converts a `u8x32` to `u16x32`, elementwise.
    #[inline(always)]
    pub fn convert_u8x32_to_u16x32(self, a: u8x32) -> u16x32 {
        cast(self.avx512bw._mm512_cvtepu8_epi16(cast(a)))
    }
    /// Converts a `u8x32` to `i16x32`, elementwise.
    #[inline(always)]
    pub fn convert_u8x32_to_i16x32(self, a: u8x32) -> i16x32 {
        cast(self.avx512bw._mm512_cvtepu8_epi16(cast(a)))
    }
    /// Converts a `u8x16` to `u32x16`, elementwise.
    #[inline(always)]
    pub fn convert_u8x16_to_u32x16(self, a: u8x16) -> u32x16 {
        cast(self.avx512f._mm512_cvtepu8_epi32(cast(a)))
    }
    /// Converts a `u8x16` to `i32x16`, elementwise.
    #[inline(always)]
    pub fn convert_u8x16_to_i32x16(self, a: u8x16) -> i32x16 {
        cast(self.avx512f._mm512_cvtepu8_epi32(cast(a)))
    }
    /// Converts a `u8x16` to `u64x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_u64x8(self, a: u8x16) -> u64x8 {
        cast(self.avx512f._mm512_cvtepu8_epi64(cast(a)))
    }
    /// Converts a `u8x16` to `i64x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_u8x16_to_i64x8(self, a: u8x16) -> i64x8 {
        cast(self.avx512f._mm512_cvtepu8_epi64(cast(a)))
    }

    /// Converts a `i8x64` to `u8x64`, elementwise.
    #[inline(always)]
    pub fn convert_i8x64_to_u8x64(self, a: i8x64) -> u8x64 {
        cast(a)
    }
    /// Converts a `i8x32` to `u16x32`, elementwise.
    #[inline(always)]
    pub fn convert_i8x32_to_u16x32(self, a: i8x32) -> u16x32 {
        cast(self.avx512bw._mm512_cvtepi8_epi16(cast(a)))
    }
    /// Converts a `i8x32` to `i16x32`, elementwise.
    #[inline(always)]
    pub fn convert_i8x32_to_i16x32(self, a: i8x32) -> i16x32 {
        cast(self.avx512bw._mm512_cvtepi8_epi16(cast(a)))
    }
    /// Converts a `i8x16` to `u32x16`, elementwise.
    #[inline(always)]
    pub fn convert_i8x16_to_u32x16(self, a: i8x16) -> u32x16 {
        cast(self.avx512f._mm512_cvtepi8_epi32(cast(a)))
    }
    /// Converts a `i8x16` to `i32x16`, elementwise.
    #[inline(always)]
    pub fn convert_i8x16_to_i32x16(self, a: i8x16) -> i32x16 {
        cast(self.avx512f._mm512_cvtepi8_epi32(cast(a)))
    }
    /// Converts a `i8x16` to `u64x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_u64x8(self, a: i8x16) -> u64x8 {
        cast(self.avx512f._mm512_cvtepi8_epi64(cast(a)))
    }
    /// Converts a `i8x16` to `i64x8`, elementwise, while truncating the extra elements.
    #[inline(always)]
    pub fn convert_i8x16_to_i64x8(self, a: i8x16) -> i64x8 {
        cast(self.avx512f._mm512_cvtepi8_epi64(cast(a)))
    }

    /// Converts a `u16x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u16x8_to_u8x16(self, a: u16x8) -> u8x16 {
        cast(self.bwvl()._mm_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `u16x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u16x8_to_i8x16(self, a: u16x8) -> i8x16 {
        cast(self.bwvl()._mm_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `u16x16` to `u8x16`, elementwise.
    #[inline(always)]
    pub fn convert_u16x16_to_u8x16(self, a: u16x16) -> u8x16 {
        cast(self.bwvl()._mm256_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `u16x16` to `i8x16`, elementwise.
    #[inline(always)]
    pub fn convert_u16x16_to_i8x16(self, a: u16x16) -> i8x16 {
        cast(self.bwvl()._mm256_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `u16x32` to `u8x32`, elementwise.
    #[inline(always)]
    pub fn convert_u16x32_to_u8x32(self, a: u16x32) -> u8x32 {
        cast(self.avx512bw._mm512_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `u16x32` to `i8x32`, elementwise.
    #[inline(always)]
    pub fn convert_u16x32_to_i8x32(self, a: u16x32) -> i8x32 {
        cast(self.avx512bw._mm512_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `u16x32` to `i16x32`, elementwise.
    #[inline(always)]
    pub fn convert_u16x32_to_i16x32(self, a: u16x32) -> i16x32 {
        cast(a)
    }
    /// Converts a `u16x16` to `u32x16`, elementwise.
    #[inline(always)]
    pub fn convert_u16x16_to_u32x16(self, a: u16x16) -> u32x16 {
        cast(self.avx512f._mm512_cvtepu16_epi32(cast(a)))
    }
    /// Converts a `u16x16` to `i32x16`, elementwise.
    #[inline(always)]
    pub fn convert_u16x16_to_i32x16(self, a: u16x16) -> i32x16 {
        cast(self.avx512f._mm512_cvtepu16_epi32(cast(a)))
    }
    /// Converts a `u16x8` to `u64x8`, elementwise.
    #[inline(always)]
    pub fn convert_u16x8_to_u64x8(self, a: u16x8) -> u64x8 {
        cast(self.avx512f._mm512_cvtepu16_epi64(cast(a)))
    }
    /// Converts a `u16x8` to `i64x8`, elementwise.
    #[inline(always)]
    pub fn convert_u16x8_to_i64x8(self, a: u16x8) -> i64x8 {
        cast(self.avx512f._mm512_cvtepu16_epi64(cast(a)))
    }

    /// Converts a `i16x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i16x8_to_u8x16(self, a: i16x8) -> u8x16 {
        cast(self.bwvl()._mm_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `i16x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i16x8_to_i8x16(self, a: i16x8) -> i8x16 {
        cast(self.bwvl()._mm_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `i16x16` to `u8x16`, elementwise.
    #[inline(always)]
    pub fn convert_i16x16_to_u8x16(self, a: i16x16) -> u8x16 {
        cast(self.bwvl()._mm256_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `i16x16` to `i8x16`, elementwise.
    #[inline(always)]
    pub fn convert_i16x16_to_i8x16(self, a: i16x16) -> i8x16 {
        cast(self.bwvl()._mm256_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `i16x32` to `u8x32`, elementwise.
    #[inline(always)]
    pub fn convert_i16x32_to_u8x32(self, a: i16x32) -> u8x32 {
        cast(self.avx512bw._mm512_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `i16x32` to `i8x32`, elementwise.
    #[inline(always)]
    pub fn convert_i16x32_to_i8x32(self, a: i16x32) -> i8x32 {
        cast(self.avx512bw._mm512_cvtepi16_epi8(cast(a)))
    }
    /// Converts a `i16x32` to `u16x32`, elementwise.
    #[inline(always)]
    pub fn convert_i16x32_to_u16x32(self, a: i16x32) -> u16x32 {
        cast(a)
    }
    /// Converts a `i16x16` to `u32x16`, elementwise.
    #[inline(always)]
    pub fn convert_i16x16_to_u32x16(self, a: i16x16) -> u32x16 {
        cast(self.avx512f._mm512_cvtepi16_epi32(cast(a)))
    }
    /// Converts a `i16x16` to `i32x16`, elementwise.
    #[inline(always)]
    pub fn convert_i16x16_to_i32x16(self, a: i16x16) -> i32x16 {
        cast(self.avx512f._mm512_cvtepi16_epi32(cast(a)))
    }
    /// Converts a `i16x8` to `u64x8`, elementwise.
    #[inline(always)]
    pub fn convert_i16x8_to_u64x8(self, a: i16x8) -> u64x8 {
        cast(self.avx512f._mm512_cvtepi16_epi64(cast(a)))
    }
    /// Converts a `i16x8` to `i64x8`, elementwise.
    #[inline(always)]
    pub fn convert_i16x8_to_i64x8(self, a: i16x8) -> i64x8 {
        cast(self.avx512f._mm512_cvtepi16_epi64(cast(a)))
    }

    /// Converts a `u32x4` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u32x4_to_u8x16(self, a: u32x4) -> u8x16 {
        cast(self.fvl()._mm_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `u32x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u32x8_to_u8x16(self, a: u32x8) -> u8x16 {
        cast(self.fvl()._mm256_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `u32x16` to `u8x16`, elementwise.
    #[inline(always)]
    pub fn convert_u32x16_to_u8x16(self, a: u32x16) -> u8x16 {
        cast(self.avx512f._mm512_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `u32x4` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u32x4_to_i8x16(self, a: u32x4) -> i8x16 {
        cast(self.fvl()._mm_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `u32x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u32x8_to_i8x16(self, a: u32x8) -> i8x16 {
        cast(self.fvl()._mm256_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `u32x16` to `i8x16`, elementwise.
    #[inline(always)]
    pub fn convert_u32x16_to_i8x16(self, a: u32x16) -> i8x16 {
        cast(self.avx512f._mm512_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `u32x4` to `u16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u32x4_to_u16x8(self, a: u32x4) -> u16x8 {
        cast(self.fvl()._mm_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `u32x8` to `u16x8`, elementwise.
    #[inline(always)]
    pub fn convert_u32x8_to_u16x8(self, a: u32x8) -> u16x8 {
        cast(self.fvl()._mm256_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `u32x16` to `u16x16`, elementwise.
    #[inline(always)]
    pub fn convert_u32x16_to_u16x16(self, a: u32x16) -> u16x16 {
        cast(self.avx512f._mm512_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `u32x4` to `i16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u32x4_to_i16x8(self, a: u32x4) -> i16x8 {
        cast(self.fvl()._mm_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `u32x8` to `i16x8`, elementwise.
    #[inline(always)]
    pub fn convert_u32x8_to_i16x8(self, a: u32x8) -> i16x8 {
        cast(self.fvl()._mm256_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `u32x16` to `i16x16`, elementwise.
    #[inline(always)]
    pub fn convert_u32x16_to_i16x16(self, a: u32x16) -> i16x16 {
        cast(self.avx512f._mm512_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `u32x16` to `i32x16`, elementwise.
    #[inline(always)]
    pub fn convert_u32x16_to_i32x16(self, a: u32x16) -> i32x16 {
        cast(a)
    }
    /// Converts a `u32x8` to `u64x8`, elementwise.
    #[inline(always)]
    pub fn convert_u32x8_to_u64x8(self, a: u32x8) -> u64x8 {
        cast(self.avx512f._mm512_cvtepu32_epi64(cast(a)))
    }
    /// Converts a `u32x8` to `i64x8`, elementwise.
    #[inline(always)]
    pub fn convert_u32x8_to_i64x8(self, a: u32x8) -> i64x8 {
        cast(self.avx512f._mm512_cvtepu32_epi64(cast(a)))
    }

    /// Converts a `i32x4` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i32x4_to_u8x16(self, a: i32x4) -> u8x16 {
        cast(self.fvl()._mm_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `i32x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i32x8_to_u8x16(self, a: i32x8) -> u8x16 {
        cast(self.fvl()._mm256_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `i32x16` to `u8x16`, elementwise.
    #[inline(always)]
    pub fn convert_i32x16_to_u8x16(self, a: i32x16) -> u8x16 {
        cast(self.avx512f._mm512_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `i32x4` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i32x4_to_i8x16(self, a: i32x4) -> i8x16 {
        cast(self.fvl()._mm_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `i32x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i32x8_to_i8x16(self, a: i32x8) -> i8x16 {
        cast(self.fvl()._mm256_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `i32x16` to `i8x16`, elementwise.
    #[inline(always)]
    pub fn convert_i32x16_to_i8x16(self, a: i32x16) -> i8x16 {
        cast(self.avx512f._mm512_cvtepi32_epi8(cast(a)))
    }
    /// Converts a `i32x4` to `u16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i32x4_to_u16x8(self, a: i32x4) -> u16x8 {
        cast(self.fvl()._mm_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `i32x8` to `u16x8`, elementwise.
    #[inline(always)]
    pub fn convert_i32x8_to_u16x8(self, a: i32x8) -> u16x8 {
        cast(self.fvl()._mm256_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `i32x16` to `u16x16`, elementwise.
    #[inline(always)]
    pub fn convert_i32x16_to_u16x16(self, a: i32x16) -> u16x16 {
        cast(self.avx512f._mm512_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `i32x4` to `i16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i32x4_to_i16x8(self, a: i32x4) -> i16x8 {
        cast(self.fvl()._mm_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `i32x8` to `i16x8`, elementwise.
    #[inline(always)]
    pub fn convert_i32x8_to_i16x8(self, a: i32x8) -> i16x8 {
        cast(self.fvl()._mm256_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `i32x16` to `i16x16`, elementwise.
    #[inline(always)]
    pub fn convert_i32x16_to_i16x16(self, a: i32x16) -> i16x16 {
        cast(self.avx512f._mm512_cvtepi32_epi16(cast(a)))
    }
    /// Converts a `i32x16` to `u32x16`, elementwise.
    #[inline(always)]
    pub fn convert_i32x16_to_u32x16(self, a: i32x16) -> u32x16 {
        cast(a)
    }
    /// Converts a `i32x16` to `f32x16`, elementwise.
    #[inline(always)]
    pub fn convert_i32x16_to_f32x16(self, a: i32x16) -> f32x16 {
        cast(self.avx512f._mm512_cvtepi32_ps(cast(a)))
    }
    /// Converts a `i32x8` to `u64x8`, elementwise.
    #[inline(always)]
    pub fn convert_i32x8_to_u64x8(self, a: i32x8) -> u64x8 {
        cast(self.avx512f._mm512_cvtepi32_epi64(cast(a)))
    }
    /// Converts a `i32x8` to `i64x8`, elementwise.
    #[inline(always)]
    pub fn convert_i32x8_to_i64x8(self, a: i32x8) -> i64x8 {
        cast(self.avx512f._mm512_cvtepi32_epi64(cast(a)))
    }
    /// Converts a `i32x8` to `f64x8`, elementwise.
    #[inline(always)]
    pub fn convert_i32x8_to_f64x8(self, a: i32x8) -> f64x8 {
        cast(self.avx512f._mm512_cvtepi32_pd(cast(a)))
    }

    /// Converts a `f32x4` to `u32x4`, elementwise.
    #[inline(always)]
    pub fn convert_f32x4_to_u32x4(self, a: f32x4) -> u32x4 {
        cast(self.fvl()._mm_cvttps_epu32(cast(a)))
    }
    /// Converts a `f32x8` to `u32x8`, elementwise.
    #[inline(always)]
    pub fn convert_f32x8_to_u32x8(self, a: f32x8) -> u32x8 {
        cast(self.fvl()._mm256_cvttps_epu32(cast(a)))
    }
    /// Converts a `f32x16` to `u32x16`, elementwise.
    #[inline(always)]
    pub fn convert_f32x16_to_u32x16(self, a: f32x16) -> u32x16 {
        cast(self.avx512f._mm512_cvttps_epu32(cast(a)))
    }
    /// Converts a `f32x16` to `i32x16`, elementwise.
    #[inline(always)]
    pub fn convert_f32x16_to_i32x16(self, a: f32x16) -> i32x16 {
        cast(self.avx512f._mm512_cvttps_epi32(cast(a)))
    }
    /// Converts a `f32x8` to `f64x8`, elementwise.
    #[inline(always)]
    pub fn convert_f32x8_to_f64x8(self, a: f32x8) -> f64x8 {
        cast(self.avx512f._mm512_cvtps_pd(cast(a)))
    }

    /// Converts a `u64x2` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x2_to_u8x16(self, a: u64x2) -> u8x16 {
        cast(self.fvl()._mm_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `u64x4` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x4_to_u8x16(self, a: u64x4) -> u8x16 {
        cast(self.fvl()._mm256_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `u64x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x8_to_u8x16(self, a: u64x8) -> u8x16 {
        cast(self.avx512f._mm512_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `u64x2` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x2_to_i8x16(self, a: u64x2) -> i8x16 {
        cast(self.fvl()._mm_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `u64x4` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x4_to_i8x16(self, a: u64x4) -> i8x16 {
        cast(self.fvl()._mm256_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `u64x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x8_to_i8x16(self, a: u64x8) -> i8x16 {
        cast(self.avx512f._mm512_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `u64x2` to `u16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x2_to_u16x8(self, a: u64x2) -> u16x8 {
        cast(self.fvl()._mm_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `u64x4` to `u16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x4_to_u16x8(self, a: u64x4) -> u16x8 {
        cast(self.fvl()._mm256_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `u64x8` to `u16x8`, elementwise.
    #[inline(always)]
    pub fn convert_u64x8_to_u16x8(self, a: u64x8) -> u16x8 {
        cast(self.avx512f._mm512_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `u64x2` to `i16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x2_to_i16x8(self, a: u64x2) -> i16x8 {
        cast(self.fvl()._mm_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `u64x4` to `i16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x4_to_i16x8(self, a: u64x4) -> i16x8 {
        cast(self.fvl()._mm256_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `u64x8` to `i16x8`, elementwise.
    #[inline(always)]
    pub fn convert_u64x8_to_i16x8(self, a: u64x8) -> i16x8 {
        cast(self.avx512f._mm512_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `u64x2` to `u32x4`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x2_to_u32x4(self, a: u64x2) -> u32x4 {
        cast(self.fvl()._mm_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `u64x4` to `u32x4`, elementwise.
    #[inline(always)]
    pub fn convert_u64x4_to_u32x4(self, a: u64x4) -> u32x4 {
        cast(self.fvl()._mm256_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `u64x8` to `u32x8`, elementwise.
    #[inline(always)]
    pub fn convert_u64x8_to_u32x8(self, a: u64x8) -> u32x8 {
        cast(self.avx512f._mm512_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `u64x2` to `i32x4`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_u64x2_to_i32x4(self, a: u64x2) -> i32x4 {
        cast(self.fvl()._mm_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `u64x4` to `i32x4`, elementwise.
    #[inline(always)]
    pub fn convert_u64x4_to_i32x4(self, a: u64x4) -> i32x4 {
        cast(self.fvl()._mm256_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `u64x8` to `i32x8`, elementwise.
    #[inline(always)]
    pub fn convert_u64x8_to_i32x8(self, a: u64x8) -> i32x8 {
        cast(self.avx512f._mm512_cvtepi64_epi32(cast(a)))
    }

    /// Converts a `i64x2` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x2_to_u8x16(self, a: i64x2) -> u8x16 {
        cast(self.fvl()._mm_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `i64x4` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x4_to_u8x16(self, a: i64x4) -> u8x16 {
        cast(self.fvl()._mm256_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `i64x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x8_to_u8x16(self, a: i64x8) -> u8x16 {
        cast(self.avx512f._mm512_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `i64x2` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x2_to_i8x16(self, a: i64x2) -> i8x16 {
        cast(self.fvl()._mm_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `i64x4` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x4_to_i8x16(self, a: i64x4) -> i8x16 {
        cast(self.fvl()._mm256_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `i64x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x8_to_i8x16(self, a: i64x8) -> i8x16 {
        cast(self.avx512f._mm512_cvtepi64_epi8(cast(a)))
    }
    /// Converts a `i64x2` to `u16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x2_to_u16x8(self, a: i64x2) -> u16x8 {
        cast(self.fvl()._mm_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `i64x4` to `u16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x4_to_u16x8(self, a: i64x4) -> u16x8 {
        cast(self.fvl()._mm256_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `i64x8` to `u16x8`, elementwise.
    #[inline(always)]
    pub fn convert_i64x8_to_u16x8(self, a: i64x8) -> u16x8 {
        cast(self.avx512f._mm512_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `i64x2` to `i16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x2_to_i16x8(self, a: i64x2) -> i16x8 {
        cast(self.fvl()._mm_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `i64x4` to `i16x8`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x4_to_i16x8(self, a: i64x4) -> i16x8 {
        cast(self.fvl()._mm256_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `i64x8` to `i16x8`, elementwise.
    #[inline(always)]
    pub fn convert_i64x8_to_i16x8(self, a: i64x8) -> i16x8 {
        cast(self.avx512f._mm512_cvtepi64_epi16(cast(a)))
    }
    /// Converts a `i64x2` to `u32x4`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x2_to_u32x4(self, a: i64x2) -> u32x4 {
        cast(self.fvl()._mm_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `i64x4` to `u32x4`, elementwise.
    #[inline(always)]
    pub fn convert_i64x4_to_u32x4(self, a: i64x4) -> u32x4 {
        cast(self.fvl()._mm256_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `i64x8` to `u32x8`, elementwise.
    #[inline(always)]
    pub fn convert_i64x8_to_u32x8(self, a: i64x8) -> u32x8 {
        cast(self.avx512f._mm512_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `i64x2` to `i32x4`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_i64x2_to_i32x4(self, a: i64x2) -> i32x4 {
        cast(self.fvl()._mm_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `i64x4` to `i32x4`, elementwise.
    #[inline(always)]
    pub fn convert_i64x4_to_i32x4(self, a: i64x4) -> i32x4 {
        cast(self.fvl()._mm256_cvtepi64_epi32(cast(a)))
    }
    /// Converts a `i64x8` to `i32x8`, elementwise.
    #[inline(always)]
    pub fn convert_i64x8_to_i32x8(self, a: i64x8) -> i32x8 {
        cast(self.avx512f._mm512_cvtepi64_epi32(cast(a)))
    }

    /// Converts a `f64x2` to `u32x4`, elementwise, filling the remaining elements with zeros.
    #[inline(always)]
    pub fn convert_f64x2_to_u32x4(self, a: f64x2) -> u32x4 {
        cast(self.fvl()._mm_cvttpd_epu32(cast(a)))
    }
    /// Converts a `f64x4` to `u32x4`, elementwise.
    #[inline(always)]
    pub fn convert_f64x4_to_u32x4(self, a: f64x4) -> u32x4 {
        cast(self.fvl()._mm256_cvttpd_epu32(cast(a)))
    }
    /// Converts a `f64x8` to `u32x8`, elementwise.
    #[inline(always)]
    pub fn convert_f64x8_to_u32x8(self, a: f64x8) -> u32x8 {
        cast(self.avx512f._mm512_cvttpd_epu32(cast(a)))
    }
    /// Converts a `f64x8` to `i32x8`, elementwise.
    #[inline(always)]
    pub fn convert_f64x8_to_i32x8(self, a: f64x8) -> i32x8 {
        cast(self.avx512f._mm512_cvttpd_epi32(cast(a)))
    }
    /// Converts a `f64x8` to `f32x8`, elementwise.
    #[inline(always)]
    pub fn convert_f64x8_to_f32x8(self, a: f64x8) -> f32x8 {
        cast(self.avx512f._mm512_cvtpd_ps(cast(a)))
    }

    /// Converts a `b16` to `u8x16`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result.
    #[inline(always)]
    pub fn convert_mask_b16_to_u8x16(self, a: b16) -> u8x16 {
        cast(self.bwvl()._mm_movm_epi8(a.0))
    }
    /// Converts a `b32` to `u8x32`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result.
    #[inline(always)]
    pub fn convert_mask_b32_to_u8x32(self, a: b32) -> u8x32 {
        cast(self.bwvl()._mm256_movm_epi8(a.0))
    }
    /// Converts a `b64` to `u8x64`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result.
    #[inline(always)]
    pub fn convert_mask_b64_to_u8x64(self, a: b64) -> u8x64 {
        cast(self.avx512bw._mm512_movm_epi8(a.0))
    }

    /// Converts a `b8` to `u16x8`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result.
    #[inline(always)]
    pub fn convert_mask_b8_to_u16x8(self, a: b8) -> u16x8 {
        cast(self.bwvl()._mm_movm_epi16(a.0))
    }
    /// Converts a `b16` to `u16x16`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result.
    #[inline(always)]
    pub fn convert_mask_b16_to_u16x16(self, a: b16) -> u16x16 {
        cast(self.bwvl()._mm256_movm_epi16(a.0))
    }
    /// Converts a `b32` to `u16x32`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result.
    #[inline(always)]
    pub fn convert_mask_b32_to_u16x32(self, a: b32) -> u16x32 {
        cast(self.avx512bw._mm512_movm_epi16(a.0))
    }

    /// Converts a `b8` to `u32x4`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result, and truncating the extra bits.
    #[inline(always)]
    pub fn convert_mask_b8_to_u32x4(self, a: b8) -> u32x4 {
        self.select_u32x4(a, self.splat_u32x4(!0), self.splat_u32x4(0))
    }
    /// Converts a `b8` to `u32x8`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result.
    #[inline(always)]
    pub fn convert_mask_b8_to_u32x8(self, a: b8) -> u32x8 {
        self.select_u32x8(a, self.splat_u32x8(!0), self.splat_u32x8(0))
    }
    /// Converts a `b16` to `u32x16`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result.
    #[inline(always)]
    pub fn convert_mask_b16_to_u32x16(self, a: b16) -> u32x16 {
        self.select_u32x16(a, self.splat_u32x16(!0), self.splat_u32x16(0))
    }

    /// Converts a `b8` to `u64x2`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result, and truncating the extra bits.
    #[inline(always)]
    pub fn convert_mask_b8_to_u64x2(self, a: b8) -> u64x2 {
        self.select_u64x2(a, self.splat_u64x2(!0), self.splat_u64x2(0))
    }
    /// Converts a `b8` to `u64x4`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result, and truncating the extra bits.
    #[inline(always)]
    pub fn convert_mask_b8_to_u64x4(self, a: b8) -> u64x4 {
        self.select_u64x4(a, self.splat_u64x4(!0), self.splat_u64x4(0))
    }
    /// Converts a `b8` to `u64x8`, broadcasting each bit in `a` to all the bits of the
    /// corresponding lane element in the result.
    #[inline(always)]
    pub fn convert_mask_b8_to_u64x8(self, a: b8) -> u64x8 {
        self.select_u64x8(a, self.splat_u64x8(!0), self.splat_u64x8(0))
    }

    //-------------------------------------------------------------------------------
    // comparisons
    //-------------------------------------------------------------------------------
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u8x64(self, a: u8x64, b: u8x64) -> b64 {
        cast(self.avx512bw._mm512_cmpeq_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i8x64(self, a: i8x64, b: i8x64) -> b64 {
        cast(self.avx512bw._mm512_cmpeq_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u16x32(self, a: u16x32, b: u16x32) -> b32 {
        cast(self.avx512bw._mm512_cmpeq_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i16x32(self, a: i16x32, b: i16x32) -> b32 {
        cast(self.avx512bw._mm512_cmpeq_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u32x16(self, a: u32x16, b: u32x16) -> b16 {
        cast(self.avx512f._mm512_cmpeq_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i32x16(self, a: i32x16, b: i32x16) -> b16 {
        cast(self.avx512f._mm512_cmpeq_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u64x8(self, a: u64x8, b: u64x8) -> b8 {
        cast(self.avx512f._mm512_cmpeq_epi64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i64x8(self, a: i64x8, b: i64x8) -> b8 {
        cast(self.avx512f._mm512_cmpeq_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u8x64(self, a: u8x64, b: u8x64) -> b64 {
        cast(self.avx512bw._mm512_cmpgt_epu8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i8x64(self, a: i8x64, b: i8x64) -> b64 {
        cast(self.avx512bw._mm512_cmpgt_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u16x32(self, a: u16x32, b: u16x32) -> b32 {
        cast(self.avx512bw._mm512_cmpgt_epu16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i16x32(self, a: i16x32, b: i16x32) -> b32 {
        cast(self.avx512bw._mm512_cmpgt_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u32x16(self, a: u32x16, b: u32x16) -> b16 {
        cast(self.avx512f._mm512_cmpgt_epu32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i32x16(self, a: i32x16, b: i32x16) -> b16 {
        cast(self.avx512f._mm512_cmpgt_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u64x8(self, a: u64x8, b: u64x8) -> b8 {
        cast(self.avx512f._mm512_cmpgt_epu64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i64x8(self, a: i64x8, b: i64x8) -> b8 {
        cast(self.avx512f._mm512_cmpgt_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u8x64(self, a: u8x64, b: u8x64) -> b64 {
        cast(self.avx512bw._mm512_cmpge_epu8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i8x64(self, a: i8x64, b: i8x64) -> b64 {
        cast(self.avx512bw._mm512_cmpge_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u16x32(self, a: u16x32, b: u16x32) -> b32 {
        cast(self.avx512bw._mm512_cmpge_epu16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i16x32(self, a: i16x32, b: i16x32) -> b32 {
        cast(self.avx512bw._mm512_cmpge_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u32x16(self, a: u32x16, b: u32x16) -> b16 {
        cast(self.avx512f._mm512_cmpge_epu32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i32x16(self, a: i32x16, b: i32x16) -> b16 {
        cast(self.avx512f._mm512_cmpge_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u64x8(self, a: u64x8, b: u64x8) -> b8 {
        cast(self.avx512f._mm512_cmpge_epu64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i64x8(self, a: i64x8, b: i64x8) -> b8 {
        cast(self.avx512f._mm512_cmpge_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u8x64(self, a: u8x64, b: u8x64) -> b64 {
        cast(self.avx512bw._mm512_cmplt_epu8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i8x64(self, a: i8x64, b: i8x64) -> b64 {
        cast(self.avx512bw._mm512_cmplt_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u16x32(self, a: u16x32, b: u16x32) -> b32 {
        cast(self.avx512bw._mm512_cmplt_epu16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i16x32(self, a: i16x32, b: i16x32) -> b32 {
        cast(self.avx512bw._mm512_cmplt_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u32x16(self, a: u32x16, b: u32x16) -> b16 {
        cast(self.avx512f._mm512_cmplt_epu32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i32x16(self, a: i32x16, b: i32x16) -> b16 {
        cast(self.avx512f._mm512_cmplt_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u64x8(self, a: u64x8, b: u64x8) -> b8 {
        cast(self.avx512f._mm512_cmplt_epu64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i64x8(self, a: i64x8, b: i64x8) -> b8 {
        cast(self.avx512f._mm512_cmplt_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u8x64(self, a: u8x64, b: u8x64) -> b64 {
        cast(self.avx512bw._mm512_cmple_epu8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i8x64(self, a: i8x64, b: i8x64) -> b64 {
        cast(self.avx512bw._mm512_cmple_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u16x32(self, a: u16x32, b: u16x32) -> b32 {
        cast(self.avx512bw._mm512_cmple_epu16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i16x32(self, a: i16x32, b: i16x32) -> b32 {
        cast(self.avx512bw._mm512_cmple_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u32x16(self, a: u32x16, b: u32x16) -> b16 {
        cast(self.avx512f._mm512_cmple_epu32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i32x16(self, a: i32x16, b: i32x16) -> b16 {
        cast(self.avx512f._mm512_cmple_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u64x8(self, a: u64x8, b: u64x8) -> b8 {
        cast(self.avx512f._mm512_cmple_epu64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i64x8(self, a: i64x8, b: i64x8) -> b8 {
        cast(self.avx512f._mm512_cmple_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_EQ_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_EQ_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_NEQ_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_NEQ_UQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_GT_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_GT_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_GE_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_GE_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_NGT_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_NGT_UQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_NGE_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_NGE_UQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_LT_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_LT_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_LE_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_LE_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_NLT_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_NLT_UQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f32x16(self, a: f32x16, b: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_NLE_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f64x8(self, a: f64x8, b: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_NLE_UQ>(cast(a), cast(b)),
        )
    }

    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f32x16(self, a: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_UNORD_Q>(cast(a), cast(a)),
        )
    }
    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f64x8(self, a: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_UNORD_Q>(cast(a), cast(a)),
        )
    }

    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f32x16(self, a: f32x16) -> b16 {
        cast(
            self.avx512f
                ._mm512_cmp_ps_mask::<_CMP_ORD_Q>(cast(a), cast(a)),
        )
    }
    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f64x8(self, a: f64x8) -> b8 {
        cast(
            self.avx512f
                ._mm512_cmp_pd_mask::<_CMP_ORD_Q>(cast(a), cast(a)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u8x32(self, a: u8x32, b: u8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpeq_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i8x32(self, a: i8x32, b: i8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpeq_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u16x16(self, a: u16x16, b: u16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpeq_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i16x16(self, a: i16x16, b: i16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpeq_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u32x8(self, a: u32x8, b: u32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpeq_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i32x8(self, a: i32x8, b: i32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpeq_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u64x4(self, a: u64x4, b: u64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpeq_epi64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i64x4(self, a: i64x4, b: i64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpeq_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u8x32(self, a: u8x32, b: u8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpgt_epu8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i8x32(self, a: i8x32, b: i8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpgt_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u16x16(self, a: u16x16, b: u16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpgt_epu16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i16x16(self, a: i16x16, b: i16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpgt_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u32x8(self, a: u32x8, b: u32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpgt_epu32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i32x8(self, a: i32x8, b: i32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpgt_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u64x4(self, a: u64x4, b: u64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpgt_epu64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i64x4(self, a: i64x4, b: i64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpgt_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u8x32(self, a: u8x32, b: u8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpge_epu8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i8x32(self, a: i8x32, b: i8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpge_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u16x16(self, a: u16x16, b: u16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpge_epu16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i16x16(self, a: i16x16, b: i16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpge_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u32x8(self, a: u32x8, b: u32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpge_epu32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i32x8(self, a: i32x8, b: i32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpge_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u64x4(self, a: u64x4, b: u64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpge_epu64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i64x4(self, a: i64x4, b: i64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmpge_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u8x32(self, a: u8x32, b: u8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmplt_epu8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i8x32(self, a: i8x32, b: i8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmplt_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u16x16(self, a: u16x16, b: u16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmplt_epu16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i16x16(self, a: i16x16, b: i16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmplt_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u32x8(self, a: u32x8, b: u32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmplt_epu32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i32x8(self, a: i32x8, b: i32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmplt_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u64x4(self, a: u64x4, b: u64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmplt_epu64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i64x4(self, a: i64x4, b: i64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmplt_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u8x32(self, a: u8x32, b: u8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmple_epu8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i8x32(self, a: i8x32, b: i8x32) -> b32 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmple_epi8_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u16x16(self, a: u16x16, b: u16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmple_epu16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i16x16(self, a: i16x16, b: i16x16) -> b16 {
        let simd = Avx512bw_Avx512vl {
            avx512bw: self.avx512bw,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmple_epi16_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u32x8(self, a: u32x8, b: u32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmple_epu32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i32x8(self, a: i32x8, b: i32x8) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmple_epi32_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u64x4(self, a: u64x4, b: u64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmple_epu64_mask(cast(a), cast(b)))
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i64x4(self, a: i64x4, b: i64x4) -> b8 {
        let simd = Avx512f_Avx512vl {
            avx512f: self.avx512f,
            avx512vl: self.avx512vl,
        };
        cast(simd._mm256_cmple_epi64_mask(cast(a), cast(b)))
    }

    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_EQ_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_EQ_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_NEQ_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_NEQ_UQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_GT_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_GT_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_GE_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_GE_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_NGT_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_NGT_UQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_NGE_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_NGE_UQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_LT_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_LT_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_LE_OQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_LE_OQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_NLT_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_NLT_UQ>(cast(a), cast(b)),
        )
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f32x8(self, a: f32x8, b: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_NLE_UQ>(cast(a), cast(b)),
        )
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f64x4(self, a: f64x4, b: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_NLE_UQ>(cast(a), cast(b)),
        )
    }

    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f32x8(self, a: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_UNORD_Q>(cast(a), cast(a)),
        )
    }
    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f64x4(self, a: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_UNORD_Q>(cast(a), cast(a)),
        )
    }

    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f32x8(self, a: f32x8) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_ps_mask::<_CMP_ORD_Q>(cast(a), cast(a)),
        )
    }
    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f64x4(self, a: f64x4) -> b8 {
        cast(
            Avx512f_Avx512vl {
                avx512f: self.avx512f,
                avx512vl: self.avx512vl,
            }
            ._mm256_cmp_pd_mask::<_CMP_ORD_Q>(cast(a), cast(a)),
        )
    }

    //-------------------------------------------------------------------------------
    // select
    //-------------------------------------------------------------------------------

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u8x16(self, mask: b16, if_true: u8x16, if_false: u8x16) -> u8x16 {
        cast(
            self.bwvl()
                ._mm_mask_blend_epi8(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u8x32(self, mask: b32, if_true: u8x32, if_false: u8x32) -> u8x32 {
        cast(
            self.bwvl()
                ._mm256_mask_blend_epi8(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u8x64(self, mask: b64, if_true: u8x64, if_false: u8x64) -> u8x64 {
        cast(
            self.avx512bw
                ._mm512_mask_blend_epi8(mask.0, cast(if_false), cast(if_true)),
        )
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i8x16(self, mask: b16, if_true: i8x16, if_false: i8x16) -> i8x16 {
        cast(
            self.bwvl()
                ._mm_mask_blend_epi8(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i8x32(self, mask: b32, if_true: i8x32, if_false: i8x32) -> i8x32 {
        cast(
            self.bwvl()
                ._mm256_mask_blend_epi8(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i8x64(self, mask: b64, if_true: i8x64, if_false: i8x64) -> i8x64 {
        cast(
            self.avx512bw
                ._mm512_mask_blend_epi8(mask.0, cast(if_false), cast(if_true)),
        )
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u16x8(self, mask: b8, if_true: u16x8, if_false: u16x8) -> u16x8 {
        cast(
            self.bwvl()
                ._mm_mask_blend_epi16(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u16x16(self, mask: b16, if_true: u16x16, if_false: u16x16) -> u16x16 {
        cast(
            self.bwvl()
                ._mm256_mask_blend_epi16(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u16x32(self, mask: b32, if_true: u16x32, if_false: u16x32) -> u16x32 {
        cast(
            self.avx512bw
                ._mm512_mask_blend_epi16(mask.0, cast(if_false), cast(if_true)),
        )
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i16x8(self, mask: b8, if_true: i16x8, if_false: i16x8) -> i16x8 {
        cast(
            self.bwvl()
                ._mm_mask_blend_epi16(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i16x16(self, mask: b16, if_true: i16x16, if_false: i16x16) -> i16x16 {
        cast(
            self.bwvl()
                ._mm256_mask_blend_epi16(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i16x32(self, mask: b32, if_true: i16x32, if_false: i16x32) -> i16x32 {
        cast(
            self.avx512bw
                ._mm512_mask_blend_epi16(mask.0, cast(if_false), cast(if_true)),
        )
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u32x4(self, mask: b8, if_true: u32x4, if_false: u32x4) -> u32x4 {
        cast(
            self.fvl()
                ._mm_mask_blend_epi32(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u32x8(self, mask: b8, if_true: u32x8, if_false: u32x8) -> u32x8 {
        cast(
            self.fvl()
                ._mm256_mask_blend_epi32(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u32x16(self, mask: b16, if_true: u32x16, if_false: u32x16) -> u32x16 {
        cast(
            self.avx512f
                ._mm512_mask_blend_epi32(mask.0, cast(if_false), cast(if_true)),
        )
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i32x4(self, mask: b8, if_true: i32x4, if_false: i32x4) -> i32x4 {
        cast(
            self.fvl()
                ._mm_mask_blend_epi32(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i32x8(self, mask: b8, if_true: i32x8, if_false: i32x8) -> i32x8 {
        cast(
            self.fvl()
                ._mm256_mask_blend_epi32(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i32x16(self, mask: b16, if_true: i32x16, if_false: i32x16) -> i32x16 {
        cast(
            self.avx512f
                ._mm512_mask_blend_epi32(mask.0, cast(if_false), cast(if_true)),
        )
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f32x4(self, mask: b8, if_true: f32x4, if_false: f32x4) -> f32x4 {
        cast(
            self.fvl()
                ._mm_mask_blend_ps(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f32x8(self, mask: b8, if_true: f32x8, if_false: f32x8) -> f32x8 {
        cast(
            self.fvl()
                ._mm256_mask_blend_ps(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f32x16(self, mask: b16, if_true: f32x16, if_false: f32x16) -> f32x16 {
        cast(
            self.avx512f
                ._mm512_mask_blend_ps(mask.0, cast(if_false), cast(if_true)),
        )
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u64x2(self, mask: b8, if_true: u64x2, if_false: u64x2) -> u64x2 {
        cast(
            self.fvl()
                ._mm_mask_blend_epi64(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u64x4(self, mask: b8, if_true: u64x4, if_false: u64x4) -> u64x4 {
        cast(
            self.fvl()
                ._mm256_mask_blend_epi64(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u64x8(self, mask: b8, if_true: u64x8, if_false: u64x8) -> u64x8 {
        cast(
            self.avx512f
                ._mm512_mask_blend_epi64(mask.0, cast(if_false), cast(if_true)),
        )
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i64x2(self, mask: b8, if_true: i64x2, if_false: i64x2) -> i64x2 {
        cast(
            self.fvl()
                ._mm_mask_blend_epi64(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i64x4(self, mask: b8, if_true: i64x4, if_false: i64x4) -> i64x4 {
        cast(
            self.fvl()
                ._mm256_mask_blend_epi64(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i64x8(self, mask: b8, if_true: i64x8, if_false: i64x8) -> i64x8 {
        cast(
            self.avx512f
                ._mm512_mask_blend_epi64(mask.0, cast(if_false), cast(if_true)),
        )
    }

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f64x2(self, mask: b8, if_true: f64x2, if_false: f64x2) -> f64x2 {
        cast(
            self.fvl()
                ._mm_mask_blend_pd(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f64x4(self, mask: b8, if_true: f64x4, if_false: f64x4) -> f64x4 {
        cast(
            self.fvl()
                ._mm256_mask_blend_pd(mask.0, cast(if_false), cast(if_true)),
        )
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// bit in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f64x8(self, mask: b8, if_true: f64x8, if_false: f64x8) -> f64x8 {
        cast(
            self.avx512f
                ._mm512_mask_blend_pd(mask.0, cast(if_false), cast(if_true)),
        )
    }
}

/// x86 arch
#[derive(Debug, Clone, Copy)]
#[non_exhaustive]
#[repr(u8)]
pub(crate) enum ArchInner {
    // msrv(1.66) discriminants on non-unit variants
    Scalar = 0,
    V3 = 1,
    #[cfg(feature = "nightly")]
    V4 = 2,

    // improves codegen for some reason
    #[allow(dead_code)]
    Dummy = u8::MAX - 1,
}

/// x86 arch
#[derive(Debug, Clone, Copy)]
#[non_exhaustive]
#[repr(u8)]
pub(crate) enum ScalarArchInner {
    // msrv(1.66) discriminants on non-unit variants
    Scalar = 0,
    V3 = 1,

    // improves codegen for some reason
    #[allow(dead_code)]
    Dummy = u8::MAX - 1,
}

impl ArchInner {
    /// Detects the best available instruction set.
    #[inline]
    pub fn new() -> Self {
        #[cfg(feature = "nightly")]
        if V4::try_new().is_some() {
            return Self::V4;
        }
        if V3::try_new().is_some() {
            return Self::V3;
        }
        Self::Scalar
    }

    /// Detects the best available instruction set.
    #[inline(always)]
    pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
        match self {
            #[cfg(feature = "nightly")]
            ArchInner::V4 => Simd::vectorize(unsafe { V4::new_unchecked() }, op),
            ArchInner::V3 => Simd::vectorize(unsafe { V3::new_unchecked() }, op),
            ArchInner::Scalar => Simd::vectorize(Scalar::new(), op),
            ArchInner::Dummy => unsafe { core::hint::unreachable_unchecked() },
        }
    }
}

impl Default for ArchInner {
    #[inline]
    fn default() -> Self {
        Self::new()
    }
}

impl ScalarArchInner {
    /// Detects the best available instruction set.
    #[inline]
    pub fn new() -> Self {
        if V3Scalar::try_new().is_some() {
            return Self::V3;
        }
        Self::Scalar
    }

    /// Detects the best available instruction set.
    #[inline(always)]
    pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
        match self {
            ScalarArchInner::V3 => Simd::vectorize(unsafe { V3Scalar::new_unchecked() }, op),
            ScalarArchInner::Scalar => Simd::vectorize(Scalar::new(), op),
            ScalarArchInner::Dummy => unsafe { core::hint::unreachable_unchecked() },
        }
    }
}

impl Default for ScalarArchInner {
    #[inline]
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    extern crate alloc;

    use super::*;
    use alloc::vec;
    use alloc::vec::Vec;
    use assert_approx_eq::assert_approx_eq;
    use core::iter::zip;
    use rand::random;

    #[allow(unused_macros)]
    macro_rules! dbgx {
        () => {
            ::std::eprintln!("[{}:{}]", ::std::file!(), ::std::line!())
        };
        ($val:expr $(,)?) => {
            match $val {
                tmp => {
                    ::std::eprintln!("[{}:{}] {} = {:#X?}",
                        ::std::file!(), ::std::line!(), ::std::stringify!($val), &tmp);
                    tmp
                }
            }
        };
        ($($val:expr),+ $(,)?) => {
            ($(dbgx!($val)),+,)
        };
    }

    #[test]
    fn times_two() {
        let n = 1312;
        let mut v = (0..n).map(|i| i as f64).collect::<Vec<_>>();
        let arch = Arch::new();

        struct TimesThree<'a>(&'a mut [f64]);
        impl<'a> WithSimd for TimesThree<'a> {
            type Output = ();

            #[inline(always)]
            fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
                let v = self.0;
                let (head, tail) = S::f64s_as_mut_simd(v);

                let three = simd.f64s_splat(3.0);
                for x in head {
                    *x = simd.f64s_mul(three, *x);
                }

                for x in tail {
                    *x *= 3.0;
                }
            }
        }

        arch.dispatch(|| {
            for x in &mut v {
                *x *= 2.0;
            }
        });

        arch.dispatch(TimesThree(&mut v));

        for (i, x) in v.into_iter().enumerate() {
            assert_eq!(x, 6.0 * i as f64);
        }
    }

    #[test]
    fn cplx_ops() {
        let n = 16;
        let a = (0..n)
            .map(|_| c32 {
                re: random(),
                im: random(),
            })
            .collect::<Vec<_>>();
        let b = (0..n)
            .map(|_| c32 {
                re: random(),
                im: random(),
            })
            .collect::<Vec<_>>();
        let c = (0..n)
            .map(|_| c32 {
                re: random(),
                im: random(),
            })
            .collect::<Vec<_>>();

        let axb_target = zip(&a, &b).map(|(a, b)| a * b).collect::<Vec<_>>();
        let conjaxb_target = zip(&a, &b).map(|(a, b)| a.conj() * b).collect::<Vec<_>>();
        let axbpc_target = zip(zip(&a, &b), &c)
            .map(|((a, b), c)| a * b + c)
            .collect::<Vec<_>>();
        let conjaxbpc_target = zip(zip(&a, &b), &c)
            .map(|((a, b), c)| a.conj() * b + c)
            .collect::<Vec<_>>();

        if let Some(simd) = V3::try_new() {
            let mut axb = vec![c32::new(0.0, 0.0); n];
            let mut conjaxb = vec![c32::new(0.0, 0.0); n];
            let mut axbpc = vec![c32::new(0.0, 0.0); n];
            let mut conjaxbpc = vec![c32::new(0.0, 0.0); n];

            {
                let a = V3::c32s_as_simd(&a).0;
                let b = V3::c32s_as_simd(&b).0;
                let c = V3::c32s_as_simd(&c).0;
                let axb = V3::c32s_as_mut_simd(&mut axb).0;
                let conjaxb = V3::c32s_as_mut_simd(&mut conjaxb).0;
                let axbpc = V3::c32s_as_mut_simd(&mut axbpc).0;
                let conjaxbpc = V3::c32s_as_mut_simd(&mut conjaxbpc).0;

                for (axb, (a, b)) in zip(axb, zip(a, b)) {
                    *axb = simd.c32s_mul_e(*a, *b);
                }
                for (conjaxb, (a, b)) in zip(conjaxb, zip(a, b)) {
                    *conjaxb = simd.c32s_conj_mul_e(*a, *b);
                }
                for (axbpc, ((a, b), c)) in zip(axbpc, zip(zip(a, b), c)) {
                    *axbpc = simd.c32s_mul_add_e(*a, *b, *c);
                }
                for (conjaxbpc, ((a, b), c)) in zip(conjaxbpc, zip(zip(a, b), c)) {
                    *conjaxbpc = simd.c32s_conj_mul_add_e(*a, *b, *c);
                }
            }

            for (target, actual) in zip(&axb_target, &axb) {
                assert_approx_eq!(target.re, actual.re);
                assert_approx_eq!(target.im, actual.im);
            }
            for (target, actual) in zip(&conjaxb_target, &conjaxb) {
                assert_approx_eq!(target.re, actual.re);
                assert_approx_eq!(target.im, actual.im);
            }
            for (target, actual) in zip(&axbpc_target, &axbpc) {
                assert_approx_eq!(target.re, actual.re);
                assert_approx_eq!(target.im, actual.im);
            }
            for (target, actual) in zip(&conjaxbpc_target, &conjaxbpc) {
                assert_approx_eq!(target.re, actual.re);
                assert_approx_eq!(target.im, actual.im);
            }
        }

        #[cfg(feature = "nightly")]
        if let Some(simd) = V4::try_new() {
            let mut axb = vec![c32::new(0.0, 0.0); n];
            let mut conjaxb = vec![c32::new(0.0, 0.0); n];
            let mut axbpc = vec![c32::new(0.0, 0.0); n];
            let mut conjaxbpc = vec![c32::new(0.0, 0.0); n];

            {
                let a = V4::c32s_as_simd(&a).0;
                let b = V4::c32s_as_simd(&b).0;
                let c = V4::c32s_as_simd(&c).0;
                let axb = V4::c32s_as_mut_simd(&mut axb).0;
                let conjaxb = V4::c32s_as_mut_simd(&mut conjaxb).0;
                let axbpc = V4::c32s_as_mut_simd(&mut axbpc).0;
                let conjaxbpc = V4::c32s_as_mut_simd(&mut conjaxbpc).0;

                for (axb, (a, b)) in zip(axb, zip(a, b)) {
                    *axb = simd.c32s_mul_e(*a, *b);
                }
                for (conjaxb, (a, b)) in zip(conjaxb, zip(a, b)) {
                    *conjaxb = simd.c32s_conj_mul_e(*a, *b);
                }
                for (axbpc, ((a, b), c)) in zip(axbpc, zip(zip(a, b), c)) {
                    *axbpc = simd.c32s_mul_add_e(*a, *b, *c);
                }
                for (conjaxbpc, ((a, b), c)) in zip(conjaxbpc, zip(zip(a, b), c)) {
                    *conjaxbpc = simd.c32s_conj_mul_add_e(*a, *b, *c);
                }
            }

            for (target, actual) in zip(&axb_target, &axb) {
                assert_approx_eq!(target.re, actual.re);
                assert_approx_eq!(target.im, actual.im);
            }
            for (target, actual) in zip(&conjaxb_target, &conjaxb) {
                assert_approx_eq!(target.re, actual.re);
                assert_approx_eq!(target.im, actual.im);
            }
            for (target, actual) in zip(&axbpc_target, &axbpc) {
                assert_approx_eq!(target.re, actual.re);
                assert_approx_eq!(target.im, actual.im);
            }
            for (target, actual) in zip(&conjaxbpc_target, &conjaxbpc) {
                assert_approx_eq!(target.re, actual.re);
                assert_approx_eq!(target.im, actual.im);
            }
        }
    }

    #[test]
    fn test_to_ref() {
        let simd_ref = unsafe { V2::new_unchecked() }.to_ref();
        let _ = *simd_ref;
    }

    #[test]
    fn test_widening_mul_u32x4() {
        if let Some(simd) = V2::try_new() {
            const N: usize = 4;
            let a = u32x4(2298413717, 568259975, 2905436181, 175547995);
            let b = u32x4(2022374205, 1446824162, 3165580604, 3011091403);
            let a_array: [u32; N] = cast(a);
            let b_array: [u32; N] = cast(b);
            let mut lo_array = [0u32; N];
            let mut hi_array = [0u32; N];

            for i in 0..N {
                let prod = a_array[i] as u64 * b_array[i] as u64;
                let lo = prod as u32;
                let hi = (prod >> 32) as u32;
                lo_array[i] = lo;
                hi_array[i] = hi;
            }

            let (lo, hi) = simd.widening_mul_u32x4(a, b);
            assert_eq!(lo, cast(lo_array));
            assert_eq!(hi, cast(hi_array));
        }
        if let Some(simd) = V3::try_new() {
            const N: usize = 8;
            let a = u32x8(
                2298413717, 568259975, 2905436181, 175547995, 2298413717, 568259975, 2905436181,
                175547995,
            );
            let b = u32x8(
                2022374205, 1446824162, 3165580604, 3011091403, 2022374205, 1446824162, 3165580604,
                3011091403,
            );
            let a_array: [u32; N] = cast(a);
            let b_array: [u32; N] = cast(b);
            let mut lo_array = [0u32; N];
            let mut hi_array = [0u32; N];

            for i in 0..N {
                let prod = a_array[i] as u64 * b_array[i] as u64;
                let lo = prod as u32;
                let hi = (prod >> 32) as u32;
                lo_array[i] = lo;
                hi_array[i] = hi;
            }

            let (lo, hi) = simd.widening_mul_u32x8(a, b);
            assert_eq!(lo, cast(lo_array));
            assert_eq!(hi, cast(hi_array));
        }
    }

    #[test]
    fn test_widening_mul_i32() {
        if let Some(simd) = V2::try_new() {
            const N: usize = 4;
            let a = cast(u32x4(2298413717, 568259975, 2905436181, 175547995));
            let b = cast(u32x4(2022374205, 1446824162, 3165580604, 3011091403));

            let a_array: [i32; N] = cast(a);
            let b_array: [i32; N] = cast(b);
            let mut lo_array = [0i32; N];
            let mut hi_array = [0i32; N];

            for i in 0..N {
                let prod = a_array[i] as i64 * b_array[i] as i64;
                let lo = prod as i32;
                let hi = (prod >> 32) as i32;
                lo_array[i] = lo;
                hi_array[i] = hi;
            }

            let (lo, hi) = simd.widening_mul_i32x4(a, b);
            assert_eq!(lo, cast(lo_array));
            assert_eq!(hi, cast(hi_array));
        }
        if let Some(simd) = V3::try_new() {
            const N: usize = 8;
            let a = cast(u32x8(
                2298413717, 568259975, 2905436181, 175547995, 2298413717, 568259975, 2905436181,
                175547995,
            ));
            let b = cast(u32x8(
                2022374205, 1446824162, 3165580604, 3011091403, 2022374205, 1446824162, 3165580604,
                3011091403,
            ));

            let a_array: [i32; N] = cast(a);
            let b_array: [i32; N] = cast(b);
            let mut lo_array = [0i32; N];
            let mut hi_array = [0i32; N];

            for i in 0..N {
                let prod = a_array[i] as i64 * b_array[i] as i64;
                let lo = prod as i32;
                let hi = (prod >> 32) as i32;
                lo_array[i] = lo;
                hi_array[i] = hi;
            }

            let (lo, hi) = simd.widening_mul_i32x8(a, b);
            assert_eq!(lo, cast(lo_array));
            assert_eq!(hi, cast(hi_array));
        }
    }

    #[test]
    fn test_shift() {
        if let Some(simd) = V2::try_new() {
            let a = u16x8(54911, 46958, 49991, 22366, 46365, 39572, 22704, 60060);
            assert_eq!(simd.shl_const_u16x8::<16>(a), simd.splat_u16x8(0));
            assert_eq!(simd.shl_u16x8(a, simd.splat_u64x2(!0)), simd.splat_u16x8(0),);
        }
    }

    #[test]
    fn test_abs() {
        if let Some(simd) = V2::try_new() {
            let a = f32x4(1.0, -2.0, -1.0, 2.0);
            assert_eq!(simd.abs_f32x4(a), f32x4(1.0, 2.0, 1.0, 2.0));
            let a = f64x2(1.0, -2.0);
            assert_eq!(simd.abs_f64x2(a), f64x2(1.0, 2.0));
        }
    }

    #[test]
    fn test_subadd() {
        if let Some(simd) = V2::try_new() {
            let a = f32x4(1.0, -2.0, -1.0, 2.0);
            assert_eq!(simd.subadd_f32x4(a, a), f32x4(0.0, -4.0, 0.0, 4.0));
        }
    }

    #[test]
    fn test_signed_to_unsigned() {
        if let Some(simd) = V2::try_new() {
            let a = i8x16(1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            assert_eq!(simd.convert_i8x16_to_u64x2(a), u64x2(1, !0));
        }
    }

    #[test]
    fn test_int_cmp() {
        if let Some(simd) = V2::try_new() {
            {
                const N: usize = 16;

                let a = u8x16(
                    174, 191, 248, 232, 11, 186, 42, 236, 3, 59, 223, 72, 161, 146, 98, 69,
                );
                let b = u8x16(
                    97, 239, 164, 173, 208, 0, 121, 247, 218, 58, 119, 131, 213, 133, 22, 128,
                );
                let lt = simd.cmp_lt_u8x16(a, b);

                let a_array: [u8; N] = cast(a);
                let b_array: [u8; N] = cast(b);
                let mut lt_array = [m8::new(false); N];

                for i in 0..N {
                    lt_array[i] = m8::new(a_array[i] < b_array[i]);
                }

                assert_eq!(lt, unsafe { transmute(lt_array) });
            }
            {
                const N: usize = 8;

                let a = u16x8(174, 191, 248, 232, 11, 186, 42, 236);
                let b = u16x8(97, 239, 164, 173, 208, 0, 121, 247);
                let lt = simd.cmp_lt_u16x8(a, b);

                let a_array: [u16; N] = cast(a);
                let b_array: [u16; N] = cast(b);
                let mut lt_array = [m16::new(false); N];

                for i in 0..N {
                    lt_array[i] = m16::new(a_array[i] < b_array[i]);
                }

                assert_eq!(lt, unsafe { transmute(lt_array) });
            }
            {
                const N: usize = 4;

                let a = u32x4(174, 191, 248, 232);
                let b = u32x4(97, 239, 164, 173);
                let lt = simd.cmp_lt_u32x4(a, b);

                let a_array: [u32; N] = cast(a);
                let b_array: [u32; N] = cast(b);
                let mut lt_array = [m32::new(false); N];

                for i in 0..N {
                    lt_array[i] = m32::new(a_array[i] < b_array[i]);
                }

                assert_eq!(lt, unsafe { transmute(lt_array) });
            }
            {
                const N: usize = 2;

                let a = u64x2(174, 191);
                let b = u64x2(97, 239);
                let lt = simd.cmp_lt_u64x2(a, b);

                let a_array: [u64; N] = cast(a);
                let b_array: [u64; N] = cast(b);
                let mut lt_array = [m64::new(false); N];

                for i in 0..N {
                    lt_array[i] = m64::new(a_array[i] < b_array[i]);
                }

                assert_eq!(lt, unsafe { transmute(lt_array) });
            }
        }
    }

    #[test]
    fn test_is_nan() {
        if let Some(simd) = V2::try_new() {
            assert_eq!(
                simd.is_nan_f32x4(f32x4(0.0, f32::NAN, f32::INFINITY, -f32::NAN)),
                m32x4(
                    m32::new(false),
                    m32::new(true),
                    m32::new(false),
                    m32::new(true),
                ),
            );
            assert_eq!(
                simd.is_nan_f64x2(f64x2(0.0, f64::NAN)),
                m64x2(m64::new(false), m64::new(true)),
            );
        }
    }

    #[test]
    fn test_aligned() {
        #[repr(align(128))]
        #[derive(Copy, Clone, Debug)]
        struct Aligned<T>(T);

        if let Some(simd) = V3::try_new() {
            {
                let data = Aligned([
                    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                    22, 23, 24, 25, 26, 27, 28, 29, 30, 31u32,
                ]);
                let data = data.0.as_slice();
                let none = u32::MAX;
                {
                    // loading aligned data
                    let offset = simd.u32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.u32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.u32s_splat(none)),
                        u32x8(0, 1, 2, 3, 4, 5, 6, 7),
                    );
                    assert_eq!(
                        body,
                        [
                            u32x8(8, 9, 10, 11, 12, 13, 14, 15),
                            u32x8(16, 17, 18, 19, 20, 21, 22, 23),
                        ],
                    );
                    assert_eq!(
                        suffix.read_or(simd.u32s_splat(none)),
                        u32x8(24, 25, 26, 27, 28, 29, 30, 31),
                    );
                }
                {
                    // loading aligned data with one chunk
                    let data = &data[0..8];
                    let offset = simd.u32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.u32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.u32s_splat(none)),
                        u32x8(0, 1, 2, 3, 4, 5, 6, 7),
                    );
                    assert_eq!(body, []);
                    assert_eq!(
                        suffix.read_or(simd.u32s_splat(none)),
                        u32x8(none, none, none, none, none, none, none, none),
                    );
                }
                {
                    // loading aligned data with one partial chunk
                    let data = &data[0..3];
                    let offset = simd.u32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.u32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.u32s_splat(none)),
                        u32x8(0, 1, 2, none, none, none, none, none),
                    );
                    assert_eq!(body, []);
                    assert_eq!(
                        suffix.read_or(simd.u32s_splat(none)),
                        u32x8(none, none, none, none, none, none, none, none),
                    );
                }
                {
                    // loading aligned data with two chunks
                    let data = &data[0..16];
                    let offset = simd.u32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.u32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.u32s_splat(none)),
                        u32x8(0, 1, 2, 3, 4, 5, 6, 7),
                    );
                    assert_eq!(body, []);
                    assert_eq!(
                        suffix.read_or(simd.u32s_splat(none)),
                        u32x8(8, 9, 10, 11, 12, 13, 14, 15),
                    );
                }
                {
                    // loading unaligned data
                    let data = &data[3..];
                    let offset = simd.u32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.u32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.u32s_splat(none)),
                        u32x8(none, none, none, 3, 4, 5, 6, 7),
                    );
                    assert_eq!(
                        body,
                        [
                            u32x8(8, 9, 10, 11, 12, 13, 14, 15),
                            u32x8(16, 17, 18, 19, 20, 21, 22, 23),
                        ],
                    );
                    assert_eq!(
                        suffix.read_or(simd.u32s_splat(none)),
                        u32x8(24, 25, 26, 27, 28, 29, 30, 31),
                    );
                }
                {
                    // loading unaligned data with one chunk
                    let data = &data[3..11];
                    let offset = simd.u32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.u32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.u32s_splat(none)),
                        u32x8(none, none, none, 3, 4, 5, 6, 7),
                    );
                    assert_eq!(body, []);
                    assert_eq!(
                        suffix.read_or(simd.u32s_splat(none)),
                        u32x8(8, 9, 10, none, none, none, none, none),
                    );
                }
                {
                    // loading unaligned data with one partial chunk
                    let data = &data[3..6];
                    let offset = simd.u32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.u32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.u32s_splat(none)),
                        u32x8(none, none, none, 3, 4, 5, none, none),
                    );
                    assert_eq!(body, []);
                    assert_eq!(
                        suffix.read_or(simd.u32s_splat(none)),
                        u32x8(none, none, none, none, none, none, none, none),
                    );
                }
                {
                    // loading unaligned data with two chunks
                    let data = &data[3..19];
                    let offset = simd.u32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.u32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.u32s_splat(none)),
                        u32x8(none, none, none, 3, 4, 5, 6, 7),
                    );
                    assert_eq!(body, [u32x8(8, 9, 10, 11, 12, 13, 14, 15)]);
                    assert_eq!(
                        suffix.read_or(simd.u32s_splat(none)),
                        u32x8(16, 17, 18, none, none, none, none, none),
                    );
                }
            }

            {
                let data = Aligned(
                    [
                        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                        21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31u32,
                    ]
                    .map(|i| i as f32),
                );
                let data = data.0.as_slice();
                let none = c32 {
                    re: 1312.0,
                    im: 0.5,
                };
                {
                    let data = bytemuck::cast_slice(data);
                    let offset = simd.c32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.c32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.c32s_splat(none)),
                        f32x8(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0),
                    );
                    assert_eq!(
                        body,
                        [
                            f32x8(8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0),
                            f32x8(16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0),
                        ],
                    );
                    assert_eq!(
                        suffix.read_or(simd.c32s_splat(none)),
                        f32x8(24.0, 25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0),
                    );
                }
                {
                    let data = bytemuck::cast_slice(&data[1..31]);
                    let offset = simd.c32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.c32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.c32s_splat(none)),
                        f32x8(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0),
                    );
                    assert_eq!(
                        body,
                        [
                            f32x8(9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0),
                            f32x8(17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0),
                        ],
                    );
                    assert_eq!(
                        suffix.read_or(simd.c32s_splat(none)),
                        f32x8(25.0, 26.0, 27.0, 28.0, 29.0, 30.0, none.re, none.im),
                    );
                }
                {
                    let data = bytemuck::cast_slice(&data[4..8]);
                    let offset = simd.c32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.c32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.c32s_splat(none)),
                        f32x8(none.re, none.im, none.re, none.im, 4.0, 5.0, 6.0, 7.0),
                    );
                    assert_eq!(body, []);
                    assert_eq!(
                        suffix.read_or(simd.c32s_splat(none)),
                        f32x8(
                            none.re, none.im, none.re, none.im, none.re, none.im, none.re, none.im
                        ),
                    );
                }

                {
                    let data = bytemuck::cast_slice(&data[4..10]);
                    let offset = simd.c32s_align_offset(data.as_ptr(), data.len());
                    let (prefix, body, suffix) = simd.c32s_as_aligned_simd(data, offset);
                    assert_eq!(
                        prefix.read_or(simd.c32s_splat(none)),
                        f32x8(none.re, none.im, none.re, none.im, 4.0, 5.0, 6.0, 7.0),
                    );
                    assert_eq!(body, []);
                    assert_eq!(
                        suffix.read_or(simd.c32s_splat(none)),
                        f32x8(8.0, 9.0, none.re, none.im, none.re, none.im, none.re, none.im),
                    );
                }
            }
        }
    }

    #[test]
    fn test_rotate() {
        if let Some(simd) = V3::try_new() {
            for amount in 0..128 {
                let mut array = [0u32; 8];
                for (i, dst) in array.iter_mut().enumerate() {
                    *dst = 1000 + i as u32;
                }

                let rot: [u32; 8] = cast(simd.u32s_rotate_right(cast(array), amount));
                for i in 0..8 {
                    assert_eq!(rot[(i + amount) % 8], array[i]);
                }
            }
            for amount in 0..128 {
                let mut array = [0u64; 4];
                for (i, dst) in array.iter_mut().enumerate() {
                    *dst = 1000 + i as u64;
                }

                let rot: [u64; 4] = cast(simd.u64s_rotate_right(cast(array), amount));
                for i in 0..4 {
                    assert_eq!(rot[(i + amount) % 4], array[i]);
                }
            }
        }

        #[cfg(feature = "nightly")]
        if let Some(simd) = V4::try_new() {
            for amount in 0..128 {
                let mut array = [0u32; 16];
                for (i, dst) in array.iter_mut().enumerate() {
                    *dst = 1000 + i as u32;
                }

                let rot: [u32; 16] = cast(simd.u32s_rotate_right(cast(array), amount));
                for i in 0..16 {
                    assert_eq!(rot[(i + amount) % 16], array[i]);
                }
            }
            for amount in 0..128 {
                let mut array = [0u64; 8];
                for (i, dst) in array.iter_mut().enumerate() {
                    *dst = 1000 + i as u64;
                }

                let rot: [u64; 8] = cast(simd.u64s_rotate_right(cast(array), amount));
                for i in 0..8 {
                    assert_eq!(rot[(i + amount) % 8], array[i]);
                }
            }
        }
    }
}
