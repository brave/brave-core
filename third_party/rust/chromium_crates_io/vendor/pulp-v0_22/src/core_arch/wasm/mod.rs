use super::arch;
use arch::*;

#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct Simd128 {
	__private: (),
}

#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct RelaxedSimd {
	__private: (),
}

impl core::fmt::Debug for Simd128 {
	#[inline]
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> core::fmt::Result {
		f.write_str("Simd128")
	}
}

impl core::fmt::Debug for RelaxedSimd {
	#[inline]
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> core::fmt::Result {
		f.write_str("RelaxedSimd")
	}
}

impl Simd128 {
	#[inline(always)]
	/// requires the corresponding feature
	pub const fn new_unchecked() -> Self {
		Self { __private: () }
	}

	#[inline(always)]
	pub fn try_new() -> Option<Self> {
		if feature_detected!("simd128") {
			Some(Self { __private: () })
		} else {
			None
		}
	}

	#[inline(always)]
	pub fn is_available() -> bool {
		feature_detected!("simd128")
	}
}

impl RelaxedSimd {
	#[inline(always)]
	/// requires the corresponding feature
	pub const fn new_unchecked() -> Self {
		Self { __private: () }
	}

	#[inline(always)]
	pub fn try_new() -> Option<Self> {
		if feature_detected!("relaxed-simd") {
			Some(Self { __private: () })
		} else {
			None
		}
	}

	#[inline(always)]
	pub fn is_available() -> bool {
		feature_detected!("relaxed-simd")
	}
}

