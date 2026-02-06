use super::*;
use core::arch::aarch64::*;
#[allow(unused_imports)]
use core::arch::asm;

#[inline]
#[target_feature(enable = "neon,fcma")]
unsafe fn vcmlaq_0_f64(mut acc: float64x2_t, lhs: float64x2_t, rhs: float64x2_t) -> float64x2_t {
	#[cfg(miri)]
	{
		let lhs: f64x2 = cast!(lhs);
		let rhs: f64x2 = cast!(rhs);
		let a: f64x2 = cast!(acc);
		acc = cast!(f64x2(
			crate::fma_f64(lhs.0, rhs.0, a.0),
			crate::fma_f64(lhs.0, rhs.1, a.1),
		));
	}
	#[cfg(not(miri))]
	{
		asm!(
        "fcmla {0:v}.2d, {1:v}.2d, {2:v}.2d, 0",
        inout(vreg) acc,
        in(vreg) lhs,
        in(vreg) rhs,
        options(pure, nomem, nostack));
	}
	acc
}

#[inline]
#[target_feature(enable = "neon,fcma")]
unsafe fn vcmlaq_90_f64(mut acc: float64x2_t, lhs: float64x2_t, rhs: float64x2_t) -> float64x2_t {
	#[cfg(miri)]
	{
		let lhs: f64x2 = cast!(lhs);
		let rhs: f64x2 = cast!(rhs);
		let a: f64x2 = cast!(acc);
		acc = cast!(f64x2(
			crate::fma_f64(lhs.1, -rhs.1, a.0),
			crate::fma_f64(lhs.1, rhs.0, a.1),
		));
	}
	#[cfg(not(miri))]
	{
		asm!(
        "fcmla {0:v}.2d, {1:v}.2d, {2:v}.2d, 90",
        inout(vreg) acc,
        in(vreg) lhs,
        in(vreg) rhs,
        options(pure, nomem, nostack));
	}
	acc
}

#[inline]
#[target_feature(enable = "neon,fcma")]
unsafe fn vcmlaq_270_f64(mut acc: float64x2_t, lhs: float64x2_t, rhs: float64x2_t) -> float64x2_t {
	#[cfg(miri)]
	{
		let lhs: f64x2 = cast!(lhs);
		let rhs: f64x2 = cast!(rhs);
		let a: f64x2 = cast!(acc);
		acc = cast!(f64x2(
			crate::fma_f64(lhs.1, rhs.1, a.0),
			crate::fma_f64(lhs.1, -rhs.0, a.1),
		));
	}
	#[cfg(not(miri))]
	{
		asm!(
        "fcmla {0:v}.2d, {1:v}.2d, {2:v}.2d, 270",
        inout(vreg) acc,
        in(vreg) lhs,
        in(vreg) rhs,
        options(pure, nomem, nostack));
	}
	acc
}

#[inline]
#[target_feature(enable = "neon,fcma")]
unsafe fn vcmlaq_0_f32(mut acc: float32x4_t, lhs: float32x4_t, rhs: float32x4_t) -> float32x4_t {
	#[cfg(miri)]
	{
		let lhs: f32x4 = cast!(lhs);
		let rhs: f32x4 = cast!(rhs);
		let a: f32x4 = cast!(acc);
		acc = cast!(f32x4(
			crate::fma_f32(lhs.0, rhs.0, a.0),
			crate::fma_f32(lhs.0, rhs.1, a.1),
			crate::fma_f32(lhs.2, rhs.2, a.2),
			crate::fma_f32(lhs.2, rhs.3, a.3),
		));
	}
	#[cfg(not(miri))]
	{
		asm!(
        "fcmla {0:v}.4s, {1:v}.4s, {2:v}.4s, 0",
        inout(vreg) acc,
        in(vreg) lhs,
        in(vreg) rhs,
        options(pure, nomem, nostack));
	}
	acc
}

#[inline]
#[target_feature(enable = "neon,fcma")]
unsafe fn vcmlaq_90_f32(mut acc: float32x4_t, lhs: float32x4_t, rhs: float32x4_t) -> float32x4_t {
	#[cfg(miri)]
	{
		let lhs: f32x4 = cast!(lhs);
		let rhs: f32x4 = cast!(rhs);
		let a: f32x4 = cast!(acc);
		acc = cast!(f32x4(
			crate::fma_f32(lhs.1, -rhs.1, a.0),
			crate::fma_f32(lhs.1, rhs.0, a.1),
			crate::fma_f32(lhs.3, -rhs.3, a.2),
			crate::fma_f32(lhs.3, rhs.2, a.3),
		));
	}
	#[cfg(not(miri))]
	{
		asm!(
        "fcmla {0:v}.4s, {1:v}.4s, {2:v}.4s, 90",
        inout(vreg) acc,
        in(vreg) lhs,
        in(vreg) rhs,
        options(pure, nomem, nostack));
	}
	acc
}

#[inline]
#[target_feature(enable = "neon,fcma")]
unsafe fn vcmlaq_270_f32(mut acc: float32x4_t, lhs: float32x4_t, rhs: float32x4_t) -> float32x4_t {
	#[cfg(miri)]
	{
		let lhs: f32x4 = cast!(lhs);
		let rhs: f32x4 = cast!(rhs);
		let a: f32x4 = cast!(acc);
		acc = cast!(f32x4(
			crate::fma_f32(lhs.1, rhs.1, a.0),
			crate::fma_f32(lhs.1, -rhs.0, a.1),
			crate::fma_f32(lhs.3, rhs.3, a.2),
			crate::fma_f32(lhs.3, -rhs.2, a.3),
		));
	}
	#[cfg(not(miri))]
	{
		asm!(
        "fcmla {0:v}.4s, {1:v}.4s, {2:v}.4s, 270",
        inout(vreg) acc,
        in(vreg) lhs,
        in(vreg) rhs,
        options(pure, nomem, nostack));
	}
	acc
}

simd_type!({
	#[allow(missing_docs)]
	pub struct Neon {
		pub neon: f!("neon"),
	}

	#[allow(missing_docs)]
	pub struct NeonFcma {
		pub neon: f!("neon"),
		pub fcma: f!("fcma"),
	}
});

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

macro_rules! impl_binop {
	($func: ident, $neon_fn: ident, $ty: ident, $factor: literal, $out: ident) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$out s>] {
				self.[<$neon_fn _ $ty x $factor>](a, b)
			}
		}
	};
	($func: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_binop!($func, $func, $ty, $factor, $ty);)*
	};
	($func: ident, $neon_fn: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_binop!($func, $neon_fn, $ty, $factor, $ty);)*
	};
	($func: ident, $neon_fn: ident, $($ty: ident x $factor: literal => $out: ident),*) => {
		$(impl_binop!($func, $neon_fn, $ty, $factor, $out);)*
	};
}

macro_rules! impl_binop_scalar {
	($func: ident, $ty: ident) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$ty s>] {
				Scalar128b.[<$func _ $ty s>](a, b)
			}
		}
	};
	($func: ident, $($ty: ident),*) => {
		$(impl_binop_scalar!($func, $ty);)*
	}
}

macro_rules! impl_unop {
	($func: ident, $neon_fn: ident, $ty: ident, $factor: literal, $out: ident) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>]) -> Self::[<$out s>] {
				self.[<$neon_fn _ $ty x $factor>](a)
			}
		}
	};
	($func: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_unop!($func, $func, $ty, $factor, $ty);)*
	};
	($func: ident, $neon_fn: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_unop!($func, $neon_fn, $ty, $factor, $ty);)*
	};
	($func: ident, $neon_fn: ident, $($ty: ident x $factor: literal => $out: ident),*) => {
		$(impl_unop!($func, $neon_fn, $ty, $factor, $out);)*
	};
}

macro_rules! splat {
	($ty: ident, $factor: literal) => {
		paste! {
			#[inline(always)]
			fn [<splat_ $ty s>](self, value: $ty) -> Self::[<$ty s>] {
				cast!(self.[<splat_ $ty x $factor>](cast!(value)))
			}
		}
	};
	($($ty: ident x $factor: literal),*) => {
		$(splat!($ty, $factor);)*
	};
}

impl Simd for Neon {
	type c32s = f32x4;
	type c64s = f64x2;
	type f32s = f32x4;
	type f64s = f64x2;
	type i16s = i16x8;
	type i32s = i32x4;
	type i64s = i64x2;
	type i8s = i8x16;
	type m16s = m16x8;
	type m32s = m32x4;
	type m64s = m64x2;
	type m8s = m8x16;
	type u16s = u16x8;
	type u32s = u32x4;
	type u64s = u64x2;
	type u8s = u8x16;

	const REGISTER_COUNT: usize = 32;

	impl_binop!(add, f32 x 4, f64 x 2);

	impl_binop!(add, wrapping_add, u8 x 16, u16 x 8, u32 x 4, u64 x 2);

	impl_binop!(sub, f32 x 4, f64 x 2);

	impl_binop!(sub, wrapping_sub, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2);

	impl_binop!(mul, u16 x 8, i16 x 8, u32 x 4, i32 x 4);

	impl_binop_scalar!(mul, u64, i64);

	impl_binop!(div, f32 x 4, f64 x 2);

