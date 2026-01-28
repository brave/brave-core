pub use bytemuck::Pod;
#[cfg(feature = "f16")]
use half::f16;
pub use pulp::{cast, NullaryFnOnce};

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
pub use x86::*;

#[cfg(target_arch = "aarch64")]
pub use aarch64::*;

use crate::gemm::{c32, c64};

pub trait Simd: Copy + Send + Sync + 'static {
    unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output;
}

#[derive(Copy, Clone, Debug)]
pub struct Scalar;

impl Simd for Scalar {
    #[inline(always)]
    unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
        f.call()
    }
}

#[cfg(feature = "f16")]
unsafe impl MixedSimd<f16, f16, f16, f32> for Scalar {
    const SIMD_WIDTH: usize = 1;

    type LhsN = f16;
    type RhsN = f16;
    type DstN = f16;
    type AccN = f32;

    #[inline]
    fn try_new() -> Option<Self> {
        Some(Self)
    }

    #[inline(always)]
    fn add(self, lhs: f32, rhs: f32) -> f32 {
        lhs + rhs
    }

    #[inline(always)]
    fn mult(self, lhs: f32, rhs: f32) -> f32 {
        lhs * rhs
    }

    #[inline(always)]
    fn mult_add(self, lhs: f32, rhs: f32, acc: f32) -> f32 {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn from_lhs(self, lhs: f16) -> f32 {
        lhs.into()
    }

    #[inline(always)]
    fn from_rhs(self, rhs: f16) -> f32 {
        rhs.into()
    }

    #[inline(always)]
    fn from_dst(self, dst: f16) -> f32 {
        dst.into()
    }

    #[inline(always)]
    fn into_dst(self, acc: f32) -> f16 {
        f16::from_f32(acc)
    }

    #[inline(always)]
    fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
        lhs.into()
    }

    #[inline(always)]
    fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
        rhs.into()
    }

    #[inline(always)]
    fn simd_splat(self, lhs: f32) -> Self::AccN {
        lhs
    }

    #[inline(always)]
    fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
        dst.into()
    }

    #[inline(always)]
    fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
        f16::from_f32(acc)
    }

    #[inline(always)]
    fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
        f.call()
    }

    #[inline(always)]
    fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs * rhs
    }

    #[inline(always)]
    fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs + rhs
    }
}

unsafe impl MixedSimd<f32, f32, f32, f32> for Scalar {
    const SIMD_WIDTH: usize = 1;

    type LhsN = f32;
    type RhsN = f32;
    type DstN = f32;
    type AccN = f32;

    #[inline]
    fn try_new() -> Option<Self> {
        Some(Self)
    }

    #[inline(always)]
    fn mult(self, lhs: f32, rhs: f32) -> f32 {
        lhs * rhs
    }

    #[inline(always)]
    fn mult_add(self, lhs: f32, rhs: f32, acc: f32) -> f32 {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn from_lhs(self, lhs: f32) -> f32 {
        lhs
    }

    #[inline(always)]
    fn from_rhs(self, rhs: f32) -> f32 {
        rhs
    }

    #[inline(always)]
    fn from_dst(self, dst: f32) -> f32 {
        dst
    }

    #[inline(always)]
    fn into_dst(self, acc: f32) -> f32 {
        acc
    }

    #[inline(always)]
    fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
        lhs
    }

    #[inline(always)]
    fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
        rhs
    }

    #[inline(always)]
    fn simd_splat(self, lhs: f32) -> Self::AccN {
        lhs
    }

    #[inline(always)]
    fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
        dst
    }

    #[inline(always)]
    fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
        acc
    }

    #[inline(always)]
    fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
        f.call()
    }

    #[inline(always)]
    fn add(self, lhs: f32, rhs: f32) -> f32 {
        lhs + rhs
    }

    #[inline(always)]
    fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs * rhs
    }

    #[inline(always)]
    fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs + rhs
    }
}

unsafe impl MixedSimd<f64, f64, f64, f64> for Scalar {
    const SIMD_WIDTH: usize = 1;

    type LhsN = f64;
    type RhsN = f64;
    type DstN = f64;
    type AccN = f64;

    #[inline]
    fn try_new() -> Option<Self> {
        Some(Self)
    }

    #[inline(always)]
    fn mult(self, lhs: f64, rhs: f64) -> f64 {
        lhs * rhs
    }

    #[inline(always)]
    fn mult_add(self, lhs: f64, rhs: f64, acc: f64) -> f64 {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn from_lhs(self, lhs: f64) -> f64 {
        lhs
    }

    #[inline(always)]
    fn from_rhs(self, rhs: f64) -> f64 {
        rhs
    }

    #[inline(always)]
    fn from_dst(self, dst: f64) -> f64 {
        dst
    }

    #[inline(always)]
    fn into_dst(self, acc: f64) -> f64 {
        acc
    }

    #[inline(always)]
    fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
        lhs
    }

    #[inline(always)]
    fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
        rhs
    }

    #[inline(always)]
    fn simd_splat(self, lhs: f64) -> Self::AccN {
        lhs
    }

    #[inline(always)]
    fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
        dst
    }

    #[inline(always)]
    fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
        acc
    }

    #[inline(always)]
    fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
        f.call()
    }

    #[inline(always)]
    fn add(self, lhs: f64, rhs: f64) -> f64 {
        lhs + rhs
    }

    #[inline(always)]
    fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs * rhs
    }

    #[inline(always)]
    fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs + rhs
    }
}

unsafe impl MixedSimd<c32, c32, c32, c32> for Scalar {
    const SIMD_WIDTH: usize = 1;

    type LhsN = c32;
    type RhsN = c32;
    type DstN = c32;
    type AccN = c32;

    #[inline]
    fn try_new() -> Option<Self> {
        Some(Self)
    }

    #[inline(always)]
    fn mult(self, lhs: c32, rhs: c32) -> c32 {
        lhs * rhs
    }

    #[inline(always)]
    fn mult_add(self, lhs: c32, rhs: c32, acc: c32) -> c32 {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn from_lhs(self, lhs: c32) -> c32 {
        lhs
    }

    #[inline(always)]
    fn from_rhs(self, rhs: c32) -> c32 {
        rhs
    }

    #[inline(always)]
    fn from_dst(self, dst: c32) -> c32 {
        dst
    }

    #[inline(always)]
    fn into_dst(self, acc: c32) -> c32 {
        acc
    }

    #[inline(always)]
    fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
        lhs
    }

