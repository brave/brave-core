#[allow(dead_code)]
mod v128_common {
    pub mod f16 {
        pub type T = half::f16;
        pub const N: usize = 8;
        pub type Pack = [T; N];

        #[inline(always)]
        pub unsafe fn splat(value: T) -> Pack {
            [value, value, value, value, value, value, value, value]
        }
    }
}

#[cfg(target_arch = "aarch64")]
pub mod neonfp16 {
    pub mod f16 {
        pub use super::super::v128_common::f16::*;
        use core::mem::transmute;
        use gemm_common::simd::aarch64::*;

        #[inline(always)]
        pub unsafe fn mul(lhs: Pack, rhs: Pack) -> Pack {
            transmute(vmulq_f16(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        pub unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(vaddq_f16(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        pub unsafe fn mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            transmute(vfmaq_f16(transmute(c), transmute(a), transmute(b)))
        }

        #[inline(always)]
        pub unsafe fn scalar_mul(lhs: T, rhs: T) -> T {
            transmute(multiply_f16_fp16(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        pub unsafe fn scalar_add(lhs: T, rhs: T) -> T {
            transmute(add_f16_fp16(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        pub unsafe fn scalar_mul_add(a: T, b: T, c: T) -> T {
            transmute(fmaq_f16(transmute(c), transmute(a), transmute(b)))
        }

        #[inline(always)]
        pub unsafe fn mul_add_lane<const LANE: i32>(a: Pack, b: Pack, c: Pack) -> Pack {
            transmute(vfmaq_laneq_f16::<LANE>(
                transmute(c),
                transmute(a),
                transmute(b),
            ))
        }

        pub unsafe fn load<const MR_DIV_N: usize>(dst: *mut Pack, ptr: *const half::f16) {
            use core::arch::aarch64::*;
            let ptr = ptr as *const f32;
            match MR_DIV_N {
                1 => *(dst as *mut [Pack; 1]) = transmute(vld1q_f32(ptr)),
                2 => *(dst as *mut [Pack; 2]) = transmute(vld1q_f32_x2(ptr)),
                3 => *(dst as *mut [Pack; 3]) = transmute(vld1q_f32_x3(ptr)),
                4 => *(dst as *mut [Pack; 4]) = transmute(vld1q_f32_x4(ptr)),
                _ => unreachable!(),
            }
        }

        microkernel!(["neon,fp16"], 4, x1x1, 1, 1);
        microkernel!(["neon,fp16"], 4, x1x2, 1, 2);
        microkernel!(["neon,fp16"], 4, x1x3, 1, 3);
        microkernel!(["neon,fp16"], 4, x1x4, 1, 4);
        microkernel!(["neon,fp16"], 4, x1x5, 1, 5);
        microkernel!(["neon,fp16"], 4, x1x6, 1, 6);
        microkernel!(["neon,fp16"], 4, x1x7, 1, 7);
        microkernel!(["neon,fp16"], 4, x1x8, 1, 8, 1, 8);

        microkernel!(["neon,fp16"], 4, x2x1, 2, 1);
        microkernel!(["neon,fp16"], 4, x2x2, 2, 2);
        microkernel!(["neon,fp16"], 4, x2x3, 2, 3);
        microkernel!(["neon,fp16"], 4, x2x4, 2, 4);
        microkernel!(["neon,fp16"], 4, x2x5, 2, 5);
        microkernel!(["neon,fp16"], 4, x2x6, 2, 6);
        microkernel!(["neon,fp16"], 4, x2x7, 2, 7);
        microkernel!(["neon,fp16"], 4, x2x8, 2, 8, 1, 8);

        microkernel_fn_array! {
            [x1x1, x1x2, x1x3, x1x4, x1x5, x1x6, x1x7, x1x8, ],
            [x2x1, x2x2, x2x3, x2x4, x2x5, x2x6, x2x7, x2x8, ],
        }
    }
}

#[cfg(target_arch = "aarch64")]
pub mod amx {
    pub mod f16 {
        pub type T = half::f16;
        pub const N: usize = 32;

        microkernel_amx!(f16, ["neon"], 4, x1x32, 1, 32, 1, 32);

        microkernel_fn_array! {
                    [
        x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,x1x32,
                    ],
                }
    }
}
