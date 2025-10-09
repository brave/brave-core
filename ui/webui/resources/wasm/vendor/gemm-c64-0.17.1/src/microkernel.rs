pub mod scalar {
    pub mod f64 {
        type T = f64;
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
    }
}

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
pub mod fma {
    pub mod f64 {
        #[cfg(target_arch = "x86")]
        use core::arch::x86::*;
        #[cfg(target_arch = "x86_64")]
        use core::arch::x86_64::*;
        use core::mem::transmute;

        type T = f64;
        const N: usize = 4;
        const CPLX_N: usize = 2;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            transmute(_mm256_set1_pd(value))
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(_mm256_add_pd(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        unsafe fn conj(a: Pack) -> Pack {
            const MASK: __m256d = unsafe { transmute([0.0_f64, -0.0_f64, 0.0_f64, -0.0_f64]) };
            transmute(_mm256_xor_pd(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn neg_conj(a: Pack) -> Pack {
            const MASK: __m256d = unsafe { transmute([-0.0_f64, 0.0_f64, -0.0_f64, 0.0_f64]) };
            transmute(_mm256_xor_pd(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn neg(a: Pack) -> Pack {
            const MASK: __m256d = unsafe { transmute([-0.0_f64, -0.0_f64, -0.0_f64, -0.0_f64]) };
            transmute(_mm256_xor_pd(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn swap_re_im(a: Pack) -> Pack {
            transmute(_mm256_permute_pd::<0b0101>(transmute(a)))
        }

        #[inline(always)]
        unsafe fn mul_cplx(a_re_im: Pack, a_im_re: Pack, b_re: Pack, b_im: Pack) -> Pack {
            transmute(_mm256_fmaddsub_pd(
                transmute(a_re_im),
                transmute(b_re),
                _mm256_mul_pd(transmute(a_im_re), transmute(b_im)),
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
                transmute(_mm256_fmaddsub_pd(
                    transmute(a_re_im),
                    transmute(b_re),
                    transmute(c_re_im),
                ))
            } else {
                transmute(_mm256_fmsubadd_pd(
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
                transmute(_mm256_fmaddsub_pd(
                    transmute(a_im_re),
                    transmute(b_im),
                    transmute(c_re_im),
                ))
            } else {
                transmute(_mm256_fmsubadd_pd(
                    transmute(a_im_re),
                    transmute(b_im),
                    transmute(c_re_im),
                ))
            }
        }

        microkernel_cplx_2step!(["fma"], 2, cplx_x1x1, 1, 1);
        microkernel_cplx_2step!(["fma"], 2, cplx_x1x2, 1, 2);
        microkernel_cplx_2step!(["fma"], 2, cplx_x1x3, 1, 3);
        microkernel_cplx_2step!(["fma"], 2, cplx_x1x4, 1, 4);

        microkernel_cplx_2step!(["fma"], 2, cplx_x2x1, 2, 1);
        microkernel_cplx_2step!(["fma"], 2, cplx_x2x2, 2, 2);
        microkernel_cplx_2step!(["fma"], 2, cplx_x2x3, 2, 3);
        microkernel_cplx_2step!(["fma"], 2, cplx_x2x4, 2, 4);

        microkernel_cplx_2step!(["fma"], 2, cplx_x3x1, 3, 1);
        microkernel_cplx_2step!(["fma"], 2, cplx_x3x2, 3, 2);
        microkernel_cplx_2step!(["fma"], 2, cplx_x3x3, 3, 3);
        microkernel_cplx_2step!(["fma"], 2, cplx_x3x4, 3, 4);

        microkernel_cplx_fn_array! {
            [cplx_x1x1, cplx_x1x2, cplx_x1x3, cplx_x1x4,],
            [cplx_x2x1, cplx_x2x2, cplx_x2x3, cplx_x2x4,],
            [cplx_x3x1, cplx_x3x2, cplx_x3x3, cplx_x3x4,],
        }
    }
}

#[cfg(all(feature = "nightly", any(target_arch = "x86", target_arch = "x86_64")))]
pub mod avx512f {
    pub mod f64 {
        #[cfg(target_arch = "x86")]
        use core::arch::x86::*;
        #[cfg(target_arch = "x86_64")]
        use core::arch::x86_64::*;
        use core::mem::transmute;

        type T = f64;
        const N: usize = 8;
        const CPLX_N: usize = 4;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            transmute(_mm512_set1_pd(value))
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            transmute(_mm512_add_pd(transmute(lhs), transmute(rhs)))
        }

        #[inline(always)]
        unsafe fn neg(a: Pack) -> Pack {
            const MASK: __m512i = unsafe {
                transmute([
                    -0.0_f64, -0.0_f64, -0.0_f64, -0.0_f64, -0.0_f64, -0.0_f64, -0.0_f64, -0.0_f64,
                ])
            };
            transmute(_mm512_xor_si512(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn conj(a: Pack) -> Pack {
            const MASK: __m512i = unsafe {
                transmute([
                    0.0_f64, -0.0_f64, 0.0_f64, -0.0_f64, 0.0_f64, -0.0_f64, 0.0_f64, -0.0_f64,
                ])
            };
            transmute(_mm512_xor_si512(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn neg_conj(a: Pack) -> Pack {
            const MASK: __m512i = unsafe {
                transmute([
                    -0.0_f64, 0.0_f64, -0.0_f64, 0.0_f64, -0.0_f64, 0.0_f64, -0.0_f64, 0.0_f64,
                ])
            };
            transmute(_mm512_xor_si512(MASK, transmute(a)))
        }

        #[inline(always)]
        unsafe fn swap_re_im(a: Pack) -> Pack {
            transmute(_mm512_permute_pd::<0b01010101>(transmute(a)))
        }

        #[inline(always)]
        unsafe fn mul_cplx(a_re_im: Pack, a_im_re: Pack, b_re: Pack, b_im: Pack) -> Pack {
            transmute(_mm512_fmaddsub_pd(
                transmute(a_re_im),
                transmute(b_re),
                _mm512_mul_pd(transmute(a_im_re), transmute(b_im)),
            ))
        }

        #[target_feature(enable = "avx512f")]
        #[inline]
        unsafe fn subadd_pd(a: __m512d, b: __m512d, c: __m512d) -> __m512d {
            _mm512_fmaddsub_pd(
                a,
                b,
                transmute(_mm512_xor_si512(
                    transmute(c),
                    transmute(_mm512_set1_pd(-0.0)),
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
                transmute(_mm512_fmaddsub_pd(
                    transmute(a_re_im),
                    transmute(b_re),
                    transmute(c_re_im),
                ))
            } else {
                transmute(subadd_pd(
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
                transmute(_mm512_fmaddsub_pd(
                    transmute(a_im_re),
                    transmute(b_im),
                    transmute(c_re_im),
                ))
            } else {
                transmute(subadd_pd(
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
                transmute(subadd_pd(
                    transmute(a_re_im),
                    transmute(b_re),
                    subadd_pd(transmute(a_im_re), transmute(b_im), transmute(c_re_im)),
                ))
            } else {
                transmute(_mm512_fmaddsub_pd(
                    transmute(a_re_im),
                    transmute(b_re),
                    _mm512_fmaddsub_pd(transmute(a_im_re), transmute(b_im), transmute(c_re_im)),
                ))
            }
        }

        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x1, 1, 1);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x2, 1, 2);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x3, 1, 3);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x4, 1, 4);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x5, 1, 5);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x6, 1, 6);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x7, 1, 7);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x1x8, 1, 8);

        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x1, 2, 1);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x2, 2, 2);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x3, 2, 3);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x4, 2, 4);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x5, 2, 5);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x6, 2, 6);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x7, 2, 7);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x2x8, 2, 8);

        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x1, 3, 1);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x2, 3, 2);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x3, 3, 3);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x4, 3, 4);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x5, 3, 5);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x6, 3, 6);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x7, 3, 7);
        microkernel_cplx_2step!(["avx512f"], 2, cplx_x3x8, 3, 8);

        microkernel_cplx_fn_array! {
            [cplx_x1x1, cplx_x1x2, cplx_x1x3, cplx_x1x4, cplx_x1x5, cplx_x1x6, cplx_x1x7, cplx_x1x8,],
            [cplx_x2x1, cplx_x2x2, cplx_x2x3, cplx_x2x4, cplx_x2x5, cplx_x2x6, cplx_x2x7, cplx_x2x8,],
            [cplx_x3x1, cplx_x3x2, cplx_x3x3, cplx_x3x4, cplx_x3x5, cplx_x3x6, cplx_x3x7, cplx_x3x8,],
        }
    }
}

#[cfg(target_arch = "aarch64")]
pub mod neonfcma {
    pub mod c64 {
        use core::arch::{aarch64::*, asm};
        use core::mem::transmute;