    #[inline(always)]
    fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
        rhs
    }

    #[inline(always)]
    fn simd_splat(self, lhs: c32) -> Self::AccN {
        lhs
    }

    #[inline(always)]
    fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
        dst
    }

    #[inline(always)]
    fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
        acc
    }

    #[inline(always)]
    fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
        f.call()
    }

    #[inline(always)]
    fn add(self, lhs: c32, rhs: c32) -> c32 {
        lhs + rhs
    }

    #[inline(always)]
    fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs * rhs
    }

    #[inline(always)]
    fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs + rhs
    }
}

unsafe impl MixedSimd<c64, c64, c64, c64> for Scalar {
    const SIMD_WIDTH: usize = 1;

    type LhsN = c64;
    type RhsN = c64;
    type DstN = c64;
    type AccN = c64;

    #[inline]
    fn try_new() -> Option<Self> {
        Some(Self)
    }

    #[inline(always)]
    fn mult(self, lhs: c64, rhs: c64) -> c64 {
        lhs * rhs
    }

    #[inline(always)]
    fn mult_add(self, lhs: c64, rhs: c64, acc: c64) -> c64 {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn from_lhs(self, lhs: c64) -> c64 {
        lhs
    }

    #[inline(always)]
    fn from_rhs(self, rhs: c64) -> c64 {
        rhs
    }

    #[inline(always)]
    fn from_dst(self, dst: c64) -> c64 {
        dst
    }

    #[inline(always)]
    fn into_dst(self, acc: c64) -> c64 {
        acc
    }

    #[inline(always)]
    fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
        lhs * rhs + acc
    }

    #[inline(always)]
    fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
        lhs
    }

    #[inline(always)]
    fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
        rhs
    }

    #[inline(always)]
    fn simd_splat(self, lhs: c64) -> Self::AccN {
        lhs
    }

    #[inline(always)]
    fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
        dst
    }

    #[inline(always)]
    fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
        acc
    }

    #[inline(always)]
    fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
        f.call()
    }

    #[inline(always)]
    fn add(self, lhs: c64, rhs: c64) -> c64 {
        lhs + rhs
    }

    #[inline(always)]
    fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs * rhs
    }

    #[inline(always)]
    fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
        lhs + rhs
    }
}

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
mod x86 {
    use super::*;
    #[cfg(target_arch = "x86")]
    use core::arch::x86::*;
    #[cfg(target_arch = "x86_64")]
    use core::arch::x86_64::*;

    #[inline(always)]
    pub unsafe fn v3_fmaf(a: f32, b: f32, c: f32) -> f32 {
        use pulp::Simd;
        pulp::x86::V3::new_unchecked().f32_scalar_mul_add(a, b, c)
    }

    #[inline(always)]
    pub unsafe fn v3_fma(a: f64, b: f64, c: f64) -> f64 {
        use pulp::Simd;
        pulp::x86::V3::new_unchecked().f64_scalar_mul_add(a, b, c)
    }

    #[derive(Copy, Clone)]
    pub struct Sse;
    #[derive(Copy, Clone)]
    pub struct Avx;
    #[derive(Copy, Clone)]
    pub struct Fma;

    #[cfg(feature = "nightly")]
    #[derive(Copy, Clone)]
    pub struct Avx512f;

