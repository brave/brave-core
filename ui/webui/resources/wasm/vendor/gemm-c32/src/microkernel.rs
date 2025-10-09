pub mod scalar {
    pub mod f32 {
        type T = f32;
        const N: usize = 2;
        const CPLX_N: usize = 1;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            [value, value]
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            [lhs[0] + rhs[0], lhs[1] + rhs[1]]
        }

        #[inline(always)]
        unsafe fn conj(a: Pack) -> Pack {
            [a[0], -a[1]]
        }

        #[inline(always)]
        unsafe fn swap_re_im(a: Pack) -> Pack {
            [a[1], a[0]]
        }

        #[inline(always)]
        unsafe fn mul_cplx(a_re_im: Pack, _a_im_re: Pack, b_re: Pack, b_im: Pack) -> Pack {
            [
                a_re_im[0] * b_re[0] - a_re_im[1] * b_im[0],
                a_re_im[1] * b_re[0] + a_re_im[0] * b_im[0],
            ]
        }

        #[inline(always)]
        unsafe fn mul_add_cplx(
            a_re_im: Pack,
            a_im_re: Pack,
            b_re: Pack,
            b_im: Pack,
            c_re_im: Pack,
            conj_rhs: bool,
        ) -> Pack {
            if conj_rhs {
                add(
                    c_re_im,
                    mul_cplx(a_re_im, a_im_re, b_re, [-b_im[0], -b_im[1]]),
                )
            } else {
                add(c_re_im, mul_cplx(a_re_im, a_im_re, b_re, b_im))
            }
        }

        microkernel_cplx!(, 2, x1x1, 1, 1);
        microkernel_cplx!(, 2, x1x2, 1, 2);
        microkernel_cplx!(, 2, x1x3, 1, 3);
        microkernel_cplx!(, 2, x1x4, 1, 4);

        microkernel_cplx!(, 2, x2x1, 2, 1);
        microkernel_cplx!(, 2, x2x2, 2, 2);
        microkernel_cplx!(, 2, x2x3, 2, 3);
        microkernel_cplx!(, 2, x2x4, 2, 4);

        microkernel_cplx_fn_array! {
            [x1x1, x1x2, x1x3, x1x4,],
            [x2x1, x2x2, x2x3, x2x4,],
        }
        pub const H_CPLX_M: usize = 0;
        pub const H_CPLX_N: usize = 0;
        pub const H_CPLX_UKR: [[gemm_common::microkernel::HMicroKernelFn<num_complex::Complex<T>>;
            H_CPLX_N]; H_CPLX_M] = [];
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

        use gemm_common::pulp::u32x8;

        type T = f32;
        const N: usize = 8;
        const CPLX_N: usize = 4;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            transmute(_mm256_set1_ps(value))
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(_mm256_add_ps(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        unsafe fn neg(a: Pack) -> Pack {
            const MASK: __m256 = unsafe { transmute([-0.0f32; 8]) };
            transmute(_mm256_xor_ps(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn conj(a: Pack) -> Pack {
            const MASK: __m256 = unsafe { transmute([[0.0, -0.0_f32]; 4]) };
            transmute(_mm256_xor_ps(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn neg_conj(a: Pack) -> Pack {
            const MASK: __m256 = unsafe { transmute([[-0.0, 0.0_f32]; 4]) };
            transmute(_mm256_xor_ps(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn swap_re_im(a: Pack) -> Pack {
            transmute(_mm256_permute_ps::<0b10110001>(transmute(a)))
        }

        #[inline(always)]
        unsafe fn mul_cplx(a_re_im: Pack, a_im_re: Pack, b_re: Pack, b_im: Pack) -> Pack {
            transmute(_mm256_fmaddsub_ps(
                transmute(a_re_im),
                transmute(b_re),
                _mm256_mul_ps(transmute(a_im_re), transmute(b_im)),
            ))
        }

        #[inline(always)]
        unsafe fn mul_add_cplx_step0(
            a_re_im: Pack,
            b_re: Pack,
            c_re_im: Pack,
            neg_conj_rhs: bool,
        ) -> Pack {
            if neg_conj_rhs {
                transmute(_mm256_fmaddsub_ps(
                    transmute(a_re_im),
                    transmute(b_re),
                    transmute(c_re_im),
                ))
            } else {
                transmute(_mm256_fmsubadd_ps(
                    transmute(a_re_im),
                    transmute(b_re),
                    transmute(c_re_im),
                ))
            }
        }

        #[inline(always)]
        unsafe fn mul_add_cplx_step1(
            a_im_re: Pack,
            b_im: Pack,
            c_re_im: Pack,
            neg_conj_rhs: bool,
        ) -> Pack {
            if neg_conj_rhs {
                transmute(_mm256_fmaddsub_ps(
                    transmute(a_im_re),
                    transmute(b_im),
                    transmute(c_re_im),
                ))
            } else {
                transmute(_mm256_fmsubadd_ps(
                    transmute(a_im_re),
                    transmute(b_im),
                    transmute(c_re_im),
                ))
            }
        }

        #[inline(always)]
        #[allow(dead_code)]
        unsafe fn mul_add_cplx(
            a_re_im: Pack,
            a_im_re: Pack,
            b_re: Pack,
            b_im: Pack,
            c_re_im: Pack,
            conj_rhs: bool,
        ) -> Pack {
            if conj_rhs {
                transmute(_mm256_fmsubadd_ps(
                    transmute(a_re_im),
                    transmute(b_re),
                    _mm256_fmsubadd_ps(transmute(a_im_re), transmute(b_im), transmute(c_re_im)),
                ))
            } else {
                transmute(_mm256_fmaddsub_ps(
                    transmute(a_re_im),
                    transmute(b_re),
                    _mm256_fmaddsub_ps(transmute(a_im_re), transmute(b_im), transmute(c_re_im)),
                ))
            }
        }

        microkernel_cplx_2step!(["fma"], 2, cplx_x1x1, 1, 1);
        microkernel_cplx_2step!(["fma"], 2, cplx_x1x2, 1, 2);
        microkernel_cplx_2step!(["fma"], 2, cplx_x1x3, 1, 3);
        microkernel_cplx_2step!(["fma"], 2, cplx_x1x4, 1, 4);
        microkernel_cplx_2step!(["fma"], 2, cplx_x1x5, 1, 5);
        microkernel_cplx_2step!(["fma"], 2, cplx_x1x6, 1, 6);

        microkernel_cplx_2step!(["fma"], 2, cplx_x2x1, 2, 1);
        microkernel_cplx_2step!(["fma"], 2, cplx_x2x2, 2, 2);
        microkernel_cplx_2step!(["fma"], 2, cplx_x2x3, 2, 3);
        microkernel_cplx_2step!(["fma"], 2, cplx_x2x4, 2, 4);
        microkernel_cplx_2step!(["fma"], 2, cplx_x2x5, 2, 5);
        microkernel_cplx_2step!(["fma"], 2, cplx_x2x6, 2, 6);

        microkernel_cplx_fn_array! {
            [cplx_x1x1, cplx_x1x2, cplx_x1x3, cplx_x1x4, cplx_x1x5, cplx_x1x6,],
            [cplx_x2x1, cplx_x2x2, cplx_x2x3, cplx_x2x4, cplx_x2x5, cplx_x2x6,],
        }

        #[inline(always)]
        unsafe fn mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = _mm256_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm256_moveldup_ps(ab);
            let bb = _mm256_movehdup_ps(ab);

            transmute(_mm256_fmaddsub_ps(
                aa,
                xy,
                _mm256_fmaddsub_ps(bb, yx, transmute(c)),
            ))
        }

        #[inline(always)]
        unsafe fn conj_mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = _mm256_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm256_moveldup_ps(ab);
            let bb = _mm256_movehdup_ps(ab);

            transmute(_mm256_fmsubadd_ps(
                aa,
                xy,
                _mm256_fmsubadd_ps(bb, yx, transmute(c)),
            ))
        }

        static U32_MASKS: [u32x8; 5] = [
            u32x8(0, 0, 0, 0, 0, 0, 0, 0),
            u32x8(!0, !0, 0, 0, 0, 0, 0, 0),
            u32x8(!0, !0, !0, !0, 0, 0, 0, 0),
            u32x8(!0, !0, !0, !0, !0, !0, 0, 0),
            u32x8(!0, !0, !0, !0, !0, !0, !0, !0),
        ];

        #[inline(always)]
        unsafe fn partial_load(ptr: *const num_complex::Complex<T>, len: usize) -> Pack {
            transmute(_mm256_maskload_ps(
                ptr as *mut T,
                transmute(*(U32_MASKS.as_ptr().add(len))),
            ))
        }

        #[inline(always)]
        unsafe fn reduce_sum(x: Pack) -> num_complex::Complex<T> {
            let a = transmute(x);
            // a0 a1 a2 a3
            let a = _mm_add_ps(_mm256_castps256_ps128(a), _mm256_extractf128_ps::<1>(a));
            // a2 a3 a2 a3
            let hi = _mm_movehl_ps(a, a);

            // a0+a2 a1+a3 _ _
            let r0 = _mm_add_ps(a, hi);

            transmute(_mm_cvtsd_f64(transmute(r0)))
        }

        horizontal_cplx_kernel!(["fma"], hx1x1, 1, 1);
        horizontal_cplx_kernel!(["fma"], hx1x2, 1, 2);
        horizontal_cplx_kernel!(["fma"], hx2x1, 2, 1);
        horizontal_cplx_kernel!(["fma"], hx2x2, 2, 2);
        hmicrokernel_cplx_fn_array! {
            [hx1x1, hx1x2,],
            [hx2x1, hx2x2,],
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
        const CPLX_N: usize = 8;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            transmute(_mm512_set1_ps(value))
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(_mm512_add_ps(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        unsafe fn neg(a: Pack) -> Pack {
            const MASK: __m512i = unsafe { transmute([-0.0f32; 16]) };
            transmute(_mm512_xor_si512(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn conj(a: Pack) -> Pack {
            const MASK: __m512i = unsafe { transmute([[0.0, -0.0_f32]; 8]) };
            transmute(_mm512_xor_si512(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn neg_conj(a: Pack) -> Pack {
            const MASK: __m512i = unsafe { transmute([[-0.0, 0.0_f32]; 8]) };
            transmute(_mm512_xor_si512(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn swap_re_im(a: Pack) -> Pack {
            transmute(_mm512_permute_ps::<0b10110001>(transmute(a)))
        }

        #[inline(always)]
        unsafe fn mul_cplx(a_re_im: Pack, a_im_re: Pack, b_re: Pack, b_im: Pack) -> Pack {
            transmute(_mm512_fmaddsub_ps(
                transmute(a_re_im),
                transmute(b_re),
                _mm512_mul_ps(transmute(a_im_re), transmute(b_im)),
            ))
        }

        #[target_feature(enable = "avx512f")]
        #[inline]
        unsafe fn subadd_ps(a: __m512, b: __m512, c: __m512) -> __m512 {
            _mm512_fmaddsub_ps(
                a,
                b,
                transmute(_mm512_xor_si512(
                    transmute(c),
                    transmute(_mm512_set1_ps(-0.0)),
                )),
            )
        }

        #[inline(always)]
        unsafe fn mul_add_cplx_step0(
            a_re_im: Pack,
            b_re: Pack,
            c_re_im: Pack,
            neg_conj_rhs: bool,
        ) -> Pack {
            if neg_conj_rhs {
                transmute(_mm512_fmaddsub_ps(
                    transmute(a_re_im),
                    transmute(b_re),
                    transmute(c_re_im),
                ))
            } else {
                transmute(subadd_ps(
                    transmute(a_re_im),
                    transmute(b_re),
                    transmute(c_re_im),
                ))
            }
        }

        #[inline(always)]
        unsafe fn mul_add_cplx_step1(
            a_im_re: Pack,
            b_im: Pack,
            c_re_im: Pack,
            neg_conj_rhs: bool,
        ) -> Pack {
            if neg_conj_rhs {
                transmute(_mm512_fmaddsub_ps(
                    transmute(a_im_re),
                    transmute(b_im),
                    transmute(c_re_im),
                ))
            } else {
                transmute(subadd_ps(
                    transmute(a_im_re),
                    transmute(b_im),
                    transmute(c_re_im),
                ))
            }
        }

        #[inline(always)]
        #[allow(dead_code)]
        unsafe fn mul_add_cplx(
            a_re_im: Pack,
            a_im_re: Pack,
            b_re: Pack,
            b_im: Pack,
            c_re_im: Pack,
            conj_rhs: bool,
        ) -> Pack {
            if conj_rhs {
                transmute(subadd_ps(
                    transmute(a_re_im),
                    transmute(b_re),
                    subadd_ps(transmute(a_im_re), transmute(b_im), transmute(c_re_im)),
                ))
            } else {
                transmute(_mm512_fmaddsub_ps(
                    transmute(a_re_im),
                    transmute(b_re),
                    _mm512_fmaddsub_ps(transmute(a_im_re), transmute(b_im), transmute(c_re_im)),
                ))
            }
        }

        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x1, 1, 1);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x2, 1, 2);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x3, 1, 3);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x4, 1, 4);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x5, 1, 5);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x6, 1, 6);

        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x1, 2, 1);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x2, 2, 2);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x3, 2, 3);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x4, 2, 4);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x5, 2, 5);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x6, 2, 6);

        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x1, 3, 1);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x2, 3, 2);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x3, 3, 3);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x4, 3, 4);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x5, 3, 5);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x6, 3, 6);

        microkernel_cplx_2step!(["avx512f"], 2, cplx_x4x1, 4, 1);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x4x2, 4, 2);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x4x3, 4, 3);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x4x4, 4, 4);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x4x5, 4, 5);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x4x6, 4, 6);

        microkernel_cplx_fn_array! {
            [cplx_x1x1, cplx_x1x2, cplx_x1x3, cplx_x1x4, cplx_x1x5, cplx_x1x6,],
            [cplx_x2x1, cplx_x2x2, cplx_x2x3, cplx_x2x4, cplx_x2x5, cplx_x2x6,],
            [cplx_x3x1, cplx_x3x2, cplx_x3x3, cplx_x3x4, cplx_x3x5, cplx_x3x6,],
            [cplx_x4x1, cplx_x4x2, cplx_x4x3, cplx_x4x4, cplx_x4x5, cplx_x4x6,],
        }

        #[inline(always)]
        unsafe fn mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = _mm512_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm512_moveldup_ps(ab);
            let bb = _mm512_movehdup_ps(ab);

            transmute(_mm512_fmaddsub_ps(
                aa,
                xy,
                _mm512_fmaddsub_ps(bb, yx, transmute(c)),
            ))
        }

        #[inline(always)]
        unsafe fn conj_mul_add(a: Pack, b: Pack, c: Pack) -> Pack {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = _mm512_permute_ps::<0b10_11_00_01>(xy);
            let aa = _mm512_moveldup_ps(ab);
            let bb = _mm512_movehdup_ps(ab);

            transmute(subadd_ps(aa, xy, subadd_ps(bb, yx, transmute(c))))
        }

        static U32_MASKS: [u16; 9] = [
            0b0000000000000000, //
            0b0000000000000011, //
            0b0000000000001111, //
            0b0000000000111111, //
            0b0000000011111111, //
            0b0000001111111111, //
            0b0000111111111111, //
            0b0011111111111111, //
            0b1111111111111111, //
        ];

        #[inline(always)]
        unsafe fn partial_load(ptr: *const num_complex::Complex<T>, len: usize) -> Pack {
            transmute(_mm512_maskz_loadu_ps(
                transmute(*(U32_MASKS.as_ptr().add(len))),
                ptr as *mut T,
            ))
        }

        #[inline(always)]
        unsafe fn reduce_sum(x: Pack) -> num_complex::Complex<T> {
            let a = transmute(x);
            let a = _mm256_add_ps(
                _mm512_castps512_ps256(a),
                transmute(_mm512_extractf64x4_pd::<1>(transmute(a))),
            );

            // a0 a1 a2 a3
            let a = _mm_add_ps(_mm256_castps256_ps128(a), _mm256_extractf128_ps::<1>(a));
            // a2 a3 a2 a3
            let hi = _mm_movehl_ps(a, a);

            // a0+a2 a1+a3 _ _
            let r0 = _mm_add_ps(a, hi);

            transmute(_mm_cvtsd_f64(transmute(r0)))
        }

        horizontal_cplx_kernel!(["avx512f"], hx1x1, 1, 1);
        horizontal_cplx_kernel!(["avx512f"], hx1x2, 1, 2);
        horizontal_cplx_kernel!(["avx512f"], hx1x3, 1, 3);
        horizontal_cplx_kernel!(["avx512f"], hx1x4, 1, 4);
        horizontal_cplx_kernel!(["avx512f"], hx2x1, 2, 1);
        horizontal_cplx_kernel!(["avx512f"], hx2x2, 2, 2);
        horizontal_cplx_kernel!(["avx512f"], hx2x3, 2, 3);
        horizontal_cplx_kernel!(["avx512f"], hx2x4, 2, 4);
        horizontal_cplx_kernel!(["avx512f"], hx3x1, 3, 1);
        horizontal_cplx_kernel!(["avx512f"], hx3x2, 3, 2);
        horizontal_cplx_kernel!(["avx512f"], hx3x3, 3, 3);
        horizontal_cplx_kernel!(["avx512f"], hx3x4, 3, 4);
        horizontal_cplx_kernel!(["avx512f"], hx4x1, 4, 1);
        horizontal_cplx_kernel!(["avx512f"], hx4x2, 4, 2);
        horizontal_cplx_kernel!(["avx512f"], hx4x3, 4, 3);
        horizontal_cplx_kernel!(["avx512f"], hx4x4, 4, 4);
        hmicrokernel_cplx_fn_array! {
            [hx1x1, hx1x2, hx1x3, hx1x4,],
            [hx2x1, hx2x2, hx2x3, hx2x4,],
            [hx3x1, hx3x2, hx3x3, hx3x4,],
            [hx4x1, hx4x2, hx4x3, hx4x4,],
        }
    }
}

