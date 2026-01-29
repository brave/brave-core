use super::*;

// https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels
simd_type!({
	/// AVX instruction set.
	///
	/// Notable additions over [`V2`] include:
	///  - Instructions operating on 256-bit SIMD vectors.
	///  - Shift functions with a separate shift per lane, such as [`V3::shl_dyn_u32x4`].
	///  - Fused multiply-accumulate instructions, such as [`V3::mul_add_f32x4`].
	#[allow(missing_docs)]
	pub struct V3 {
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
	}
});

// copied from the standard library
#[inline(always)]
fn avx2_pshufb(simd: V3, bytes: __m256i, idxs: __m256i) -> __m256i {
	let mid = simd.avx._mm256_set1_epi8(16i8);
	let high = simd.avx._mm256_set1_epi8(32i8);
	// This is ordering sensitive, and LLVM will order these how you put them.
	// Most AVX2 impls use ~5 "ports", and only 1 or 2 are capable of permutes.
	// But the "compose" step will lower to ops that can also use at least 1 other port.
	// So this tries to break up permutes so composition flows through "open" ports.
	// Comparative benches should be done on multiple AVX2 CPUs before reordering this

	let hihi = simd.avx2._mm256_permute2x128_si256::<0x11>(bytes, bytes);
	let hi_shuf = simd.avx2._mm256_shuffle_epi8(
		hihi, // duplicate the vector's top half
		idxs, // so that using only 4 bits of an index still picks bytes 16-31
	);
	// A zero-fill during the compose step gives the "all-Neon-like" OOB-is-0 semantics
	let compose = simd.avx2._mm256_blendv_epi8(
		simd.avx._mm256_set1_epi8(0),
		hi_shuf,
		simd.avx2._mm256_cmpgt_epi8(high, idxs),
	);
	let lolo = simd.avx2._mm256_permute2x128_si256::<0x00>(bytes, bytes);
	let lo_shuf = simd.avx2._mm256_shuffle_epi8(lolo, idxs);
	// Repeat, then pick indices < 16, overwriting indices 0-15 from previous compose step
	simd.avx2
		._mm256_blendv_epi8(compose, lo_shuf, simd.avx2._mm256_cmpgt_epi8(mid, idxs))
}

static AVX2_ROTATE_IDX: [u8x32; 32] = [
	u8x32(
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
		25, 26, 27, 28, 29, 30, 31,
	),
	u8x32(
		31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
		24, 25, 26, 27, 28, 29, 30,
	),
	u8x32(
		30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
		23, 24, 25, 26, 27, 28, 29,
	),
	u8x32(
		29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
		22, 23, 24, 25, 26, 27, 28,
	),
	u8x32(
		28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
		21, 22, 23, 24, 25, 26, 27,
	),
	u8x32(
		27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26,
	),
	u8x32(
		26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
		19, 20, 21, 22, 23, 24, 25,
	),
	u8x32(
		25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
		18, 19, 20, 21, 22, 23, 24,
	),
	u8x32(
		24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		17, 18, 19, 20, 21, 22, 23,
	),
	u8x32(
		23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22,
	),
	u8x32(
		22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21,
	),
	u8x32(
		21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
		14, 15, 16, 17, 18, 19, 20,
	),
	u8x32(
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
		13, 14, 15, 16, 17, 18, 19,
	),
	u8x32(
		19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
		12, 13, 14, 15, 16, 17, 18,
	),
	u8x32(
		18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
		11, 12, 13, 14, 15, 16, 17,
	),
	u8x32(
		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		10, 11, 12, 13, 14, 15, 16,
	),
	u8x32(
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8,
		9, 10, 11, 12, 13, 14, 15,
	),
	u8x32(
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14,
	),
	u8x32(
		14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5,
		6, 7, 8, 9, 10, 11, 12, 13,
	),
	u8x32(
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4,
		5, 6, 7, 8, 9, 10, 11, 12,
	),
	u8x32(
		12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3,
		4, 5, 6, 7, 8, 9, 10, 11,
	),
	u8x32(
		11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1,
		2, 3, 4, 5, 6, 7, 8, 9, 10,
	),
	u8x32(
		10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0,
		1, 2, 3, 4, 5, 6, 7, 8, 9,
	),
	u8x32(
		9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		0, 1, 2, 3, 4, 5, 6, 7, 8,
	),
	u8x32(
		8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		31, 0, 1, 2, 3, 4, 5, 6, 7,
	),
	u8x32(
		7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		30, 31, 0, 1, 2, 3, 4, 5, 6,
	),
	u8x32(
		6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		30, 31, 0, 1, 2, 3, 4, 5,
	),
	u8x32(
		5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
		29, 30, 31, 0, 1, 2, 3, 4,
	),
	u8x32(
		4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
		28, 29, 30, 31, 0, 1, 2, 3,
	),
	u8x32(
		3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
		27, 28, 29, 30, 31, 0, 1, 2,
	),
	u8x32(
		2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
		27, 28, 29, 30, 31, 0, 1,
	),
	u8x32(
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
		26, 27, 28, 29, 30, 31, 0,
	),
];