	impl_binop!(and, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_binop!(or, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_binop!(xor, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_binop!(equal, cmp_eq, u8 x 16 => m8, u16 x 8 => m16, u32 x 4 => m32, u64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(greater_than, cmp_gt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(greater_than_or_equal, cmp_ge, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(less_than_or_equal, cmp_le, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(less_than, cmp_lt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(max, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_binop_scalar!(max, u64, i64);

	impl_binop!(min, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_binop_scalar!(min, u64, i64);

	impl_unop!(not, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	splat!(u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2, f32 x 4, f64 x 2);

	#[inline(always)]
	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
		unsafe {
			let sqr = self.mul_f32s(a, a);
			let sqr_rev = cast!(vrev64q_f32(cast!(sqr)));
			self.add_f32s(sqr, sqr_rev)
		}
	}

	#[inline(always)]
	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
		unsafe {
			let sqr = self.mul_f64s(a, a);
			let sqr_rev = cast!(vcombine_u64(
				vget_high_u64(cast!(sqr)),
				vget_low_u64(cast!(sqr)),
			));
			self.add_f64s(sqr, sqr_rev)
		}
	}

	#[inline(always)]
	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
		unsafe {
			let max = self.max_f32s(a, a);
			let max_rev = cast!(vrev64q_f32(cast!(max)));
			self.max_f32s(max, max_rev)
		}
	}

	#[inline(always)]
	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
		unsafe {
			let sqr = self.max_f64s(a, a);
			let sqr_rev = cast!(vcombine_u64(
				vget_high_u64(cast!(sqr)),
				vget_low_u64(cast!(sqr)),
			));
			self.max_f64s(sqr, sqr_rev)
		}
	}

	#[inline(always)]
	fn add_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.add_f32x4(a, b)
	}

	#[inline(always)]
	fn add_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.add_f64s(a, b)
	}

	#[inline(always)]
	fn equal_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::m32s {
		self.equal_f32s(a, b)
	}

	#[inline(always)]
	fn equal_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::m64s {
		self.equal_f64s(a, b)
	}

	#[inline(always)]
	fn conj_c32s(self, a: Self::c32s) -> Self::c32s {
		self.xor_f32s(a, self.splat_c32s(c32 { re: 0.0, im: -0.0 }))
	}

	#[inline(always)]
	fn conj_c64s(self, a: Self::c64s) -> Self::c64s {
		self.xor_f64s(a, self.splat_c64s(c64 { re: 0.0, im: -0.0 }))
	}

	#[inline(always)]
	fn conj_mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		unsafe {
			let ab = cast!(a);
			let xy = cast!(b);

			let yx = vrev64q_f32(xy);
			let aa = vtrn1q_f32(ab, ab);
			let bb = vtrn2q_f32(ab, ab);

			let bb_sign = cast!(veorq_u64(cast!(f32x4(0.0, -0.0, 0.0, -0.0)), cast!(bb),));

			cast!(vfmaq_f32(vfmaq_f32(cast!(c), bb_sign, yx), aa, xy))
		}
	}

	#[inline(always)]
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		unsafe {
			let ab = cast!(a);
			let xy = cast!(b);

			let yx = cast!(vcombine_u64(
				vget_high_u64(cast!(xy)),
				vget_low_u64(cast!(xy)),
			));
			let aa = vtrn1q_f64(ab, ab);
			let bb = vtrn2q_f64(ab, ab);

			let bb_sign = cast!(veorq_u64(cast!(f64x2(0.0, -0.0)), cast!(bb)));

			cast!(vfmaq_f64(vfmaq_f64(cast!(c), bb_sign, yx), aa, xy))
		}
	}

	#[inline(always)]
	fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		unsafe {
			let ab = cast!(a);
			let xy = cast!(b);

			let yx = vrev64q_f32(xy);
			let aa = vtrn1q_f32(ab, ab);
			let bb = vtrn2q_f32(ab, ab);

			let bb_sign = cast!(veorq_u64(cast!(f32x4(0.0, -0.0, 0.0, -0.0)), cast!(bb),));

			cast!(vfmaq_f32(vmulq_f32(bb_sign, yx), aa, xy))
		}
	}

	#[inline(always)]
	fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		unsafe {
			let ab = cast!(a);
			let xy = cast!(b);

			let yx = cast!(vcombine_u64(
				vget_high_u64(cast!(xy)),
				vget_low_u64(cast!(xy)),
			));
			let aa = vtrn1q_f64(ab, ab);
			let bb = vtrn2q_f64(ab, ab);

			let bb_sign = cast!(veorq_u64(cast!(f64x2(0.0, -0.0)), cast!(bb)));

			cast!(vfmaq_f64(vmulq_f64(bb_sign, yx), aa, xy))
		}
	}

	#[inline(always)]
	fn deinterleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		if const { !cfg!(miri) && core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f32s>() }
		{
			unsafe { core::mem::transmute_copy(&vld2q_f32((&values) as *const _ as *const f32)) }
		} else if const {
			!cfg!(miri) && core::mem::size_of::<T>() == 3 * core::mem::size_of::<Self::f32s>()
		} {
			unsafe { core::mem::transmute_copy(&vld3q_f32((&values) as *const _ as *const f32)) }
		} else if const {
			!cfg!(miri) && core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f32s>()
		} {
			unsafe { core::mem::transmute_copy(&vld4q_f32((&values) as *const _ as *const f32)) }
		} else {
			unsafe { deinterleave_fallback::<f32, Self::f32s, T>(values) }
		}
	}