#[cfg(target_arch = "aarch64")]
pub mod neonfcma {
    pub mod c32 {
        use core::arch::{aarch64::*, asm};
        use core::mem::transmute;

        type T = num_complex::Complex32;
        const N: usize = 2;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            [value, value]
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(vaddq_f32(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        unsafe fn conj(a: Pack) -> Pack {
            [T::new(a[0].re, -a[0].im), T::new(a[1].re, -a[1].im)]
        }

        #[inline(always)]
        unsafe fn mul_cplx(lhs: Pack, rhs: Pack) -> Pack {
            mul_add_cplx(lhs, rhs, core::mem::zeroed(), false)
        }

        #[inline]
        #[target_feature(enable = "neon,fcma")]
        unsafe fn vcmlaq_0_f32(
            mut acc: float32x4_t,
            lhs: float32x4_t,
            rhs: float32x4_t,
        ) -> float32x4_t {
            asm!(
                "fcmla {0:v}.4s, {1:v}.4s, {2:v}.4s, 0",
                inout(vreg) acc,
                in(vreg) lhs,
                in(vreg) rhs,
                options(pure, nomem, nostack));
            acc
        }

        #[inline]
        #[target_feature(enable = "neon,fcma")]
        unsafe fn vcmlaq_90_f32(
            mut acc: float32x4_t,
            lhs: float32x4_t,
            rhs: float32x4_t,
        ) -> float32x4_t {
            asm!(
                "fcmla {0:v}.4s, {1:v}.4s, {2:v}.4s, 90",
                inout(vreg) acc,
                in(vreg) lhs,
                in(vreg) rhs,
                options(pure, nomem, nostack));
            acc
        }

        #[inline]
        #[target_feature(enable = "neon,fcma")]
        unsafe fn vcmlaq_270_f32(
            mut acc: float32x4_t,
            lhs: float32x4_t,
            rhs: float32x4_t,
        ) -> float32x4_t {
            asm!(
                "fcmla {0:v}.4s, {1:v}.4s, {2:v}.4s, 270",
                inout(vreg) acc,
                in(vreg) lhs,
                in(vreg) rhs,
                options(pure, nomem, nostack));
            acc
        }

        #[inline(always)]
        unsafe fn mul_add_cplx(lhs: Pack, rhs: Pack, acc: Pack, conj_rhs: bool) -> Pack {
            let _ = conj_rhs;
            let lhs = transmute(lhs);
            let rhs = transmute(rhs);
            let acc = transmute(acc);

            if conj_rhs {
                transmute(vcmlaq_270_f32(vcmlaq_0_f32(acc, rhs, lhs), rhs, lhs))
            } else {
                transmute(vcmlaq_90_f32(vcmlaq_0_f32(acc, lhs, rhs), lhs, rhs))
            }
        }

        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x1x1, 1, 1);
        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x1x2, 1, 2);
        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x1x3, 1, 3);
        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x1x4, 1, 4);

        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x2x1, 2, 1);
        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x2x2, 2, 2);
        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x2x3, 2, 3);
        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x2x4, 2, 4);

        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x3x1, 3, 1);
        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x3x2, 3, 2);
        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x3x3, 3, 3);
        microkernel_cplx_packed!(["neon,fcma"], 4, cplx_x3x4, 3, 4);

        microkernel_fn_array! {
            [cplx_x1x1, cplx_x1x2, cplx_x1x3, cplx_x1x4,],
            [cplx_x2x1, cplx_x2x2, cplx_x2x3, cplx_x2x4,],
            [cplx_x3x1, cplx_x3x2, cplx_x3x3, cplx_x3x4,],
        }
        pub const H_M: usize = 0;
        pub const H_N: usize = 0;
        pub const H_UKR: [[gemm_common::microkernel::HMicroKernelFn<T>; H_N]; H_M] = [];
    }
}
