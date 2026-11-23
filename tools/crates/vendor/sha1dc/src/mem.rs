//! Every load and store between memory and a vector register in the crate.
//!
//! Each function moves exactly the bytes of the reference it is handed, and
//! the intrinsic it wraps has no alignment requirement beyond that of the
//! reference's type. That is the whole of the safety argument, and it is the
//! same one for every function here.
//!
//! Working out *which* words to move is left to the callers, in safe code:
//! they index arrays, and [`Schedule::window`](crate::Schedule::window) proves
//! its bound at compile time. A mistake there is a build failure or a panic,
//! never a read out of bounds.

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
mod imp {
    #[cfg(target_arch = "x86")]
    use core::arch::x86::*;
    #[cfg(target_arch = "x86_64")]
    use core::arch::x86_64::*;

    /// Four words, in memory order.
    #[inline]
    #[target_feature(enable = "sse2")]
    pub(crate) fn load_u32x4(src: &[u32; 4]) -> __m128i {
        // SAFETY: `src` is 16 readable bytes, which is what this reads, and
        // `_mm_loadu_si128` needs no alignment.
        unsafe { _mm_loadu_si128(src.as_ptr().cast()) }
    }

    /// Sixteen bytes, in memory order.
    #[inline]
    #[target_feature(enable = "sse2")]
    pub(crate) fn load_u8x16(src: &[u8; 16]) -> __m128i {
        // SAFETY: `src` is 16 readable bytes, which is what this reads, and
        // `_mm_loadu_si128` needs no alignment.
        unsafe { _mm_loadu_si128(src.as_ptr().cast()) }
    }

    /// Eight words, in memory order.
    #[allow(
        dead_code,
        reason = "only the AVX2 form reads eight words, and not every build has it"
    )]
    #[inline]
    #[target_feature(enable = "avx")]
    pub(crate) fn load_u32x8(src: &[u32; 8]) -> __m256i {
        // SAFETY: `src` is 32 readable bytes, which is what this reads, and
        // `_mm256_loadu_si256` needs no alignment.
        unsafe { _mm256_loadu_si256(src.as_ptr().cast()) }
    }

    /// Writes four words, in memory order.
    #[inline]
    #[target_feature(enable = "sse2")]
    pub(crate) fn store_u32x4(dst: &mut [u32; 4], v: __m128i) {
        // SAFETY: `dst` is 16 writable bytes, which is what this writes, and
        // `_mm_storeu_si128` needs no alignment.
        unsafe { _mm_storeu_si128(dst.as_mut_ptr().cast(), v) }
    }
}

#[cfg(all(target_arch = "aarch64", target_feature = "neon"))]
mod imp {
    use core::arch::aarch64::*;

    /// Four words, in memory order.
    #[inline]
    #[target_feature(enable = "neon")]
    pub(crate) fn load_u32x4(src: &[u32; 4]) -> uint32x4_t {
        // SAFETY: `src` is four readable, aligned `u32`s, which is what this
        // reads.
        unsafe { vld1q_u32(src.as_ptr()) }
    }

    /// Sixteen bytes, in memory order.
    #[inline]
    #[target_feature(enable = "neon")]
    pub(crate) fn load_u8x16(src: &[u8; 16]) -> uint8x16_t {
        // SAFETY: `src` is sixteen readable bytes, which is what this reads.
        unsafe { vld1q_u8(src.as_ptr()) }
    }

    /// Writes four words, in memory order.
    #[inline]
    #[target_feature(enable = "neon")]
    pub(crate) fn store_u32x4(dst: &mut [u32; 4], v: uint32x4_t) {
        // SAFETY: `dst` is four writable, aligned `u32`s, which is what this
        // writes.
        unsafe { vst1q_u32(dst.as_mut_ptr(), v) }
    }
}

#[cfg(any(
    target_arch = "x86",
    target_arch = "x86_64",
    all(target_arch = "aarch64", target_feature = "neon")
))]
#[allow(
    unused_imports,
    reason = "a target without a vector form uses only some"
)]
pub(crate) use imp::*;