	#[inline(always)]
	fn deinterleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		if const { !cfg!(miri) && core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f32s>() }
		{
			unsafe { core::mem::transmute_copy(&vld2q_f64((&values) as *const _ as *const f64)) }
		} else if const {
			!cfg!(miri) && core::mem::size_of::<T>() == 3 * core::mem::size_of::<Self::f32s>()
		} {
			unsafe { core::mem::transmute_copy(&vld3q_f64((&values) as *const _ as *const f64)) }
		} else if const {
			!cfg!(miri) && core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f32s>()
		} {
			unsafe { core::mem::transmute_copy(&vld4q_f64((&values) as *const _ as *const f64)) }
		} else {
			unsafe { deinterleave_fallback::<f64, Self::f64s, T>(values) }
		}
	}

	#[inline(always)]
	fn interleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		unsafe {
			let mut out: T = core::mem::zeroed();

			if const {
				!cfg!(miri) && core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f32s>()
			} {
				vst2q_f32(
					(&mut out) as *mut _ as *mut f32,
					core::mem::transmute_copy(&values),
				);
			} else if const {
				!cfg!(miri) && core::mem::size_of::<T>() == 3 * core::mem::size_of::<Self::f32s>()
			} {
				vst3q_f32(
					(&mut out) as *mut _ as *mut f32,
					core::mem::transmute_copy(&values),
				);
			} else if const {
				!cfg!(miri) && core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f32s>()
			} {
				vst4q_f32(
					(&mut out) as *mut _ as *mut f32,
					core::mem::transmute_copy(&values),
				);
			} else {
				return interleave_fallback::<f32, Self::f32s, T>(values);
			}

			out
		}
	}

	#[inline(always)]
	fn interleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		unsafe {
			let mut out: T = core::mem::zeroed();

			if const {
				!cfg!(miri) && core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f32s>()
			} {
				vst2q_f64(
					(&mut out) as *mut _ as *mut f64,
					core::mem::transmute_copy(&values),
				);
			} else if const {
				!cfg!(miri) && core::mem::size_of::<T>() == 3 * core::mem::size_of::<Self::f32s>()
			} {
				vst3q_f64(
					(&mut out) as *mut _ as *mut f64,
					core::mem::transmute_copy(&values),
				);
			} else if const {
				!cfg!(miri) && core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f32s>()
			} {
				vst4q_f64(
					(&mut out) as *mut _ as *mut f64,
					core::mem::transmute_copy(&values),
				);
			} else {
				return interleave_fallback::<f64, Self::f64s, T>(values);
			}

			out
		}
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_c32s(self, mask: MemMask<Self::m32s>, ptr: *const c32) -> Self::c32s {
		let mask = mask.mask;
		let ptr = ptr as *const f32;
		f32x4(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
			if mask.2.is_set() {
				*ptr.wrapping_add(2)
			} else {
				core::mem::zeroed()
			},
			if mask.3.is_set() {
				*ptr.wrapping_add(3)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_c64s(self, mask: MemMask<Self::m64s>, ptr: *const c64) -> Self::c64s {
		let mask = mask.mask;
		let ptr = ptr as *const f64;
		f64x2(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *const u8) -> Self::u8s {
		let mask = mask.mask;
		u8x16(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
			if mask.2.is_set() {
				*ptr.wrapping_add(2)
			} else {
				core::mem::zeroed()
			},
			if mask.3.is_set() {
				*ptr.wrapping_add(3)
			} else {
				core::mem::zeroed()
			},
			if mask.4.is_set() {
				*ptr.wrapping_add(4)
			} else {
				core::mem::zeroed()
			},
			if mask.5.is_set() {
				*ptr.wrapping_add(5)
			} else {
				core::mem::zeroed()
			},
			if mask.6.is_set() {
				*ptr.wrapping_add(6)
			} else {
				core::mem::zeroed()
			},
			if mask.7.is_set() {
				*ptr.wrapping_add(7)
			} else {
				core::mem::zeroed()
			},
			if mask.8.is_set() {
				*ptr.wrapping_add(8)
			} else {
				core::mem::zeroed()
			},
			if mask.9.is_set() {
				*ptr.wrapping_add(9)
			} else {
				core::mem::zeroed()
			},
			if mask.10.is_set() {
				*ptr.wrapping_add(10)
			} else {
				core::mem::zeroed()
			},
			if mask.11.is_set() {
				*ptr.wrapping_add(11)
			} else {
				core::mem::zeroed()
			},
			if mask.12.is_set() {
				*ptr.wrapping_add(12)
			} else {
				core::mem::zeroed()
			},
			if mask.13.is_set() {
				*ptr.wrapping_add(13)
			} else {
				core::mem::zeroed()
			},
			if mask.14.is_set() {
				*ptr.wrapping_add(14)
			} else {
				core::mem::zeroed()
			},
			if mask.15.is_set() {
				*ptr.wrapping_add(15)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u16s(self, mask: MemMask<Self::m16s>, ptr: *const u16) -> Self::u16s {
		let mask = mask.mask;
		u16x8(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
			if mask.2.is_set() {
				*ptr.wrapping_add(2)
			} else {
				core::mem::zeroed()
			},
			if mask.3.is_set() {
				*ptr.wrapping_add(3)
			} else {
				core::mem::zeroed()
			},
			if mask.4.is_set() {
				*ptr.wrapping_add(4)
			} else {
				core::mem::zeroed()
			},
			if mask.5.is_set() {
				*ptr.wrapping_add(5)
			} else {
				core::mem::zeroed()
			},
			if mask.6.is_set() {
				*ptr.wrapping_add(6)
			} else {
				core::mem::zeroed()
			},
			if mask.7.is_set() {
				*ptr.wrapping_add(7)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u32s(self, mask: MemMask<Self::m32s>, ptr: *const u32) -> Self::u32s {
		let mask = mask.mask;
		u32x4(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
			if mask.2.is_set() {
				*ptr.wrapping_add(2)
			} else {
				core::mem::zeroed()
			},
			if mask.3.is_set() {
				*ptr.wrapping_add(3)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u64s(self, mask: MemMask<Self::m64s>, ptr: *const u64) -> Self::u64s {
		let mask = mask.mask;
		u64x2(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_store_ptr_c32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut c32,
		values: Self::c32s,
	) {
		let mask = mask.mask;
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
	unsafe fn mask_store_ptr_c64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut c64,
		values: Self::c64s,
	) {
		let mask = mask.mask;
		let ptr = ptr as *mut f64;
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
	unsafe fn mask_store_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *mut u8, values: Self::u8s) {
		let mask = mask.mask;
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
		if mask.4.is_set() {
			*ptr.wrapping_add(4) = values.4
		}
		if mask.5.is_set() {
			*ptr.wrapping_add(5) = values.5
		}
		if mask.6.is_set() {
			*ptr.wrapping_add(6) = values.6
		}
		if mask.7.is_set() {
			*ptr.wrapping_add(7) = values.7
		}
		if mask.8.is_set() {
			*ptr.wrapping_add(8) = values.8
		}
		if mask.9.is_set() {
			*ptr.wrapping_add(9) = values.9
		}
		if mask.10.is_set() {
			*ptr.wrapping_add(10) = values.10
		}
		if mask.11.is_set() {
			*ptr.wrapping_add(11) = values.11
		}
		if mask.12.is_set() {
			*ptr.wrapping_add(12) = values.12
		}
		if mask.13.is_set() {
			*ptr.wrapping_add(13) = values.13
		}
		if mask.14.is_set() {
			*ptr.wrapping_add(14) = values.14
		}
		if mask.15.is_set() {
			*ptr.wrapping_add(15) = values.15
		}
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_store_ptr_u16s(
		self,
		mask: MemMask<Self::m16s>,
		ptr: *mut u16,
		values: Self::u16s,
	) {
		let mask = mask.mask;
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
		if mask.4.is_set() {
			*ptr.wrapping_add(4) = values.4
		}
		if mask.5.is_set() {
			*ptr.wrapping_add(5) = values.5
		}
		if mask.6.is_set() {
			*ptr.wrapping_add(6) = values.6
		}
		if mask.7.is_set() {
			*ptr.wrapping_add(7) = values.7
		}
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_store_ptr_u32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut u32,
		values: Self::u32s,
	) {
		let mask = mask.mask;
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
	unsafe fn mask_store_ptr_u64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut u64,
		values: Self::u64s,
	) {
		let mask = mask.mask;
		if mask.0.is_set() {
			*ptr.wrapping_add(0) = values.0
		}
		if mask.1.is_set() {
			*ptr.wrapping_add(1) = values.1
		}
	}

	#[inline(always)]
	fn mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		unsafe {
			let ab = cast!(a);
			let xy = cast!(b);

			let yx = vrev64q_f32(xy);
			let aa = vtrn1q_f32(ab, ab);
			let bb = vtrn2q_f32(ab, ab);

			let bb_sign = cast!(veorq_u64(cast!(f32x4(-0.0, 0.0, -0.0, 0.0)), cast!(bb),));

			cast!(vfmaq_f32(vfmaq_f32(cast!(c), bb_sign, yx), aa, xy))
		}
	}

	#[inline(always)]
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		unsafe {
			let ab = cast!(a);
			let xy = cast!(b);

			let yx = cast!(vcombine_u64(
				vget_high_u64(cast!(xy)),
				vget_low_u64(cast!(xy)),
			));
			let aa = vtrn1q_f64(ab, ab);
			let bb = vtrn2q_f64(ab, ab);

			let bb_sign = cast!(veorq_u64(cast!(f64x2(-0.0, 0.0)), cast!(bb)));

			cast!(vfmaq_f64(vfmaq_f64(cast!(c), bb_sign, yx), aa, xy))
		}
	}

	#[inline(always)]
	fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		self.mul_add_f32s(a, b, c)
	}

	#[inline(always)]
	fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		self.mul_add_f64s(a, b, c)
	}

	#[inline(always)]
	fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		self.mul_add_f32x4(a, b, c)
	}

	#[inline(always)]
	fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		self.mul_add_f64x2(a, b, c)
	}

	#[inline(always)]
	fn mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		unsafe {
			let ab = cast!(a);
			let xy = cast!(b);

			let yx = vrev64q_f32(xy);
			let aa = vtrn1q_f32(ab, ab);
			let bb = vtrn2q_f32(ab, ab);

			let bb_sign = cast!(veorq_u64(cast!(f32x4(-0.0, 0.0, -0.0, 0.0)), cast!(bb),));

			cast!(vfmaq_f32(vmulq_f32(bb_sign, yx), aa, xy))
		}
	}

	#[inline(always)]
	fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		unsafe {
			let ab = cast!(a);
			let xy = cast!(b);

			let yx = cast!(vcombine_u64(
				vget_high_u64(cast!(xy)),
				vget_low_u64(cast!(xy)),
			));
			let aa = vtrn1q_f64(ab, ab);
			let bb = vtrn2q_f64(ab, ab);

			let bb_sign = cast!(veorq_u64(cast!(f64x2(-0.0, 0.0)), cast!(bb)));

			cast!(vfmaq_f64(vmulq_f64(bb_sign, yx), aa, xy))
		}
	}

	#[inline(always)]
	fn mul_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		self.mul_f32x4(a, b)
	}

	#[inline(always)]
	fn mul_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		self.mul_f64x2(a, b)
	}

	#[inline(always)]
	fn neg_c32s(self, a: Self::c32s) -> Self::c32s {
		self.xor_f32s(a, self.splat_f32s(-0.0))
	}

	#[inline(always)]
	fn neg_c64s(self, a: Self::c64s) -> Self::c64s {
		self.xor_f64s(a, self.splat_f64s(-0.0))
	}

	#[inline(always)]
	fn partial_load_u32s(self, slice: &[u32]) -> Self::u32s {
		match slice.len() {
			0 => u32x4(0, 0, 0, 0),
			1 => u32x4(slice[0], 0, 0, 0),
			2 => u32x4(slice[0], slice[1], 0, 0),
			3 => u32x4(slice[0], slice[1], slice[2], 0),
			_ => u32x4(slice[0], slice[1], slice[2], slice[3]),
		}
	}

	#[inline(always)]
	fn partial_load_u64s(self, slice: &[u64]) -> Self::u64s {
		match slice.len() {
			0 => u64x2(0, 0),
			1 => u64x2(slice[0], 0),
			_ => u64x2(slice[0], slice[1]),
		}
	}

	#[inline(always)]
	fn partial_store_u32s(self, slice: &mut [u32], values: Self::u32s) {
		match slice.len() {
			0 => {},
			1 => {
				slice[0] = values.0;
			},
			2 => {
				slice[0] = values.0;
				slice[1] = values.1;
			},
			3 => {
				slice[0] = values.0;
				slice[1] = values.1;
				slice[2] = values.2;
			},
			_ => {
				slice[0] = values.0;
				slice[1] = values.1;
				slice[2] = values.2;
				slice[3] = values.3;
			},
		}
	}

	#[inline(always)]
	fn partial_store_u64s(self, slice: &mut [u64], values: Self::u64s) {
		match slice.len() {
			0 => {},
			1 => {
				slice[0] = values.0;
			},
			_ => {
				slice[0] = values.0;
				slice[1] = values.1;
			},
		}
	}

	#[inline(always)]
	fn reduce_max_c32s(self, a: Self::c32s) -> c32 {
		unsafe {
			// a0 a1 a2 a3
			let a = cast!(a);
			// a2 a3 a2 a3
			let hi = vcombine_u64(vget_high_u64(a), vget_low_u64(a));

			// a0+a2 a1+a3 _ _
			cast_lossy(self.max_f32s(cast!(a), cast!(hi)))
		}
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
		self.reduce_max_f32x4(a)
	}

	#[inline(always)]
	fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
		self.reduce_max_f64x2(a)
	}

	#[inline(always)]
	fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
		unsafe {
			// a0 a1 a2 a3
			let a = cast!(a);
			// a2 a3 a2 a3
			let hi = vcombine_u64(vget_high_u64(a), vget_low_u64(a));

			// a0+a2 a1+a3 _ _
			cast_lossy(self.min_f32s(cast!(a), cast!(hi)))
		}
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
		self.reduce_min_f32x4(a)
	}

	#[inline(always)]
	fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
		self.reduce_min_f64x2(a)
	}

	#[inline(always)]
	fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
		self.reduce_product_f32x4(a)
	}

	#[inline(always)]
	fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
		self.reduce_product_f64x2(a)
	}

	#[inline(always)]
	fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
		unsafe {
			// a0 a1 a2 a3
			let a = cast!(a);
			// a2 a3 a2 a3
			let hi = vcombine_u64(vget_high_u64(a), vget_low_u64(a));

			// a0+a2 a1+a3 _ _
			cast_lossy(self.add_f32s(cast!(a), cast!(hi)))
		}
	}

	#[inline(always)]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
		self.reduce_sum_f32x4(a)
	}

	#[inline(always)]
	fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
		self.reduce_sum_f64x2(a)
	}

	#[inline(always)]
	fn rotate_right_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s {
		cast!(self.rotate_right_u64s(cast!(a), amount))
	}

	#[inline(always)]
	fn rotate_right_c64s(self, a: Self::c64s, _amount: usize) -> Self::c64s {
		a
	}

	#[inline(always)]
	fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s {
		#[cfg(miri)]
		{
			let mut a: [u32; 4] = cast!(a);
			a.rotate_right(amount % 4);
			cast!(a)
		}
		#[cfg(not(miri))]
		{
			unsafe {
				cast!(vqtbl1q_u8(
					cast!(a),
					cast!(NEON_ROTATE_IDX[4 * (amount % 4)]),
				))
			}
		}
	}

	#[inline(always)]
	fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s {
		#[cfg(miri)]
		{
			let mut a: [u64; 2] = cast!(a);
			a.rotate_right(amount % 2);
			cast!(a)
		}
		#[cfg(not(miri))]
		{
			unsafe {
				cast!(vqtbl1q_u8(
					cast!(a),
					cast!(NEON_ROTATE_IDX[8 * (amount % 2)]),
				))
			}
		}
	}

	#[inline(always)]
	fn select_u32s(
		self,
		mask: Self::m32s,
		if_true: Self::u32s,
		if_false: Self::u32s,
	) -> Self::u32s {
		self.select_u32x4(mask, if_true, if_false)
	}

	#[inline(always)]
	fn select_u64s(
		self,
		mask: Self::m64s,
		if_true: Self::u64s,
		if_false: Self::u64s,
	) -> Self::u64s {
		self.select_u64x2(mask, if_true, if_false)
	}

	#[inline(always)]
	fn splat_c32s(self, value: c32) -> Self::c32s {
		cast!(self.splat_f64x2(cast!(value)))
	}

	#[inline(always)]
	fn splat_c64s(self, value: c64) -> Self::c64s {
		cast!(value)
	}

	#[inline(always)]
	fn sub_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.sub_f32x4(a, b)
	}

	#[inline(always)]
	fn sub_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.sub_f64s(a, b)
	}

	#[inline(always)]
	fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s {
		unsafe { cast!(vrev64q_f32(cast!(a))) }
	}

	#[inline(always)]
	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
		unsafe {
			cast!(vcombine_u64(
				vget_high_u64(cast!(a)),
				vget_low_u64(cast!(a)),
			))
		}
	}

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
	fn widening_mul_u32s(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
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
	fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		self.shl_dyn_u32x4(a, self.and_i32x4(cast!(amount), self.splat_i32x4(32 - 1)))
	}

	#[inline(always)]
	fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		u32x4(
			a.0 >> amount.0,
			a.1 >> amount.1,
			a.2 >> amount.2,
			a.3 >> amount.3,
		)
	}

	#[inline(always)]
	fn sqrt_f32s(self, a: Self::f32s) -> Self::f32s {
		cast!(self.neon.vsqrtq_f32(cast!(a)))
	}

	#[inline(always)]
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
		cast!(self.neon.vsqrtq_f64(cast!(a)))
	}
}

