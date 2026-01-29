use super::*;
pub use pulp_wasm_simd_flag::*;

simd_type!({
	#[allow(missing_docs)]
	pub struct Simd128 {
		pub simd128: f!("simd128"),
	}

	#[allow(missing_docs)]
	pub struct RelaxedSimd {
		pub simd128: f!("simd128"),
		pub relaxed_simd: f!("relaxed-simd"),
	}
});

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
				cast!(self.simd128.[<$ty x $factor _ $neon_fn>](cast!(a), cast!(b)))
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

macro_rules! impl_bitwise_binop {
	($func: ident, $neon_fn: ident, $ty: ident, $factor: literal, $out: ident) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$out s>] {
				cast!(self.simd128.[<v128 _ $neon_fn>](cast!(a), cast!(b)))
			}
		}
	};
	($func: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_bitwise_binop!($func, $func, $ty, $factor, $ty);)*
	};
	($func: ident, $neon_fn: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_bitwise_binop!($func, $neon_fn, $ty, $factor, $ty);)*
	};
	($func: ident, $neon_fn: ident, $($ty: ident x $factor: literal => $out: ident),*) => {
		$(impl_bitwise_binop!($func, $neon_fn, $ty, $factor, $out);)*
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

macro_rules! impl_bitwise_unop {
	($func: ident, $neon_fn: ident, $ty: ident, $factor: literal, $out: ident) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>]) -> Self::[<$out s>] {
				cast!(self.simd128.[<v128 _ $neon_fn>](cast!(a)))
			}
		}
	};
	($func: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_bitwise_unop!($func, $func, $ty, $factor, $ty);)*
	};
	($func: ident, $neon_fn: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_bitwise_unop!($func, $neon_fn, $ty, $factor, $ty);)*
	};
	($func: ident, $neon_fn: ident, $($ty: ident x $factor: literal => $out: ident),*) => {
		$(impl_bitwise_unop!($func, $neon_fn, $ty, $factor, $out);)*
	};
}

macro_rules! splat {
	($ty: ident, $factor: literal) => {
		paste! {
			#[inline(always)]
			fn [<splat_ $ty s>](self, value: $ty) -> Self::[<$ty s>] {
				cast!(self.simd128.[<$ty x $factor _splat>](cast!(value)))
			}
		}
	};
	($($ty: ident x $factor: literal),*) => {
		$(splat!($ty, $factor);)*
	};
}

impl crate::seal::Seal for Simd128 {}
impl Simd for Simd128 {
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

	impl_binop!(add, u8 x 16, u16 x 8, u32 x 4, u64 x 2);

	impl_binop!(sub, f32 x 4, f64 x 2);

	impl_binop!(sub, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2);

	impl_binop!(mul, u16 x 8, i16 x 8, u32 x 4, i32 x 4);

	impl_binop_scalar!(mul, u64, i64);

	impl_binop!(div, f32 x 4, f64 x 2);

	impl_bitwise_binop!(and, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_bitwise_binop!(or, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_bitwise_binop!(xor, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_bitwise_unop!(not, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_binop!(equal, eq, u8 x 16 => m8, u16 x 8 => m16, u32 x 4 => m32, u64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(greater_than, gt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(greater_than_or_equal, ge, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(less_than_or_equal, le, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(less_than, lt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(max, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_binop_scalar!(max, u64, i64);

	impl_binop!(min, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_binop_scalar!(min, u64, i64);

	splat!(u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2, f32 x 4, f64 x 2);

	#[inline(always)]
	fn greater_than_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		let s = self.splat_u64s(i64::MIN as u64);
		self.greater_than_i64s(cast!(self.xor_u64s(s, a)), cast!(self.xor_u64s(s, b)))
	}

	#[inline(always)]
	fn less_than_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		let s = self.splat_u64s(i64::MIN as u64);
		self.less_than_i64s(cast!(self.xor_u64s(s, a)), cast!(self.xor_u64s(s, b)))
	}

	#[inline(always)]
	fn greater_than_or_equal_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		let s = self.splat_u64s(i64::MIN as u64);
		self.greater_than_or_equal_i64s(cast!(self.xor_u64s(s, a)), cast!(self.xor_u64s(s, b)))
	}

	#[inline(always)]
	fn less_than_or_equal_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		let s = self.splat_u64s(i64::MIN as u64);
		self.less_than_or_equal_i64s(cast!(self.xor_u64s(s, a)), cast!(self.xor_u64s(s, b)))
	}

	#[inline(always)]
	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
		let sqr = self.mul_f32s(a, a);
		let sqr_rev = cast!(
			self.simd128
				.u32x4_shuffle::<1, 0, 3, 2>(cast!(sqr), cast!(sqr))
		);
		self.add_f32s(sqr, sqr_rev)
	}

	#[inline(always)]
	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
		let sqr = self.mul_f64s(a, a);
		let sqr_rev = cast!(self.simd128.u64x2_shuffle::<1, 0>(cast!(sqr), cast!(sqr)));
		self.add_f64s(sqr, sqr_rev)
	}

	#[inline(always)]
	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
		let max = self.max_f32s(a, a);
		let max_rev = cast!(
			self.simd128
				.u32x4_shuffle::<1, 0, 3, 2>(cast!(max), cast!(max))
		);
		self.max_f32s(max, max_rev)
	}

	#[inline(always)]
	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
		let max = self.max_f64s(a, a);
		let max_rev = cast!(self.simd128.u64x2_shuffle::<1, 0>(cast!(max), cast!(max)));
		self.max_f64s(max, max_rev)
	}

	#[inline(always)]
	fn add_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		cast!(self.simd128.f32x4_add(cast!(a), cast!(b)))
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
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(0.0, -0.0, 0.0, -0.0)), cast!(bb))
		);

