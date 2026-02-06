use super::*;

// https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels
simd_type!({
	/// SSE instruction set.
	#[allow(missing_docs)]
	pub struct V1 {
		pub sse: f!("sse"),
		pub sse2: f!("sse2"),
		pub fxsr: f!("fxsr"),
	}
});

impl Seal for V1 {}

impl V1 {
	binop_128_nosign!(sse: add, "Computes `a + b` for each lane of `a` and `b`.", f32 x 4);

	binop_128_nosign!(sse2: add, "Adds the elements of each lane of `a` and `b`.", f64 x 2);

	binop_128_nosign!(sse2: add, "Adds the elements of each lane of `a` and `b`, with wrapping on overflow.", wrapping_add, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2);

	binop_128!(sse: and, "Returns `a & b` for each bit in `a` and `b`.", f32 x 4);

	binop_128!(sse2: and, "Returns `a & b` for each bit in `a` and `b`.", f64 x 2);

	binop_128_full!(sse2: and, "Returns `a & b` for each bit in `a` and `b`.", m8 x 16, u8 x 16, i8 x 16, m16 x 8, u16 x 8, i16 x 8, m32 x 4, u32 x 4, i32 x 4, m64 x 2, u64 x 2, i64 x 2);

	binop_128!(sse: andnot, "Returns `!a & b` for each bit in `a` and `b`.", f32 x 4);

	binop_128!(sse2: andnot, "Returns `!a & b` for each bit in `a` and `b`.", f64 x 2);

	binop_128_full!(sse2: andnot, "Returns `!a & b` for each bit in `a` and `b`.", m8 x 16, u8 x 16, i8 x 16, m16 x 8, u16 x 8, i16 x 8, m32 x 4, u32 x 4, i32 x 4, m64 x 2, u64 x 2, i64 x 2);

	binop_128!(sse2: avg, "Computes `average(a, b)` for each lane of `a` and `b`.", average, u8 x 16, u16 x 8);

	binop_128_nosign!(sse: cmpeq, "Compares the elements in each lane of `a` and `b` for equality.", cmp_eq, f32 x 4 => m32);

	binop_128_nosign!(sse2: cmpeq, "Compares the elements in each lane of `a` and `b` for equality.", cmp_eq, m8 x 16 => m8, u8 x 16 => m8, i8 x 16 => m8, m16 x 8 => m16, u16 x 8 => m16, i16 x 8 => m16, m32 x 4 => m32, u32 x 4 => m32, i32 x 4 => m32, f64 x 2 => m64);

	binop_128!(sse: cmpge, "Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.", cmp_ge, f32 x 4 => m32);

	binop_128!(sse2: cmpge, "Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.", cmp_ge, f64 x 2 => m64);

	binop_128!(sse: cmpgt, "Compares the elements in each lane of `a` and `b` for greater-than.", cmp_gt, f32 x 4 => m32);

	binop_128!(sse2: cmpgt, "Compares the elements in each lane of `a` and `b` for equality.", cmp_gt, i8 x 16 => m8, i16 x 8 => m16, i32 x 4 => m32, f64 x 2 => m64);

	binop_128!(sse: cmplt, "Compares the elements in each lane of `a` and `b` for greater-than.", cmp_lt, f32 x 4 => m32);

	binop_128!(sse2: cmplt, "Compares the elements in each lane of `a` and `b` for less-than.", cmp_lt, i8 x 16 => m8, i16 x 8 => m16, i32 x 4 => m32, f64 x 2 => m64);

	binop_128!(sse: cmple, "Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.", cmp_le, f32 x 4 => m32);

	binop_128!(sse2: cmple, "Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.", cmp_le, f64 x 2 => m64);

	binop_128!(sse: cmpneq, "Compares the elements in each lane of `a` and `b` for inequality.", cmp_not_eq, f32 x 4 => m32);

	binop_128!(sse2: cmpneq, "Compares the elements in each lane of `a` and `b` for inequality.", cmp_not_eq, f64 x 2 => m64);

