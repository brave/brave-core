use super::*;
use crate::core_arch::x86::Avx2;

#[cfg(target_arch = "x86")]
use core::arch::x86::*;
#[cfg(target_arch = "x86_64")]
use core::arch::x86_64::*;

mod v2;
mod v3;

#[cfg(feature = "nightly")]
#[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
mod v4;

pub use v2::*;
pub use v3::*;

#[cfg(feature = "nightly")]
#[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
pub use v4::*;

#[target_feature(enable = "avx,avx2")]
#[inline]
unsafe fn avx_ld_u32s(ptr: *const u32, f: unsafe extern "C" fn()) -> u32x8 {
	let ret: __m256;
	#[cfg(target_arch = "x86_64")]
	core::arch::asm! {
		"lea rcx, [rip + 2f]",
		"jmp {f}",
		"2:",
		f = in(reg) f,
		in("rax") ptr,
		out("rcx") _,
		out("ymm0") ret,
		out("ymm1") _,
	};

	#[cfg(target_arch = "x86")]
	core::arch::asm! {
		"lea ecx, [eip + 2f]",
		"jmp {f}",
		"2:",
		f = in(reg) f,
		in("eax") ptr,
		out("ecx") _,
		out("ymm0") ret,
		out("ymm1") _,
	};

	cast!(ret)
}

#[target_feature(enable = "avx,avx2")]
#[inline]
unsafe fn avx_st_u32s(ptr: *mut u32, value: u32x8, f: unsafe extern "C" fn()) {
	#[cfg(target_arch = "x86_64")]
	core::arch::asm! {
		"lea rcx, [rip + 2f]",
		"jmp {f}",
		"2:",
		f = in(reg) f,

		in("rax") ptr,
		out("rcx") _,
		inout("ymm0") cast::<_, __m256>(value) => _,
		out("ymm1") _,
	};

	#[cfg(target_arch = "x86")]
	core::arch::asm! {
		"lea ecx, [eip + 2f]",
		"jmp {f}",
		"2:",
		f = in(reg) f,

		in("eax") ptr,
		out("ecx") _,
		inout("ymm0") cast::<_, __m256>(value) => _,
		out("ymm1") _,
	};
}

/// x86 arch
#[derive(Debug, Clone, Copy)]
#[non_exhaustive]
#[repr(u8)]
pub enum Arch {
	Scalar = 0,

	#[cfg(feature = "x86-v3")]
	#[cfg_attr(docsrs, doc(cfg(feature = "x86-v3")))]
	V3(V3) = 1,

	#[cfg(feature = "nightly-x86-v4")]
	#[cfg_attr(docsrs, doc(cfg(feature = "nightly-x86-v4")))]
	V4(V4) = 2,
}

impl Arch {
	/// Detects the best available instruction set.
	#[inline]
	pub fn new() -> Self {
		#[cfg(feature = "nightly-x86-v4")]
		if let Some(simd) = V4::try_new() {
			return Self::V4(simd);
		}
		#[cfg(feature = "x86-v3")]
		if let Some(simd) = V3::try_new() {
			return Self::V3(simd);
		}
		Self::Scalar
	}

	/// Detects the best available instruction set.
	#[inline(always)]
	pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
		match self {
			#[cfg(feature = "nightly-x86-v4")]
			Arch::V4(simd) => Simd::vectorize(simd, op),
			#[cfg(feature = "x86-v3")]
			Arch::V3(simd) => Simd::vectorize(simd, op),

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

include!(concat!(env!("OUT_DIR"), "/x86_64_asm.rs"));

#[cfg(test)]
mod tests {
	extern crate alloc;

	use super::*;
	use alloc::vec;
	use alloc::vec::Vec;
	use assert_approx_eq::assert_approx_eq;
	use core::iter::zip;
	use rand::random;

	#[allow(unused_macros)]
	macro_rules! dbgx {
        () => {
            ::std::eprintln!("[{}:{}]", ::std::file!(), ::std::line!())
        };
        ($val:expr $(,)?) => {
            match $val {
                tmp => {
                    ::std::eprintln!("[{}:{}] {} = {:#X?}",
                        ::std::file!(), ::std::line!(), ::std::stringify!($val), &tmp);
                    tmp
                }
            }
        };
        ($($val:expr),+ $(,)?) => {
            ($(dbgx!($val)),+,)
        };
    }

	#[test]
	fn times_two() {
		let n = 1312;
		let mut v = (0..n).map(|i| i as f64).collect::<Vec<_>>();
		let arch = Arch::new();

		struct TimesThree<'a>(&'a mut [f64]);
		impl WithSimd for TimesThree<'_> {
			type Output = ();

			#[inline(always)]
			fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
				let v = self.0;
				let (head, tail) = S::as_mut_simd_f64s(v);

				let three = simd.splat_f64s(3.0);
				for x in head {
					*x = simd.mul_f64s(three, *x);
				}

				for x in tail {
					*x *= 3.0;
				}
			}
		}