        type T = num_complex::Complex64;
        const N: usize = 1;
        type Pack = [T; N];

        #[inline(always)]
        unsafe fn splat(value: T) -> Pack {
            [value]
        }

        #[inline(always)]
        unsafe fn add(lhs: Pack, rhs: Pack) -> Pack {
            [transmute(vaddq_f64(transmute(lhs), transmute(rhs)))]
        }

        #[inline(always)]
        unsafe fn conj(a: Pack) -> Pack {
            [T::new(a[0].re, -a[0].im)]
        }

        #[inline(always)]
        unsafe fn mul_cplx(lhs: Pack, rhs: Pack) -> Pack {
            mul_add_cplx(lhs, rhs, core::mem::zeroed(), false)
        }

        #[inline]
        #[target_feature(enable = "neon,fcma")]
        unsafe fn vcmlaq_0_f64(
            mut acc: float64x2_t,
            lhs: float64x2_t,
            rhs: float64x2_t,
        ) -> float64x2_t {
            asm!(
                "fcmla {0:v}.2d, {1:v}.2d, {2:v}.2d, 0",
                inout(vreg) acc,
                in(vreg) lhs,
                in(vreg) rhs,
                options(pure, nomem, nostack));
            acc
        }

        #[inline]
        #[target_feature(enable = "neon,fcma")]
        unsafe fn vcmlaq_90_f64(
            mut acc: float64x2_t,
            lhs: float64x2_t,
            rhs: float64x2_t,
        ) -> float64x2_t {
            asm!(
                "fcmla {0:v}.2d, {1:v}.2d, {2:v}.2d, 90",
                inout(vreg) acc,
                in(vreg) lhs,
                in(vreg) rhs,
                options(pure, nomem, nostack));
            acc
        }

        #[inline]
        #[target_feature(enable = "neon,fcma")]
        unsafe fn vcmlaq_270_f64(
            mut acc: float64x2_t,
            lhs: float64x2_t,
            rhs: float64x2_t,
        ) -> float64x2_t {
            asm!(
                "fcmla {0:v}.2d, {1:v}.2d, {2:v}.2d, 270",
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
                transmute(vcmlaq_270_f64(vcmlaq_0_f64(acc, rhs, lhs), rhs, lhs))
            } else {
                transmute(vcmlaq_90_f64(vcmlaq_0_f64(acc, lhs, rhs), lhs, rhs))
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
    }
}