static AVX2_128_ROTATE_IDX: [u8x16; 16] = [
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

static V3_U32_MASKS: [u32x8; 9] = [
	u32x8(0, 0, 0, 0, 0, 0, 0, 0),
	u32x8(!0, 0, 0, 0, 0, 0, 0, 0),
	u32x8(!0, !0, 0, 0, 0, 0, 0, 0),
	u32x8(!0, !0, !0, 0, 0, 0, 0, 0),
	u32x8(!0, !0, !0, !0, 0, 0, 0, 0),
	u32x8(!0, !0, !0, !0, !0, 0, 0, 0),
	u32x8(!0, !0, !0, !0, !0, !0, 0, 0),
	u32x8(!0, !0, !0, !0, !0, !0, !0, 0),
	u32x8(!0, !0, !0, !0, !0, !0, !0, !0),
];
static V3_U32_LAST_MASKS: [u32x8; 9] = [
	u32x8(0, 0, 0, 0, 0, 0, 0, 0),
	u32x8(0, 0, 0, 0, 0, 0, 0, !0),
	u32x8(0, 0, 0, 0, 0, 0, !0, !0),
	u32x8(0, 0, 0, 0, 0, !0, !0, !0),
	u32x8(0, 0, 0, 0, !0, !0, !0, !0),
	u32x8(0, 0, 0, !0, !0, !0, !0, !0),
	u32x8(0, 0, !0, !0, !0, !0, !0, !0),
	u32x8(0, !0, !0, !0, !0, !0, !0, !0),
	u32x8(!0, !0, !0, !0, !0, !0, !0, !0),
];

impl Seal for V3 {}
impl Seal for V3_Scalar {}

#[derive(Copy, Clone, Debug)]
#[repr(transparent)]
pub struct V3_Scalar(pub V3);

#[cfg(target_arch = "x86_64")]
#[inline(always)]
pub(super) fn avx_load_u32s(simd: Avx2, slice: &[u32]) -> u32x8 {
	_ = simd;
	unsafe { avx_ld_u32s(slice.as_ptr(), LD_ST[2 * (16 * slice.len().min(8))]) }
}

#[cfg(target_arch = "x86_64")]
#[inline(always)]
pub(super) fn avx_store_u32s(simd: Avx2, slice: &mut [u32], value: u32x8) {
	_ = simd;
	unsafe {
		avx_st_u32s(
			slice.as_mut_ptr(),
			value,
			LD_ST[2 * (16 * slice.len().min(8)) + 1],
		);
	}
}

impl core::ops::Deref for V3 {
	type Target = V2;

	#[inline(always)]
	fn deref(&self) -> &Self::Target {
		V2 {
			sse: self.sse,
			sse2: self.sse2,
			fxsr: self.fxsr,
			sse3: self.sse3,
			ssse3: self.ssse3,
			sse4_1: self.sse4_1,
			sse4_2: self.sse4_2,
			popcnt: self.popcnt,
		}
		.to_ref()
	}
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
				Scalar256b.[<$func _ $ty s>](a, b)
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

impl Simd for V3 {
	type c32s = f32x8;
	type c64s = f64x4;
	type f32s = f32x8;
	type f64s = f64x4;
	type i16s = i16x16;
	type i32s = i32x8;
	type i64s = i64x4;
	type i8s = i8x32;
	type m16s = m16x16;
	type m32s = m32x8;
	type m64s = m64x4;
	type m8s = m8x32;
	type u16s = u16x16;
	type u32s = u32x8;
	type u64s = u64x4;
	type u8s = u8x32;

	const REGISTER_COUNT: usize = 16;

	impl_simd_binop!(add, f32 x 8, f64 x 4);

	impl_simd_binop!(add, wrapping_add, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, u64 x 4, i64 x 4);

	impl_simd_binop!(sub, f32 x 8, f64 x 4);

	impl_simd_binop!(sub, wrapping_sub, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, u64 x 4, i64 x 4);

	impl_simd_binop!(mul, f32 x 8, f64 x 4);

	impl_simd_binop!(mul, wrapping_mul, u16 x 16, i16 x 16, u32 x 8, i32 x 8);

	impl_scalar_binop!(mul, u64, i64);

	impl_simd_binop!(and, m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4, f32 x 8, f64 x 4);

	impl_simd_binop!(or, m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4, f32 x 8, f64 x 4);

	impl_simd_binop!(xor, m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4, f32 x 8, f64 x 4);

	impl_simd_binop!(div, f32 x 8, f64 x 4);

	impl_simd_binop!(equal, cmp_eq, u8 x 32 => m8, u16 x 16 => m16, u32 x 8 => m32, u64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	impl_simd_binop!(greater_than, cmp_gt, u8 x 32 => m8, i8 x 32 => m8, u16 x 16 => m16, i16 x 16 => m16, u32 x 8 => m32, i32 x 8 => m32, u64 x 4 => m64, i64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	impl_simd_binop!(greater_than_or_equal, cmp_ge, u8 x 32 => m8, i8 x 32 => m8, u16 x 16 => m16, i16 x 16 => m16, u32 x 8 => m32, i32 x 8 => m32, u64 x 4 => m64, i64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	impl_simd_binop!(less_than, cmp_lt, u8 x 32 => m8, i8 x 32 => m8, u16 x 16 => m16, i16 x 16 => m16, u32 x 8 => m32, i32 x 8 => m32, u64 x 4 => m64, i64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	impl_simd_binop!(less_than_or_equal, cmp_le, u8 x 32 => m8, i8 x 32 => m8, u16 x 16 => m16, i16 x 16 => m16, u32 x 8 => m32, i32 x 8 => m32, u64 x 4 => m64, i64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	splat!(u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, u64 x 4, i64 x 4, f32 x 8, f64 x 4);

	impl_simd_binop!(max, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, f32 x 8, f64 x 4);

	impl_scalar_binop!(max, u64, i64);

	impl_simd_binop!(min, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, f32 x 8, f64 x 4);

	impl_scalar_binop!(min, u64, i64);

	impl_simd_unop!(not, m8 x 32, u8 x 32, m16 x 16, u16 x 16, m32 x 8, u32 x 8, m64 x 4, u64 x 4);

	#[inline(always)]
	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
		let sqr = self.mul_f32s(a, a);
		let sqr_rev = self
			.avx
			._mm256_shuffle_ps::<0b10_11_00_01>(cast!(sqr), cast!(sqr));
		self.add_f32s(sqr, cast!(sqr_rev))
	}

	#[inline(always)]
	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
		let sqr = self.mul_f64s(a, a);
		let sqr_rev = self.avx._mm256_shuffle_pd::<0b0101>(cast!(sqr), cast!(sqr));
		self.add_f64s(sqr, cast!(sqr_rev))
	}

	#[inline(always)]
	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
		let max = self.abs_f32s(a);
		let max_rev = self
			.avx
			._mm256_shuffle_ps::<0b10_11_00_01>(cast!(a), cast!(a));
		self.max_f32s(max, cast!(max_rev))
	}

	#[inline(always)]
	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
		let max = self.abs_f64s(a);
		let max_rev = self.avx._mm256_shuffle_pd::<0b0101>(cast!(max), cast!(max));
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
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm256_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.avx._mm256_moveldup_ps(ab);
		let bb = self.avx._mm256_movehdup_ps(ab);

		cast!(
			self.fma
				._mm256_fmsubadd_ps(aa, xy, self.fma._mm256_fmsubadd_ps(bb, yx, cast!(c)))
		)
	}

	#[inline(always)]
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm256_permute_pd::<0b0101>(xy);
		let aa = self.avx._mm256_unpacklo_pd(ab, ab);
		let bb = self.avx._mm256_unpackhi_pd(ab, ab);

		cast!(
			self.fma
				._mm256_fmsubadd_pd(aa, xy, self.fma._mm256_fmsubadd_pd(bb, yx, cast!(c)))
		)
	}

	#[inline(always)]
	fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm256_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.avx._mm256_moveldup_ps(ab);
		let bb = self.avx._mm256_movehdup_ps(ab);

		cast!(
			self.fma
				._mm256_fmsubadd_ps(aa, xy, self.avx._mm256_mul_ps(bb, yx))
		)
	}

	#[inline(always)]
	fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm256_permute_pd::<0b0101>(xy);
		let aa = self.avx._mm256_unpacklo_pd(ab, ab);
		let bb = self.avx._mm256_unpackhi_pd(ab, ab);

		cast!(
			self.fma
				._mm256_fmsubadd_pd(aa, xy, self.avx._mm256_mul_pd(bb, yx))
		)
	}

	#[inline(always)]
	fn deinterleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		let avx = self.avx;

		if const { core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f32s>() } {
			let values: [__m256d; 2] = unsafe { core::mem::transmute_copy(&values) };
			// a0 b0 a1 b1 a2 b2 a3 b3
			// a4 b4 a5 b5 a6 b6 a7 b7

			// a0 a4 b0 b4 a2 a6 b2 b6
			// a1 a5 b1 b5 a3 a7 b3 b7
			let values = [
				cast!(avx._mm256_unpacklo_ps(cast!(values[0]), cast!(values[1]))),
				cast!(avx._mm256_unpackhi_ps(cast!(values[0]), cast!(values[1]))),
			];

			// a0 a4 a1 a5 a2 a6 a3 a7
			// b0 b4 b1 b5 b2 b6 b3 b7
			let values = [
				avx._mm256_unpacklo_pd(values[0], values[1]),
				avx._mm256_unpackhi_pd(values[0], values[1]),
			];

			unsafe { core::mem::transmute_copy(&values) }
		} else if const { core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f32s>() } {
			// a0 b0 c0 d0 a1 b1 c1 d1
			// a2 b2 c2 d2 a3 b3 c3 d3
			// a4 b4 c4 d4 a5 b5 c5 d5
			// a6 b6 c6 d6 a7 b7 c7 d7
			let values: [__m256d; 4] = unsafe { core::mem::transmute_copy(&values) };

			// a0 a2 c0 c2 a1 a3 c1 c3
			// b0 b2 d0 d2 b1 b3 d1 d3
			// a4 a6 c4 c6 a5 a7 c5 c7
			// b4 b6 d4 d6 b5 b7 d5 d7
			let values = [
				cast!(avx._mm256_unpacklo_ps(cast!(values[0]), cast!(values[1]))),
				cast!(avx._mm256_unpackhi_ps(cast!(values[0]), cast!(values[1]))),
				cast!(avx._mm256_unpacklo_ps(cast!(values[2]), cast!(values[3]))),
				cast!(avx._mm256_unpackhi_ps(cast!(values[2]), cast!(values[3]))),
			];

			let values = [
				avx._mm256_unpacklo_pd(values[0], values[1]),
				avx._mm256_unpackhi_pd(values[0], values[1]),
				avx._mm256_unpacklo_pd(values[2], values[3]),
				avx._mm256_unpackhi_pd(values[2], values[3]),
			];

			// a0 a2 a4 a6 a1 a3 a5 a7
			// b0 b2 b4 b6 b1 b3 b5 b7
			// c0 c2 c4 c6 c1 c3 c5 c7
			// d0 d2 d4 d6 d1 d3 d5 d7
			let values = [
				avx._mm256_unpacklo_pd(values[0], values[2]),
				avx._mm256_unpacklo_pd(values[1], values[3]),
				avx._mm256_unpackhi_pd(values[0], values[2]),
				avx._mm256_unpackhi_pd(values[1], values[3]),
			];

			unsafe { core::mem::transmute_copy(&values) }
		} else {
			unsafe { deinterleave_fallback::<f32, Self::f32s, T>(values) }
		}
	}

	#[inline(always)]
	fn deinterleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		let avx = self.avx;

		if const { core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f64s>() } {
			let values: [__m256d; 2] = unsafe { core::mem::transmute_copy(&values) };
			let values = [
				avx._mm256_unpacklo_pd(values[0], values[1]),
				avx._mm256_unpackhi_pd(values[0], values[1]),
			];
			unsafe { core::mem::transmute_copy(&values) }
		} else if const { core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f64s>() } {
			let values: [__m256d; 4] = unsafe { core::mem::transmute_copy(&values) };

			// a0 b0 c0 d0
			// a1 b1 c1 d1
			// a2 b2 c2 d2
			// a3 b3 c3 d3

			// a0 a1 c0 c1
			// b0 b1 d0 d1
			// a2 a3 c2 c3
			// b2 b3 d2 d3
			let values: [__m256d; 4] = [
				avx._mm256_unpacklo_pd(values[0], values[1]),
				avx._mm256_unpackhi_pd(values[0], values[1]),
				avx._mm256_unpacklo_pd(values[2], values[3]),
				avx._mm256_unpackhi_pd(values[2], values[3]),
			];

			// a0 a1 a2 a3
			// b0 b1 b2 b3
			// c0 c1 c2 c3
			// d0 d1 d2 d3
			let values = [
				avx._mm256_permute2f128_pd::<0b0010_0000>(values[0], values[2]),
				avx._mm256_permute2f128_pd::<0b0010_0000>(values[1], values[3]),
				avx._mm256_permute2f128_pd::<0b0011_0001>(values[0], values[2]),
				avx._mm256_permute2f128_pd::<0b0011_0001>(values[1], values[3]),
			];

			unsafe { core::mem::transmute_copy(&values) }
		} else {
			unsafe { deinterleave_fallback::<f64, Self::f64s, T>(values) }
		}
	}

	#[inline(always)]
	fn interleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		if const {
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
		if const {
			(core::mem::size_of::<T>() == 2 * core::mem::size_of::<Self::f64s>())
				|| (core::mem::size_of::<T>() == 4 * core::mem::size_of::<Self::f64s>())
		} {
			// permutation is inverse of itself in this case
			self.deinterleave_shfl_f64s(values)
		} else {
			unsafe { interleave_fallback::<f64, Self::f64s, T>(values) }
		}
	}

	#[inline(always)]
	fn mask_between_m32s(self, start: u32, end: u32) -> MemMask<Self::m32s> {
		let start = start.min(8) as usize;
		let end = end.min(8) as usize;
		MemMask {
			mask: self.and_m32s(
				cast!(V3_U32_LAST_MASKS[8 - start]),
				cast!(V3_U32_MASKS[end]),
			),
			#[cfg(target_arch = "x86_64")]
			load: Some(LD_ST[2 * (16 * end + start) + 0]),
			#[cfg(target_arch = "x86_64")]
			store: Some(LD_ST[2 * (16 * end + start) + 1]),
		}
	}

	#[inline(always)]
	fn mask_between_m64s(self, start: u64, end: u64) -> MemMask<Self::m64s> {
		let start = (2 * start.min(4)) as usize;
		let end = (2 * end.min(4)) as usize;
		MemMask {
			mask: self.and_m64s(
				cast!(V3_U32_LAST_MASKS[8 - start]),
				cast!(V3_U32_MASKS[end]),
			),
			#[cfg(target_arch = "x86_64")]
			load: Some(LD_ST[2 * (16 * end + start) + 0]),
			#[cfg(target_arch = "x86_64")]
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
	unsafe fn mask_load_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *const u8) -> Self::u8s {
		Scalar256b.mask_load_ptr_u8s(mask, ptr)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u16s(self, mask: MemMask<Self::m16s>, ptr: *const u16) -> Self::u16s {
		Scalar256b.mask_load_ptr_u16s(mask, ptr)
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u32s(self, mask: MemMask<Self::m32s>, ptr: *const u32) -> Self::u32s {
		#[cfg(target_arch = "x86_64")]
		if let Some(load) = mask.load {
			return avx_ld_u32s(ptr, load);
		}
		cast!(self.avx2._mm256_maskload_epi32(ptr as _, cast!(mask.mask)))
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u64s(self, mask: MemMask<Self::m64s>, ptr: *const u64) -> Self::u64s {
		#[cfg(target_arch = "x86_64")]
		if let Some(load) = mask.load {
			return cast!(avx_ld_u32s(ptr as _, load));
		}
		cast!(self.avx2._mm256_maskload_epi64(ptr as _, cast!(mask.mask)))
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
		Scalar256b.mask_store_ptr_u8s(mask, ptr, values);
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
		Scalar256b.mask_store_ptr_u16s(mask, ptr, values);
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
		#[cfg(target_arch = "x86_64")]
		if let Some(store) = mask.store {
			return avx_st_u32s(ptr, values, store);
		}
		_mm256_maskstore_epi32(ptr as *mut i32, cast!(mask.mask), cast!(values))
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
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm256_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.avx._mm256_moveldup_ps(ab);
		let bb = self.avx._mm256_movehdup_ps(ab);

		cast!(
			self.fma
				._mm256_fmaddsub_ps(aa, xy, self.fma._mm256_fmaddsub_ps(bb, yx, cast!(c)))
		)
	}

	#[inline(always)]
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm256_permute_pd::<0b0101>(xy);
		let aa = self.avx._mm256_unpacklo_pd(ab, ab);
		let bb = self.avx._mm256_unpackhi_pd(ab, ab);

		cast!(
			self.fma
				._mm256_fmaddsub_pd(aa, xy, self.fma._mm256_fmaddsub_pd(bb, yx, cast!(c)))
		)
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
		cast!(self.fma._mm256_fmadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	#[inline(always)]
	fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		cast!(self.fma._mm256_fmadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	#[inline(always)]
	fn mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm256_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.avx._mm256_moveldup_ps(ab);
		let bb = self.avx._mm256_movehdup_ps(ab);

		cast!(
			self.fma
				._mm256_fmaddsub_ps(aa, xy, self.avx._mm256_mul_ps(bb, yx))
		)
	}

	#[inline(always)]
	fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm256_permute_pd::<0b0101>(xy);
		let aa = self.avx._mm256_unpacklo_pd(ab, ab);
		let bb = self.avx._mm256_unpackhi_pd(ab, ab);

		cast!(
			self.fma
				._mm256_fmaddsub_pd(aa, xy, self.avx._mm256_mul_pd(bb, yx))
		)
	}

	#[inline(always)]
	fn neg_c32s(self, a: Self::c32s) -> Self::c32s {
		self.xor_f32s(a, self.splat_f32s(-0.0))
	}

	#[inline(always)]
	fn neg_c64s(self, a: Self::c64s) -> Self::c64s {
		self.xor_f64s(a, self.splat_f64s(-0.0))
	}

	#[cfg(target_arch = "x86_64")]
	#[inline(always)]
	fn partial_load_u32s(self, slice: &[u32]) -> Self::u32s {
		avx_load_u32s(self.avx2, slice)
	}

	#[cfg(target_arch = "x86_64")]
	#[inline(always)]
	fn partial_load_u64s(self, slice: &[u64]) -> Self::u64s {
		cast!(self.partial_load_u32s(bytemuck::cast_slice(slice)))
	}

	#[cfg(target_arch = "x86_64")]
	#[inline(always)]
	fn partial_store_u32s(self, slice: &mut [u32], values: Self::u32s) {
		avx_store_u32s(self.avx2, slice, values)
	}

	#[cfg(target_arch = "x86_64")]
	#[inline(always)]
	fn partial_store_u64s(self, slice: &mut [u64], values: Self::u64s) {
		self.partial_store_u32s(bytemuck::cast_slice_mut(slice), cast!(values))
	}

	#[inline(always)]
	fn reduce_max_c32s(self, a: Self::c32s) -> c32 {
		let a: __m256 = cast!(a);
		let r = self.sse._mm_max_ps(
			self.avx._mm256_castps256_ps128(a),
			self.avx._mm256_extractf128_ps::<1>(a),
		);
		(*self).reduce_max_c32x2(cast!(r))
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		let a: __m256d = cast!(a);
		let r = self.sse2._mm_max_pd(
			self.avx._mm256_castpd256_pd128(a),
			self.avx._mm256_extractf128_pd::<1>(a),
		);
		(*self).reduce_max_c64x1(cast!(r))
	}

	#[inline(always)]
	fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
		let a: __m256 = cast!(a);
		let r = self.sse._mm_max_ps(
			self.avx._mm256_castps256_ps128(a),
			self.avx._mm256_extractf128_ps::<1>(a),
		);
		(*self).reduce_max_f32x4(cast!(r))
	}

	#[inline(always)]
	fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
		let a: __m256d = cast!(a);
		let r = self.sse2._mm_max_pd(
			self.avx._mm256_castpd256_pd128(a),
			self.avx._mm256_extractf128_pd::<1>(a),
		);
		(*self).reduce_max_f64x2(cast!(r))
	}

	#[inline(always)]
	fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
		let a: __m256 = cast!(a);
		let r = self.sse._mm_min_ps(
			self.avx._mm256_castps256_ps128(a),
			self.avx._mm256_extractf128_ps::<1>(a),
		);
		(*self).reduce_min_c32x2(cast!(r))
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		let a: __m256d = cast!(a);
		let r = self.sse2._mm_min_pd(
			self.avx._mm256_castpd256_pd128(a),
			self.avx._mm256_extractf128_pd::<1>(a),
		);
		(*self).reduce_min_c64x1(cast!(r))
	}

	#[inline(always)]
	fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
		let a: __m256 = cast!(a);
		let r = self.sse._mm_min_ps(
			self.avx._mm256_castps256_ps128(a),
			self.avx._mm256_extractf128_ps::<1>(a),
		);
		(*self).reduce_min_f32x4(cast!(r))
	}

	#[inline(always)]
	fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
		let a: __m256d = cast!(a);
		let r = self.sse2._mm_min_pd(
			self.avx._mm256_castpd256_pd128(a),
			self.avx._mm256_extractf128_pd::<1>(a),
		);
		(*self).reduce_min_f64x2(cast!(r))
	}

	#[inline(always)]
	fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
		let a: __m256 = cast!(a);
		let r = self.sse._mm_mul_ps(
			self.avx._mm256_castps256_ps128(a),
			self.avx._mm256_extractf128_ps::<1>(a),
		);
		(*self).reduce_product_f32x4(cast!(r))
	}

	#[inline(always)]
	fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
		let a: __m256d = cast!(a);
		let r = self.sse2._mm_mul_pd(
			self.avx._mm256_castpd256_pd128(a),
			self.avx._mm256_extractf128_pd::<1>(a),
		);
		(*self).reduce_product_f64x2(cast!(r))
	}

	#[inline(always)]
	fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
		let a: __m256 = cast!(a);
		let r = self.sse._mm_add_ps(
			self.avx._mm256_castps256_ps128(a),
			self.avx._mm256_extractf128_ps::<1>(a),
		);
		(*self).reduce_sum_c32x2(cast!(r))
	}

	#[inline(always)]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		let a: __m256d = cast!(a);
		let r = self.sse2._mm_add_pd(
			self.avx._mm256_castpd256_pd128(a),
			self.avx._mm256_extractf128_pd::<1>(a),
		);
		(*self).reduce_sum_c64x1(cast!(r))
	}

	#[inline(always)]
	fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
		let a: __m256 = cast!(a);
		let r = self.sse._mm_add_ps(
			self.avx._mm256_castps256_ps128(a),
			self.avx._mm256_extractf128_ps::<1>(a),
		);
		(*self).reduce_sum_f32x4(cast!(r))
	}

	#[inline(always)]
	fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
		let a: __m256d = cast!(a);
		let r = self.sse2._mm_add_pd(
			self.avx._mm256_castpd256_pd128(a),
			self.avx._mm256_extractf128_pd::<1>(a),
		);
		(*self).reduce_sum_f64x2(cast!(r))
	}

	#[inline(always)]
	fn rotate_right_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s {
		cast!(avx2_pshufb(
			self,
			cast!(a),
			cast!(AVX2_ROTATE_IDX[8 * (amount % 4)]),
		))
	}

	#[inline(always)]
	fn rotate_right_c64s(self, a: Self::c64s, amount: usize) -> Self::c64s {
		cast!(avx2_pshufb(
			self,
			cast!(a),
			cast!(AVX2_ROTATE_IDX[16 * (amount % 2)]),
		))
	}

	#[inline(always)]
	fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s {
		cast!(avx2_pshufb(
			self,
			cast!(a),
			cast!(AVX2_ROTATE_IDX[4 * (amount % 8)]),
		))
	}

	#[inline(always)]
	fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s {
		cast!(avx2_pshufb(
			self,
			cast!(a),
			cast!(AVX2_ROTATE_IDX[8 * (amount % 4)]),
		))
	}

	#[inline(always)]
	fn select_u32s(
		self,
		mask: Self::m32s,
		if_true: Self::u32s,
		if_false: Self::u32s,
	) -> Self::u32s {
		let mask: __m256 = cast!(mask);
		let if_true: __m256 = cast!(if_true);
		let if_false: __m256 = cast!(if_false);

		cast!(self.avx._mm256_blendv_ps(if_false, if_true, mask))
	}

	#[inline(always)]
	fn select_u64s(
		self,
		mask: Self::m64s,
		if_true: Self::u64s,
		if_false: Self::u64s,
	) -> Self::u64s {
		let mask: __m256d = cast!(mask);
		let if_true: __m256d = cast!(if_true);
		let if_false: __m256d = cast!(if_false);

		cast!(self.avx._mm256_blendv_pd(if_false, if_true, mask))
	}

	#[inline(always)]
	fn splat_c32s(self, value: c32) -> Self::c32s {
		cast!(self.splat_f64s(cast!(value)))
	}

	#[inline(always)]
	fn splat_c64s(self, value: c64) -> Self::c64s {
		cast!(self.avx._mm256_broadcast_pd(&cast!(value)))
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
		cast!(self.avx._mm256_permute_ps::<0b10_11_00_01>(cast!(a)))
	}

	#[inline(always)]
	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
		cast!(self.avx._mm256_permute_pd::<0b0101>(cast!(a)))
	}

	#[inline(always)]
	fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
		struct Impl<Op> {
			this: V3,
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
		self.widening_mul_u32x8(a, b)
	}

	#[inline(always)]
	fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		self.shl_dyn_u32x8(a, self.and_u32x8(amount, self.splat_u32x8(32 - 1)))
	}

	#[inline(always)]
	fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		self.shr_dyn_u32x8(a, self.and_u32x8(amount, self.splat_u32x8(32 - 1)))
	}

	#[inline(always)]
	fn sqrt_f32s(self, a: Self::f32s) -> Self::f32s {
		self.sqrt_f32x8(a)
	}

	#[inline(always)]
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
		self.sqrt_f64x4(a)
	}
}