impl Simd for NeonFcma {
	type c32s = f32x4;
	type c64s = f64x2;
	type f32s = f32x4;
	type f64s = f64x2;
	type i16s = i16x8;
	type i32s = i32x4;
	type i64s = i64x2;
	type i8s = i8x16;
	type m16s = m16x8;
	type m32s = m32x4;
	type m64s = m64x2;
	type m8s = m8x16;
	type u16s = u16x8;
	type u32s = u32x4;
	type u64s = u64x2;
	type u8s = u8x16;

	const REGISTER_COUNT: usize = 32;

	impl_binop!(add, f32 x 4, f64 x 2);

	impl_binop!(add, wrapping_add, u8 x 16, u16 x 8, u32 x 4, u64 x 2);

	impl_binop!(sub, f32 x 4, f64 x 2);

	impl_binop!(sub, wrapping_sub, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2);

	impl_binop!(mul, u16 x 8, i16 x 8, u32 x 4, i32 x 4);

	impl_binop_scalar!(mul, u64, i64);

	impl_binop!(div, f32 x 4, f64 x 2);

	impl_binop!(and, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_binop!(or, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_binop!(xor, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_binop!(equal, cmp_eq, u8 x 16 => m8, u16 x 8 => m16, u32 x 4 => m32, u64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(greater_than, cmp_gt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(greater_than_or_equal, cmp_ge, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(less_than_or_equal, cmp_le, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(less_than, cmp_lt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(max, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_binop_scalar!(max, u64, i64);

	impl_binop!(min, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_binop_scalar!(min, u64, i64);

	impl_unop!(not, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	splat!(u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2, f32 x 4, f64 x 2);

	#[inline(always)]
	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
		unsafe {
			let sqr = self.mul_f32s(a, a);
			let sqr_rev = cast!(vrev64q_f32(cast!(sqr)));
			self.add_f32s(sqr, sqr_rev)
		}
	}

	#[inline(always)]
	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
		unsafe {
			let sqr = self.mul_f64s(a, a);
			let sqr_rev = cast!(vcombine_u64(
				vget_high_u64(cast!(sqr)),
				vget_low_u64(cast!(sqr)),
			));
			self.add_f64s(sqr, sqr_rev)
		}
	}

	#[inline(always)]
	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
		unsafe {
			let max = self.max_f32s(a, a);
			let max_rev = cast!(vrev64q_f32(cast!(max)));
			self.max_f32s(max, max_rev)
		}
	}

	#[inline(always)]
	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
		unsafe {
			let sqr = self.max_f64s(a, a);
			let sqr_rev = cast!(vcombine_u64(
				vget_high_u64(cast!(sqr)),
				vget_low_u64(cast!(sqr)),
			));
			self.max_f64s(sqr, sqr_rev)
		}
	}

	#[inline(always)]
	fn add_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.add_f32x4(a, b)
	}

	#[inline(always)]
	fn add_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.add_f64s(a, b)
	}

	#[inline(always)]
	fn equal_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::m32s {
		self.equal_f32s(a, b)
	}

	#[inline(always)]
	fn equal_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::m64s {
		self.equal_f64s(a, b)
	}

	#[inline(always)]
	fn conj_c32s(self, a: Self::c32s) -> Self::c32s {
		self.xor_f32s(a, self.splat_c32s(c32 { re: 0.0, im: -0.0 }))
	}

	#[inline(always)]
	fn conj_c64s(self, a: Self::c64s) -> Self::c64s {
		self.xor_f64s(a, self.splat_c64s(c64 { re: 0.0, im: -0.0 }))
	}

	#[inline(always)]
	fn conj_mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		let a = cast!(a);
		let b = cast!(b);
		let c = cast!(c);
		unsafe { cast!(vcmlaq_270_f32(vcmlaq_0_f32(c, a, b), a, b)) }
	}

	#[inline(always)]
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let a = cast!(a);
		let b = cast!(b);
		let c = cast!(c);
		unsafe { cast!(vcmlaq_270_f64(vcmlaq_0_f64(c, a, b), a, b)) }
	}

	#[inline(always)]
	fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.conj_mul_add_c32s(a, b, self.splat_f32s(0.0))
	}

	#[inline(always)]
	fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.conj_mul_add_c64s(a, b, self.splat_f64s(0.0))
	}

	#[inline(always)]
	fn deinterleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		(*self).deinterleave_shfl_f32s(values)
	}