impl Simd128 {
	delegate!({
		unsafe fn v128_load(m: *const v128) -> v128;
		unsafe fn i16x8_load_extend_i8x8(m: *const i8) -> v128;
		unsafe fn i16x8_load_extend_u8x8(m: *const u8) -> v128;
		unsafe fn i32x4_load_extend_i16x4(m: *const i16) -> v128;
		unsafe fn i32x4_load_extend_u16x4(m: *const u16) -> v128;
		unsafe fn i64x2_load_extend_i32x2(m: *const i32) -> v128;
		unsafe fn i64x2_load_extend_u32x2(m: *const u32) -> v128;
		unsafe fn v128_load8_splat(m: *const u8) -> v128;
		unsafe fn v128_load16_splat(m: *const u16) -> v128;
		unsafe fn v128_load32_splat(m: *const u32) -> v128;
		unsafe fn v128_load64_splat(m: *const u64) -> v128;
		unsafe fn v128_load32_zero(m: *const u32) -> v128;
		unsafe fn v128_load64_zero(m: *const u64) -> v128;
		unsafe fn v128_store(m: *mut v128, a: v128);
		unsafe fn v128_load8_lane<const L: usize>(v: v128, m: *const u8) -> v128;
		unsafe fn v128_load16_lane<const L: usize>(v: v128, m: *const u16) -> v128;
		unsafe fn v128_load32_lane<const L: usize>(v: v128, m: *const u32) -> v128;
		unsafe fn v128_load64_lane<const L: usize>(v: v128, m: *const u64) -> v128;
		unsafe fn v128_store8_lane<const L: usize>(v: v128, m: *mut u8);
		unsafe fn v128_store16_lane<const L: usize>(v: v128, m: *mut u16);
		unsafe fn v128_store32_lane<const L: usize>(v: v128, m: *mut u32);
		unsafe fn v128_store64_lane<const L: usize>(v: v128, m: *mut u64);
		const fn i8x16(
			a0: i8,
			a1: i8,
			a2: i8,
			a3: i8,
			a4: i8,
			a5: i8,
			a6: i8,
			a7: i8,
			a8: i8,
			a9: i8,
			a10: i8,
			a11: i8,
			a12: i8,
			a13: i8,
			a14: i8,
			a15: i8,
		) -> v128;
		const fn u8x16(
			a0: u8,
			a1: u8,
			a2: u8,
			a3: u8,
			a4: u8,
			a5: u8,
			a6: u8,
			a7: u8,
			a8: u8,
			a9: u8,
			a10: u8,
			a11: u8,
			a12: u8,
			a13: u8,
			a14: u8,
			a15: u8,
		) -> v128;
		const fn i16x8(
			a0: i16,
			a1: i16,
			a2: i16,
			a3: i16,
			a4: i16,
			a5: i16,
			a6: i16,
			a7: i16,
		) -> v128;
		const fn u16x8(
			a0: u16,
			a1: u16,
			a2: u16,
			a3: u16,
			a4: u16,
			a5: u16,
			a6: u16,
			a7: u16,
		) -> v128;
		const fn i32x4(a0: i32, a1: i32, a2: i32, a3: i32) -> v128;
		const fn u32x4(a0: u32, a1: u32, a2: u32, a3: u32) -> v128;
		const fn i64x2(a0: i64, a1: i64) -> v128;
		const fn u64x2(a0: u64, a1: u64) -> v128;
		const fn f32x4(a0: f32, a1: f32, a2: f32, a3: f32) -> v128;
		const fn f64x2(a0: f64, a1: f64) -> v128;
		fn i8x16_shuffle<
			const I0: usize,
			const I1: usize,
			const I2: usize,
			const I3: usize,
			const I4: usize,
			const I5: usize,
			const I6: usize,
			const I7: usize,
			const I8: usize,
			const I9: usize,
			const I10: usize,
			const I11: usize,
			const I12: usize,
			const I13: usize,
			const I14: usize,
			const I15: usize,
		>(
			a: v128,
			b: v128,
		) -> v128;
		fn i16x8_shuffle<
			const I0: usize,
			const I1: usize,
			const I2: usize,
			const I3: usize,
			const I4: usize,
			const I5: usize,
			const I6: usize,
			const I7: usize,
		>(
			a: v128,
			b: v128,
		) -> v128;
		fn i32x4_shuffle<const I0: usize, const I1: usize, const I2: usize, const I3: usize>(
			a: v128,
			b: v128,
		) -> v128;
		fn u32x4_shuffle<const I0: usize, const I1: usize, const I2: usize, const I3: usize>(
			a: v128,
			b: v128,
		) -> v128;
		fn i64x2_shuffle<const I0: usize, const I1: usize>(a: v128, b: v128) -> v128;
		fn u64x2_shuffle<const I0: usize, const I1: usize>(a: v128, b: v128) -> v128;
		fn i8x16_extract_lane<const N: usize>(a: v128) -> i8;
		fn u8x16_extract_lane<const N: usize>(a: v128) -> u8;
		fn i8x16_replace_lane<const N: usize>(a: v128, val: i8) -> v128;
		fn u8x16_replace_lane<const N: usize>(a: v128, val: u8) -> v128;
		fn i16x8_extract_lane<const N: usize>(a: v128) -> i16;
		fn u16x8_extract_lane<const N: usize>(a: v128) -> u16;
		fn i16x8_replace_lane<const N: usize>(a: v128, val: i16) -> v128;
		fn u16x8_replace_lane<const N: usize>(a: v128, val: u16) -> v128;
		fn i32x4_extract_lane<const N: usize>(a: v128) -> i32;
		fn u32x4_extract_lane<const N: usize>(a: v128) -> u32;
		fn i32x4_replace_lane<const N: usize>(a: v128, val: i32) -> v128;
		fn u32x4_replace_lane<const N: usize>(a: v128, val: u32) -> v128;
		fn i64x2_extract_lane<const N: usize>(a: v128) -> i64;
		fn u64x2_extract_lane<const N: usize>(a: v128) -> u64;
		fn i64x2_replace_lane<const N: usize>(a: v128, val: i64) -> v128;
		fn u64x2_replace_lane<const N: usize>(a: v128, val: u64) -> v128;
		fn f32x4_extract_lane<const N: usize>(a: v128) -> f32;
		fn f32x4_replace_lane<const N: usize>(a: v128, val: f32) -> v128;
		fn f64x2_extract_lane<const N: usize>(a: v128) -> f64;
		fn f64x2_replace_lane<const N: usize>(a: v128, val: f64) -> v128;
		fn i8x16_swizzle(a: v128, s: v128) -> v128;
		fn i8x16_splat(a: i8) -> v128;
		fn u8x16_splat(a: u8) -> v128;
		fn i16x8_splat(a: i16) -> v128;
		fn u16x8_splat(a: u16) -> v128;
		fn i32x4_splat(a: i32) -> v128;
		fn u32x4_splat(a: u32) -> v128;
		fn i64x2_splat(a: i64) -> v128;
		fn u64x2_splat(a: u64) -> v128;
		fn f32x4_splat(a: f32) -> v128;
		fn f64x2_splat(a: f64) -> v128;
		fn i8x16_eq(a: v128, b: v128) -> v128;
		fn i8x16_ne(a: v128, b: v128) -> v128;
		fn u8x16_eq(a: v128, b: v128) -> v128;
		fn u8x16_ne(a: v128, b: v128) -> v128;
		fn i8x16_lt(a: v128, b: v128) -> v128;
		fn u8x16_lt(a: v128, b: v128) -> v128;
		fn i8x16_gt(a: v128, b: v128) -> v128;
		fn u8x16_gt(a: v128, b: v128) -> v128;
		fn i8x16_le(a: v128, b: v128) -> v128;
		fn u8x16_le(a: v128, b: v128) -> v128;
		fn i8x16_ge(a: v128, b: v128) -> v128;
		fn u8x16_ge(a: v128, b: v128) -> v128;
		fn i16x8_eq(a: v128, b: v128) -> v128;
		fn i16x8_ne(a: v128, b: v128) -> v128;
		fn u16x8_eq(a: v128, b: v128) -> v128;
		fn u16x8_ne(a: v128, b: v128) -> v128;
		fn i16x8_lt(a: v128, b: v128) -> v128;
		fn u16x8_lt(a: v128, b: v128) -> v128;
		fn i16x8_gt(a: v128, b: v128) -> v128;
		fn u16x8_gt(a: v128, b: v128) -> v128;
		fn i16x8_le(a: v128, b: v128) -> v128;
		fn u16x8_le(a: v128, b: v128) -> v128;
		fn i16x8_ge(a: v128, b: v128) -> v128;
		fn u16x8_ge(a: v128, b: v128) -> v128;
		fn i32x4_eq(a: v128, b: v128) -> v128;
		fn i32x4_ne(a: v128, b: v128) -> v128;
		fn u32x4_eq(a: v128, b: v128) -> v128;
		fn u32x4_ne(a: v128, b: v128) -> v128;
		fn i32x4_lt(a: v128, b: v128) -> v128;
		fn u32x4_lt(a: v128, b: v128) -> v128;
		fn i32x4_gt(a: v128, b: v128) -> v128;
		fn u32x4_gt(a: v128, b: v128) -> v128;
		fn i32x4_le(a: v128, b: v128) -> v128;
		fn u32x4_le(a: v128, b: v128) -> v128;
		fn i32x4_ge(a: v128, b: v128) -> v128;
		fn u32x4_ge(a: v128, b: v128) -> v128;
		fn i64x2_eq(a: v128, b: v128) -> v128;
		fn i64x2_ne(a: v128, b: v128) -> v128;
		fn u64x2_eq(a: v128, b: v128) -> v128;
		fn u64x2_ne(a: v128, b: v128) -> v128;
		fn i64x2_lt(a: v128, b: v128) -> v128;
		fn i64x2_gt(a: v128, b: v128) -> v128;
		fn i64x2_le(a: v128, b: v128) -> v128;
		fn i64x2_ge(a: v128, b: v128) -> v128;
		fn f32x4_eq(a: v128, b: v128) -> v128;
		fn f32x4_ne(a: v128, b: v128) -> v128;
		fn f32x4_lt(a: v128, b: v128) -> v128;
		fn f32x4_gt(a: v128, b: v128) -> v128;
		fn f32x4_le(a: v128, b: v128) -> v128;
		fn f32x4_ge(a: v128, b: v128) -> v128;
		fn f64x2_eq(a: v128, b: v128) -> v128;
		fn f64x2_ne(a: v128, b: v128) -> v128;
		fn f64x2_lt(a: v128, b: v128) -> v128;
		fn f64x2_gt(a: v128, b: v128) -> v128;
		fn f64x2_le(a: v128, b: v128) -> v128;
		fn f64x2_ge(a: v128, b: v128) -> v128;
		fn v128_not(a: v128) -> v128;
		fn v128_and(a: v128, b: v128) -> v128;
		fn v128_andnot(a: v128, b: v128) -> v128;
		fn v128_or(a: v128, b: v128) -> v128;
		fn v128_xor(a: v128, b: v128) -> v128;
		fn v128_bitselect(v1: v128, v2: v128, c: v128) -> v128;
		fn v128_any_true(a: v128) -> bool;
		fn i8x16_abs(a: v128) -> v128;
		fn i8x16_neg(a: v128) -> v128;
		fn i8x16_popcnt(v: v128) -> v128;
		fn i8x16_all_true(a: v128) -> bool;
		fn i8x16_bitmask(a: v128) -> u16;
		fn i8x16_narrow_i16x8(a: v128, b: v128) -> v128;
		fn u8x16_narrow_i16x8(a: v128, b: v128) -> v128;
		fn i8x16_shl(a: v128, amt: u32) -> v128;
		fn i8x16_shr(a: v128, amt: u32) -> v128;
		fn u8x16_shr(a: v128, amt: u32) -> v128;
		fn i8x16_add(a: v128, b: v128) -> v128;
		fn i8x16_sub(a: v128, b: v128) -> v128;
		fn u8x16_add(a: v128, b: v128) -> v128;
		fn u8x16_sub(a: v128, b: v128) -> v128;
		fn i8x16_add_sat(a: v128, b: v128) -> v128;
		fn u8x16_add_sat(a: v128, b: v128) -> v128;
		fn i8x16_sub_sat(a: v128, b: v128) -> v128;
		fn u8x16_sub_sat(a: v128, b: v128) -> v128;
		fn i8x16_min(a: v128, b: v128) -> v128;
		fn u8x16_min(a: v128, b: v128) -> v128;
		fn i8x16_max(a: v128, b: v128) -> v128;
		fn u8x16_max(a: v128, b: v128) -> v128;
		fn u8x16_avgr(a: v128, b: v128) -> v128;
		fn i16x8_extadd_pairwise_i8x16(a: v128) -> v128;
		fn i16x8_extadd_pairwise_u8x16(a: v128) -> v128;
		fn i16x8_abs(a: v128) -> v128;
		fn i16x8_neg(a: v128) -> v128;
		fn i16x8_q15mulr_sat(a: v128, b: v128) -> v128;
		fn i16x8_all_true(a: v128) -> bool;
		fn i16x8_bitmask(a: v128) -> u8;
		fn i16x8_narrow_i32x4(a: v128, b: v128) -> v128;
		fn u16x8_narrow_i32x4(a: v128, b: v128) -> v128;
		fn i16x8_extend_low_i8x16(a: v128) -> v128;
		fn i16x8_extend_high_i8x16(a: v128) -> v128;
		fn i16x8_extend_low_u8x16(a: v128) -> v128;
		fn i16x8_extend_high_u8x16(a: v128) -> v128;
		fn i16x8_shl(a: v128, amt: u32) -> v128;
		fn i16x8_shr(a: v128, amt: u32) -> v128;
		fn u16x8_shr(a: v128, amt: u32) -> v128;
		fn i16x8_add(a: v128, b: v128) -> v128;
		fn i16x8_sub(a: v128, b: v128) -> v128;
		fn u16x8_add(a: v128, b: v128) -> v128;
		fn u16x8_sub(a: v128, b: v128) -> v128;
		fn i16x8_add_sat(a: v128, b: v128) -> v128;
		fn u16x8_add_sat(a: v128, b: v128) -> v128;
		fn i16x8_sub_sat(a: v128, b: v128) -> v128;
		fn u16x8_sub_sat(a: v128, b: v128) -> v128;
		fn u16x8_mul(a: v128, b: v128) -> v128;
		fn i16x8_mul(a: v128, b: v128) -> v128;
		fn i16x8_min(a: v128, b: v128) -> v128;
		fn u16x8_min(a: v128, b: v128) -> v128;
		fn i16x8_max(a: v128, b: v128) -> v128;
		fn u16x8_max(a: v128, b: v128) -> v128;
		fn u16x8_avgr(a: v128, b: v128) -> v128;
		fn i16x8_extmul_low_i8x16(a: v128, b: v128) -> v128;
		fn i16x8_extmul_high_i8x16(a: v128, b: v128) -> v128;
		fn i16x8_extmul_low_u8x16(a: v128, b: v128) -> v128;
		fn i16x8_extmul_high_u8x16(a: v128, b: v128) -> v128;
		fn i32x4_extadd_pairwise_i16x8(a: v128) -> v128;
		fn i32x4_extadd_pairwise_u16x8(a: v128) -> v128;
		fn i32x4_abs(a: v128) -> v128;
		fn i32x4_neg(a: v128) -> v128;
		fn i32x4_all_true(a: v128) -> bool;
		fn i32x4_bitmask(a: v128) -> u8;
		fn i32x4_extend_low_i16x8(a: v128) -> v128;
		fn i32x4_extend_high_i16x8(a: v128) -> v128;
		fn i32x4_extend_low_u16x8(a: v128) -> v128;
		fn i32x4_extend_high_u16x8(a: v128) -> v128;
		fn i32x4_shl(a: v128, amt: u32) -> v128;
		fn i32x4_shr(a: v128, amt: u32) -> v128;
		fn u32x4_shr(a: v128, amt: u32) -> v128;
		fn i32x4_add(a: v128, b: v128) -> v128;
		fn i32x4_sub(a: v128, b: v128) -> v128;
		fn u32x4_add(a: v128, b: v128) -> v128;
		fn u32x4_sub(a: v128, b: v128) -> v128;
		fn i32x4_mul(a: v128, b: v128) -> v128;
		fn u32x4_mul(a: v128, b: v128) -> v128;
		fn i32x4_min(a: v128, b: v128) -> v128;
		fn u32x4_min(a: v128, b: v128) -> v128;
		fn i32x4_max(a: v128, b: v128) -> v128;
		fn u32x4_max(a: v128, b: v128) -> v128;
		fn i32x4_dot_i16x8(a: v128, b: v128) -> v128;
		fn i32x4_extmul_low_i16x8(a: v128, b: v128) -> v128;
		fn i32x4_extmul_high_i16x8(a: v128, b: v128) -> v128;
		fn i32x4_extmul_low_u16x8(a: v128, b: v128) -> v128;
		fn i32x4_extmul_high_u16x8(a: v128, b: v128) -> v128;
		fn i64x2_abs(a: v128) -> v128;
		fn i64x2_neg(a: v128) -> v128;
		fn i64x2_all_true(a: v128) -> bool;
		fn i64x2_bitmask(a: v128) -> u8;
		fn i64x2_extend_low_i32x4(a: v128) -> v128;
		fn i64x2_extend_high_i32x4(a: v128) -> v128;
		fn i64x2_extend_low_u32x4(a: v128) -> v128;
		fn i64x2_extend_high_u32x4(a: v128) -> v128;
		fn i64x2_shl(a: v128, amt: u32) -> v128;
		fn i64x2_shr(a: v128, amt: u32) -> v128;
		fn u64x2_shr(a: v128, amt: u32) -> v128;
		fn i64x2_add(a: v128, b: v128) -> v128;
		fn i64x2_sub(a: v128, b: v128) -> v128;
		fn u64x2_add(a: v128, b: v128) -> v128;
		fn u64x2_sub(a: v128, b: v128) -> v128;
		fn i64x2_mul(a: v128, b: v128) -> v128;
		fn i64x2_extmul_low_i32x4(a: v128, b: v128) -> v128;
		fn i64x2_extmul_high_i32x4(a: v128, b: v128) -> v128;
		fn i64x2_extmul_low_u32x4(a: v128, b: v128) -> v128;
		fn i64x2_extmul_high_u32x4(a: v128, b: v128) -> v128;
		fn f32x4_ceil(a: v128) -> v128;
		fn f32x4_floor(a: v128) -> v128;
		fn f32x4_trunc(a: v128) -> v128;
		fn f32x4_nearest(a: v128) -> v128;
		fn f32x4_abs(a: v128) -> v128;
		fn f32x4_neg(a: v128) -> v128;
		fn f32x4_sqrt(a: v128) -> v128;
		fn f32x4_add(a: v128, b: v128) -> v128;
		fn f32x4_sub(a: v128, b: v128) -> v128;
		fn f32x4_mul(a: v128, b: v128) -> v128;
		fn f32x4_div(a: v128, b: v128) -> v128;
		fn f32x4_min(a: v128, b: v128) -> v128;
		fn f32x4_max(a: v128, b: v128) -> v128;
		fn f32x4_pmin(a: v128, b: v128) -> v128;
		fn f32x4_pmax(a: v128, b: v128) -> v128;
		fn f64x2_ceil(a: v128) -> v128;
		fn f64x2_floor(a: v128) -> v128;
		fn f64x2_trunc(a: v128) -> v128;
		fn f64x2_nearest(a: v128) -> v128;
		fn f64x2_abs(a: v128) -> v128;
		fn f64x2_neg(a: v128) -> v128;
		fn f64x2_sqrt(a: v128) -> v128;
		fn f64x2_add(a: v128, b: v128) -> v128;
		fn f64x2_sub(a: v128, b: v128) -> v128;
		fn f64x2_mul(a: v128, b: v128) -> v128;
		fn f64x2_div(a: v128, b: v128) -> v128;
		fn f64x2_min(a: v128, b: v128) -> v128;
		fn f64x2_max(a: v128, b: v128) -> v128;
		fn f64x2_pmin(a: v128, b: v128) -> v128;
		fn f64x2_pmax(a: v128, b: v128) -> v128;
		fn i32x4_trunc_sat_f32x4(a: v128) -> v128;
		fn u32x4_trunc_sat_f32x4(a: v128) -> v128;
		fn f32x4_convert_i32x4(a: v128) -> v128;
		fn f32x4_convert_u32x4(a: v128) -> v128;
		fn i32x4_trunc_sat_f64x2_zero(a: v128) -> v128;
		fn u32x4_trunc_sat_f64x2_zero(a: v128) -> v128;
		fn f64x2_convert_low_i32x4(a: v128) -> v128;
		fn f64x2_convert_low_u32x4(a: v128) -> v128;
		fn f32x4_demote_f64x2_zero(a: v128) -> v128;
		fn f64x2_promote_low_f32x4(a: v128) -> v128;
	});
}