#[derive(Copy, Clone, Debug)]
#[repr(transparent)]
pub struct V3_128b(pub V3);

#[derive(Copy, Clone, Debug)]
#[repr(transparent)]
pub struct V3_256b(pub V3);

#[derive(Copy, Clone, Debug)]
#[repr(transparent)]
pub struct V3_512b(pub V3);

impl core::ops::Deref for V3_128b {
	type Target = V3;

	#[inline]
	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

impl core::ops::Deref for V3_256b {
	type Target = V3;

	#[inline]
	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

impl core::ops::Deref for V3_512b {
	type Target = V3;

	#[inline]
	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

impl Seal for V3_128b {}
impl Seal for V3_256b {}
impl Seal for V3_512b {}

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

impl Simd for V3_128b {
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

	impl_scalar_binop!(mul, u64, i64);

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

	#[inline(always)]
	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
		let sqr = self.mul_f32s(a, a);
		let sqr_rev = self
			.sse
			._mm_shuffle_ps::<0b10_11_00_01>(cast!(sqr), cast!(sqr));
		self.add_f32s(sqr, cast!(sqr_rev))
	}

	#[inline(always)]
	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
		let sqr = self.mul_f64s(a, a);
		let sqr_rev = self.sse2._mm_shuffle_pd::<0b01>(cast!(sqr), cast!(sqr));
		self.add_f64s(sqr, cast!(sqr_rev))
	}

	#[inline(always)]
	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
		let sqr = self.abs_f32s(a);
		let sqr_rev = self
			.sse
			._mm_shuffle_ps::<0b10_11_00_01>(cast!(sqr), cast!(sqr));
		self.max_f32s(sqr, cast!(sqr_rev))
	}

	#[inline(always)]
	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
		let sqr = self.abs_f64s(a);
		let sqr_rev = self.sse2._mm_shuffle_pd::<0b01>(cast!(sqr), cast!(sqr));
		self.max_f64s(sqr, cast!(sqr_rev))
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
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.sse3._mm_moveldup_ps(ab);
		let bb = self.sse3._mm_movehdup_ps(ab);

		cast!(
			self.fma
				._mm_fmsubadd_ps(aa, xy, self.fma._mm_fmsubadd_ps(bb, yx, cast!(c)))
		)
	}

	#[inline(always)]
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm_permute_pd::<0b01>(xy);
		let aa = self.sse2._mm_unpacklo_pd(ab, ab);
		let bb = self.sse2._mm_unpackhi_pd(ab, ab);

		cast!(
			self.fma
				._mm_fmsubadd_pd(aa, xy, self.fma._mm_fmsubadd_pd(bb, yx, cast!(c)))
		)
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_c32s(self, mask: MemMask<Self::m32s>, ptr: *const c32) -> Self::c32s {
		cast!(self.mask_load_ptr_u32s(mask, ptr as _))
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_c64s(self, mask: MemMask<Self::m64s>, ptr: *const c64) -> Self::c64s {
		cast!(self.mask_load_ptr_u64s(mask, ptr as _))
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *const u8) -> Self::u8s {
		Scalar128b.mask_load_ptr_u8s(mask, ptr)
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_u16s(self, mask: MemMask<Self::m16s>, ptr: *const u16) -> Self::u16s {
		Scalar128b.mask_load_ptr_u16s(mask, ptr)
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_u32s(self, mask: MemMask<Self::m32s>, ptr: *const u32) -> Self::u32s {
		#[cfg(target_arch = "x86_64")]
		if let Some(load) = mask.load {
			return cast_lossy(avx_ld_u32s(ptr, load));
		}
		cast!(self.avx2._mm_maskload_epi32(ptr as _, cast!(mask.mask)))
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_u64s(self, mask: MemMask<Self::m64s>, ptr: *const u64) -> Self::u64s {
		#[cfg(target_arch = "x86_64")]
		if let Some(load) = mask.load {
			return cast_lossy(avx_ld_u32s(ptr as _, load));
		}
		cast!(self.avx2._mm_maskload_epi64(ptr as _, cast!(mask.mask)))
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_c32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut c32,
		values: Self::c32s,
	) {
		self.mask_store_ptr_u32s(mask, ptr as _, cast!(values));
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_c64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut c64,
		values: Self::c64s,
	) {
		self.mask_store_ptr_u64s(mask, ptr as _, cast!(values))
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *mut u8, values: Self::u8s) {
		Scalar128b.mask_store_ptr_u8s(mask, ptr, values);
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_u16s(
		self,
		mask: MemMask<Self::m16s>,
		ptr: *mut u16,
		values: Self::u16s,
	) {
		Scalar128b.mask_store_ptr_u16s(mask, ptr, values);
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_u32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut u32,
		values: Self::u32s,
	) {
		#[cfg(target_arch = "x86_64")]
		if let Some(store) = mask.store {
			return avx_st_u32s(ptr, cast!([values, self.splat_u32s(0)]), store);
		}
		self.avx2
			._mm_maskstore_epi32(ptr as _, cast!(mask.mask), cast!(values));
	}

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
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.sse3._mm_moveldup_ps(ab);
		let bb = self.sse3._mm_movehdup_ps(ab);

		cast!(
			self.fma
				._mm_fmaddsub_ps(aa, xy, self.fma._mm_fmaddsub_ps(bb, yx, cast!(c)))
		)
	}

	#[inline(always)]
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm_permute_pd::<0b01>(xy);
		let aa = self.sse2._mm_unpacklo_pd(ab, ab);
		let bb = self.sse2._mm_unpackhi_pd(ab, ab);

		cast!(
			self.fma
				._mm_fmaddsub_pd(aa, xy, self.fma._mm_fmaddsub_pd(bb, yx, cast!(c)))
		)
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
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm_permute_ps::<0b10_11_00_01>(xy);
		let aa = self.sse3._mm_moveldup_ps(ab);
		let bb = self.sse3._mm_movehdup_ps(ab);

		cast!(
			self.fma
				._mm_fmaddsub_ps(aa, xy, self.sse._mm_mul_ps(bb, yx))
		)
	}

	#[inline(always)]
	fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let ab = cast!(a);
		let xy = cast!(b);

		let yx = self.avx._mm_permute_pd::<0b01>(xy);
		let aa = self.sse2._mm_unpacklo_pd(ab, ab);
		let bb = self.sse2._mm_unpackhi_pd(ab, ab);

		cast!(
			self.fma
				._mm_fmaddsub_pd(aa, xy, self.sse2._mm_mul_pd(bb, yx))
		)
	}

	#[inline(always)]
	fn neg_c32s(self, a: Self::c32s) -> Self::c32s {
		self.xor_f32s(a, self.splat_f32s(-0.0))
	}

	#[inline(always)]
	fn neg_c64s(self, a: Self::c64s) -> Self::c64s {
		self.xor_f64s(a, self.splat_f64s(-0.0))
	}

	#[cfg(target_arch = "x86_64")]
	#[inline(always)]
	fn partial_load_u32s(self, slice: &[u32]) -> Self::u32s {
		cast_lossy(avx_load_u32s(self.avx2, slice))
	}

	#[cfg(target_arch = "x86_64")]
	#[inline(always)]
	fn partial_load_u64s(self, slice: &[u64]) -> Self::u64s {
		cast!(self.partial_load_u32s(bytemuck::cast_slice(slice)))
	}

	#[cfg(target_arch = "x86_64")]
	#[inline(always)]
	fn partial_store_u32s(self, slice: &mut [u32], values: Self::u32s) {
		avx_store_u32s(self.avx2, slice, cast!([values, self.splat_u32s(0)]))
	}

	#[cfg(target_arch = "x86_64")]
	#[inline(always)]
	fn partial_store_u64s(self, slice: &mut [u64], values: Self::u64s) {
		self.partial_store_u32s(bytemuck::cast_slice_mut(slice), cast!(values))
	}

	#[inline(always)]
	fn reduce_max_c32s(self, a: Self::c32s) -> c32 {
		let a: __m128 = cast!(a);
		let hi = self.sse._mm_movehl_ps(a, a);
		let r0 = self.sse._mm_max_ps(a, hi);
		cast!(self.sse2._mm_cvtsd_f64(cast!(r0)))
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
		let a: __m128 = cast!(a);
		let hi = self.sse._mm_movehl_ps(a, a);
		let r0 = self.sse._mm_max_ps(a, hi);
		let r0_shuffled = self.sse._mm_shuffle_ps::<0b0001>(r0, r0);
		let r = self.sse._mm_max_ss(r0, r0_shuffled);
		self.sse._mm_cvtss_f32(r)
	}

	#[inline(always)]
	fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
		let a: __m128d = cast!(a);
		let hi = cast!(self.sse._mm_movehl_ps(cast!(a), cast!(a)));
		let r = self.sse2._mm_max_sd(a, hi);
		self.sse2._mm_cvtsd_f64(r)
	}

	#[inline(always)]
	fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
		let a: __m128 = cast!(a);
		let hi = self.sse._mm_movehl_ps(a, a);
		let r0 = self.sse._mm_min_ps(a, hi);
		cast!(self.sse2._mm_cvtsd_f64(cast!(r0)))
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
		let a: __m128 = cast!(a);
		let hi = self.sse._mm_movehl_ps(a, a);
		let r0 = self.sse._mm_min_ps(a, hi);
		let r0_shuffled = self.sse._mm_shuffle_ps::<0b0001>(r0, r0);
		let r = self.sse._mm_min_ss(r0, r0_shuffled);
		self.sse._mm_cvtss_f32(r)
	}

	#[inline(always)]
	fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
		let a: __m128d = cast!(a);
		let hi = cast!(self.sse._mm_movehl_ps(cast!(a), cast!(a)));
		let r = self.sse2._mm_min_sd(a, hi);
		self.sse2._mm_cvtsd_f64(r)
	}

	#[inline(always)]
	fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
		let a: __m128 = cast!(a);
		let hi = self.sse._mm_movehl_ps(a, a);
		let r0 = self.sse._mm_mul_ps(a, hi);
		let r0_shuffled = self.sse._mm_shuffle_ps::<0b0001>(r0, r0);
		let r = self.sse._mm_mul_ss(r0, r0_shuffled);
		self.sse._mm_cvtss_f32(r)
	}

	#[inline(always)]
	fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
		let a: __m128d = cast!(a);
		let hi = cast!(self.sse._mm_movehl_ps(cast!(a), cast!(a)));
		let r = self.sse2._mm_mul_sd(a, hi);
		self.sse2._mm_cvtsd_f64(r)
	}

	#[inline(always)]
	fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
		// a0 a1 a2 a3
		let a: __m128 = cast!(a);
		// a2 a3 a2 a3
		let hi = self.sse._mm_movehl_ps(a, a);

		// a0+a2 a1+a3 _ _
		let r0 = self.sse._mm_add_ps(a, hi);

		cast!(self.sse2._mm_cvtsd_f64(cast!(r0)))
	}

	#[inline(always)]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		cast!(a)
	}

	#[inline(always)]
	fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
		let a: __m128 = cast!(a);
		let hi = self.sse._mm_movehl_ps(a, a);
		let r0 = self.sse._mm_add_ps(a, hi);
		let r0_shuffled = self.sse._mm_shuffle_ps::<0b0001>(r0, r0);
		let r = self.sse._mm_add_ss(r0, r0_shuffled);
		self.sse._mm_cvtss_f32(r)
	}

	#[inline(always)]
	fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
		let a: __m128d = cast!(a);
		let hi = cast!(self.sse._mm_movehl_ps(cast!(a), cast!(a)));
		let r = self.sse2._mm_add_sd(a, hi);
		self.sse2._mm_cvtsd_f64(r)
	}

	#[inline(always)]
	fn rotate_right_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s {
		cast!(
			self.ssse3
				._mm_shuffle_epi8(cast!(a), cast!(AVX2_128_ROTATE_IDX[8 * (amount % 2)]))
		)
	}

	#[inline(always)]
	fn rotate_right_c64s(self, a: Self::c64s, amount: usize) -> Self::c64s {
		_ = amount;
		a
	}

	#[inline(always)]
	fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s {
		cast!(
			self.ssse3
				._mm_shuffle_epi8(cast!(a), cast!(AVX2_128_ROTATE_IDX[4 * (amount % 4)]))
		)
	}

	#[inline(always)]
	fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s {
		cast!(
			self.ssse3
				._mm_shuffle_epi8(cast!(a), cast!(AVX2_128_ROTATE_IDX[8 * (amount % 2)]))
		)
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
		self.sub_f64x2(a, b)
	}

	#[inline(always)]
	fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s {
		cast!(self.avx._mm_permute_ps::<0b10_11_00_01>(cast!(a)))
	}

	#[inline(always)]
	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
		cast!(self.avx._mm_permute_pd::<0b01>(cast!(a)))
	}

	#[inline(always)]
	fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
		Simd::vectorize(self.0, op)
	}

	#[inline(always)]
	fn widening_mul_u32s(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
		self.widening_mul_u32x4(a, b)
	}

	#[inline(always)]
	fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		self.shl_dyn_u32x4(a, self.and_u32x4(amount, self.splat_u32x4(32 - 1)))
	}

	#[inline(always)]
	fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		self.shr_dyn_u32x4(a, self.and_u32x4(amount, self.splat_u32x4(32 - 1)))
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