		cast!(vfmaq_f32(vfmaq_f32(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev128q_f64(xy);
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(0.0, -0.0)), cast!(bb)));

		cast!(vfmaq_f64(vfmaq_f64(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(0.0, -0.0, 0.0, -0.0)), cast!(bb),)
		);

		cast!(vfmaq_f32(self.mul_f32s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev128q_f64(xy);
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(0.0, -0.0)), cast!(bb)));

		cast!(vfmaq_f64(self.mul_f64s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn deinterleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		unsafe { deinterleave_fallback::<f32, Self::f32s, T>(values) }
	}

	#[inline(always)]
	fn deinterleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		unsafe { deinterleave_fallback::<f64, Self::f64s, T>(values) }
	}

	#[inline(always)]
	fn interleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		unsafe { interleave_fallback::<f32, Self::f32s, T>(values) }
	}

	#[inline(always)]
	fn interleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		unsafe { interleave_fallback::<f64, Self::f64s, T>(values) }
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
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(-0.0, 0.0, -0.0, 0.0)), cast!(bb),)
		);

		cast!(vfmaq_f32(vfmaq_f32(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev128q_f64(xy);
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(-0.0, 0.0)), cast!(bb)));

		cast!(vfmaq_f64(vfmaq_f64(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		vfmaq_f32(c, a, b)
	}

	#[inline(always)]
	fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		vfmaq_f64(c, a, b)
	}

	#[inline(always)]
	fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		f32x4(
			fma_f32(a.0, b.0, c.0),
			fma_f32(a.1, b.1, c.1),
			fma_f32(a.2, b.2, c.2),
			fma_f32(a.3, b.3, c.3),
		)
	}

	#[inline(always)]
	fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		f64x2(fma_f64(a.0, b.0, c.0), fma_f64(a.1, b.1, c.1))
	}

	#[inline(always)]
	fn mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(-0.0, 0.0, -0.0, 0.0)), cast!(bb),)
		);

		cast!(vfmaq_f32(self.mul_f32s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = cast!(vrev128q_f64(cast!(xy)));
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(-0.0, 0.0)), cast!(bb)));

		cast!(vfmaq_f64(self.mul_f64s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		cast!(self.simd128.f32x4_mul(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn mul_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		cast!(self.simd128.f64x2_mul(cast!(a), cast!(b)))
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
		// a0 a1 a2 a3
		let a = cast!(a);
		// a2 a3 a2 a3
		let hi = cast!(vrev128q_f64(cast!(a)));

		// a0+a2 a1+a3 _ _
		cast_lossy(self.max_f32s(a, hi))
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
		max_f32(max_f32(a.0, a.2), max_f32(a.1, a.3))
	}

	#[inline(always)]
	fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
		max_f64(a.0, a.1)
	}

	#[inline(always)]
	fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
		// a0 a1 a2 a3
		let a = cast!(a);
		// a2 a3 a2 a3
		let hi = cast!(vrev128q_f64(cast!(a)));

		// a0+a2 a1+a3 _ _
		cast_lossy(self.min_f32s(a, hi))
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
		min_f32(min_f32(a.0, a.2), min_f32(a.1, a.3))
	}

	#[inline(always)]
	fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
		min_f64(a.0, a.1)
	}

	#[inline(always)]
	fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
		(a.0 * a.2) * (a.1 * a.3)
	}

	#[inline(always)]
	fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
		a.0 * a.1
	}

	#[inline(always)]
	fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
		// a0 a1 a2 a3
		let a = cast!(a);
		// a2 a3 a2 a3
		let hi = cast!(vrev128q_f64(cast!(a)));

		// a0+a2 a1+a3 _ _
		cast_lossy(self.add_f32s(a, hi))
	}

	#[inline(always)]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
		let a = self.reduce_sum_c32s(a);
		a.re + a.im
	}

	#[inline(always)]
	fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
		a.0 + a.1
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
			cast!(
				self.simd128
					.i8x16_swizzle(cast!(a), cast!(NEON_ROTATE_IDX[4 * (amount % 4)]),)
			)
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
			cast!(
				self.simd128
					.i8x16_swizzle(cast!(a), cast!(NEON_ROTATE_IDX[8 * (amount % 2)]),)
			)
		}
	}

	#[inline(always)]
	fn select_u32s(
		self,
		mask: Self::m32s,
		if_true: Self::u32s,
		if_false: Self::u32s,
	) -> Self::u32s {
		cast!(
			self.simd128
				.v128_bitselect(cast!(if_true), cast!(if_false), cast!(mask))
		)
	}

	#[inline(always)]
	fn select_u64s(
		self,
		mask: Self::m64s,
		if_true: Self::u64s,
		if_false: Self::u64s,
	) -> Self::u64s {
		cast!(
			self.simd128
				.v128_bitselect(cast!(if_true), cast!(if_false), cast!(mask))
		)
	}

	#[inline(always)]
	fn splat_c32s(self, value: c32) -> Self::c32s {
		cast!(self.simd128.f64x2_splat(cast!(value)))
	}

	#[inline(always)]
	fn splat_c64s(self, value: c64) -> Self::c64s {
		cast!(value)
	}

	#[inline(always)]
	fn sub_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		cast!(self.simd128.f32x4_sub(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn sub_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.sub_f64s(a, b)
	}

	#[inline(always)]
	fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s {
		cast!(vrev64q_f32(cast!(a)))
	}

	#[inline(always)]
	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
		cast!(vrev128q_f64(cast!(a)))
	}

	#[inline(always)]
	fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
		struct Impl<Op> {
			this: Simd128,
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
		u32x4(
			a.0 << amount.0,
			a.1 << amount.1,
			a.2 << amount.2,
			a.3 << amount.3,
		)
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
		cast!(self.simd128.f32x4_sqrt(cast!(a)))
	}

	#[inline(always)]
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
		cast!(self.simd128.f64x2_sqrt(cast!(a)))
	}
}