    impl Simd for Sse {
        #[inline]
        #[target_feature(enable = "sse,sse2")]
        unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
            f.call()
        }
    }

    impl Simd for Avx {
        #[inline]
        #[target_feature(enable = "avx")]
        unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
            f.call()
        }
    }

    impl Simd for Fma {
        #[inline]
        #[target_feature(enable = "fma")]
        unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
            f.call()
        }
    }

    #[cfg(feature = "nightly")]
    impl Simd for Avx512f {
        #[inline]
        #[target_feature(enable = "avx512f")]
        unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
            f.call()
        }
    }

    #[derive(Debug, Copy, Clone)]
    pub struct V3Half {
        __private: (),
    }

    #[cfg(feature = "f16")]
    pulp::simd_type! {
        pub struct V3 {
            pub sse: "sse",
            pub sse2: "sse2",
            pub fxsr: "fxsr",
            pub sse3: "sse3",
            pub ssse3: "ssse3",
            pub sse4_1: "sse4.1",
            pub sse4_2: "sse4.2",
            pub avx: "avx",
            pub avx2: "avx2",
            pub fma: "fma",
            pub f16c: "f16c",
        }
    }

    #[cfg(feature = "nightly")]
    #[cfg(feature = "f16")]
    pulp::simd_type! {
        pub struct V4 {
            pub sse: "sse",
            pub sse2: "sse2",
            pub fxsr: "fxsr",
            pub sse3: "sse3",
            pub ssse3: "ssse3",
            pub sse4_1: "sse4.1",
            pub sse4_2: "sse4.2",
            pub avx: "avx",
            pub avx2: "avx2",
            pub fma: "fma",
            pub f16c: "f16c",
            pub avx512f: "avx512f",
        }
    }

    #[cfg(not(feature = "f16"))]
    pulp::simd_type! {
        pub struct V3 {
            pub sse: "sse",
            pub sse2: "sse2",
            pub fxsr: "fxsr",
            pub sse3: "sse3",
            pub ssse3: "ssse3",
            pub sse4_1: "sse4.1",
            pub sse4_2: "sse4.2",
            pub avx: "avx",
            pub avx2: "avx2",
            pub fma: "fma",
        }
    }

    #[cfg(feature = "nightly")]
    #[cfg(not(feature = "f16"))]
    pulp::simd_type! {
        pub struct V4 {
            pub sse: "sse",
            pub sse2: "sse2",
            pub fxsr: "fxsr",
            pub sse3: "sse3",
            pub ssse3: "ssse3",
            pub sse4_1: "sse4.1",
            pub sse4_2: "sse4.2",
            pub avx: "avx",
            pub avx2: "avx2",
            pub fma: "fma",
            pub avx512f: "avx512f",
        }
    }

    impl Simd for V3Half {
        unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
            f.call()
        }
    }

    #[cfg(feature = "f16")]
    unsafe impl MixedSimd<f16, f16, f16, f32> for V3Half {
        const SIMD_WIDTH: usize = 4;

        type LhsN = [f16; 4];
        type RhsN = [f16; 4];
        type DstN = [f16; 4];
        type AccN = [f32; 4];

        #[inline]
        fn try_new() -> Option<Self> {
            Some(Self { __private: () })
        }

        #[inline(always)]
        fn mult(self, lhs: f32, rhs: f32) -> f32 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: f32, rhs: f32, acc: f32) -> f32 {
            unsafe { v3_fmaf(lhs, rhs, acc) }
        }

        #[inline(always)]
        fn from_lhs(self, _lhs: f16) -> f32 {
            todo!()
        }

        #[inline(always)]
        fn from_rhs(self, _rhs: f16) -> f32 {
            todo!()
        }

        #[inline(always)]
        fn from_dst(self, _dst: f16) -> f32 {
            todo!()
        }

        #[inline(always)]
        fn into_dst(self, _acc: f32) -> f16 {
            todo!()
        }

        #[inline(always)]
        fn simd_mult_add(self, _lhs: Self::AccN, _rhs: Self::AccN, _acc: Self::AccN) -> Self::AccN {
            todo!()
        }

        #[inline(always)]
        fn simd_from_lhs(self, _lhs: Self::LhsN) -> Self::AccN {
            todo!()
        }

        #[inline(always)]
        fn simd_from_rhs(self, _rhs: Self::RhsN) -> Self::AccN {
            todo!()
        }

        #[inline(always)]
        fn simd_splat(self, _lhs: f32) -> Self::AccN {
            todo!()
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            unsafe { cast(_mm_cvtph_ps(cast([dst, [f16::ZERO; 4]]))) }
        }

        #[inline(always)]
        fn simd_into_dst(self, _acc: Self::AccN) -> Self::DstN {
            todo!()
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, _f: F) -> F::Output {
            todo!()
        }

        #[inline(always)]
        fn add(self, _lhs: f32, _rhs: f32) -> f32 {
            todo!()
        }

        #[inline(always)]
        fn simd_mul(self, _lhs: Self::AccN, _rhs: Self::AccN) -> Self::AccN {
            todo!()
        }

        #[inline(always)]
        fn simd_add(self, _lhs: Self::AccN, _rhs: Self::AccN) -> Self::AccN {
            todo!()
        }
    }

    #[cfg(feature = "f16")]
    unsafe impl MixedSimd<f16, f16, f16, f32> for V3 {
        const SIMD_WIDTH: usize = 8;

        type LhsN = [f16; 8];
        type RhsN = [f16; 8];
        type DstN = [f16; 8];
        type AccN = [f32; 8];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: f32, rhs: f32) -> f32 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: f32, rhs: f32, acc: f32) -> f32 {
            unsafe { v3_fmaf(lhs, rhs, acc) }
        }

        #[inline(always)]
        fn from_lhs(self, lhs: f16) -> f32 {
            unsafe { pulp::cast_lossy(_mm_cvtph_ps(self.sse2._mm_set1_epi16(cast(lhs)))) }
        }

        #[inline(always)]
        fn from_rhs(self, rhs: f16) -> f32 {
            unsafe { pulp::cast_lossy(_mm_cvtph_ps(self.sse2._mm_set1_epi16(cast(rhs)))) }
        }

        #[inline(always)]
        fn from_dst(self, dst: f16) -> f32 {
            unsafe { pulp::cast_lossy(_mm_cvtph_ps(self.sse2._mm_set1_epi16(cast(dst)))) }
        }

        #[inline(always)]
        fn into_dst(self, acc: f32) -> f16 {
            unsafe {
                pulp::cast_lossy(_mm_cvtps_ph::<_MM_FROUND_CUR_DIRECTION>(
                    self.sse._mm_load_ss(&acc),
                ))
            }
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            cast(self.fma._mm256_fmadd_ps(cast(lhs), cast(rhs), cast(acc)))
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            unsafe { cast(_mm256_cvtph_ps(cast(lhs))) }
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            unsafe { cast(_mm256_cvtph_ps(cast(rhs))) }
        }

        #[inline(always)]
        fn simd_splat(self, lhs: f32) -> Self::AccN {
            cast(self.avx._mm256_set1_ps(lhs))
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            unsafe { cast(_mm256_cvtph_ps(cast(dst))) }
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            unsafe { cast(_mm256_cvtps_ph::<_MM_FROUND_CUR_DIRECTION>(cast(acc))) }
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: f32, rhs: f32) -> f32 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx._mm256_mul_ps(cast(lhs), cast(rhs)))
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx._mm256_add_ps(cast(lhs), cast(rhs)))
        }
    }

    unsafe impl MixedSimd<c32, c32, c32, c32> for V3 {
        const SIMD_WIDTH: usize = 4;

        type LhsN = [c32; 4];
        type RhsN = [c32; 4];
        type DstN = [c32; 4];
        type AccN = [c32; 4];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: c32, rhs: c32) -> c32 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: c32, rhs: c32, acc: c32) -> c32 {
            lhs * rhs + acc
        }

        #[inline(always)]
        fn from_lhs(self, lhs: c32) -> c32 {
            lhs
        }

        #[inline(always)]
        fn from_rhs(self, rhs: c32) -> c32 {
            rhs
        }

        #[inline(always)]
        fn from_dst(self, dst: c32) -> c32 {
            dst
        }

        #[inline(always)]
        fn into_dst(self, acc: c32) -> c32 {
            acc
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            unsafe {
                let ab = cast(lhs);
                let xy = cast(rhs);

                let yx = _mm256_permute_ps::<0b10_11_00_01>(xy);
                let aa = _mm256_moveldup_ps(ab);
                let bb = _mm256_movehdup_ps(ab);

                cast(_mm256_fmaddsub_ps(
                    aa,
                    xy,
                    _mm256_fmaddsub_ps(bb, yx, cast(acc)),
                ))
            }
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            lhs
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            rhs
        }

        #[inline(always)]
        fn simd_splat(self, lhs: c32) -> Self::AccN {
            cast(self.avx._mm256_set1_pd(cast(lhs)))
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            dst
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            acc
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: c32, rhs: c32) -> c32 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe {
                let ab = cast(lhs);
                let xy = cast(rhs);

                let yx = _mm256_permute_ps::<0b10_11_00_01>(xy);
                let aa = _mm256_moveldup_ps(ab);
                let bb = _mm256_movehdup_ps(ab);

                cast(_mm256_fmaddsub_ps(aa, xy, _mm256_mul_ps(bb, yx)))
            }
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx._mm256_add_ps(cast(lhs), cast(rhs)))
        }
    }

    unsafe impl MixedSimd<c64, c64, c64, c64> for V3 {
        const SIMD_WIDTH: usize = 2;

        type LhsN = [c64; 2];
        type RhsN = [c64; 2];
        type DstN = [c64; 2];
        type AccN = [c64; 2];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: c64, rhs: c64) -> c64 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: c64, rhs: c64, acc: c64) -> c64 {
            lhs * rhs + acc
        }

        #[inline(always)]
        fn from_lhs(self, lhs: c64) -> c64 {
            lhs
        }

        #[inline(always)]
        fn from_rhs(self, rhs: c64) -> c64 {
            rhs
        }

        #[inline(always)]
        fn from_dst(self, dst: c64) -> c64 {
            dst
        }

        #[inline(always)]
        fn into_dst(self, acc: c64) -> c64 {
            acc
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            unsafe {
                let ab = cast(lhs);
                let xy = cast(rhs);

                let yx = _mm256_permute_pd::<0b0101>(xy);
                let aa = _mm256_unpacklo_pd(ab, ab);
                let bb = _mm256_unpackhi_pd(ab, ab);

                cast(_mm256_fmaddsub_pd(
                    aa,
                    xy,
                    _mm256_fmaddsub_pd(bb, yx, cast(acc)),
                ))
            }
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            lhs
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            rhs
        }

        #[inline(always)]
        fn simd_splat(self, lhs: c64) -> Self::AccN {
            cast([lhs; 2])
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            dst
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            acc
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: c64, rhs: c64) -> c64 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe {
                let ab = cast(lhs);
                let xy = cast(rhs);

                let yx = _mm256_permute_pd::<0b0101>(xy);
                let aa = _mm256_unpacklo_pd(ab, ab);
                let bb = _mm256_unpackhi_pd(ab, ab);

                cast(_mm256_fmaddsub_pd(aa, xy, _mm256_mul_pd(bb, yx)))
            }
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx._mm256_add_pd(cast(lhs), cast(rhs)))
        }
    }

    #[cfg(feature = "nightly")]
    unsafe impl MixedSimd<c32, c32, c32, c32> for V4 {
        const SIMD_WIDTH: usize = 8;

        type LhsN = [c32; 8];
        type RhsN = [c32; 8];
        type DstN = [c32; 8];
        type AccN = [c32; 8];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: c32, rhs: c32) -> c32 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: c32, rhs: c32, acc: c32) -> c32 {
            lhs * rhs + acc
        }

        #[inline(always)]
        fn from_lhs(self, lhs: c32) -> c32 {
            lhs
        }

        #[inline(always)]
        fn from_rhs(self, rhs: c32) -> c32 {
            rhs
        }

        #[inline(always)]
        fn from_dst(self, dst: c32) -> c32 {
            dst
        }

        #[inline(always)]
        fn into_dst(self, acc: c32) -> c32 {
            acc
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            unsafe {
                let ab = cast(lhs);
                let xy = cast(rhs);

                let yx = _mm512_permute_ps::<0b10_11_00_01>(xy);
                let aa = _mm512_moveldup_ps(ab);
                let bb = _mm512_movehdup_ps(ab);

                cast(_mm512_fmaddsub_ps(
                    aa,
                    xy,
                    _mm512_fmaddsub_ps(bb, yx, cast(acc)),
                ))
            }
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            lhs
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            rhs
        }

        #[inline(always)]
        fn simd_splat(self, lhs: c32) -> Self::AccN {
            cast(self.avx512f._mm512_set1_pd(cast(lhs)))
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            dst
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            acc
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: c32, rhs: c32) -> c32 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe {
                let ab = cast(lhs);
                let xy = cast(rhs);

                let yx = _mm512_permute_ps::<0b10_11_00_01>(xy);
                let aa = _mm512_moveldup_ps(ab);
                let bb = _mm512_movehdup_ps(ab);

                cast(_mm512_fmaddsub_ps(aa, xy, _mm512_mul_ps(bb, yx)))
            }
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx512f._mm512_add_ps(cast(lhs), cast(rhs)))
        }
    }

    #[cfg(feature = "nightly")]
    unsafe impl MixedSimd<c64, c64, c64, c64> for V4 {
        const SIMD_WIDTH: usize = 4;

        type LhsN = [c64; 4];
        type RhsN = [c64; 4];
        type DstN = [c64; 4];
        type AccN = [c64; 4];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: c64, rhs: c64) -> c64 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: c64, rhs: c64, acc: c64) -> c64 {
            lhs * rhs + acc
        }

        #[inline(always)]
        fn from_lhs(self, lhs: c64) -> c64 {
            lhs
        }

        #[inline(always)]
        fn from_rhs(self, rhs: c64) -> c64 {
            rhs
        }

        #[inline(always)]
        fn from_dst(self, dst: c64) -> c64 {
            dst
        }

        #[inline(always)]
        fn into_dst(self, acc: c64) -> c64 {
            acc
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            unsafe {
                let ab = cast(lhs);
                let xy = cast(rhs);

                let yx = _mm512_permute_pd::<0b01010101>(xy);
                let aa = _mm512_unpacklo_pd(ab, ab);
                let bb = _mm512_unpackhi_pd(ab, ab);

                cast(_mm512_fmaddsub_pd(
                    aa,
                    xy,
                    _mm512_fmaddsub_pd(bb, yx, cast(acc)),
                ))
            }
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            lhs
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            rhs
        }

        #[inline(always)]
        fn simd_splat(self, lhs: c64) -> Self::AccN {
            cast([lhs; 4])
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            dst
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            acc
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: c64, rhs: c64) -> c64 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe {
                let ab = cast(lhs);
                let xy = cast(rhs);

                let yx = _mm512_permute_pd::<0b01010101>(xy);
                let aa = _mm512_unpacklo_pd(ab, ab);
                let bb = _mm512_unpackhi_pd(ab, ab);

                cast(_mm512_fmaddsub_pd(aa, xy, _mm512_mul_pd(bb, yx)))
            }
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx512f._mm512_add_pd(cast(lhs), cast(rhs)))
        }
    }

    unsafe impl MixedSimd<f32, f32, f32, f32> for V3 {
        const SIMD_WIDTH: usize = 8;

        type LhsN = [f32; 8];
        type RhsN = [f32; 8];
        type DstN = [f32; 8];
        type AccN = [f32; 8];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: f32, rhs: f32) -> f32 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: f32, rhs: f32, acc: f32) -> f32 {
            unsafe { v3_fmaf(lhs, rhs, acc) }
        }

        #[inline(always)]
        fn from_lhs(self, lhs: f32) -> f32 {
            lhs
        }

        #[inline(always)]
        fn from_rhs(self, rhs: f32) -> f32 {
            rhs
        }

        #[inline(always)]
        fn from_dst(self, dst: f32) -> f32 {
            dst
        }

        #[inline(always)]
        fn into_dst(self, acc: f32) -> f32 {
            acc
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            cast(self.fma._mm256_fmadd_ps(cast(lhs), cast(rhs), cast(acc)))
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            lhs
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            rhs
        }

        #[inline(always)]
        fn simd_splat(self, lhs: f32) -> Self::AccN {
            cast(self.avx._mm256_set1_ps(lhs))
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            dst
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            acc
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: f32, rhs: f32) -> f32 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx._mm256_mul_ps(cast(lhs), cast(rhs)))
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx._mm256_add_ps(cast(lhs), cast(rhs)))
        }
    }

    unsafe impl MixedSimd<f64, f64, f64, f64> for V3 {
        const SIMD_WIDTH: usize = 4;

        type LhsN = [f64; 4];
        type RhsN = [f64; 4];
        type DstN = [f64; 4];
        type AccN = [f64; 4];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: f64, rhs: f64) -> f64 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: f64, rhs: f64, acc: f64) -> f64 {
            unsafe { v3_fma(lhs, rhs, acc) }
        }

        #[inline(always)]
        fn from_lhs(self, lhs: f64) -> f64 {
            lhs
        }

        #[inline(always)]
        fn from_rhs(self, rhs: f64) -> f64 {
            rhs
        }

        #[inline(always)]
        fn from_dst(self, dst: f64) -> f64 {
            dst
        }

        #[inline(always)]
        fn into_dst(self, acc: f64) -> f64 {
            acc
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            cast(self.fma._mm256_fmadd_pd(cast(lhs), cast(rhs), cast(acc)))
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            lhs
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            rhs
        }

        #[inline(always)]
        fn simd_splat(self, lhs: f64) -> Self::AccN {
            cast(self.avx._mm256_set1_pd(lhs))
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            dst
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            acc
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: f64, rhs: f64) -> f64 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx._mm256_mul_pd(cast(lhs), cast(rhs)))
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx._mm256_add_pd(cast(lhs), cast(rhs)))
        }
    }

    impl Simd for V3 {
        #[inline(always)]
        unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
            Self::new_unchecked().vectorize(f)
        }
    }

    #[cfg(feature = "nightly")]
    impl Simd for V4 {
        #[inline(always)]
        unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
            Self::new_unchecked().vectorize(f)
        }
    }

    #[cfg(feature = "nightly")]
    #[cfg(feature = "f16")]
    unsafe impl MixedSimd<f16, f16, f16, f32> for V4 {
        const SIMD_WIDTH: usize = 16;

        type LhsN = [f16; 16];
        type RhsN = [f16; 16];
        type DstN = [f16; 16];
        type AccN = [f32; 16];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: f32, rhs: f32) -> f32 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: f32, rhs: f32, acc: f32) -> f32 {
            unsafe { v3_fmaf(lhs, rhs, acc) }
        }

        #[inline(always)]
        fn from_lhs(self, lhs: f16) -> f32 {
            unsafe { pulp::cast_lossy(_mm_cvtph_ps(self.sse2._mm_set1_epi16(cast(lhs)))) }
        }

        #[inline(always)]
        fn from_rhs(self, rhs: f16) -> f32 {
            unsafe { pulp::cast_lossy(_mm_cvtph_ps(self.sse2._mm_set1_epi16(cast(rhs)))) }
        }

        #[inline(always)]
        fn from_dst(self, dst: f16) -> f32 {
            unsafe { pulp::cast_lossy(_mm_cvtph_ps(self.sse2._mm_set1_epi16(cast(dst)))) }
        }

        #[inline(always)]
        fn into_dst(self, acc: f32) -> f16 {
            unsafe {
                pulp::cast_lossy(_mm_cvtps_ph::<_MM_FROUND_CUR_DIRECTION>(
                    self.sse._mm_load_ss(&acc),
                ))
            }
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            cast(
                self.avx512f
                    ._mm512_fmadd_ps(cast(lhs), cast(rhs), cast(acc)),
            )
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            unsafe { cast(_mm512_cvtph_ps(cast(lhs))) }
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            unsafe { cast(_mm512_cvtph_ps(cast(rhs))) }
        }

        #[inline(always)]
        fn simd_splat(self, lhs: f32) -> Self::AccN {
            cast(self.avx512f._mm512_set1_ps(lhs))
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            unsafe { cast(_mm512_cvtph_ps(cast(dst))) }
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            unsafe { cast(_mm512_cvtps_ph::<_MM_FROUND_CUR_DIRECTION>(cast(acc))) }
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: f32, rhs: f32) -> f32 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx512f._mm512_mul_ps(cast(lhs), cast(rhs)))
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx512f._mm512_add_ps(cast(lhs), cast(rhs)))
        }
    }

    #[cfg(feature = "nightly")]
    unsafe impl MixedSimd<f32, f32, f32, f32> for V4 {
        const SIMD_WIDTH: usize = 16;

        type LhsN = [f32; 16];
        type RhsN = [f32; 16];
        type DstN = [f32; 16];
        type AccN = [f32; 16];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: f32, rhs: f32) -> f32 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: f32, rhs: f32, acc: f32) -> f32 {
            unsafe { v3_fmaf(lhs, rhs, acc) }
        }

        #[inline(always)]
        fn from_lhs(self, lhs: f32) -> f32 {
            lhs
        }

        #[inline(always)]
        fn from_rhs(self, rhs: f32) -> f32 {
            rhs
        }

        #[inline(always)]
        fn from_dst(self, dst: f32) -> f32 {
            dst
        }

        #[inline(always)]
        fn into_dst(self, acc: f32) -> f32 {
            acc
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            cast(
                self.avx512f
                    ._mm512_fmadd_ps(cast(lhs), cast(rhs), cast(acc)),
            )
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            lhs
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            rhs
        }

        #[inline(always)]
        fn simd_splat(self, lhs: f32) -> Self::AccN {
            cast(self.avx512f._mm512_set1_ps(lhs))
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            dst
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            acc
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: f32, rhs: f32) -> f32 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx512f._mm512_mul_ps(cast(lhs), cast(rhs)))
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx512f._mm512_add_ps(cast(lhs), cast(rhs)))
        }
    }

    #[cfg(feature = "nightly")]
    unsafe impl MixedSimd<f64, f64, f64, f64> for V4 {
        const SIMD_WIDTH: usize = 8;

        type LhsN = [f64; 8];
        type RhsN = [f64; 8];
        type DstN = [f64; 8];
        type AccN = [f64; 8];

        #[inline]
        fn try_new() -> Option<Self> {
            Self::try_new()
        }

        #[inline(always)]
        fn mult(self, lhs: f64, rhs: f64) -> f64 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: f64, rhs: f64, acc: f64) -> f64 {
            unsafe { v3_fma(lhs, rhs, acc) }
        }

        #[inline(always)]
        fn from_lhs(self, lhs: f64) -> f64 {
            lhs
        }

        #[inline(always)]
        fn from_rhs(self, rhs: f64) -> f64 {
            rhs
        }

        #[inline(always)]
        fn from_dst(self, dst: f64) -> f64 {
            dst
        }

        #[inline(always)]
        fn into_dst(self, acc: f64) -> f64 {
            acc
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            cast(
                self.avx512f
                    ._mm512_fmadd_pd(cast(lhs), cast(rhs), cast(acc)),
            )
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            lhs
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            rhs
        }

        #[inline(always)]
        fn simd_splat(self, lhs: f64) -> Self::AccN {
            cast(self.avx512f._mm512_set1_pd(lhs))
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            dst
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            acc
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            self.vectorize(f)
        }

        #[inline(always)]
        fn add(self, lhs: f64, rhs: f64) -> f64 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx512f._mm512_mul_pd(cast(lhs), cast(rhs)))
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            cast(self.avx512f._mm512_add_pd(cast(lhs), cast(rhs)))
        }
    }
}

