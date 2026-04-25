pub mod scalar {
    pub mod f32 {
        type T = f32;
        const N: usize = 1;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            [value]
        }

        #[inline(always)]
        unsafe fn mul(lhs: Pack, rhs: Pack) -> Pack {
            [lhs[0] * rhs[0]]
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            [lhs[0] + rhs[0]]
        }

        #[inline(always)]
        unsafe fn mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            add(mul(a, b), c)
        }

        #[inline(always)]
        pub unsafe fn scalar_mul(lhs: T, rhs: T) -> T {
            lhs * rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_add(lhs: T, rhs: T) -> T {
            lhs + rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_mul_add(a: T, b: T, c: T) -> T {
            a * b + c
        }

        microkernel!(, 2, x1x1, 1, 1);
        microkernel!(, 2, x1x2, 1, 2);
        microkernel!(, 2, x1x3, 1, 3);
        microkernel!(, 2, x1x4, 1, 4);

        microkernel!(, 2, x2x1, 2, 1);
        microkernel!(, 2, x2x2, 2, 2);
        microkernel!(, 2, x2x3, 2, 3);
        microkernel!(, 2, x2x4, 2, 4);

        microkernel_fn_array! {
            [x1x1, x1x2, x1x3, x1x4,],
            [x2x1, x2x2, x2x3, x2x4,],
        }
    }
}

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
pub mod fma {
    pub mod f32 {
        #[cfg(target_arch = "x86")]
        use core::arch::x86::*;
        #[cfg(target_arch = "x86_64")]
        use core::arch::x86_64::*;
        use core::mem::transmute;

        type T = f32;
        const N: usize = 8;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            transmute(_mm256_set1_ps(value))
        }

