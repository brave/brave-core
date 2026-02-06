use super::*;

// https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels
simd_type!({
	/// SSE instruction set.
	#[allow(missing_docs)]
	pub struct V2 {
		pub sse: f!("sse"),
		pub sse2: f!("sse2"),
		pub fxsr: f!("fxsr"),
		pub sse3: f!("sse3"),
		pub ssse3: f!("ssse3"),
		pub sse4_1: f!("sse4.1"),
		pub sse4_2: f!("sse4.2"),
		pub popcnt: f!("popcnt"),
	}
});

impl Seal for V2 {}

impl Deref for V2 {
	type Target = V1;

	#[inline(always)]
	fn deref(&self) -> &Self::Target {
		V1 {
			sse: self.sse,
			sse2: self.sse2,
			fxsr: self.fxsr,
		}
		.to_ref()
	}
}

impl V2 {
	binop_128!(ssse3: sign, r#"Applies the sign of each element of `sign` to the corresponding lane in `a`.
- If `sign` is zero, the corresponding element is zeroed.
- If `sign` is positive, the corresponding element is returned as is.
- If `sign` is negative, the corresponding element is negated."#, apply_sign, i8 x 16, i16 x 8, i32 x 4);

	unop_128!(sse4_1: ceil, "Returns `ceil(a)` for each lane of `a`, rounding towards positive infinity.", f32 x 4, f64 x 2);

	binop_128_nosign!(sse4_1: cmpeq, "Compares the elements in each lane of `a` and `b` for equality.", cmp_eq, m64 x 2 => m64, u64 x 2 => m64, i64 x 2 => m64);

	binop_128!(sse4_2: cmpgt, "Compares the elements in each lane of `a` and `b` for equality.", cmp_gt, i64 x 2 => m64);

	unop_128!(sse4_1: floor, "Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.", f32 x 4, f64 x 2);

	binop_128_nosign!(sse3: hadd, "[_mm_hadd_ps](core::arch::x86_64::_mm_hadd_ps)", horizontal_add_pack, f32 x 4, f64 x 2);

	binop_128_nosign!(ssse3: hadd, "[_mm_hadd_ps](core::arch::x86_64::_mm_hadd_ps)", horizontal_add_pack, u16 x 8, i16 x 8, u32 x 4, i32 x 4);

	binop_128_nosign!(sse3: hsub, "[_mm_hsub_ps](core::arch::x86_64::_mm_hsub_ps)", horizontal_sub_pack, f32 x 4, f64 x 2);

	binop_128_nosign!(ssse3: hsub, "[_mm_hsub_ps](core::arch::x86_64::_mm_hsub_ps)", horizontal_sub_pack, u16 x 8, i16 x 8, u32 x 4, i32 x 4);

	binop_128!(sse4_1: max, "Computes `max(a, b)`. for each lane in `a` and `b`.", i8 x 16, u16 x 8, u32 x 4, i32 x 4);

	binop_128!(sse4_1: min, "Computes `max(a, b)`. for each lane in `a` and `b`.", i8 x 16, u16 x 8, u32 x 4, i32 x 4);

	binop_128_nosign!(sse4_1: mullo, "Computes `a * b` for each lane in `a` and `b`, with wrapping overflow.", wrapping_mul, u32 x 4, i32 x 4);

	binop_128!(sse3: addsub, "Alternatively subtracts and adds the elements of each lane of `a` and `b`.", subadd, f32 x 4, f64 x 2);

	unop_128!(ssse3: abs, "Computes the unsigned absolute value of the elements of each lane of `a`.", unsigned_abs, i8 x 16, i16 x 8, i32 x 4);

	/// Computes `abs(a)` for each lane of `a`.
	#[inline(always)]
	pub fn abs_f32x4(self, a: f32x4) -> f32x4 {
		self.and_f32x4(a, cast!(self.splat_u32x4((1 << 31) - 1)))
	}

	/// Computes `abs(a)` for each lane of `a`.
	#[inline(always)]
	pub fn abs_f64x2(self, a: f64x2) -> f64x2 {
		self.and_f64x2(a, cast!(self.splat_u64x2((1 << 63) - 1)))
	}

	/// Computes the approximate reciprocal of the elements of each lane of `a`.
	#[inline(always)]
	pub fn approx_reciprocal_f32x4(self, a: f32x4) -> f32x4 {
		cast!(self.sse._mm_rcp_ps(cast!(a)))
	}

	/// Computes the approximate reciprocal of the square roots of the elements of each lane of `a`.
	#[inline(always)]
	pub fn approx_reciprocal_sqrt_f32x4(self, a: f32x4) -> f32x4 {
		cast!(self.sse._mm_rsqrt_ps(cast!(a)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
		self.not_m64x2(self.cmp_lt_i64x2(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
		self.not_m64x2(self.cmp_lt_u64x2(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
		let k = self.splat_u64x2(0x8000000000000000);
		self.cmp_gt_i64x2(cast!(self.xor_u64x2(a, k)), cast!(self.xor_u64x2(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
		self.not_m64x2(self.cmp_gt_i64x2(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
		self.not_m64x2(self.cmp_gt_u64x2(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
		cast!(self.sse4_2._mm_cmpgt_epi64(cast!(b), cast!(a)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
		let k = self.splat_u64x2(0x8000000000000000);
		self.cmp_lt_i64x2(cast!(self.xor_u64x2(a, k)), cast!(self.xor_u64x2(b, k)))
	}

	/// Converts a `i16x8` to `i32x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i16x8_to_i32x4(self, a: i16x8) -> i32x4 {
		cast!(self.sse4_1._mm_cvtepi16_epi32(cast!(a)))
	}

	/// Converts a `i16x8` to `i64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i16x8_to_i64x2(self, a: i16x8) -> i64x2 {
		cast!(self.sse4_1._mm_cvtepi16_epi64(cast!(a)))
	}

	/// Converts a `i16x8` to `u32x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i16x8_to_u32x4(self, a: i16x8) -> u32x4 {
		cast!(self.sse4_1._mm_cvtepi16_epi32(cast!(a)))
	}

	/// Converts a `i16x8` to `u64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i16x8_to_u64x2(self, a: i16x8) -> u64x2 {
		cast!(self.sse4_1._mm_cvtepi16_epi64(cast!(a)))
	}

	/// Converts a `i32x4` to `i64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i32x4_to_i64x2(self, a: i32x4) -> i64x2 {
		cast!(self.sse4_1._mm_cvtepi32_epi64(cast!(a)))
	}

	/// Converts a `i32x4` to `u64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i32x4_to_u64x2(self, a: i32x4) -> u64x2 {
		cast!(self.sse4_1._mm_cvtepi32_epi64(cast!(a)))
	}

	/// Converts a `i8x16` to `i16x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_i16x8(self, a: i8x16) -> i16x8 {
		cast!(self.sse4_1._mm_cvtepi8_epi16(cast!(a)))
	}

	/// Converts a `i8x16` to `i32x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_i32x4(self, a: i8x16) -> i32x4 {
		cast!(self.sse4_1._mm_cvtepi8_epi32(cast!(a)))
	}

	/// Converts a `i8x16` to `i64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_i64x2(self, a: i8x16) -> i64x2 {
		cast!(self.sse4_1._mm_cvtepi8_epi64(cast!(a)))
	}

	/// Converts a `i8x16` to `u16x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_u16x8(self, a: i8x16) -> u16x8 {
		cast!(self.sse4_1._mm_cvtepi8_epi16(cast!(a)))
	}

	/// Converts a `i8x16` to `u32x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_u32x4(self, a: i8x16) -> u32x4 {
		cast!(self.sse4_1._mm_cvtepi8_epi32(cast!(a)))
	}

	/// Converts a `i8x16` to `u64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_u64x2(self, a: i8x16) -> u64x2 {
		cast!(self.sse4_1._mm_cvtepi8_epi64(cast!(a)))
	}

	/// Converts a `u16x8` to `i32x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u16x8_to_i32x4(self, a: u16x8) -> i32x4 {
		cast!(self.sse4_1._mm_cvtepu16_epi32(cast!(a)))
	}

	/// Converts a `u16x8` to `i64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u16x8_to_i64x2(self, a: u16x8) -> i64x2 {
		cast!(self.sse4_1._mm_cvtepu16_epi64(cast!(a)))
	}

	/// Converts a `u16x8` to `u32x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u16x8_to_u32x4(self, a: u16x8) -> u32x4 {
		cast!(self.sse4_1._mm_cvtepu16_epi32(cast!(a)))
	}

	/// Converts a `u16x8` to `u64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u16x8_to_u64x2(self, a: u16x8) -> u64x2 {
		cast!(self.sse4_1._mm_cvtepu16_epi64(cast!(a)))
	}