#[cfg(target_arch = "aarch64")]
pub mod aarch64 {
    use super::*;
    use core::arch::aarch64::*;
    use core::arch::asm;
    #[allow(unused_imports)]
    use core::mem::transmute;
    use core::mem::MaybeUninit;
    use core::ptr;

    #[inline(always)]
    pub unsafe fn neon_fmaf(a: f32, b: f32, c: f32) -> f32 {
        #[cfg(feature = "std")]
        {
            f32::mul_add(a, b, c)
        }
        #[cfg(not(feature = "std"))]
        {
            a * b + c
        }
    }

    #[inline(always)]
    pub unsafe fn neon_fma(a: f64, b: f64, c: f64) -> f64 {
        #[cfg(feature = "std")]
        {
            f64::mul_add(a, b, c)
        }
        #[cfg(not(feature = "std"))]
        {
            a * b + c
        }
    }

    #[target_feature(enable = "fp16,neon")]
    #[inline]
    pub unsafe fn f16_to_f32_fp16(i: u16) -> f32 {
        let result: f32;
        asm!(
        "fcvt {0:s}, {1:h}",
        out(vreg) result,
        in(vreg) i,
        options(pure, nomem, nostack));
        result
    }

    #[target_feature(enable = "fp16,neon")]
    #[inline]
    pub unsafe fn f32_to_f16_fp16(f: f32) -> u16 {
        let result: u16;
        asm!(
        "fcvt {0:h}, {1:s}",
        out(vreg) result,
        in(vreg) f,
        options(pure, nomem, nostack));
        result
    }