impl RelaxedSimd {
	delegate!({
		fn i8x16_relaxed_swizzle(a: v128, s: v128) -> v128;
		fn i32x4_relaxed_trunc_f32x4(a: v128) -> v128;
		fn u32x4_relaxed_trunc_f32x4(a: v128) -> v128;
		fn i32x4_relaxed_trunc_f64x2_zero(a: v128) -> v128;
		fn u32x4_relaxed_trunc_f64x2_zero(a: v128) -> v128;
		fn f32x4_relaxed_madd(a: v128, b: v128, c: v128) -> v128;
		fn f32x4_relaxed_nmadd(a: v128, b: v128, c: v128) -> v128;
		fn f64x2_relaxed_madd(a: v128, b: v128, c: v128) -> v128;
		fn f64x2_relaxed_nmadd(a: v128, b: v128, c: v128) -> v128;
		fn i8x16_relaxed_laneselect(a: v128, b: v128, m: v128) -> v128;
		fn i16x8_relaxed_laneselect(a: v128, b: v128, m: v128) -> v128;
		fn i32x4_relaxed_laneselect(a: v128, b: v128, m: v128) -> v128;
		fn i64x2_relaxed_laneselect(a: v128, b: v128, m: v128) -> v128;
		fn f32x4_relaxed_min(a: v128, b: v128) -> v128;
		fn f32x4_relaxed_max(a: v128, b: v128) -> v128;
		fn f64x2_relaxed_min(a: v128, b: v128) -> v128;
		fn f64x2_relaxed_max(a: v128, b: v128) -> v128;
		fn i16x8_relaxed_q15mulr(a: v128, b: v128) -> v128;
		fn i16x8_relaxed_dot_i8x16_i7x16(a: v128, b: v128) -> v128;
		fn i32x4_relaxed_dot_i8x16_i7x16_add(a: v128, b: v128, c: v128) -> v128;
	});
}