	binop_128!(sse: cmpnge, "Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.", cmp_not_ge, f32 x 4 => m32);

	binop_128!(sse2: cmpnge, "Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.", cmp_not_ge, f64 x 2 => m64);

	binop_128!(sse: cmpngt, "Compares the elements in each lane of `a` and `b` for not-greater-than.", cmp_not_gt, f32 x 4 => m32);

	binop_128!(sse2: cmpngt, "Compares the elements in each lane of `a` and `b` for not-greater-than.", cmp_not_gt, f64 x 2 => m64);

	binop_128!(sse: cmpnle, "Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.", cmp_not_le, f32 x 4 => m32);

	binop_128!(sse2: cmpnle, "Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.", cmp_not_le, f64 x 2 => m64);

	binop_128!(sse: cmpnlt, "Compares the elements in each lane of `a` and `b` for not-less-than.", cmp_not_lt, f32 x 4 => m32);

	binop_128!(sse2: cmpnlt, "Compares the elements in each lane of `a` and `b` for not-less-than.", cmp_not_lt, f64 x 2 => m64);

	binop_128!(sse: div, "Divides the elements of each lane of `a` and `b`.", f32 x 4);

	binop_128!(sse2: div, "Divides the elements of each lane of `a` and `b`.", f64 x 2);

	binop_128!(sse: max, "Computes `max(a, b)`. for each lane in `a` and `b`.", f32 x 4);

	binop_128!(sse2: max, "Computes `max(a, b)`. for each lane in `a` and `b`.", u8 x 16, i16 x 8, f64 x 2);

	binop_128!(sse: min, "Computes `max(a, b)`. for each lane in `a` and `b`.", f32 x 4);

	binop_128!(sse2: min, "Computes `max(a, b)`. for each lane in `a` and `b`.", u8 x 16, i16 x 8, f64 x 2);

	binop_128!(sse: mul, "Computes `a * b` for each lane in `a` and `b`.", f32 x 4);

	binop_128!(sse2: mul, "Computes `a * b` for each lane in `a` and `b`.", f64 x 2);

	binop_128_nosign!(sse2: mullo, "Computes `a * b` for each lane in `a` and `b`, with wrapping overflow.", wrapping_mul, u16 x 8, i16 x 8);

	binop_128!(sse: or, "Returns `a | b` for each bit in `a` and `b`.", f32 x 4);

	binop_128!(sse2: or, "Returns `a | b` for each bit in `a` and `b`.", f64 x 2);

	binop_128_full!(sse2: or, "Returns `a | b` for each bit in `a` and `b`.", m8 x 16, u8 x 16, i8 x 16, m16 x 8, u16 x 8, i16 x 8, m32 x 4, u32 x 4, i32 x 4, m64 x 2, u64 x 2, i64 x 2);

	binop_128!(sse2: adds, "Adds the elements of each lane of `a` and `b`, with saturation.", saturating_add, u8 x 16, i8 x 16, u16 x 8, i16 x 8);

	binop_128!(sse2: subs, "Subtracts the elements of each lane of `a` and `b`, with saturation.", saturating_sub, u8 x 16, i8 x 16, u16 x 8, i16 x 8);

	binop_128_nosign!(sse: sub, "Subtracts the elements of each lane of `a` and `b`.", f32 x 4);

	binop_128_nosign!(sse2: sub, "Subtracts the elements of each lane of `a` and `b`.", f64 x 2);

	binop_128_nosign!(sse2: sub, "Subtracts the elements of each lane of `a` and `b`, with wrapping overflow.", wrapping_sub, u8 x 16, i8 x 16, u16 x 8, i16 x 8, u32 x 4, i32 x 4, u64 x 2, i64 x 2);

	binop_128!(sse: xor, "Returns `a ^ b` for each bit in `a` and `b`.", f32 x 4);

	binop_128!(sse2: xor, "Returns `a ^ b` for each bit in `a` and `b`.", f64 x 2);