    #[target_feature(enable = "fp16,neon")]
    #[inline]
    pub unsafe fn f16x4_to_f32x4_fp16(v: &[u16; 4]) -> [f32; 4] {
        let mut vec = MaybeUninit::<uint16x4_t>::uninit();
        ptr::copy_nonoverlapping(v.as_ptr(), vec.as_mut_ptr().cast(), 4);
        let result: float32x4_t;
        asm!(
        "fcvtl {0:v}.4s, {1:v}.4h",
        out(vreg) result,
        in(vreg) vec.assume_init(),
        options(pure, nomem, nostack));
        *(&result as *const float32x4_t).cast()
    }

    #[target_feature(enable = "fp16,neon")]
    #[inline]
    pub unsafe fn f32x4_to_f16x4_fp16(v: &[f32; 4]) -> [u16; 4] {
        let mut vec = MaybeUninit::<float32x4_t>::uninit();
        ptr::copy_nonoverlapping(v.as_ptr(), vec.as_mut_ptr().cast(), 4);
        let result: uint16x4_t;
        asm!(
        "fcvtn {0:v}.4h, {1:v}.4s",
        out(vreg) result,
        in(vreg) vec.assume_init(),
        options(pure, nomem, nostack));
        *(&result as *const uint16x4_t).cast()
    }

