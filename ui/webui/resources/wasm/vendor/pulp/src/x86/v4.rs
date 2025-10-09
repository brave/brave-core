use super::*;

// https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels
simd_type!({
	/// AVX512 instruction set.
	///
	/// Notable additions over [`V3`] include:
	///  - Instructions operating on 512-bit SIMD vectors.
	///  - Masks are now composed of bits rather than vector lanes.
	#[allow(missing_docs)]
	pub struct V4 {
		pub sse: f!("sse"),
		pub sse2: f!("sse2"),
		pub fxsr: f!("fxsr"),
		pub sse3: f!("sse3"),
		pub ssse3: f!("ssse3"),
		pub sse4_1: f!("sse4.1"),
		pub sse4_2: f!("sse4.2"),
		pub popcnt: f!("popcnt"),
		pub avx: f!("avx"),
		pub avx2: f!("avx2"),
		pub bmi1: f!("bmi1"),
		pub bmi2: f!("bmi2"),
		pub fma: f!("fma"),
		pub lzcnt: f!("lzcnt"),
		pub avx512f: f!("avx512f"),
		pub avx512bw: f!("avx512bw"),
		pub avx512cd: f!("avx512cd"),
		pub avx512dq: f!("avx512dq"),
		pub avx512vl: f!("avx512vl"),
	}
});

impl core::ops::Deref for V4 {
	type Target = V3;

	#[inline(always)]
	fn deref(&self) -> &Self::Target {
		V3 {
			sse: self.sse,
			sse2: self.sse2,
			fxsr: self.fxsr,
			sse3: self.sse3,
			ssse3: self.ssse3,
			sse4_1: self.sse4_1,
			sse4_2: self.sse4_2,
			popcnt: self.popcnt,
			avx: self.avx,
			avx2: self.avx2,
			bmi1: self.bmi1,
			bmi2: self.bmi2,
			fma: self.fma,
			lzcnt: self.lzcnt,
		}
		.to_ref()
	}
}

#[inline(always)]
fn avx512_load_u32s(simd: V4, slice: &[u32]) -> u32x16 {
	_ = simd;
	unsafe { avx512_ld_u32s(slice.as_ptr(), LD_ST[2 * (16 * slice.len().min(16))]) }
}

#[inline(always)]
fn avx512_store_u32s(simd: V4, slice: &mut [u32], value: u32x16) {
	_ = simd;
	unsafe {
		avx512_st_u32s(
			slice.as_mut_ptr(),
			value,
			LD_ST[2 * (16 * slice.len().min(16)) + 1],
		);
	}
}

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
static V4_U64_MASKS: [u8; 9] = [
	0b00000000, 0b00000001, 0b00000011, 0b00000111, 0b00001111, 0b00011111, 0b00111111, 0b01111111,
	0b11111111,
];
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
static V4_U64_LAST_MASKS: [u8; 9] = [
	0b00000000, 0b10000000, 0b11000000, 0b11100000, 0b11110000, 0b11111000, 0b11111100, 0b11111110,
	0b11111111,
];

impl Seal for V4 {}

impl Simd for V4 {
	type c32s = f32x16;
	type c64s = f64x8;
	type f32s = f32x16;
	type f64s = f64x8;
	type i32s = i32x16;
	type i64s = i64x8;
	type m32s = b16;
	type m64s = b8;
	type u32s = u32x16;
	type u64s = u64x8;

	const REGISTER_COUNT: usize = 32;