        #[inline(always)]
        unsafe fn mul(lhs: Pack, rhs: Pack) -> Pack {
            transmute(_mm256_mul_ps(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(_mm256_add_ps(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        unsafe fn mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            transmute(_mm256_fmadd_ps(transmute(a), transmute(b), transmute(c)))
        }

        #[inline(always)]
        pub unsafe fn scalar_mul(lhs: T, rhs: T) -> T {
            lhs * rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_add(lhs: T, rhs: T) -> T {
            lhs + rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_mul_add(a: T, b: T, c: T) -> T {
            gemm_common::simd::v3_fmaf(a, b, c)
        }

        microkernel!(["fma"], 2, x1x1, 1, 1);
        microkernel!(["fma"], 2, x1x2, 1, 2);
        microkernel!(["fma"], 2, x1x3, 1, 3);
        microkernel!(["fma"], 2, x1x4, 1, 4);

        microkernel!(["fma"], 2, x2x1, 2, 1);
        microkernel!(["fma"], 2, x2x2, 2, 2);
        microkernel!(["fma"], 2, x2x3, 2, 3);
        microkernel!(["fma"], 2, x2x4, 2, 4);

        microkernel!(["fma"], 2, x3x1, 3, 1);
        microkernel!(["fma"], 2, x3x2, 3, 2);
        microkernel!(["fma"], 2, x3x3, 3, 3);
        microkernel!(["fma"], 2, x3x4, 3, 4);

        microkernel_fn_array! {
            [x1x1, x1x2, x1x3, x1x4,],
            [x2x1, x2x2, x2x3, x2x4,],
            [x3x1, x3x2, x3x3, x3x4,],
        }
    }
}

#[cfg(all(feature = "nightly", any(target_arch = "x86", target_arch = "x86_64")))]
pub mod avx512f {
    pub mod f32 {
        #[cfg(target_arch = "x86")]
        use core::arch::x86::*;
        #[cfg(target_arch = "x86_64")]
        use core::arch::x86_64::*;
        use core::mem::transmute;

        type T = f32;
        const N: usize = 16;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            transmute(_mm512_set1_ps(value))
        }

        #[inline(always)]
        unsafe fn mul(lhs: Pack, rhs: Pack) -> Pack {
            transmute(_mm512_mul_ps(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(_mm512_add_ps(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        unsafe fn mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            transmute(_mm512_fmadd_ps(transmute(a), transmute(b), transmute(c)))
        }

        #[inline(always)]
        pub unsafe fn scalar_mul(lhs: T, rhs: T) -> T {
            lhs * rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_add(lhs: T, rhs: T) -> T {
            lhs + rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_mul_add(a: T, b: T, c: T) -> T {
            gemm_common::simd::v3_fmaf(a, b, c)
        }

        microkernel!(["avx512f"], 4, x1x1, 1, 1);
        microkernel!(["avx512f"], 4, x1x2, 1, 2);
        microkernel!(["avx512f"], 4, x1x3, 1, 3);
        microkernel!(["avx512f"], 4, x1x4, 1, 4);
        microkernel!(["avx512f"], 4, x1x5, 1, 5);
        microkernel!(["avx512f"], 4, x1x6, 1, 6);
        microkernel!(["avx512f"], 4, x1x7, 1, 7);
        microkernel!(["avx512f"], 4, x1x8, 1, 8);

        microkernel!(["avx512f"], 4, x2x1, 2, 1);
        microkernel!(["avx512f"], 4, x2x2, 2, 2);
        microkernel!(["avx512f"], 4, x2x3, 2, 3);
        microkernel!(["avx512f"], 4, x2x4, 2, 4);
        microkernel!(["avx512f"], 4, x2x5, 2, 5);
        microkernel!(["avx512f"], 4, x2x6, 2, 6);
        microkernel!(["avx512f"], 4, x2x7, 2, 7);
        microkernel!(["avx512f"], 4, x2x8, 2, 8);

        microkernel!(["avx512f"], 4, x3x1, 3, 1);
        microkernel!(["avx512f"], 4, x3x2, 3, 2);
        microkernel!(["avx512f"], 4, x3x3, 3, 3);
        microkernel!(["avx512f"], 4, x3x4, 3, 4);
        microkernel!(["avx512f"], 4, x3x5, 3, 5);
        microkernel!(["avx512f"], 4, x3x6, 3, 6);
        microkernel!(["avx512f"], 4, x3x7, 3, 7);
        microkernel!(["avx512f"], 4, x3x8, 3, 8);

        microkernel_fn_array! {
            [x1x1, x1x2, x1x3, x1x4, x1x5, x1x6, x1x7, x1x8,],
            [x2x1, x2x2, x2x3, x2x4, x2x5, x2x6, x2x7, x2x8,],
            [x3x1, x3x2, x3x3, x3x4, x3x5, x3x6, x3x7, x3x8,],
        }
    }
}

#[allow(dead_code)]
mod v128_common {
    pub mod f32 {
        pub type T = f32;
        pub const N: usize = 4;
        pub type Pack = [T; N];

        #[inline(always)]
        pub unsafe fn splat(value: T) -> Pack {
            [value, value, value, value]
        }
    }
}

#[cfg(target_arch = "aarch64")]
pub mod neon {
    pub mod f32 {
        use super::super::v128_common::f32::*;
        use core::arch::aarch64::*;
        use core::mem::transmute;

        #[cfg(miri)]
        unsafe fn vfmaq_f32(c: float32x4_t, a: float32x4_t, b: float32x4_t) -> float32x4_t {
            let c: [f32; 4] = transmute(c);
            let a: [f32; 4] = transmute(a);
            let b: [f32; 4] = transmute(b);

            transmute([
                f32::mul_add(a[0], b[0], c[0]),
                f32::mul_add(a[1], b[1], c[1]),
                f32::mul_add(a[2], b[2], c[2]),
                f32::mul_add(a[3], b[3], c[3]),
            ])
        }

        #[inline(always)]
        pub unsafe fn mul(lhs: Pack, rhs: Pack) -> Pack {
            transmute(vmulq_f32(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        pub unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(vaddq_f32(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        pub unsafe fn mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            transmute(vfmaq_f32(transmute(c), transmute(a), transmute(b)))
        }

        #[inline(always)]
        pub unsafe fn mul_add_lane<const LANE: i32>(a: Pack, b: Pack, c: Pack) -> Pack {
            transmute(vfmaq_laneq_f32::<LANE>(
                transmute(c),
                transmute(a),
                transmute(b),
            ))
        }

        #[inline(always)]
        pub unsafe fn scalar_mul(lhs: T, rhs: T) -> T {
            lhs * rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_add(lhs: T, rhs: T) -> T {
            lhs + rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_mul_add(a: T, b: T, c: T) -> T {
            gemm_common::simd::neon_fmaf(a, b, c)
        }

        #[inline(always)]
        pub unsafe fn load<const MR_DIV_N: usize>(dst: *mut Pack, ptr: *const f32) {
            match MR_DIV_N {
                1 => *(dst as *mut [Pack; 1]) = transmute(vld1q_f32(ptr)),
                2 => *(dst as *mut [Pack; 2]) = transmute(vld1q_f32_x2(ptr)),
                3 => *(dst as *mut [Pack; 3]) = transmute(vld1q_f32_x3(ptr)),
                4 => *(dst as *mut [Pack; 4]) = transmute(vld1q_f32_x4(ptr)),
                _ => unreachable!(),
            }
        }

        microkernel!(["neon"], 4, x1x1, 1, 1);
        microkernel!(["neon"], 4, x1x2, 1, 2);
        microkernel!(["neon"], 4, x1x3, 1, 3);
        microkernel!(["neon"], 4, x1x4, 1, 4, 1, 4);

        microkernel!(["neon"], 4, x2x1, 2, 1);
        microkernel!(["neon"], 4, x2x2, 2, 2);
        microkernel!(["neon"], 4, x2x3, 2, 3);
        microkernel!(["neon"], 4, x2x4, 2, 4, 1, 4);

        microkernel!(["neon"], 4, x3x1, 3, 1);
        microkernel!(["neon"], 4, x3x2, 3, 2);
        microkernel!(["neon"], 4, x3x3, 3, 3);
        microkernel!(["neon"], 4, x3x4, 3, 4, 1, 4);

        microkernel!(["neon"], 4, x4x1, 4, 1);
        microkernel!(["neon"], 4, x4x2, 4, 2);
        microkernel!(["neon"], 4, x4x3, 4, 3);
        microkernel!(["neon"], 4, x4x4, 4, 4, 1, 4);

        microkernel_fn_array! {
            [x1x1, x1x2, x1x3, x1x4, ],
            [x2x1, x2x2, x2x3, x2x4, ],
            [x3x1, x3x2, x3x3, x3x4, ],
            [x4x1, x4x2, x4x3, x4x4, ],
        }
    }
}

#[cfg(target_arch = "aarch64")]
pub mod amx {
    pub mod f32 {
        pub type T = f32;
        pub const N: usize = 16;

        #[inline(always)]
        pub unsafe fn scalar_mul(lhs: T, rhs: T) -> T {
            lhs * rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_add(lhs: T, rhs: T) -> T {
            lhs + rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_mul_add(a: T, b: T, c: T) -> T {
            gemm_common::simd::neon_fmaf(a, b, c)
        }

        microkernel_amx!(f32, ["neon"], 4, x1x16, 1, 16, 1, 16);
        microkernel_amx!(f32, ["neon"], 4, x1x32, 1, 32, 2, 16);
        microkernel_amx!(f32, ["neon"], 4, x2x16, 2, 16, 1, 16);
        microkernel_amx!(f32, ["neon"], 4, x2x32, 2, 32, 2, 16);

        microkernel_fn_array! {
            [
                x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,x1x16,
                x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,
            ],
        }
    }
}

#[cfg(target_arch = "wasm32")]
pub mod simd128 {
    pub mod f32 {
        use super::super::v128_common::f32::*;
        use core::arch::wasm32::*;
        use core::mem::transmute;

        #[inline(always)]
        pub unsafe fn mul(lhs: Pack, rhs: Pack) -> Pack {
            transmute(f32x4_mul(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        pub unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(f32x4_add(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        pub unsafe fn mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            add(c, mul(a, b))
        }

        #[inline(always)]
        pub unsafe fn scalar_mul(lhs: T, rhs: T) -> T {
            lhs * rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_add(lhs: T, rhs: T) -> T {
            lhs + rhs
        }

        #[inline(always)]
        pub unsafe fn scalar_mul_add(a: T, b: T, c: T) -> T {
            a * b + c
        }

        microkernel!(["simd128"], 2, x1x1, 1, 1);
        microkernel!(["simd128"], 2, x1x2, 1, 2);
        microkernel!(["simd128"], 2, x1x3, 1, 3);
        microkernel!(["simd128"], 2, x1x4, 1, 4);

        microkernel!(["simd128"], 2, x2x1, 2, 1);
        microkernel!(["simd128"], 2, x2x2, 2, 2);
        microkernel!(["simd128"], 2, x2x3, 2, 3);
        microkernel!(["simd128"], 2, x2x4, 2, 4);

        microkernel!(["simd128"], 2, x3x1, 3, 1);
        microkernel!(["simd128"], 2, x3x2, 3, 2);
        microkernel!(["simd128"], 2, x3x3, 3, 3);
        microkernel!(["simd128"], 2, x3x4, 3, 4);

        microkernel_fn_array! {
            [x1x1, x1x2, x1x3, x1x4,],
            [x2x1, x2x2, x2x3, x2x4,],
            [x3x1, x3x2, x3x3, x3x4,],
        }
    }
}