    #[target_feature(enable = "fp16")]
    #[inline]
    pub unsafe fn add_f16_fp16(a: u16, b: u16) -> u16 {
        let result: u16;
        asm!(
        "fadd {0:h}, {1:h}, {2:h}",
        out(vreg) result,
        in(vreg) a,
        in(vreg) b,
        options(pure, nomem, nostack));
        result
    }

    #[target_feature(enable = "fp16")]
    #[inline]
    pub unsafe fn fmaq_f16(mut a: u16, b: u16, c: u16) -> u16 {
        asm!(
        "fmadd {0:h}, {1:h}, {2:h}, {0:h}",
        inout(vreg) a,
        in(vreg) b,
        in(vreg) c,
        options(pure, nomem, nostack));
        a
    }

    #[target_feature(enable = "fp16")]
    #[inline]
    pub unsafe fn multiply_f16_fp16(a: u16, b: u16) -> u16 {
        let result: u16;
        asm!(
        "fmul {0:h}, {1:h}, {2:h}",
        out(vreg) result,
        in(vreg) a,
        in(vreg) b,
        options(pure, nomem, nostack));
        result
    }

    #[allow(non_camel_case_types)]
    type float16x8_t = uint16x8_t;

    /// Floating point multiplication
    /// [doc](https://developer.arm.com/documentation/dui0801/g/A64-SIMD-Vector-Instructions/FMUL--vector-)
    #[inline]
    pub unsafe fn vmulq_f16(a: float16x8_t, b: float16x8_t) -> float16x8_t {
        let result: float16x8_t;
        asm!(
                "fmul {0:v}.8h, {1:v}.8h, {2:v}.8h",
                out(vreg) result,
                in(vreg) a,
                in(vreg) b,
                options(pure, nomem, nostack));
        result
    }

    /// Floating point addition
    /// [doc](https://developer.arm.com/documentation/dui0801/g/A64-SIMD-Vector-Instructions/FADD--vector-)
    #[inline]
    pub unsafe fn vaddq_f16(a: float16x8_t, b: float16x8_t) -> float16x8_t {
        let result: float16x8_t;
        asm!(
                "fadd {0:v}.8h, {1:v}.8h, {2:v}.8h",
                out(vreg) result,
                in(vreg) a,
                in(vreg) b,
                options(pure, nomem, nostack));
        result
    }