	/// Converts a `u32x4` to `i64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u32x4_to_i64x2(self, a: u32x4) -> i64x2 {
		cast!(self.sse4_1._mm_cvtepu32_epi64(cast!(a)))
	}

	/// Converts a `u32x4` to `u64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u32x4_to_u64x2(self, a: u32x4) -> u64x2 {
		cast!(self.sse4_1._mm_cvtepu32_epi64(cast!(a)))
	}

	/// Converts a `u8x16` to `i16x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_i16x8(self, a: u8x16) -> i16x8 {
		cast!(self.sse4_1._mm_cvtepu8_epi16(cast!(a)))
	}

	/// Converts a `u8x16` to `i32x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_i32x4(self, a: u8x16) -> i32x4 {
		cast!(self.sse4_1._mm_cvtepu8_epi32(cast!(a)))
	}

	/// Converts a `u8x16` to `i64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_i64x2(self, a: u8x16) -> i64x2 {
		cast!(self.sse4_1._mm_cvtepu8_epi64(cast!(a)))
	}

	/// Converts a `u8x16` to `i8x16`, elementwise.
	#[inline(always)]
	pub fn convert_u8x16_to_i8x16(self, a: u8x16) -> i8x16 {
		cast!(a)
	}

	/// Converts a `u8x16` to `u16x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_u16x8(self, a: u8x16) -> u16x8 {
		cast!(self.sse4_1._mm_cvtepu8_epi16(cast!(a)))
	}

	/// Converts a `u8x16` to `u32x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_u32x4(self, a: u8x16) -> u32x4 {
		cast!(self.sse4_1._mm_cvtepu8_epi32(cast!(a)))
	}

	/// Converts a `u8x16` to `u64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_u64x2(self, a: u8x16) -> u64x2 {
		cast!(self.sse4_1._mm_cvtepu8_epi64(cast!(a)))
	}

	/// See [_mm_hadds_epi16].
	///
	/// [_mm_hadds_epi16]: core::arch::x86_64::_mm_hadds_epi16
	#[inline(always)]
	pub fn horizontal_saturating_add_pack_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.ssse3._mm_hadds_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm_hsubs_epi16].
	///
	/// [_mm_hsubs_epi16]: core::arch::x86_64::_mm_hsubs_epi16
	#[inline(always)]
	pub fn horizontal_saturating_sub_pack_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.ssse3._mm_hsubs_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm_maddubs_epi16].
	///
	/// [_mm_maddubs_epi16]: core::arch::x86_64::_mm_maddubs_epi16
	#[inline(always)]
	pub fn multiply_saturating_add_adjacent_i8x16(self, a: i8x16, b: i8x16) -> i16x8 {
		cast!(self.ssse3._mm_maddubs_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm_mpsadbw_epu8].
	///
	/// [_mm_mpsadbw_epu8]: core::arch::x86_64::_mm_mpsadbw_epu8
	#[inline(always)]
	pub fn multisum_of_absolute_differences_u8x16<const OFFSETS: i32>(
		self,
		a: u8x16,
		b: u8x16,
	) -> u16x8 {
		cast!(self.sse4_1._mm_mpsadbw_epu8::<OFFSETS>(cast!(a), cast!(b)))
	}

	/// See [_mm_packus_epi32].
	///
	/// [_mm_packus_epi32]: core::arch::x86_64::_mm_packus_epi32
	#[inline(always)]
	pub fn pack_with_unsigned_saturation_i32x4(self, a: i32x4, b: i32x4) -> u16x8 {
		cast!(self.sse4_1._mm_packus_epi32(cast!(a), cast!(b)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
	/// close, the even value is returned.
	#[inline(always)]
	pub fn round_f32x4(self, a: f32x4) -> f32x4 {
		const ROUNDING: i32 = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC;
		cast!(self.sse4_1._mm_round_ps::<ROUNDING>(cast!(a)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
	/// close, the even value is returned.
	#[inline(always)]
	pub fn round_f64x2(self, a: f64x2) -> f64x2 {
		const ROUNDING: i32 = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC;
		cast!(self.sse4_1._mm_round_pd::<ROUNDING>(cast!(a)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_f32x4<const MASK4: i32>(self, if_true: f32x4, if_false: f32x4) -> f32x4 {
		cast!(self.select_const_u32x4::<MASK4>(cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_f64x2<const MASK2: i32>(self, if_true: f64x2, if_false: f64x2) -> f64x2 {
		cast!(self.select_const_u64x2::<MASK2>(cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_i32x4<const MASK4: i32>(self, if_true: i32x4, if_false: i32x4) -> i32x4 {
		cast!(self.select_const_u32x4::<MASK4>(cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_i64x2<const MASK2: i32>(self, if_true: i64x2, if_false: i64x2) -> i64x2 {
		cast!(self.select_const_u64x2::<MASK2>(cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_u32x4<const MASK4: i32>(self, if_true: u32x4, if_false: u32x4) -> u32x4 {
		cast!(
			self.sse4_1
				._mm_blend_ps::<MASK4>(cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_u64x2<const MASK2: i32>(self, if_true: u64x2, if_false: u64x2) -> u64x2 {
		cast!(
			self.sse4_1
				._mm_blend_pd::<MASK2>(cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f32x4(self, mask: m32x4, if_true: f32x4, if_false: f32x4) -> f32x4 {
		cast!(
			self.sse4_1
				._mm_blendv_ps(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f64x2(self, mask: m64x2, if_true: f64x2, if_false: f64x2) -> f64x2 {
		cast!(
			self.sse4_1
				._mm_blendv_pd(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i16x8(self, mask: m16x8, if_true: i16x8, if_false: i16x8) -> i16x8 {
		cast!(self.select_u16x8(mask, cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i32x4(self, mask: m32x4, if_true: i32x4, if_false: i32x4) -> i32x4 {
		cast!(self.select_u32x4(mask, cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i64x2(self, mask: m64x2, if_true: i64x2, if_false: i64x2) -> i64x2 {
		cast!(self.select_u64x2(mask, cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i8x16(self, mask: m8x16, if_true: i8x16, if_false: i8x16) -> i8x16 {
		cast!(self.select_u8x16(mask, cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u16x8(self, mask: m16x8, if_true: u16x8, if_false: u16x8) -> u16x8 {
		cast!(
			self.sse4_1
				._mm_blendv_epi8(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u32x4(self, mask: m32x4, if_true: u32x4, if_false: u32x4) -> u32x4 {
		cast!(
			self.sse4_1
				._mm_blendv_epi8(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u64x2(self, mask: m64x2, if_true: u64x2, if_false: u64x2) -> u64x2 {
		cast!(
			self.sse4_1
				._mm_blendv_epi8(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u8x16(self, mask: m8x16, if_true: u8x16, if_false: u8x16) -> u8x16 {
		cast!(
			self.sse4_1
				._mm_blendv_epi8(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards zero.
	#[inline(always)]
	pub fn truncate_f32x4(self, a: f32x4) -> f32x4 {
		const ROUNDING: i32 = _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC;
		cast!(self.sse4_1._mm_round_ps::<ROUNDING>(cast!(a)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards zero.
	#[inline(always)]
	pub fn truncate_f64x2(self, a: f64x2) -> f64x2 {
		const ROUNDING: i32 = _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC;
		cast!(self.sse4_1._mm_round_pd::<ROUNDING>(cast!(a)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_i32x4(self, a: i32x4, b: i32x4) -> (u32x4, i32x4) {
		let a = cast!(a);
		let b = cast!(b);
		let sse = self.sse2;

		// a0b0_lo a0b0_hi a2b2_lo a2b2_hi
		let ab_evens = self.sse4_1._mm_mul_epi32(a, b);
		// a1b1_lo a1b1_hi a3b3_lo a3b3_hi
		let ab_odds = self
			.sse4_1
			._mm_mul_epi32(sse._mm_srli_epi64::<32>(a), sse._mm_srli_epi64::<32>(b));

		let ab_lo = self.sse4_1._mm_blend_ps::<0b1010>(
			// a0b0_lo xxxxxxx a2b2_lo xxxxxxx
			cast!(ab_evens),
			// xxxxxxx a1b1_lo xxxxxxx a3b3_lo
			cast!(sse._mm_slli_epi64::<32>(ab_odds)),
		);
		let ab_hi = self.sse4_1._mm_blend_ps::<0b1010>(
			// a0b0_hi xxxxxxx a2b2_hi xxxxxxx
			cast!(sse._mm_srli_epi64::<32>(ab_evens)),
			// xxxxxxx a1b1_hi xxxxxxx a3b3_hi
			cast!(ab_odds),
		);

		(cast!(ab_lo), cast!(ab_hi))
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_u32x4(self, a: u32x4, b: u32x4) -> (u32x4, u32x4) {
		let a = cast!(a);
		let b = cast!(b);
		let sse = self.sse2;

		// a0b0_lo a0b0_hi a2b2_lo a2b2_hi
		let ab_evens = sse._mm_mul_epu32(a, b);
		// a1b1_lo a1b1_hi a3b3_lo a3b3_hi
		let ab_odds = sse._mm_mul_epu32(sse._mm_srli_epi64::<32>(a), sse._mm_srli_epi64::<32>(b));

		let ab_lo = self.sse4_1._mm_blend_ps::<0b1010>(
			// a0b0_lo xxxxxxx a2b2_lo xxxxxxx
			cast!(ab_evens),
			// xxxxxxx a1b1_lo xxxxxxx a3b3_lo
			cast!(sse._mm_slli_epi64::<32>(ab_odds)),
		);
		let ab_hi = self.sse4_1._mm_blend_ps::<0b1010>(
			// a0b0_hi xxxxxxx a2b2_hi xxxxxxx
			cast!(sse._mm_srli_epi64::<32>(ab_evens)),
			// xxxxxxx a1b1_hi xxxxxxx a3b3_hi
			cast!(ab_odds),
		);

		(cast!(ab_lo), cast!(ab_hi))
	}
}

macro_rules! impl_simd_binop {
	($func: ident, $op: ident, $ty: ident, $out: ty, $factor: literal) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$out s>] {
				self.[<$op _ $ty x $factor>](a, b)
			}
		}
	};
	($func: ident, $op: ident, $($ty: ident x $factor: literal => $out: ty),*) => {
		$(impl_simd_binop!($func, $op, $ty, $out, $factor);)*
	};
	($func: ident, $op: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_simd_binop!($func, $op, $ty, $ty, $factor);)*
	};
	($func: ident, $($ty: ident x $factor: literal => $out: ty),*) => {
		$(impl_simd_binop!($func, $func, $ty, $out, $factor);)*
	};
	($func: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_simd_binop!($func, $func, $ty, $ty, $factor);)*
	};
}

macro_rules! impl_simd_unop {
	($func: ident, $op: ident, $ty: ident, $out: ty, $factor: literal) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>]) -> Self::[<$out s>] {
				self.[<$op _ $ty x $factor>](a)
			}
		}
	};
	($func: ident, $($ty: ident x $factor: literal),*) => {
		$(impl_simd_unop!($func, $func, $ty, $ty, $factor);)*
	};
}

macro_rules! impl_scalar_binop {
	($func: ident, $ty: ident, $out: ty, impl) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$out s>] {
				Scalar128b.[<$func _ $ty s>](a, b)
			}
		}
	};
	($func: ident, $($ty: ident => $out: ty),*) => {
		$(impl_scalar_binop!($func, $ty, $out, impl);)*
	};
	($func: ident, $($ty: ident),*) => {
		$(impl_scalar_binop!($func, $ty, $ty, impl);)*
	};
}

macro_rules! splat {
	($ty: ty, $factor: literal) => {
		paste! {
			#[inline(always)]
			fn [<splat_ $ty s>](self, value: $ty) -> Self::[<$ty s>] {
				self.[<splat_ $ty x $factor>](value)
			}
		}
	};
	($($ty: ident x $factor: literal),*) => {
		$(splat!($ty, $factor);)*
	};
}

impl Simd for V2 {
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

	const REGISTER_COUNT: usize = 16;

	impl_simd_binop!(add, f32 x 4, f64 x 2);

	impl_simd_binop!(add, wrapping_add, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2);

	impl_simd_binop!(sub, f32 x 4, f64 x 2);

	impl_simd_binop!(sub, wrapping_sub, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2);

	impl_simd_binop!(mul, f32 x 4, f64 x 2);

	impl_simd_binop!(mul, wrapping_mul, u16 x 8, i16 x 8, u32 x 4, i32 x 4);

	impl_scalar_binop!(mul, u64, i64, c32, c64);

	impl_simd_binop!(and, m8 x 16, u8 x 16, i8 x 16, m16 x 8, u16 x 8, i16 x 8, m32 x 4, u32 x 4, i32 x 4, m64 x 2, u64 x 2, i64 x 2, f32 x 4, f64 x 2);

	impl_simd_binop!(or, m8 x 16, u8 x 16, i8 x 16, m16 x 8, u16 x 8, i16 x 8, m32 x 4, u32 x 4, i32 x 4, m64 x 2, u64 x 2, i64 x 2, f32 x 4, f64 x 2);

	impl_simd_binop!(xor, m8 x 16, u8 x 16, i8 x 16, m16 x 8, u16 x 8, i16 x 8, m32 x 4, u32 x 4, i32 x 4, m64 x 2, u64 x 2, i64 x 2, f32 x 4, f64 x 2);

	impl_simd_binop!(div, f32 x 4, f64 x 2);

	impl_simd_binop!(equal, cmp_eq, u8 x 16 => m8, u16 x 8 => m16, u32 x 4 => m32, u64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_simd_binop!(greater_than, cmp_gt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_simd_binop!(greater_than_or_equal, cmp_ge, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_simd_binop!(less_than, cmp_lt, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_simd_binop!(less_than_or_equal, cmp_le, u8 x 16 => m8, i8 x 16 => m8, u16 x 8 => m16, i16 x 8 => m16, u32 x 4 => m32, i32 x 4 => m32, u64 x 2 => m64, i64 x 2 => m64, f32 x 4 => m32, f64 x 2 => m64);

	impl_scalar_binop!(conj_mul, c32, c64);

	splat!(u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2, f32 x 4, f64 x 2);

	impl_simd_binop!(max, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_scalar_binop!(max, u64, i64);

	impl_simd_binop!(min, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, f32 x 4, f64 x 2);

	impl_scalar_binop!(min, u64, i64);

	impl_simd_unop!(not, m8 x 16, u8 x 16, m16 x 8, u16 x 8, m32 x 4, u32 x 4, m64 x 2, u64 x 2);

	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
		let sqr = self.mul_f32s(a, a);
		let sqr_rev = self
			.sse
			._mm_shuffle_ps::<0b10_11_00_01>(cast!(sqr), cast!(sqr));
		self.add_f32s(sqr, cast!(sqr_rev))
	}

	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
		let sqr = self.mul_f64s(a, a);
		let sqr_rev = self.sse2._mm_shuffle_pd::<0b0101>(cast!(sqr), cast!(sqr));
		self.add_f64s(sqr, cast!(sqr_rev))
	}

	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
		let max = self.abs_f32s(a);
		let max_rev = self.sse._mm_shuffle_ps::<0b10_11_00_01>(cast!(a), cast!(a));
		self.max_f32s(max, cast!(max_rev))
	}

	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
		let max = self.abs_f64s(a);
		let max_rev = self.sse2._mm_shuffle_pd::<0b0101>(cast!(max), cast!(max));
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
		Scalar128b.conj_mul_add_c32s(a, b, c)
	}

	#[inline(always)]
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		Scalar128b.conj_mul_add_c64s(a, b, c)
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
	unsafe fn mask_load_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *const u8) -> Self::u8s {
		Scalar128b.mask_load_ptr_u8s(mask, ptr)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u16s(self, mask: MemMask<Self::m16s>, ptr: *const u16) -> Self::u16s {
		Scalar128b.mask_load_ptr_u16s(mask, ptr)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u32s(self, mask: MemMask<Self::m32s>, ptr: *const u32) -> Self::u32s {
		Scalar128b.mask_load_ptr_u32s(mask, ptr)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u64s(self, mask: MemMask<Self::m64s>, ptr: *const u64) -> Self::u64s {
		cast!(self.mask_load_ptr_u32s(
			MemMask {
				mask: cast!(mask.mask),
				#[cfg(target_arch = "x86_64")]
				load: mask.load,
				#[cfg(target_arch = "x86_64")]
				store: mask.store
			},
			ptr as _
		))
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
	unsafe fn mask_store_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *mut u8, values: Self::u8s) {
		Scalar128b.mask_store_ptr_u8s(mask, ptr, values);
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
		Scalar128b.mask_store_ptr_u16s(mask, ptr, values);
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
		Scalar128b.mask_store_ptr_u32s(mask, ptr, values);
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
		self.mask_store_ptr_u32s(
			MemMask {
				mask: cast!(mask.mask),
				#[cfg(target_arch = "x86_64")]
				load: mask.load,
				#[cfg(target_arch = "x86_64")]
				store: mask.store,
			},
			ptr as _,
			cast!(values),
		)
	}

	#[inline(always)]
	fn mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		Scalar128b.mul_add_c32s(a, b, c)
	}

	#[inline(always)]
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		Scalar128b.mul_add_c64s(a, b, c)
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
		Scalar128b.mul_add_f32s(a, b, c)
	}

	#[inline(always)]
	fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		Scalar128b.mul_add_f64s(a, b, c)
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
	fn reduce_max_c32s(self, a: Self::c32s) -> c32 {
		self.reduce_max_c32x2(a)
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		self.reduce_max_c64x1(a)
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
		self.reduce_min_c32x2(a)
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		self.reduce_min_c64x1(a)
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
		self.reduce_sum_c32x2(a)
	}

	#[inline(always)]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		self.reduce_sum_c64x1(a)
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
		Scalar128b.rotate_right_c32s(a, amount)
	}

	#[inline(always)]
	fn rotate_right_c64s(self, a: Self::c64s, amount: usize) -> Self::c64s {
		Scalar128b.rotate_right_c64s(a, amount)
	}

	#[inline(always)]
	fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s {
		Scalar128b.rotate_right_u32s(a, amount)
	}

	#[inline(always)]
	fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s {
		Scalar128b.rotate_right_u64s(a, amount)
	}

	#[inline(always)]
	fn select_u32s(
		self,
		mask: Self::m32s,
		if_true: Self::u32s,
		if_false: Self::u32s,
	) -> Self::u32s {
		let mask: __m128 = cast!(mask);
		let if_true: __m128 = cast!(if_true);
		let if_false: __m128 = cast!(if_false);

		cast!(self.sse4_1._mm_blendv_ps(if_false, if_true, mask))
	}

	#[inline(always)]
	fn select_u64s(
		self,
		mask: Self::m64s,
		if_true: Self::u64s,
		if_false: Self::u64s,
	) -> Self::u64s {
		let mask: __m128d = cast!(mask);
		let if_true: __m128d = cast!(if_true);
		let if_false: __m128d = cast!(if_false);

		cast!(self.sse4_1._mm_blendv_pd(if_false, if_true, mask))
	}

	#[inline(always)]
	fn splat_c32s(self, value: c32) -> Self::c32s {
		cast!(self.splat_f64s(cast!(value)))
	}

	#[inline(always)]
	fn splat_c64s(self, value: c64) -> Self::c64s {
		Scalar128b.splat_c64s(value)
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
	fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s {
		Scalar128b.swap_re_im_c32s(a)
	}

	#[inline(always)]
	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
		Scalar128b.swap_re_im_c64s(a)
	}

	#[inline(always)]
	fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
		struct Impl<Op> {
			this: V2,
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
		self.widening_mul_u32x4(a, b)
	}

	#[inline(always)]
	fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		Scalar128b.wrapping_dyn_shl_u32s(a, amount)
	}

	#[inline(always)]
	fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		Scalar128b.wrapping_dyn_shr_u32s(a, amount)
	}

	#[inline(always)]
	fn sqrt_f32s(self, a: Self::f32s) -> Self::f32s {
		self.sqrt_f32x4(a)
	}

	#[inline(always)]
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
		self.sqrt_f64x2(a)
	}
}
