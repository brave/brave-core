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

impl V2 {
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

	/// Computes `a + b` for each lane of `a` and `b`.
	#[inline(always)]
	pub fn add_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_add_ps(cast!(a), cast!(b)))
	}

	/// Computes `a + b` for each lane of `a` and `b`.
	#[inline(always)]
	pub fn add_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_add_pd(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_and_ps(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_and_pd(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn and_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_and_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_andnot_ps(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_andnot_pd(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Returns `!a & b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn andnot_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_andnot_si128(cast!(a), cast!(b)))
	}

	/// Applies the sign of each element of `sign` to the corresponding lane in `a`.
	/// - If `sign` is zero, the corresponding element is zeroed.
	/// - If `sign` is positive, the corresponding element is returned as is.
	/// - If `sign` is negative, the corresponding element is negated.
	#[inline(always)]
	pub fn apply_sign_i16x8(self, sign: i16x8, a: i16x8) -> i16x8 {
		cast!(self.ssse3._mm_sign_epi16(cast!(a), cast!(sign)))
	}

	/// Applies the sign of each element of `sign` to the corresponding lane in `a`.
	/// - If `sign` is zero, the corresponding element is zeroed.
	/// - If `sign` is positive, the corresponding element is returned as is.
	/// - If `sign` is negative, the corresponding element is negated.
	#[inline(always)]
	pub fn apply_sign_i32x4(self, sign: i32x4, a: i32x4) -> i32x4 {
		cast!(self.ssse3._mm_sign_epi32(cast!(a), cast!(sign)))
	}

	/// Applies the sign of each element of `sign` to the corresponding lane in `a`.
	/// - If `sign` is zero, the corresponding element is zeroed.
	/// - If `sign` is positive, the corresponding element is returned as is.
	/// - If `sign` is negative, the corresponding element is negated.
	#[inline(always)]
	pub fn apply_sign_i8x16(self, sign: i8x16, a: i8x16) -> i8x16 {
		cast!(self.ssse3._mm_sign_epi8(cast!(a), cast!(sign)))
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

	/// Computes `average(a, b)` for each lane of `a` and `b`.
	#[inline(always)]
	pub fn average_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_avg_epu16(cast!(a), cast!(b)))
	}

	/// Computes `average(a, b)` for each lane of `a` and `b`.
	#[inline(always)]
	pub fn average_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_avg_epu8(cast!(a), cast!(b)))
	}

	/// Returns `ceil(a)` for each lane of `a`, rounding towards positive infinity.
	#[inline(always)]
	pub fn ceil_f32x4(self, a: f32x4) -> f32x4 {
		cast!(self.sse4_1._mm_ceil_ps(cast!(a)))
	}

	/// Returns `ceil(a)` for each lane of `a`, rounding towards positive infinity.
	#[inline(always)]
	pub fn ceil_f64x2(self, a: f64x2) -> f64x2 {
		cast!(self.sse4_1._mm_ceil_pd(cast!(a)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpeq_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpeq_pd(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
		cast!(self.sse2._mm_cmpeq_epi16(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
		cast!(self.sse2._mm_cmpeq_epi32(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
		cast!(self.sse4_1._mm_cmpeq_epi64(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
		cast!(self.sse2._mm_cmpeq_epi8(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u16x8(self, a: u16x8, b: u16x8) -> m16x8 {
		cast!(self.sse2._mm_cmpeq_epi16(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u32x4(self, a: u32x4, b: u32x4) -> m32x4 {
		cast!(self.sse2._mm_cmpeq_epi32(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
		cast!(self.sse4_1._mm_cmpeq_epi64(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
		cast!(self.sse2._mm_cmpeq_epi8(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpge_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpge_pd(cast!(a), cast!(b)))
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
	pub fn cmp_ge_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
		self.not_m64x2(self.cmp_lt_i64x2(a, b))
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
	pub fn cmp_ge_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
		self.not_m64x2(self.cmp_lt_u64x2(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
		self.not_m8x16(self.cmp_lt_u8x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpgt_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpgt_pd(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
		cast!(self.sse2._mm_cmpgt_epi16(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
		cast!(self.sse2._mm_cmpgt_epi32(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
		cast!(self.sse4_2._mm_cmpgt_epi64(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
		cast!(self.sse2._mm_cmpgt_epi8(cast!(a), cast!(b)))
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
	pub fn cmp_gt_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
		let k = self.splat_u64x2(0x8000000000000000);
		self.cmp_gt_i64x2(cast!(self.xor_u64x2(a, k)), cast!(self.xor_u64x2(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
		let k = self.splat_u8x16(0x80);
		self.cmp_gt_i8x16(cast!(self.xor_u8x16(a, k)), cast!(self.xor_u8x16(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmple_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmple_pd(cast!(a), cast!(b)))
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
	pub fn cmp_le_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
		self.not_m64x2(self.cmp_gt_i64x2(a, b))
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
	pub fn cmp_le_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
		self.not_m64x2(self.cmp_gt_u64x2(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
		self.not_m8x16(self.cmp_gt_u8x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmplt_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmplt_pd(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i16x8(self, a: i16x8, b: i16x8) -> m16x8 {
		cast!(self.sse2._mm_cmplt_epi16(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i32x4(self, a: i32x4, b: i32x4) -> m32x4 {
		cast!(self.sse2._mm_cmplt_epi32(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i64x2(self, a: i64x2, b: i64x2) -> m64x2 {
		cast!(self.sse4_2._mm_cmpgt_epi64(cast!(b), cast!(a)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i8x16(self, a: i8x16, b: i8x16) -> m8x16 {
		cast!(self.sse2._mm_cmplt_epi8(cast!(a), cast!(b)))
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
	pub fn cmp_lt_u64x2(self, a: u64x2, b: u64x2) -> m64x2 {
		let k = self.splat_u64x2(0x8000000000000000);
		self.cmp_lt_i64x2(cast!(self.xor_u64x2(a, k)), cast!(self.xor_u64x2(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u8x16(self, a: u8x16, b: u8x16) -> m8x16 {
		let k = self.splat_u8x16(0x80);
		self.cmp_lt_i8x16(cast!(self.xor_u8x16(a, k)), cast!(self.xor_u8x16(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for inequality.
	#[inline(always)]
	pub fn cmp_not_eq_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpneq_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for inequality.
	#[inline(always)]
	pub fn cmp_not_eq_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpneq_pd(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_ge_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpnge_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_ge_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpnge_pd(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than.
	#[inline(always)]
	pub fn cmp_not_gt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpngt_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than.
	#[inline(always)]
	pub fn cmp_not_gt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpngt_pd(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_le_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpnle_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_le_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpnle_pd(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f32x4(self, a: f32x4, b: f32x4) -> m32x4 {
		cast!(self.sse._mm_cmpnlt_ps(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f64x2(self, a: f64x2, b: f64x2) -> m64x2 {
		cast!(self.sse2._mm_cmpnlt_pd(cast!(a), cast!(b)))
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

	/// Converts a `i16x8` to `u16x8`, elementwise.
	#[inline(always)]
	pub fn convert_i16x8_to_u16x8(self, a: i16x8) -> u16x8 {
		cast!(a)
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

	/// Converts a `i32x4` to `i64x2`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i32x4_to_i64x2(self, a: i32x4) -> i64x2 {
		cast!(self.sse4_1._mm_cvtepi32_epi64(cast!(a)))
	}

	/// Converts a `i32x4` to `u32x4`, elementwise.
	#[inline(always)]
	pub fn convert_i32x4_to_u32x4(self, a: i32x4) -> u32x4 {
		cast!(a)
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

	/// Converts a `u32x4` to `i32x4`, elementwise.
	#[inline(always)]
	pub fn convert_u32x4_to_i32x4(self, a: u32x4) -> i32x4 {
		cast!(a)
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

	/// Divides the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn div_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_div_ps(cast!(a), cast!(b)))
	}

	/// Divides the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn div_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_div_pd(cast!(a), cast!(b)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
	#[inline(always)]
	pub fn floor_f32x4(self, a: f32x4) -> f32x4 {
		cast!(self.sse4_1._mm_floor_ps(cast!(a)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.
	#[inline(always)]
	pub fn floor_f64x2(self, a: f64x2) -> f64x2 {
		cast!(self.sse4_1._mm_floor_pd(cast!(a)))
	}

	/// See [_mm_hadd_ps].
	///
	/// [_mm_hadd_ps]: core::arch::x86_64::_mm_hadd_ps
	#[inline(always)]
	pub fn horizontal_add_pack_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse3._mm_hadd_ps(cast!(a), cast!(b)))
	}

	/// See [_mm_hadd_pd].
	///
	/// [_mm_hadd_pd]: core::arch::x86_64::_mm_hadd_pd
	#[inline(always)]
	pub fn horizontal_add_pack_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse3._mm_hadd_pd(cast!(a), cast!(b)))
	}

	/// See [_mm_hadd_epi16].
	///
	/// [_mm_hadd_epi16]: core::arch::x86_64::_mm_hadd_epi16
	#[inline(always)]
	pub fn horizontal_add_pack_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.ssse3._mm_hadd_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm_hadd_epi32].
	///
	/// [_mm_hadd_epi32]: core::arch::x86_64::_mm_hadd_epi32
	#[inline(always)]
	pub fn horizontal_add_pack_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.ssse3._mm_hadd_epi32(cast!(a), cast!(b)))
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

	/// See [_mm_hsub_ps].
	///
	/// [_mm_hsub_ps]: core::arch::x86_64::_mm_hsub_ps
	#[inline(always)]
	pub fn horizontal_sub_pack_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse3._mm_hsub_ps(cast!(a), cast!(b)))
	}

	/// See [_mm_hsub_pd].
	///
	/// [_mm_hsub_pd]: core::arch::x86_64::_mm_hsub_pd
	#[inline(always)]
	pub fn horizontal_sub_pack_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse3._mm_hsub_pd(cast!(a), cast!(b)))
	}

	/// See [_mm_hsub_epi16].
	///
	/// [_mm_hsub_epi16]: core::arch::x86_64::_mm_hsub_epi16
	#[inline(always)]
	pub fn horizontal_sub_pack_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.ssse3._mm_hsub_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm_hsub_epi32].
	///
	/// [_mm_hsub_epi32]: core::arch::x86_64::_mm_hsub_epi32
	#[inline(always)]
	pub fn horizontal_sub_pack_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.ssse3._mm_hsub_epi32(cast!(a), cast!(b)))
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

	/// Computes `max(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn max_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_max_ps(cast!(a), cast!(b)))
	}

	/// Computes `max(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn max_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_max_pd(cast!(a), cast!(b)))
	}

	/// Computes `max(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn max_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_max_epi16(cast!(a), cast!(b)))
	}

	/// Computes `max(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn max_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.sse4_1._mm_max_epi32(cast!(a), cast!(b)))
	}

	/// Computes `max(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn max_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse4_1._mm_max_epi8(cast!(a), cast!(b)))
	}

	/// Computes `max(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn max_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse4_1._mm_max_epu16(cast!(a), cast!(b)))
	}

	/// Computes `max(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn max_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		cast!(self.sse4_1._mm_max_epu32(cast!(a), cast!(b)))
	}

	/// Computes `max(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn max_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_max_epu8(cast!(a), cast!(b)))
	}

	/// Computes `min(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn min_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_min_ps(cast!(a), cast!(b)))
	}

	/// Computes `min(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn min_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_min_pd(cast!(a), cast!(b)))
	}

	/// Computes `min(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn min_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_min_epi16(cast!(a), cast!(b)))
	}

	/// Computes `min(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn min_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.sse4_1._mm_min_epi32(cast!(a), cast!(b)))
	}

	/// Computes `min(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn min_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse4_1._mm_min_epi8(cast!(a), cast!(b)))
	}

	/// Computes `min(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn min_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse4_1._mm_min_epu16(cast!(a), cast!(b)))
	}

	/// Computes `min(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn min_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		cast!(self.sse4_1._mm_min_epu32(cast!(a), cast!(b)))
	}

	/// Computes `min(a, b)`. for each lane in `a` and `b`.
	#[inline(always)]
	pub fn min_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_min_epu8(cast!(a), cast!(b)))
	}

	/// Computes `a * b` for each lane in `a` and `b`.
	#[inline(always)]
	pub fn mul_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_mul_ps(cast!(a), cast!(b)))
	}

	/// Computes `a * b` for each lane in `a` and `b`.
	#[inline(always)]
	pub fn mul_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_mul_pd(cast!(a), cast!(b)))
	}

	/// See [_mm_maddubs_epi16].
	///
	/// [_mm_maddubs_epi16]: core::arch::x86_64::_mm_maddubs_epi16
	#[inline(always)]
	pub fn multiply_saturating_add_adjacent_i8x16(self, a: i8x16, b: i8x16) -> i16x8 {
		cast!(self.ssse3._mm_maddubs_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm_madd_epi16].
	///
	/// [_mm_madd_epi16]: core::arch::x86_64::_mm_madd_epi16
	#[inline(always)]
	pub fn multiply_wrapping_add_adjacent_i16x8(self, a: i16x8, b: i16x8) -> i32x4 {
		cast!(self.sse2._mm_madd_epi16(cast!(a), cast!(b)))
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

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_or_ps(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_or_pd(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
	}

	/// Returns `a | b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn or_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_or_si128(cast!(a), cast!(b)))
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

	/// See [_mm_packus_epi32].
	///
	/// [_mm_packus_epi32]: core::arch::x86_64::_mm_packus_epi32
	#[inline(always)]
	pub fn pack_with_unsigned_saturation_i32x4(self, a: i32x4, b: i32x4) -> u16x8 {
		cast!(self.sse4_1._mm_packus_epi32(cast!(a), cast!(b)))
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

	/// Adds the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_add_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_adds_epi16(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_add_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse2._mm_adds_epi8(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_add_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_adds_epu16(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_add_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_adds_epu8(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_sub_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_subs_epi16(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_sub_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse2._mm_subs_epi8(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_sub_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_subs_epu16(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with saturation.
	#[inline(always)]
	pub fn saturating_sub_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_subs_epu8(cast!(a), cast!(b)))
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

	/// Calculates `a - b` for each lane in `a` and `b`.
	#[inline(always)]
	pub fn sub_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_sub_ps(cast!(a), cast!(b)))
	}

	/// Calculates `a - b` for each lane in `a` and `b`.
	#[inline(always)]
	pub fn sub_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_sub_pd(cast!(a), cast!(b)))
	}

	/// Alternatively subtracts and adds the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn subadd_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse3._mm_addsub_ps(cast!(a), cast!(b)))
	}

	/// Alternatively subtracts and adds the elements of each lane of `a` and `b`.
	#[inline(always)]
	pub fn subadd_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse3._mm_addsub_pd(cast!(a), cast!(b)))
	}

	/// See [_mm_sad_epu8].
	///
	/// [_mm_sad_epu8]: core::arch::x86_64::_mm_sad_epu8
	#[inline(always)]
	pub fn sum_of_absolute_differences_u8x16(self, a: u8x16, b: u8x16) -> u64x2 {
		cast!(self.sse2._mm_sad_epu8(cast!(a), cast!(b)))
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

	/// Computes the unsigned absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn unsigned_abs_i16x8(self, a: i16x8) -> u16x8 {
		cast!(self.ssse3._mm_abs_epi16(cast!(a)))
	}

	/// Computes the unsigned absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn unsigned_abs_i32x4(self, a: i32x4) -> u32x4 {
		cast!(self.ssse3._mm_abs_epi32(cast!(a)))
	}

	/// Computes the unsigned absolute value of the elements of each lane of `a`.
	#[inline(always)]
	pub fn unsigned_abs_i8x16(self, a: i8x16) -> u8x16 {
		cast!(self.ssse3._mm_abs_epi8(cast!(a)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_i16x8(self, a: i16x8, b: i16x8) -> (i16x8, i16x8) {
		(
			cast!(self.sse2._mm_mullo_epi16(cast!(a), cast!(b))),
			cast!(self.sse2._mm_mulhi_epi16(cast!(a), cast!(b))),
		)
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_i32x4(self, a: i32x4, b: i32x4) -> (i32x4, i32x4) {
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
	pub fn widening_mul_u16x8(self, a: u16x8, b: u16x8) -> (u16x8, u16x8) {
		(
			cast!(self.sse2._mm_mullo_epi16(cast!(a), cast!(b))),
			cast!(self.sse2._mm_mulhi_epu16(cast!(a), cast!(b))),
		)
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

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_add_epi16(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.sse2._mm_add_epi32(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		cast!(self.sse2._mm_add_epi64(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse2._mm_add_epi8(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_add_epi16(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		cast!(self.sse2._mm_add_epi32(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		cast!(self.sse2._mm_add_epi64(cast!(a), cast!(b)))
	}

	/// Adds the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_add_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_add_epi8(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_mullo_epi16(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.sse4_1._mm_mullo_epi32(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_mullo_epi16(cast!(a), cast!(b)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_mul_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		cast!(self.sse4_1._mm_mullo_epi32(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_sub_epi16(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.sse2._mm_sub_epi32(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		cast!(self.sse2._mm_sub_epi64(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse2._mm_sub_epi8(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_sub_epi16(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		cast!(self.sse2._mm_sub_epi32(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		cast!(self.sse2._mm_sub_epi64(cast!(a), cast!(b)))
	}

	/// Subtracts the elements of each lane of `a` and `b`, with wrapping on overflow.
	#[inline(always)]
	pub fn wrapping_sub_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_sub_epi8(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_f32x4(self, a: f32x4, b: f32x4) -> f32x4 {
		cast!(self.sse._mm_xor_ps(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_f64x2(self, a: f64x2, b: f64x2) -> f64x2 {
		cast!(self.sse2._mm_xor_pd(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_i16x8(self, a: i16x8, b: i16x8) -> i16x8 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_i32x4(self, a: i32x4, b: i32x4) -> i32x4 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_i64x2(self, a: i64x2, b: i64x2) -> i64x2 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_i8x16(self, a: i8x16, b: i8x16) -> i8x16 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_m16x8(self, a: m16x8, b: m16x8) -> m16x8 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_m32x4(self, a: m32x4, b: m32x4) -> m32x4 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_m64x2(self, a: m64x2, b: m64x2) -> m64x2 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_m8x16(self, a: m8x16, b: m8x16) -> m8x16 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_u16x8(self, a: u16x8, b: u16x8) -> u16x8 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_u32x4(self, a: u32x4, b: u32x4) -> u32x4 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_u64x2(self, a: u64x2, b: u64x2) -> u64x2 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}

	/// Returns `a ^ b` for each bit in `a` and `b`.
	#[inline(always)]
	pub fn xor_u8x16(self, a: u8x16, b: u8x16) -> u8x16 {
		cast!(self.sse2._mm_xor_si128(cast!(a), cast!(b)))
	}
}