    /// Fused multiply add [doc](https://developer.arm.com/documentation/dui0801/g/A64-SIMD-Vector-Instructions/FMLA--vector-)
    #[inline]
    pub unsafe fn vfmaq_f16(mut a: float16x8_t, b: float16x8_t, c: float16x8_t) -> float16x8_t {
        asm!(
                "fmla {0:v}.8h, {1:v}.8h, {2:v}.8h",
                inout(vreg) a,
                in(vreg) b,
                in(vreg) c,
                options(pure, nomem, nostack));
        a
    }

    #[inline]
    pub unsafe fn vfmaq_laneq_f16<const LANE: i32>(
        mut a: float16x8_t,
        b: float16x8_t,
        c: float16x8_t,
    ) -> float16x8_t {
        match LANE {
            0 => asm!(
                "fmla {0:v}.8h, {1:v}.8h, {2:v}.h[0]",
                inout(vreg) a,
                in(vreg) b,
                in(vreg_low16) c,
                options(pure, nomem, nostack)),
            1 => asm!(
                "fmla {0:v}.8h, {1:v}.8h, {2:v}.h[1]",
                inout(vreg) a,
                in(vreg) b,
                in(vreg_low16) c,
                options(pure, nomem, nostack)),
            2 => asm!(
                "fmla {0:v}.8h, {1:v}.8h, {2:v}.h[2]",
                inout(vreg) a,
                in(vreg) b,
                in(vreg_low16) c,
                options(pure, nomem, nostack)),
            3 => asm!(
                "fmla {0:v}.8h, {1:v}.8h, {2:v}.h[3]",
                inout(vreg) a,
                in(vreg) b,
                in(vreg_low16) c,
                options(pure, nomem, nostack)),
            4 => asm!(
                "fmla {0:v}.8h, {1:v}.8h, {2:v}.h[4]",
                inout(vreg) a,
                in(vreg) b,
                in(vreg_low16) c,
                options(pure, nomem, nostack)),
            5 => asm!(
                "fmla {0:v}.8h, {1:v}.8h, {2:v}.h[5]",
                inout(vreg) a,
                in(vreg) b,
                in(vreg_low16) c,
                options(pure, nomem, nostack)),
            6 => asm!(
                "fmla {0:v}.8h, {1:v}.8h, {2:v}.h[6]",
                inout(vreg) a,
                in(vreg) b,
                in(vreg_low16) c,
                options(pure, nomem, nostack)),
            7 => asm!(
                "fmla {0:v}.8h, {1:v}.8h, {2:v}.h[7]",
                inout(vreg) a,
                in(vreg) b,
                in(vreg_low16) c,
                options(pure, nomem, nostack)),
            _ => unreachable!(),
        }
        a
    }

    #[derive(Copy, Clone, Debug)]
    pub struct Neon {
        __private: (),
    }

    #[derive(Copy, Clone, Debug)]
    pub struct NeonFp16 {
        __private: (),
    }

    #[derive(Copy, Clone, Debug)]
    pub struct NeonFcma {
        __private: (),
    }