impl crate::seal::Seal for RelaxedSimd {}
impl Simd for RelaxedSimd {
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

	impl_binop!(add, u8 x 16, u16 x 8, u32 x 4, u64 x 2);

	impl_binop!(sub, f32 x 4, f64 x 2);

	impl_binop!(sub, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2);

	impl_binop!(mul, u16 x 8, i16 x 8, u32 x 4, i32 x 4);

	impl_binop_scalar!(mul, u64, i64);

	impl_binop!(div, f32 x 4, f64 x 2);

	impl_bitwise_binop!(and, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_bitwise_binop!(or, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_bitwise_binop!(xor, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_bitwise_unop!(not, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	impl_binop!(equal, eq, u8 x 16 => m8, u16 x 8 => m16, u32 x 4 => m32, u64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(greater_than, gt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(greater_than_or_equal, ge, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(less_than_or_equal, le, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(less_than, lt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_binop!(max, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_binop_scalar!(max, u64, i64);

	impl_binop!(min, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_binop_scalar!(min, u64, i64);

	splat!(u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2, f32 x 4, f64 x 2);

	#[inline(always)]
	fn greater_than_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		let s = self.splat_u64s(i64::MIN as u64);
		self.greater_than_i64s(cast!(self.xor_u64s(s, a)), cast!(self.xor_u64s(s, b)))
	}

	#[inline(always)]
	fn less_than_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		let s = self.splat_u64s(i64::MIN as u64);
		self.less_than_i64s(cast!(self.xor_u64s(s, a)), cast!(self.xor_u64s(s, b)))
	}

	#[inline(always)]
	fn greater_than_or_equal_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		let s = self.splat_u64s(i64::MIN as u64);
		self.greater_than_or_equal_i64s(cast!(self.xor_u64s(s, a)), cast!(self.xor_u64s(s, b)))
	}

	#[inline(always)]
	fn less_than_or_equal_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		let s = self.splat_u64s(i64::MIN as u64);
		self.less_than_or_equal_i64s(cast!(self.xor_u64s(s, a)), cast!(self.xor_u64s(s, b)))
	}

	#[inline(always)]
	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
		let sqr = self.mul_f32s(a, a);
		let sqr_rev = cast!(
			self.simd128
				.u32x4_shuffle::<1, 0, 3, 2>(cast!(sqr), cast!(sqr))
		);
		self.add_f32s(sqr, sqr_rev)
	}

	#[inline(always)]
	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
		let sqr = self.mul_f64s(a, a);
		let sqr_rev = cast!(self.simd128.u64x2_shuffle::<1, 0>(cast!(sqr), cast!(sqr)));
		self.add_f64s(sqr, sqr_rev)
	}

	#[inline(always)]
	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
		let max = self.max_f32s(a, a);
		let max_rev = cast!(
			self.simd128
				.u32x4_shuffle::<1, 0, 3, 2>(cast!(max), cast!(max))
		);
		self.max_f32s(max, max_rev)
	}

	#[inline(always)]
	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
		let max = self.max_f64s(a, a);
		let max_rev = cast!(self.simd128.u64x2_shuffle::<1, 0>(cast!(max), cast!(max)));
		self.max_f64s(max, max_rev)
	}

	#[inline(always)]
	fn add_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		cast!(self.simd128.f32x4_add(cast!(a), cast!(b)))
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
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(0.0, -0.0, 0.0, -0.0)), cast!(bb))
		);