	#[inline(always)]
	fn deinterleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		(*self).deinterleave_shfl_f64s(values)
	}

	#[inline(always)]
	fn interleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		(*self).interleave_shfl_f32s(values)
	}

	#[inline(always)]
	fn interleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		(*self).interleave_shfl_f64s(values)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_c32s(self, mask: MemMask<Self::m32s>, ptr: *const c32) -> Self::c32s {
		let mask = mask.mask;
		let ptr = ptr as *const f32;
		f32x4(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
			if mask.2.is_set() {
				*ptr.wrapping_add(2)
			} else {
				core::mem::zeroed()
			},
			if mask.3.is_set() {
				*ptr.wrapping_add(3)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_c64s(self, mask: MemMask<Self::m64s>, ptr: *const c64) -> Self::c64s {
		let mask = mask.mask;
		let ptr = ptr as *const f64;
		f64x2(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *const u8) -> Self::u8s {
		let mask = mask.mask;
		u8x16(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
			if mask.2.is_set() {
				*ptr.wrapping_add(2)
			} else {
				core::mem::zeroed()
			},
			if mask.3.is_set() {
				*ptr.wrapping_add(3)
			} else {
				core::mem::zeroed()
			},
			if mask.4.is_set() {
				*ptr.wrapping_add(4)
			} else {
				core::mem::zeroed()
			},
			if mask.5.is_set() {
				*ptr.wrapping_add(5)
			} else {
				core::mem::zeroed()
			},
			if mask.6.is_set() {
				*ptr.wrapping_add(6)
			} else {
				core::mem::zeroed()
			},
			if mask.7.is_set() {
				*ptr.wrapping_add(7)
			} else {
				core::mem::zeroed()
			},
			if mask.8.is_set() {
				*ptr.wrapping_add(8)
			} else {
				core::mem::zeroed()
			},
			if mask.9.is_set() {
				*ptr.wrapping_add(9)
			} else {
				core::mem::zeroed()
			},
			if mask.10.is_set() {
				*ptr.wrapping_add(10)
			} else {
				core::mem::zeroed()
			},
			if mask.11.is_set() {
				*ptr.wrapping_add(11)
			} else {
				core::mem::zeroed()
			},
			if mask.12.is_set() {
				*ptr.wrapping_add(12)
			} else {
				core::mem::zeroed()
			},
			if mask.13.is_set() {
				*ptr.wrapping_add(13)
			} else {
				core::mem::zeroed()
			},
			if mask.14.is_set() {
				*ptr.wrapping_add(14)
			} else {
				core::mem::zeroed()
			},
			if mask.15.is_set() {
				*ptr.wrapping_add(15)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u16s(self, mask: MemMask<Self::m16s>, ptr: *const u16) -> Self::u16s {
		let mask = mask.mask;
		u16x8(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
			if mask.2.is_set() {
				*ptr.wrapping_add(2)
			} else {
				core::mem::zeroed()
			},
			if mask.3.is_set() {
				*ptr.wrapping_add(3)
			} else {
				core::mem::zeroed()
			},
			if mask.4.is_set() {
				*ptr.wrapping_add(4)
			} else {
				core::mem::zeroed()
			},
			if mask.5.is_set() {
				*ptr.wrapping_add(5)
			} else {
				core::mem::zeroed()
			},
			if mask.6.is_set() {
				*ptr.wrapping_add(6)
			} else {
				core::mem::zeroed()
			},
			if mask.7.is_set() {
				*ptr.wrapping_add(7)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u32s(self, mask: MemMask<Self::m32s>, ptr: *const u32) -> Self::u32s {
		let mask = mask.mask;
		u32x4(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
			if mask.2.is_set() {
				*ptr.wrapping_add(2)
			} else {
				core::mem::zeroed()
			},
			if mask.3.is_set() {
				*ptr.wrapping_add(3)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u64s(self, mask: MemMask<Self::m64s>, ptr: *const u64) -> Self::u64s {
		let mask = mask.mask;
		u64x2(
			if mask.0.is_set() {
				*ptr.wrapping_add(0)
			} else {
				core::mem::zeroed()
			},
			if mask.1.is_set() {
				*ptr.wrapping_add(1)
			} else {
				core::mem::zeroed()
			},
		)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_store_ptr_c32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut c32,
		values: Self::c32s,
	) {
		let mask = mask.mask;
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
	unsafe fn mask_store_ptr_c64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut c64,
		values: Self::c64s,
	) {
		let mask = mask.mask;
		let ptr = ptr as *mut f64;
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
	unsafe fn mask_store_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *mut u8, values: Self::u8s) {
		let mask = mask.mask;
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
		if mask.4.is_set() {
			*ptr.wrapping_add(4) = values.4
		}
		if mask.5.is_set() {
			*ptr.wrapping_add(5) = values.5
		}
		if mask.6.is_set() {
			*ptr.wrapping_add(6) = values.6
		}
		if mask.7.is_set() {
			*ptr.wrapping_add(7) = values.7
		}
		if mask.8.is_set() {
			*ptr.wrapping_add(8) = values.8
		}
		if mask.9.is_set() {
			*ptr.wrapping_add(9) = values.9
		}
		if mask.10.is_set() {
			*ptr.wrapping_add(10) = values.10
		}
		if mask.11.is_set() {
			*ptr.wrapping_add(11) = values.11
		}
		if mask.12.is_set() {
			*ptr.wrapping_add(12) = values.12
		}
		if mask.13.is_set() {
			*ptr.wrapping_add(13) = values.13
		}
		if mask.14.is_set() {
			*ptr.wrapping_add(14) = values.14
		}
		if mask.15.is_set() {
			*ptr.wrapping_add(15) = values.15
		}
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_store_ptr_u16s(
		self,
		mask: MemMask<Self::m16s>,
		ptr: *mut u16,
		values: Self::u16s,
	) {
		let mask = mask.mask;
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
		if mask.4.is_set() {
			*ptr.wrapping_add(4) = values.4
		}
		if mask.5.is_set() {
			*ptr.wrapping_add(5) = values.5
		}
		if mask.6.is_set() {
			*ptr.wrapping_add(6) = values.6
		}
		if mask.7.is_set() {
			*ptr.wrapping_add(7) = values.7
		}
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_store_ptr_u32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut u32,
		values: Self::u32s,
	) {
		let mask = mask.mask;
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
	unsafe fn mask_store_ptr_u64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut u64,
		values: Self::u64s,
	) {
		let mask = mask.mask;
		if mask.0.is_set() {
			*ptr.wrapping_add(0) = values.0
		}
		if mask.1.is_set() {
			*ptr.wrapping_add(1) = values.1
		}
	}

	#[inline(always)]
	fn mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		let a = cast!(a);
		let b = cast!(b);
		let c = cast!(c);
		unsafe { cast!(vcmlaq_90_f32(vcmlaq_0_f32(c, a, b), a, b)) }
	}

	#[inline(always)]
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let a = cast!(a);
		let b = cast!(b);
		let c = cast!(c);
		unsafe { cast!(vcmlaq_90_f64(vcmlaq_0_f64(c, a, b), a, b)) }
	}

	#[inline(always)]
	fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		self.mul_add_f32s(a, b, c)
	}

	#[inline(always)]
	fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		self.mul_add_f64s(a, b, c)
	}

	#[inline(always)]
	fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		self.mul_add_f32x4(a, b, c)
	}

	#[inline(always)]
	fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		self.mul_add_f64x2(a, b, c)
	}

	#[inline(always)]
	fn mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.mul_add_c32s(a, b, self.splat_f32s(0.0))
	}

	#[inline(always)]
	fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.mul_add_c64s(a, b, self.splat_f64s(0.0))
	}

	#[inline(always)]
	fn mul_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		self.mul_f32x4(a, b)
	}

	#[inline(always)]
	fn mul_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		self.mul_f64x2(a, b)
	}

	#[inline(always)]
	fn neg_c32s(self, a: Self::c32s) -> Self::c32s {
		self.xor_f32s(a, self.splat_f32s(-0.0))
	}

	#[inline(always)]
	fn neg_c64s(self, a: Self::c64s) -> Self::c64s {
		self.xor_f64s(a, self.splat_f64s(-0.0))
	}

	#[inline(always)]
	fn partial_load_u32s(self, slice: &[u32]) -> Self::u32s {
		match slice.len() {
			0 => u32x4(0, 0, 0, 0),
			1 => u32x4(slice[0], 0, 0, 0),
			2 => u32x4(slice[0], slice[1], 0, 0),
			3 => u32x4(slice[0], slice[1], slice[2], 0),
			_ => u32x4(slice[0], slice[1], slice[2], slice[3]),
		}
	}

	#[inline(always)]
	fn partial_load_u64s(self, slice: &[u64]) -> Self::u64s {
		match slice.len() {
			0 => u64x2(0, 0),
			1 => u64x2(slice[0], 0),
			_ => u64x2(slice[0], slice[1]),
		}
	}

	#[inline(always)]
	fn partial_store_u32s(self, slice: &mut [u32], values: Self::u32s) {
		match slice.len() {
			0 => {},
			1 => {
				slice[0] = values.0;
			},
			2 => {
				slice[0] = values.0;
				slice[1] = values.1;
			},
			3 => {
				slice[0] = values.0;
				slice[1] = values.1;
				slice[2] = values.2;
			},
			_ => {
				slice[0] = values.0;
				slice[1] = values.1;
				slice[2] = values.2;
				slice[3] = values.3;
			},
		}
	}

	#[inline(always)]
	fn partial_store_u64s(self, slice: &mut [u64], values: Self::u64s) {
		match slice.len() {
			0 => {},
			1 => {
				slice[0] = values.0;
			},
			_ => {
				slice[0] = values.0;
				slice[1] = values.1;
			},
		}
	}

	#[inline(always)]
	fn reduce_max_c32s(self, a: Self::c32s) -> c32 {
		(*self).reduce_max_c32s(a)
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
		self.reduce_max_f32x4(a)
	}

	#[inline(always)]
	fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
		self.reduce_max_f64x2(a)
	}

	#[inline(always)]
	fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
		(*self).reduce_min_c32s(a)
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
		self.reduce_min_f32x4(a)
	}

	#[inline(always)]
	fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
		self.reduce_min_f64x2(a)
	}

	#[inline(always)]
	fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
		self.reduce_product_f32x4(a)
	}

	#[inline(always)]
	fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
		self.reduce_product_f64x2(a)
	}

	#[inline(always)]
	fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
		(*self).reduce_sum_c32s(a)
	}

	#[inline(always)]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
		self.reduce_sum_f32x4(a)
	}

	#[inline(always)]
	fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
		self.reduce_sum_f64x2(a)
	}

	#[inline(always)]
	fn rotate_right_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s {
		unsafe {
			cast!(vqtbl1q_u8(
				cast!(a),
				cast!(NEON_ROTATE_IDX[8 * (amount % 2)]),
			))
		}
	}

	#[inline(always)]
	fn rotate_right_c64s(self, a: Self::c64s, _amount: usize) -> Self::c64s {
		a
	}

	#[inline(always)]
	fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s {
		unsafe {
			cast!(vqtbl1q_u8(
				cast!(a),
				cast!(NEON_ROTATE_IDX[4 * (amount % 4)]),
			))
		}
	}

	#[inline(always)]
	fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s {
		unsafe {
			cast!(vqtbl1q_u8(
				cast!(a),
				cast!(NEON_ROTATE_IDX[8 * (amount % 2)]),
			))
		}
	}

	#[inline(always)]
	fn select_u32s(
		self,
		mask: Self::m32s,
		if_true: Self::u32s,
		if_false: Self::u32s,
	) -> Self::u32s {
		self.select_u32x4(mask, if_true, if_false)
	}

	#[inline(always)]
	fn select_u64s(
		self,
		mask: Self::m64s,
		if_true: Self::u64s,
		if_false: Self::u64s,
	) -> Self::u64s {
		self.select_u64x2(mask, if_true, if_false)
	}

	#[inline(always)]
	fn splat_c32s(self, value: c32) -> Self::c32s {
		cast!(self.splat_f64x2(cast!(value)))
	}

	#[inline(always)]
	fn splat_c64s(self, value: c64) -> Self::c64s {
		cast!(value)
	}

	#[inline(always)]
	fn sub_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.sub_f32x4(a, b)
	}

	#[inline(always)]
	fn sub_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.sub_f64s(a, b)
	}

	#[inline(always)]
	fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s {
		unsafe { cast!(vrev64q_f32(cast!(a))) }
	}

	#[inline(always)]
	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
		unsafe {
			cast!(vcombine_u64(
				vget_high_u64(cast!(a)),
				vget_low_u64(cast!(a)),
			))
		}
	}

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
	fn widening_mul_u32s(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
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
	fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		self.shl_dyn_u32x4(a, self.and_i32x4(cast!(amount), self.splat_i32x4(32 - 1)))
	}

	#[inline(always)]
	fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		u32x4(
			a.0 >> amount.0,
			a.1 >> amount.1,
			a.2 >> amount.2,
			a.3 >> amount.3,
		)
	}

	#[inline(always)]
	fn sqrt_f32s(self, a: Self::f32s) -> Self::f32s {
		cast!(self.neon.vsqrtq_f32(cast!(a)))
	}

	#[inline(always)]
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
		cast!(self.neon.vsqrtq_f64(cast!(a)))
	}
}