	#[inline(always)]
	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
		let sqr = self.mul_f32s(a, a);
		let sqr_rev = self
			.avx512f
			._mm512_shuffle_ps::<0b10_11_00_01>(cast!(sqr), cast!(sqr));
		self.add_f32s(sqr, cast!(sqr_rev))
	}

	#[inline(always)]
	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
		let sqr = self.mul_f64s(a, a);
		let sqr_rev = self
			.avx512f
			._mm512_shuffle_pd::<0b01010101>(cast!(sqr), cast!(sqr));
		self.add_f64s(sqr, cast!(sqr_rev))
	}

	#[inline(always)]
	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
		let max = self.abs_f32s(a);
		let max_rev = self
			.avx512f
			._mm512_shuffle_ps::<0b10_11_00_01>(cast!(max), cast!(max));
		self.max_f32s(max, cast!(max_rev))
	}

	#[inline(always)]
	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
		let max = self.abs_f64s(a);
		let max_rev = self
			.avx512f
			._mm512_shuffle_pd::<0b01010101>(cast!(max), cast!(max));
		self.max_f64s(max, cast!(max_rev))
	}

	#[inline(always)]
	fn add_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.add_f32s(a, b)
	}

	#[inline(always)]
	fn add_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.add_f64s(a, b)
	}

	#[inline(always)]
	fn add_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		cast!(self.avx512f._mm512_add_ps(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn add_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		cast!(self.avx512f._mm512_add_pd(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn add_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
		cast!(self.avx512f._mm512_add_epi32(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn add_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
		cast!(self.avx512f._mm512_add_epi64(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn and_m32s(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
		b16(a.0 & b.0)
	}

	#[inline(always)]
	fn and_m64s(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
		b8(a.0 & b.0)
	}

	#[inline(always)]
	fn and_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn and_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
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

		let yx = self.avx512f._mm512_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.avx512f._mm512_moveldup_ps(ab);
		let bb = self.avx512f._mm512_movehdup_ps(ab);

		cast!(self.avx512f._mm512_fmsubadd_ps(
			aa,
			xy,
			self.avx512f._mm512_fmsubadd_ps(bb, yx, cast!(c)),
		))
	}

	#[inline(always)]
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx512f._mm512_permute_pd::<0b01010101>(xy);
		let aa = self.avx512f._mm512_unpacklo_pd(ab, ab);
		let bb = self.avx512f._mm512_unpackhi_pd(ab, ab);

		cast!(self.avx512f._mm512_fmsubadd_pd(
			aa,
			xy,
			self.avx512f._mm512_fmsubadd_pd(bb, yx, cast!(c)),
		))
	}

	#[inline(always)]
	fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx512f._mm512_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.avx512f._mm512_moveldup_ps(ab);
		let bb = self.avx512f._mm512_movehdup_ps(ab);

		cast!(
			self.avx512f
				._mm512_fmsubadd_ps(aa, xy, self.avx512f._mm512_mul_ps(bb, yx))
		)
	}

	#[inline(always)]
	fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx512f._mm512_permute_pd::<0b01010101>(xy);
		let aa = self.avx512f._mm512_unpacklo_pd(ab, ab);
		let bb = self.avx512f._mm512_unpackhi_pd(ab, ab);

		cast!(
			self.avx512f
				._mm512_fmsubadd_pd(aa, xy, self.avx512f._mm512_mul_pd(bb, yx))
		)
	}

	#[inline(always)]
	fn deinterleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		let avx = self.avx512f;

		if try_const! { core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f32s>() } {
			let values: [__m512d; 2] = unsafe { core::mem::transmute_copy(&values) };
			// a0 b0 a1 b1 a2 b2 a3 b3
			// a4 b4 a5 b5 a6 b6 a7 b7

			// a0 a4 b0 b4 a2 a6 b2 b6
			// a1 a5 b1 b5 a3 a7 b3 b7
			let values = [
				cast!(avx._mm512_unpacklo_ps(cast!(values[0]), cast!(values[1]))),
				cast!(avx._mm512_unpackhi_ps(cast!(values[0]), cast!(values[1]))),
			];

			// a0 a4 a1 a5 a2 a6 a3 a7
			// b0 b4 b1 b5 b2 b6 b3 b7
			let values = [
				avx._mm512_unpacklo_pd(values[0], values[1]),
				avx._mm512_unpackhi_pd(values[0], values[1]),
			];

			unsafe { core::mem::transmute_copy(&values) }
		} else if try_const! { core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f32s>() }
		{
			// a0 b0 c0 d0 a1 b1 c1 d1
			// a2 b2 c2 d2 a3 b3 c3 d3
			// a4 b4 c4 d4 a5 b5 c5 d5
			// a6 b6 c6 d6 a7 b7 c7 d7
			let values: [__m512d; 4] = unsafe { core::mem::transmute_copy(&values) };

			// a0 a2 c0 c2 a1 a3 c1 c3
			// b0 b2 d0 d2 b1 b3 d1 d3
			// a4 a6 c4 c6 a5 a7 c5 c7
			// b4 b6 d4 d6 b5 b7 d5 d7
			let values = [
				cast!(avx._mm512_unpacklo_ps(cast!(values[0]), cast!(values[1]))),
				cast!(avx._mm512_unpackhi_ps(cast!(values[0]), cast!(values[1]))),
				cast!(avx._mm512_unpacklo_ps(cast!(values[2]), cast!(values[3]))),
				cast!(avx._mm512_unpackhi_ps(cast!(values[2]), cast!(values[3]))),
			];

			let values = [
				avx._mm512_unpacklo_pd(values[0], values[1]),
				avx._mm512_unpackhi_pd(values[0], values[1]),
				avx._mm512_unpacklo_pd(values[2], values[3]),
				avx._mm512_unpackhi_pd(values[2], values[3]),
			];

			// a0 a2 a4 a6 a1 a3 a5 a7
			// b0 b2 b4 b6 b1 b3 b5 b7
			// c0 c2 c4 c6 c1 c3 c5 c7
			// d0 d2 d4 d6 d1 d3 d5 d7
			let values = [
				avx._mm512_unpacklo_pd(values[0], values[2]),
				avx._mm512_unpacklo_pd(values[1], values[3]),
				avx._mm512_unpackhi_pd(values[0], values[2]),
				avx._mm512_unpackhi_pd(values[1], values[3]),
			];

			unsafe { core::mem::transmute_copy(&values) }
		} else {
			unsafe { deinterleave_fallback::<f32, Self::f32s, T>(values) }
		}
	}

	#[inline(always)]
	fn deinterleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		let avx = self.avx512f;

		if try_const! { core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f64s>() } {
			let values: [__m512d; 2] = unsafe { core::mem::transmute_copy(&values) };
			unsafe {
				core::mem::transmute_copy(&[
					avx._mm512_unpacklo_pd(values[0], values[1]),
					avx._mm512_unpackhi_pd(values[0], values[1]),
				])
			}
		} else if try_const! { core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f64s>() }
		{
			let values: [__m512d; 4] = unsafe { core::mem::transmute_copy(&values) };

			// a0 b0 c0 d0
			// a1 b1 c1 d1
			// a2 b2 c2 d2
			// a3 b3 c3 d3

			// a0 a1 c0 c1
			// b0 b1 d0 d1
			// a2 a3 c2 c3
			// b2 b3 d2 d3
			let values: [__m512d; 4] = [
				avx._mm512_unpacklo_pd(values[0], values[1]),
				avx._mm512_unpackhi_pd(values[0], values[1]),
				avx._mm512_unpacklo_pd(values[2], values[3]),
				avx._mm512_unpackhi_pd(values[2], values[3]),
			];

			// a0 a1 a2 a3
			// b0 b1 b2 b3
			// c0 c1 c2 c3
			// d0 d1 d2 d3
			unsafe {
				core::mem::transmute_copy(&[
					self.avx512f
						._mm512_shuffle_f64x2::<0b10_00_10_00>(values[0], values[2]),
					self.avx512f
						._mm512_shuffle_f64x2::<0b10_00_10_00>(values[1], values[3]),
					self.avx512f
						._mm512_shuffle_f64x2::<0b11_01_11_01>(values[0], values[2]),
					self.avx512f
						._mm512_shuffle_f64x2::<0b11_01_11_01>(values[1], values[3]),
				])
			}
		} else {
			unsafe { deinterleave_fallback::<f64, Self::f64s, T>(values) }
		}
	}

	#[inline(always)]
	fn div_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		cast!(self.avx512f._mm512_div_ps(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn div_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		cast!(self.avx512f._mm512_div_pd(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn equal_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_EQ_OQ>(cast!(a), cast!(b))
		)
	}

	#[inline(always)]
	fn equal_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_EQ_OQ>(cast!(a), cast!(b))
		)
	}

	#[inline(always)]
	fn greater_than_or_equal_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
		self.cmp_ge_u32x16(a, b)
	}

	#[inline(always)]
	fn greater_than_or_equal_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		self.cmp_ge_u64x8(a, b)
	}

	#[inline(always)]
	fn greater_than_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
		self.cmp_gt_u32x16(a, b)
	}

	#[inline(always)]
	fn greater_than_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		self.cmp_gt_u64x8(a, b)
	}

	#[inline(always)]
	fn interleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		if try_const! {
			(core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f32s>())
				|| (core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f32s>())
		} {
			// permutation is inverse of itself in this case
			self.deinterleave_shfl_f32s(values)
		} else {
			unsafe { interleave_fallback::<f32, Self::f32s, T>(values) }
		}
	}

	#[inline(always)]
	fn interleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		if try_const! { core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f64s>() } {
			let values: [__m512d; 4] = unsafe { core::mem::transmute_copy(&values) };
			let avx = self.avx512f;

			let values = [
				avx._mm512_shuffle_f64x2::<0b10_00_10_00>(values[0], values[2]),
				avx._mm512_shuffle_f64x2::<0b10_00_10_00>(values[1], values[3]),
				avx._mm512_shuffle_f64x2::<0b11_01_11_01>(values[0], values[2]),
				avx._mm512_shuffle_f64x2::<0b11_01_11_01>(values[1], values[3]),
			];

			let values = [
				avx._mm512_shuffle_f64x2::<0b10_00_10_00>(values[0], values[2]),
				avx._mm512_shuffle_f64x2::<0b10_00_10_00>(values[1], values[3]),
				avx._mm512_shuffle_f64x2::<0b11_01_11_01>(values[0], values[2]),
				avx._mm512_shuffle_f64x2::<0b11_01_11_01>(values[1], values[3]),
			];

			unsafe {
				core::mem::transmute_copy(&[
					avx._mm512_unpacklo_pd(values[0], values[1]),
					avx._mm512_unpackhi_pd(values[0], values[1]),
					avx._mm512_unpacklo_pd(values[2], values[3]),
					avx._mm512_unpackhi_pd(values[2], values[3]),
				])
			}
		} else if try_const! { core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f64s>() }
		{
			// permutation is inverse of itself in this case
			self.deinterleave_shfl_f64s(values)
		} else {
			unsafe { interleave_fallback::<f64, Self::f64s, T>(values) }
		}
	}

	#[inline(always)]
	fn less_than_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_LT_OQ>(cast!(a), cast!(b))
		)
	}

	#[inline(always)]
	fn less_than_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_LT_OQ>(cast!(a), cast!(b))
		)
	}

	#[inline(always)]
	fn less_than_or_equal_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_LE_OQ>(cast!(a), cast!(b))
		)
	}

	#[inline(always)]
	fn less_than_or_equal_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_LE_OQ>(cast!(a), cast!(b))
		)
	}

	#[inline(always)]
	fn less_than_or_equal_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
		self.cmp_le_u32x16(a, b)
	}

	#[inline(always)]
	fn less_than_or_equal_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		self.cmp_le_u64x8(a, b)
	}

	#[inline(always)]
	fn less_than_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
		self.cmp_lt_u32x16(a, b)
	}

	#[inline(always)]
	fn less_than_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
		self.cmp_lt_u64x8(a, b)
	}

	#[inline(always)]
	fn mask_between_m32s(self, start: u32, end: u32) -> MemMask<Self::m32s> {
		let start = start.min(16) as usize;
		let end = end.min(16) as usize;
		MemMask {
			mask: b16(V4_U32_LAST_MASKS[16 - start] & V4_U32_MASKS[end]),
			load: Some(LD_ST[2 * (16 * end + start) + 0]),
			store: Some(LD_ST[2 * (16 * end + start) + 1]),
		}
	}

	#[inline(always)]
	fn mask_between_m64s(self, start: u64, end: u64) -> MemMask<Self::m64s> {
		let start = (2 * start.min(8)) as usize;
		let end = (2 * end.min(8)) as usize;
		MemMask {
			mask: b8(V4_U64_LAST_MASKS[8 - start / 2] & V4_U64_MASKS[end / 2]),
			load: Some(LD_ST[2 * (16 * end + start) + 0]),
			store: Some(LD_ST[2 * (16 * end + start) + 1]),
		}
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_c32s(self, mask: MemMask<Self::m32s>, ptr: *const c32) -> Self::c32s {
		cast!(self.mask_load_ptr_u32s(mask, ptr as _))
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_c64s(self, mask: MemMask<Self::m64s>, ptr: *const c64) -> Self::c64s {
		cast!(self.mask_load_ptr_u64s(mask, ptr as _))
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u32s(self, mask: MemMask<Self::m32s>, ptr: *const u32) -> Self::u32s {
		match mask.load {
			Some(load) => avx512_ld_u32s(ptr, load),
			None => cast!(self.avx512f._mm512_maskz_loadu_epi32(mask.mask.0, ptr as _)),
		}
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u64s(self, mask: MemMask<Self::m64s>, ptr: *const u64) -> Self::u64s {
		match mask.load {
			Some(load) => cast!(avx512_ld_u32s(ptr as _, load)),
			None => cast!(self.avx512f._mm512_maskz_loadu_epi64(mask.mask.0, ptr as _)),
		}
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
		self.mask_store_ptr_u32s(mask, ptr as _, cast!(values))
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
		self.mask_store_ptr_u64s(mask, ptr as _, cast!(values))
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
		match mask.store {
			Some(store) => avx512_st_u32s(ptr, values, store),
			None => {
				self.avx512f
					._mm512_mask_storeu_epi32(ptr as *mut i32, mask.mask.0, cast!(values))
			},
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
		match mask.store {
			Some(store) => avx512_st_u32s(ptr as _, cast!(values), store),
			None => {
				self.avx512f
					._mm512_mask_storeu_epi64(ptr as *mut _, mask.mask.0, cast!(values))
			},
		}
	}

	#[inline(always)]
	fn max_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		cast!(self.avx512f._mm512_max_ps(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn max_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		cast!(self.avx512f._mm512_max_pd(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn min_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		cast!(self.avx512f._mm512_min_ps(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn min_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		cast!(self.avx512f._mm512_min_pd(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx512f._mm512_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.avx512f._mm512_moveldup_ps(ab);
		let bb = self.avx512f._mm512_movehdup_ps(ab);

		cast!(self.avx512f._mm512_fmaddsub_ps(
			aa,
			xy,
			self.avx512f._mm512_fmaddsub_ps(bb, yx, cast!(c)),
		))
	}

	#[inline(always)]
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx512f._mm512_permute_pd::<0b01010101>(xy);
		let aa = self.avx512f._mm512_unpacklo_pd(ab, ab);
		let bb = self.avx512f._mm512_unpackhi_pd(ab, ab);

		cast!(self.avx512f._mm512_fmaddsub_pd(
			aa,
			xy,
			self.avx512f._mm512_fmaddsub_pd(bb, yx, cast!(c)),
		))
	}

	#[inline(always)]
	fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		cast!(self.avx512f._mm512_fmadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	#[inline(always)]
	fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		cast!(self.avx512f._mm512_fmadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	#[inline(always)]
	fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		self.mul_add_e_f32s(a, b, c)
	}

	#[inline(always)]
	fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		self.mul_add_e_f64s(a, b, c)
	}

	#[inline(always)]
	fn mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx512f._mm512_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.avx512f._mm512_moveldup_ps(ab);
		let bb = self.avx512f._mm512_movehdup_ps(ab);

		cast!(
			self.avx512f
				._mm512_fmaddsub_ps(aa, xy, self.avx512f._mm512_mul_ps(bb, yx))
		)
	}

	#[inline(always)]
	fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx512f._mm512_permute_pd::<0b01010101>(xy);
		let aa = self.avx512f._mm512_unpacklo_pd(ab, ab);
		let bb = self.avx512f._mm512_unpackhi_pd(ab, ab);

		cast!(
			self.avx512f
				._mm512_fmaddsub_pd(aa, xy, self.avx512f._mm512_mul_pd(bb, yx))
		)
	}

	#[inline(always)]
	fn mul_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		cast!(self.avx512f._mm512_mul_ps(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn mul_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		cast!(self.avx512f._mm512_mul_pd(cast!(a), cast!(b)))
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
	fn not_m32s(self, a: Self::m32s) -> Self::m32s {
		b16(!a.0)
	}

	#[inline(always)]
	fn not_m64s(self, a: Self::m64s) -> Self::m64s {
		b8(!a.0)
	}

	#[inline(always)]
	fn not_u32s(self, a: Self::u32s) -> Self::u32s {
		cast!(
			self.avx512f
				._mm512_xor_si512(self.avx512f._mm512_set1_epi32(-1), cast!(a))
		)
	}

	#[inline(always)]
	fn not_u64s(self, a: Self::u64s) -> Self::u64s {
		cast!(
			self.avx512f
				._mm512_xor_si512(self.avx512f._mm512_set1_epi32(-1), cast!(a))
		)
	}

	#[inline(always)]
	fn or_m32s(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
		b16(a.0 | b.0)
	}

	#[inline(always)]
	fn or_m64s(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
		b8(a.0 | b.0)
	}

	#[inline(always)]
	fn or_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn or_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn partial_load_u32s(self, slice: &[u32]) -> Self::u32s {
		avx512_load_u32s(self, slice)
	}

	#[inline(always)]
	fn partial_load_u64s(self, slice: &[u64]) -> Self::u64s {
		cast!(avx512_load_u32s(self, bytemuck::cast_slice(slice)))
	}

	#[inline(always)]
	fn partial_store_u32s(self, slice: &mut [u32], values: Self::u32s) {
		avx512_store_u32s(self, slice, values)
	}

	#[inline(always)]
	fn partial_store_u64s(self, slice: &mut [u64], values: Self::u64s) {
		avx512_store_u32s(self, bytemuck::cast_slice_mut(slice), cast!(values))
	}

	#[inline(always)]
	fn reduce_max_c32s(self, a: Self::c32s) -> c32 {
		let a: __m512 = cast!(a);
		let r = self.avx._mm256_max_ps(
			self.avx512f._mm512_castps512_ps256(a),
			cast!(self.avx512f._mm512_extractf64x4_pd::<1>(cast!(a))),
		);
		(*self).reduce_max_c32s(cast!(r))
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		let a: __m512d = cast!(a);
		let r = self.avx._mm256_max_pd(
			self.avx512f._mm512_castpd512_pd256(a),
			self.avx512f._mm512_extractf64x4_pd::<1>(a),
		);
		(*self).reduce_max_c64s(cast!(r))
	}

	#[inline(always)]
	fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
		let a: __m512 = cast!(a);
		let r = self.avx._mm256_max_ps(
			self.avx512f._mm512_castps512_ps256(a),
			cast!(self.avx512f._mm512_extractf64x4_pd::<1>(cast!(a))),
		);
		(*self).reduce_max_f32s(cast!(r))
	}

	#[inline(always)]
	fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
		let a: __m512d = cast!(a);
		let r = self.avx._mm256_max_pd(
			self.avx512f._mm512_castpd512_pd256(a),
			self.avx512f._mm512_extractf64x4_pd::<1>(a),
		);
		(*self).reduce_max_f64s(cast!(r))
	}

	#[inline(always)]
	fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
		let a: __m512 = cast!(a);
		let r = self.avx._mm256_min_ps(
			self.avx512f._mm512_castps512_ps256(a),
			cast!(self.avx512f._mm512_extractf64x4_pd::<1>(cast!(a))),
		);
		(*self).reduce_min_c32s(cast!(r))
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		let a: __m512d = cast!(a);
		let r = self.avx._mm256_min_pd(
			self.avx512f._mm512_castpd512_pd256(a),
			self.avx512f._mm512_extractf64x4_pd::<1>(a),
		);
		(*self).reduce_min_c64s(cast!(r))
	}

	#[inline(always)]
	fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
		let a: __m512 = cast!(a);
		let r = self.avx._mm256_min_ps(
			self.avx512f._mm512_castps512_ps256(a),
			cast!(self.avx512f._mm512_extractf64x4_pd::<1>(cast!(a))),
		);
		(*self).reduce_min_f32s(cast!(r))
	}

	#[inline(always)]
	fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
		let a: __m512d = cast!(a);
		let r = self.avx._mm256_min_pd(
			self.avx512f._mm512_castpd512_pd256(a),
			self.avx512f._mm512_extractf64x4_pd::<1>(a),
		);
		(*self).reduce_min_f64s(cast!(r))
	}

	#[inline(always)]
	fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
		let a: __m512 = cast!(a);
		let r = self.avx._mm256_mul_ps(
			self.avx512f._mm512_castps512_ps256(a),
			cast!(self.avx512f._mm512_extractf64x4_pd::<1>(cast!(a))),
		);
		(*self).reduce_product_f32s(cast!(r))
	}

	#[inline(always)]
	fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
		let a: __m512d = cast!(a);
		let r = self.avx._mm256_mul_pd(
			self.avx512f._mm512_castpd512_pd256(a),
			self.avx512f._mm512_extractf64x4_pd::<1>(a),
		);
		(*self).reduce_product_f64s(cast!(r))
	}

	#[inline(always)]
	fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
		let a: __m512 = cast!(a);
		let r = self.avx._mm256_add_ps(
			self.avx512f._mm512_castps512_ps256(a),
			cast!(self.avx512f._mm512_extractf64x4_pd::<1>(cast!(a))),
		);
		(*self).reduce_sum_c32s(cast!(r))
	}

	#[inline(always)]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		let a: __m512d = cast!(a);
		let r = self.avx._mm256_add_pd(
			self.avx512f._mm512_castpd512_pd256(a),
			self.avx512f._mm512_extractf64x4_pd::<1>(a),
		);
		(*self).reduce_sum_c64s(cast!(r))
	}

	#[inline(always)]
	fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
		let a: __m512 = cast!(a);
		let r = self.avx._mm256_add_ps(
			self.avx512f._mm512_castps512_ps256(a),
			cast!(self.avx512f._mm512_extractf64x4_pd::<1>(cast!(a))),
		);
		(*self).reduce_sum_f32s(cast!(r))
	}

	#[inline(always)]
	fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
		let a: __m512d = cast!(a);
		let r = self.avx._mm256_add_pd(
			self.avx512f._mm512_castpd512_pd256(a),
			self.avx512f._mm512_extractf64x4_pd::<1>(a),
		);
		(*self).reduce_sum_f64s(cast!(r))
	}

	#[inline(always)]
	fn rotate_right_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s {
		cast!(
			self.avx512f
				._mm512_permutexvar_epi32(cast!(AVX512_ROTATE_IDX[2 * (amount % 8)]), cast!(a),)
		)
	}

	#[inline(always)]
	fn rotate_right_c64s(self, a: Self::c64s, amount: usize) -> Self::c64s {
		cast!(
			self.avx512f
				._mm512_permutexvar_epi32(cast!(AVX512_ROTATE_IDX[4 * (amount % 4)]), cast!(a),)
		)
	}

	#[inline(always)]
	fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s {
		cast!(
			self.avx512f
				._mm512_permutexvar_epi32(cast!(AVX512_ROTATE_IDX[amount % 16]), cast!(a),)
		)
	}

	#[inline(always)]
	fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s {
		cast!(
			self.avx512f
				._mm512_permutexvar_epi32(cast!(AVX512_ROTATE_IDX[2 * (amount % 8)]), cast!(a),)
		)
	}

	#[inline(always)]
	fn select_u32s_m32s(
		self,
		mask: Self::m32s,
		if_true: Self::u32s,
		if_false: Self::u32s,
	) -> Self::u32s {
		let mask: __mmask16 = mask.0;
		let if_true: __m512 = cast!(if_true);
		let if_false: __m512 = cast!(if_false);

		cast!(self.avx512f._mm512_mask_blend_ps(mask, if_false, if_true))
	}

	#[inline(always)]
	fn select_u64s_m64s(
		self,
		mask: Self::m64s,
		if_true: Self::u64s,
		if_false: Self::u64s,
	) -> Self::u64s {
		let mask: __mmask8 = mask.0;
		let if_true: __m512d = cast!(if_true);
		let if_false: __m512d = cast!(if_false);

		cast!(self.avx512f._mm512_mask_blend_pd(mask, if_false, if_true))
	}

	#[inline(always)]
	fn splat_c32s(self, value: c32) -> Self::c32s {
		cast!(self.splat_f64s(cast!(value)))
	}

	#[inline(always)]
	fn splat_c64s(self, value: c64) -> Self::c64s {
		cast!(self.avx512f._mm512_broadcast_f32x4(cast!(value)))
	}

	#[inline(always)]
	fn splat_f32s(self, value: f32) -> Self::f32s {
		cast!(self.avx512f._mm512_set1_ps(value))
	}

	#[inline(always)]
	fn splat_f64s(self, value: f64) -> Self::f64s {
		cast!(self.avx512f._mm512_set1_pd(value))
	}

	#[inline(always)]
	fn splat_u32s(self, value: u32) -> Self::u32s {
		cast!(self.avx512f._mm512_set1_epi32(value as i32))
	}

	#[inline(always)]
	fn splat_u64s(self, value: u64) -> Self::u64s {
		cast!(self.avx512f._mm512_set1_epi64(value as i64))
	}

	#[inline(always)]
	fn sub_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.sub_f32s(a, b)
	}

	#[inline(always)]
	fn sub_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.sub_f64s(a, b)
	}

	#[inline(always)]
	fn sub_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
		cast!(self.avx512f._mm512_sub_ps(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn sub_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
		cast!(self.avx512f._mm512_sub_pd(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn sub_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
		cast!(self.avx512f._mm512_sub_epi32(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn sub_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
		cast!(self.avx512f._mm512_sub_epi64(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s {
		cast!(self.avx512f._mm512_permute_ps::<0b10_11_00_01>(cast!(a)))
	}

	#[inline(always)]
	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
		cast!(self.avx512f._mm512_permute_pd::<0b01010101>(cast!(a)))
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
	fn widening_mul_u32s(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
		self.widening_mul_u32x16(a, b)
	}

	#[inline(always)]
	fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		self.shl_dyn_u32x16(a, self.and_u32x16(amount, self.splat_u32x16(32 - 1)))
	}

	#[inline(always)]
	fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		self.shr_dyn_u32x16(a, self.and_u32x16(amount, self.splat_u32x16(32 - 1)))
	}

	#[inline(always)]
	fn xor_m32s(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
		b16(a.0 ^ b.0)
	}

	#[inline(always)]
	fn xor_m64s(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
		b8(a.0 ^ b.0)
	}

	#[inline(always)]
	fn xor_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn xor_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	#[inline(always)]
	fn greater_than_or_equal_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::m32s {
		self.cmp_ge_i32x16(a, b)
	}

	#[inline(always)]
	fn greater_than_or_equal_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::m64s {
		self.cmp_ge_i64x8(a, b)
	}

	#[inline(always)]
	fn greater_than_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::m32s {
		self.cmp_gt_i32x16(a, b)
	}

	#[inline(always)]
	fn greater_than_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::m64s {
		self.cmp_gt_i64x8(a, b)
	}

	#[inline(always)]
	fn less_than_or_equal_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::m32s {
		self.cmp_le_i32x16(a, b)
	}

	#[inline(always)]
	fn less_than_or_equal_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::m64s {
		self.cmp_le_i64x8(a, b)
	}

	#[inline(always)]
	fn less_than_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::m32s {
		self.cmp_lt_i32x16(a, b)
	}

	#[inline(always)]
	fn less_than_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::m64s {
		self.cmp_lt_i64x8(a, b)
	}
}

impl V4 {
	/// Computes the absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn abs_f32x16(self, a: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_abs_ps(cast!(a)))
	}

	/// Computes the absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn abs_f64x8(self, a: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_abs_pd(cast!(a)))
	}

	/// Adds the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn add_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_add_ps(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn add_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_add_pd(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of `a` and `b`.
	#[inline(always)]
	pub fn and_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512f._mm512_and_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise AND of NOT `a` and `b`.
	#[inline(always)]
	pub fn andnot_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512f._mm512_andnot_si512(cast!(a), cast!(b)))
	}

	/// Averages the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn average_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_avg_epu16(cast!(a), cast!(b)))
	}

	/// Averages the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn average_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512bw._mm512_avg_epu8(cast!(a), cast!(b)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards positive infinity.
	#[inline(always)]
	pub fn ceil_f32x16(self, a: f32x16) -> f32x16 {
		cast!(
			self.avx512f
				._mm512_roundscale_ps::<_MM_FROUND_TO_POS_INF>(cast!(a)),
		)
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards positive infinity.
	#[inline(always)]
	pub fn ceil_f64x8(self, a: f64x8) -> f64x8 {
		cast!(
			self.avx512f
				._mm512_roundscale_pd::<_MM_FROUND_TO_POS_INF>(cast!(a)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_EQ_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_EQ_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_EQ_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_EQ_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i16x16(self, a: i16x16, b: i16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpeq_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i16x32(self, a: i16x32, b: i16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmpeq_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i32x16(self, a: i32x16, b: i32x16) -> b16 {
		cast!(self.avx512f._mm512_cmpeq_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i32x8(self, a: i32x8, b: i32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpeq_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i64x4(self, a: i64x4, b: i64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpeq_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i64x8(self, a: i64x8, b: i64x8) -> b8 {
		cast!(self.avx512f._mm512_cmpeq_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i8x32(self, a: i8x32, b: i8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpeq_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i8x64(self, a: i8x64, b: i8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmpeq_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u16x16(self, a: u16x16, b: u16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpeq_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u16x32(self, a: u16x32, b: u16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmpeq_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u32x16(self, a: u32x16, b: u32x16) -> b16 {
		cast!(self.avx512f._mm512_cmpeq_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u32x8(self, a: u32x8, b: u32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpeq_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u64x4(self, a: u64x4, b: u64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpeq_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u64x8(self, a: u64x8, b: u64x8) -> b8 {
		cast!(self.avx512f._mm512_cmpeq_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u8x32(self, a: u8x32, b: u8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpeq_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u8x64(self, a: u8x64, b: u8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmpeq_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_GE_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_GE_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_GE_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_GE_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i16x16(self, a: i16x16, b: i16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpge_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i16x32(self, a: i16x32, b: i16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmpge_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i32x16(self, a: i32x16, b: i32x16) -> b16 {
		cast!(self.avx512f._mm512_cmpge_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i32x8(self, a: i32x8, b: i32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpge_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i64x4(self, a: i64x4, b: i64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpge_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i64x8(self, a: i64x8, b: i64x8) -> b8 {
		cast!(self.avx512f._mm512_cmpge_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i8x32(self, a: i8x32, b: i8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpge_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i8x64(self, a: i8x64, b: i8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmpge_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u16x16(self, a: u16x16, b: u16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpge_epu16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u16x32(self, a: u16x32, b: u16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmpge_epu16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u32x16(self, a: u32x16, b: u32x16) -> b16 {
		cast!(self.avx512f._mm512_cmpge_epu32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u32x8(self, a: u32x8, b: u32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpge_epu32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u64x4(self, a: u64x4, b: u64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpge_epu64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u64x8(self, a: u64x8, b: u64x8) -> b8 {
		cast!(self.avx512f._mm512_cmpge_epu64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u8x32(self, a: u8x32, b: u8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpge_epu8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u8x64(self, a: u8x64, b: u8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmpge_epu8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_GT_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_GT_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_GT_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_GT_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i16x16(self, a: i16x16, b: i16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpgt_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i16x32(self, a: i16x32, b: i16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmpgt_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i32x16(self, a: i32x16, b: i32x16) -> b16 {
		cast!(self.avx512f._mm512_cmpgt_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i32x8(self, a: i32x8, b: i32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpgt_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i64x4(self, a: i64x4, b: i64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpgt_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i64x8(self, a: i64x8, b: i64x8) -> b8 {
		cast!(self.avx512f._mm512_cmpgt_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i8x32(self, a: i8x32, b: i8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpgt_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i8x64(self, a: i8x64, b: i8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmpgt_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u16x16(self, a: u16x16, b: u16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpgt_epu16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u16x32(self, a: u16x32, b: u16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmpgt_epu16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u32x16(self, a: u32x16, b: u32x16) -> b16 {
		cast!(self.avx512f._mm512_cmpgt_epu32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u32x8(self, a: u32x8, b: u32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpgt_epu32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u64x4(self, a: u64x4, b: u64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmpgt_epu64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u64x8(self, a: u64x8, b: u64x8) -> b8 {
		cast!(self.avx512f._mm512_cmpgt_epu64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u8x32(self, a: u8x32, b: u8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmpgt_epu8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u8x64(self, a: u8x64, b: u8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmpgt_epu8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_LE_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_LE_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_LE_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_LE_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i16x16(self, a: i16x16, b: i16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmple_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i16x32(self, a: i16x32, b: i16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmple_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i32x16(self, a: i32x16, b: i32x16) -> b16 {
		cast!(self.avx512f._mm512_cmple_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i32x8(self, a: i32x8, b: i32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmple_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i64x4(self, a: i64x4, b: i64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmple_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i64x8(self, a: i64x8, b: i64x8) -> b8 {
		cast!(self.avx512f._mm512_cmple_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i8x32(self, a: i8x32, b: i8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmple_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i8x64(self, a: i8x64, b: i8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmple_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u16x16(self, a: u16x16, b: u16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmple_epu16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u16x32(self, a: u16x32, b: u16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmple_epu16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u32x16(self, a: u32x16, b: u32x16) -> b16 {
		cast!(self.avx512f._mm512_cmple_epu32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u32x8(self, a: u32x8, b: u32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmple_epu32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u64x4(self, a: u64x4, b: u64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmple_epu64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u64x8(self, a: u64x8, b: u64x8) -> b8 {
		cast!(self.avx512f._mm512_cmple_epu64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u8x32(self, a: u8x32, b: u8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmple_epu8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u8x64(self, a: u8x64, b: u8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmple_epu8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_LT_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_LT_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_LT_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_LT_OQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i16x16(self, a: i16x16, b: i16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmplt_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i16x32(self, a: i16x32, b: i16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmplt_epi16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i32x16(self, a: i32x16, b: i32x16) -> b16 {
		cast!(self.avx512f._mm512_cmplt_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i32x8(self, a: i32x8, b: i32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmplt_epi32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i64x4(self, a: i64x4, b: i64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmplt_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i64x8(self, a: i64x8, b: i64x8) -> b8 {
		cast!(self.avx512f._mm512_cmplt_epi64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i8x32(self, a: i8x32, b: i8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmplt_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i8x64(self, a: i8x64, b: i8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmplt_epi8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u16x16(self, a: u16x16, b: u16x16) -> b16 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmplt_epu16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u16x32(self, a: u16x32, b: u16x32) -> b32 {
		cast!(self.avx512bw._mm512_cmplt_epu16_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u32x16(self, a: u32x16, b: u32x16) -> b16 {
		cast!(self.avx512f._mm512_cmplt_epu32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u32x8(self, a: u32x8, b: u32x8) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmplt_epu32_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u64x4(self, a: u64x4, b: u64x4) -> b8 {
		let simd = self.avx512f;
		cast!(simd._mm256_cmplt_epu64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u64x8(self, a: u64x8, b: u64x8) -> b8 {
		cast!(self.avx512f._mm512_cmplt_epu64_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u8x32(self, a: u8x32, b: u8x32) -> b32 {
		let simd = self.avx512bw;
		cast!(simd._mm256_cmplt_epu8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u8x64(self, a: u8x64, b: u8x64) -> b64 {
		cast!(self.avx512bw._mm512_cmplt_epu8_mask(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for inequality.
	#[inline(always)]
	pub fn cmp_not_eq_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_NEQ_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for inequality.
	#[inline(always)]
	pub fn cmp_not_eq_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_NEQ_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for inequality.
	#[inline(always)]
	pub fn cmp_not_eq_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_NEQ_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for inequality.
	#[inline(always)]
	pub fn cmp_not_eq_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_NEQ_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_ge_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_NGE_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_ge_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_NGE_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_ge_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_NGE_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_ge_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_NGE_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than.
	#[inline(always)]
	pub fn cmp_not_gt_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_NGT_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than.
	#[inline(always)]
	pub fn cmp_not_gt_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_NGT_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than.
	#[inline(always)]
	pub fn cmp_not_gt_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_NGT_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than.
	#[inline(always)]
	pub fn cmp_not_gt_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_NGT_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_le_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_NLE_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_le_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_NLE_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_le_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_NLE_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_le_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_NLE_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f32x16(self, a: f32x16, b: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_NLT_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f32x8(self, a: f32x8, b: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_NLT_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f64x4(self, a: f64x4, b: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_NLT_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f64x8(self, a: f64x8, b: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_NLT_UQ>(cast!(a), cast!(b)),
		)
	}

	/// Converts a `f32x16` to `i32x16`, elementwise.
	#[inline(always)]
	pub fn convert_f32x16_to_i32x16(self, a: f32x16) -> i32x16 {
		cast!(self.avx512f._mm512_cvttps_epi32(cast!(a)))
	}

	/// Converts a `f32x16` to `u32x16`, elementwise.
	#[inline(always)]
	pub fn convert_f32x16_to_u32x16(self, a: f32x16) -> u32x16 {
		cast!(self.avx512f._mm512_cvttps_epu32(cast!(a)))
	}

	/// Converts a `f32x4` to `u32x4`, elementwise.
	#[inline(always)]
	pub fn convert_f32x4_to_u32x4(self, a: f32x4) -> u32x4 {
		cast!(self.avx512f._mm_cvttps_epu32(cast!(a)))
	}

	/// Converts a `f32x8` to `f64x8`, elementwise.
	#[inline(always)]
	pub fn convert_f32x8_to_f64x8(self, a: f32x8) -> f64x8 {
		cast!(self.avx512f._mm512_cvtps_pd(cast!(a)))
	}

	/// Converts a `f32x8` to `u32x8`, elementwise.
	#[inline(always)]
	pub fn convert_f32x8_to_u32x8(self, a: f32x8) -> u32x8 {
		cast!(self.avx512f._mm256_cvttps_epu32(cast!(a)))
	}

	/// Converts a `f64x2` to `u32x4`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_f64x2_to_u32x4(self, a: f64x2) -> u32x4 {
		cast!(self.avx512f._mm_cvttpd_epu32(cast!(a)))
	}

	/// Converts a `f64x4` to `u32x4`, elementwise.
	#[inline(always)]
	pub fn convert_f64x4_to_u32x4(self, a: f64x4) -> u32x4 {
		cast!(self.avx512f._mm256_cvttpd_epu32(cast!(a)))
	}

	/// Converts a `f64x8` to `f32x8`, elementwise.
	#[inline(always)]
	pub fn convert_f64x8_to_f32x8(self, a: f64x8) -> f32x8 {
		cast!(self.avx512f._mm512_cvtpd_ps(cast!(a)))
	}

	/// Converts a `f64x8` to `i32x8`, elementwise.
	#[inline(always)]
	pub fn convert_f64x8_to_i32x8(self, a: f64x8) -> i32x8 {
		cast!(self.avx512f._mm512_cvttpd_epi32(cast!(a)))
	}

	/// Converts a `f64x8` to `u32x8`, elementwise.
	#[inline(always)]
	pub fn convert_f64x8_to_u32x8(self, a: f64x8) -> u32x8 {
		cast!(self.avx512f._mm512_cvttpd_epu32(cast!(a)))
	}

	/// Converts a `i16x16` to `i32x16`, elementwise.
	#[inline(always)]
	pub fn convert_i16x16_to_i32x16(self, a: i16x16) -> i32x16 {
		cast!(self.avx512f._mm512_cvtepi16_epi32(cast!(a)))
	}

	/// Converts a `i16x16` to `i8x16`, elementwise.
	#[inline(always)]
	pub fn convert_i16x16_to_i8x16(self, a: i16x16) -> i8x16 {
		cast!(self.avx512bw._mm256_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `i16x16` to `u32x16`, elementwise.
	#[inline(always)]
	pub fn convert_i16x16_to_u32x16(self, a: i16x16) -> u32x16 {
		cast!(self.avx512f._mm512_cvtepi16_epi32(cast!(a)))
	}

	/// Converts a `i16x16` to `u8x16`, elementwise.
	#[inline(always)]
	pub fn convert_i16x16_to_u8x16(self, a: i16x16) -> u8x16 {
		cast!(self.avx512bw._mm256_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `i16x32` to `i8x32`, elementwise.
	#[inline(always)]
	pub fn convert_i16x32_to_i8x32(self, a: i16x32) -> i8x32 {
		cast!(self.avx512bw._mm512_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `i16x32` to `u16x32`, elementwise.
	#[inline(always)]
	pub fn convert_i16x32_to_u16x32(self, a: i16x32) -> u16x32 {
		cast!(a)
	}

	/// Converts a `i16x32` to `u8x32`, elementwise.
	#[inline(always)]
	pub fn convert_i16x32_to_u8x32(self, a: i16x32) -> u8x32 {
		cast!(self.avx512bw._mm512_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `i16x8` to `i64x8`, elementwise.
	#[inline(always)]
	pub fn convert_i16x8_to_i64x8(self, a: i16x8) -> i64x8 {
		cast!(self.avx512f._mm512_cvtepi16_epi64(cast!(a)))
	}

	/// Converts a `i16x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i16x8_to_i8x16(self, a: i16x8) -> i8x16 {
		cast!(self.avx512bw._mm_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `i16x8` to `u64x8`, elementwise.
	#[inline(always)]
	pub fn convert_i16x8_to_u64x8(self, a: i16x8) -> u64x8 {
		cast!(self.avx512f._mm512_cvtepi16_epi64(cast!(a)))
	}

	/// Converts a `i16x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i16x8_to_u8x16(self, a: i16x8) -> u8x16 {
		cast!(self.avx512bw._mm_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `i32x16` to `f32x16`, elementwise.
	#[inline(always)]
	pub fn convert_i32x16_to_f32x16(self, a: i32x16) -> f32x16 {
		cast!(self.avx512f._mm512_cvtepi32_ps(cast!(a)))
	}

	/// Converts a `i32x16` to `i16x16`, elementwise.
	#[inline(always)]
	pub fn convert_i32x16_to_i16x16(self, a: i32x16) -> i16x16 {
		cast!(self.avx512f._mm512_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `i32x16` to `i8x16`, elementwise.
	#[inline(always)]
	pub fn convert_i32x16_to_i8x16(self, a: i32x16) -> i8x16 {
		cast!(self.avx512f._mm512_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `i32x16` to `u16x16`, elementwise.
	#[inline(always)]
	pub fn convert_i32x16_to_u16x16(self, a: i32x16) -> u16x16 {
		cast!(self.avx512f._mm512_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `i32x16` to `u32x16`, elementwise.
	#[inline(always)]
	pub fn convert_i32x16_to_u32x16(self, a: i32x16) -> u32x16 {
		cast!(a)
	}

	/// Converts a `i32x16` to `u8x16`, elementwise.
	#[inline(always)]
	pub fn convert_i32x16_to_u8x16(self, a: i32x16) -> u8x16 {
		cast!(self.avx512f._mm512_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `i32x4` to `i16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i32x4_to_i16x8(self, a: i32x4) -> i16x8 {
		cast!(self.avx512f._mm_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `i32x4` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i32x4_to_i8x16(self, a: i32x4) -> i8x16 {
		cast!(self.avx512f._mm_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `i32x4` to `u16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i32x4_to_u16x8(self, a: i32x4) -> u16x8 {
		cast!(self.avx512f._mm_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `i32x4` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i32x4_to_u8x16(self, a: i32x4) -> u8x16 {
		cast!(self.avx512f._mm_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `i32x8` to `f64x8`, elementwise.
	#[inline(always)]
	pub fn convert_i32x8_to_f64x8(self, a: i32x8) -> f64x8 {
		cast!(self.avx512f._mm512_cvtepi32_pd(cast!(a)))
	}

	/// Converts a `i32x8` to `i16x8`, elementwise.
	#[inline(always)]
	pub fn convert_i32x8_to_i16x8(self, a: i32x8) -> i16x8 {
		cast!(self.avx512f._mm256_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `i32x8` to `i64x8`, elementwise.
	#[inline(always)]
	pub fn convert_i32x8_to_i64x8(self, a: i32x8) -> i64x8 {
		cast!(self.avx512f._mm512_cvtepi32_epi64(cast!(a)))
	}

	/// Converts a `i32x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i32x8_to_i8x16(self, a: i32x8) -> i8x16 {
		cast!(self.avx512f._mm256_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `i32x8` to `u16x8`, elementwise.
	#[inline(always)]
	pub fn convert_i32x8_to_u16x8(self, a: i32x8) -> u16x8 {
		cast!(self.avx512f._mm256_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `i32x8` to `u64x8`, elementwise.
	#[inline(always)]
	pub fn convert_i32x8_to_u64x8(self, a: i32x8) -> u64x8 {
		cast!(self.avx512f._mm512_cvtepi32_epi64(cast!(a)))
	}

	/// Converts a `i32x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i32x8_to_u8x16(self, a: i32x8) -> u8x16 {
		cast!(self.avx512f._mm256_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `i64x2` to `i16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x2_to_i16x8(self, a: i64x2) -> i16x8 {
		cast!(self.avx512f._mm_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `i64x2` to `i32x4`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x2_to_i32x4(self, a: i64x2) -> i32x4 {
		cast!(self.avx512f._mm_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `i64x2` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x2_to_i8x16(self, a: i64x2) -> i8x16 {
		cast!(self.avx512f._mm_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `i64x2` to `u16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x2_to_u16x8(self, a: i64x2) -> u16x8 {
		cast!(self.avx512f._mm_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `i64x2` to `u32x4`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x2_to_u32x4(self, a: i64x2) -> u32x4 {
		cast!(self.avx512f._mm_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `i64x2` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x2_to_u8x16(self, a: i64x2) -> u8x16 {
		cast!(self.avx512f._mm_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `i64x4` to `i16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x4_to_i16x8(self, a: i64x4) -> i16x8 {
		cast!(self.avx512f._mm256_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `i64x4` to `i32x4`, elementwise.
	#[inline(always)]
	pub fn convert_i64x4_to_i32x4(self, a: i64x4) -> i32x4 {
		cast!(self.avx512f._mm256_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `i64x4` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x4_to_i8x16(self, a: i64x4) -> i8x16 {
		cast!(self.avx512f._mm256_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `i64x4` to `u16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x4_to_u16x8(self, a: i64x4) -> u16x8 {
		cast!(self.avx512f._mm256_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `i64x4` to `u32x4`, elementwise.
	#[inline(always)]
	pub fn convert_i64x4_to_u32x4(self, a: i64x4) -> u32x4 {
		cast!(self.avx512f._mm256_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `i64x4` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x4_to_u8x16(self, a: i64x4) -> u8x16 {
		cast!(self.avx512f._mm256_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `i64x8` to `i16x8`, elementwise.
	#[inline(always)]
	pub fn convert_i64x8_to_i16x8(self, a: i64x8) -> i16x8 {
		cast!(self.avx512f._mm512_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `i64x8` to `i32x8`, elementwise.
	#[inline(always)]
	pub fn convert_i64x8_to_i32x8(self, a: i64x8) -> i32x8 {
		cast!(self.avx512f._mm512_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `i64x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x8_to_i8x16(self, a: i64x8) -> i8x16 {
		cast!(self.avx512f._mm512_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `i64x8` to `u16x8`, elementwise.
	#[inline(always)]
	pub fn convert_i64x8_to_u16x8(self, a: i64x8) -> u16x8 {
		cast!(self.avx512f._mm512_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `i64x8` to `u32x8`, elementwise.
	#[inline(always)]
	pub fn convert_i64x8_to_u32x8(self, a: i64x8) -> u32x8 {
		cast!(self.avx512f._mm512_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `i64x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_i64x8_to_u8x16(self, a: i64x8) -> u8x16 {
		cast!(self.avx512f._mm512_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `i8x16` to `i32x16`, elementwise.
	#[inline(always)]
	pub fn convert_i8x16_to_i32x16(self, a: i8x16) -> i32x16 {
		cast!(self.avx512f._mm512_cvtepi8_epi32(cast!(a)))
	}

	/// Converts a `i8x16` to `i64x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_i64x8(self, a: i8x16) -> i64x8 {
		cast!(self.avx512f._mm512_cvtepi8_epi64(cast!(a)))
	}

	/// Converts a `i8x16` to `u32x16`, elementwise.
	#[inline(always)]
	pub fn convert_i8x16_to_u32x16(self, a: i8x16) -> u32x16 {
		cast!(self.avx512f._mm512_cvtepi8_epi32(cast!(a)))
	}

	/// Converts a `i8x16` to `u64x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_u64x8(self, a: i8x16) -> u64x8 {
		cast!(self.avx512f._mm512_cvtepi8_epi64(cast!(a)))
	}

	/// Converts a `i8x32` to `i16x32`, elementwise.
	#[inline(always)]
	pub fn convert_i8x32_to_i16x32(self, a: i8x32) -> i16x32 {
		cast!(self.avx512bw._mm512_cvtepi8_epi16(cast!(a)))
	}

	/// Converts a `i8x32` to `u16x32`, elementwise.
	#[inline(always)]
	pub fn convert_i8x32_to_u16x32(self, a: i8x32) -> u16x32 {
		cast!(self.avx512bw._mm512_cvtepi8_epi16(cast!(a)))
	}

	/// Converts a `i8x64` to `u8x64`, elementwise.
	#[inline(always)]
	pub fn convert_i8x64_to_u8x64(self, a: i8x64) -> u8x64 {
		cast!(a)
	}

	/// Converts a `b16` to `u16x16`, broadcasting each bit in `a` to all the bits of the
	/// corresponding lane element in the result.
	#[inline(always)]
	pub fn convert_mask_b16_to_u16x16(self, a: b16) -> u16x16 {
		cast!(self.avx512bw._mm256_movm_epi16(a.0))
	}

	/// Converts a `b16` to `u32x16`, broadcasting each bit in `a` to all the bits of the
	/// corresponding lane element in the result.
	#[inline(always)]
	pub fn convert_mask_b16_to_u32x16(self, a: b16) -> u32x16 {
		self.select_u32x16(a, self.splat_u32x16(!0), self.splat_u32x16(0))
	}

	/// Converts a `b16` to `u8x16`, broadcasting each bit in `a` to all the bits of the
	/// corresponding lane element in the result.
	#[inline(always)]
	pub fn convert_mask_b16_to_u8x16(self, a: b16) -> u8x16 {
		cast!(self.avx512bw._mm_movm_epi8(a.0))
	}

	/// Converts a `b32` to `u16x32`, broadcasting each bit in `a` to all the bits of the
	/// corresponding lane element in the result.
	#[inline(always)]
	pub fn convert_mask_b32_to_u16x32(self, a: b32) -> u16x32 {
		cast!(self.avx512bw._mm512_movm_epi16(a.0))
	}

	/// Converts a `b32` to `u8x32`, broadcasting each bit in `a` to all the bits of the
	/// corresponding lane element in the result.
	#[inline(always)]
	pub fn convert_mask_b32_to_u8x32(self, a: b32) -> u8x32 {
		cast!(self.avx512bw._mm256_movm_epi8(a.0))
	}

	/// Converts a `b64` to `u8x64`, broadcasting each bit in `a` to all the bits of the
	/// corresponding lane element in the result.
	#[inline(always)]
	pub fn convert_mask_b64_to_u8x64(self, a: b64) -> u8x64 {
		cast!(self.avx512bw._mm512_movm_epi8(a.0))
	}

	/// Converts a `b8` to `u16x8`, broadcasting each bit in `a` to all the bits of the
	/// corresponding lane element in the result.
	#[inline(always)]
	pub fn convert_mask_b8_to_u16x8(self, a: b8) -> u16x8 {
		cast!(self.avx512bw._mm_movm_epi16(a.0))
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

	/// Converts a `u16x16` to `i32x16`, elementwise.
	#[inline(always)]
	pub fn convert_u16x16_to_i32x16(self, a: u16x16) -> i32x16 {
		cast!(self.avx512f._mm512_cvtepu16_epi32(cast!(a)))
	}

	/// Converts a `u16x16` to `i8x16`, elementwise.
	#[inline(always)]
	pub fn convert_u16x16_to_i8x16(self, a: u16x16) -> i8x16 {
		cast!(self.avx512bw._mm256_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `u16x16` to `u32x16`, elementwise.
	#[inline(always)]
	pub fn convert_u16x16_to_u32x16(self, a: u16x16) -> u32x16 {
		cast!(self.avx512f._mm512_cvtepu16_epi32(cast!(a)))
	}

	/// Converts a `u16x16` to `u8x16`, elementwise.
	#[inline(always)]
	pub fn convert_u16x16_to_u8x16(self, a: u16x16) -> u8x16 {
		cast!(self.avx512bw._mm256_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `u16x32` to `i16x32`, elementwise.
	#[inline(always)]
	pub fn convert_u16x32_to_i16x32(self, a: u16x32) -> i16x32 {
		cast!(a)
	}

	/// Converts a `u16x32` to `i8x32`, elementwise.
	#[inline(always)]
	pub fn convert_u16x32_to_i8x32(self, a: u16x32) -> i8x32 {
		cast!(self.avx512bw._mm512_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `u16x32` to `u8x32`, elementwise.
	#[inline(always)]
	pub fn convert_u16x32_to_u8x32(self, a: u16x32) -> u8x32 {
		cast!(self.avx512bw._mm512_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `u16x8` to `i64x8`, elementwise.
	#[inline(always)]
	pub fn convert_u16x8_to_i64x8(self, a: u16x8) -> i64x8 {
		cast!(self.avx512f._mm512_cvtepu16_epi64(cast!(a)))
	}

	/// Converts a `u16x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u16x8_to_i8x16(self, a: u16x8) -> i8x16 {
		cast!(self.avx512bw._mm_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `u16x8` to `u64x8`, elementwise.
	#[inline(always)]
	pub fn convert_u16x8_to_u64x8(self, a: u16x8) -> u64x8 {
		cast!(self.avx512f._mm512_cvtepu16_epi64(cast!(a)))
	}

	/// Converts a `u16x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u16x8_to_u8x16(self, a: u16x8) -> u8x16 {
		cast!(self.avx512bw._mm_cvtepi16_epi8(cast!(a)))
	}

	/// Converts a `u32x16` to `i16x16`, elementwise.
	#[inline(always)]
	pub fn convert_u32x16_to_i16x16(self, a: u32x16) -> i16x16 {
		cast!(self.avx512f._mm512_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `u32x16` to `i32x16`, elementwise.
	#[inline(always)]
	pub fn convert_u32x16_to_i32x16(self, a: u32x16) -> i32x16 {
		cast!(a)
	}

	/// Converts a `u32x16` to `i8x16`, elementwise.
	#[inline(always)]
	pub fn convert_u32x16_to_i8x16(self, a: u32x16) -> i8x16 {
		cast!(self.avx512f._mm512_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `u32x16` to `u16x16`, elementwise.
	#[inline(always)]
	pub fn convert_u32x16_to_u16x16(self, a: u32x16) -> u16x16 {
		cast!(self.avx512f._mm512_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `u32x16` to `u8x16`, elementwise.
	#[inline(always)]
	pub fn convert_u32x16_to_u8x16(self, a: u32x16) -> u8x16 {
		cast!(self.avx512f._mm512_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `u32x4` to `i16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u32x4_to_i16x8(self, a: u32x4) -> i16x8 {
		cast!(self.avx512f._mm_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `u32x4` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u32x4_to_i8x16(self, a: u32x4) -> i8x16 {
		cast!(self.avx512f._mm_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `u32x4` to `u16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u32x4_to_u16x8(self, a: u32x4) -> u16x8 {
		cast!(self.avx512f._mm_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `u32x4` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u32x4_to_u8x16(self, a: u32x4) -> u8x16 {
		cast!(self.avx512f._mm_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `u32x8` to `i16x8`, elementwise.
	#[inline(always)]
	pub fn convert_u32x8_to_i16x8(self, a: u32x8) -> i16x8 {
		cast!(self.avx512f._mm256_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `u32x8` to `i64x8`, elementwise.
	#[inline(always)]
	pub fn convert_u32x8_to_i64x8(self, a: u32x8) -> i64x8 {
		cast!(self.avx512f._mm512_cvtepu32_epi64(cast!(a)))
	}

	/// Converts a `u32x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u32x8_to_i8x16(self, a: u32x8) -> i8x16 {
		cast!(self.avx512f._mm256_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `u32x8` to `u16x8`, elementwise.
	#[inline(always)]
	pub fn convert_u32x8_to_u16x8(self, a: u32x8) -> u16x8 {
		cast!(self.avx512f._mm256_cvtepi32_epi16(cast!(a)))
	}

	/// Converts a `u32x8` to `u64x8`, elementwise.
	#[inline(always)]
	pub fn convert_u32x8_to_u64x8(self, a: u32x8) -> u64x8 {
		cast!(self.avx512f._mm512_cvtepu32_epi64(cast!(a)))
	}

	/// Converts a `u32x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u32x8_to_u8x16(self, a: u32x8) -> u8x16 {
		cast!(self.avx512f._mm256_cvtepi32_epi8(cast!(a)))
	}

	/// Converts a `u64x2` to `i16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x2_to_i16x8(self, a: u64x2) -> i16x8 {
		cast!(self.avx512f._mm_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `u64x2` to `i32x4`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x2_to_i32x4(self, a: u64x2) -> i32x4 {
		cast!(self.avx512f._mm_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `u64x2` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x2_to_i8x16(self, a: u64x2) -> i8x16 {
		cast!(self.avx512f._mm_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `u64x2` to `u16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x2_to_u16x8(self, a: u64x2) -> u16x8 {
		cast!(self.avx512f._mm_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `u64x2` to `u32x4`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x2_to_u32x4(self, a: u64x2) -> u32x4 {
		cast!(self.avx512f._mm_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `u64x2` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x2_to_u8x16(self, a: u64x2) -> u8x16 {
		cast!(self.avx512f._mm_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `u64x4` to `i16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x4_to_i16x8(self, a: u64x4) -> i16x8 {
		cast!(self.avx512f._mm256_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `u64x4` to `i32x4`, elementwise.
	#[inline(always)]
	pub fn convert_u64x4_to_i32x4(self, a: u64x4) -> i32x4 {
		cast!(self.avx512f._mm256_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `u64x4` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x4_to_i8x16(self, a: u64x4) -> i8x16 {
		cast!(self.avx512f._mm256_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `u64x4` to `u16x8`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x4_to_u16x8(self, a: u64x4) -> u16x8 {
		cast!(self.avx512f._mm256_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `u64x4` to `u32x4`, elementwise.
	#[inline(always)]
	pub fn convert_u64x4_to_u32x4(self, a: u64x4) -> u32x4 {
		cast!(self.avx512f._mm256_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `u64x4` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x4_to_u8x16(self, a: u64x4) -> u8x16 {
		cast!(self.avx512f._mm256_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `u64x8` to `i16x8`, elementwise.
	#[inline(always)]
	pub fn convert_u64x8_to_i16x8(self, a: u64x8) -> i16x8 {
		cast!(self.avx512f._mm512_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `u64x8` to `i32x8`, elementwise.
	#[inline(always)]
	pub fn convert_u64x8_to_i32x8(self, a: u64x8) -> i32x8 {
		cast!(self.avx512f._mm512_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `u64x8` to `i8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x8_to_i8x16(self, a: u64x8) -> i8x16 {
		cast!(self.avx512f._mm512_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `u64x8` to `u16x8`, elementwise.
	#[inline(always)]
	pub fn convert_u64x8_to_u16x8(self, a: u64x8) -> u16x8 {
		cast!(self.avx512f._mm512_cvtepi64_epi16(cast!(a)))
	}

	/// Converts a `u64x8` to `u32x8`, elementwise.
	#[inline(always)]
	pub fn convert_u64x8_to_u32x8(self, a: u64x8) -> u32x8 {
		cast!(self.avx512f._mm512_cvtepi64_epi32(cast!(a)))
	}

	/// Converts a `u64x8` to `u8x16`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_u64x8_to_u8x16(self, a: u64x8) -> u8x16 {
		cast!(self.avx512f._mm512_cvtepi64_epi8(cast!(a)))
	}

	/// Converts a `u8x16` to `i32x16`, elementwise.
	#[inline(always)]
	pub fn convert_u8x16_to_i32x16(self, a: u8x16) -> i32x16 {
		cast!(self.avx512f._mm512_cvtepu8_epi32(cast!(a)))
	}

	/// Converts a `u8x16` to `i64x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_i64x8(self, a: u8x16) -> i64x8 {
		cast!(self.avx512f._mm512_cvtepu8_epi64(cast!(a)))
	}

	/// Converts a `u8x16` to `u32x16`, elementwise.
	#[inline(always)]
	pub fn convert_u8x16_to_u32x16(self, a: u8x16) -> u32x16 {
		cast!(self.avx512f._mm512_cvtepu8_epi32(cast!(a)))
	}

	/// Converts a `u8x16` to `u64x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_u64x8(self, a: u8x16) -> u64x8 {
		cast!(self.avx512f._mm512_cvtepu8_epi64(cast!(a)))
	}

	/// Converts a `u8x32` to `i16x32`, elementwise.
	#[inline(always)]
	pub fn convert_u8x32_to_i16x32(self, a: u8x32) -> i16x32 {
		cast!(self.avx512bw._mm512_cvtepu8_epi16(cast!(a)))
	}

	/// Converts a `u8x32` to `u16x32`, elementwise.
	#[inline(always)]
	pub fn convert_u8x32_to_u16x32(self, a: u8x32) -> u16x32 {
		cast!(self.avx512bw._mm512_cvtepu8_epi16(cast!(a)))
	}

	/// Converts a `u8x64` to `i8x64`, elementwise.
	#[inline(always)]
	pub fn convert_u8x64_to_i8x64(self, a: u8x64) -> i8x64 {
		cast!(a)
	}

	/// Divides the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn div_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_div_ps(cast!(a), cast!(b)))
	}

	/// Divides the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn div_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_div_pd(cast!(a), cast!(b)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
	#[inline(always)]
	pub fn floor_f32x16(self, a: f32x16) -> f32x16 {
		cast!(
			self.avx512f
				._mm512_roundscale_ps::<_MM_FROUND_TO_NEG_INF>(cast!(a)),
		)
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
	#[inline(always)]
	pub fn floor_f64x8(self, a: f64x8) -> f64x8 {
		cast!(
			self.avx512f
				._mm512_roundscale_pd::<_MM_FROUND_TO_NEG_INF>(cast!(a)),
		)
	}

	/// Checks if the elements in each lane of `a` are NaN.
	#[inline(always)]
	pub fn is_nan_f32x16(self, a: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_UNORD_Q>(cast!(a), cast!(a)),
		)
	}

	/// Checks if the elements in each lane of `a` are NaN.
	#[inline(always)]
	pub fn is_nan_f32x8(self, a: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_UNORD_Q>(cast!(a), cast!(a)),
		)
	}

	/// Checks if the elements in each lane of `a` are NaN.
	#[inline(always)]
	pub fn is_nan_f64x4(self, a: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_UNORD_Q>(cast!(a), cast!(a)),
		)
	}

	/// Checks if the elements in each lane of `a` are NaN.
	#[inline(always)]
	pub fn is_nan_f64x8(self, a: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_UNORD_Q>(cast!(a), cast!(a)),
		)
	}

	/// Checks if the elements in each lane of `a` are not NaN.
	#[inline(always)]
	pub fn is_not_nan_f32x16(self, a: f32x16) -> b16 {
		cast!(
			self.avx512f
				._mm512_cmp_ps_mask::<_CMP_ORD_Q>(cast!(a), cast!(a)),
		)
	}

	/// Checks if the elements in each lane of `a` are not NaN.
	#[inline(always)]
	pub fn is_not_nan_f32x8(self, a: f32x8) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_ps_mask::<_CMP_ORD_Q>(cast!(a), cast!(a)),
		)
	}

	/// Checks if the elements in each lane of `a` are not NaN.
	#[inline(always)]
	pub fn is_not_nan_f64x4(self, a: f64x4) -> b8 {
		cast!(
			self.avx512f
				._mm256_cmp_pd_mask::<_CMP_ORD_Q>(cast!(a), cast!(a)),
		)
	}

	/// Checks if the elements in each lane of `a` are not NaN.
	#[inline(always)]
	pub fn is_not_nan_f64x8(self, a: f64x8) -> b8 {
		cast!(
			self.avx512f
				._mm512_cmp_pd_mask::<_CMP_ORD_Q>(cast!(a), cast!(a)),
		)
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_max_ps(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_max_pd(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512bw._mm512_max_epi16(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_max_epi32(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		cast!(self.avx512f._mm_max_epi64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
		cast!(self.avx512f._mm256_max_epi64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_max_epi64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512bw._mm512_max_epi8(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_max_epu16(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_max_epu32(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		cast!(self.avx512f._mm_max_epu64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
		cast!(self.avx512f._mm256_max_epu64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_max_epu64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise maximum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn max_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512bw._mm512_max_epu8(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_min_ps(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_min_pd(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512bw._mm512_min_epi16(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_min_epi32(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_i64x4(self, a: i64x4, b: i64x4) -> i64x4 {
		cast!(self.avx512f._mm256_min_epi64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_min_epi64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512bw._mm512_min_epi8(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_min_epu16(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_min_epu32(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		cast!(self.avx512f._mm_min_epu64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_u64x4(self, a: u64x4, b: u64x4) -> u64x4 {
		cast!(self.avx512f._mm256_min_epu64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_min_epu64(cast!(a), cast!(b)))
	}

	/// Computes the elementwise minimum of each lane of `a` and `b`.
	#[inline(always)]
	pub fn min_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512bw._mm512_min_epu8(cast!(a), cast!(b)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
	/// `c`.
	#[inline(always)]
	pub fn mul_add_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_fmadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
	/// `c`.
	#[inline(always)]
	pub fn mul_add_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_fmadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_addsub_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_fmaddsub_ps(
			cast!(a),
			cast!(b),
			cast!(self.sub_f32x16(self.splat_f32x16(-0.0), c)),
		))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_addsub_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_fmaddsub_pd(
			cast!(a),
			cast!(b),
			cast!(self.sub_f64x8(self.splat_f64x8(-0.0), c)),
		))
	}

	/// Multiplies the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn mul_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_mul_ps(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn mul_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_mul_pd(cast!(a), cast!(b)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the results.
	#[inline(always)]
	pub fn mul_sub_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_fmsub_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the results.
	#[inline(always)]
	pub fn mul_sub_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_fmsub_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_subadd_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
		cast!(
			self.avx512f
				._mm512_fmaddsub_ps(cast!(a), cast!(b), cast!(c))
		)
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_subadd_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
		cast!(
			self.avx512f
				._mm512_fmaddsub_pd(cast!(a), cast!(b), cast!(c))
		)
	}

	/// See `_mm512_maddubs_epi16`
	#[inline(always)]
	pub fn multiply_saturating_add_adjacent_i8x64(self, a: i8x64, b: i8x64) -> i16x32 {
		cast!(self.avx512bw._mm512_maddubs_epi16(cast!(a), cast!(b)))
	}

	/// See `_mm512_madd_epi16`
	#[inline(always)]
	pub fn multiply_wrapping_add_adjacent_i16x32(self, a: i16x32, b: i16x32) -> i32x16 {
		cast!(self.avx512bw._mm512_madd_epi16(cast!(a), cast!(b)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
	/// each lane of `c`.
	#[inline(always)]
	pub fn negate_mul_add_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_fnmadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
	/// each lane of `c`.
	#[inline(always)]
	pub fn negate_mul_add_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_fnmadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the negation of the results.
	#[inline(always)]
	pub fn negate_mul_sub_f32x16(self, a: f32x16, b: f32x16, c: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_fnmsub_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the negation of the results.
	#[inline(always)]
	pub fn negate_mul_sub_f64x8(self, a: f64x8, b: f64x8, c: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_fnmsub_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_i16x32(self, a: i16x32) -> i16x32 {
		self.xor_i16x32(a, self.splat_i16x32(!0))
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_i32x16(self, a: i32x16) -> i32x16 {
		self.xor_i32x16(a, self.splat_i32x16(!0))
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_i64x8(self, a: i64x8) -> i64x8 {
		self.xor_i64x8(a, self.splat_i64x8(!0))
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
	pub fn not_u32x16(self, a: u32x16) -> u32x16 {
		self.xor_u32x16(a, self.splat_u32x16(!0))
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_u64x8(self, a: u64x8) -> u64x8 {
		self.xor_u64x8(a, self.splat_u64x8(!0))
	}

	/// Returns the bitwise NOT of `a`.
	#[inline(always)]
	pub fn not_u8x64(self, a: u8x64) -> u8x64 {
		self.xor_u8x64(a, self.splat_u8x64(!0))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise OR of `a` and `b`.
	#[inline(always)]
	pub fn or_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512f._mm512_or_si512(cast!(a), cast!(b)))
	}

	/// See `_mm512_packs_epi16`
	#[inline(always)]
	pub fn pack_with_signed_saturation_i16x32(self, a: i16x32, b: i16x32) -> i8x64 {
		cast!(self.avx512bw._mm512_packs_epi16(cast!(a), cast!(b)))
	}

	/// See `_mm512_packs_epi32`
	#[inline(always)]
	pub fn pack_with_signed_saturation_i32x16(self, a: i32x16, b: i32x16) -> i16x32 {
		cast!(self.avx512bw._mm512_packs_epi32(cast!(a), cast!(b)))
	}

	/// See `_mm512_packus_epi16`
	#[inline(always)]
	pub fn pack_with_unsigned_saturation_i16x32(self, a: i16x32, b: i16x32) -> u8x64 {
		cast!(self.avx512bw._mm512_packus_epi16(cast!(a), cast!(b)))
	}

	/// See `_mm512_packus_epi32`
	#[inline(always)]
	pub fn pack_with_unsigned_saturation_i32x16(self, a: i32x16, b: i32x16) -> u16x32 {
		cast!(self.avx512bw._mm512_packus_epi32(cast!(a), cast!(b)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
	/// close, the even value is returned.
	#[inline(always)]
	pub fn round_f32x16(self, a: f32x16) -> f32x16 {
		cast!(
			self.avx512f
				._mm512_roundscale_pd::<_MM_FROUND_TO_NEAREST_INT>(cast!(a)),
		)
	}

	/// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
	/// close, the even value is returned.
	#[inline(always)]
	pub fn round_f64x8(self, a: f64x8) -> f64x8 {
		cast!(
			self.avx512f
				._mm512_roundscale_pd::<_MM_FROUND_TO_NEAREST_INT>(cast!(a)),
		)
	}

	/// Adds the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_add_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512bw._mm512_adds_epi16(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_add_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512bw._mm512_adds_epi8(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_add_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_adds_epu16(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_add_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512bw._mm512_adds_epu8(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_sub_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512bw._mm512_subs_epi16(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_sub_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512bw._mm512_subs_epi8(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_sub_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_subs_epu16(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_sub_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512bw._mm512_subs_epu8(cast!(a), cast!(b)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f32x16(self, mask: b16, if_true: f32x16, if_false: f32x16) -> f32x16 {
		cast!(
			self.avx512f
				._mm512_mask_blend_ps(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f32x4(self, mask: b8, if_true: f32x4, if_false: f32x4) -> f32x4 {
		cast!(
			self.avx512f
				._mm_mask_blend_ps(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f32x8(self, mask: b8, if_true: f32x8, if_false: f32x8) -> f32x8 {
		cast!(
			self.avx512f
				._mm256_mask_blend_ps(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f64x2(self, mask: b8, if_true: f64x2, if_false: f64x2) -> f64x2 {
		cast!(
			self.avx512f
				._mm_mask_blend_pd(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f64x4(self, mask: b8, if_true: f64x4, if_false: f64x4) -> f64x4 {
		cast!(
			self.avx512f
				._mm256_mask_blend_pd(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f64x8(self, mask: b8, if_true: f64x8, if_false: f64x8) -> f64x8 {
		cast!(
			self.avx512f
				._mm512_mask_blend_pd(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i16x16(self, mask: b16, if_true: i16x16, if_false: i16x16) -> i16x16 {
		cast!(
			self.avx512bw
				._mm256_mask_blend_epi16(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i16x32(self, mask: b32, if_true: i16x32, if_false: i16x32) -> i16x32 {
		cast!(
			self.avx512bw
				._mm512_mask_blend_epi16(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i16x8(self, mask: b8, if_true: i16x8, if_false: i16x8) -> i16x8 {
		cast!(
			self.avx512bw
				._mm_mask_blend_epi16(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i32x16(self, mask: b16, if_true: i32x16, if_false: i32x16) -> i32x16 {
		cast!(
			self.avx512f
				._mm512_mask_blend_epi32(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i32x4(self, mask: b8, if_true: i32x4, if_false: i32x4) -> i32x4 {
		cast!(
			self.avx512f
				._mm_mask_blend_epi32(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i32x8(self, mask: b8, if_true: i32x8, if_false: i32x8) -> i32x8 {
		cast!(
			self.avx512f
				._mm256_mask_blend_epi32(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i64x2(self, mask: b8, if_true: i64x2, if_false: i64x2) -> i64x2 {
		cast!(
			self.avx512f
				._mm_mask_blend_epi64(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i64x4(self, mask: b8, if_true: i64x4, if_false: i64x4) -> i64x4 {
		cast!(
			self.avx512f
				._mm256_mask_blend_epi64(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i64x8(self, mask: b8, if_true: i64x8, if_false: i64x8) -> i64x8 {
		cast!(
			self.avx512f
				._mm512_mask_blend_epi64(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i8x16(self, mask: b16, if_true: i8x16, if_false: i8x16) -> i8x16 {
		cast!(
			self.avx512bw
				._mm_mask_blend_epi8(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i8x32(self, mask: b32, if_true: i8x32, if_false: i8x32) -> i8x32 {
		cast!(
			self.avx512bw
				._mm256_mask_blend_epi8(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i8x64(self, mask: b64, if_true: i8x64, if_false: i8x64) -> i8x64 {
		cast!(
			self.avx512bw
				._mm512_mask_blend_epi8(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u16x16(self, mask: b16, if_true: u16x16, if_false: u16x16) -> u16x16 {
		cast!(
			self.avx512bw
				._mm256_mask_blend_epi16(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u16x32(self, mask: b32, if_true: u16x32, if_false: u16x32) -> u16x32 {
		cast!(
			self.avx512bw
				._mm512_mask_blend_epi16(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u16x8(self, mask: b8, if_true: u16x8, if_false: u16x8) -> u16x8 {
		cast!(
			self.avx512bw
				._mm_mask_blend_epi16(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u32x16(self, mask: b16, if_true: u32x16, if_false: u32x16) -> u32x16 {
		cast!(
			self.avx512f
				._mm512_mask_blend_epi32(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u32x4(self, mask: b8, if_true: u32x4, if_false: u32x4) -> u32x4 {
		cast!(
			self.avx512f
				._mm_mask_blend_epi32(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u32x8(self, mask: b8, if_true: u32x8, if_false: u32x8) -> u32x8 {
		cast!(
			self.avx512f
				._mm256_mask_blend_epi32(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u64x2(self, mask: b8, if_true: u64x2, if_false: u64x2) -> u64x2 {
		cast!(
			self.avx512f
				._mm_mask_blend_epi64(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u64x4(self, mask: b8, if_true: u64x4, if_false: u64x4) -> u64x4 {
		cast!(
			self.avx512f
				._mm256_mask_blend_epi64(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u64x8(self, mask: b8, if_true: u64x8, if_false: u64x8) -> u64x8 {
		cast!(
			self.avx512f
				._mm512_mask_blend_epi64(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u8x16(self, mask: b16, if_true: u8x16, if_false: u8x16) -> u8x16 {
		cast!(
			self.avx512bw
				._mm_mask_blend_epi8(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u8x32(self, mask: b32, if_true: u8x32, if_false: u8x32) -> u8x32 {
		cast!(
			self.avx512bw
				._mm256_mask_blend_epi8(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u8x64(self, mask: b64, if_true: u8x64, if_false: u8x64) -> u8x64 {
		cast!(
			self.avx512bw
				._mm512_mask_blend_epi8(mask.0, cast!(if_false), cast!(if_true)),
		)
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_i16x32<const AMOUNT: u32>(self, a: i16x32) -> i16x32 {
		cast!(self.avx512bw._mm512_slli_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_i32x16<const AMOUNT: u32>(self, a: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_slli_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_i64x8<const AMOUNT: u32>(self, a: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_slli_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_u16x32<const AMOUNT: u32>(self, a: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_slli_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_u32x16<const AMOUNT: u32>(self, a: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_slli_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_u64x8<const AMOUNT: u32>(self, a: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_slli_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_i32x16(self, a: i32x16, amount: u32x16) -> i32x16 {
		cast!(self.avx512f._mm512_sllv_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_i64x8(self, a: i64x8, amount: u64x8) -> i64x8 {
		cast!(self.avx512f._mm512_sllv_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_u32x16(self, a: u32x16, amount: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_sllv_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_u64x8(self, a: u64x8, amount: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_sllv_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_i16x32(self, a: i16x32, amount: u64x2) -> i16x32 {
		cast!(self.avx512bw._mm512_sll_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_i32x16(self, a: i32x16, amount: u64x2) -> i32x16 {
		cast!(self.avx512f._mm512_sll_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_i64x8(self, a: i64x8, amount: u64x2) -> i64x8 {
		cast!(self.avx512f._mm512_sll_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_u16x32(self, a: u16x32, amount: u64x2) -> u16x32 {
		cast!(self.avx512bw._mm512_sll_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_u32x16(self, a: u32x16, amount: u64x2) -> u32x16 {
		cast!(self.avx512f._mm512_sll_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_u64x8(self, a: u64x8, amount: u64x2) -> u64x8 {
		cast!(self.avx512f._mm512_sll_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_const_i16x32<const AMOUNT: u32>(self, a: i16x32) -> i16x32 {
		cast!(self.avx512bw._mm512_srai_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_const_i32x16<const AMOUNT: u32>(self, a: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_srai_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_const_i64x2<const AMOUNT: u32>(self, a: i64x2) -> i64x2 {
		cast!(self.avx512f._mm_srai_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_const_i64x4<const AMOUNT: u32>(self, a: i64x4) -> i64x4 {
		cast!(self.avx512f._mm256_srai_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_const_i64x8<const AMOUNT: u32>(self, a: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_srai_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_const_u16x32<const AMOUNT: u32>(self, a: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_srli_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_const_u32x16<const AMOUNT: u32>(self, a: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_srli_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_const_u64x8<const AMOUNT: u32>(self, a: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_srli_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in sign bits.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_dyn_i32x16(self, a: i32x16, amount: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_srav_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in sign bits.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_dyn_i64x2(self, a: i64x2, amount: u64x2) -> i64x2 {
		cast!(self.avx512f._mm_srav_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in sign bits.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_dyn_i64x4(self, a: i64x4, amount: u64x4) -> i64x4 {
		cast!(self.avx512f._mm256_srav_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in sign bits.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_dyn_i64x8(self, a: i64x8, amount: u64x8) -> i64x8 {
		cast!(self.avx512f._mm512_srav_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_dyn_u32x16(self, a: u32x16, amount: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_srlv_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_dyn_u64x8(self, a: u64x8, amount: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_srlv_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_i16x32(self, a: i16x32, amount: u64x2) -> i16x32 {
		cast!(self.avx512bw._mm512_sra_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_i32x16(self, a: i32x16, amount: u64x2) -> i32x16 {
		cast!(self.avx512f._mm512_sra_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_i64x2(self, a: i64x2, amount: u64x2) -> i64x2 {
		cast!(self.avx512f._mm_sra_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_i64x4(self, a: i64x4, amount: u64x2) -> i64x4 {
		cast!(self.avx512f._mm256_sra_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_i64x8(self, a: i64x8, amount: u64x2) -> i64x8 {
		cast!(self.avx512f._mm512_sra_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_u16x32(self, a: u16x32, amount: u64x2) -> u16x32 {
		cast!(self.avx512bw._mm512_srl_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_u32x16(self, a: u32x16, amount: u64x2) -> u32x16 {
		cast!(self.avx512f._mm512_srl_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.  
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_u64x8(self, a: u64x8, amount: u64x2) -> u64x8 {
		cast!(self.avx512f._mm512_srl_epi64(cast!(a), cast!(amount)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_f32x16(self, value: f32) -> f32x16 {
		cast!(self.avx512f._mm512_set1_ps(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_f64x8(self, value: f64) -> f64x8 {
		cast!(self.avx512f._mm512_set1_pd(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i16x32(self, value: i16) -> i16x32 {
		cast!(self.avx512f._mm512_set1_epi16(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i32x16(self, value: i32) -> i32x16 {
		cast!(self.avx512f._mm512_set1_epi32(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i64x8(self, value: i64) -> i64x8 {
		cast!(self.avx512f._mm512_set1_epi64(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i8x64(self, value: i8) -> i8x64 {
		cast!(self.avx512f._mm512_set1_epi8(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u16x32(self, value: u16) -> u16x32 {
		cast!(self.avx512f._mm512_set1_epi16(value as i16))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u32x16(self, value: u32) -> u32x16 {
		cast!(self.avx512f._mm512_set1_epi32(value as i32))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u64x8(self, value: u64) -> u64x8 {
		cast!(self.avx512f._mm512_set1_epi64(value as i64))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u8x64(self, value: u8) -> u8x64 {
		cast!(self.avx512f._mm512_set1_epi8(value as i8))
	}

	/// Computes the square roots of the elements of each lane of `a`.
	#[inline(always)]
	pub fn sqrt_f32x16(self, a: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_sqrt_ps(cast!(a)))
	}

	/// Computes the square roots of the elements of each lane of `a`.
	#[inline(always)]
	pub fn sqrt_f64x8(self, a: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_sqrt_pd(cast!(a)))
	}

	/// Subtracts the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn sub_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_sub_ps(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn sub_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_sub_pd(cast!(a), cast!(b)))
	}

	/// See `_mm512_sad_epu8`
	#[inline(always)]
	pub fn sum_of_absolute_differences_u8x64(self, a: u8x64, b: u8x64) -> u64x8 {
		cast!(self.avx512bw._mm512_sad_epu8(cast!(a), cast!(b)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards zero.
	#[inline(always)]
	pub fn truncate_f32x16(self, a: f32x16) -> f32x16 {
		cast!(
			self.avx512f
				._mm512_roundscale_pd::<_MM_FROUND_TO_ZERO>(cast!(a)),
		)
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards zero.
	#[inline(always)]
	pub fn truncate_f64x8(self, a: f64x8) -> f64x8 {
		cast!(
			self.avx512f
				._mm512_roundscale_pd::<_MM_FROUND_TO_ZERO>(cast!(a)),
		)
	}

	/// Computes the unsigned absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn unsigned_abs_i16x32(self, a: i16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_abs_epi16(cast!(a)))
	}

	/// Computes the unsigned absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn unsigned_abs_i32x16(self, a: i32x16) -> u32x16 {
		cast!(self.avx512f._mm512_abs_epi32(cast!(a)))
	}

	/// Computes the unsigned absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn unsigned_abs_i64x4(self, a: i64x4) -> u64x4 {
		cast!(self.avx512f._mm256_abs_epi64(cast!(a)))
	}

	/// Computes the unsigned absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn unsigned_abs_i64x8(self, a: i64x8) -> u64x8 {
		cast!(self.avx512f._mm512_abs_epi64(cast!(a)))
	}

	/// Computes the unsigned absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn unsigned_abs_i8x64(self, a: i8x64) -> u8x64 {
		cast!(self.avx512bw._mm512_abs_epi8(cast!(a)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_i16x32(self, a: i16x32, b: i16x32) -> (i16x32, i16x32) {
		(
			cast!(self.avx512bw._mm512_mullo_epi16(cast!(a), cast!(b))),
			cast!(self.avx512bw._mm512_mulhi_epi16(cast!(a), cast!(b))),
		)
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_i32x16(self, a: i32x16, b: i32x16) -> (i32x16, i32x16) {
		let a = cast!(a);
		let b = cast!(b);
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
			ab_evens,
			// xxxxxxx a1b1_lo xxxxxxx a3b3_lo
			avx512f._mm512_slli_epi64::<32>(ab_odds),
		);
		let ab_hi = self.avx512f._mm512_mask_blend_epi32(
			0b1010101010101010,
			// a0b0_hi xxxxxxx a2b2_hi xxxxxxx
			avx512f._mm512_srli_epi64::<32>(ab_evens),
			// xxxxxxx a1b1_hi xxxxxxx a3b3_hi
			ab_odds,
		);

		(cast!(ab_lo), cast!(ab_hi))
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_u16x32(self, a: u16x32, b: u16x32) -> (u16x32, u16x32) {
		(
			cast!(self.avx512bw._mm512_mullo_epi16(cast!(a), cast!(b))),
			cast!(self.avx512bw._mm512_mulhi_epu16(cast!(a), cast!(b))),
		)
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_u32x16(self, a: u32x16, b: u32x16) -> (u32x16, u32x16) {
		let a = cast!(a);
		let b = cast!(b);
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
			ab_evens,
			// xxxxxxx a1b1_lo xxxxxxx a3b3_lo
			avx512f._mm512_slli_epi64::<32>(ab_odds),
		);
		let ab_hi = self.avx512f._mm512_mask_blend_epi32(
			0b1010101010101010,
			// a0b0_hi xxxxxxx a2b2_hi xxxxxxx
			avx512f._mm512_srli_epi64::<32>(ab_evens),
			// xxxxxxx a1b1_hi xxxxxxx a3b3_hi
			ab_odds,
		);

		(cast!(ab_lo), cast!(ab_hi))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512bw._mm512_add_epi16(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_add_epi32(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_add_epi64(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512bw._mm512_add_epi8(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_add_epi16(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_add_epi32(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_add_epi64(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512bw._mm512_add_epi8(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512bw._mm512_mullo_epi16(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_mullo_epi32(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_mullox_epi64(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_mullo_epi16(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_mullo_epi32(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_mullox_epi64(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512bw._mm512_sub_epi16(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_sub_epi32(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_sub_epi64(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512bw._mm512_sub_epi8(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512bw._mm512_sub_epi16(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_sub_epi32(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_sub_epi64(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512bw._mm512_sub_epi8(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_f32x16(self, a: f32x16, b: f32x16) -> f32x16 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_f64x8(self, a: f64x8, b: f64x8) -> f64x8 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_i16x32(self, a: i16x32, b: i16x32) -> i16x32 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_i32x16(self, a: i32x16, b: i32x16) -> i32x16 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_i64x8(self, a: i64x8, b: i64x8) -> i64x8 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_i8x64(self, a: i8x64, b: i8x64) -> i8x64 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_u16x32(self, a: u16x32, b: u16x32) -> u16x32 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_u32x16(self, a: u32x16, b: u32x16) -> u32x16 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_u64x8(self, a: u64x8, b: u64x8) -> u64x8 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}

	/// Returns the bitwise XOR of `a` and `b`.
	#[inline(always)]
	pub fn xor_u8x64(self, a: u8x64, b: u8x64) -> u8x64 {
		cast!(self.avx512f._mm512_xor_si512(cast!(a), cast!(b)))
	}
}

#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512vl")]
#[inline]
unsafe fn avx512_ld_u32s(ptr: *const u32, f: unsafe extern "C" fn()) -> u32x16 {
	let ret: __m512;
	#[cfg(target_arch = "x86_64")]
	core::arch::asm! {
		"lea rcx, [rip + 2f]",
		"jmp {f}",
		"2:",
		f = in(reg) f,
		in("rax") ptr,
		out("rcx") _,
		out("zmm0") ret,
		out("zmm1") _,
	};
	#[cfg(target_arch = "x86")]
	core::arch::asm! {
		"lea ecx, [eip + 2f]",
		"jmp {f}",
		"2:",
		f = in(reg) f,
		in("eax") ptr,
		out("ecx") _,
		out("zmm0") ret,
		out("zmm1") _,
	};

	cast!(ret)
}

#[target_feature(enable = "avx512f")]
#[target_feature(enable = "avx512vl")]
#[inline]
unsafe fn avx512_st_u32s(ptr: *mut u32, value: u32x16, f: unsafe extern "C" fn()) {
	#[cfg(target_arch = "x86_64")]
	core::arch::asm! {
		"lea rcx, [rip + 2f]",
		"jmp {f}",
		"2:",
		f = in(reg) f,

		in("rax") ptr,
		out("rcx") _,
		inout("zmm0") cast::<_, __m512>(value) => _,
		out("zmm1") _,
	};
	#[cfg(target_arch = "x86")]
	core::arch::asm! {
		"lea ecx, [eip + 2f]",
		"jmp {f}",
		"2:",
		f = in(reg) f,

		in("eax") ptr,
		out("ecx") _,
		inout("zmm0") cast::<_, __m512>(value) => _,
		out("zmm1") _,
	};
}