		cast!(vfmaq_f32(vfmaq_f32(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev128q_f64(xy);
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(0.0, -0.0)), cast!(bb)));

		cast!(vfmaq_f64(vfmaq_f64(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(0.0, -0.0, 0.0, -0.0)), cast!(bb),)
		);

		cast!(vfmaq_f32(self.mul_f32s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev128q_f64(xy);
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(0.0, -0.0)), cast!(bb)));

		cast!(vfmaq_f64(self.mul_f64s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn deinterleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		unsafe { deinterleave_fallback::<f32, Self::f32s, T>(values) }
	}

	#[inline(always)]
	fn deinterleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		unsafe { deinterleave_fallback::<f64, Self::f64s, T>(values) }
	}

	#[inline(always)]
	fn interleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		unsafe { interleave_fallback::<f32, Self::f32s, T>(values) }
	}

	#[inline(always)]
	fn interleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		unsafe { interleave_fallback::<f64, Self::f64s, T>(values) }
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
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(-0.0, 0.0, -0.0, 0.0)), cast!(bb),)
		);

		cast!(vfmaq_f32(vfmaq_f32(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev128q_f64(xy);
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(-0.0, 0.0)), cast!(bb)));

		cast!(vfmaq_f64(vfmaq_f64(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_add_e_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(-0.0, 0.0, -0.0, 0.0)), cast!(bb),)
		);

		cast!(self.mul_add_e_f32s(self.mul_add_e_f32s(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_add_e_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev128q_f64(xy);
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(-0.0, 0.0)), cast!(bb)));

		cast!(self.mul_add_e_f64s(self.mul_add_e_f64s(cast!(c), bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		cast!(
			self.relaxed_simd
				.f32x4_relaxed_madd(cast!(a), cast!(b), cast!(c))
		)
	}

	#[inline(always)]
	fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		cast!(
			self.relaxed_simd
				.f64x2_relaxed_madd(cast!(a), cast!(b), cast!(c))
		)
	}

	#[inline(always)]
	fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		f32x4(
			fma_f32(a.0, b.0, c.0),
			fma_f32(a.1, b.1, c.1),
			fma_f32(a.2, b.2, c.2),
			fma_f32(a.3, b.3, c.3),
		)
	}

	#[inline(always)]
	fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		f64x2(fma_f64(a.0, b.0, c.0), fma_f64(a.1, b.1, c.1))
	}

	#[inline(always)]
	fn mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(-0.0, 0.0, -0.0, 0.0)), cast!(bb),)
		);

		cast!(self.mul_add_f32s(self.mul_f32s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_e_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = vrev64q_f32(xy);
		let aa = vtrn1q_f32(ab);
		let bb = vtrn2q_f32(ab);

		let bb_sign = cast!(
			self.simd128
				.v128_xor(cast!(f32x4(-0.0, 0.0, -0.0, 0.0)), cast!(bb),)
		);

		cast!(self.mul_add_e_f32s(self.mul_f32s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = cast!(vrev128q_f64(cast!(xy)));
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(-0.0, 0.0)), cast!(bb)));

		cast!(self.mul_add_f64s(self.mul_f64s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_e_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = cast!(vrev128q_f64(cast!(xy)));
		let aa = vtrn1q_f64(ab);
		let bb = vtrn2q_f64(ab);

		let bb_sign = cast!(self.simd128.v128_xor(cast!(f64x2(-0.0, 0.0)), cast!(bb)));

		cast!(self.mul_add_e_f64s(self.mul_f64s(bb_sign, yx), aa, xy))
	}

	#[inline(always)]
	fn mul_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		cast!(self.simd128.f32x4_mul(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn mul_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		cast!(self.simd128.f64x2_mul(cast!(a), cast!(b)))
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
		// a0 a1 a2 a3
		let a = cast!(a);
		// a2 a3 a2 a3
		let hi = cast!(vrev128q_f64(cast!(a)));

		// a0+a2 a1+a3 _ _
		cast_lossy(self.max_f32s(a, hi))
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
		max_f32(max_f32(a.0, a.2), max_f32(a.1, a.3))
	}

	#[inline(always)]
	fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
		max_f64(a.0, a.1)
	}

	#[inline(always)]
	fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
		// a0 a1 a2 a3
		let a = cast!(a);
		// a2 a3 a2 a3
		let hi = cast!(vrev128q_f64(cast!(a)));

		// a0+a2 a1+a3 _ _
		cast_lossy(self.min_f32s(a, hi))
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
		min_f32(min_f32(a.0, a.2), min_f32(a.1, a.3))
	}

	#[inline(always)]
	fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
		min_f64(a.0, a.1)
	}

	#[inline(always)]
	fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
		(a.0 * a.2) * (a.1 * a.3)
	}

	#[inline(always)]
	fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
		a.0 * a.1
	}

	#[inline(always)]
	fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
		// a0 a1 a2 a3
		let a = cast!(a);
		// a2 a3 a2 a3
		let hi = cast!(vrev128q_f64(cast!(a)));

		// a0+a2 a1+a3 _ _
		cast_lossy(self.add_f32s(a, hi))
	}

	#[inline(always)]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
		let a = self.reduce_sum_c32s(a);
		a.re + a.im
	}

	#[inline(always)]
	fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
		a.0 + a.1
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
			cast!(
				self.simd128
					.i8x16_swizzle(cast!(a), cast!(NEON_ROTATE_IDX[4 * (amount % 4)]),)
			)
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
			cast!(
				self.simd128
					.i8x16_swizzle(cast!(a), cast!(NEON_ROTATE_IDX[8 * (amount % 2)]),)
			)
		}
	}

	#[inline(always)]
	fn select_u32s(
		self,
		mask: Self::m32s,
		if_true: Self::u32s,
		if_false: Self::u32s,
	) -> Self::u32s {
		cast!(
			self.simd128
				.v128_bitselect(cast!(if_true), cast!(if_false), cast!(mask))
		)
	}

	#[inline(always)]
	fn select_u64s(
		self,
		mask: Self::m64s,
		if_true: Self::u64s,
		if_false: Self::u64s,
	) -> Self::u64s {
		cast!(
			self.simd128
				.v128_bitselect(cast!(if_true), cast!(if_false), cast!(mask))
		)
	}

	#[inline(always)]
	fn splat_c32s(self, value: c32) -> Self::c32s {
		cast!(self.simd128.f64x2_splat(cast!(value)))
	}

	#[inline(always)]
	fn splat_c64s(self, value: c64) -> Self::c64s {
		cast!(value)
	}

	#[inline(always)]
	fn sub_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		cast!(self.simd128.f32x4_sub(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn sub_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.sub_f64s(a, b)
	}

	#[inline(always)]
	fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s {
		cast!(vrev64q_f32(cast!(a)))
	}

	#[inline(always)]
	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
		cast!(vrev128q_f64(cast!(a)))
	}

	#[inline(always)]
	fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
		struct Impl<Op> {
			this: RelaxedSimd,
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
		u32x4(
			a.0 << amount.0,
			a.1 << amount.1,
			a.2 << amount.2,
			a.3 << amount.3,
		)
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
		cast!(self.simd128.f32x4_sqrt(cast!(a)))
	}

	#[inline(always)]
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
		cast!(self.simd128.f64x2_sqrt(cast!(a)))
	}
}