#[cfg(miri)]
unsafe fn vfmaq_f64(c: float64x2_t, a: float64x2_t, b: float64x2_t) -> float64x2_t {
	let c: f64x2 = cast!(c);
	let a: f64x2 = cast!(a);
	let b: f64x2 = cast!(b);

	cast!(f64x2(
		crate::fma_f64(a.0, b.0, c.0),
		crate::fma_f64(a.1, b.1, c.1),
	))
}

#[cfg(miri)]
unsafe fn vfmaq_f32(c: float32x4_t, a: float32x4_t, b: float32x4_t) -> float32x4_t {
	let c: f32x4 = cast!(c);
	let a: f32x4 = cast!(a);
	let b: f32x4 = cast!(b);

	cast!(f32x4(
		crate::fma_f32(a.0, b.0, c.0),
		crate::fma_f32(a.1, b.1, c.1),
		crate::fma_f32(a.2, b.2, c.2),
		crate::fma_f32(a.3, b.3, c.3),
	))
}

macro_rules! neon_ty {
	($func: ident, i8) => {
		paste!([<v $func q_ s8>])
	};
	($func: ident, i16) => {
		paste!([<v $func q_ s16>])
	};
	($func: ident, i32) => {
		paste!([<v $func q_ s32>])
	};
	($func: ident, i64) => {
		paste!([<v $func q_ s64>])
	};
	($func: ident, m8) => {
		paste!([<v $func q_ u8>])
	};
	($func: ident, m16) => {
		paste!([<v $func q_ u16>])
	};
	($func: ident, m32) => {
		paste!([<v $func q_ u32>])
	};
	($func: ident, m64) => {
		paste!([<v $func q_ u64>])
	};
	($func: ident, $ty: ident) => {
		paste!([<v $func q_ $ty>])
	};
}

macro_rules! simple_binop {
	($func: ident, $docs: literal, $ty: ident, $out: ident, $factor: literal, $neon_fn: ident) => {
		paste!{
			#[inline(always)]
			#[doc = $docs]
			pub fn [<$func _ $ty x $factor>](self, a: [<$ty x $factor>], b: [<$ty x $factor>]) -> [<$out x $factor>] {
				unsafe { cast!(neon_ty!($neon_fn, $ty)(cast!(a), cast!(b))) }
			}
		}
	};
	($func: ident, $docs: literal, $neon_fn: ident, $($ty: ident x $factor: literal => $out: ident),*) => {
		$(simple_binop!($func, $docs, $ty, $out, $factor, $neon_fn);)*
	};
	($func: ident, $docs: literal, $neon_fn: ident, $($ty: ident x $factor: literal),*) => {
		$(simple_binop!($func, $docs, $ty, $ty, $factor, $neon_fn);)*
	};
	($func: ident, $docs: literal, $($ty: ident x $factor: literal),*) => {
		$(simple_binop!($func, $docs, $ty, $ty, $factor, $func);)*
	};
}

impl Neon {
	simple_binop!(add, "Adds the elements of each lane of `a` and `b`.", u8 x 16, u16 x 8, u32 x 4, u64 x 2, f32 x 4, f64 x 2);