impl Simd for V3_256b {
	type c32s = f32x8;
	type c64s = f64x4;
	type f32s = f32x8;
	type f64s = f64x4;
	type i16s = i16x16;
	type i32s = i32x8;
	type i64s = i64x4;
	type i8s = i8x32;
	type m16s = m16x16;
	type m32s = m32x8;
	type m64s = m64x4;
	type m8s = m8x32;
	type u16s = u16x16;
	type u32s = u32x8;
	type u64s = u64x4;
	type u8s = u8x32;

	const REGISTER_COUNT: usize = 16;

	impl_simd_binop!(add, f32 x 8, f64 x 4);

	impl_simd_binop!(add, wrapping_add, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, u64 x 4, i64 x 4);

	impl_simd_binop!(sub, f32 x 8, f64 x 4);

	impl_simd_binop!(sub, wrapping_sub, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, u64 x 4, i64 x 4);

	impl_simd_binop!(mul, f32 x 8, f64 x 4);

	impl_simd_binop!(mul, wrapping_mul, u16 x 16, i16 x 16, u32 x 8, i32 x 8);

	impl_simd_binop!(and, m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4, f32 x 8, f64 x 4);

	impl_simd_binop!(or, m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4, f32 x 8, f64 x 4);

	impl_simd_binop!(xor, m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4, f32 x 8, f64 x 4);

	impl_simd_binop!(div, f32 x 8, f64 x 4);

	impl_simd_binop!(equal, cmp_eq, u8 x 32 => m8, u16 x 16 => m16, u32 x 8 => m32, u64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	impl_simd_binop!(greater_than, cmp_gt, u8 x 32 => m8, i8 x 32 => m8, u16 x 16 => m16, i16 x 16 => m16, u32 x 8 => m32, i32 x 8 => m32, u64 x 4 => m64, i64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	impl_simd_binop!(greater_than_or_equal, cmp_ge, u8 x 32 => m8, i8 x 32 => m8, u16 x 16 => m16, i16 x 16 => m16, u32 x 8 => m32, i32 x 8 => m32, u64 x 4 => m64, i64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	impl_simd_binop!(less_than, cmp_lt, u8 x 32 => m8, i8 x 32 => m8, u16 x 16 => m16, i16 x 16 => m16, u32 x 8 => m32, i32 x 8 => m32, u64 x 4 => m64, i64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	impl_simd_binop!(less_than_or_equal, cmp_le, u8 x 32 => m8, i8 x 32 => m8, u16 x 16 => m16, i16 x 16 => m16, u32 x 8 => m32, i32 x 8 => m32, u64 x 4 => m64, i64 x 4 => m64, f32 x 8 => m32, f64 x 4 => m64);

	splat!(u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, u64 x 4, i64 x 4, f32 x 8, f64 x 4);

	impl_simd_binop!(max, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, f32 x 8, f64 x 4);

	impl_simd_binop!(min, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, f32 x 8, f64 x 4);

	impl_simd_unop!(not, m8 x 32, u8 x 32, m16 x 16, u16 x 16, m32 x 8, u32 x 8, m64 x 4, u64 x 4);

	inherit!({
		fn abs2_c32s(self, a: Self::c32s) -> Self::c32s;
		fn abs2_c64s(self, a: Self::c64s) -> Self::c64s;
		fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s;
		fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s;
		fn add_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
		fn add_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
		fn conj_c32s(self, a: Self::c32s) -> Self::c32s;
		fn conj_c64s(self, a: Self::c64s) -> Self::c64s;
		fn conj_mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s;
		fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s;
		fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
		fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
		fn equal_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::m32s;
		fn equal_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::m64s;
		fn mask_between_m32s(self, start: u32, end: u32) -> MemMask<Self::m32s>;
		fn mask_between_m64s(self, start: u64, end: u64) -> MemMask<Self::m64s>;
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_load_ptr_c32s(
			self,
			mask: MemMask<Self::m32s>,
			ptr: *const c32,
		) -> Self::c32s;
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_load_ptr_c64s(
			self,
			mask: MemMask<Self::m64s>,
			ptr: *const c64,
		) -> Self::c64s;
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_load_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *const u8) -> Self::u8s;
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_load_ptr_u16s(
			self,
			mask: MemMask<Self::m16s>,
			ptr: *const u16,
		) -> Self::u16s;
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_load_ptr_u32s(
			self,
			mask: MemMask<Self::m32s>,
			ptr: *const u32,
		) -> Self::u32s;
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_load_ptr_u64s(
			self,
			mask: MemMask<Self::m64s>,
			ptr: *const u64,
		) -> Self::u64s;
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_store_ptr_c32s(
			self,
			mask: MemMask<Self::m32s>,
			ptr: *mut c32,
			values: Self::c32s,
		);
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_store_ptr_c64s(
			self,
			mask: MemMask<Self::m64s>,
			ptr: *mut c64,
			values: Self::c64s,
		);
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_store_ptr_u8s(
			self,
			mask: MemMask<Self::m8s>,
			ptr: *mut u8,
			values: Self::u8s,
		);
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_store_ptr_u16s(
			self,
			mask: MemMask<Self::m16s>,
			ptr: *mut u16,
			values: Self::u16s,
		);
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_store_ptr_u32s(
			self,
			mask: MemMask<Self::m32s>,
			ptr: *mut u32,
			values: Self::u32s,
		);
		/// # Safety
		///
		/// See the trait-level safety documentation.
		unsafe fn mask_store_ptr_u64s(
			self,
			mask: MemMask<Self::m64s>,
			ptr: *mut u64,
			values: Self::u64s,
		);
		fn mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s;
		fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s;
		fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s;
		fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s;
		fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s;
		fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s;
		fn mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
		fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
		fn neg_c32s(self, a: Self::c32s) -> Self::c32s;
		fn neg_c64s(self, a: Self::c64s) -> Self::c64s;
		fn min_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
		fn min_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::i64s;
		fn max_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
		fn max_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::i64s;
		fn mul_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
		fn mul_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::i64s;
		fn partial_load_u32s(self, slice: &[u32]) -> Self::u32s;
		fn partial_load_u64s(self, slice: &[u64]) -> Self::u64s;
		fn partial_store_u32s(self, slice: &mut [u32], values: Self::u32s);
		fn partial_store_u64s(self, slice: &mut [u64], values: Self::u64s);
		fn reduce_max_c32s(self, a: Self::c32s) -> c32;
		fn reduce_max_c64s(self, a: Self::c64s) -> c64;
		fn reduce_max_f32s(self, a: Self::f32s) -> f32;
		fn reduce_max_f64s(self, a: Self::f64s) -> f64;
		fn reduce_min_c32s(self, a: Self::c32s) -> c32;
		fn reduce_min_c64s(self, a: Self::c64s) -> c64;
		fn reduce_min_f32s(self, a: Self::f32s) -> f32;
		fn reduce_min_f64s(self, a: Self::f64s) -> f64;
		fn reduce_product_f32s(self, a: Self::f32s) -> f32;
		fn reduce_product_f64s(self, a: Self::f64s) -> f64;
		fn reduce_sum_c32s(self, a: Self::c32s) -> c32;
		fn reduce_sum_c64s(self, a: Self::c64s) -> c64;
		fn reduce_sum_f32s(self, a: Self::f32s) -> f32;
		fn reduce_sum_f64s(self, a: Self::f64s) -> f64;
		fn rotate_right_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s;
		fn rotate_right_c64s(self, a: Self::c64s, amount: usize) -> Self::c64s;
		fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s;
		fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s;
		fn select_u32s(
			self,
			mask: Self::m32s,
			if_true: Self::u32s,
			if_false: Self::u32s,
		) -> Self::u32s;
		fn select_u64s(
			self,
			mask: Self::m64s,
			if_true: Self::u64s,
			if_false: Self::u64s,
		) -> Self::u64s;
		fn splat_c32s(self, a: c32) -> Self::c32s;
		fn splat_c64s(self, a: c64) -> Self::c64s;
		fn sub_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
		fn sub_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
		fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s;
		fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s;
		fn widening_mul_u32s(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s);
		fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s;
		fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s;
	});

	#[inline(always)]
	fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
		Simd::vectorize(self.0, op)
	}

	#[inline(always)]
	fn sqrt_f32s(self, a: Self::f32s) -> Self::f32s {
		self.sqrt_f32x8(a)
	}

	#[inline(always)]
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
		self.sqrt_f64x4(a)
	}
}

impl Simd for V3_512b {
	type c32s = f32x16;
	type c64s = f64x8;
	type f32s = f32x16;
	type f64s = f64x8;
	type i16s = i16x32;
	type i32s = i32x16;
	type i64s = i64x8;
	type i8s = i8x64;
	type m16s = m16x32;
	type m32s = m32x16;
	type m64s = m64x8;
	type m8s = m8x64;
	type u16s = u16x32;
	type u32s = u32x16;
	type u64s = u64x8;
	type u8s = u8x64;

	const REGISTER_COUNT: usize = 8;