		arch.dispatch(|| {
			for x in &mut v {
				*x *= 2.0;
			}
		});

		arch.dispatch(TimesThree(&mut v));

		for (i, x) in v.into_iter().enumerate() {
			assert_eq!(x, 6.0 * i as f64);
		}
	}

	#[test]
	fn cplx_ops() {
		let n = 16;
		let a = (0..n)
			.map(|_| c32 {
				re: random(),
				im: random(),
			})
			.collect::<Vec<_>>();
		let b = (0..n)
			.map(|_| c32 {
				re: random(),
				im: random(),
			})
			.collect::<Vec<_>>();
		let c = (0..n)
			.map(|_| c32 {
				re: random(),
				im: random(),
			})
			.collect::<Vec<_>>();

		let axb_target = zip(&a, &b).map(|(a, b)| a * b).collect::<Vec<_>>();
		let conjaxb_target = zip(&a, &b).map(|(a, b)| a.conj() * b).collect::<Vec<_>>();
		let axbpc_target = zip(zip(&a, &b), &c)
			.map(|((a, b), c)| a * b + c)
			.collect::<Vec<_>>();
		let conjaxbpc_target = zip(zip(&a, &b), &c)
			.map(|((a, b), c)| a.conj() * b + c)
			.collect::<Vec<_>>();

		if let Some(simd) = V3::try_new() {
			let mut axb = vec![c32::new(0.0, 0.0); n];
			let mut conjaxb = vec![c32::new(0.0, 0.0); n];
			let mut axbpc = vec![c32::new(0.0, 0.0); n];
			let mut conjaxbpc = vec![c32::new(0.0, 0.0); n];

			{
				let a = V3::as_simd_c32s(&a).0;
				let b = V3::as_simd_c32s(&b).0;
				let c = V3::as_simd_c32s(&c).0;
				let axb = V3::as_mut_simd_c32s(&mut axb).0;
				let conjaxb = V3::as_mut_simd_c32s(&mut conjaxb).0;
				let axbpc = V3::as_mut_simd_c32s(&mut axbpc).0;
				let conjaxbpc = V3::as_mut_simd_c32s(&mut conjaxbpc).0;

				for (axb, (a, b)) in zip(axb, zip(a, b)) {
					*axb = simd.mul_e_c32s(*a, *b);
				}
				for (conjaxb, (a, b)) in zip(conjaxb, zip(a, b)) {
					*conjaxb = simd.conj_mul_e_c32s(*a, *b);
				}
				for (axbpc, ((a, b), c)) in zip(axbpc, zip(zip(a, b), c)) {
					*axbpc = simd.mul_add_e_c32s(*a, *b, *c);
				}
				for (conjaxbpc, ((a, b), c)) in zip(conjaxbpc, zip(zip(a, b), c)) {
					*conjaxbpc = simd.conj_mul_add_e_c32s(*a, *b, *c);
				}
			}

			for (target, actual) in zip(&axb_target, &axb) {
				assert_approx_eq!(target.re, actual.re);
				assert_approx_eq!(target.im, actual.im);
			}
			for (target, actual) in zip(&conjaxb_target, &conjaxb) {
				assert_approx_eq!(target.re, actual.re);
				assert_approx_eq!(target.im, actual.im);
			}
			for (target, actual) in zip(&axbpc_target, &axbpc) {
				assert_approx_eq!(target.re, actual.re);
				assert_approx_eq!(target.im, actual.im);
			}
			for (target, actual) in zip(&conjaxbpc_target, &conjaxbpc) {
				assert_approx_eq!(target.re, actual.re);
				assert_approx_eq!(target.im, actual.im);
			}
		}

		#[cfg(feature = "nightly")]
		if let Some(simd) = V4::try_new() {
			let mut axb = vec![c32::new(0.0, 0.0); n];
			let mut conjaxb = vec![c32::new(0.0, 0.0); n];
			let mut axbpc = vec![c32::new(0.0, 0.0); n];
			let mut conjaxbpc = vec![c32::new(0.0, 0.0); n];

			{
				let a = V4::as_simd_c32s(&a).0;
				let b = V4::as_simd_c32s(&b).0;
				let c = V4::as_simd_c32s(&c).0;
				let axb = V4::as_mut_simd_c32s(&mut axb).0;
				let conjaxb = V4::as_mut_simd_c32s(&mut conjaxb).0;
				let axbpc = V4::as_mut_simd_c32s(&mut axbpc).0;
				let conjaxbpc = V4::as_mut_simd_c32s(&mut conjaxbpc).0;

				for (axb, (a, b)) in zip(axb, zip(a, b)) {
					*axb = simd.mul_e_c32s(*a, *b);
				}
				for (conjaxb, (a, b)) in zip(conjaxb, zip(a, b)) {
					*conjaxb = simd.conj_mul_e_c32s(*a, *b);
				}
				for (axbpc, ((a, b), c)) in zip(axbpc, zip(zip(a, b), c)) {
					*axbpc = simd.mul_add_e_c32s(*a, *b, *c);
				}
				for (conjaxbpc, ((a, b), c)) in zip(conjaxbpc, zip(zip(a, b), c)) {
					*conjaxbpc = simd.conj_mul_add_e_c32s(*a, *b, *c);
				}
			}

			for (target, actual) in zip(&axb_target, &axb) {
				assert_approx_eq!(target.re, actual.re);
				assert_approx_eq!(target.im, actual.im);
			}
			for (target, actual) in zip(&conjaxb_target, &conjaxb) {
				assert_approx_eq!(target.re, actual.re);
				assert_approx_eq!(target.im, actual.im);
			}
			for (target, actual) in zip(&axbpc_target, &axbpc) {
				assert_approx_eq!(target.re, actual.re);
				assert_approx_eq!(target.im, actual.im);
			}
			for (target, actual) in zip(&conjaxbpc_target, &conjaxbpc) {
				assert_approx_eq!(target.re, actual.re);
				assert_approx_eq!(target.im, actual.im);
			}
		}
	}

	#[test]
	fn test_to_ref() {
		let simd_ref = unsafe { V2::new_unchecked() }.to_ref();
		let _ = *simd_ref;
	}

	#[test]
	fn test_widening_mul_u32x4() {
		if let Some(simd) = V2::try_new() {
			const N: usize = 4;
			let a = u32x4(2298413717, 568259975, 2905436181, 175547995);
			let b = u32x4(2022374205, 1446824162, 3165580604, 3011091403);
			let a_array: [u32; N] = cast!(a);
			let b_array: [u32; N] = cast!(b);
			let mut lo_array = [0u32; N];
			let mut hi_array = [0u32; N];

			for i in 0..N {
				let prod = a_array[i] as u64 * b_array[i] as u64;
				let lo = prod as u32;
				let hi = (prod >> 32) as u32;
				lo_array[i] = lo;
				hi_array[i] = hi;
			}

			let (lo, hi) = simd.widening_mul_u32x4(a, b);
			assert_eq!(lo, cast!(lo_array));
			assert_eq!(hi, cast!(hi_array));
		}
		if let Some(simd) = V3::try_new() {
			const N: usize = 8;
			let a = u32x8(
				2298413717, 568259975, 2905436181, 175547995, 2298413717, 568259975, 2905436181,
				175547995,
			);
			let b = u32x8(
				2022374205, 1446824162, 3165580604, 3011091403, 2022374205, 1446824162, 3165580604,
				3011091403,
			);
			let a_array: [u32; N] = cast!(a);
			let b_array: [u32; N] = cast!(b);
			let mut lo_array = [0u32; N];
			let mut hi_array = [0u32; N];

			for i in 0..N {
				let prod = a_array[i] as u64 * b_array[i] as u64;
				let lo = prod as u32;
				let hi = (prod >> 32) as u32;
				lo_array[i] = lo;
				hi_array[i] = hi;
			}

			let (lo, hi) = simd.widening_mul_u32x8(a, b);
			assert_eq!(lo, cast!(lo_array));
			assert_eq!(hi, cast!(hi_array));
		}
	}

	#[test]
	fn test_widening_mul_i32() {
		if let Some(simd) = V2::try_new() {
			const N: usize = 4;
			let a = cast!(u32x4(2298413717, 568259975, 2905436181, 175547995));
			let b = cast!(u32x4(2022374205, 1446824162, 3165580604, 3011091403));

			let a_array: [i32; N] = cast!(a);
			let b_array: [i32; N] = cast!(b);
			let mut lo_array = [0i32; N];
			let mut hi_array = [0i32; N];

			for i in 0..N {
				let prod = a_array[i] as i64 * b_array[i] as i64;
				let lo = prod as i32;
				let hi = (prod >> 32) as i32;
				lo_array[i] = lo;
				hi_array[i] = hi;
			}

			let (lo, hi) = simd.widening_mul_i32x4(a, b);
			assert_eq!(lo, cast!(lo_array));
			assert_eq!(hi, cast!(hi_array));
		}
		if let Some(simd) = V3::try_new() {
			const N: usize = 8;
			let a = cast!(u32x8(
				2298413717, 568259975, 2905436181, 175547995, 2298413717, 568259975, 2905436181,
				175547995,
			));
			let b = cast!(u32x8(
				2022374205, 1446824162, 3165580604, 3011091403, 2022374205, 1446824162, 3165580604,
				3011091403,
			));

			let a_array: [i32; N] = cast!(a);
			let b_array: [i32; N] = cast!(b);
			let mut lo_array = [0i32; N];
			let mut hi_array = [0i32; N];

			for i in 0..N {
				let prod = a_array[i] as i64 * b_array[i] as i64;
				let lo = prod as i32;
				let hi = (prod >> 32) as i32;
				lo_array[i] = lo;
				hi_array[i] = hi;
			}

			let (lo, hi) = simd.widening_mul_i32x8(a, b);
			assert_eq!(lo, cast!(lo_array));
			assert_eq!(hi, cast!(hi_array));
		}
	}

	#[test]
	fn test_shift() {
		if let Some(simd) = V2::try_new() {
			let a = u16x8(54911, 46958, 49991, 22366, 46365, 39572, 22704, 60060);
			assert_eq!(simd.shl_const_u16x8::<16>(a), simd.splat_u16x8(0));
			assert_eq!(simd.shl_u16x8(a, simd.splat_u64x2(!0)), simd.splat_u16x8(0),);
		}
	}

	#[test]
	fn test_abs() {
		if let Some(simd) = V2::try_new() {
			let a = f32x4(1.0, -2.0, -1.0, 2.0);
			assert_eq!(simd.abs_f32x4(a), f32x4(1.0, 2.0, 1.0, 2.0));
			let a = f64x2(1.0, -2.0);
			assert_eq!(simd.abs_f64x2(a), f64x2(1.0, 2.0));
		}
	}

	#[test]
	fn test_subadd() {
		if let Some(simd) = V2::try_new() {
			let a = f32x4(1.0, -2.0, -1.0, 2.0);
			assert_eq!(simd.subadd_f32x4(a, a), f32x4(0.0, -4.0, 0.0, 4.0));
		}
	}

	#[test]
	fn test_signed_to_unsigned() {
		if let Some(simd) = V2::try_new() {
			let a = i8x16(1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
			assert_eq!(simd.convert_i8x16_to_u64x2(a), u64x2(1, !0));
		}
	}

	#[test]
	fn test_int_cmp() {
		if let Some(simd) = V2::try_new() {
			{
				const N: usize = 16;

				let a = u8x16(
					174, 191, 248, 232, 11, 186, 42, 236, 3, 59, 223, 72, 161, 146, 98, 69,
				);
				let b = u8x16(
					97, 239, 164, 173, 208, 0, 121, 247, 218, 58, 119, 131, 213, 133, 22, 128,
				);
				let lt = simd.cmp_lt_u8x16(a, b);

				let a_array: [u8; N] = cast!(a);
				let b_array: [u8; N] = cast!(b);
				let mut lt_array = [m8::new(false); N];

				for i in 0..N {
					lt_array[i] = m8::new(a_array[i] < b_array[i]);
				}

				assert_eq!(lt, cast!(lt_array));
			}
			{
				const N: usize = 8;

				let a = u16x8(174, 191, 248, 232, 11, 186, 42, 236);
				let b = u16x8(97, 239, 164, 173, 208, 0, 121, 247);
				let lt = simd.cmp_lt_u16x8(a, b);

				let a_array: [u16; N] = cast!(a);
				let b_array: [u16; N] = cast!(b);
				let mut lt_array = [m16::new(false); N];

				for i in 0..N {
					lt_array[i] = m16::new(a_array[i] < b_array[i]);
				}

				assert_eq!(lt, cast!(lt_array));
			}
			{
				const N: usize = 4;

				let a = u32x4(174, 191, 248, 232);
				let b = u32x4(97, 239, 164, 173);
				let lt = simd.cmp_lt_u32x4(a, b);

				let a_array: [u32; N] = cast!(a);
				let b_array: [u32; N] = cast!(b);
				let mut lt_array = [m32::new(false); N];

				for i in 0..N {
					lt_array[i] = m32::new(a_array[i] < b_array[i]);
				}

				assert_eq!(lt, cast!(lt_array));
			}
			{
				const N: usize = 2;

				let a = u64x2(174, 191);
				let b = u64x2(97, 239);
				let lt = simd.cmp_lt_u64x2(a, b);

				let a_array: [u64; N] = cast!(a);
				let b_array: [u64; N] = cast!(b);
				let mut lt_array = [m64::new(false); N];

				for i in 0..N {
					lt_array[i] = m64::new(a_array[i] < b_array[i]);
				}

				assert_eq!(lt, cast!(lt_array));
			}
		}
	}

	#[test]
	fn test_is_nan() {
		if let Some(simd) = V2::try_new() {
			assert_eq!(
				simd.is_nan_f32x4(f32x4(0.0, f32::NAN, f32::INFINITY, -f32::NAN)),
				m32x4(
					m32::new(false),
					m32::new(true),
					m32::new(false),
					m32::new(true),
				),
			);
			assert_eq!(
				simd.is_nan_f64x2(f64x2(0.0, f64::NAN)),
				m64x2(m64::new(false), m64::new(true)),
			);
		}
	}

	#[test]
	fn test_rotate() {
		if let Some(simd) = V3::try_new() {
			for amount in 0..128 {
				let mut array = [0u32; 8];
				for (i, dst) in array.iter_mut().enumerate() {
					*dst = 1000 + i as u32;
				}

				let rot: [u32; 8] = cast!(simd.rotate_right_u32s(cast!(array), amount));
				for i in 0..8 {
					assert_eq!(rot[(i + amount) % 8], array[i]);
				}
			}
			for amount in 0..128 {
				let mut array = [0u64; 4];
				for (i, dst) in array.iter_mut().enumerate() {
					*dst = 1000 + i as u64;
				}

				let rot: [u64; 4] = cast!(simd.rotate_right_u64s(cast!(array), amount));
				for i in 0..4 {
					assert_eq!(rot[(i + amount) % 4], array[i]);
				}
			}
		}

		#[cfg(feature = "nightly")]
		if let Some(simd) = V4::try_new() {
			for amount in 0..128 {
				let mut array = [0u32; 16];
				for (i, dst) in array.iter_mut().enumerate() {
					*dst = 1000 + i as u32;
				}

				let rot: [u32; 16] = cast!(simd.rotate_right_u32s(cast!(array), amount));
				for i in 0..16 {
					assert_eq!(rot[(i + amount) % 16], array[i]);
				}
			}
			for amount in 0..128 {
				let mut array = [0u64; 8];
				for (i, dst) in array.iter_mut().enumerate() {
					*dst = 1000 + i as u64;
				}

				let rot: [u64; 8] = cast!(simd.rotate_right_u64s(cast!(array), amount));
				for i in 0..8 {
					assert_eq!(rot[(i + amount) % 8], array[i]);
				}
			}
		}
	}

	#[test]
	fn test_partial() {
		if let Some(simd) = V3::try_new() {
			for n in 0..=8 {
				let src = core::array::from_fn::<f32, 8, _>(|i| i as _);
				let mut dst = [0.0f32; 8];
				let zero = dst;

				assert_eq!(simd.partial_load_f32s(&src[..n]), unsafe {
					simd.mask_load_ptr_f32s(simd.mask_between_m32s(0, n as u32), src.as_ptr())
				});
				{
					let src = &src[..n];
					let dst = &mut dst[..n];

					simd.partial_store_f32s(dst, simd.partial_load_f32s(src));

					assert_eq!(src, dst);
				}
				assert_eq!(dst[n..], zero[n..]);
			}
		}

		#[cfg(feature = "nightly")]
		if let Some(simd) = V4::try_new() {
			for n in 0..=16 {
				let src = core::array::from_fn::<f32, 16, _>(|i| i as _);
				let mut dst = [0.0f32; 16];
				let zero = dst;

				assert_eq!(simd.partial_load_f32s(&src[..n]), unsafe {
					simd.mask_load_ptr_f32s(simd.mask_between_m32s(0, n as u32), src.as_ptr())
				});

				{
					let src = &src[..n];
					let dst = &mut dst[..n];

					simd.partial_store_f32s(dst, simd.partial_load_f32s(src));

					assert_eq!(src, dst);
				}
				assert_eq!(dst[n..], zero[n..]);
			}
		}
	}

	#[test]
	fn test_interleave() {
		if let Some(simd) = V3::try_new() {
			{
				let src = [f64x4(0.0, 0.1, 1.0, 1.1), f64x4(2.0, 2.1, 3.0, 3.1)];
				let dst = simd.deinterleave_shfl_f64s(src);
				assert_eq!(dst[1], simd.add_f64x4(dst[0], simd.splat_f64x4(0.1)));
				assert_eq!(src, simd.interleave_shfl_f64s(dst));
			}
			{
				let src = [
					f64x4(0.0, 0.1, 0.2, 0.3),
					f64x4(1.0, 1.1, 1.2, 1.3),
					f64x4(2.0, 2.1, 2.2, 2.3),
					f64x4(3.0, 3.1, 3.2, 3.3),
				];
				let dst = simd.deinterleave_shfl_f64s(src);
				assert_eq!(dst[1], simd.add_f64x4(dst[0], simd.splat_f64x4(0.1)));
				assert_eq!(dst[2], simd.add_f64x4(dst[0], simd.splat_f64x4(0.2)));
				assert_eq!(dst[3], simd.add_f64x4(dst[0], simd.splat_f64x4(0.3)));
				assert_eq!(src, simd.interleave_shfl_f64s(dst));
			}
			{
				let src = [
					f32x8(0.0, 0.1, 1.0, 1.1, 2.0, 2.1, 3.0, 3.1),
					f32x8(4.0, 4.1, 5.0, 5.1, 6.0, 6.1, 7.0, 7.1),
				];
				let dst = simd.deinterleave_shfl_f32s(src);
				assert_eq!(dst[1], simd.add_f32x8(dst[0], simd.splat_f32x8(0.1)));
				assert_eq!(src, simd.interleave_shfl_f32s(dst));
			}
			{
				let src = [
					f32x8(0.0, 0.1, 0.2, 0.3, 1.0, 1.1, 1.2, 1.3),
					f32x8(2.0, 2.1, 2.2, 2.3, 3.0, 3.1, 3.2, 3.3),
					f32x8(4.0, 4.1, 4.2, 4.3, 5.0, 5.1, 5.2, 5.3),
					f32x8(6.0, 6.1, 6.2, 6.3, 7.0, 7.1, 7.2, 7.3),
				];
				let dst = simd.deinterleave_shfl_f32s(src);
				assert_eq!(dst[1], simd.add_f32x8(dst[0], simd.splat_f32x8(0.1)));
				assert_eq!(dst[2], simd.add_f32x8(dst[0], simd.splat_f32x8(0.2)));
				assert_eq!(dst[3], simd.add_f32x8(dst[0], simd.splat_f32x8(0.3)));
				assert_eq!(src, simd.interleave_shfl_f32s(dst));
			}
		}
		#[cfg(feature = "nightly")]
		if let Some(simd) = V4::try_new() {
			{
				let src = [
					f64x8(0.0, 0.1, 1.0, 1.1, 2.0, 2.1, 3.0, 3.1),
					f64x8(4.0, 4.1, 5.0, 5.1, 6.0, 6.1, 7.0, 7.1),
				];
				let dst = simd.deinterleave_shfl_f64s(src);
				assert_eq!(dst[1], simd.add_f64x8(dst[0], simd.splat_f64x8(0.1)));
				assert_eq!(src, simd.interleave_shfl_f64s(dst));
			}

			{
				let src = [
					f64x8(0.0, 0.1, 0.2, 1.0, 1.1, 1.2, 2.0, 2.1),
					f64x8(2.2, 3.0, 3.1, 3.2, 4.0, 4.1, 4.2, 5.0),
					f64x8(5.1, 5.2, 6.0, 6.1, 6.2, 7.0, 7.1, 7.2),
				];
				let dst = simd.deinterleave_shfl_f64s(src);
				assert_eq!(dst[1], simd.add_f64x8(dst[0], simd.splat_f64x8(0.1)));
				assert_eq!(dst[2], simd.add_f64x8(dst[0], simd.splat_f64x8(0.2)));
				assert_eq!(src, simd.interleave_shfl_f64s(dst));
			}
			{
				let src = [
					f64x8(0.0, 0.1, 0.2, 0.3, 1.0, 1.1, 1.2, 1.3),
					f64x8(2.0, 2.1, 2.2, 2.3, 3.0, 3.1, 3.2, 3.3),
					f64x8(4.0, 4.1, 4.2, 4.3, 5.0, 5.1, 5.2, 5.3),
					f64x8(6.0, 6.1, 6.2, 6.3, 7.0, 7.1, 7.2, 7.3),
				];
				let dst = simd.deinterleave_shfl_f64s(src);
				assert_eq!(dst[1], simd.add_f64x8(dst[0], simd.splat_f64x8(0.1)));
				assert_eq!(dst[2], simd.add_f64x8(dst[0], simd.splat_f64x8(0.2)));
				assert_eq!(dst[3], simd.add_f64x8(dst[0], simd.splat_f64x8(0.3)));
				assert_eq!(src, simd.interleave_shfl_f64s(dst));
			}

			{
				let src = [
					f32x16(
						0.0, 0.1, 1.0, 1.1, 2.0, 2.1, 3.0, 3.1, 4.0, 4.1, 5.0, 5.1, 6.0, 6.1, 7.0,
						7.1,
					),
					f32x16(
						8.0, 8.1, 9.0, 9.1, 10.0, 10.1, 11.0, 11.1, 12.0, 12.1, 13.0, 13.1, 14.0,
						14.1, 15.0, 15.1,
					),
				];
				let dst = simd.deinterleave_shfl_f32s(src);
				assert_eq!(dst[1], simd.add_f32x16(dst[0], simd.splat_f32x16(0.1)));
				assert_eq!(src, simd.interleave_shfl_f32s(dst));
			}
			{
				let src = [
					f32x16(
						0.0, 0.1, 0.2, 0.3, 1.0, 1.1, 1.2, 1.3, 2.0, 2.1, 2.2, 2.3, 3.0, 3.1, 3.2,
						3.3,
					),
					f32x16(
						4.0, 4.1, 4.2, 4.3, 5.0, 5.1, 5.2, 5.3, 6.0, 6.1, 6.2, 6.3, 7.0, 7.1, 7.2,
						7.3,
					),
					f32x16(
						8.0, 8.1, 8.2, 8.3, 9.0, 9.1, 9.2, 9.3, 10.0, 10.1, 10.2, 10.3, 11.0, 11.1,
						11.2, 11.3,
					),
					f32x16(
						12.0, 12.1, 12.2, 12.3, 13.0, 13.1, 13.2, 13.3, 14.0, 14.1, 14.2, 14.3,
						15.0, 15.1, 15.2, 15.3,
					),
				];
				let dst = simd.deinterleave_shfl_f32s(src);
				assert_eq!(dst[1], simd.add_f32x16(dst[0], simd.splat_f32x16(0.1)));
				assert_eq!(dst[2], simd.add_f32x16(dst[0], simd.splat_f32x16(0.2)));
				assert_eq!(dst[3], simd.add_f32x16(dst[0], simd.splat_f32x16(0.3)));
				assert_eq!(src, simd.interleave_shfl_f32s(dst));
			}
		}
	}
}