	simple_binop!(mul, "Multiplies the elements of each lane of `a` and `b`.", u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	simple_binop!(sub, "Multiplies the elements of each lane of `a` and `b`.", u8 x 16, u16 x 8, u32 x 4, u64 x 2, f32 x 4, f64 x 2);

	simple_binop!(div, "Divides the elements of each lane of `a` and `b`.", f32 x 4, f64 x 2);

	simple_binop!(and, "Returns the bitwise AND of `a` and `b`.", m8 x 16, u8 x 16, i8 x 16, m16 x 8, u16 x 8, i16 x 8, m32 x 4, u32 x 4, i32 x 4, m64 x 2, u64 x 2, i64 x 2);

	simple_binop!(or, "Returns the bitwise OR of `a` and `b`.", orr, m8 x 16, u8 x 16, i8 x 16, m16 x 8, u16 x 8, i16 x 8, m32 x 4, u32 x 4, i32 x 4, m64 x 2, u64 x 2, i64 x 2);

	simple_binop!(cmp_eq, "Compares the elements in each lane of `a` and `b` for equality.", ceq, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	simple_binop!(cmp_gt, "Compares the elements in each lane of `a` and `b` for greater-than.", cgt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	simple_binop!(cmp_ge, "Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.", cge, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	simple_binop!(cmp_le, "Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.", cle, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	simple_binop!(cmp_lt, "Compares the elements in each lane of `a` and `b` for less-than.", clt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	simple_binop!(min, "Computes the elementwise minimum of each lane of `a` and `b`.", u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	simple_binop!(max, "Computes the elementwise maximum of each lane of `a` and `b`.", u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		unsafe { cast!(vandq_u32(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		unsafe { cast!(vandq_u64(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		self.and_f32x4(cast!(self.not_u32x4(cast!(a))), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		self.and_f64x2(cast!(self.not_u64x2(cast!(a))), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		self.and_i16x8(self.not_i16x8(a), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		self.and_i32x4(self.not_i32x4(a), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		self.and_i64x2(self.not_i64x2(a), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		self.and_i8x16(self.not_i8x16(a), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
		self.and_m16x8(self.not_m16x8(a), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
		self.and_m32x4(self.not_m32x4(a), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
		self.and_m64x2(self.not_m64x2(a), b)
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
	pub fn andnot_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		self.and_u32x4(self.not_u32x4(a), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		self.and_u64x2(self.not_u64x2(a), b)
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		self.and_u8x16(self.not_u8x16(a), b)
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

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		unsafe { cast!(vcltq_f32(cast!(a), cast!(b))) }
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		unsafe { cast!(vcltq_f64(cast!(a), cast!(b))) }
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

	/// Multiplies the elements of each lane of `a` and `b` and adds the result to `c`.
	#[inline(always)]
	pub fn mul_add_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
		unsafe { cast!(vfmaq_f32(cast!(c), cast!(a), cast!(b))) }
	}

	/// Multiplies the elements of each lane of `a` and `b` and adds the result to `c`.
	#[inline(always)]
	pub fn mul_add_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
		unsafe { cast!(vfmaq_f64(cast!(c), cast!(a), cast!(b))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_i16x8(self, a: i16x8) -> i16x8 {
		unsafe { cast!(vmvnq_u16(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_i32x4(self, a: i32x4) -> i32x4 {
		unsafe { cast!(vmvnq_u32(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_i64x2(self, a: i64x2) -> i64x2 {
		unsafe { cast!(vmvnq_u32(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_i8x16(self, a: i8x16) -> i8x16 {
		unsafe { cast!(vmvnq_u8(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_m16x8(self, a: m16x8) -> m16x8 {
		unsafe { cast!(vmvnq_u16(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_m32x4(self, a: m32x4) -> m32x4 {
		unsafe { cast!(vmvnq_u32(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_m64x2(self, a: m64x2) -> m64x2 {
		unsafe { cast!(vmvnq_u32(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_m8x16(self, a: m8x16) -> m8x16 {
		unsafe { cast!(vmvnq_u8(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_u16x8(self, a: u16x8) -> u16x8 {
		unsafe { cast!(vmvnq_u16(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_u32x4(self, a: u32x4) -> u32x4 {
		unsafe { cast!(vmvnq_u32(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_u64x2(self, a: u64x2) -> u64x2 {
		unsafe { cast!(vmvnq_u32(cast!(a))) }
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_u8x16(self, a: u8x16) -> u8x16 {
		unsafe { cast!(vmvnq_u8(cast!(a))) }
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		unsafe { cast!(vorrq_u32(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		unsafe { cast!(vorrq_u64(cast!(a), cast!(b))) }
	}

	#[inline(always)]
	pub fn reduce_max_f32x4(self, a: f32x4) -> f32 {
		unsafe { vmaxnmvq_f32(cast!(a)) }
	}

	#[inline(always)]
	pub fn reduce_max_f64x2(self, a: f64x2) -> f64 {
		unsafe { vmaxnmvq_f64(cast!(a)) }
	}

	#[inline(always)]
	pub fn reduce_min_f32x4(self, a: f32x4) -> f32 {
		unsafe { vminnmvq_f32(cast!(a)) }
	}

	#[inline(always)]
	pub fn reduce_min_f64x2(self, a: f64x2) -> f64 {
		unsafe { vminnmvq_f64(cast!(a)) }
	}

	#[inline(always)]
	pub fn reduce_product_f32x4(self, a: f32x4) -> f32 {
		(a.0 * a.2) * (a.1 * a.3)
	}

	#[inline(always)]
	pub fn reduce_product_f64x2(self, a: f64x2) -> f64 {
		a.0 * a.1
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
		cast!(a)
	}

	#[inline(always)]
	pub fn reduce_sum_f32x4(self, a: f32x4) -> f32 {
		(a.0 + a.2) + (a.1 + a.3)
	}

	#[inline(always)]
	pub fn reduce_sum_f64x2(self, a: f64x2) -> f64 {
		a.0 + a.1
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f32x4(self, mask: m32x4, if_true: f32x4, if_false: f32x4) -> f32x4 {
		unsafe { cast!(vbslq_f32(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f64x2(self, mask: m64x2, if_true: f64x2, if_false: f64x2) -> f64x2 {
		unsafe { cast!(vbslq_f64(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i16x8(self, mask: m16x8, if_true: i16x8, if_false: i16x8) -> i16x8 {
		unsafe { cast!(vbslq_u16(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i32x4(self, mask: m32x4, if_true: i32x4, if_false: i32x4) -> i32x4 {
		unsafe { cast!(vbslq_u32(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i64x2(self, mask: m64x2, if_true: i64x2, if_false: i64x2) -> i64x2 {
		unsafe { cast!(vbslq_u64(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i8x16(self, mask: m8x16, if_true: i8x16, if_false: i8x16) -> i8x16 {
		unsafe { cast!(vbslq_u8(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u16x8(self, mask: m16x8, if_true: u16x8, if_false: u16x8) -> u16x8 {
		unsafe { cast!(vbslq_u16(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u32x4(self, mask: m32x4, if_true: u32x4, if_false: u32x4) -> u32x4 {
		unsafe { cast!(vbslq_u32(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u64x2(self, mask: m64x2, if_true: u64x2, if_false: u64x2) -> u64x2 {
		unsafe { cast!(vbslq_u64(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u8x16(self, mask: m8x16, if_true: u8x16, if_false: u8x16) -> u8x16 {
		unsafe { cast!(vbslq_u8(cast!(mask), cast!(if_true), cast!(if_false),)) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_const_i16x8<const AMOUNT: i32>(self, a: i16x8) -> i16x8 {
		unsafe { cast!(vshlq_n_s16::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_const_i32x4<const AMOUNT: i32>(self, a: i32x4) -> i32x4 {
		unsafe { cast!(vshlq_n_s32::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_const_i64x2<const AMOUNT: i32>(self, a: i64x2) -> i64x2 {
		unsafe { cast!(vshlq_n_s64::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_const_i8x16<const AMOUNT: i32>(self, a: i8x16) -> i8x16 {
		unsafe { cast!(vshlq_n_s8::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_const_u16x8<const AMOUNT: i32>(self, a: u16x8) -> u16x8 {
		unsafe { cast!(vshlq_n_u16::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_const_u32x4<const AMOUNT: i32>(self, a: u32x4) -> u32x4 {
		unsafe { cast!(vshlq_n_u32::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_const_u64x2<const AMOUNT: i32>(self, a: u64x2) -> u64x2 {
		unsafe { cast!(vshlq_n_u64::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_const_u8x16<const AMOUNT: i32>(self, a: u8x16) -> u8x16 {
		unsafe { cast!(vshlq_n_u8::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_dyn_i16x8(self, a: i16x8, amount: i16x8) -> i16x8 {
		unsafe { cast!(vshlq_u16(cast!(a), cast!(amount))) }
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_dyn_i32x4(self, a: i32x4, amount: i32x4) -> i32x4 {
		unsafe { cast!(vshlq_u32(cast!(a), cast!(amount))) }
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_dyn_i64x2(self, a: i64x2, amount: i64x2) -> i64x2 {
		unsafe { cast!(vshlq_u64(cast!(a), cast!(amount))) }
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_dyn_i8x16(self, a: i8x16, amount: i8x16) -> i8x16 {
		unsafe { cast!(vshlq_u8(cast!(a), cast!(amount))) }
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_dyn_u16x8(self, a: u16x8, amount: i16x8) -> u16x8 {
		unsafe { cast!(vshlq_u16(cast!(a), cast!(amount))) }
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_dyn_u32x4(self, a: u32x4, amount: i32x4) -> u32x4 {
		unsafe { cast!(vshlq_u32(cast!(a), cast!(amount))) }
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_dyn_u64x2(self, a: u64x2, amount: i64x2) -> u64x2 {
		unsafe { cast!(vshlq_u64(cast!(a), cast!(amount))) }
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	#[inline(always)]
	pub fn shl_dyn_u8x16(self, a: u8x16, amount: i8x16) -> u8x16 {
		unsafe { cast!(vshlq_u8(cast!(a), cast!(amount))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in sign bits.
	#[inline(always)]
	pub fn shr_const_i16x8<const AMOUNT: i32>(self, a: i16x8) -> i16x8 {
		unsafe { cast!(vshrq_n_s16::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in sign bits.
	#[inline(always)]
	pub fn shr_const_i32x4<const AMOUNT: i32>(self, a: i32x4) -> i32x4 {
		unsafe { cast!(vshrq_n_s32::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in sign bits.
	#[inline(always)]
	pub fn shr_const_i64x2<const AMOUNT: i32>(self, a: i64x2) -> i64x2 {
		unsafe { cast!(vshrq_n_s64::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in sign bits.
	#[inline(always)]
	pub fn shr_const_i8x16<const AMOUNT: i32>(self, a: i8x16) -> i8x16 {
		unsafe { cast!(vshrq_n_s8::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shr_const_u16x8<const AMOUNT: i32>(self, a: u16x8) -> u16x8 {
		unsafe { cast!(vshrq_n_u16::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shr_const_u32x4<const AMOUNT: i32>(self, a: u32x4) -> u32x4 {
		unsafe { cast!(vshrq_n_u32::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shr_const_u64x2<const AMOUNT: i32>(self, a: u64x2) -> u64x2 {
		unsafe { cast!(vshrq_n_u64::<AMOUNT>(cast!(a))) }
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	#[inline(always)]
	pub fn shr_const_u8x16<const AMOUNT: i32>(self, a: u8x16) -> u8x16 {
		unsafe { cast!(vshrq_n_u8::<AMOUNT>(cast!(a))) }
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_f32x4(self, value: f32) -> f32x4 {
		cast!(self.neon.vdupq_n_f32(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_f64x2(self, value: f64) -> f64x2 {
		cast!(self.neon.vdupq_n_f64(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i16x8(self, value: i16) -> i16x8 {
		cast!(self.neon.vdupq_n_s16(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i32x4(self, value: i32) -> i32x4 {
		cast!(self.neon.vdupq_n_s32(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i64x2(self, value: i64) -> i64x2 {
		cast!(self.neon.vdupq_n_s64(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i8x16(self, value: i8) -> i8x16 {
		cast!(self.neon.vdupq_n_s8(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m16x8(self, value: m16) -> m16x8 {
		cast!(self.splat_i16x8(cast!(value)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m32x4(self, value: m32) -> m32x4 {
		cast!(self.splat_i32x4(cast!(value)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m64x2(self, value: m64) -> m64x2 {
		cast!(self.splat_i64x2(cast!(value)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m8x16(self, value: m8) -> m8x16 {
		cast!(self.splat_i8x16(cast!(value)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u16x8(self, value: u16) -> u16x8 {
		cast!(self.splat_i16x8(cast!(value)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u32x4(self, value: u32) -> u32x4 {
		cast!(self.splat_i32x4(cast!(value)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u64x2(self, value: u64) -> u64x2 {
		cast!(self.splat_i64x2(cast!(value)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u8x16(self, value: u8) -> u8x16 {
		cast!(self.splat_i8x16(cast!(value)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		unsafe { cast!(vaddq_s16(cast!(a), cast!(b))) }
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		unsafe { cast!(vaddq_s32(cast!(a), cast!(b))) }
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		unsafe { cast!(vaddq_s64(cast!(a), cast!(b))) }
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		unsafe { cast!(vaddq_s8(cast!(a), cast!(b))) }
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		unsafe { cast!(vaddq_u16(cast!(a), cast!(b))) }
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		unsafe { cast!(vaddq_u32(cast!(a), cast!(b))) }
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		unsafe { cast!(vaddq_u64(cast!(a), cast!(b))) }
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		unsafe { cast!(vaddq_u8(cast!(a), cast!(b))) }
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		unsafe { cast!(vsubq_s16(cast!(a), cast!(b))) }
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		unsafe { cast!(vsubq_s32(cast!(a), cast!(b))) }
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		unsafe { cast!(vsubq_s64(cast!(a), cast!(b))) }
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		unsafe { cast!(vsubq_s8(cast!(a), cast!(b))) }
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		unsafe { cast!(vsubq_u16(cast!(a), cast!(b))) }
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		unsafe { cast!(vsubq_u32(cast!(a), cast!(b))) }
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		unsafe { cast!(vsubq_u64(cast!(a), cast!(b))) }
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		unsafe { cast!(vsubq_u8(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		unsafe { cast!(veorq_u32(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		unsafe { cast!(veorq_u64(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		unsafe { cast!(veorq_u16(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		unsafe { cast!(veorq_u32(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		unsafe { cast!(veorq_u64(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		unsafe { cast!(veorq_u8(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
		unsafe { cast!(veorq_u16(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
		unsafe { cast!(veorq_u32(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
		unsafe { cast!(veorq_u64(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
		unsafe { cast!(veorq_u8(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		unsafe { cast!(veorq_u16(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		unsafe { cast!(veorq_u32(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		unsafe { cast!(veorq_u64(cast!(a), cast!(b))) }
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		unsafe { cast!(veorq_u8(cast!(a), cast!(b))) }
	}
}

/// aarch64 arch
#[derive(Debug, Clone, Copy)]
#[non_exhaustive]
#[repr(u8)]
pub enum Arch {
	Scalar = 0,
	Neon(Neon) = 1,
}

impl Arch {
	/// Detects the best available instruction set.
	#[inline]
	pub fn new() -> Self {
		if let Some(simd) = Neon::try_new() {
			return Self::Neon(simd);
		}
		Self::Scalar
	}

	/// Detects the best available instruction set.
	#[inline(always)]
	pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
		match self {
			Arch::Neon(simd) => Simd::vectorize(simd, op),
			Arch::Scalar => Simd::vectorize(Scalar::new(), op),
		}
	}
}

impl Default for Arch {
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

			let expected = cast!(scalar.mul_c64s(cast!(a), cast!(b)));
			if let Some(simd) = Neon::try_new() {
				let c = simd.mul_c64s(a, b);
				assert_eq_c64(c, expected);
			}
			let expected = cast!(scalar.mul_add_c64s(cast!(a), cast!(b), cast!(acc)));
			if let Some(simd) = Neon::try_new() {
				let c = simd.mul_add_c64s(a, b, acc);
				assert_eq_c64(c, expected);
			}

			let expected = cast!(scalar.conj_mul_c64s(cast!(a), cast!(b)));
			if let Some(simd) = Neon::try_new() {
				let c = simd.conj_mul_c64s(a, b);
				assert_eq_c64(c, expected);
			}
			let expected = cast!(scalar.conj_mul_add_c64s(cast!(a), cast!(b), cast!(acc)));
			if let Some(simd) = Neon::try_new() {
				let c = simd.conj_mul_add_c64s(a, b, acc);
				assert_eq_c64(c, expected);
			}

			let expected = cast!(scalar.mul_c64s(cast!(a), cast!(b)));
			if let Some(simd) = NeonFcma::try_new() {
				let c = simd.mul_c64s(a, b);
				assert_eq_c64(c, expected);
			}
			let expected = cast!(scalar.mul_add_c64s(cast!(a), cast!(b), cast!(acc)));
			if let Some(simd) = NeonFcma::try_new() {
				let c = simd.mul_add_c64s(a, b, acc);
				assert_eq_c64(c, expected);
			}

			let expected = cast!(scalar.conj_mul_c64s(cast!(a), cast!(b)));
			if let Some(simd) = NeonFcma::try_new() {
				let c = simd.conj_mul_c64s(a, b);
				assert_eq_c64(c, expected);
			}
			let expected = cast!(scalar.conj_mul_add_c64s(cast!(a), cast!(b), cast!(acc)));
			if let Some(simd) = NeonFcma::try_new() {
				let c = simd.conj_mul_add_c64s(a, b, acc);
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

			let expected = cast!([
				scalar.mul_c32s(cast!([a.0, a.1]), cast!([b.0, b.1])),
				scalar.mul_c32s(cast!([a.2, a.3]), cast!([b.2, b.3])),
			]);
			if let Some(simd) = Neon::try_new() {
				let c = simd.mul_c32s(a, b);
				assert_eq_c32(c, expected);
			}
			let expected = cast!([
				scalar.mul_add_c32s(cast!([a.0, a.1]), cast!([b.0, b.1]), cast!([acc.0, acc.1])),
				scalar.mul_add_c32s(cast!([a.2, a.3]), cast!([b.2, b.3]), cast!([acc.2, acc.3])),
			]);
			if let Some(simd) = Neon::try_new() {
				let c = simd.mul_add_c32s(a, b, acc);
				assert_eq_c32(c, expected);
			}

			let expected = cast!([
				scalar.conj_mul_c32s(cast!([a.0, a.1]), cast!([b.0, b.1])),
				scalar.conj_mul_c32s(cast!([a.2, a.3]), cast!([b.2, b.3])),
			]);
			if let Some(simd) = Neon::try_new() {
				let c = simd.conj_mul_c32s(a, b);
				assert_eq_c32(c, expected);
			}
			let expected = cast!([
				scalar.conj_mul_add_c32s(
					cast!([a.0, a.1]),
					cast!([b.0, b.1]),
					cast!([acc.0, acc.1])
				),
				scalar.conj_mul_add_c32s(
					cast!([a.2, a.3]),
					cast!([b.2, b.3]),
					cast!([acc.2, acc.3])
				),
			]);
			if let Some(simd) = Neon::try_new() {
				let c = simd.conj_mul_add_c32s(a, b, acc);
				assert_eq_c32(c, expected);
			}

			let expected = cast!([
				scalar.mul_c32s(cast!([a.0, a.1]), cast!([b.0, b.1])),
				scalar.mul_c32s(cast!([a.2, a.3]), cast!([b.2, b.3])),
			]);
			if let Some(simd) = NeonFcma::try_new() {
				let c = simd.mul_c32s(a, b);
				assert_eq_c32(c, expected);
			}
			let expected = cast!([
				scalar.mul_add_c32s(cast!([a.0, a.1]), cast!([b.0, b.1]), cast!([acc.0, acc.1])),
				scalar.mul_add_c32s(cast!([a.2, a.3]), cast!([b.2, b.3]), cast!([acc.2, acc.3])),
			]);
			if let Some(simd) = NeonFcma::try_new() {
				let c = simd.mul_add_c32s(a, b, acc);
				assert_eq_c32(c, expected);
			}

			let expected = cast!([
				scalar.conj_mul_c32s(cast!([a.0, a.1]), cast!([b.0, b.1])),
				scalar.conj_mul_c32s(cast!([a.2, a.3]), cast!([b.2, b.3])),
			]);
			if let Some(simd) = NeonFcma::try_new() {
				let c = simd.conj_mul_c32s(a, b);
				assert_eq_c32(c, expected);
			}
			let expected = cast!([
				scalar.conj_mul_add_c32s(
					cast!([a.0, a.1]),
					cast!([b.0, b.1]),
					cast!([acc.0, acc.1])
				),
				scalar.conj_mul_add_c32s(
					cast!([a.2, a.3]),
					cast!([b.2, b.3]),
					cast!([acc.2, acc.3])
				),
			]);
			if let Some(simd) = NeonFcma::try_new() {
				let c = simd.conj_mul_add_c32s(a, b, acc);
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

				let rot: [u32; 4] = cast!(simd.rotate_right_u32s(cast!(array), amount));
				for i in 0..4 {
					assert_eq!(rot[(i + amount) % 4], array[i]);
				}
			}
			for amount in 0..128 {
				let mut array = [0u64; 2];
				for (i, dst) in array.iter_mut().enumerate() {
					*dst = 1000 + i as u64;
				}

				let rot: [u64; 2] = cast!(simd.rotate_right_u64s(cast!(array), amount));
				for i in 0..2 {
					assert_eq!(rot[(i + amount) % 2], array[i]);
				}
			}
		}
	}

	#[test]
	#[cfg(miri)]
	fn test_sqrt() {
		if let Some(simd) = Neon::try_new() {
			simd.sqrt_f32s(simd.splat_f32s(1.0));
			simd.sqrt_f64s(simd.splat_f64s(1.0));
		}
	}

	#[test]
	fn test_interleave() {
		if let Some(simd) = Neon::try_new() {
			{
				let src = [f64x2(0.0, 0.1), f64x2(1.0, 1.1)];
				let dst = simd.deinterleave_shfl_f64s(src);
				assert_eq!(dst[1], simd.add_f64x2(dst[0], simd.splat_f64x2(0.1)));
				assert_eq!(src, simd.interleave_shfl_f64s(dst));
			}
			{
				let src = [
					f64x2(0.0, 0.1),
					f64x2(0.2, 0.3),
					f64x2(1.0, 1.1),
					f64x2(1.2, 1.3),
				];
				let dst = simd.deinterleave_shfl_f64s(src);
				assert_eq!(dst[1], simd.add_f64x2(dst[0], simd.splat_f64x2(0.1)));
				assert_eq!(dst[2], simd.add_f64x2(dst[0], simd.splat_f64x2(0.2)));
				assert_eq!(dst[3], simd.add_f64x2(dst[0], simd.splat_f64x2(0.3)));
				assert_eq!(src, simd.interleave_shfl_f64s(dst));
			}
			{
				let src = [f32x4(0.0, 0.1, 1.0, 1.1), f32x4(2.0, 2.1, 3.0, 3.1)];
				let dst = simd.deinterleave_shfl_f32s(src);
				assert_eq!(dst[1], simd.add_f32x4(dst[0], simd.splat_f32x4(0.1)));
				assert_eq!(src, simd.interleave_shfl_f32s(dst));
			}
			{
				let src = [
					f32x4(0.0, 0.1, 0.2, 0.3),
					f32x4(1.0, 1.1, 1.2, 1.3),
					f32x4(2.0, 2.1, 2.2, 2.3),
					f32x4(3.0, 3.1, 3.2, 3.3),
				];
				let dst = simd.deinterleave_shfl_f32s(src);
				assert_eq!(dst[1], simd.add_f32x4(dst[0], simd.splat_f32x4(0.1)));
				assert_eq!(dst[2], simd.add_f32x4(dst[0], simd.splat_f32x4(0.2)));
				assert_eq!(dst[3], simd.add_f32x4(dst[0], simd.splat_f32x4(0.3)));
				assert_eq!(src, simd.interleave_shfl_f32s(dst));
			}
		}
	}
}