    impl Simd for Neon {
        #[inline]
        #[target_feature(enable = "neon")]
        unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
            f.call()
        }
    }

    impl Simd for NeonFp16 {
        #[inline]
        #[target_feature(enable = "neon,fp16")]
        unsafe fn vectorize<F: NullaryFnOnce>(f: F) -> F::Output {
            f.call()
        }
    }

    #[cfg(feature = "f16")]
    unsafe impl MixedSimd<f16, f16, f16, f32> for NeonFp16 {
        const SIMD_WIDTH: usize = 4;

        type LhsN = [f16; 4];
        type RhsN = [f16; 4];
        type DstN = [f16; 4];
        type AccN = [f32; 4];

        #[inline]
        fn try_new() -> Option<Self> {
            if crate::feature_detected!("neon") && crate::feature_detected!("fp16") {
                Some(Self { __private: () })
            } else {
                None
            }
        }

        #[inline(always)]
        fn mult(self, lhs: f32, rhs: f32) -> f32 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: f32, rhs: f32, acc: f32) -> f32 {
            unsafe { neon_fmaf(lhs, rhs, acc) }
        }

        #[inline(always)]
        fn from_lhs(self, lhs: f16) -> f32 {
            unsafe { f16_to_f32_fp16(cast(lhs)) }
        }

        #[inline(always)]
        fn from_rhs(self, rhs: f16) -> f32 {
            unsafe { f16_to_f32_fp16(cast(rhs)) }
        }

        #[inline(always)]
        fn from_dst(self, dst: f16) -> f32 {
            unsafe { f16_to_f32_fp16(cast(dst)) }
        }

        #[inline(always)]
        fn into_dst(self, acc: f32) -> f16 {
            unsafe { cast(f32_to_f16_fp16(acc)) }
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            unsafe { transmute(vfmaq_f32(transmute(acc), transmute(lhs), transmute(rhs))) }
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            unsafe { f16x4_to_f32x4_fp16(&cast(lhs)) }
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            unsafe { f16x4_to_f32x4_fp16(&cast(rhs)) }
        }

        #[inline(always)]
        fn simd_splat(self, lhs: f32) -> Self::AccN {
            [lhs, lhs, lhs, lhs]
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            unsafe { f16x4_to_f32x4_fp16(&cast(dst)) }
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            unsafe { cast(f32x4_to_f16x4_fp16(&acc)) }
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            #[inline]
            #[target_feature(enable = "neon,fp16")]
            unsafe fn implementation<F: NullaryFnOnce>(f: F) -> F::Output {
                f.call()
            }

            unsafe { implementation(f) }
        }

        #[inline(always)]
        fn add(self, lhs: f32, rhs: f32) -> f32 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe { transmute(vmulq_f32(transmute(lhs), transmute(rhs))) }
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe { transmute(vaddq_f32(transmute(lhs), transmute(rhs))) }
        }
    }

    #[cfg(feature = "f16")]
    unsafe impl MixedSimd<f16, f16, f16, f16> for NeonFp16 {
        const SIMD_WIDTH: usize = 8;

        type LhsN = [f16; 8];
        type RhsN = [f16; 8];
        type DstN = [f16; 8];
        type AccN = [f16; 8];

        #[inline]
        fn try_new() -> Option<Self> {
            if crate::feature_detected!("neon") && crate::feature_detected!("fp16") {
                Some(Self { __private: () })
            } else {
                None
            }
        }

        #[inline(always)]
        fn mult(self, lhs: f16, rhs: f16) -> f16 {
            unsafe { cast(multiply_f16_fp16(cast(lhs), cast(rhs))) }
        }

        #[inline(always)]
        fn mult_add(self, lhs: f16, rhs: f16, acc: f16) -> f16 {
            unsafe { cast(fmaq_f16(cast(acc), cast(lhs), cast(rhs))) }
        }

        #[inline(always)]
        fn from_lhs(self, lhs: f16) -> f16 {
            lhs
        }

        #[inline(always)]
        fn from_rhs(self, rhs: f16) -> f16 {
            rhs
        }

        #[inline(always)]
        fn from_dst(self, dst: f16) -> f16 {
            dst
        }

        #[inline(always)]
        fn into_dst(self, acc: f16) -> f16 {
            acc
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            unsafe { transmute(vfmaq_f16(transmute(acc), transmute(lhs), transmute(rhs))) }
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            lhs
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            rhs
        }

        #[inline(always)]
        fn simd_splat(self, lhs: f16) -> Self::AccN {
            [lhs, lhs, lhs, lhs, lhs, lhs, lhs, lhs]
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            dst
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            acc
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            #[inline]
            #[target_feature(enable = "neon,fp16")]
            unsafe fn implementation<F: NullaryFnOnce>(f: F) -> F::Output {
                f.call()
            }

            unsafe { implementation(f) }
        }

        #[inline(always)]
        fn add(self, lhs: f16, rhs: f16) -> f16 {
            unsafe { cast(add_f16_fp16(cast(lhs), cast(rhs))) }
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe { transmute(vmulq_f16(transmute(lhs), transmute(rhs))) }
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe { transmute(vaddq_f16(transmute(lhs), transmute(rhs))) }
        }
    }

    #[cfg(feature = "f16")]
    unsafe impl MixedSimd<f16, f16, f16, f32> for Neon {
        const SIMD_WIDTH: usize = 4;

        type LhsN = [f16; 4];
        type RhsN = [f16; 4];
        type DstN = [f16; 4];
        type AccN = [f32; 4];

        #[inline]
        fn try_new() -> Option<Self> {
            if crate::feature_detected!("neon") {
                Some(Self { __private: () })
            } else {
                None
            }
        }

        #[inline(always)]
        fn mult(self, lhs: f32, rhs: f32) -> f32 {
            lhs * rhs
        }

        #[inline(always)]
        fn mult_add(self, lhs: f32, rhs: f32, acc: f32) -> f32 {
            unsafe { neon_fmaf(lhs, rhs, acc) }
        }

        #[inline(always)]
        fn from_lhs(self, lhs: f16) -> f32 {
            lhs.into()
        }

        #[inline(always)]
        fn from_rhs(self, rhs: f16) -> f32 {
            rhs.into()
        }

        #[inline(always)]
        fn from_dst(self, dst: f16) -> f32 {
            dst.into()
        }

        #[inline(always)]
        fn into_dst(self, acc: f32) -> f16 {
            f16::from_f32(acc)
        }

        #[inline(always)]
        fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN {
            unsafe { transmute(vfmaq_f32(transmute(acc), transmute(lhs), transmute(rhs))) }
        }

        #[inline(always)]
        fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN {
            [lhs[0].into(), lhs[1].into(), lhs[2].into(), lhs[3].into()]
        }

        #[inline(always)]
        fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN {
            [rhs[0].into(), rhs[1].into(), rhs[2].into(), rhs[3].into()]
        }

        #[inline(always)]
        fn simd_splat(self, lhs: f32) -> Self::AccN {
            [lhs, lhs, lhs, lhs]
        }

        #[inline(always)]
        fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN {
            [dst[0].into(), dst[1].into(), dst[2].into(), dst[3].into()]
        }

        #[inline(always)]
        fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN {
            [
                f16::from_f32(acc[0]),
                f16::from_f32(acc[1]),
                f16::from_f32(acc[2]),
                f16::from_f32(acc[3]),
            ]
        }

        #[inline(always)]
        fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output {
            #[inline]
            #[target_feature(enable = "neon")]
            unsafe fn implementation<F: NullaryFnOnce>(f: F) -> F::Output {
                f.call()
            }

            unsafe { implementation(f) }
        }

        #[inline(always)]
        fn add(self, lhs: f32, rhs: f32) -> f32 {
            lhs + rhs
        }

        #[inline(always)]
        fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe { transmute(vmulq_f32(transmute(lhs), transmute(rhs))) }
        }

        #[inline(always)]
        fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN {
            unsafe { transmute(vaddq_f32(transmute(lhs), transmute(rhs))) }
        }
    }
}

pub trait Boilerplate: Copy + Send + Sync + core::fmt::Debug + 'static + PartialEq {}
impl<T: Copy + Send + Sync + core::fmt::Debug + PartialEq + 'static> Boilerplate for T {}

pub unsafe trait MixedSimd<Lhs, Rhs, Dst, Acc>: Simd {
    const SIMD_WIDTH: usize;

    type LhsN: Boilerplate;
    type RhsN: Boilerplate;
    type DstN: Boilerplate;
    type AccN: Boilerplate;

    fn try_new() -> Option<Self>;

    fn vectorize<F: NullaryFnOnce>(self, f: F) -> F::Output;

    fn add(self, lhs: Acc, rhs: Acc) -> Acc;
    fn mult(self, lhs: Acc, rhs: Acc) -> Acc;
    fn mult_add(self, lhs: Acc, rhs: Acc, acc: Acc) -> Acc;
    fn from_lhs(self, lhs: Lhs) -> Acc;
    fn from_rhs(self, rhs: Rhs) -> Acc;
    fn from_dst(self, dst: Dst) -> Acc;
    fn into_dst(self, acc: Acc) -> Dst;

    fn simd_mul(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN;
    fn simd_add(self, lhs: Self::AccN, rhs: Self::AccN) -> Self::AccN;
    fn simd_mult_add(self, lhs: Self::AccN, rhs: Self::AccN, acc: Self::AccN) -> Self::AccN;
    fn simd_from_lhs(self, lhs: Self::LhsN) -> Self::AccN;
    fn simd_from_rhs(self, rhs: Self::RhsN) -> Self::AccN;
    fn simd_splat(self, lhs: Acc) -> Self::AccN;

    fn simd_from_dst(self, dst: Self::DstN) -> Self::AccN;
    fn simd_into_dst(self, acc: Self::AccN) -> Self::DstN;
}
