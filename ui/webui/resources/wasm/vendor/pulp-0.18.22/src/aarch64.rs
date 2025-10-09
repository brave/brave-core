use super::*;
use crate::core_arch::internal_simd_type;
use core::arch::aarch64::*;
use core::arch::asm;
use core::mem::transmute;

#[inline]
#[target_feature(enable = "neon,fcma")]
unsafe fn vcmlaq_0_f64(mut acc: float64x2_t, lhs: float64x2_t, rhs: float64x2_t) -> float64x2_t {
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
unsafe fn vcmlaq_90_f64(mut acc: float64x2_t, lhs: float64x2_t, rhs: float64x2_t) -> float64x2_t {
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
unsafe fn vcmlaq_270_f64(mut acc: float64x2_t, lhs: float64x2_t, rhs: float64x2_t) -> float64x2_t {
    asm!(
        "fcmla {0:v}.2d, {1:v}.2d, {2:v}.2d, 270",
        inout(vreg) acc,
        in(vreg) lhs,
        in(vreg) rhs,
        options(pure, nomem, nostack));
    acc
}

#[inline]
#[target_feature(enable = "neon,fcma")]
unsafe fn vcmlaq_0_f32(mut acc: float32x4_t, lhs: float32x4_t, rhs: float32x4_t) -> float32x4_t {
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
unsafe fn vcmlaq_90_f32(mut acc: float32x4_t, lhs: float32x4_t, rhs: float32x4_t) -> float32x4_t {
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
unsafe fn vcmlaq_270_f32(mut acc: float32x4_t, lhs: float32x4_t, rhs: float32x4_t) -> float32x4_t {
    asm!(
        "fcmla {0:v}.4s, {1:v}.4s, {2:v}.4s, 270",
        inout(vreg) acc,
        in(vreg) lhs,
        in(vreg) rhs,
        options(pure, nomem, nostack));
    acc
}

internal_simd_type! {
    #[allow(missing_docs)]
    pub struct Neon {
        pub neon: "neon",
    }

    #[allow(missing_docs)]
    pub struct NeonFcma {
        pub neon: "neon",
        pub fcma: "fcma",
    }
}

impl core::ops::Deref for NeonFcma {
    type Target = Neon;

    #[inline]
    fn deref(&self) -> &Self::Target {
        unsafe { &*(self as *const _ as *const Neon) }
    }
}

impl Seal for Neon {}
impl Seal for NeonFcma {}

static NEON_ROTATE_IDX: [u8x16; 16] = [
    u8x16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    u8x16(15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14),
    u8x16(14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13),
    u8x16(13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12),
    u8x16(12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11),
    u8x16(11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10),
    u8x16(10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9),
    u8x16(9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8),
    u8x16(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7),
    u8x16(7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6),
    u8x16(6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5),
    u8x16(5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4),
    u8x16(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3),
    u8x16(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2),
    u8x16(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1),
    u8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0),
];

impl Simd for Neon {
    type m32s = m32x4;
    type f32s = f32x4;
    type c32s = f32x4;
    type i32s = i32x4;
    type u32s = u32x4;

    type m64s = m64x2;
    type f64s = f64x2;
    type c64s = f64x2;
    type i64s = i64x2;
    type u64s = u64x2;

    #[inline(always)]
    fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
        struct Impl<Op> {
            this: Neon,
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
    fn u32s_partial_load(self, slice: &[u32]) -> Self::u32s {
        match slice.len() {
            0 => u32x4(0, 0, 0, 0),
            1 => u32x4(slice[0], 0, 0, 0),
            2 => u32x4(slice[0], slice[1], 0, 0),
            3 => u32x4(slice[0], slice[1], slice[2], 0),
            _ => u32x4(slice[0], slice[1], slice[2], slice[3]),
        }
    }

    #[inline(always)]
    fn u32s_partial_store(self, slice: &mut [u32], values: Self::u32s) {
        match slice.len() {
            0 => {}
            1 => {
                slice[0] = values.0;
            }
            2 => {
                slice[0] = values.0;
                slice[1] = values.1;
            }
            3 => {
                slice[0] = values.0;
                slice[1] = values.1;
                slice[2] = values.2;
            }
            _ => {
                slice[0] = values.0;
                slice[1] = values.1;
                slice[2] = values.2;
                slice[3] = values.3;
            }
        }
    }

    #[inline(always)]
    fn u64s_partial_load(self, slice: &[u64]) -> Self::u64s {
        match slice.len() {
            0 => u64x2(0, 0),
            1 => u64x2(slice[0], 0),
            _ => u64x2(slice[0], slice[1]),
        }
    }

    #[inline(always)]
    fn u64s_partial_store(self, slice: &mut [u64], values: Self::u64s) {
        match slice.len() {
            0 => {}
            1 => {
                slice[0] = values.0;
            }
            _ => {
                slice[0] = values.0;
                slice[1] = values.1;
            }
        }
    }

    #[inline(always)]
    fn u32s_partial_load_last(self, slice: &[u32]) -> Self::u32s {
        match slice.len() {
            0 => u32x4(0, 0, 0, 0),
            1 => u32x4(0, 0, 0, slice[0]),
            2 => u32x4(0, 0, slice[0], slice[1]),
            3 => u32x4(0, slice[0], slice[1], slice[2]),
            _ => {
                let slice = &slice[slice.len() - 4..];
                u32x4(slice[0], slice[1], slice[2], slice[3])
            }
        }
    }

    #[inline(always)]
    fn u32s_partial_store_last(self, slice: &mut [u32], values: Self::u32s) {
        match slice.len() {
            0 => {}
            1 => {
                slice[0] = values.3;
            }
            2 => {
                slice[0] = values.2;
                slice[1] = values.3;
            }
            3 => {
                slice[0] = values.1;
                slice[1] = values.2;
                slice[2] = values.3;
            }
            _ => {
                let len = slice.len();
                let slice = &mut slice[len - 4..];
                slice[0] = values.0;
                slice[1] = values.1;
                slice[2] = values.2;
                slice[3] = values.3;
            }
        }
    }

    #[inline(always)]
    fn u64s_partial_load_last(self, slice: &[u64]) -> Self::u64s {
        match slice.len() {
            0 => u64x2(0, 0),
            1 => u64x2(0, slice[0]),
            _ => {
                let slice = &slice[slice.len() - 2..];
                u64x2(slice[0], slice[1])
            }
        }
    }

    #[inline(always)]
    fn u64s_partial_store_last(self, slice: &mut [u64], values: Self::u64s) {
        match slice.len() {
            0 => {}
            1 => {
                slice[0] = values.1;
            }
            _ => {
                let len = slice.len();
                let slice = &mut slice[len - 2..];
                slice[0] = values.0;
                slice[1] = values.1;
            }
        }
    }

    #[inline(always)]
    fn m32s_not(self, a: Self::m32s) -> Self::m32s {
        self.not_m32x4(a)
    }

    #[inline(always)]
    fn m32s_and(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        self.and_m32x4(a, b)
    }

    #[inline(always)]
    fn m32s_or(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        self.or_m32x4(a, b)
    }

    #[inline(always)]
    fn m32s_xor(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        self.xor_m32x4(a, b)
    }

    #[inline(always)]
    fn m64s_not(self, a: Self::m64s) -> Self::m64s {
        self.not_m64x2(a)
    }

    #[inline(always)]
    fn m64s_and(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        self.and_m64x2(a, b)
    }

    #[inline(always)]
    fn m64s_or(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        self.or_m64x2(a, b)
    }

    #[inline(always)]
    fn m64s_xor(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        self.xor_m64x2(a, b)
    }

    #[inline(always)]
    fn u32s_not(self, a: Self::u32s) -> Self::u32s {
        self.not_u32x4(a)
    }

    #[inline(always)]
    fn u32s_and(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.and_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_or(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.or_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_xor(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.xor_u32x4(a, b)
    }

    #[inline(always)]
    fn u64s_not(self, a: Self::u64s) -> Self::u64s {
        self.not_u64x2(a)
    }

    #[inline(always)]
    fn u64s_and(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.and_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_or(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.or_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_xor(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.xor_u64x2(a, b)
    }

    #[inline(always)]
    fn m32s_select_u32s(
        self,
        mask: Self::m32s,
        if_true: Self::u32s,
        if_false: Self::u32s,
    ) -> Self::u32s {
        self.select_u32x4(mask, if_true, if_false)
    }

    #[inline(always)]
    fn m64s_select_u64s(
        self,
        mask: Self::m64s,
        if_true: Self::u64s,
        if_false: Self::u64s,
    ) -> Self::u64s {
        self.select_u64x2(mask, if_true, if_false)
    }

    #[inline(always)]
    fn u32s_splat(self, value: u32) -> Self::u32s {
        self.splat_u32x4(value)
    }

    #[inline(always)]
    fn u32s_add(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.wrapping_add_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_sub(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.wrapping_sub_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_less_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_lt_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_gt_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_less_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_le_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_ge_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shl(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        self.shl_dyn_u32x4(a, self.and_i32x4(cast(amount), self.splat_i32x4(32 - 1)))
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shr(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        u32x4(
            a.0 >> amount.0,
            a.1 >> amount.1,
            a.2 >> amount.2,
            a.3 >> amount.3,
        )
    }

    #[inline(always)]
    fn u32s_widening_mul(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
        #[inline(always)]
        fn widen_mul(a: u32, b: u32) -> (u32, u32) {
            let a = a as u64;
            let b = b as u64;
            let c = a * b;

            (c as u32, (c >> 32) as u32)
        }

        let (c0, d0) = widen_mul(a.0, b.0);
        let (c1, d1) = widen_mul(a.1, b.1);
        let (c2, d2) = widen_mul(a.2, b.2);
        let (c3, d3) = widen_mul(a.3, b.3);

        (u32x4(c0, c1, c2, c3), u32x4(d0, d1, d2, d3))
    }

    #[inline(always)]
    fn u64s_splat(self, value: u64) -> Self::u64s {
        self.splat_u64x2(value)
    }

    #[inline(always)]
    fn u64s_add(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.wrapping_add_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_sub(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.wrapping_sub_u64x2(a, b)
    }

    #[inline(always)]
    fn f32s_splat(self, value: f32) -> Self::f32s {
        self.splat_f32x4(value)
    }

    #[inline(always)]
    fn f32s_add(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.add_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_sub(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.sub_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_mul(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.mul_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_div(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.div_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_mul_add(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        self.mul_add_f32x4(a, b, c)
    }

    #[inline(always)]
    fn f32s_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        self.cmp_eq_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_less_than(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        self.cmp_lt_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_less_than_or_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        self.cmp_le_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_min(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.min_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_max(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.max_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_reduce_sum(self, a: Self::f32s) -> f32 {
        self.reduce_sum_f32x4(a)
    }

    #[inline(always)]
    fn f32s_reduce_product(self, a: Self::f32s) -> f32 {
        self.reduce_product_f32x4(a)
    }

    #[inline(always)]
    fn f32s_reduce_min(self, a: Self::f32s) -> f32 {
        self.reduce_min_f32x4(a)
    }

    #[inline(always)]
    fn f32s_reduce_max(self, a: Self::f32s) -> f32 {
        self.reduce_max_f32x4(a)
    }

    #[inline(always)]
    fn c32s_splat(self, value: c32) -> Self::c32s {
        cast([value; 2])
    }

    #[inline(always)]
    fn c32s_conj(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.c32s_splat(c32 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c32s_neg(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.f32s_splat(-0.0))
    }

    #[inline(always)]
    fn c32s_add(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.add_f32x4(a, b)
    }

    #[inline(always)]
    fn c32s_sub(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.sub_f32x4(a, b)
    }

    #[inline(always)]
    fn c32s_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = vrev64q_f32(xy);
            let aa = vtrn1q_f32(ab, ab);
            let bb = vtrn2q_f32(ab, ab);

            let bb_sign = transmute(veorq_u64(
                transmute(f32x4(-0.0, 0.0, -0.0, 0.0)),
                transmute(bb),
            ));

            transmute(vfmaq_f32(vmulq_f32(bb_sign, yx), aa, xy))
        }
    }

    #[inline(always)]
    fn c32s_conj_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = vrev64q_f32(xy);
            let aa = vtrn1q_f32(ab, ab);
            let bb = vtrn2q_f32(ab, ab);

            let bb_sign = transmute(veorq_u64(
                transmute(f32x4(0.0, -0.0, 0.0, -0.0)),
                transmute(bb),
            ));

            transmute(vfmaq_f32(vmulq_f32(bb_sign, yx), aa, xy))
        }
    }

    #[inline(always)]
    fn c32s_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = vrev64q_f32(xy);
            let aa = vtrn1q_f32(ab, ab);
            let bb = vtrn2q_f32(ab, ab);

            let bb_sign = transmute(veorq_u64(
                transmute(f32x4(-0.0, 0.0, -0.0, 0.0)),
                transmute(bb),
            ));

            transmute(vfmaq_f32(vfmaq_f32(transmute(c), bb_sign, yx), aa, xy))
        }
    }

    #[inline(always)]
    fn c32s_conj_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        unsafe {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = vrev64q_f32(xy);
            let aa = vtrn1q_f32(ab, ab);
            let bb = vtrn2q_f32(ab, ab);

            let bb_sign = transmute(veorq_u64(
                transmute(f32x4(0.0, -0.0, 0.0, -0.0)),
                transmute(bb),
            ));

            transmute(vfmaq_f32(vfmaq_f32(transmute(c), bb_sign, yx), aa, xy))
        }
    }

    #[inline(always)]
    fn c32s_abs2(self, a: Self::c32s) -> Self::c32s {
        unsafe {
            let sqr = self.f32s_mul(a, a);
            let sqr_rev = transmute(vrev64q_f32(transmute(sqr)));
            self.f32s_add(sqr, sqr_rev)
        }
    }

    #[inline(always)]
    fn c32s_reduce_sum(self, a: Self::c32s) -> c32 {
        unsafe {
            // a0 a1 a2 a3
            let a = transmute(a);
            // a2 a3 a2 a3
            let hi = vcombine_u64(vget_high_u64(a), vget_low_u64(a));

            // a0+a2 a1+a3 _ _
            cast_lossy(self.f32s_add(transmute(a), transmute(hi)))
        }
    }

    #[inline(always)]
    fn f64s_splat(self, value: f64) -> Self::f64s {
        self.splat_f64x2(value)
    }

    #[inline(always)]
    fn f64s_add(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.add_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_sub(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.sub_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_mul(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.mul_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_div(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.div_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_mul_add(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        self.mul_add_f64x2(a, b, c)
    }

    #[inline(always)]
    fn f64s_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        self.cmp_eq_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_less_than(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        self.cmp_lt_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_less_than_or_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        self.cmp_le_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_min(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.min_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_max(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.max_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_reduce_sum(self, a: Self::f64s) -> f64 {
        self.reduce_sum_f64x2(a)
    }

    #[inline(always)]
    fn f64s_reduce_product(self, a: Self::f64s) -> f64 {
        self.reduce_product_f64x2(a)
    }

    #[inline(always)]
    fn f64s_reduce_min(self, a: Self::f64s) -> f64 {
        self.reduce_min_f64x2(a)
    }

    #[inline(always)]
    fn f64s_reduce_max(self, a: Self::f64s) -> f64 {
        self.reduce_max_f64x2(a)
    }

    #[inline(always)]
    fn c64s_splat(self, value: c64) -> Self::c64s {
        cast(value)
    }

    #[inline(always)]
    fn c64s_conj(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.c64s_splat(c64 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c64s_neg(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.f64s_splat(-0.0))
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

            let yx = transmute(vcombine_u64(
                vget_high_u64(transmute(xy)),
                vget_low_u64(transmute(xy)),
            ));
            let aa = vtrn1q_f64(ab, ab);
            let bb = vtrn2q_f64(ab, ab);

            let bb_sign = transmute(veorq_u64(transmute(f64x2(-0.0, 0.0)), transmute(bb)));

            transmute(vfmaq_f64(vmulq_f64(bb_sign, yx), aa, xy))
        }
    }

    #[inline(always)]
    fn c64s_conj_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = transmute(vcombine_u64(
                vget_high_u64(transmute(xy)),
                vget_low_u64(transmute(xy)),
            ));
            let aa = vtrn1q_f64(ab, ab);
            let bb = vtrn2q_f64(ab, ab);

            let bb_sign = transmute(veorq_u64(transmute(f64x2(0.0, -0.0)), transmute(bb)));

            transmute(vfmaq_f64(vmulq_f64(bb_sign, yx), aa, xy))
        }
    }

    #[inline(always)]
    fn c64s_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = transmute(vcombine_u64(
                vget_high_u64(transmute(xy)),
                vget_low_u64(transmute(xy)),
            ));
            let aa = vtrn1q_f64(ab, ab);
            let bb = vtrn2q_f64(ab, ab);

            let bb_sign = transmute(veorq_u64(transmute(f64x2(-0.0, 0.0)), transmute(bb)));

            transmute(vfmaq_f64(vfmaq_f64(transmute(c), bb_sign, yx), aa, xy))
        }
    }

    #[inline(always)]
    fn c64s_conj_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        unsafe {
            let ab = transmute(a);
            let xy = transmute(b);

            let yx = transmute(vcombine_u64(
                vget_high_u64(transmute(xy)),
                vget_low_u64(transmute(xy)),
            ));
            let aa = vtrn1q_f64(ab, ab);
            let bb = vtrn2q_f64(ab, ab);

            let bb_sign = transmute(veorq_u64(transmute(f64x2(0.0, -0.0)), transmute(bb)));

            transmute(vfmaq_f64(vfmaq_f64(transmute(c), bb_sign, yx), aa, xy))
        }
    }

    #[inline(always)]
    fn c64s_abs2(self, a: Self::c64s) -> Self::c64s {
        unsafe {
            let sqr = self.f64s_mul(a, a);
            let sqr_rev = transmute(vcombine_u64(
                vget_high_u64(transmute(sqr)),
                vget_low_u64(transmute(sqr)),
            ));
            self.f64s_add(sqr, sqr_rev)
        }
    }

    #[inline(always)]
    fn c64s_reduce_sum(self, a: Self::c64s) -> c64 {
        cast(a)
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
        u32x4(
            if mask.0.is_set() {
                *ptr.wrapping_add(0)
            } else {
                or.0
            },
            if mask.1.is_set() {
                *ptr.wrapping_add(1)
            } else {
                or.1
            },
            if mask.2.is_set() {
                *ptr.wrapping_add(2)
            } else {
                or.2
            },
            if mask.3.is_set() {
                *ptr.wrapping_add(3)
            } else {
                or.3
            },
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
        let ptr = ptr as *const f32;
        f32x4(
            if mask.0.is_set() {
                *ptr.wrapping_add(0)
            } else {
                or.0
            },
            if mask.1.is_set() {
                *ptr.wrapping_add(1)
            } else {
                or.1
            },
            if mask.2.is_set() {
                *ptr.wrapping_add(2)
            } else {
                or.2
            },
            if mask.3.is_set() {
                *ptr.wrapping_add(3)
            } else {
                or.3
            },
        )
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut u32, values: Self::u32s) {
        if mask.0.is_set() {
            *ptr.wrapping_add(0) = values.0
        }
        if mask.1.is_set() {
            *ptr.wrapping_add(1) = values.1
        }
        if mask.2.is_set() {
            *ptr.wrapping_add(2) = values.2
        }
        if mask.3.is_set() {
            *ptr.wrapping_add(3) = values.3
        }
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut c32, values: Self::c32s) {
        let ptr = ptr as *mut f32;
        if mask.0.is_set() {
            *ptr.wrapping_add(0) = values.0
        }
        if mask.1.is_set() {
            *ptr.wrapping_add(1) = values.1
        }
        if mask.2.is_set() {
            *ptr.wrapping_add(2) = values.2
        }
        if mask.3.is_set() {
            *ptr.wrapping_add(3) = values.3
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
        u64x2(
            if mask.0.is_set() {
                *ptr.wrapping_add(0)
            } else {
                or.0
            },
            if mask.1.is_set() {
                *ptr.wrapping_add(1)
            } else {
                or.1
            },
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
        let ptr = ptr as *const f64;
        f64x2(
            if mask.0.is_set() {
                *ptr.wrapping_add(0)
            } else {
                or.0
            },
            if mask.1.is_set() {
                *ptr.wrapping_add(1)
            } else {
                or.1
            },
        )
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut u64, values: Self::u64s) {
        if mask.0.is_set() {
            *ptr.wrapping_add(0) = values.0
        }
        if mask.1.is_set() {
            *ptr.wrapping_add(1) = values.1
        }
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut c64, values: Self::c64s) {
        let ptr = ptr as *mut f64;
        if mask.0.is_set() {
            *ptr.wrapping_add(0) = values.0
        }
        if mask.1.is_set() {
            *ptr.wrapping_add(1) = values.1
        }
    }

    #[inline(always)]
    fn u64s_less_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_lt_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_gt_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_less_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_le_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_ge_u64x2(a, b)
    }

    #[inline(always)]
    fn u32s_rotate_right(self, a: Self::u32s, amount: usize) -> Self::u32s {
        unsafe {
            transmute(vqtbl1q_u8(
                transmute(a),
                transmute(NEON_ROTATE_IDX[4 * (amount % 4)]),
            ))
        }
    }

    #[inline(always)]
    fn c32s_rotate_right(self, a: Self::c32s, amount: usize) -> Self::c32s {
        unsafe {
            transmute(vqtbl1q_u8(
                transmute(a),
                transmute(NEON_ROTATE_IDX[8 * (amount % 2)]),
            ))
        }
    }

    #[inline(always)]
    fn u64s_rotate_right(self, a: Self::u64s, amount: usize) -> Self::u64s {
        unsafe {
            transmute(vqtbl1q_u8(
                transmute(a),
                transmute(NEON_ROTATE_IDX[8 * (amount % 2)]),
            ))
        }
    }

    #[inline(always)]
    fn c64s_rotate_right(self, a: Self::c64s, _amount: usize) -> Self::c64s {
        a
    }

    #[inline(always)]
    fn c32s_swap_re_im(self, a: Self::c32s) -> Self::c32s {
        unsafe { transmute(vrev64q_f32(transmute(a))) }
    }
    #[inline(always)]
    fn c64s_swap_re_im(self, a: Self::c64s) -> Self::c64s {
        unsafe {
            transmute(vcombine_u64(
                vget_high_u64(transmute(a)),
                vget_low_u64(transmute(a)),
            ))
        }
    }
}

impl Simd for NeonFcma {
    type m32s = m32x4;
    type f32s = f32x4;
    type c32s = f32x4;
    type i32s = i32x4;
    type u32s = u32x4;

    type m64s = m64x2;
    type f64s = f64x2;
    type c64s = f64x2;
    type i64s = i64x2;
    type u64s = u64x2;

    #[inline(always)]
    fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
        struct Impl<Op> {
            this: NeonFcma,
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
    fn u32s_partial_load(self, slice: &[u32]) -> Self::u32s {
        match slice.len() {
            0 => u32x4(0, 0, 0, 0),
            1 => u32x4(slice[0], 0, 0, 0),
            2 => u32x4(slice[0], slice[1], 0, 0),
            3 => u32x4(slice[0], slice[1], slice[2], 0),
            _ => u32x4(slice[0], slice[1], slice[2], slice[3]),
        }
    }

    #[inline(always)]
    fn u32s_partial_store(self, slice: &mut [u32], values: Self::u32s) {
        match slice.len() {
            0 => {}
            1 => {
                slice[0] = values.0;
            }
            2 => {
                slice[0] = values.0;
                slice[1] = values.1;
            }
            3 => {
                slice[0] = values.0;
                slice[1] = values.1;
                slice[2] = values.2;
            }
            _ => {
                slice[0] = values.0;
                slice[1] = values.1;
                slice[2] = values.2;
                slice[3] = values.3;
            }
        }
    }

    #[inline(always)]
    fn u64s_partial_load(self, slice: &[u64]) -> Self::u64s {
        match slice.len() {
            0 => u64x2(0, 0),
            1 => u64x2(slice[0], 0),
            _ => u64x2(slice[0], slice[1]),
        }
    }

    #[inline(always)]
    fn u64s_partial_store(self, slice: &mut [u64], values: Self::u64s) {
        match slice.len() {
            0 => {}
            1 => {
                slice[0] = values.0;
            }
            _ => {
                slice[0] = values.0;
                slice[1] = values.1;
            }
        }
    }

    #[inline(always)]
    fn u32s_partial_load_last(self, slice: &[u32]) -> Self::u32s {
        match slice.len() {
            0 => u32x4(0, 0, 0, 0),
            1 => u32x4(0, 0, 0, slice[0]),
            2 => u32x4(0, 0, slice[0], slice[1]),
            3 => u32x4(0, slice[0], slice[1], slice[2]),
            _ => {
                let slice = &slice[slice.len() - 4..];
                u32x4(slice[0], slice[1], slice[2], slice[3])
            }
        }
    }

    #[inline(always)]
    fn u32s_partial_store_last(self, slice: &mut [u32], values: Self::u32s) {
        match slice.len() {
            0 => {}
            1 => {
                slice[0] = values.3;
            }
            2 => {
                slice[0] = values.2;
                slice[1] = values.3;
            }
            3 => {
                slice[0] = values.1;
                slice[1] = values.2;
                slice[2] = values.3;
            }
            _ => {
                let len = slice.len();
                let slice = &mut slice[len - 4..];
                slice[0] = values.0;
                slice[1] = values.1;
                slice[2] = values.2;
                slice[3] = values.3;
            }
        }
    }

    #[inline(always)]
    fn u64s_partial_load_last(self, slice: &[u64]) -> Self::u64s {
        match slice.len() {
            0 => u64x2(0, 0),
            1 => u64x2(0, slice[0]),
            _ => {
                let slice = &slice[slice.len() - 2..];
                u64x2(slice[0], slice[1])
            }
        }
    }

    #[inline(always)]
    fn u64s_partial_store_last(self, slice: &mut [u64], values: Self::u64s) {
        match slice.len() {
            0 => {}
            1 => {
                slice[0] = values.1;
            }
            _ => {
                let len = slice.len();
                let slice = &mut slice[len - 2..];
                slice[0] = values.0;
                slice[1] = values.1;
            }
        }
    }

    #[inline(always)]
    fn m32s_not(self, a: Self::m32s) -> Self::m32s {
        self.not_m32x4(a)
    }

    #[inline(always)]
    fn m32s_and(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        self.and_m32x4(a, b)
    }

    #[inline(always)]
    fn m32s_or(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        self.or_m32x4(a, b)
    }

    #[inline(always)]
    fn m32s_xor(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        self.xor_m32x4(a, b)
    }

    #[inline(always)]
    fn m64s_not(self, a: Self::m64s) -> Self::m64s {
        self.not_m64x2(a)
    }

    #[inline(always)]
    fn m64s_and(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        self.and_m64x2(a, b)
    }

    #[inline(always)]
    fn m64s_or(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        self.or_m64x2(a, b)
    }

    #[inline(always)]
    fn m64s_xor(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        self.xor_m64x2(a, b)
    }

    #[inline(always)]
    fn u32s_not(self, a: Self::u32s) -> Self::u32s {
        self.not_u32x4(a)
    }

    #[inline(always)]
    fn u32s_and(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.and_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_or(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.or_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_xor(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.xor_u32x4(a, b)
    }

    #[inline(always)]
    fn u64s_not(self, a: Self::u64s) -> Self::u64s {
        self.not_u64x2(a)
    }

    #[inline(always)]
    fn u64s_and(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.and_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_or(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.or_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_xor(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.xor_u64x2(a, b)
    }

    #[inline(always)]
    fn m32s_select_u32s(
        self,
        mask: Self::m32s,
        if_true: Self::u32s,
        if_false: Self::u32s,
    ) -> Self::u32s {
        self.select_u32x4(mask, if_true, if_false)
    }

    #[inline(always)]
    fn m64s_select_u64s(
        self,
        mask: Self::m64s,
        if_true: Self::u64s,
        if_false: Self::u64s,
    ) -> Self::u64s {
        self.select_u64x2(mask, if_true, if_false)
    }

    #[inline(always)]
    fn u32s_splat(self, value: u32) -> Self::u32s {
        self.splat_u32x4(value)
    }

    #[inline(always)]
    fn u32s_add(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.wrapping_add_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_sub(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        self.wrapping_sub_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_less_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_lt_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_gt_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_less_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_le_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_greater_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        self.cmp_ge_u32x4(a, b)
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shl(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        self.shl_dyn_u32x4(a, self.and_i32x4(cast(amount), self.splat_i32x4(32 - 1)))
    }

    #[inline(always)]
    fn u32s_wrapping_dyn_shr(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        u32x4(
            a.0 >> amount.0,
            a.1 >> amount.1,
            a.2 >> amount.2,
            a.3 >> amount.3,
        )
    }

    #[inline(always)]
    fn u32s_widening_mul(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
        #[inline(always)]
        fn widen_mul(a: u32, b: u32) -> (u32, u32) {
            let a = a as u64;
            let b = b as u64;
            let c = a * b;

            (c as u32, (c >> 32) as u32)
        }

        let (c0, d0) = widen_mul(a.0, b.0);
        let (c1, d1) = widen_mul(a.1, b.1);
        let (c2, d2) = widen_mul(a.2, b.2);
        let (c3, d3) = widen_mul(a.3, b.3);

        (u32x4(c0, c1, c2, c3), u32x4(d0, d1, d2, d3))
    }

    #[inline(always)]
    fn u64s_splat(self, value: u64) -> Self::u64s {
        self.splat_u64x2(value)
    }

    #[inline(always)]
    fn u64s_add(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.wrapping_add_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_sub(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        self.wrapping_sub_u64x2(a, b)
    }

    #[inline(always)]
    fn f32s_splat(self, value: f32) -> Self::f32s {
        self.splat_f32x4(value)
    }

    #[inline(always)]
    fn f32s_add(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.add_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_sub(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.sub_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_mul(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.mul_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_div(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.div_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_mul_add(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        self.mul_add_f32x4(a, b, c)
    }

    #[inline(always)]
    fn f32s_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        self.cmp_eq_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_less_than(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        self.cmp_lt_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_less_than_or_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        self.cmp_le_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_min(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.min_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_max(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.max_f32x4(a, b)
    }

    #[inline(always)]
    fn f32s_reduce_sum(self, a: Self::f32s) -> f32 {
        self.reduce_sum_f32x4(a)
    }

    #[inline(always)]
    fn f32s_reduce_product(self, a: Self::f32s) -> f32 {
        self.reduce_product_f32x4(a)
    }

    #[inline(always)]
    fn f32s_reduce_min(self, a: Self::f32s) -> f32 {
        self.reduce_min_f32x4(a)
    }

    #[inline(always)]
    fn f32s_reduce_max(self, a: Self::f32s) -> f32 {
        self.reduce_max_f32x4(a)
    }

    #[inline(always)]
    fn c32s_splat(self, value: c32) -> Self::c32s {
        cast([value; 2])
    }

    #[inline(always)]
    fn c32s_conj(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.c32s_splat(c32 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c32s_neg(self, a: Self::c32s) -> Self::c32s {
        self.f32s_xor(a, self.f32s_splat(-0.0))
    }

    #[inline(always)]
    fn c32s_add(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.add_f32x4(a, b)
    }

    #[inline(always)]
    fn c32s_sub(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.sub_f32x4(a, b)
    }

    #[inline(always)]
    fn c32s_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.c32s_mul_add(a, b, self.f32s_splat(0.0))
    }

    #[inline(always)]
    fn c32s_conj_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.c32s_conj_mul_add(a, b, self.f32s_splat(0.0))
    }

    #[inline(always)]
    fn c32s_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        let a = cast(a);
        let b = cast(b);
        let c = cast(c);
        unsafe { cast(vcmlaq_90_f32(vcmlaq_0_f32(c, a, b), a, b)) }
    }

    #[inline(always)]
    fn c32s_conj_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        let a = cast(a);
        let b = cast(b);
        let c = cast(c);
        unsafe { cast(vcmlaq_270_f32(vcmlaq_0_f32(c, a, b), a, b)) }
    }

    #[inline(always)]
    fn c32s_abs2(self, a: Self::c32s) -> Self::c32s {
        unsafe {
            let sqr = self.f32s_mul(a, a);
            let sqr_rev = transmute(vrev64q_f32(transmute(sqr)));
            self.f32s_add(sqr, sqr_rev)
        }
    }

    #[inline(always)]
    fn c32s_reduce_sum(self, a: Self::c32s) -> c32 {
        unsafe {
            // a0 a1 a2 a3
            let a = transmute(a);
            // a2 a3 a2 a3
            let hi = vcombine_u64(vget_high_u64(a), vget_low_u64(a));

            // a0+a2 a1+a3 _ _
            cast_lossy(self.f32s_add(transmute(a), transmute(hi)))
        }
    }

    #[inline(always)]
    fn f64s_splat(self, value: f64) -> Self::f64s {
        self.splat_f64x2(value)
    }

    #[inline(always)]
    fn f64s_add(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.add_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_sub(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.sub_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_mul(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.mul_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_div(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.div_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_mul_add(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        self.mul_add_f64x2(a, b, c)
    }

    #[inline(always)]
    fn f64s_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        self.cmp_eq_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_less_than(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        self.cmp_lt_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_less_than_or_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        self.cmp_le_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_min(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.min_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_max(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.max_f64x2(a, b)
    }

    #[inline(always)]
    fn f64s_reduce_sum(self, a: Self::f64s) -> f64 {
        self.reduce_sum_f64x2(a)
    }

    #[inline(always)]
    fn f64s_reduce_product(self, a: Self::f64s) -> f64 {
        self.reduce_product_f64x2(a)
    }

    #[inline(always)]
    fn f64s_reduce_min(self, a: Self::f64s) -> f64 {
        self.reduce_min_f64x2(a)
    }

    #[inline(always)]
    fn f64s_reduce_max(self, a: Self::f64s) -> f64 {
        self.reduce_max_f64x2(a)
    }

    #[inline(always)]
    fn c64s_splat(self, value: c64) -> Self::c64s {
        cast(value)
    }

    #[inline(always)]
    fn c64s_conj(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.c64s_splat(c64 { re: 0.0, im: -0.0 }))
    }

    #[inline(always)]
    fn c64s_neg(self, a: Self::c64s) -> Self::c64s {
        self.f64s_xor(a, self.f64s_splat(-0.0))
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
        self.c64s_mul_add(a, b, self.f64s_splat(0.0))
    }

    #[inline(always)]
    fn c64s_conj_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.c64s_conj_mul_add(a, b, self.f64s_splat(0.0))
    }

    #[inline(always)]
    fn c64s_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        let a = cast(a);
        let b = cast(b);
        let c = cast(c);
        unsafe { cast(vcmlaq_90_f64(vcmlaq_0_f64(c, a, b), a, b)) }
    }

    #[inline(always)]
    fn c64s_conj_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        let a = cast(a);
        let b = cast(b);
        let c = cast(c);
        unsafe { cast(vcmlaq_270_f64(vcmlaq_0_f64(c, a, b), a, b)) }
    }

    #[inline(always)]
    fn c64s_abs2(self, a: Self::c64s) -> Self::c64s {
        unsafe {
            let sqr = self.f64s_mul(a, a);
            let sqr_rev = transmute(vcombine_u64(
                vget_high_u64(transmute(sqr)),
                vget_low_u64(transmute(sqr)),
            ));
            self.f64s_add(sqr, sqr_rev)
        }
    }

    #[inline(always)]
    fn c64s_reduce_sum(self, a: Self::c64s) -> c64 {
        cast(a)
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
        u32x4(
            if mask.0.is_set() {
                *ptr.wrapping_add(0)
            } else {
                or.0
            },
            if mask.1.is_set() {
                *ptr.wrapping_add(1)
            } else {
                or.1
            },
            if mask.2.is_set() {
                *ptr.wrapping_add(2)
            } else {
                or.2
            },
            if mask.3.is_set() {
                *ptr.wrapping_add(3)
            } else {
                or.3
            },
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
        let ptr = ptr as *const f32;
        f32x4(
            if mask.0.is_set() {
                *ptr.wrapping_add(0)
            } else {
                or.0
            },
            if mask.1.is_set() {
                *ptr.wrapping_add(1)
            } else {
                or.1
            },
            if mask.2.is_set() {
                *ptr.wrapping_add(2)
            } else {
                or.2
            },
            if mask.3.is_set() {
                *ptr.wrapping_add(3)
            } else {
                or.3
            },
        )
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut u32, values: Self::u32s) {
        if mask.0.is_set() {
            *ptr.wrapping_add(0) = values.0
        }
        if mask.1.is_set() {
            *ptr.wrapping_add(1) = values.1
        }
        if mask.2.is_set() {
            *ptr.wrapping_add(2) = values.2
        }
        if mask.3.is_set() {
            *ptr.wrapping_add(3) = values.3
        }
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut c32, values: Self::c32s) {
        let ptr = ptr as *mut f32;
        if mask.0.is_set() {
            *ptr.wrapping_add(0) = values.0
        }
        if mask.1.is_set() {
            *ptr.wrapping_add(1) = values.1
        }
        if mask.2.is_set() {
            *ptr.wrapping_add(2) = values.2
        }
        if mask.3.is_set() {
            *ptr.wrapping_add(3) = values.3
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
        u64x2(
            if mask.0.is_set() {
                *ptr.wrapping_add(0)
            } else {
                or.0
            },
            if mask.1.is_set() {
                *ptr.wrapping_add(1)
            } else {
                or.1
            },
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
        let ptr = ptr as *const f64;
        f64x2(
            if mask.0.is_set() {
                *ptr.wrapping_add(0)
            } else {
                or.0
            },
            if mask.1.is_set() {
                *ptr.wrapping_add(1)
            } else {
                or.1
            },
        )
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn u64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut u64, values: Self::u64s) {
        if mask.0.is_set() {
            *ptr.wrapping_add(0) = values.0
        }
        if mask.1.is_set() {
            *ptr.wrapping_add(1) = values.1
        }
    }

    /// # Safety
    ///
    /// See the trait-level safety documentation.
    #[inline(always)]
    unsafe fn c64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut c64, values: Self::c64s) {
        let ptr = ptr as *mut f64;
        if mask.0.is_set() {
            *ptr.wrapping_add(0) = values.0
        }
        if mask.1.is_set() {
            *ptr.wrapping_add(1) = values.1
        }
    }

    #[inline(always)]
    fn u64s_less_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_lt_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_gt_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_less_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_le_u64x2(a, b)
    }

    #[inline(always)]
    fn u64s_greater_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        self.cmp_ge_u64x2(a, b)
    }

    #[inline(always)]
    fn u32s_rotate_right(self, a: Self::u32s, amount: usize) -> Self::u32s {
        unsafe {
            transmute(vqtbl1q_u8(
                transmute(a),
                transmute(NEON_ROTATE_IDX[4 * (amount % 4)]),
            ))
        }
    }

    #[inline(always)]
    fn c32s_rotate_right(self, a: Self::c32s, amount: usize) -> Self::c32s {
        unsafe {
            transmute(vqtbl1q_u8(
                transmute(a),
                transmute(NEON_ROTATE_IDX[8 * (amount % 2)]),
            ))
        }
    }

    #[inline(always)]
    fn u64s_rotate_right(self, a: Self::u64s, amount: usize) -> Self::u64s {
        unsafe {
            transmute(vqtbl1q_u8(
                transmute(a),
                transmute(NEON_ROTATE_IDX[8 * (amount % 2)]),
            ))
        }
    }

    #[inline(always)]
    fn c64s_rotate_right(self, a: Self::c64s, _amount: usize) -> Self::c64s {
        a
    }

    #[inline(always)]
    fn c32s_swap_re_im(self, a: Self::c32s) -> Self::c32s {
        unsafe { transmute(vrev64q_f32(transmute(a))) }
    }
    #[inline(always)]
    fn c64s_swap_re_im(self, a: Self::c64s) -> Self::c64s {
        unsafe {
            transmute(vcombine_u64(
                vget_high_u64(transmute(a)),
                vget_low_u64(transmute(a)),
            ))
        }
    }
}

#[cfg(miri)]
unsafe fn vfmaq_f64(c: float64x2_t, a: float64x2_t, b: float64x2_t) -> float64x2_t {
    let c: f64x2 = transmute(c);
    let a: f64x2 = transmute(a);
    let b: f64x2 = transmute(b);

    transmute(f64x2(
        f64::mul_add(a.0, b.0, c.0),
        f64::mul_add(a.1, b.1, c.1),
    ))
}

#[cfg(miri)]
unsafe fn vfmaq_f32(c: float32x4_t, a: float32x4_t, b: float32x4_t) -> float32x4_t {
    let c: f32x4 = transmute(c);
    let a: f32x4 = transmute(a);
    let b: f32x4 = transmute(b);

    transmute(f32x4(
        f32::mul_add(a.0, b.0, c.0),
        f32::mul_add(a.1, b.1, c.1),
        f32::mul_add(a.2, b.2, c.2),
        f32::mul_add(a.3, b.3, c.3),
    ))
}

impl Neon {
    //-------------------------------------------------------------------------------
    // splat
    //-------------------------------------------------------------------------------

    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u8x16(self, value: u8) -> u8x16 {
        cast([value; 16])
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i8x16(self, value: i8) -> i8x16 {
        cast([value; 16])
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m8x16(self, value: m8) -> m8x16 {
        unsafe { transmute([value; 16]) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u16x8(self, value: u16) -> u16x8 {
        cast([value; 8])
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i16x8(self, value: i16) -> i16x8 {
        cast([value; 8])
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m16x8(self, value: m16) -> m16x8 {
        unsafe { transmute([value; 8]) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u32x4(self, value: u32) -> u32x4 {
        cast([value; 4])
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i32x4(self, value: i32) -> i32x4 {
        cast([value; 4])
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m32x4(self, value: m32) -> m32x4 {
        unsafe { transmute([value; 4]) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_f32x4(self, value: f32) -> f32x4 {
        cast([value; 4])
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_u64x2(self, value: u64) -> u64x2 {
        cast([value; 2])
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_i64x2(self, value: i64) -> i64x2 {
        cast([value; 2])
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_m64x2(self, value: m64) -> m64x2 {
        unsafe { transmute([value; 2]) }
    }
    /// Returns a SIMD vector with all lanes set to the given value.
    #[inline(always)]
    pub fn splat_f64x2(self, value: f64) -> f64x2 {
        cast([value; 2])
    }

    //-------------------------------------------------------------------------------
    // bitwise
    //-------------------------------------------------------------------------------

    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        unsafe { transmute(vandq_u8(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        unsafe { transmute(vandq_s8(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
        unsafe { transmute(vandq_u8(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        unsafe { transmute(vandq_u16(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        unsafe { transmute(vandq_s16(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
        unsafe { transmute(vandq_u16(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        unsafe { transmute(vandq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        unsafe { transmute(vandq_s32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
        unsafe { transmute(vandq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        unsafe { transmute(vandq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        unsafe { transmute(vandq_u64(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        unsafe { transmute(vandq_s64(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
        unsafe { transmute(vandq_u64(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise AND of `a` and `b`.
    #[inline(always)]
    pub fn and_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        unsafe { transmute(vandq_u64(transmute(a), transmute(b))) }
    }

    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        unsafe { transmute(vorrq_u8(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        unsafe { transmute(vorrq_u8(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
        unsafe { transmute(vorrq_u8(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        unsafe { transmute(vorrq_u16(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        unsafe { transmute(vorrq_u16(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
        unsafe { transmute(vorrq_u16(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        unsafe { transmute(vorrq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        unsafe { transmute(vorrq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
        unsafe { transmute(vorrq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        unsafe { transmute(vorrq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        unsafe { transmute(vorrq_u64(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        unsafe { transmute(vorrq_u64(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
        unsafe { transmute(vorrq_u64(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise OR of `a` and `b`.
    #[inline(always)]
    pub fn or_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        unsafe { transmute(vorrq_u64(transmute(a), transmute(b))) }
    }

    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        unsafe { transmute(veorq_u8(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        unsafe { transmute(veorq_u8(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
        unsafe { transmute(veorq_u8(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        unsafe { transmute(veorq_u16(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        unsafe { transmute(veorq_u16(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
        unsafe { transmute(veorq_u16(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        unsafe { transmute(veorq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        unsafe { transmute(veorq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
        unsafe { transmute(veorq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        unsafe { transmute(veorq_u32(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        unsafe { transmute(veorq_u64(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        unsafe { transmute(veorq_u64(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
        unsafe { transmute(veorq_u64(transmute(a), transmute(b))) }
    }
    /// Returns the bitwise XOR of `a` and `b`.
    #[inline(always)]
    pub fn xor_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        unsafe { transmute(veorq_u64(transmute(a), transmute(b))) }
    }

    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u8x16(self, a: u8x16) -> u8x16 {
        unsafe { transmute(vmvnq_u8(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i8x16(self, a: i8x16) -> i8x16 {
        unsafe { transmute(vmvnq_u8(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m8x16(self, a: m8x16) -> m8x16 {
        unsafe { transmute(vmvnq_u8(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u16x8(self, a: u16x8) -> u16x8 {
        unsafe { transmute(vmvnq_u16(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i16x8(self, a: i16x8) -> i16x8 {
        unsafe { transmute(vmvnq_u16(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m16x8(self, a: m16x8) -> m16x8 {
        unsafe { transmute(vmvnq_u16(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u32x4(self, a: u32x4) -> u32x4 {
        unsafe { transmute(vmvnq_u32(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i32x4(self, a: i32x4) -> i32x4 {
        unsafe { transmute(vmvnq_u32(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m32x4(self, a: m32x4) -> m32x4 {
        unsafe { transmute(vmvnq_u32(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_u64x2(self, a: u64x2) -> u64x2 {
        unsafe { transmute(vmvnq_u32(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_i64x2(self, a: i64x2) -> i64x2 {
        unsafe { transmute(vmvnq_u32(transmute(a))) }
    }
    /// Returns the bitwise NOT of `a`.
    #[inline(always)]
    pub fn not_m64x2(self, a: m64x2) -> m64x2 {
        unsafe { transmute(vmvnq_u32(transmute(a))) }
    }

    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        self.and_u8x16(self.not_u8x16(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        self.and_i8x16(self.not_i8x16(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
        self.and_m8x16(self.not_m8x16(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        self.and_u16x8(self.not_u16x8(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        self.and_i16x8(self.not_i16x8(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
        self.and_m16x8(self.not_m16x8(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        self.and_u32x4(self.not_u32x4(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        self.and_i32x4(self.not_i32x4(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
        self.and_m32x4(self.not_m32x4(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        self.and_f32x4(cast(self.not_u32x4(cast(a))), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        self.and_u64x2(self.not_u64x2(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        self.and_i64x2(self.not_i64x2(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
        self.and_m64x2(self.not_m64x2(a), b)
    }
    /// Returns the bitwise AND of NOT `a` and `b`.
    #[inline(always)]
    pub fn andnot_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        self.and_f64x2(cast(self.not_u64x2(cast(a))), b)
    }

    //-------------------------------------------------------------------------------
    // bit shifts
    //-------------------------------------------------------------------------------

    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_const_u8x16<const AMOUNT: i32>(self, a: u8x16) -> u8x16 {
        unsafe { transmute(vshlq_n_u8::<AMOUNT>(transmute(a))) }
    }

    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_const_i8x16<const AMOUNT: i32>(self, a: i8x16) -> i8x16 {
        unsafe { transmute(vshlq_n_s8::<AMOUNT>(transmute(a))) }
    }

    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_const_u16x8<const AMOUNT: i32>(self, a: u16x8) -> u16x8 {
        unsafe { transmute(vshlq_n_u16::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_const_i16x8<const AMOUNT: i32>(self, a: i16x8) -> i16x8 {
        unsafe { transmute(vshlq_n_s16::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_const_u32x4<const AMOUNT: i32>(self, a: u32x4) -> u32x4 {
        unsafe { transmute(vshlq_n_u32::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_const_i32x4<const AMOUNT: i32>(self, a: i32x4) -> i32x4 {
        unsafe { transmute(vshlq_n_s32::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_const_u64x2<const AMOUNT: i32>(self, a: u64x2) -> u64x2 {
        unsafe { transmute(vshlq_n_u64::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_const_i64x2<const AMOUNT: i32>(self, a: i64x2) -> i64x2 {
        unsafe { transmute(vshlq_n_s64::<AMOUNT>(transmute(a))) }
    }

    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shr_const_u8x16<const AMOUNT: i32>(self, a: u8x16) -> u8x16 {
        unsafe { transmute(vshrq_n_u8::<AMOUNT>(transmute(a))) }
    }

    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in sign bits.  
    #[inline(always)]
    pub fn shr_const_i8x16<const AMOUNT: i32>(self, a: i8x16) -> i8x16 {
        unsafe { transmute(vshrq_n_s8::<AMOUNT>(transmute(a))) }
    }

    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shr_const_u16x8<const AMOUNT: i32>(self, a: u16x8) -> u16x8 {
        unsafe { transmute(vshrq_n_u16::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in sign bits.  
    #[inline(always)]
    pub fn shr_const_i16x8<const AMOUNT: i32>(self, a: i16x8) -> i16x8 {
        unsafe { transmute(vshrq_n_s16::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shr_const_u32x4<const AMOUNT: i32>(self, a: u32x4) -> u32x4 {
        unsafe { transmute(vshrq_n_u32::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in sign bits.  
    #[inline(always)]
    pub fn shr_const_i32x4<const AMOUNT: i32>(self, a: i32x4) -> i32x4 {
        unsafe { transmute(vshrq_n_s32::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
    #[inline(always)]
    pub fn shr_const_u64x2<const AMOUNT: i32>(self, a: u64x2) -> u64x2 {
        unsafe { transmute(vshrq_n_u64::<AMOUNT>(transmute(a))) }
    }
    /// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in sign bits.  
    #[inline(always)]
    pub fn shr_const_i64x2<const AMOUNT: i32>(self, a: i64x2) -> i64x2 {
        unsafe { transmute(vshrq_n_s64::<AMOUNT>(transmute(a))) }
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_dyn_u8x16(self, a: u8x16, amount: i8x16) -> u8x16 {
        unsafe { transmute(vshlq_u8(transmute(a), transmute(amount))) }
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_dyn_u16x8(self, a: u16x8, amount: i16x8) -> u16x8 {
        unsafe { transmute(vshlq_u16(transmute(a), transmute(amount))) }
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_dyn_u32x4(self, a: u32x4, amount: i32x4) -> u32x4 {
        unsafe { transmute(vshlq_u32(transmute(a), transmute(amount))) }
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_dyn_u64x2(self, a: u64x2, amount: i64x2) -> u64x2 {
        unsafe { transmute(vshlq_u64(transmute(a), transmute(amount))) }
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_dyn_i8x16(self, a: i8x16, amount: i8x16) -> i8x16 {
        unsafe { transmute(vshlq_u8(transmute(a), transmute(amount))) }
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_dyn_i16x8(self, a: i16x8, amount: i16x8) -> i16x8 {
        unsafe { transmute(vshlq_u16(transmute(a), transmute(amount))) }
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_dyn_i32x4(self, a: i32x4, amount: i32x4) -> i32x4 {
        unsafe { transmute(vshlq_u32(transmute(a), transmute(amount))) }
    }

    /// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
    /// `amount`, while shifting in zeros.  
    #[inline(always)]
    pub fn shl_dyn_i64x2(self, a: i64x2, amount: i64x2) -> i64x2 {
        unsafe { transmute(vshlq_u64(transmute(a), transmute(amount))) }
    }

    //-------------------------------------------------------------------------------
    // arithmetic
    //-------------------------------------------------------------------------------

    /// Adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn add_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        unsafe { transmute(vaddq_f32(transmute(a), transmute(b))) }
    }
    /// Adds the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn add_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        unsafe { transmute(vaddq_f64(transmute(a), transmute(b))) }
    }

    /// Subtracts the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn sub_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        unsafe { transmute(vsubq_f32(transmute(a), transmute(b))) }
    }
    /// Subtracts the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn sub_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        unsafe { transmute(vsubq_f64(transmute(a), transmute(b))) }
    }

    /// Multiplies the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn mul_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        unsafe { transmute(vmulq_f32(transmute(a), transmute(b))) }
    }
    /// Multiplies the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn mul_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        unsafe { transmute(vmulq_f64(transmute(a), transmute(b))) }
    }

    /// Multiplies the elements of each lane of `a` and `b` and adds the result to `c`.
    #[inline(always)]
    pub fn mul_add_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
        unsafe { transmute(vfmaq_f32(transmute(c), transmute(a), transmute(b))) }
    }
    /// Multiplies the elements of each lane of `a` and `b` and adds the result to `c`.
    #[inline(always)]
    pub fn mul_add_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
        unsafe { transmute(vfmaq_f64(transmute(c), transmute(a), transmute(b))) }
    }

    /// Divides the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn div_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        unsafe { transmute(vdivq_f32(transmute(a), transmute(b))) }
    }
    /// Divides the elements of each lane of `a` and `b`.
    #[inline(always)]
    pub fn div_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        unsafe { transmute(vdivq_f64(transmute(a), transmute(b))) }
    }

    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        unsafe { transmute(vaddq_u8(transmute(a), transmute(b))) }
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        unsafe { transmute(vaddq_s8(transmute(a), transmute(b))) }
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        unsafe { transmute(vaddq_u16(transmute(a), transmute(b))) }
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        unsafe { transmute(vaddq_s16(transmute(a), transmute(b))) }
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        unsafe { transmute(vaddq_u32(transmute(a), transmute(b))) }
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        unsafe { transmute(vaddq_s32(transmute(a), transmute(b))) }
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        unsafe { transmute(vaddq_u64(transmute(a), transmute(b))) }
    }
    /// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_add_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        unsafe { transmute(vaddq_s64(transmute(a), transmute(b))) }
    }

    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
        unsafe { transmute(vsubq_u8(transmute(a), transmute(b))) }
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
        unsafe { transmute(vsubq_s8(transmute(a), transmute(b))) }
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
        unsafe { transmute(vsubq_u16(transmute(a), transmute(b))) }
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
        unsafe { transmute(vsubq_s16(transmute(a), transmute(b))) }
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
        unsafe { transmute(vsubq_u32(transmute(a), transmute(b))) }
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
        unsafe { transmute(vsubq_s32(transmute(a), transmute(b))) }
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
        unsafe { transmute(vsubq_u64(transmute(a), transmute(b))) }
    }
    /// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
    #[inline(always)]
    pub fn wrapping_sub_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
        unsafe { transmute(vsubq_s64(transmute(a), transmute(b))) }
    }

    //-------------------------------------------------------------------------------
    // math
    //-------------------------------------------------------------------------------

    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        unsafe { transmute(vminq_f32(transmute(a), transmute(b))) }
    }
    /// Computes the elementwise minimum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn min_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        unsafe { transmute(vminq_f64(transmute(a), transmute(b))) }
    }

    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
        unsafe { transmute(vmaxq_f32(transmute(a), transmute(b))) }
    }
    /// Computes the elementwise maximum of each lane of `a` and `b`.
    #[inline(always)]
    pub fn max_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
        unsafe { transmute(vmaxq_f64(transmute(a), transmute(b))) }
    }

    //-------------------------------------------------------------------------------
    // comparisons
    //-------------------------------------------------------------------------------

    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        unsafe { transmute(vceqq_u8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        unsafe { transmute(vceqq_s8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        unsafe { transmute(vceqq_u16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        unsafe { transmute(vceqq_s16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        unsafe { transmute(vceqq_u32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        unsafe { transmute(vceqq_s32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        unsafe { transmute(vceqq_u64(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        unsafe { transmute(vceqq_s64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        unsafe { transmute(vcgtq_u8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        unsafe { transmute(vcgtq_s8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        unsafe { transmute(vcgtq_u16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        unsafe { transmute(vcgtq_s16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        unsafe { transmute(vcgtq_u32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        unsafe { transmute(vcgtq_s32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        unsafe { transmute(vcgtq_u64(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        unsafe { transmute(vcgtq_s64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        unsafe { transmute(vcgeq_u8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        unsafe { transmute(vcgeq_s8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        unsafe { transmute(vcgeq_u16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        unsafe { transmute(vcgeq_s16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        unsafe { transmute(vcgeq_u32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        unsafe { transmute(vcgeq_s32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        unsafe { transmute(vcgeq_u64(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        unsafe { transmute(vcgeq_s64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        unsafe { transmute(vcltq_u8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        unsafe { transmute(vcltq_s8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        unsafe { transmute(vcltq_u16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        unsafe { transmute(vcltq_s16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        unsafe { transmute(vcltq_u32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        unsafe { transmute(vcltq_s32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        unsafe { transmute(vcltq_u64(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        unsafe { transmute(vcltq_s64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
        unsafe { transmute(vcleq_u8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
        unsafe { transmute(vcleq_s8(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
        unsafe { transmute(vcleq_u16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
        unsafe { transmute(vcleq_s16(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
        unsafe { transmute(vcleq_u32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
        unsafe { transmute(vcleq_s32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
        unsafe { transmute(vcleq_u64(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
        unsafe { transmute(vcleq_s64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(vceqq_f32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for equality.
    #[inline(always)]
    pub fn cmp_eq_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(vceqq_f64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        self.not_m32x4(self.cmp_eq_f32x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for inequality.
    #[inline(always)]
    pub fn cmp_not_eq_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        self.not_m64x2(self.cmp_eq_f64x2(a, b))
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(vcgtq_f32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than.
    #[inline(always)]
    pub fn cmp_gt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(vcgtq_f64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(vcgeq_f32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_ge_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(vcgeq_f64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        self.not_m32x4(self.cmp_gt_f32x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than.
    #[inline(always)]
    pub fn cmp_not_gt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        self.not_m64x2(self.cmp_gt_f64x2(a, b))
    }

    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        self.not_m32x4(self.cmp_ge_f32x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_ge_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        self.not_m64x2(self.cmp_ge_f64x2(a, b))
    }

    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(vcltq_f32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than.
    #[inline(always)]
    pub fn cmp_lt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(vcltq_f64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(vcleq_f32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
    #[inline(always)]
    pub fn cmp_le_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(vcleq_f64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        unsafe { transmute(vcltq_f32(transmute(a), transmute(b))) }
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than.
    #[inline(always)]
    pub fn cmp_not_lt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        unsafe { transmute(vcltq_f64(transmute(a), transmute(b))) }
    }

    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
        self.not_m32x4(self.cmp_le_f32x4(a, b))
    }
    /// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
    #[inline(always)]
    pub fn cmp_not_le_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
        self.not_m64x2(self.cmp_le_f64x2(a, b))
    }

    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f32x4(self, a: f32x4) -> m32x4 {
        self.not_m32x4(self.cmp_eq_f32x4(a, a))
    }
    /// Checks if the elements in each lane of `a` are NaN.
    #[inline(always)]
    pub fn is_nan_f64x2(self, a: f64x2) -> m64x2 {
        self.not_m64x2(self.cmp_eq_f64x2(a, a))
    }

    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f32x4(self, a: f32x4) -> m32x4 {
        self.cmp_eq_f32x4(a, a)
    }
    /// Checks if the elements in each lane of `a` are not NaN.
    #[inline(always)]
    pub fn is_not_nan_f64x2(self, a: f64x2) -> m64x2 {
        self.cmp_eq_f64x2(a, a)
    }

    //-------------------------------------------------------------------------------
    // select
    //-------------------------------------------------------------------------------

    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u8x16(self, mask: m8x16, if_true: u8x16, if_false: u8x16) -> u8x16 {
        unsafe {
            transmute(vbslq_u8(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i8x16(self, mask: m8x16, if_true: i8x16, if_false: i8x16) -> i8x16 {
        unsafe {
            transmute(vbslq_u8(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u16x8(self, mask: m16x8, if_true: u16x8, if_false: u16x8) -> u16x8 {
        unsafe {
            transmute(vbslq_u16(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i16x8(self, mask: m16x8, if_true: i16x8, if_false: i16x8) -> i16x8 {
        unsafe {
            transmute(vbslq_u16(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u32x4(self, mask: m32x4, if_true: u32x4, if_false: u32x4) -> u32x4 {
        unsafe {
            transmute(vbslq_u32(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i32x4(self, mask: m32x4, if_true: i32x4, if_false: i32x4) -> i32x4 {
        unsafe {
            transmute(vbslq_u32(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f32x4(self, mask: m32x4, if_true: f32x4, if_false: f32x4) -> f32x4 {
        unsafe {
            transmute(vbslq_f32(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_u64x2(self, mask: m64x2, if_true: u64x2, if_false: u64x2) -> u64x2 {
        unsafe {
            transmute(vbslq_u64(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_i64x2(self, mask: m64x2, if_true: i64x2, if_false: i64x2) -> i64x2 {
        unsafe {
            transmute(vbslq_u64(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }
    /// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
    /// mask in `mask` is set, otherwise selecting elements from `if_false`.
    #[inline(always)]
    pub fn select_f64x2(self, mask: m64x2, if_true: f64x2, if_false: f64x2) -> f64x2 {
        unsafe {
            transmute(vbslq_f64(
                transmute(mask),
                transmute(if_true),
                transmute(if_false),
            ))
        }
    }

    #[inline(always)]
    pub fn reduce_sum_f32x4(self, a: f32x4) -> f32 {
        (a.0 + a.1) + (a.2 + a.3)
    }
    #[inline(always)]
    pub fn reduce_product_f32x4(self, a: f32x4) -> f32 {
        (a.0 * a.1) * (a.2 * a.3)
    }
    #[inline(always)]
    pub fn reduce_min_f32x4(self, a: f32x4) -> f32 {
        unsafe { vminnmvq_f32(transmute(a)) }
    }
    #[inline(always)]
    pub fn reduce_max_f32x4(self, a: f32x4) -> f32 {
        unsafe { vmaxnmvq_f32(transmute(a)) }
    }
    #[inline(always)]
    pub fn reduce_sum_f64x2(self, a: f64x2) -> f64 {
        a.0 + a.1
    }
    #[inline(always)]
    pub fn reduce_product_f64x2(self, a: f64x2) -> f64 {
        a.0 * a.1
    }
    #[inline(always)]
    pub fn reduce_min_f64x2(self, a: f64x2) -> f64 {
        unsafe { vminnmvq_f64(transmute(a)) }
    }
    #[inline(always)]
    pub fn reduce_max_f64x2(self, a: f64x2) -> f64 {
        unsafe { vmaxnmvq_f64(transmute(a)) }
    }

    #[inline(always)]
    pub fn reduce_sum_c32x2(self, a: f32x4) -> c32 {
        c32 {
            re: a.0 + a.2,
            im: a.1 + a.3,
        }
    }

    #[inline(always)]
    pub fn reduce_sum_c64x1(self, a: f64x2) -> c64 {
        cast(a)
    }
}

/// aarch64 arch
#[derive(Debug, Clone, Copy)]
#[non_exhaustive]
#[repr(u8)]
pub(crate) enum ArchInner {
    // msrv(1.66) discriminants on non-unit variants
    Scalar = 0,
    Neon = 1,

    // improves codegen for some reason
    #[allow(dead_code)]
    Dummy = u8::MAX - 1,
}

impl ArchInner {
    /// Detects the best available instruction set.
    #[inline]
    pub fn new() -> Self {
        if Neon::try_new().is_some() {
            return Self::Neon;
        }
        Self::Scalar
    }

    /// Detects the best available instruction set.
    #[inline(always)]
    pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
        match self {
            ArchInner::Neon => Simd::vectorize(unsafe { Neon::new_unchecked() }, op),
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

#[cfg(test)]
mod tests {
    use rand::random;

    use super::*;

    #[track_caller]
    fn assert_eq_c64(a: f64x2, b: f64x2) {
        assert!((a.0 - b.0).abs() < 1e-14);
        assert!((a.1 - b.1).abs() < 1e-14);
    }
    #[track_caller]
    fn assert_eq_c32(a: f32x4, b: f32x4) {
        assert!((a.0 - b.0).abs() < 1e-5);
        assert!((a.1 - b.1).abs() < 1e-5);
        assert!((a.2 - b.2).abs() < 1e-5);
        assert!((a.3 - b.3).abs() < 1e-5);
    }

    #[test]
    fn test_cplx64_mul() {
        for _ in 0..100 {
            let a = f64x2(random(), random());
            let b = f64x2(random(), random());
            let acc = f64x2(random(), random());

            let scalar = Scalar::new();

            let expected = cast(scalar.c64s_mul(cast(a), cast(b)));
            if let Some(simd) = Neon::try_new() {
                let c = simd.c64s_mul(a, b);
                assert_eq_c64(c, expected);
            }
            let expected = cast(scalar.c64s_mul_add(cast(a), cast(b), cast(acc)));
            if let Some(simd) = Neon::try_new() {
                let c = simd.c64s_mul_add(a, b, acc);
                assert_eq_c64(c, expected);
            }

            let expected = cast(scalar.c64s_conj_mul(cast(a), cast(b)));
            if let Some(simd) = Neon::try_new() {
                let c = simd.c64s_conj_mul(a, b);
                assert_eq_c64(c, expected);
            }
            let expected = cast(scalar.c64s_conj_mul_add(cast(a), cast(b), cast(acc)));
            if let Some(simd) = Neon::try_new() {
                let c = simd.c64s_conj_mul_add(a, b, acc);
                assert_eq_c64(c, expected);
            }

            let expected = cast(scalar.c64s_mul(cast(a), cast(b)));
            if let Some(simd) = NeonFcma::try_new() {
                let c = simd.c64s_mul(a, b);
                assert_eq_c64(c, expected);
            }
            let expected = cast(scalar.c64s_mul_add(cast(a), cast(b), cast(acc)));
            if let Some(simd) = NeonFcma::try_new() {
                let c = simd.c64s_mul_add(a, b, acc);
                assert_eq_c64(c, expected);
            }

            let expected = cast(scalar.c64s_conj_mul(cast(a), cast(b)));
            if let Some(simd) = NeonFcma::try_new() {
                let c = simd.c64s_conj_mul(a, b);
                assert_eq_c64(c, expected);
            }
            let expected = cast(scalar.c64s_conj_mul_add(cast(a), cast(b), cast(acc)));
            if let Some(simd) = NeonFcma::try_new() {
                let c = simd.c64s_conj_mul_add(a, b, acc);
                assert_eq_c64(c, expected);
            }
        }
    }

    #[test]
    fn test_cplx32_mul() {
        for _ in 0..100 {
            let a = f32x4(random(), random(), random(), random());
            let b = f32x4(random(), random(), random(), random());
            let acc = f32x4(random(), random(), random(), random());

            let scalar = Scalar::new();

            let expected = cast([
                scalar.c32s_mul(cast([a.0, a.1]), cast([b.0, b.1])),
                scalar.c32s_mul(cast([a.2, a.3]), cast([b.2, b.3])),
            ]);
            if let Some(simd) = Neon::try_new() {
                let c = simd.c32s_mul(a, b);
                assert_eq_c32(c, expected);
            }
            let expected = cast([
                scalar.c32s_mul_add(cast([a.0, a.1]), cast([b.0, b.1]), cast([acc.0, acc.1])),
                scalar.c32s_mul_add(cast([a.2, a.3]), cast([b.2, b.3]), cast([acc.2, acc.3])),
            ]);
            if let Some(simd) = Neon::try_new() {
                let c = simd.c32s_mul_add(a, b, acc);
                assert_eq_c32(c, expected);
            }

            let expected = cast([
                scalar.c32s_conj_mul(cast([a.0, a.1]), cast([b.0, b.1])),
                scalar.c32s_conj_mul(cast([a.2, a.3]), cast([b.2, b.3])),
            ]);
            if let Some(simd) = Neon::try_new() {
                let c = simd.c32s_conj_mul(a, b);
                assert_eq_c32(c, expected);
            }
            let expected = cast([
                scalar.c32s_conj_mul_add(cast([a.0, a.1]), cast([b.0, b.1]), cast([acc.0, acc.1])),
                scalar.c32s_conj_mul_add(cast([a.2, a.3]), cast([b.2, b.3]), cast([acc.2, acc.3])),
            ]);
            if let Some(simd) = Neon::try_new() {
                let c = simd.c32s_conj_mul_add(a, b, acc);
                assert_eq_c32(c, expected);
            }

            let expected = cast([
                scalar.c32s_mul(cast([a.0, a.1]), cast([b.0, b.1])),
                scalar.c32s_mul(cast([a.2, a.3]), cast([b.2, b.3])),
            ]);
            if let Some(simd) = NeonFcma::try_new() {
                let c = simd.c32s_mul(a, b);
                assert_eq_c32(c, expected);
            }
            let expected = cast([
                scalar.c32s_mul_add(cast([a.0, a.1]), cast([b.0, b.1]), cast([acc.0, acc.1])),
                scalar.c32s_mul_add(cast([a.2, a.3]), cast([b.2, b.3]), cast([acc.2, acc.3])),
            ]);
            if let Some(simd) = NeonFcma::try_new() {
                let c = simd.c32s_mul_add(a, b, acc);
                assert_eq_c32(c, expected);
            }

            let expected = cast([
                scalar.c32s_conj_mul(cast([a.0, a.1]), cast([b.0, b.1])),
                scalar.c32s_conj_mul(cast([a.2, a.3]), cast([b.2, b.3])),
            ]);
            if let Some(simd) = NeonFcma::try_new() {
                let c = simd.c32s_conj_mul(a, b);
                assert_eq_c32(c, expected);
            }
            let expected = cast([
                scalar.c32s_conj_mul_add(cast([a.0, a.1]), cast([b.0, b.1]), cast([acc.0, acc.1])),
                scalar.c32s_conj_mul_add(cast([a.2, a.3]), cast([b.2, b.3]), cast([acc.2, acc.3])),
            ]);
            if let Some(simd) = NeonFcma::try_new() {
                let c = simd.c32s_conj_mul_add(a, b, acc);
                assert_eq_c32(c, expected);
            }
        }
    }

    #[test]
    fn test_rotate() {
        if let Some(simd) = Neon::try_new() {
            for amount in 0..128 {
                let mut array = [0u32; 4];
                for (i, dst) in array.iter_mut().enumerate() {
                    *dst = 1000 + i as u32;
                }

                let rot: [u32; 4] = cast(simd.u32s_rotate_right(cast(array), amount));
                for i in 0..4 {
                    assert_eq!(rot[(i + amount) % 4], array[i]);
                }
            }
            for amount in 0..128 {
                let mut array = [0u64; 2];
                for (i, dst) in array.iter_mut().enumerate() {
                    *dst = 1000 + i as u64;
                }

                let rot: [u64; 2] = cast(simd.u64s_rotate_right(cast(array), amount));
                for i in 0..2 {
                    assert_eq!(rot[(i + amount) % 2], array[i]);
                }
            }
        }
    }
}