	inherit_x2!(V3_256b(*self), {
		fn abs2_c32s(self, a: Self::c32s) -> Self::c32s;
		fn abs2_c64s(self, a: Self::c64s) -> Self::c64s;
		fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s;
		fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s;
		fn add_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
		fn add_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
		fn add_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
		fn add_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
		fn add_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::u8s;
		fn add_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::u16s;
		fn add_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
		fn add_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
		fn and_m32s(self, a: Self::m32s, b: Self::m32s) -> Self::m32s;
		fn and_m64s(self, a: Self::m64s, b: Self::m64s) -> Self::m64s;
		fn and_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
		fn and_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
		fn and_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::u8s;
		fn and_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::u16s;
		fn and_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
		fn and_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
		fn conj_c32s(self, a: Self::c32s) -> Self::c32s;
		fn conj_c64s(self, a: Self::c64s) -> Self::c64s;
		fn conj_mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s;
		fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s;
		fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
		fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
		fn div_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
		fn div_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
		fn equal_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::m32s;
		fn equal_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::m64s;
		fn equal_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::m32s;
		fn equal_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::m64s;
		fn equal_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::m8s;
		fn equal_i8s(self, a: Self::i8s, b: Self::i8s) -> Self::m8s;
		fn equal_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::m16s;
		fn equal_i16s(self, a: Self::i16s, b: Self::i16s) -> Self::m16s;
		fn equal_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::m32s;
		fn equal_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::m32s;
		fn equal_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s;
		fn equal_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::m64s;
		fn greater_than_or_equal_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::m8s;
		fn greater_than_or_equal_i8s(self, a: Self::i8s, b: Self::i8s) -> Self::m8s;
		fn greater_than_or_equal_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::m16s;
		fn greater_than_or_equal_i16s(self, a: Self::i16s, b: Self::i16s) -> Self::m16s;
		fn greater_than_or_equal_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::m32s;
		fn greater_than_or_equal_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::m32s;
		fn greater_than_or_equal_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s;
		fn greater_than_or_equal_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::m64s;
		fn greater_than_or_equal_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::m32s;
		fn greater_than_or_equal_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::m64s;
		fn greater_than_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::m8s;
		fn greater_than_i8s(self, a: Self::i8s, b: Self::i8s) -> Self::m8s;
		fn greater_than_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::m16s;
		fn greater_than_i16s(self, a: Self::i16s, b: Self::i16s) -> Self::m16s;
		fn greater_than_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::m32s;
		fn greater_than_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::m32s;
		fn greater_than_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s;
		fn greater_than_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::m64s;
		fn greater_than_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::m32s;
		fn greater_than_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::m64s;
		fn less_than_or_equal_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::m8s;
		fn less_than_or_equal_i8s(self, a: Self::i8s, b: Self::i8s) -> Self::m8s;
		fn less_than_or_equal_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::m16s;
		fn less_than_or_equal_i16s(self, a: Self::i16s, b: Self::i16s) -> Self::m16s;
		fn less_than_or_equal_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::m32s;
		fn less_than_or_equal_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::m32s;
		fn less_than_or_equal_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s;
		fn less_than_or_equal_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::m64s;
		fn less_than_or_equal_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::m32s;
		fn less_than_or_equal_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::m64s;
		fn less_than_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::m8s;
		fn less_than_i8s(self, a: Self::i8s, b: Self::i8s) -> Self::m8s;
		fn less_than_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::m16s;
		fn less_than_i16s(self, a: Self::i16s, b: Self::i16s) -> Self::m16s;
		fn less_than_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::m32s;
		fn less_than_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::m32s;
		fn less_than_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::m64s;
		fn less_than_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::m64s;
		fn less_than_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::m32s;
		fn less_than_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::m64s;
		fn min_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::u8s;
		fn max_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::u8s;
		fn min_i8s(self, a: Self::i8s, b: Self::i8s) -> Self::i8s;
		fn max_i8s(self, a: Self::i8s, b: Self::i8s) -> Self::i8s;
		fn min_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::u16s;
		fn max_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::u16s;
		fn min_i16s(self, a: Self::i16s, b: Self::i16s) -> Self::i16s;
		fn max_i16s(self, a: Self::i16s, b: Self::i16s) -> Self::i16s;
		fn min_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
		fn max_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
		fn min_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::i32s;
		fn max_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::i32s;
		fn max_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
		fn max_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
		fn min_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
		fn min_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
		fn mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s;
		fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s;
		fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s;
		fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s;
		fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s;
		fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s;
		fn mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
		fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
		fn mul_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
		fn mul_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
		fn mul_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::u16s;
		fn mul_i16s(self, a: Self::i16s, b: Self::i16s) -> Self::i16s;
		fn mul_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
		fn mul_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::i32s;
		fn mul_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
		fn mul_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::i64s;
		fn neg_c32s(self, a: Self::c32s) -> Self::c32s;
		fn neg_c64s(self, a: Self::c64s) -> Self::c64s;
		fn not_m8s(self, a: Self::m8s) -> Self::m8s;
		fn not_m16s(self, a: Self::m16s) -> Self::m16s;
		fn not_m32s(self, a: Self::m32s) -> Self::m32s;
		fn not_m64s(self, a: Self::m64s) -> Self::m64s;
		fn not_f32s(self, a: Self::f32s) -> Self::f32s;
		fn not_f64s(self, a: Self::f64s) -> Self::f64s;
		fn not_u8s(self, a: Self::u8s) -> Self::u8s;
		fn not_u16s(self, a: Self::u16s) -> Self::u16s;
		fn not_u32s(self, a: Self::u32s) -> Self::u32s;
		fn not_u64s(self, a: Self::u64s) -> Self::u64s;
		fn or_m8s(self, a: Self::m8s, b: Self::m8s) -> Self::m8s;
		fn or_m16s(self, a: Self::m16s, b: Self::m16s) -> Self::m16s;
		fn or_m32s(self, a: Self::m32s, b: Self::m32s) -> Self::m32s;
		fn or_m64s(self, a: Self::m64s, b: Self::m64s) -> Self::m64s;
		fn or_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
		fn or_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
		fn or_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::u8s;
		fn or_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::u16s;
		fn or_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
		fn or_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
		fn select_u32s(
			self,
			mask: Self::m32s,
			if_true: Self::u32s,
			if_false: Self::u32s,
		) -> Self::u32s;
		fn select_u64s(
			self,
			mask: Self::m64s,
			if_true: Self::u64s,
			if_false: Self::u64s,
		) -> Self::u64s;
		fn sub_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
		fn sub_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
		fn sub_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
		fn sub_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
		fn sub_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::u8s;
		fn sub_i8s(self, a: Self::i8s, b: Self::i8s) -> Self::i8s;
		fn sub_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::u16s;
		fn sub_i16s(self, a: Self::i16s, b: Self::i16s) -> Self::i16s;
		fn sub_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
		fn sub_i32s(self, a: Self::i32s, b: Self::i32s) -> Self::i32s;
		fn sub_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
		fn sub_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::i64s;
		fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s;
		fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s;
		fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s;
		fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s;
		fn xor_m8s(self, a: Self::m8s, b: Self::m8s) -> Self::m8s;
		fn xor_m16s(self, a: Self::m16s, b: Self::m16s) -> Self::m16s;
		fn xor_m32s(self, a: Self::m32s, b: Self::m32s) -> Self::m32s;
		fn xor_m64s(self, a: Self::m64s, b: Self::m64s) -> Self::m64s;
		fn xor_f32s(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
		fn xor_f64s(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
		fn xor_u8s(self, a: Self::u8s, b: Self::u8s) -> Self::u8s;
		fn xor_u16s(self, a: Self::u16s, b: Self::u16s) -> Self::u16s;
		fn xor_u32s(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
		fn xor_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
	});

	inherit_x2!(V3_256b(*self), splat, {
		fn splat_c32s(self, value: c32) -> Self::c32s;
		fn splat_c64s(self, value: c64) -> Self::c64s;
		fn splat_f32s(self, value: f32) -> Self::f32s;
		fn splat_f64s(self, value: f64) -> Self::f64s;
		fn splat_u8s(self, value: u8) -> Self::u8s;
		fn splat_i8s(self, value: i8) -> Self::i8s;
		fn splat_u16s(self, value: u16) -> Self::u16s;
		fn splat_i16s(self, value: i16) -> Self::i16s;
		fn splat_u32s(self, value: u32) -> Self::u32s;
		fn splat_i32s(self, value: i32) -> Self::i32s;
		fn splat_u64s(self, value: u64) -> Self::u64s;
		fn splat_i64s(self, value: i64) -> Self::i64s;
	});

	inherit_x2!(V3_256b(*self), wide, {
		fn widening_mul_u32s(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s);
	});

	#[inline(always)]
	fn rotate_right_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s {
		let simd = V3_256b(*self);
		let amount = amount % Self::C32_LANES;
		let [mut a0, mut a1]: [_; 2] = cast!(a);
		if amount >= Self::C32_LANES / 2 {
			core::mem::swap(&mut a0, &mut a1);
		}
		let amount = amount % (Self::C32_LANES / 2);
		let mask = simd.mask_between_m32s(0, amount as _).mask();
		let a0 = simd.rotate_right_c32s(a0, amount);
		let a1 = simd.rotate_right_c32s(a1, amount);

		cast!([
			simd.select_f32s(mask, a1, a0),
			simd.select_f32s(mask, a0, a1),
		])
	}

	#[inline(always)]
	fn rotate_right_c64s(self, a: Self::c64s, amount: usize) -> Self::c64s {
		let simd = V3_256b(*self);
		let amount = amount % Self::C64_LANES;
		let [mut a0, mut a1]: [_; 2] = cast!(a);
		if amount >= Self::C64_LANES / 2 {
			core::mem::swap(&mut a0, &mut a1);
		}
		let amount = amount % (Self::C64_LANES / 2);
		let mask = simd.mask_between_m64s(0, amount as _).mask();
		let a0 = simd.rotate_right_c64s(a0, amount);
		let a1 = simd.rotate_right_c64s(a1, amount);

		cast!([
			simd.select_f64s(mask, a1, a0),
			simd.select_f64s(mask, a0, a1),
		])
	}

	#[inline(always)]
	fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s {
		let simd = V3_256b(*self);
		let amount = amount % Self::U32_LANES;
		let [mut a0, mut a1]: [_; 2] = cast!(a);
		if amount >= Self::U32_LANES / 2 {
			core::mem::swap(&mut a0, &mut a1);
		}
		let amount = amount % (Self::U32_LANES / 2);
		let mask = simd.mask_between_m32s(0, amount as _).mask();
		let a0 = simd.rotate_right_u32s(a0, amount);
		let a1 = simd.rotate_right_u32s(a1, amount);

		cast!([
			simd.select_u32s(mask, a1, a0),
			simd.select_u32s(mask, a0, a1),
		])
	}

	#[inline(always)]
	fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s {
		let simd = V3_256b(*self);
		let amount = amount % Self::U64_LANES;
		let [mut a0, mut a1]: [_; 2] = cast!(a);
		if amount >= Self::U64_LANES / 2 {
			core::mem::swap(&mut a0, &mut a1);
		}
		let amount = amount % (Self::U64_LANES / 2);
		let mask = simd.mask_between_m64s(0, amount as _).mask();
		let a0 = simd.rotate_right_u64s(a0, amount);
		let a1 = simd.rotate_right_u64s(a1, amount);

		cast!([
			simd.select_u64s(mask, a1, a0),
			simd.select_u64s(mask, a0, a1),
		])
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_c32s(self, mask: MemMask<Self::m32s>, ptr: *const c32) -> Self::c32s {
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		cast!([
			simd.mask_load_ptr_c32s(MemMask::new(mask[0]), ptr.wrapping_add(0)),
			simd.mask_load_ptr_c32s(MemMask::new(mask[1]), ptr.wrapping_add(Self::C32_LANES)),
		])
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_c64s(self, mask: MemMask<Self::m64s>, ptr: *const c64) -> Self::c64s {
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		cast!([
			simd.mask_load_ptr_c64s(MemMask::new(mask[0]), ptr.wrapping_add(0)),
			simd.mask_load_ptr_c64s(MemMask::new(mask[1]), ptr.wrapping_add(Self::C64_LANES)),
		])
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *const u8) -> Self::u8s {
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		cast!([
			simd.mask_load_ptr_u8s(MemMask::new(mask[0]), ptr.wrapping_add(0)),
			simd.mask_load_ptr_u8s(MemMask::new(mask[1]), ptr.wrapping_add(V3_256b::U8_LANES)),
		])
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u16s(self, mask: MemMask<Self::m16s>, ptr: *const u16) -> Self::u16s {
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		cast!([
			simd.mask_load_ptr_u16s(MemMask::new(mask[0]), ptr.wrapping_add(0)),
			simd.mask_load_ptr_u16s(MemMask::new(mask[1]), ptr.wrapping_add(V3_256b::U16_LANES)),
		])
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u32s(self, mask: MemMask<Self::m32s>, ptr: *const u32) -> Self::u32s {
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		cast!([
			simd.mask_load_ptr_u32s(MemMask::new(mask[0]), ptr.wrapping_add(0)),
			simd.mask_load_ptr_u32s(MemMask::new(mask[1]), ptr.wrapping_add(Self::U32_LANES)),
		])
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_load_ptr_u64s(self, mask: MemMask<Self::m64s>, ptr: *const u64) -> Self::u64s {
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		cast!([
			simd.mask_load_ptr_u64s(MemMask::new(mask[0]), ptr.wrapping_add(0)),
			simd.mask_load_ptr_u64s(MemMask::new(mask[1]), ptr.wrapping_add(Self::U64_LANES)),
		])
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
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		let values: [_; 2] = cast!(values);
		cast!([
			simd.mask_store_ptr_c32s(MemMask::new(mask[0]), ptr.wrapping_add(0), values[0]),
			simd.mask_store_ptr_c32s(
				MemMask::new(mask[1]),
				ptr.wrapping_add(Self::C32_LANES),
				values[1]
			),
		])
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
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		let values: [_; 2] = cast!(values);
		cast!([
			simd.mask_store_ptr_c64s(MemMask::new(mask[0]), ptr.wrapping_add(0), values[0]),
			simd.mask_store_ptr_c64s(
				MemMask::new(mask[1]),
				ptr.wrapping_add(Self::C64_LANES),
				values[1]
			),
		])
	}

	/// # Safety
	///
	/// See the trait-level safety documentation.
	#[inline(always)]
	unsafe fn mask_store_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *mut u8, values: Self::u8s) {
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		let values: [_; 2] = cast!(values);
		cast!([
			simd.mask_store_ptr_u8s(MemMask::new(mask[0]), ptr.wrapping_add(0), values[0]),
			simd.mask_store_ptr_u8s(
				MemMask::new(mask[1]),
				ptr.wrapping_add(V3_256b::U8_LANES),
				values[1]
			),
		])
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
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		let values: [_; 2] = cast!(values);
		cast!([
			simd.mask_store_ptr_u16s(MemMask::new(mask[0]), ptr.wrapping_add(0), values[0]),
			simd.mask_store_ptr_u16s(
				MemMask::new(mask[1]),
				ptr.wrapping_add(V3_256b::U16_LANES),
				values[1]
			),
		])
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
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		let values: [_; 2] = cast!(values);
		cast!([
			simd.mask_store_ptr_u32s(MemMask::new(mask[0]), ptr.wrapping_add(0), values[0]),
			simd.mask_store_ptr_u32s(
				MemMask::new(mask[1]),
				ptr.wrapping_add(Self::U32_LANES),
				values[1]
			),
		])
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
		let simd = V3_256b(*self);
		let mask: [_; 2] = cast!(mask.mask());
		let values: [_; 2] = cast!(values);
		cast!([
			simd.mask_store_ptr_u64s(MemMask::new(mask[0]), ptr.wrapping_add(0), values[0]),
			simd.mask_store_ptr_u64s(
				MemMask::new(mask[1]),
				ptr.wrapping_add(Self::U64_LANES),
				values[1]
			),
		])
	}

	#[inline(always)]
	fn reduce_max_c32s(self, a: Self::c32s) -> c32 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_max_c32s(simd.max_f32s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_max_c64s(simd.max_f64s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_max_f32s(simd.max_f32s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_max_f64s(simd.max_f64s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_min_c32s(simd.min_f32s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_min_c64s(simd.min_f64s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_min_f32s(simd.min_f32s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_min_f64s(simd.min_f64s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_product_f32s(simd.mul_f32s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_product_f64s(simd.mul_f64s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_sum_c32s(simd.add_c32s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_sum_c64s(simd.add_c64s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_sum_f32s(simd.add_f32s(a[0], a[1]))
	}

	#[inline(always)]
	fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
		let simd = V3_256b(*self);
		let a: [_; 2] = cast!(a);
		simd.reduce_sum_f64s(simd.add_f64s(a[0], a[1]))
	}

	#[inline(always)]
	fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
		Simd::vectorize(self.0, op)
	}

	#[inline(always)]
	fn max_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
		Scalar512b.max_u64s(a, b)
	}

	#[inline(always)]
	fn max_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::i64s {
		Scalar512b.max_i64s(a, b)
	}

	#[inline(always)]
	fn min_u64s(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
		Scalar512b.min_u64s(a, b)
	}

	#[inline(always)]
	fn min_i64s(self, a: Self::i64s, b: Self::i64s) -> Self::i64s {
		Scalar512b.max_i64s(a, b)
	}

	#[inline(always)]
	fn sqrt_f32s(self, a: Self::f32s) -> Self::f32s {
		let a: [_; 2] = cast!(a);
		cast!([self.sqrt_f32x8(a[0]), self.sqrt_f32x8(a[1]),])
	}

	#[inline(always)]
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
		let a: [_; 2] = cast!(a);
		cast!([self.sqrt_f64x4(a[0]), self.sqrt_f64x4(a[1]),])
	}
}

impl V3 {
	binop_256_nosign!(avx: add, "Adds the elements of each lane of `a` and `b`.", f32 x 8, f64 x 4);

	binop_256_nosign!(avx2: add, "Adds the elements of each lane of `a` and `b`, with wrapping on overflow.", wrapping_add, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, u64 x 4, i64 x 4);

	binop_256!(avx: and, "Returns `a & b` for each bit in `a` and `b`.", f32 x 8, f64 x 4);

	binop_256_full!(avx2: and, "Returns `a & b` for each bit in `a` and `b`.", m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4);

	binop_256!(avx: andnot, "Returns `!a & b` for each bit in `a` and `b`.", f32 x 8, f64 x 4);

	binop_256_full!(avx2: andnot, "Returns `!a & b` for each bit in `a` and `b`.", m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4);

	binop_256!(avx2: sign, r#"Applies the sign of each element of `sign` to the corresponding lane in `a`.
- If `sign` is zero, the corresponding element is zeroed.
- If `sign` is positive, the corresponding element is returned as is.
- If `sign` is negative, the corresponding element is negated."#, apply_sign, i8 x 32, i16 x 16, i32 x 8);

	binop_256!(avx2: avg, "Computes `average(a, b)` for each lane of `a` and `b`.", average, u8 x 32, u16 x 16);

	unop_256!(avx: ceil, "Returns `ceil(a)` for each lane of `a`, rounding towards positive infinity.", f32 x 8, f64 x 4);

	binop_256_nosign!(avx2: cmpeq, "Compares the elements in each lane of `a` and `b` for equality.", cmp_eq, m8 x 32 => m8, u8 x 32 => m8, i8 x 32 => m8, m16 x 16 => m16, u16 x 16 => m16, i16 x 16 => m16, m32 x 8 => m32, u32 x 8 => m32, i32 x 8 => m32, m64 x 4 => m64, u64 x 4 => m64, i64 x 4 => m64);

	binop_256!(avx2: cmpgt, "Compares the elements in each lane of `a` and `b` for equality.", cmp_gt, i8 x 32 => m8, i16 x 16 => m16, i32 x 8 => m32, i64 x 4 => m64);

	binop_256!(avx: div, "Divides the elements of each lane of `a` and `b`.", f32 x 8, f64 x 4);

	unop_256!(avx: floor, "Rounds the elements of each lane of `a` to the nearest integer towards negative infinity.", f32 x 8, f64 x 4);

	binop_256_nosign!(avx: hadd, "[_mm_hadd_ps](core::arch::x86_64::_mm_hadd_ps)", horizontal_add_pack, f32 x 8, f64 x 4);

	binop_256_nosign!(avx2: hadd, "[_mm_hadd_ps](core::arch::x86_64::_mm_hadd_ps)", horizontal_add_pack, u16 x 16, i16 x 16, u32 x 8, i32 x 8);

	binop_256_nosign!(avx: hsub, "[_mm_hsub_ps](core::arch::x86_64::_mm_hsub_ps)", horizontal_sub_pack, f32 x 8, f64 x 4);

	binop_256_nosign!(avx2: hsub, "[_mm_hsub_ps](core::arch::x86_64::_mm_hsub_ps)", horizontal_sub_pack, u16 x 16, i16 x 16, u32 x 8, i32 x 8);

	binop_256!(avx: max, "Computes `max(a, b)`. for each lane in `a` and `b`.", f32 x 8, f64 x 4);

	binop_256!(avx2: max, "Computes `max(a, b)`. for each lane in `a` and `b`.", u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8);

	binop_256!(avx: min, "Computes `min(a, b)`. for each lane in `a` and `b`.", f32 x 8, f64 x 4);

	binop_256!(avx2: min, "Computes `min(a, b)`. for each lane in `a` and `b`.", u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8);

	binop_256!(avx: mul, "Computes `a * b` for each lane in `a` and `b`.", f32 x 8, f64 x 4);

	binop_256_nosign!(avx2: mullo, "Computes `a * b` for each lane in `a` and `b`, with wrapping overflow.", wrapping_mul, u16 x 16, i16 x 16, u32 x 8, i32 x 8);

	binop_256!(avx: or, "Returns `a | b` for each bit in `a` and `b`.", f32 x 8, f64 x 4);

	binop_256_full!(avx2: or, "Returns `a | b` for each bit in `a` and `b`.", m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4);

	binop_256!(avx2: adds, "Adds the elements of each lane of `a` and `b`, with saturation.", saturating_add, u8 x 32, i8 x 32, u16 x 16, i16 x 16);

	binop_256!(avx2: subs, "Subtracts the elements of each lane of `a` and `b`, with saturation.", saturating_sub, u8 x 32, i8 x 32, u16 x 16, i16 x 16);

	binop_256_nosign!(avx: sub, "Subtracts the elements of each lane of `a` and `b`.", f32 x 8, f64 x 4);

	binop_256_nosign!(avx2: sub, "Subtracts the elements of each lane of `a` and `b`, with wrapping overflow.", wrapping_sub, u8 x 32, i8 x 32, u16 x 16, i16 x 16, u32 x 8, i32 x 8, u64 x 4, i64 x 4);

	binop_256!(avx: addsub, "Alternatively subtracts and adds the elements of each lane of `a` and `b`.", subadd, f32 x 8, f64 x 4);

	unop_256!(avx2: abs, "Computes the unsigned absolute value of the elements of each lane of `a`.", unsigned_abs, i8 x 32, i16 x 16, i32 x 8);

	binop_256!(avx: xor, "Returns `a ^ b` for each bit in `a` and `b`.", f32 x 8, f64 x 4);

	binop_256_full!(avx2: xor, "Returns `a ^ b` for each bit in `a` and `b`.", m8 x 32, u8 x 32, i8 x 32, m16 x 16, u16 x 16, i16 x 16, m32 x 8, u32 x 8, i32 x 8, m64 x 4, u64 x 4, i64 x 4);

	/// Computes `abs(a)` for each lane of `a`.
	#[inline(always)]
	pub fn abs_f32x8(self, a: f32x8) -> f32x8 {
		self.and_f32x8(a, cast!(self.splat_u32x8((1 << 31) - 1)))
	}

	/// Computes `abs(a)` for each lane of `a`.
	#[inline(always)]
	pub fn abs_f64x4(self, a: f64x4) -> f64x4 {
		self.and_f64x4(a, cast!(self.splat_u64x4((1 << 63) - 1)))
	}

	/// Computes the approximate reciprocal of the elements of each lane of `a`.
	#[inline(always)]
	pub fn approx_reciprocal_f32x8(self, a: f32x8) -> f32x8 {
		cast!(self.avx._mm256_rcp_ps(cast!(a)))
	}

	/// Computes the approximate reciprocal of the square roots of the elements of each lane of `a`.
	#[inline(always)]
	pub fn approx_reciprocal_sqrt_f32x8(self, a: f32x8) -> f32x8 {
		cast!(self.avx._mm256_rsqrt_ps(cast!(a)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_EQ_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for equality.
	#[inline(always)]
	pub fn cmp_eq_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_EQ_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_GE_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_GE_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i16x16(self, a: i16x16, b: i16x16) -> m16x16 {
		self.not_m16x16(self.cmp_lt_i16x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i32x8(self, a: i32x8, b: i32x8) -> m32x8 {
		self.not_m32x8(self.cmp_lt_i32x8(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i64x4(self, a: i64x4, b: i64x4) -> m64x4 {
		self.not_m64x4(self.cmp_lt_i64x4(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_i8x32(self, a: i8x32, b: i8x32) -> m8x32 {
		self.not_m8x32(self.cmp_lt_i8x32(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u16x16(self, a: u16x16, b: u16x16) -> m16x16 {
		self.not_m16x16(self.cmp_lt_u16x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u32x8(self, a: u32x8, b: u32x8) -> m32x8 {
		self.not_m32x8(self.cmp_lt_u32x8(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u64x4(self, a: u64x4, b: u64x4) -> m64x4 {
		self.not_m64x4(self.cmp_lt_u64x4(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_ge_u8x32(self, a: u8x32, b: u8x32) -> m8x32 {
		self.not_m8x32(self.cmp_lt_u8x32(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_GT_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_GT_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u16x16(self, a: u16x16, b: u16x16) -> m16x16 {
		let k = self.splat_u16x16(0x8000);
		self.cmp_gt_i16x16(cast!(self.xor_u16x16(a, k)), cast!(self.xor_u16x16(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u32x8(self, a: u32x8, b: u32x8) -> m32x8 {
		let k = self.splat_u32x8(0x80000000);
		self.cmp_gt_i32x8(cast!(self.xor_u32x8(a, k)), cast!(self.xor_u32x8(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u64x4(self, a: u64x4, b: u64x4) -> m64x4 {
		let k = self.splat_u64x4(0x8000000000000000);
		self.cmp_gt_i64x4(cast!(self.xor_u64x4(a, k)), cast!(self.xor_u64x4(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for greater-than.
	#[inline(always)]
	pub fn cmp_gt_u8x32(self, a: u8x32, b: u8x32) -> m8x32 {
		let k = self.splat_u8x32(0x80);
		self.cmp_gt_i8x32(cast!(self.xor_u8x32(a, k)), cast!(self.xor_u8x32(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_LE_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_LE_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i16x16(self, a: i16x16, b: i16x16) -> m16x16 {
		self.not_m16x16(self.cmp_gt_i16x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i32x8(self, a: i32x8, b: i32x8) -> m32x8 {
		self.not_m32x8(self.cmp_gt_i32x8(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i64x4(self, a: i64x4, b: i64x4) -> m64x4 {
		self.not_m64x4(self.cmp_gt_i64x4(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_i8x32(self, a: i8x32, b: i8x32) -> m8x32 {
		self.not_m8x32(self.cmp_gt_i8x32(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u16x16(self, a: u16x16, b: u16x16) -> m16x16 {
		self.not_m16x16(self.cmp_gt_u16x16(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u32x8(self, a: u32x8, b: u32x8) -> m32x8 {
		self.not_m32x8(self.cmp_gt_u32x8(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u64x4(self, a: u64x4, b: u64x4) -> m64x4 {
		self.not_m64x4(self.cmp_gt_u64x4(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than-or-equal-to.
	#[inline(always)]
	pub fn cmp_le_u8x32(self, a: u8x32, b: u8x32) -> m8x32 {
		self.not_m8x32(self.cmp_gt_u8x32(a, b))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_LT_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_LT_OQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i16x16(self, a: i16x16, b: i16x16) -> m16x16 {
		cast!(self.avx2._mm256_cmpgt_epi16(cast!(b), cast!(a)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i32x8(self, a: i32x8, b: i32x8) -> m32x8 {
		cast!(self.avx2._mm256_cmpgt_epi32(cast!(b), cast!(a)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i64x4(self, a: i64x4, b: i64x4) -> m64x4 {
		cast!(self.avx2._mm256_cmpgt_epi64(cast!(b), cast!(a)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_i8x32(self, a: i8x32, b: i8x32) -> m8x32 {
		cast!(self.avx2._mm256_cmpgt_epi8(cast!(b), cast!(a)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u16x16(self, a: u16x16, b: u16x16) -> m16x16 {
		let k = self.splat_u16x16(0x8000);
		self.cmp_lt_i16x16(cast!(self.xor_u16x16(a, k)), cast!(self.xor_u16x16(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u32x8(self, a: u32x8, b: u32x8) -> m32x8 {
		let k = self.splat_u32x8(0x80000000);
		self.cmp_lt_i32x8(cast!(self.xor_u32x8(a, k)), cast!(self.xor_u32x8(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u64x4(self, a: u64x4, b: u64x4) -> m64x4 {
		let k = self.splat_u64x4(0x8000000000000000);
		self.cmp_lt_i64x4(cast!(self.xor_u64x4(a, k)), cast!(self.xor_u64x4(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for less-than.
	#[inline(always)]
	pub fn cmp_lt_u8x32(self, a: u8x32, b: u8x32) -> m8x32 {
		let k = self.splat_u8x32(0x80);
		self.cmp_lt_i8x32(cast!(self.xor_u8x32(a, k)), cast!(self.xor_u8x32(b, k)))
	}

	/// Compares the elements in each lane of `a` and `b` for inequality.
	#[inline(always)]
	pub fn cmp_not_eq_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_NEQ_UQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for inequality.
	#[inline(always)]
	pub fn cmp_not_eq_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_NEQ_UQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_ge_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_NGE_UQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_ge_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_NGE_UQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than.
	#[inline(always)]
	pub fn cmp_not_gt_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_NGT_UQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-greater-than.
	#[inline(always)]
	pub fn cmp_not_gt_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_NGT_UQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_le_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_NLE_UQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than-or-equal.
	#[inline(always)]
	pub fn cmp_not_le_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_NLE_UQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f32x8(self, a: f32x8, b: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_NLT_UQ>(cast!(a), cast!(b)))
	}

	/// Compares the elements in each lane of `a` and `b` for not-less-than.
	#[inline(always)]
	pub fn cmp_not_lt_f64x4(self, a: f64x4, b: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_NLT_UQ>(cast!(a), cast!(b)))
	}

	/// Converts a `f32x4` to `f64x4`, elementwise.
	#[inline(always)]
	pub fn convert_f32x4_to_f64x4(self, a: f32x4) -> f64x4 {
		cast!(self.avx._mm256_cvtps_pd(cast!(a)))
	}

	/// Converts a `f32x8` to `i32x8`, elementwise.
	#[inline(always)]
	pub fn convert_f32x8_to_i32x8(self, a: f32x8) -> i32x8 {
		cast!(self.avx._mm256_cvttps_epi32(cast!(a)))
	}

	/// Converts a `f64x4` to `f32x4`, elementwise.
	#[inline(always)]
	pub fn convert_f64x4_to_f32x4(self, a: f64x4) -> f32x4 {
		cast!(self.avx._mm256_cvtpd_ps(cast!(a)))
	}

	/// Converts a `f64x4` to `i32x4`, elementwise.
	#[inline(always)]
	pub fn convert_f64x4_to_i32x4(self, a: f64x4) -> i32x4 {
		cast!(self.avx._mm256_cvttpd_epi32(cast!(a)))
	}

	/// Converts a `i16x16` to `u16x16`, elementwise.
	#[inline(always)]
	pub fn convert_i16x16_to_u16x16(self, a: i16x16) -> u16x16 {
		cast!(a)
	}

	/// Converts a `i16x8` to `i32x8`, elementwise.
	#[inline(always)]
	pub fn convert_i16x8_to_i32x8(self, a: i16x8) -> i32x8 {
		cast!(self.avx2._mm256_cvtepi16_epi32(cast!(a)))
	}

	/// Converts a `i16x8` to `i64x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i16x8_to_i64x4(self, a: i16x8) -> i64x4 {
		cast!(self.avx2._mm256_cvtepi16_epi64(cast!(a)))
	}

	/// Converts a `i16x8` to `u32x8`, elementwise.
	#[inline(always)]
	pub fn convert_i16x8_to_u32x8(self, a: i16x8) -> u32x8 {
		cast!(self.avx2._mm256_cvtepi16_epi32(cast!(a)))
	}

	/// Converts a `i16x8` to `u64x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i16x8_to_u64x4(self, a: i16x8) -> u64x4 {
		cast!(self.avx2._mm256_cvtepi16_epi64(cast!(a)))
	}

	/// Converts a `i32x4` to `f64x4`, elementwise.
	#[inline(always)]
	pub fn convert_i32x4_to_f64x4(self, a: i32x4) -> f64x4 {
		cast!(self.avx._mm256_cvtepi32_pd(cast!(a)))
	}

	/// Converts a `i32x4` to `i64x4`, elementwise.
	#[inline(always)]
	pub fn convert_i32x4_to_i64x4(self, a: i32x4) -> i64x4 {
		cast!(self.avx2._mm256_cvtepi32_epi64(cast!(a)))
	}

	/// Converts a `i32x4` to `u64x4`, elementwise.
	#[inline(always)]
	pub fn convert_i32x4_to_u64x4(self, a: i32x4) -> u64x4 {
		cast!(self.avx2._mm256_cvtepi32_epi64(cast!(a)))
	}

	/// Converts a `i32x8` to `f32x8`, elementwise.
	#[inline(always)]
	pub fn convert_i32x8_to_f32x8(self, a: i32x8) -> f32x8 {
		cast!(self.avx._mm256_cvtepi32_ps(cast!(a)))
	}

	/// Converts a `i32x8` to `u32x8`, elementwise.
	#[inline(always)]
	pub fn convert_i32x8_to_u32x8(self, a: i32x8) -> u32x8 {
		cast!(a)
	}

	/// Converts a `i8x16` to `i16x16`, elementwise.
	#[inline(always)]
	pub fn convert_i8x16_to_i16x16(self, a: i8x16) -> i16x16 {
		cast!(self.avx2._mm256_cvtepi8_epi16(cast!(a)))
	}

	/// Converts a `i8x16` to `i32x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_i32x8(self, a: i8x16) -> i32x8 {
		cast!(self.avx2._mm256_cvtepi8_epi32(cast!(a)))
	}

	/// Converts a `i8x16` to `i64x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_i64x4(self, a: i8x16) -> i64x4 {
		cast!(self.avx2._mm256_cvtepi8_epi64(cast!(a)))
	}

	/// Converts a `i8x16` to `u16x16`, elementwise.
	#[inline(always)]
	pub fn convert_i8x16_to_u16x16(self, a: i8x16) -> u16x16 {
		cast!(self.avx2._mm256_cvtepi8_epi16(cast!(a)))
	}

	/// Converts a `i8x16` to `u32x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_u32x8(self, a: i8x16) -> u32x8 {
		cast!(self.avx2._mm256_cvtepi8_epi32(cast!(a)))
	}

	/// Converts a `i8x16` to `u64x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_i8x16_to_u64x4(self, a: i8x16) -> u64x4 {
		cast!(self.avx2._mm256_cvtepi8_epi64(cast!(a)))
	}

	/// Converts a `i8x32` to `u8x32`, elementwise.
	#[inline(always)]
	pub fn convert_i8x32_to_u8x32(self, a: i8x32) -> u8x32 {
		cast!(a)
	}

	/// Converts a `u16x16` to `i16x16`, elementwise.
	#[inline(always)]
	pub fn convert_u16x16_to_i16x16(self, a: u16x16) -> i16x16 {
		cast!(a)
	}

	/// Converts a `u16x8` to `i32x8`, elementwise.
	#[inline(always)]
	pub fn convert_u16x8_to_i32x8(self, a: u16x8) -> i32x8 {
		cast!(self.avx2._mm256_cvtepu16_epi32(cast!(a)))
	}

	/// Converts a `u16x8` to `i64x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u16x8_to_i64x4(self, a: u16x8) -> i64x4 {
		cast!(self.avx2._mm256_cvtepu16_epi64(cast!(a)))
	}

	/// Converts a `u16x8` to `u32x8`, elementwise.
	#[inline(always)]
	pub fn convert_u16x8_to_u32x8(self, a: u16x8) -> u32x8 {
		cast!(self.avx2._mm256_cvtepu16_epi32(cast!(a)))
	}

	/// Converts a `u16x8` to `u64x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u16x8_to_u64x4(self, a: u16x8) -> u64x4 {
		cast!(self.avx2._mm256_cvtepu16_epi64(cast!(a)))
	}

	/// Converts a `u32x4` to `i64x4`, elementwise.
	#[inline(always)]
	pub fn convert_u32x4_to_i64x4(self, a: u32x4) -> i64x4 {
		cast!(self.avx2._mm256_cvtepu32_epi64(cast!(a)))
	}

	/// Converts a `u32x4` to `u64x4`, elementwise.
	#[inline(always)]
	pub fn convert_u32x4_to_u64x4(self, a: u32x4) -> u64x4 {
		cast!(self.avx2._mm256_cvtepu32_epi64(cast!(a)))
	}

	/// Converts a `u32x8` to `i32x8`, elementwise.
	#[inline(always)]
	pub fn convert_u32x8_to_i32x8(self, a: u32x8) -> i32x8 {
		cast!(a)
	}

	/// Converts a `u8x16` to `i16x16`, elementwise.
	#[inline(always)]
	pub fn convert_u8x16_to_i16x16(self, a: u8x16) -> i16x16 {
		cast!(self.avx2._mm256_cvtepu8_epi16(cast!(a)))
	}

	/// Converts a `u8x16` to `i32x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_i32x8(self, a: u8x16) -> i32x8 {
		cast!(self.avx2._mm256_cvtepu8_epi32(cast!(a)))
	}

	/// Converts a `u8x16` to `i64x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_i64x4(self, a: u8x16) -> i64x4 {
		cast!(self.avx2._mm256_cvtepu8_epi64(cast!(a)))
	}

	/// Converts a `u8x16` to `u16x16`, elementwise.
	#[inline(always)]
	pub fn convert_u8x16_to_u16x16(self, a: u8x16) -> u16x16 {
		cast!(self.avx2._mm256_cvtepu8_epi16(cast!(a)))
	}

	/// Converts a `u8x16` to `u32x8`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_u32x8(self, a: u8x16) -> u32x8 {
		cast!(self.avx2._mm256_cvtepu8_epi32(cast!(a)))
	}

	/// Converts a `u8x16` to `u64x4`, elementwise, while truncating the extra elements.
	#[inline(always)]
	pub fn convert_u8x16_to_u64x4(self, a: u8x16) -> u64x4 {
		cast!(self.avx2._mm256_cvtepu8_epi64(cast!(a)))
	}

	/// Converts a `u8x32` to `i8x32`, elementwise.
	#[inline(always)]
	pub fn convert_u8x32_to_i8x32(self, a: u8x32) -> i8x32 {
		cast!(a)
	}

	/// See [_mm_hadds_epi16].
	///
	/// [_mm_hadds_epi16]: core::arch::x86_64::_mm_hadds_epi16
	#[inline(always)]
	pub fn horizontal_saturating_add_pack_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
		cast!(self.avx2._mm256_hadds_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm_hsubs_epi16].
	///
	/// [_mm_hsubs_epi16]: core::arch::x86_64::_mm_hsubs_epi16
	#[inline(always)]
	pub fn horizontal_saturating_sub_pack_i16x16(self, a: i16x16, b: i16x16) -> i16x16 {
		cast!(self.avx2._mm256_hsubs_epi16(cast!(a), cast!(b)))
	}

	/// Checks if the elements in each lane of `a` are NaN.
	#[inline(always)]
	pub fn is_nan_f32x8(self, a: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_UNORD_Q>(cast!(a), cast!(a)))
	}

	/// Checks if the elements in each lane of `a` are NaN.
	#[inline(always)]
	pub fn is_nan_f64x4(self, a: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_UNORD_Q>(cast!(a), cast!(a)))
	}

	/// Checks if the elements in each lane of `a` are not NaN.
	#[inline(always)]
	pub fn is_not_nan_f32x8(self, a: f32x8) -> m32x8 {
		cast!(self.avx._mm256_cmp_ps::<_CMP_ORD_Q>(cast!(a), cast!(a)))
	}

	/// Checks if the elements in each lane of `a` are not NaN.
	#[inline(always)]
	pub fn is_not_nan_f64x4(self, a: f64x4) -> m64x4 {
		cast!(self.avx._mm256_cmp_pd::<_CMP_ORD_Q>(cast!(a), cast!(a)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
	/// `c`.
	#[inline(always)]
	pub fn mul_add_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
		cast!(self.fma._mm_fmadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
	/// `c`.
	#[inline(always)]
	pub fn mul_add_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
		cast!(self.fma._mm256_fmadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
	/// `c`.
	#[inline(always)]
	pub fn mul_add_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
		cast!(self.fma._mm_fmadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and adds the results to each lane of
	/// `c`.
	#[inline(always)]
	pub fn mul_add_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
		cast!(self.fma._mm256_fmadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_addsub_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
		cast!(self.fma._mm_fmsubadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_addsub_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
		cast!(self.fma._mm256_fmsubadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_addsub_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
		cast!(self.fma._mm_fmsubadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively adds/subtracts 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_addsub_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
		cast!(self.fma._mm256_fmsubadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the results.
	#[inline(always)]
	pub fn mul_sub_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
		cast!(self.fma._mm_fmsub_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the results.
	#[inline(always)]
	pub fn mul_sub_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
		cast!(self.fma._mm256_fmsub_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the results.
	#[inline(always)]
	pub fn mul_sub_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
		cast!(self.fma._mm_fmsub_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the results.
	#[inline(always)]
	pub fn mul_sub_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
		cast!(self.fma._mm256_fmsub_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_subadd_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
		cast!(self.fma._mm_fmaddsub_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_subadd_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
		cast!(self.fma._mm256_fmaddsub_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_subadd_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
		cast!(self.fma._mm_fmaddsub_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and alternatively subtracts/adds 'c'
	/// to/from the results.
	#[inline(always)]
	pub fn mul_subadd_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
		cast!(self.fma._mm256_fmaddsub_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// See [_mm256_maddubs_epi16].
	///
	/// [_mm256_maddubs_epi16]: core::arch::x86_64::_mm256_maddubs_epi16
	#[inline(always)]
	pub fn multiply_saturating_add_adjacent_i8x32(self, a: i8x32, b: i8x32) -> i16x16 {
		cast!(self.avx2._mm256_maddubs_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm256_madd_epi16].
	///
	/// [_mm256_madd_epi16]: core::arch::x86_64::_mm256_madd_epi16
	#[inline(always)]
	pub fn multiply_wrapping_add_adjacent_i16x16(self, a: i16x16, b: i16x16) -> i32x8 {
		cast!(self.avx2._mm256_madd_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm256_mpsadbw_epu8].
	///
	/// [_mm256_mpsadbw_epu8]: core::arch::x86_64::_mm256_mpsadbw_epu8
	#[inline(always)]
	pub fn multisum_of_absolute_differences_u8x32<const OFFSETS: i32>(
		self,
		a: u8x32,
		b: u8x32,
	) -> u16x16 {
		cast!(self.avx2._mm256_mpsadbw_epu8::<OFFSETS>(cast!(a), cast!(b)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
	/// each lane of `c`.
	#[inline(always)]
	pub fn negate_mul_add_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
		cast!(self.fma._mm_fnmadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
	/// each lane of `c`.
	#[inline(always)]
	pub fn negate_mul_add_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
		cast!(self.fma._mm256_fnmadd_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
	/// each lane of `c`.
	#[inline(always)]
	pub fn negate_mul_add_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
		cast!(self.fma._mm_fnmadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, negates the results, and adds them to
	/// each lane of `c`.
	#[inline(always)]
	pub fn negate_mul_add_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
		cast!(self.fma._mm256_fnmadd_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the negation of the results.
	#[inline(always)]
	pub fn negate_mul_sub_f32x4(self, a: f32x4, b: f32x4, c: f32x4) -> f32x4 {
		cast!(self.fma._mm_fnmsub_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the negation of the results.
	#[inline(always)]
	pub fn negate_mul_sub_f32x8(self, a: f32x8, b: f32x8, c: f32x8) -> f32x8 {
		cast!(self.fma._mm256_fnmsub_ps(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the negation of the results.
	#[inline(always)]
	pub fn negate_mul_sub_f64x2(self, a: f64x2, b: f64x2, c: f64x2) -> f64x2 {
		cast!(self.fma._mm_fnmsub_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Multiplies the elements in each lane of `a` and `b`, and subtracts each lane of `c` from
	/// the negation of the results.
	#[inline(always)]
	pub fn negate_mul_sub_f64x4(self, a: f64x4, b: f64x4, c: f64x4) -> f64x4 {
		cast!(self.fma._mm256_fnmsub_pd(cast!(a), cast!(b), cast!(c)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_i16x16(self, a: i16x16) -> i16x16 {
		self.xor_i16x16(a, self.splat_i16x16(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_i32x8(self, a: i32x8) -> i32x8 {
		self.xor_i32x8(a, self.splat_i32x8(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_i64x4(self, a: i64x4) -> i64x4 {
		self.xor_i64x4(a, self.splat_i64x4(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_i8x32(self, a: i8x32) -> i8x32 {
		self.xor_i8x32(a, self.splat_i8x32(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_m16x16(self, a: m16x16) -> m16x16 {
		self.xor_m16x16(a, self.splat_m16x16(m16::new(true)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_m32x8(self, a: m32x8) -> m32x8 {
		self.xor_m32x8(a, self.splat_m32x8(m32::new(true)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_m64x4(self, a: m64x4) -> m64x4 {
		self.xor_m64x4(a, self.splat_m64x4(m64::new(true)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_m8x32(self, a: m8x32) -> m8x32 {
		self.xor_m8x32(a, self.splat_m8x32(m8::new(true)))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_u16x16(self, a: u16x16) -> u16x16 {
		self.xor_u16x16(a, self.splat_u16x16(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_u32x8(self, a: u32x8) -> u32x8 {
		self.xor_u32x8(a, self.splat_u32x8(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_u64x4(self, a: u64x4) -> u64x4 {
		self.xor_u64x4(a, self.splat_u64x4(!0))
	}

	/// Returns `!a` for each bit in a.
	#[inline(always)]
	pub fn not_u8x32(self, a: u8x32) -> u8x32 {
		self.xor_u8x32(a, self.splat_u8x32(!0))
	}

	/// See [_mm256_packs_epi16].
	///
	/// [_mm256_packs_epi16]: core::arch::x86_64::_mm256_packs_epi16
	#[inline(always)]
	pub fn pack_with_signed_saturation_i16x16(self, a: i16x16, b: i16x16) -> i8x32 {
		cast!(self.avx2._mm256_packs_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm256_packs_epi32].
	///
	/// [_mm256_packs_epi32]: core::arch::x86_64::_mm256_packs_epi32
	#[inline(always)]
	pub fn pack_with_signed_saturation_i32x8(self, a: i32x8, b: i32x8) -> i16x16 {
		cast!(self.avx2._mm256_packs_epi32(cast!(a), cast!(b)))
	}

	/// See [_mm256_packus_epi16].
	///
	/// [_mm256_packus_epi16]: core::arch::x86_64::_mm256_packus_epi16
	#[inline(always)]
	pub fn pack_with_unsigned_saturation_i16x16(self, a: i16x16, b: i16x16) -> u8x32 {
		cast!(self.avx2._mm256_packus_epi16(cast!(a), cast!(b)))
	}

	/// See [_mm256_packus_epi32].
	///
	/// [_mm256_packus_epi32]: core::arch::x86_64::_mm256_packus_epi32
	#[inline(always)]
	pub fn pack_with_unsigned_saturation_i32x8(self, a: i32x8, b: i32x8) -> u16x16 {
		cast!(self.avx2._mm256_packus_epi32(cast!(a), cast!(b)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
	/// close, the even value is returned.
	#[inline(always)]
	pub fn round_f32x8(self, a: f32x8) -> f32x8 {
		const ROUNDING: i32 = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC;
		cast!(self.avx._mm256_round_ps::<ROUNDING>(cast!(a)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer. If two values are equally
	/// close, the even value is returned.
	#[inline(always)]
	pub fn round_f64x4(self, a: f64x4) -> f64x4 {
		const ROUNDING: i32 = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC;
		cast!(self.avx._mm256_round_pd::<ROUNDING>(cast!(a)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_f32x8<const MASK8: i32>(self, if_true: f32x8, if_false: f32x8) -> f32x8 {
		cast!(
			self.avx
				._mm256_blend_ps::<MASK8>(cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_f64x4<const MASK4: i32>(self, if_true: f64x4, if_false: f64x4) -> f64x4 {
		cast!(self.select_const_u64x4::<MASK4>(cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_i32x8<const MASK8: i32>(self, if_true: i32x8, if_false: i32x8) -> i32x8 {
		cast!(self.select_const_u32x8::<MASK8>(cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_i64x4<const MASK4: i32>(self, if_true: i64x4, if_false: i64x4) -> i64x4 {
		cast!(self.select_const_u64x4::<MASK4>(cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_u32x8<const MASK8: i32>(self, if_true: u32x8, if_false: u32x8) -> u32x8 {
		cast!(
			self.avx2
				._mm256_blend_epi32::<MASK8>(cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// bit in the mask is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_const_u64x4<const MASK4: i32>(self, if_true: u64x4, if_false: u64x4) -> u64x4 {
		cast!(
			self.avx
				._mm256_blend_pd::<MASK4>(cast!(if_false), cast!(if_true)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f32x8(self, mask: m32x8, if_true: f32x8, if_false: f32x8) -> f32x8 {
		cast!(
			self.avx
				._mm256_blendv_ps(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_f64x4(self, mask: m64x4, if_true: f64x4, if_false: f64x4) -> f64x4 {
		cast!(
			self.avx
				._mm256_blendv_pd(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i16x16(self, mask: m16x16, if_true: i16x16, if_false: i16x16) -> i16x16 {
		cast!(self.select_u16x16(mask, cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i32x8(self, mask: m32x8, if_true: i32x8, if_false: i32x8) -> i32x8 {
		cast!(self.select_u32x8(mask, cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i64x4(self, mask: m64x4, if_true: i64x4, if_false: i64x4) -> i64x4 {
		cast!(self.select_u64x4(mask, cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_i8x32(self, mask: m8x32, if_true: i8x32, if_false: i8x32) -> i8x32 {
		cast!(self.select_u8x32(mask, cast!(if_true), cast!(if_false)))
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u16x16(self, mask: m16x16, if_true: u16x16, if_false: u16x16) -> u16x16 {
		cast!(
			self.avx2
				._mm256_blendv_epi8(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u32x8(self, mask: m32x8, if_true: u32x8, if_false: u32x8) -> u32x8 {
		cast!(
			self.avx2
				._mm256_blendv_epi8(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u64x4(self, mask: m64x4, if_true: u64x4, if_false: u64x4) -> u64x4 {
		cast!(
			self.avx2
				._mm256_blendv_epi8(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Combines `if_true` and `if_false`, selecting elements from `if_true` if the corresponding
	/// mask in `mask` is set, otherwise selecting elements from `if_false`.
	#[inline(always)]
	pub fn select_u8x32(self, mask: m8x32, if_true: u8x32, if_false: u8x32) -> u8x32 {
		cast!(
			self.avx2
				._mm256_blendv_epi8(cast!(if_false), cast!(if_true), cast!(mask)),
		)
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_i16x16<const AMOUNT: i32>(self, a: i16x16) -> i16x16 {
		cast!(self.avx2._mm256_slli_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_i32x8<const AMOUNT: i32>(self, a: i32x8) -> i32x8 {
		cast!(self.avx2._mm256_slli_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_i64x4<const AMOUNT: i32>(self, a: i64x4) -> i64x4 {
		cast!(self.avx2._mm256_slli_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_u16x16<const AMOUNT: i32>(self, a: u16x16) -> u16x16 {
		cast!(self.avx2._mm256_slli_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_u32x8<const AMOUNT: i32>(self, a: u32x8) -> u32x8 {
		cast!(self.avx2._mm256_slli_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_const_u64x4<const AMOUNT: i32>(self, a: u64x4) -> u64x4 {
		cast!(self.avx2._mm256_slli_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_i32x4(self, a: i32x4, amount: u32x4) -> i32x4 {
		cast!(self.avx2._mm_sllv_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_i32x8(self, a: i32x8, amount: u32x8) -> i32x8 {
		cast!(self.avx2._mm256_sllv_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_i64x2(self, a: i64x2, amount: u64x2) -> i64x2 {
		cast!(self.avx2._mm_sllv_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_i64x4(self, a: i64x4, amount: u64x4) -> i64x4 {
		cast!(self.avx2._mm256_sllv_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_u32x4(self, a: u32x4, amount: u32x4) -> u32x4 {
		cast!(self.avx2._mm_sllv_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_u32x8(self, a: u32x8, amount: u32x8) -> u32x8 {
		cast!(self.avx2._mm256_sllv_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_u64x2(self, a: u64x2, amount: u64x2) -> u64x2 {
		cast!(self.avx2._mm_sllv_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_dyn_u64x4(self, a: u64x4, amount: u64x4) -> u64x4 {
		cast!(self.avx2._mm256_sllv_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_i16x16(self, a: i16x16, amount: u64x2) -> i16x16 {
		cast!(self.avx2._mm256_sll_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_i32x8(self, a: i32x8, amount: u64x2) -> i32x8 {
		cast!(self.avx2._mm256_sll_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_i64x4(self, a: i64x4, amount: u64x2) -> i64x4 {
		cast!(self.avx2._mm256_sll_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_u16x16(self, a: u16x16, amount: u64x2) -> u16x16 {
		cast!(self.avx2._mm256_sll_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_u32x8(self, a: u32x8, amount: u64x2) -> u32x8 {
		cast!(self.avx2._mm256_sll_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the left by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shl_u64x4(self, a: u64x4, amount: u64x2) -> u64x4 {
		cast!(self.avx2._mm256_sll_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_const_i16x16<const AMOUNT: i32>(self, a: i16x16) -> i16x16 {
		cast!(self.avx2._mm256_srai_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in sign bits.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_const_i32x8<const AMOUNT: i32>(self, a: i32x8) -> i32x8 {
		cast!(self.avx2._mm256_srai_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_const_u16x16<const AMOUNT: i32>(self, a: u16x16) -> u16x16 {
		cast!(self.avx2._mm256_srli_epi16::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_const_u32x8<const AMOUNT: i32>(self, a: u32x8) -> u32x8 {
		cast!(self.avx2._mm256_srli_epi32::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by `AMOUNT`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_const_u64x4<const AMOUNT: i32>(self, a: u64x4) -> u64x4 {
		cast!(self.avx2._mm256_srli_epi64::<AMOUNT>(cast!(a)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in sign bits.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_dyn_i32x4(self, a: i32x4, amount: u32x4) -> i32x4 {
		cast!(self.avx2._mm_srav_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in sign bits.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_dyn_i32x8(self, a: i32x8, amount: u32x8) -> i32x8 {
		cast!(self.avx2._mm256_srav_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_dyn_u32x4(self, a: u32x4, amount: u32x4) -> u32x4 {
		cast!(self.avx2._mm_srlv_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_dyn_u32x8(self, a: u32x8, amount: u32x8) -> u32x8 {
		cast!(self.avx2._mm256_srlv_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_dyn_u64x2(self, a: u64x2, amount: u64x2) -> u64x2 {
		cast!(self.avx2._mm_srlv_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the element in the corresponding lane in
	/// `amount`, while shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_dyn_u64x4(self, a: u64x4, amount: u64x4) -> u64x4 {
		cast!(self.avx2._mm256_srlv_epi64(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_i16x16(self, a: i16x16, amount: u64x2) -> i16x16 {
		cast!(self.avx2._mm256_sra_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero if the
	/// sign bit is not set, and to `-1` if the sign bit is set.
	#[inline(always)]
	pub fn shr_i32x8(self, a: i32x8, amount: u64x2) -> i32x8 {
		cast!(self.avx2._mm256_sra_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_u16x16(self, a: u16x16, amount: u64x2) -> u16x16 {
		cast!(self.avx2._mm256_srl_epi16(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_u32x8(self, a: u32x8, amount: u64x2) -> u32x8 {
		cast!(self.avx2._mm256_srl_epi32(cast!(a), cast!(amount)))
	}

	/// Shift the bits of each lane of `a` to the right by the first element in `amount`, while
	/// shifting in zeros.
	/// Shifting by a value greater than the bit width of the type sets the result to zero.
	#[inline(always)]
	pub fn shr_u64x4(self, a: u64x4, amount: u64x2) -> u64x4 {
		cast!(self.avx2._mm256_srl_epi64(cast!(a), cast!(amount)))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_f32x8(self, value: f32) -> f32x8 {
		cast!(self.avx._mm256_set1_ps(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_f64x4(self, value: f64) -> f64x4 {
		cast!(self.avx._mm256_set1_pd(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i16x16(self, value: i16) -> i16x16 {
		cast!(self.avx._mm256_set1_epi16(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i32x8(self, value: i32) -> i32x8 {
		cast!(self.avx._mm256_set1_epi32(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i64x4(self, value: i64) -> i64x4 {
		cast!(self.avx._mm256_set1_epi64x(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_i8x32(self, value: i8) -> i8x32 {
		cast!(self.avx._mm256_set1_epi8(value))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m16x16(self, value: m16) -> m16x16 {
		cast!(self.avx._mm256_set1_epi16(value.0 as i16))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m32x8(self, value: m32) -> m32x8 {
		cast!(self.avx._mm256_set1_epi32(value.0 as i32))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m64x4(self, value: m64) -> m64x4 {
		cast!(self.avx._mm256_set1_epi64x(value.0 as i64))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_m8x32(self, value: m8) -> m8x32 {
		cast!(self.avx._mm256_set1_epi8(value.0 as i8))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u16x16(self, value: u16) -> u16x16 {
		cast!(self.avx._mm256_set1_epi16(value as i16))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u32x8(self, value: u32) -> u32x8 {
		cast!(self.avx._mm256_set1_epi32(value as i32))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u64x4(self, value: u64) -> u64x4 {
		cast!(self.avx._mm256_set1_epi64x(value as i64))
	}

	/// Returns a SIMD vector with all lanes set to the given value.
	#[inline(always)]
	pub fn splat_u8x32(self, value: u8) -> u8x32 {
		cast!(self.avx._mm256_set1_epi8(value as i8))
	}

	/// Computes the square roots of the elements of each lane of `a`.
	#[inline(always)]
	pub fn sqrt_f32x8(self, a: f32x8) -> f32x8 {
		cast!(self.avx._mm256_sqrt_ps(cast!(a)))
	}

	/// Computes the square roots of the elements of each lane of `a`.
	#[inline(always)]
	pub fn sqrt_f64x4(self, a: f64x4) -> f64x4 {
		cast!(self.avx._mm256_sqrt_pd(cast!(a)))
	}

	/// See [_mm256_sad_epu8].
	///
	/// [_mm256_sad_epu8]: core::arch::x86_64::_mm256_sad_epu8
	#[inline(always)]
	pub fn sum_of_absolute_differences_u8x32(self, a: u8x32, b: u8x32) -> u64x4 {
		cast!(self.avx2._mm256_sad_epu8(cast!(a), cast!(b)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards zero.
	#[inline(always)]
	pub fn truncate_f32x8(self, a: f32x8) -> f32x8 {
		const ROUNDING: i32 = _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC;
		cast!(self.avx._mm256_round_ps::<ROUNDING>(cast!(a)))
	}

	/// Rounds the elements of each lane of `a` to the nearest integer towards zero.
	#[inline(always)]
	pub fn truncate_f64x4(self, a: f64x4) -> f64x4 {
		const ROUNDING: i32 = _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC;
		cast!(self.avx._mm256_round_pd::<ROUNDING>(cast!(a)))
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_i16x16(self, a: i16x16, b: i16x16) -> (u16x16, i16x16) {
		(
			cast!(self.avx2._mm256_mullo_epi16(cast!(a), cast!(b))),
			cast!(self.avx2._mm256_mulhi_epi16(cast!(a), cast!(b))),
		)
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_i32x8(self, a: i32x8, b: i32x8) -> (u32x8, i32x8) {
		let a = cast!(a);
		let b = cast!(b);
		let avx2 = self.avx2;

		// a0b0_lo a0b0_hi a2b2_lo a2b2_hi
		let ab_evens = self.avx2._mm256_mul_epi32(a, b);
		// a1b1_lo a1b1_hi a3b3_lo a3b3_hi
		let ab_odds = self.avx2._mm256_mul_epi32(
			avx2._mm256_srli_epi64::<32>(a),
			avx2._mm256_srli_epi64::<32>(b),
		);

		let ab_lo = self.avx2._mm256_blend_epi32::<0b10101010>(
			// a0b0_lo xxxxxxx a2b2_lo xxxxxxx
			ab_evens,
			// xxxxxxx a1b1_lo xxxxxxx a3b3_lo
			avx2._mm256_slli_epi64::<32>(ab_odds),
		);
		let ab_hi = self.avx2._mm256_blend_epi32::<0b10101010>(
			// a0b0_hi xxxxxxx a2b2_hi xxxxxxx
			avx2._mm256_srli_epi64::<32>(ab_evens),
			// xxxxxxx a1b1_hi xxxxxxx a3b3_hi
			ab_odds,
		);

		(cast!(ab_lo), cast!(ab_hi))
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_u16x16(self, a: u16x16, b: u16x16) -> (u16x16, u16x16) {
		(
			cast!(self.avx2._mm256_mullo_epi16(cast!(a), cast!(b))),
			cast!(self.avx2._mm256_mulhi_epu16(cast!(a), cast!(b))),
		)
	}

	/// Multiplies the elements of each lane of `a` and `b`, and returns separately the low and
	/// high bits of the result.
	#[inline(always)]
	pub fn widening_mul_u32x8(self, a: u32x8, b: u32x8) -> (u32x8, u32x8) {
		let a = cast!(a);
		let b = cast!(b);
		let avx2 = self.avx2;

		// a0b0_lo a0b0_hi a2b2_lo a2b2_hi
		let ab_evens = avx2._mm256_mul_epu32(a, b);
		// a1b1_lo a1b1_hi a3b3_lo a3b3_hi
		let ab_odds = avx2._mm256_mul_epu32(
			avx2._mm256_srli_epi64::<32>(a),
			avx2._mm256_srli_epi64::<32>(b),
		);

		let ab_lo = self.avx2._mm256_blend_epi32::<0b10101010>(
			// a0b0_lo xxxxxxx a2b2_lo xxxxxxx
			ab_evens,
			// xxxxxxx a1b1_lo xxxxxxx a3b3_lo
			avx2._mm256_slli_epi64::<32>(ab_odds),
		);
		let ab_hi = self.avx2._mm256_blend_epi32::<0b10101010>(
			// a0b0_hi xxxxxxx a2b2_hi xxxxxxx
			avx2._mm256_srli_epi64::<32>(ab_evens),
			// xxxxxxx a1b1_hi xxxxxxx a3b3_hi
			ab_odds,
		);

		(cast!(ab_lo), cast!(ab_hi))
	}
}