	binop_128_full!(sse2: xor, "Returns `a ^ b` for each bit in `a` and `b`.", m8 x 16, u8 x 16, i8 x 16, m16 x 8, u16 x 8, i16 x 8, m32 x 4, u32 x 4, i32 x 4, m64 x 2, u64 x 2, i64 x 2);

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
	pub fn cmp_ge_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
		self.not_m16x8(self.cmp_lt_i16x8(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
		self.not_m32x4(self.cmp_lt_i32x4(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
		self.not_m8x16(self.cmp_lt_i8x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
		self.not_m16x8(self.cmp_lt_u16x8(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
		self.not_m32x4(self.cmp_lt_u32x4(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
		self.not_m8x16(self.cmp_lt_u8x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
		let k = self.splat_u16x8(0x8000);
		self.cmp_gt_i16x8(cast!(self.xor_u16x8(a, k)), cast!(self.xor_u16x8(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
		let k = self.splat_u32x4(0x80000000);
		self.cmp_gt_i32x4(cast!(self.xor_u32x4(a, k)), cast!(self.xor_u32x4(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
		let k = self.splat_u8x16(0x80);
		self.cmp_gt_i8x16(cast!(self.xor_u8x16(a, k)), cast!(self.xor_u8x16(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
		self.not_m16x8(self.cmp_gt_i16x8(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
		self.not_m32x4(self.cmp_gt_i32x4(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
		self.not_m8x16(self.cmp_gt_i8x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
		self.not_m16x8(self.cmp_gt_u16x8(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
		self.not_m32x4(self.cmp_gt_u32x4(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
		self.not_m8x16(self.cmp_gt_u8x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
		let k = self.splat_u16x8(0x8000);
		self.cmp_lt_i16x8(cast!(self.xor_u16x8(a, k)), cast!(self.xor_u16x8(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
		let k = self.splat_u32x4(0x80000000);
		self.cmp_lt_i32x4(cast!(self.xor_u32x4(a, k)), cast!(self.xor_u32x4(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
		let k = self.splat_u8x16(0x80);
		self.cmp_lt_i8x16(cast!(self.xor_u8x16(a, k)), cast!(self.xor_u8x16(b, k)))
	}

	/// Converts a `f32x4` to `f64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_f32x4_to_f64x2(self, a: f32x4) -> f64x2 {
		cast!(self.sse2._mm_cvtps_pd(cast!(a)))
	}

	/// Converts a `f32x4` to `i32x4`, elementwise.
	#[inline(always)]
	pub fn convert_f32x4_to_i32x4(self, a: f32x4) -> i32x4 {
		cast!(self.sse2._mm_cvttps_epi32(cast!(a)))
	}

	/// Converts a `f64x2` to `f32x4`, elementwise, filling the remaining elements with zeros.
	#[inline(always)]
	pub fn convert_f64x2_to_f32x4(self, a: f64x2) -> f32x4 {
		cast!(self.sse2._mm_cvtpd_ps(cast!(a)))
	}

	/// Converts a `f64x2` to `i32x4`, elementwise.
	#[inline(always)]
	pub fn convert_f64x2_to_i32x4(self, a: f64x2) -> i32x4 {
		cast!(self.sse2._mm_cvttpd_epi32(cast!(a)))
	}

	/// Converts a `i16x8` to `u16x8`, elementwise.
	#[inline(always)]
	pub fn convert_i16x8_to_u16x8(self, a: i16x8) -> u16x8 {
		cast!(a)
	}

	/// Converts a `i32x4` to `f32x4`, elementwise.
	#[inline(always)]
	pub fn convert_i32x4_to_f32x4(self, a: i32x4) -> f32x4 {
		cast!(self.sse2._mm_cvtepi32_ps(cast!(a)))
	}

	/// Converts a `i32x4` to `f64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i32x4_to_f64x2(self, a: i32x4) -> f64x2 {
		cast!(self.sse2._mm_cvtepi32_pd(cast!(a)))
	}

	/// Converts a `i32x4` to `u32x4`, elementwise.
	#[inline(always)]
	pub fn convert_i32x4_to_u32x4(self, a: i32x4) -> u32x4 {
		cast!(a)
	}

	/// Converts a `i8x16` to `u8x16`, elementwise.
	#[inline(always)]
	pub fn convert_i8x16_to_u8x16(self, a: i8x16) -> u8x16 {
		cast!(a)
	}

	/// Converts a `u16x8` to `i16x8`, elementwise.
	#[inline(always)]
	pub fn convert_u16x8_to_i16x8(self, a: u16x8) -> i16x8 {
		cast!(a)
	}

	/// Converts a `u32x4` to `i32x4`, elementwise.
	#[inline(always)]
	pub fn convert_u32x4_to_i32x4(self, a: u32x4) -> i32x4 {
		cast!(a)
	}

	/// Converts a `u8x16` to `i8x16`, elementwise.
	#[inline(always)]
	pub fn convert_u8x16_to_i8x16(self, a: u8x16) -> i8x16 {
		cast!(a)
	}

	/// Checks if the elements in each lane of `a` are NaN.
	#[inline(always)]
	pub fn is_nan_f32x4(self, a: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpunord_ps(cast!(a), cast!(a)))
	}

	/// Checks if the elements in each lane of `a` are NaN.
	#[inline(always)]
	pub fn is_nan_f64x2(self, a: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpunord_pd(cast!(a), cast!(a)))
	}

	/// Checks if the elements in each lane of `a` are not NaN.
	#[inline(always)]
	pub fn is_not_nan_f32x4(self, a: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpord_ps(cast!(a), cast!(a)))
	}

	/// Checks if the elements in each lane of `a` are not NaN.
	#[inline(always)]
	pub fn is_not_nan_f64x2(self, a: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpord_pd(cast!(a), cast!(a)))
	}

	/// See [_mm_madd_epi16].
	///
	/// [_mm_madd_epi16]: core::arch::x86_64::_mm_madd_epi16
	#[inline(always)]
	pub fn multiply_wrapping_add_adjacent_i16x8(self, a: i16x8, b: i16x8) -> i32x4 {
		cast!(self.sse2._mm_madd_epi16(cast!(a), cast!(b)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_i16x8(self, a: i16x8) -> i16x8 {
		self.xor_i16x8(a, self.splat_i16x8(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_i32x4(self, a: i32x4) -> i32x4 {
		self.xor_i32x4(a, self.splat_i32x4(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_i64x2(self, a: i64x2) -> i64x2 {
		self.xor_i64x2(a, self.splat_i64x2(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_i8x16(self, a: i8x16) -> i8x16 {
		self.xor_i8x16(a, self.splat_i8x16(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_m16x8(self, a: m16x8) -> m16x8 {
		self.xor_m16x8(a, self.splat_m16x8(m16::new(true)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_m32x4(self, a: m32x4) -> m32x4 {
		self.xor_m32x4(a, self.splat_m32x4(m32::new(true)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_m64x2(self, a: m64x2) -> m64x2 {
		self.xor_m64x2(a, self.splat_m64x2(m64::new(true)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_m8x16(self, a: m8x16) -> m8x16 {
		self.xor_m8x16(a, self.splat_m8x16(m8::new(true)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_u16x8(self, a: u16x8) -> u16x8 {
		self.xor_u16x8(a, self.splat_u16x8(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_u32x4(self, a: u32x4) -> u32x4 {
		self.xor_u32x4(a, self.splat_u32x4(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_u64x2(self, a: u64x2) -> u64x2 {
		self.xor_u64x2(a, self.splat_u64x2(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_u8x16(self, a: u8x16) -> u8x16 {
		self.xor_u8x16(a, self.splat_u8x16(!0))
	}

	/// See [_mm_packs_epi16].
	///
	/// [_mm_packs_epi16]: core::arch::x86_64::_mm_packs_epi16
	#[inline(always)]
	pub fn pack_with_signed_saturation_i16x8(self, a: i16x8, b: i16x8) -> i8x16 {
		cast!(self.sse2._mm_packs_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm_packs_epi32].
	///
	/// [_mm_packs_epi32]: core::arch::x86_64::_mm_packs_epi32
	#[inline(always)]
	pub fn pack_with_signed_saturation_i32x4(self, a: i32x4, b: i32x4) -> i16x8 {
		cast!(self.sse2._mm_packs_epi32(cast!(a), cast!(b)))
	}

	/// See [_mm_packus_epi16].
	///
	/// [_mm_packus_epi16]: core::arch::x86_64::_mm_packus_epi16
	#[inline(always)]
	pub fn pack_with_unsigned_saturation_i16x8(self, a: i16x8, b: i16x8) -> u8x16 {
		cast!(self.sse2._mm_packus_epi16(cast!(a), cast!(b)))
	}

	#[inline(always)]
	pub fn reduce_max_c32x2(self, a: f32x4) -> c32 {
		// a0 a1 a2 a3
		let a: __m128 = cast!(a);
		// a2 a3 a2 a3
		let hi = self.sse._mm_movehl_ps(a, a);

		// a0+a2 a1+a3 _ _
		let r0 = self.sse._mm_max_ps(a, hi);

		cast!(self.sse2._mm_cvtsd_f64(cast!(r0)))
	}

	#[inline(always)]
	pub fn reduce_max_c64x1(self, a: f64x2) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	pub fn reduce_max_f32x4(self, a: f32x4) -> f32 {
		let a: __m128 = cast!(a);
		let hi = self.sse._mm_movehl_ps(a, a);
		let r0 = self.sse._mm_max_ps(a, hi);
		let r0_shuffled = self.sse._mm_shuffle_ps::<0b0001>(r0, r0);
		let r = self.sse._mm_max_ss(r0, r0_shuffled);
		self.sse._mm_cvtss_f32(r)
	}

	#[inline(always)]
	pub fn reduce_max_f64x2(self, a: f64x2) -> f64 {
		let a: __m128d = cast!(a);
		let hi = cast!(self.sse._mm_movehl_ps(cast!(a), cast!(a)));
		let r = self.sse2._mm_max_sd(a, hi);
		self.sse2._mm_cvtsd_f64(r)
	}

	#[inline(always)]
	pub fn reduce_min_c32x2(self, a: f32x4) -> c32 {
		// a0 a1 a2 a3
		let a: __m128 = cast!(a);
		// a2 a3 a2 a3
		let hi = self.sse._mm_movehl_ps(a, a);

		// a0+a2 a1+a3 _ _
		let r0 = self.sse._mm_min_ps(a, hi);

		cast!(self.sse2._mm_cvtsd_f64(cast!(r0)))
	}

	#[inline(always)]
	pub fn reduce_min_c64x1(self, a: f64x2) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	pub fn reduce_min_f32x4(self, a: f32x4) -> f32 {
		let a: __m128 = cast!(a);
		let hi = self.sse._mm_movehl_ps(a, a);
		let r0 = self.sse._mm_min_ps(a, hi);
		let r0_shuffled = self.sse._mm_shuffle_ps::<0b0001>(r0, r0);
		let r = self.sse._mm_min_ss(r0, r0_shuffled);
		self.sse._mm_cvtss_f32(r)
	}

	#[inline(always)]
	pub fn reduce_min_f64x2(self, a: f64x2) -> f64 {
		let a: __m128d = cast!(a);
		let hi = cast!(self.sse._mm_movehl_ps(cast!(a), cast!(a)));
		let r = self.sse2._mm_min_sd(a, hi);
		self.sse2._mm_cvtsd_f64(r)
	}

	#[inline(always)]
	pub fn reduce_product_f32x4(self, a: f32x4) -> f32 {
		let a: __m128 = cast!(a);
		let hi = self.sse._mm_movehl_ps(a, a);
		let r0 = self.sse._mm_mul_ps(a, hi);
		let r0_shuffled = self.sse._mm_shuffle_ps::<0b0001>(r0, r0);
		let r = self.sse._mm_mul_ss(r0, r0_shuffled);
		self.sse._mm_cvtss_f32(r)
	}

	#[inline(always)]
	pub fn reduce_product_f64x2(self, a: f64x2) -> f64 {
		let a: __m128d = cast!(a);
		let hi = cast!(self.sse._mm_movehl_ps(cast!(a), cast!(a)));
		let r = self.sse2._mm_mul_sd(a, hi);
		self.sse2._mm_cvtsd_f64(r)
	}

	#[inline(always)]
	pub fn reduce_sum_c32x2(self, a: f32x4) -> c32 {
		// a0 a1 a2 a3
		let a: __m128 = cast!(a);
		// a2 a3 a2 a3
		let hi = self.sse._mm_movehl_ps(a, a);

		// a0+a2 a1+a3 _ _
		let r0 = self.sse._mm_add_ps(a, hi);

		cast!(self.sse2._mm_cvtsd_f64(cast!(r0)))
	}

	#[inline(always)]
	pub fn reduce_sum_c64x1(self, a: f64x2) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	pub fn reduce_sum_f32x4(self, a: f32x4) -> f32 {
		// a0 a1 a2 a3
		let a: __m128 = cast!(a);
		// a2 a3 a2 a3
		let hi = self.sse._mm_movehl_ps(a, a);

		// a0+a2 a1+a3 _ _
		let r0 = self.sse._mm_add_ps(a, hi);
		// a1+a3 a2+a1 _ _
		let r0_shuffled = self.sse._mm_shuffle_ps::<0b0001>(r0, r0);

		let r = self.sse._mm_add_ss(r0, r0_shuffled);

		self.sse._mm_cvtss_f32(r)
	}

	#[inline(always)]
	pub fn reduce_sum_f64x2(self, a: f64x2) -> f64 {
		let a: __m128d = cast!(a);
		let hi = cast!(self.sse._mm_movehl_ps(cast!(a), cast!(a)));
		let r = self.sse2._mm_add_sd(a, hi);
		self.sse2._mm_cvtsd_f64(r)
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_i16x8<const AMOUNT: i32>(self, a: i16x8) -> i16x8 {
		cast!(self.sse2._mm_slli_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_i32x4<const AMOUNT: i32>(self, a: i32x4) -> i32x4 {
		cast!(self.sse2._mm_slli_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_i64x2<const AMOUNT: i32>(self, a: i64x2) -> i64x2 {
		cast!(self.sse2._mm_slli_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_u16x8<const AMOUNT: i32>(self, a: u16x8) -> u16x8 {
		cast!(self.sse2._mm_slli_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_u32x4<const AMOUNT: i32>(self, a: u32x4) -> u32x4 {
		cast!(self.sse2._mm_slli_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_u64x2<const AMOUNT: i32>(self, a: u64x2) -> u64x2 {
		cast!(self.sse2._mm_slli_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_i16x8(self, a: i16x8, amount: u64x2) -> i16x8 {
		cast!(self.sse2._mm_sll_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_i32x4(self, a: i32x4, amount: u64x2) -> i32x4 {
		cast!(self.sse2._mm_sll_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_i64x2(self, a: i64x2, amount: u64x2) -> u64x2 {
		cast!(self.sse2._mm_sll_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_u16x8(self, a: u16x8, amount: u64x2) -> u16x8 {
		cast!(self.sse2._mm_sll_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_u32x4(self, a: u32x4, amount: u64x2) -> u32x4 {
		cast!(self.sse2._mm_sll_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_u64x2(self, a: u64x2, amount: u64x2) -> u64x2 {
		cast!(self.sse2._mm_sll_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_const_i16x8<const AMOUNT: i32>(self, a: i16x8) -> i16x8 {
		cast!(self.sse2._mm_srai_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_const_i32x4<const AMOUNT: i32>(self, a: i32x4) -> i32x4 {
		cast!(self.sse2._mm_srai_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_const_u16x8<const AMOUNT: i32>(self, a: u16x8) -> u16x8 {
		cast!(self.sse2._mm_srli_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_const_u32x4<const AMOUNT: i32>(self, a: u32x4) -> u32x4 {
		cast!(self.sse2._mm_srli_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_const_u64x2<const AMOUNT: i32>(self, a: u64x2) -> u64x2 {
		cast!(self.sse2._mm_srli_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_i16x8(self, a: i16x8, amount: u64x2) -> i16x8 {
		cast!(self.sse2._mm_sra_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_i32x4(self, a: i32x4, amount: u64x2) -> i32x4 {
		cast!(self.sse2._mm_sra_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_u16x8(self, a: u16x8, amount: u64x2) -> u16x8 {
		cast!(self.sse2._mm_srl_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_u32x4(self, a: u32x4, amount: u64x2) -> u32x4 {
		cast!(self.sse2._mm_srl_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_u64x2(self, a: u64x2, amount: u64x2) -> u64x2 {
		cast!(self.sse2._mm_srl_epi64(cast!(a), cast!(amount)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_f32x4(self, value: f32) -> f32x4 {
		cast!(self.sse._mm_set1_ps(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_f64x2(self, value: f64) -> f64x2 {
		cast!(self.sse2._mm_set1_pd(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i16x8(self, value: i16) -> i16x8 {
		cast!(self.sse2._mm_set1_epi16(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i32x4(self, value: i32) -> i32x4 {
		cast!(self.sse2._mm_set1_epi32(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i64x2(self, value: i64) -> i64x2 {
		cast!(self.sse2._mm_set1_epi64x(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i8x16(self, value: i8) -> i8x16 {
		cast!(self.sse2._mm_set1_epi8(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m16x8(self, value: m16) -> m16x8 {
		cast!(self.sse2._mm_set1_epi16(value.0 as i16))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m32x4(self, value: m32) -> m32x4 {
		cast!(self.sse2._mm_set1_epi32(value.0 as i32))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m64x2(self, value: m64) -> m64x2 {
		cast!(self.sse2._mm_set1_epi64x(value.0 as i64))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m8x16(self, value: m8) -> m8x16 {
		cast!(self.sse2._mm_set1_epi8(value.0 as i8))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u16x8(self, value: u16) -> u16x8 {
		cast!(self.sse2._mm_set1_epi16(value as i16))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u32x4(self, value: u32) -> u32x4 {
		cast!(self.sse2._mm_set1_epi32(value as i32))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u64x2(self, value: u64) -> u64x2 {
		cast!(self.sse2._mm_set1_epi64x(value as i64))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u8x16(self, value: u8) -> u8x16 {
		cast!(self.sse2._mm_set1_epi8(value as i8))
	}

	/// Computes the square roots of the elements of each lane of `a`.
	#[inline(always)]
	pub fn sqrt_f32x4(self, a: f32x4) -> f32x4 {
		cast!(self.sse._mm_sqrt_ps(cast!(a)))
	}

	/// Computes the square roots of the elements of each lane of `a`.
	#[inline(always)]
	pub fn sqrt_f64x2(self, a: f64x2) -> f64x2 {
		cast!(self.sse2._mm_sqrt_pd(cast!(a)))
	}

	/// See [_mm_sad_epu8].
	///
	/// [_mm_sad_epu8]: core::arch::x86_64::_mm_sad_epu8
	#[inline(always)]
	pub fn sum_of_absolute_differences_u8x16(self, a: u8x16, b: u8x16) -> u64x2 {
		cast!(self.sse2._mm_sad_epu8(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_i16x8(self, a: i16x8, b: i16x8) -> (u16x8, i16x8) {
		(
			cast!(self.sse2._mm_mullo_epi16(cast!(a), cast!(b))),
			cast!(self.sse2._mm_mulhi_epi16(cast!(a), cast!(b))),
		)
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_u16x8(self, a: u16x8, b: u16x8) -> (u16x8, u16x8) {
		(
			cast!(self.sse2._mm_mullo_epi16(cast!(a), cast!(b))),
			cast!(self.sse2._mm_mulhi_epu16(cast!(a), cast!(b))),
		)
	}
}