#[inline(always)]
fn vfmaq_f64(c: f64x2, a: f64x2, b: f64x2) -> f64x2 {
	let simd = unsafe { Simd128::new_unchecked() };
	cast!(
		simd.simd128
			.f64x2_add(cast!(c), simd.simd128.f64x2_mul(cast!(a), cast!(b)))
	)
}

#[inline(always)]
fn vfmaq_f32(c: f32x4, a: f32x4, b: f32x4) -> f32x4 {
	let simd = unsafe { Simd128::new_unchecked() };
	cast!(
		simd.simd128
			.f32x4_add(cast!(c), simd.simd128.f32x4_mul(cast!(a), cast!(b)))
	)
}

#[inline(always)]
fn vrev64q_f32(a: f32x4) -> f32x4 {
	let simd = unsafe { Simd128::new_unchecked() };
	cast!(simd.simd128.u32x4_shuffle::<1, 0, 3, 2>(cast!(a), cast!(a)))
}

#[inline(always)]
fn vtrn1q_f32(a: f32x4) -> f32x4 {
	let simd = unsafe { Simd128::new_unchecked() };
	cast!(simd.simd128.u32x4_shuffle::<0, 0, 2, 2>(cast!(a), cast!(a)))
}

#[inline(always)]
fn vtrn2q_f32(a: f32x4) -> f32x4 {
	let simd = unsafe { Simd128::new_unchecked() };
	cast!(simd.simd128.u32x4_shuffle::<1, 1, 3, 3>(cast!(a), cast!(a)))
}

#[inline(always)]
fn vrev128q_f64(a: f64x2) -> f64x2 {
	let simd = unsafe { Simd128::new_unchecked() };
	cast!(simd.simd128.u64x2_shuffle::<1, 0>(cast!(a), cast!(a)))
}

#[inline(always)]
fn vtrn1q_f64(a: f64x2) -> f64x2 {
	let simd = unsafe { Simd128::new_unchecked() };
	cast!(simd.simd128.u64x2_shuffle::<0, 0>(cast!(a), cast!(a)))
}

#[inline(always)]
fn vtrn2q_f64(a: f64x2) -> f64x2 {
	let simd = unsafe { Simd128::new_unchecked() };
	cast!(simd.simd128.u64x2_shuffle::<1, 1>(cast!(a), cast!(a)))
}

#[inline]
fn min_f32(a: f32, b: f32) -> f32 {
	f32::min(a, b)
}

#[inline]
fn min_f64(a: f64, b: f64) -> f64 {
	f64::min(a, b)
}

#[inline]
fn max_f32(a: f32, b: f32) -> f32 {
	f32::max(a, b)
}

#[inline]
fn max_f64(a: f64, b: f64) -> f64 {
	f64::max(a, b)
}

/// wasm arch
#[derive(Debug, Clone, Copy)]
#[non_exhaustive]
#[repr(u8)]
pub enum Arch {
	Scalar = 0,

	RelaxedSimd(RelaxedSimd),
	Simd128(Simd128),
}

impl Arch {
	/// Detects the best available instruction set.
	#[inline]
	pub fn new() -> Self {
		if let Some(simd) = RelaxedSimd::try_new() {
			return Self::RelaxedSimd(simd);
		}
		if let Some(simd) = Simd128::try_new() {
			return Self::Simd128(simd);
		}
		Self::Scalar
	}

	/// Detects the best available instruction set.
	#[inline(always)]
	pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
		match self {
			Arch::RelaxedSimd(simd) => Simd::vectorize(simd, op),
			Arch::Simd128(simd) => Simd::vectorize(simd, op),

			Arch::Scalar => Simd::vectorize(Scalar, op),
		}
	}
}

impl Default for Arch {
	#[inline]
	fn default() -> Self {
		Self::new()
	}
}
