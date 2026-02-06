//! `pulp` is a safe abstraction over SIMD instructions, that allows you to write a function once
//! and dispatch to equivalent vectorized versions based on the features detected at runtime.
//!
//! # Autovectorization example
//!
//! ```
//! use pulp::Arch;
//!
//! let mut v = (0..1000).map(|i| i as f64).collect::<Vec<_>>();
//! let arch = Arch::new();
//!
//! arch.dispatch(|| {
//! 	for x in &mut v {
//! 		*x *= 2.0;
//! 	}
//! });
//!
//! for (i, x) in v.into_iter().enumerate() {
//! 	assert_eq!(x, 2.0 * i as f64);
//! }
//! ```
//!
//! # Manual vectorization example
//!
//! ```
//! use pulp::{Arch, Simd, WithSimd};
//!
//! struct TimesThree<'a>(&'a mut [f64]);
//! impl<'a> WithSimd for TimesThree<'a> {
//! 	type Output = ();
//!
//! 	#[inline(always)]
//! 	fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
//! 		let v = self.0;
//! 		let (head, tail) = S::as_mut_simd_f64s(v);
//!
//! 		let three = simd.splat_f64s(3.0);
//! 		for x in head {
//! 			*x = simd.mul_f64s(three, *x);
//! 		}
//!
//! 		for x in tail {
//! 			*x = *x * 3.0;
//! 		}
//! 	}
//! }
//!
//! let mut v = (0..1000).map(|i| i as f64).collect::<Vec<_>>();
//! let arch = Arch::new();
//!
//! arch.dispatch(TimesThree(&mut v));
//!
//! for (i, x) in v.into_iter().enumerate() {
//! 	assert_eq!(x, 3.0 * i as f64);
//! }
//! ```

// FIXME: replace x86 non-ieee min/max functions to propagate nans instead

#![allow(
	non_camel_case_types,
	unknown_lints,
	clippy::zero_prefixed_literal,
	clippy::identity_op,
	clippy::too_many_arguments,
	clippy::type_complexity,
	clippy::missing_transmute_annotations,
	clippy::tabs_in_doc_comments,
	clippy::modulo_one,
	clippy::useless_transmute,
	clippy::not_unsafe_ptr_arg_deref,
	clippy::manual_is_multiple_of
)]
#![cfg_attr(
	all(feature = "nightly", any(target_arch = "aarch64")),
	feature(stdarch_neon_i8mm),
	feature(stdarch_neon_sm4),
	feature(stdarch_neon_ftts),
	feature(stdarch_neon_fcma),
	feature(stdarch_neon_dotprod)
)]
#![cfg_attr(not(feature = "std"), no_std)]
#![cfg_attr(docsrs, feature(doc_cfg))]

macro_rules! match_cfg {
    (item, match cfg!() {
        $(
            const { $i_meta:meta } => { $( $i_tokens:tt )* },
        )*
        $(_ => { $( $e_tokens:tt )* },)?
    }) => {
        $crate::match_cfg! {
            @__items () ;
            $(
                (( $i_meta ) ( $( $i_tokens )* )) ,
            )*
            $((() ( $( $e_tokens )* )),)?
        }
    };

    (match cfg!() {
        $(
            const { $i_meta:meta } => $i_expr: expr,
        )*
        $(_ => $e_expr: expr,)?
    }) => {
        $crate::match_cfg! {
            @ __result @ __exprs ();
            $(
                (( $i_meta ) ( $i_expr  )) ,
            )*
            $((() ( $e_expr  )),)?
        }
    };

    // Internal and recursive macro to emit all the items
    //
    // Collects all the previous cfgs in a list at the beginning, so they can be
    // negated. After the semicolon are all the remaining items.
    (@__items ( $( $_:meta , )* ) ; ) => {};
    (
        @__items ( $( $no:meta , )* ) ;
        (( $( $yes:meta )? ) ( $( $tokens:tt )* )) ,
        $( $rest:tt , )*
    ) => {
        // Emit all items within one block, applying an appropriate [cfg]. The
        // [cfg] will require all `$yes` matchers specified and must also negate
        // all previous matchers.
        #[cfg(all(
            $( $yes , )?
            not(any( $( $no ),* ))
        ))]
        $crate::match_cfg! { @__identity $( $tokens )* }

        // Recurse to emit all other items in `$rest`, and when we do so add all
        // our `$yes` matchers to the list of `$no` matchers as future emissions
        // will have to negate everything we just matched as well.
        $crate::match_cfg! {
            @__items ( $( $no , )* $( $yes , )? ) ;
            $( $rest , )*
        }
    };

    // Internal and recursive macro to emit all the exprs
    //
    // Collects all the previous cfgs in a list at the beginning, so they can be
    // negated. After the semicolon are all the remaining exprs.
    (@ $ret: ident @ __exprs ( $( $_:meta , )* ) ; ) => {
    	$ret
    };

    (
        @ $ret: ident @__exprs ( $( $no:meta , )* ) ;
        (( $( $yes:meta )? ) ( $( $tokens:tt )* )) ,
        $( $rest:tt , )*
    ) => {{
        // Emit all exprs within one block, applying an appropriate [cfg]. The
        // [cfg] will require all `$yes` matchers specified and must also negate
        // all previous matchers.
        #[cfg(all(
            $( $yes , )?
            not(any( $( $no ),* ))
        ))]
        let $ret = $crate::match_cfg! { @__identity $( $tokens )* };

        // // Recurse to emit all other exprs in `$rest`, and when we do so add all
        // // our `$yes` matchers to the list of `$no` matchers as future emissions
        // // will have to negate everything we just matched as well.
        $crate::match_cfg! {
            @ $ret @ __exprs ( $( $no , )* $( $yes , )? ) ;
            $( $rest , )*
        }
    }};

    // Internal macro to make __apply work out right for different match types,
    // because of how macros match/expand stuff.
    (@__identity $( $tokens:tt )* ) => {
        $( $tokens )*
    };
}

const MAX_REGISTER_BYTES: usize = 256;

use match_cfg;

/// Safe transmute macro.
///
/// This function asserts at compile time that the two types have the same size.
#[macro_export]
macro_rules! cast {
	($val: expr $(,)?) => {{
		let __val = $val;
		if const { false } {
			// checks type constraints
			$crate::cast(__val)
		} else {
			#[allow(
				unused_unsafe,
				unnecessary_transmutes,
				clippy::missing_transmute_annotations
			)]
			unsafe {
				::core::mem::transmute(__val)
			}
		}
	}};
}

use bytemuck::{AnyBitPattern, CheckedBitPattern, NoUninit, Pod, Zeroable, checked};
use core::fmt::Debug;
use core::marker::PhantomData;
use core::mem::MaybeUninit;
use core::ops::*;
use core::slice::{from_raw_parts, from_raw_parts_mut};
use num_complex::Complex;
use paste::paste;
use seal::Seal;

/// Requires the first non-lifetime generic parameter, as well as the function's
/// first input parameter to be the SIMD type.
/// Also currently requires that all the lifetimes be explicitly specified.
#[cfg(feature = "macro")]
#[cfg_attr(docsrs, doc(cfg(feature = "macro")))]
pub use pulp_macro::with_simd;

pub use {bytemuck, num_complex};

pub type c32 = Complex<f32>;
pub type c64 = Complex<f64>;

#[derive(Copy, Clone)]
#[repr(transparent)]
struct DebugCplx<T>(T);

unsafe impl<T: Zeroable> Zeroable for DebugCplx<T> {}
unsafe impl<T: Pod> Pod for DebugCplx<T> {}

impl Debug for DebugCplx<c32> {
	fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
		let c32 { re, im } = self.0;
		re.fmt(f)?;

		let sign = if im.is_sign_positive() { " + " } else { " - " };
		f.write_str(sign)?;

		let im = f32::from_bits(im.to_bits() & (u32::MAX >> 1));
		im.abs().fmt(f)?;

		f.write_str("i")?;

		Ok(())
	}
}

impl Debug for DebugCplx<c64> {
	fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
		let c64 { re, im } = self.0;
		re.fmt(f)?;

		let sign = if im.is_sign_positive() { " + " } else { " - " };
		f.write_str(sign)?;

		let im = f64::from_bits(im.to_bits() & (u64::MAX >> 1));
		im.abs().fmt(f)?;

		f.write_str("i")?;

		Ok(())
	}
}

match_cfg!(
	item,
	match cfg!() {
		const { any(target_arch = "x86_64") } => {
			#[derive(Debug, Copy, Clone)]
			pub struct MemMask<T> {
				mask: T,
				load: Option<unsafe extern "C" fn()>,
				store: Option<unsafe extern "C" fn()>,
			}

			impl<T> MemMask<T> {
				#[inline]
				pub fn new(mask: T) -> Self {
					Self {
						mask,
						load: None,
						store: None,
					}
				}
			}

			impl<T> From<T> for MemMask<T> {
				#[inline]
				fn from(value: T) -> Self {
					Self {
						mask: value,
						load: None,
						store: None,
					}
				}
			}
		},

		_ => {
			#[derive(Debug, Copy, Clone)]
			pub struct MemMask<T> {
				mask: T,
			}

			impl<T> MemMask<T> {
				#[inline]
				pub fn new(mask: T) -> Self {
					Self { mask }
				}
			}

			impl<T> From<T> for MemMask<T> {
				#[inline]
				fn from(value: T) -> Self {
					Self { mask: value }
				}
			}
		},
	}
);

impl<T: Copy> MemMask<T> {
	#[inline]
	pub fn mask(self) -> T {
		self.mask
	}
}

mod seal {
	pub trait Seal {}
}

pub trait NullaryFnOnce {
	type Output;

	fn call(self) -> Self::Output;
}

impl<R, F: FnOnce() -> R> NullaryFnOnce for F {
	type Output = R;

	#[inline(always)]
	fn call(self) -> Self::Output {
		self()
	}
}

pub trait WithSimd {
	type Output;

	fn with_simd<S: Simd>(self, simd: S) -> Self::Output;
}

impl<F: NullaryFnOnce> WithSimd for F {
	type Output = F::Output;

	#[inline(always)]
	fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
		let _simd = &simd;
		self.call()
	}
}

#[inline(always)]
fn fma_f32(a: f32, b: f32, c: f32) -> f32 {
	match_cfg!(match cfg!() {
		const { feature = "std" } => f32::mul_add(a, b, c),
		_ => libm::fmaf(a, b, c),
	})
}

#[inline(always)]
fn fma_f64(a: f64, b: f64, c: f64) -> f64 {
	match_cfg!(match cfg!() {
		const { feature = "std" } => f64::mul_add(a, b, c),
		_ => libm::fma(a, b, c),
	})
}

#[inline(always)]
fn sqrt_f32(a: f32) -> f32 {
	match_cfg!(match cfg!() {
		const { feature = "std" } => f32::sqrt(a),
		_ => libm::sqrtf(a),
	})
}

#[inline(always)]
fn sqrt_f64(a: f64) -> f64 {
	match_cfg!(match cfg!() {
		const { feature = "std" } => f64::sqrt(a, ),
		_ => libm::sqrt(a),
	})
}

// a0,0 ... a0,m-1
// ...
// an-1,0 ... an-1,m-1
#[inline(always)]
unsafe fn interleave_fallback<Unit: Pod, Reg: Pod, AosReg>(x: AosReg) -> AosReg {
	assert!(core::mem::size_of::<AosReg>() % core::mem::size_of::<Reg>() == 0);
	assert!(core::mem::size_of::<Reg>() % core::mem::size_of::<Unit>() == 0);
	assert!(!core::mem::needs_drop::<AosReg>());

	if const { core::mem::size_of::<AosReg>() == core::mem::size_of::<Reg>() } {
		x
	} else {
		let mut y = core::ptr::read(&x);

		let n = const { core::mem::size_of::<AosReg>() / core::mem::size_of::<Reg>() };
		let m = const { core::mem::size_of::<Reg>() / core::mem::size_of::<Unit>() };

		unsafe {
			let y = (&mut y) as *mut _ as *mut Unit;
			let x = (&x) as *const _ as *const Unit;
			for j in 0..m {
				for i in 0..n {
					*y.add(i + n * j) = *x.add(j + i * m);
				}
			}
		}

		y
	}
}

#[inline(always)]
unsafe fn deinterleave_fallback<Unit: Pod, Reg: Pod, SoaReg>(y: SoaReg) -> SoaReg {
	assert!(core::mem::size_of::<SoaReg>() % core::mem::size_of::<Reg>() == 0);
	assert!(core::mem::size_of::<Reg>() % core::mem::size_of::<Unit>() == 0);
	assert!(!core::mem::needs_drop::<SoaReg>());

	if const { core::mem::size_of::<SoaReg>() == core::mem::size_of::<Reg>() } {
		y
	} else {
		let mut x = core::ptr::read(&y);

		let n = const { core::mem::size_of::<SoaReg>() / core::mem::size_of::<Reg>() };
		let m = const { core::mem::size_of::<Reg>() / core::mem::size_of::<Unit>() };

		unsafe {
			let y = (&y) as *const _ as *const Unit;
			let x = (&mut x) as *mut _ as *mut Unit;
			for j in 0..m {
				for i in 0..n {
					*x.add(j + i * m) = *y.add(i + n * j);
				}
			}
		}

		x
	}
}

macro_rules! define_binop {
	($func: ident, $ty: ident, $out: ident) => {
		paste! {
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$out s>];
		}
	};
}

macro_rules! define_binop_all {
	($func: ident, $($ty: ident),*) => {
		$(define_binop!($func, $ty, $ty);)*
	};
	($func: ident, $($ty: ident => $out: ident),*) => {
		$(define_binop!($func, $ty, $out);)*
	};
}

macro_rules! transmute_binop {
	($func: ident, $ty: ident, $to: ident) => {
		paste! {
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$ty s>] {
				self.[<transmute_ $ty s_ $to s>](
					self.[<$func _ $to s>](self.[<transmute_ $to s_ $ty s>](a), self.[<transmute_ $to s_ $ty s>](b)),
				)
			}
		}
	};
	($func: ident, $($ty: ident => $to: ident),*) => {
		$(transmute_binop!($func, $ty, $to);)*
	};
}

macro_rules! define_unop {
	($func: ident, $ty: ident, $out: ident) => {
		paste! {
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>]) -> Self::[<$out s>];
		}
	};
}

macro_rules! define_unop_all {
	($func: ident, $($ty: ident),*) => {
		$(define_unop!($func, $ty, $ty);)*
	};
	($func: ident, $($ty: ident => $out: ident),*) => {
		$(define_unop!($func, $ty, $out);)*
	};
}

macro_rules! transmute_unop {
	($func: ident, $ty: ident, $to: ident) => {
		paste! {
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>]) -> Self::[<$ty s>] {
				self.[<transmute_ $ty s_ $to s>](
					self.[<$func _ $to s>](self.[<transmute_ $to s_ $ty s>](a)),
				)
			}
		}
	};
	($func: ident, $($ty: ident => $to: ident),*) => {
		$(transmute_unop!($func, $ty, $to);)*
	};
}

macro_rules! transmute_cmp {
	($func: ident, $ty: ident, $to: ident, $out: ident) => {
		paste! {
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$out s>] {
				self.[<$func _ $to s>](self.[<transmute_ $to s_ $ty s>](a), self.[<transmute_ $to s_ $ty s>](b))
			}
		}
	};
	($func: ident, $($ty: ident => $to: ident => $out: ident),*) => {
		$(transmute_cmp!($func, $ty, $to, $out);)*
	};
}

macro_rules! define_splat {
	($ty: ty) => {
		paste! {
			fn [<splat_ $ty s>](self, value: $ty) -> Self::[<$ty s>];
		}
	};
	($($ty: ident),*) => {
		$(define_splat!($ty);)*
	};
}

macro_rules! split_slice {
	($ty: ident) => {
		paste! {
			#[inline(always)]
			fn [<as_mut_rsimd_ $ty s>](slice: &mut [$ty]) -> (&mut [$ty], &mut [Self::[<$ty s>]]) {
				unsafe { rsplit_mut_slice(slice) }
			}
			#[inline(always)]
			fn [<as_rsimd_ $ty s>](slice: &[$ty]) -> (&[$ty], &[Self::[<$ty s>]]) {
				unsafe { rsplit_slice(slice) }
			}
			#[inline(always)]
			fn [<as_mut_simd_ $ty s>](slice: &mut [$ty]) -> (&mut [Self::[<$ty s>]], &mut [$ty]) {
				unsafe { split_mut_slice(slice) }
			}
			#[inline(always)]
			fn [<as_simd_ $ty s>](slice: &[$ty]) -> (&[Self::[<$ty s>]], &[$ty]) {
				unsafe { split_slice(slice) }
			}
			#[inline(always)]
			fn [<as_uninit_mut_rsimd_ $ty s>](
				slice: &mut [MaybeUninit<$ty>],
			) -> (&mut [MaybeUninit<$ty>], &mut [MaybeUninit<Self::[<$ty s>]>]) {
				unsafe { rsplit_mut_slice(slice) }
			}
			#[inline(always)]
			fn [<as_uninit_mut_simd_ $ty s>](
				slice: &mut [MaybeUninit<$ty>],
			) -> (&mut [MaybeUninit<Self::[<$ty s>]>], &mut [MaybeUninit<$ty>]) {
				unsafe { split_mut_slice(slice) }
			}
		}
	};
	($($ty: ident),*) => {
		$(split_slice!($ty);)*
	};
}

/// Types that allow \[de\]interleaving.
///
/// # Safety
/// Instances of this type passed to simd \[de\]interleave functions must be `Pod`.
pub unsafe trait Interleave {}
unsafe impl<T: Pod> Interleave for T {}

pub trait Simd: Seal + Debug + Copy + Send + Sync + 'static {
	const IS_SCALAR: bool = false;

	const M64_LANES: usize = core::mem::size_of::<Self::m64s>() / core::mem::size_of::<m64>();
	const U64_LANES: usize = core::mem::size_of::<Self::u64s>() / core::mem::size_of::<u64>();
	const I64_LANES: usize = core::mem::size_of::<Self::i64s>() / core::mem::size_of::<i64>();
	const F64_LANES: usize = core::mem::size_of::<Self::f64s>() / core::mem::size_of::<f64>();
	const C64_LANES: usize = core::mem::size_of::<Self::c64s>() / core::mem::size_of::<c64>();

	const M32_LANES: usize = core::mem::size_of::<Self::m32s>() / core::mem::size_of::<m32>();
	const U32_LANES: usize = core::mem::size_of::<Self::u32s>() / core::mem::size_of::<u32>();
	const I32_LANES: usize = core::mem::size_of::<Self::i32s>() / core::mem::size_of::<i32>();
	const F32_LANES: usize = core::mem::size_of::<Self::f32s>() / core::mem::size_of::<f32>();
	const C32_LANES: usize = core::mem::size_of::<Self::c32s>() / core::mem::size_of::<c32>();

	const M16_LANES: usize = core::mem::size_of::<Self::m16s>() / core::mem::size_of::<m16>();
	const U16_LANES: usize = core::mem::size_of::<Self::u16s>() / core::mem::size_of::<u16>();
	const I16_LANES: usize = core::mem::size_of::<Self::i16s>() / core::mem::size_of::<i16>();

	const M8_LANES: usize = core::mem::size_of::<Self::m8s>() / core::mem::size_of::<m8>();
	const U8_LANES: usize = core::mem::size_of::<Self::u8s>() / core::mem::size_of::<u8>();
	const I8_LANES: usize = core::mem::size_of::<Self::i8s>() / core::mem::size_of::<i8>();

	const REGISTER_COUNT: usize;

	type m8s: Debug + Copy + Send + Sync + Zeroable + NoUninit + CheckedBitPattern + 'static;
	type i8s: Debug + Copy + Send + Sync + Pod + 'static;
	type u8s: Debug + Copy + Send + Sync + Pod + 'static;

	type m16s: Debug + Copy + Send + Sync + Zeroable + NoUninit + CheckedBitPattern + 'static;
	type i16s: Debug + Copy + Send + Sync + Pod + 'static;
	type u16s: Debug + Copy + Send + Sync + Pod + 'static;

	type m32s: Debug + Copy + Send + Sync + Zeroable + NoUninit + CheckedBitPattern + 'static;
	type f32s: Debug + Copy + Send + Sync + Pod + 'static;
	type c32s: Debug + Copy + Send + Sync + Pod + 'static;
	type i32s: Debug + Copy + Send + Sync + Pod + 'static;
	type u32s: Debug + Copy + Send + Sync + Pod + 'static;

	type m64s: Debug + Copy + Send + Sync + Zeroable + NoUninit + CheckedBitPattern + 'static;
	type f64s: Debug + Copy + Send + Sync + Pod + 'static;
	type c64s: Debug + Copy + Send + Sync + Pod + 'static;
	type i64s: Debug + Copy + Send + Sync + Pod + 'static;
	type u64s: Debug + Copy + Send + Sync + Pod + 'static;

	/// Contains the square of the norm in both the real and imaginary components.
	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s;

	/// Contains the square of the norm in both the real and imaginary components.
	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s;
	#[inline]
	fn abs_f32s(self, a: Self::f32s) -> Self::f32s {
		self.and_f32s(self.not_f32s(self.splat_f32s(-0.0)), a)
	}
	#[inline]
	fn abs_f64s(self, a: Self::f64s) -> Self::f64s {
		self.and_f64s(self.not_f64s(self.splat_f64s(-0.0)), a)
	}
	/// Contains the max norm in both the real and imaginary components.
	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s;
	/// Contains the max norm in both the real and imaginary components.
	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s;

	define_binop_all!(add, c32, c64, f32, f64, u8, u16, u32, u64);
	define_binop_all!(
		sub, c32, c64, f32, f64, u8, i8, u16, i16, u32, i32, u64, i64
	);
	define_binop_all!(mul, c32, c64, f32, f64, u16, i16, u32, i32, u64, i64);
	define_binop_all!(div, f32, f64);
	define_binop_all!(equal, u8 => m8, u16 => m16, u32 => m32, u64 => m64, c32 => m32, f32 => m32, c64 => m64, f64 => m64);
	define_binop_all!(greater_than, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);
	define_binop_all!(greater_than_or_equal, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);
	define_binop_all!(less_than_or_equal, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);
	define_binop_all!(less_than, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);

	define_binop_all!(and, u8, u16, u32, u64);
	define_binop_all!(or, u8, u16, u32, u64);
	define_binop_all!(xor, u8, u16, u32, u64);

	transmute_binop!(and, m8 => u8, i8 => u8, m16 => u16, i16 => u16, m32 => u32, i32 => u32, m64 => u64, i64 => u64, f32 => u32, f64 => u64);
	transmute_binop!(or, m8 => u8, i8 => u8, m16 => u16, i16 => u16, m32 => u32, i32 => u32, m64 => u64, i64 => u64, f32 => u32, f64 => u64);
	transmute_binop!(xor, m8 => u8, i8 => u8, m16 => u16, i16 => u16, m32 => u32, i32 => u32, m64 => u64, i64 => u64, f32 => u32, f64 => u64);

	transmute_binop!(add, i8 => u8, i16 => u16, i32 => u32, i64 => u64);
	transmute_cmp!(equal, m8 => u8 => m8, i8 => u8 => m8, m16 => u16 => m16, i16 => u16 => m16, m32 => u32 => m32, i32 => u32 => m32, m64 => u64 => m64, i64 => u64 => m64);

	define_binop_all!(min, f32, f64, u8, i8, u16, i16, u32, i32, u64, i64);
	define_binop_all!(max, f32, f64, u8, i8, u16, i16, u32, i32, u64, i64);

	define_unop_all!(neg, c32, c64);
	define_unop_all!(not, m8, u8, m16, u16, m32, u32, m64, u64);

	transmute_unop!(not, i8 => u8, i16 => u16, i32 => u32, i64 => u64, f32 => u32, f64 => u64);

	split_slice!(u8, i8, u16, i16, u32, i32, u64, i64, c32, f32, c64, f64);
	define_splat!(u8, i8, u16, i16, u32, i32, u64, i64, c32, f32, c64, f64);

	fn sqrt_f32s(self, a: Self::f32s) -> Self::f32s;
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s;

	fn conj_c32s(self, a: Self::c32s) -> Self::c32s;
	fn conj_c64s(self, a: Self::c64s) -> Self::c64s;
	fn conj_mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s;
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s;

	/// Computes `conj(a) * b + c`
	#[inline]
	fn conj_mul_add_e_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		self.conj_mul_add_c32s(a, b, c)
	}
	/// Computes `conj(a) * b + c`
	#[inline]
	fn conj_mul_add_e_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		self.conj_mul_add_c64s(a, b, c)
	}
	fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;

	fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
	/// Computes `conj(a) * b`
	#[inline]
	fn conj_mul_e_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.conj_mul_c32s(a, b)
	}
	/// Computes `conj(a) * b`
	#[inline]
	fn conj_mul_e_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.conj_mul_c64s(a, b)
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
	fn first_true_m8s(self, mask: Self::m8s) -> usize {
		if const { core::mem::size_of::<Self::m8s>() == core::mem::size_of::<Self::u8s>() } {
			let mask: Self::u8s = bytemuck::cast(mask);
			let slice = bytemuck::cast_slice::<Self::u8s, u8>(core::slice::from_ref(&mask));
			let mut i = 0;
			for &x in slice.iter() {
				if x != 0 {
					break;
				}
				i += 1;
			}
			i
		} else if const { core::mem::size_of::<Self::m8s>() == core::mem::size_of::<u8>() } {
			let mask: u8 = bytemuck::cast(mask);
			mask.leading_zeros() as usize
		} else if const { core::mem::size_of::<Self::m8s>() == core::mem::size_of::<u16>() } {
			let mask: u16 = bytemuck::cast(mask);
			mask.leading_zeros() as usize
		} else {
			panic!()
		}
	}

	#[inline(always)]
	fn first_true_m16s(self, mask: Self::m16s) -> usize {
		if const { core::mem::size_of::<Self::m16s>() == core::mem::size_of::<Self::u16s>() } {
			let mask: Self::u16s = bytemuck::cast(mask);
			let slice = bytemuck::cast_slice::<Self::u16s, u16>(core::slice::from_ref(&mask));
			let mut i = 0;
			for &x in slice.iter() {
				if x != 0 {
					break;
				}
				i += 1;
			}
			i
		} else if const { core::mem::size_of::<Self::m16s>() == core::mem::size_of::<u8>() } {
			let mask: u8 = bytemuck::cast(mask);
			mask.leading_zeros() as usize
		} else if const { core::mem::size_of::<Self::m16s>() == core::mem::size_of::<u16>() } {
			let mask: u16 = bytemuck::cast(mask);
			mask.leading_zeros() as usize
		} else {
			panic!()
		}
	}

	#[inline(always)]
	fn first_true_m32s(self, mask: Self::m32s) -> usize {
		if const { core::mem::size_of::<Self::m32s>() == core::mem::size_of::<Self::u32s>() } {
			let mask: Self::u32s = bytemuck::cast(mask);
			let slice = bytemuck::cast_slice::<Self::u32s, u32>(core::slice::from_ref(&mask));
			let mut i = 0;
			for &x in slice.iter() {
				if x != 0 {
					break;
				}
				i += 1;
			}
			i
		} else if const { core::mem::size_of::<Self::m32s>() == core::mem::size_of::<u8>() } {
			let mask: u8 = bytemuck::cast(mask);
			mask.leading_zeros() as usize
		} else if const { core::mem::size_of::<Self::m32s>() == core::mem::size_of::<u16>() } {
			let mask: u16 = bytemuck::cast(mask);
			mask.leading_zeros() as usize
		} else {
			panic!()
		}
	}

	#[inline(always)]
	fn first_true_m64s(self, mask: Self::m64s) -> usize {
		if const { core::mem::size_of::<Self::m64s>() == core::mem::size_of::<Self::u64s>() } {
			let mask: Self::u64s = bytemuck::cast(mask);
			let slice = bytemuck::cast_slice::<Self::u64s, u64>(core::slice::from_ref(&mask));
			let mut i = 0;
			for &x in slice.iter() {
				if x != 0 {
					break;
				}
				i += 1;
			}
			i
		} else if const { core::mem::size_of::<Self::m64s>() == core::mem::size_of::<u8>() } {
			let mask: u8 = bytemuck::cast(mask);
			mask.leading_zeros() as usize
		} else if const { core::mem::size_of::<Self::m64s>() == core::mem::size_of::<u16>() } {
			let mask: u16 = bytemuck::cast(mask);
			mask.leading_zeros() as usize
		} else {
			panic!()
		}
	}

	#[inline(always)]
	fn interleave_shfl_f32s<T: Interleave>(self, values: T) -> T {
		unsafe { interleave_fallback::<f32, Self::f32s, T>(values) }
	}

	#[inline(always)]
	fn interleave_shfl_f64s<T: Interleave>(self, values: T) -> T {
		unsafe { interleave_fallback::<f64, Self::f64s, T>(values) }
	}

	#[inline(always)]
	fn mask_between_m8s(self, start: u8, end: u8) -> MemMask<Self::m8s> {
		let iota: Self::u8s = const {
			unsafe { core::mem::transmute_copy(&iota_8::<u8, { MAX_REGISTER_BYTES / 1 }>()) }
		};
		self.and_m8s(
			self.greater_than_or_equal_u8s(iota, self.splat_u8s(start)),
			self.less_than_u8s(iota, self.splat_u8s(end)),
		)
		.into()
	}

	#[inline(always)]
	fn mask_between_m16s(self, start: u16, end: u16) -> MemMask<Self::m16s> {
		let iota: Self::u16s = const {
			unsafe { core::mem::transmute_copy(&iota_16::<u16, { MAX_REGISTER_BYTES / 2 }>()) }
		};
		self.and_m16s(
			self.greater_than_or_equal_u16s(iota, self.splat_u16s(start)),
			self.less_than_u16s(iota, self.splat_u16s(end)),
		)
		.into()
	}

	#[inline(always)]
	fn mask_between_m32s(self, start: u32, end: u32) -> MemMask<Self::m32s> {
		let iota: Self::u32s = const {
			unsafe { core::mem::transmute_copy(&iota_32::<u32, { MAX_REGISTER_BYTES / 4 }>()) }
		};
		self.and_m32s(
			self.greater_than_or_equal_u32s(iota, self.splat_u32s(start)),
			self.less_than_u32s(iota, self.splat_u32s(end)),
		)
		.into()
	}

	#[inline(always)]
	fn mask_between_m64s(self, start: u64, end: u64) -> MemMask<Self::m64s> {
		let iota: Self::u64s = const {
			unsafe { core::mem::transmute_copy(&iota_64::<u64, { MAX_REGISTER_BYTES / 8 }>()) }
		};
		self.and_m64s(
			self.greater_than_or_equal_u64s(iota, self.splat_u64s(start)),
			self.less_than_u64s(iota, self.splat_u64s(end)),
		)
		.into()
	}
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	unsafe fn mask_load_ptr_c32s(self, mask: MemMask<Self::m32s>, ptr: *const c32) -> Self::c32s;
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	unsafe fn mask_load_ptr_c64s(self, mask: MemMask<Self::m64s>, ptr: *const c64) -> Self::c64s;
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	#[inline(always)]
	unsafe fn mask_load_ptr_f32s(self, mask: MemMask<Self::m32s>, ptr: *const f32) -> Self::f32s {
		self.transmute_f32s_u32s(self.mask_load_ptr_u32s(mask, ptr as *const u32))
	}

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	#[inline(always)]
	unsafe fn mask_load_ptr_f64s(self, mask: MemMask<Self::m64s>, ptr: *const f64) -> Self::f64s {
		self.transmute_f64s_u64s(self.mask_load_ptr_u64s(mask, ptr as *const u64))
	}
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	#[inline(always)]
	unsafe fn mask_load_ptr_i8s(self, mask: MemMask<Self::m8s>, ptr: *const i8) -> Self::i8s {
		self.transmute_i8s_u8s(self.mask_load_ptr_u8s(mask, ptr as *const u8))
	}
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	#[inline(always)]
	unsafe fn mask_load_ptr_i16s(self, mask: MemMask<Self::m16s>, ptr: *const i16) -> Self::i16s {
		self.transmute_i16s_u16s(self.mask_load_ptr_u16s(mask, ptr as *const u16))
	}
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	#[inline(always)]
	unsafe fn mask_load_ptr_i32s(self, mask: MemMask<Self::m32s>, ptr: *const i32) -> Self::i32s {
		self.transmute_i32s_u32s(self.mask_load_ptr_u32s(mask, ptr as *const u32))
	}
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	#[inline(always)]
	unsafe fn mask_load_ptr_i64s(self, mask: MemMask<Self::m64s>, ptr: *const i64) -> Self::i64s {
		self.transmute_i64s_u64s(self.mask_load_ptr_u64s(mask, ptr as *const u64))
	}

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	unsafe fn mask_load_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *const u8) -> Self::u8s;

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	unsafe fn mask_load_ptr_u16s(self, mask: MemMask<Self::m16s>, ptr: *const u16) -> Self::u16s;

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	unsafe fn mask_load_ptr_u32s(self, mask: MemMask<Self::m32s>, ptr: *const u32) -> Self::u32s;

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::read`].
	unsafe fn mask_load_ptr_u64s(self, mask: MemMask<Self::m64s>, ptr: *const u64) -> Self::u64s;
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	unsafe fn mask_store_ptr_c32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut c32,
		values: Self::c32s,
	);
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	unsafe fn mask_store_ptr_c64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut c64,
		values: Self::c64s,
	);
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	#[inline(always)]
	unsafe fn mask_store_ptr_f32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut f32,
		values: Self::f32s,
	) {
		self.mask_store_ptr_u32s(mask, ptr as *mut u32, self.transmute_u32s_f32s(values));
	}

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	#[inline(always)]
	unsafe fn mask_store_ptr_f64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut f64,
		values: Self::f64s,
	) {
		self.mask_store_ptr_u64s(mask, ptr as *mut u64, self.transmute_u64s_f64s(values));
	}
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	#[inline(always)]
	unsafe fn mask_store_ptr_i8s(self, mask: MemMask<Self::m8s>, ptr: *mut i8, values: Self::i8s) {
		self.mask_store_ptr_u8s(mask, ptr as *mut u8, self.transmute_u8s_i8s(values));
	}
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	#[inline(always)]
	unsafe fn mask_store_ptr_i16s(
		self,
		mask: MemMask<Self::m16s>,
		ptr: *mut i16,
		values: Self::i16s,
	) {
		self.mask_store_ptr_u16s(mask, ptr as *mut u16, self.transmute_u16s_i16s(values));
	}
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	#[inline(always)]
	unsafe fn mask_store_ptr_i32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut i32,
		values: Self::i32s,
	) {
		self.mask_store_ptr_u32s(mask, ptr as *mut u32, self.transmute_u32s_i32s(values));
	}
	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	#[inline(always)]
	unsafe fn mask_store_ptr_i64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut i64,
		values: Self::i64s,
	) {
		self.mask_store_ptr_u64s(mask, ptr as *mut u64, self.transmute_u64s_i64s(values));
	}

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	unsafe fn mask_store_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *mut u8, values: Self::u8s);

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	unsafe fn mask_store_ptr_u16s(
		self,
		mask: MemMask<Self::m16s>,
		ptr: *mut u16,
		values: Self::u16s,
	);

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	unsafe fn mask_store_ptr_u32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut u32,
		values: Self::u32s,
	);

	/// # Safety
	///
	/// Addresses corresponding to enabled lanes in the mask have the same restrictions as
	/// [`core::ptr::write`].
	unsafe fn mask_store_ptr_u64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut u64,
		values: Self::u64s,
	);

	fn mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s;
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s;
	/// Computes `a * b + c`
	#[inline]
	fn mul_add_e_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		self.mul_add_c32s(a, b, c)
	}
	/// Computes `a * b + c`
	#[inline]
	fn mul_add_e_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		self.mul_add_c64s(a, b, c)
	}
	fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s;
	fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s;
	fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s;
	fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s;
	/// Computes `a * b`
	#[inline]
	fn mul_e_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		self.mul_c32s(a, b)
	}
	/// Computes `a * b`
	fn mul_e_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		self.mul_c64s(a, b)
	}

	#[inline]
	fn neg_f32s(self, a: Self::f32s) -> Self::f32s {
		self.xor_f32s(self.splat_f32s(-0.0), a)
	}
	#[inline]
	fn neg_f64s(self, a: Self::f64s) -> Self::f64s {
		self.xor_f64s(a, self.splat_f64s(-0.0))
	}

	#[inline(always)]
	fn partial_load_c32s(self, slice: &[c32]) -> Self::c32s {
		cast(self.partial_load_f64s(bytemuck::cast_slice(slice)))
	}
	#[inline(always)]
	fn partial_load_c64s(self, slice: &[c64]) -> Self::c64s {
		cast(self.partial_load_f64s(bytemuck::cast_slice(slice)))
	}
	#[inline(always)]
	fn partial_load_f32s(self, slice: &[f32]) -> Self::f32s {
		cast(self.partial_load_u32s(bytemuck::cast_slice(slice)))
	}
	#[inline(always)]
	fn partial_load_f64s(self, slice: &[f64]) -> Self::f64s {
		cast(self.partial_load_u64s(bytemuck::cast_slice(slice)))
	}
	#[inline(always)]
	fn partial_load_i8s(self, slice: &[i8]) -> Self::i8s {
		cast(self.partial_load_u8s(bytemuck::cast_slice(slice)))
	}
	#[inline(always)]
	fn partial_load_i16s(self, slice: &[i16]) -> Self::i16s {
		cast(self.partial_load_u16s(bytemuck::cast_slice(slice)))
	}
	#[inline(always)]
	fn partial_load_i32s(self, slice: &[i32]) -> Self::i32s {
		cast(self.partial_load_u32s(bytemuck::cast_slice(slice)))
	}
	#[inline(always)]
	fn partial_load_i64s(self, slice: &[i64]) -> Self::i64s {
		cast(self.partial_load_u64s(bytemuck::cast_slice(slice)))
	}
	#[inline(always)]
	fn partial_load_u8s(self, slice: &[u8]) -> Self::u8s {
		unsafe {
			self.mask_load_ptr_u8s(self.mask_between_m8s(0, slice.len() as u8), slice.as_ptr())
		}
	}
	#[inline(always)]
	fn partial_load_u16s(self, slice: &[u16]) -> Self::u16s {
		unsafe {
			self.mask_load_ptr_u16s(
				self.mask_between_m16s(0, slice.len() as u16),
				slice.as_ptr(),
			)
		}
	}
	#[inline(always)]
	fn partial_load_u32s(self, slice: &[u32]) -> Self::u32s {
		unsafe {
			self.mask_load_ptr_u32s(
				self.mask_between_m32s(0, slice.len() as u32),
				slice.as_ptr(),
			)
		}
	}
	#[inline(always)]
	fn partial_load_u64s(self, slice: &[u64]) -> Self::u64s {
		unsafe {
			self.mask_load_ptr_u64s(
				self.mask_between_m64s(0, slice.len() as u64),
				slice.as_ptr(),
			)
		}
	}

	#[inline(always)]
	fn partial_store_c32s(self, slice: &mut [c32], values: Self::c32s) {
		self.partial_store_f64s(bytemuck::cast_slice_mut(slice), cast(values))
	}
	#[inline(always)]
	fn partial_store_c64s(self, slice: &mut [c64], values: Self::c64s) {
		self.partial_store_f64s(bytemuck::cast_slice_mut(slice), cast(values))
	}

	#[inline(always)]
	fn partial_store_f32s(self, slice: &mut [f32], values: Self::f32s) {
		self.partial_store_u32s(bytemuck::cast_slice_mut(slice), cast(values))
	}
	#[inline(always)]
	fn partial_store_f64s(self, slice: &mut [f64], values: Self::f64s) {
		self.partial_store_u64s(bytemuck::cast_slice_mut(slice), cast(values))
	}
	#[inline(always)]
	fn partial_store_i8s(self, slice: &mut [i8], values: Self::i8s) {
		self.partial_store_u16s(bytemuck::cast_slice_mut(slice), cast(values))
	}
	#[inline(always)]
	fn partial_store_i16s(self, slice: &mut [i16], values: Self::i16s) {
		self.partial_store_u16s(bytemuck::cast_slice_mut(slice), cast(values))
	}
	#[inline(always)]
	fn partial_store_i32s(self, slice: &mut [i32], values: Self::i32s) {
		self.partial_store_u32s(bytemuck::cast_slice_mut(slice), cast(values))
	}
	#[inline(always)]
	fn partial_store_i64s(self, slice: &mut [i64], values: Self::i64s) {
		self.partial_store_u64s(bytemuck::cast_slice_mut(slice), cast(values))
	}
	#[inline(always)]
	fn partial_store_u8s(self, slice: &mut [u8], values: Self::u8s) {
		unsafe {
			self.mask_store_ptr_u8s(
				self.mask_between_m8s(0, slice.len() as u8),
				slice.as_mut_ptr(),
				values,
			)
		}
	}
	#[inline(always)]
	fn partial_store_u16s(self, slice: &mut [u16], values: Self::u16s) {
		unsafe {
			self.mask_store_ptr_u16s(
				self.mask_between_m16s(0, slice.len() as u16),
				slice.as_mut_ptr(),
				values,
			)
		}
	}
	#[inline(always)]
	fn partial_store_u32s(self, slice: &mut [u32], values: Self::u32s) {
		unsafe {
			self.mask_store_ptr_u32s(
				self.mask_between_m32s(0, slice.len() as u32),
				slice.as_mut_ptr(),
				values,
			)
		}
	}
	#[inline(always)]
	fn partial_store_u64s(self, slice: &mut [u64], values: Self::u64s) {
		unsafe {
			self.mask_store_ptr_u64s(
				self.mask_between_m64s(0, slice.len() as u64),
				slice.as_mut_ptr(),
				values,
			)
		}
	}
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
	#[inline(always)]
	fn rotate_left_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s {
		self.rotate_right_c32s(a, amount.wrapping_neg())
	}
	#[inline(always)]
	fn rotate_left_c64s(self, a: Self::c64s, amount: usize) -> Self::c64s {
		self.rotate_right_c64s(a, amount.wrapping_neg())
	}

	#[inline(always)]
	fn rotate_left_f32s(self, a: Self::f32s, amount: usize) -> Self::f32s {
		cast(self.rotate_left_u32s(cast(a), amount))
	}
	#[inline(always)]
	fn rotate_left_f64s(self, a: Self::f64s, amount: usize) -> Self::f64s {
		cast(self.rotate_left_u64s(cast(a), amount))
	}
	#[inline(always)]
	fn rotate_left_i32s(self, a: Self::i32s, amount: usize) -> Self::i32s {
		cast(self.rotate_left_u32s(cast(a), amount))
	}

	#[inline(always)]
	fn rotate_left_i64s(self, a: Self::i64s, amount: usize) -> Self::i64s {
		cast(self.rotate_left_u64s(cast(a), amount))
	}

	#[inline(always)]
	fn rotate_left_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s {
		self.rotate_right_u32s(a, amount.wrapping_neg())
	}
	#[inline(always)]
	fn rotate_left_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s {
		self.rotate_right_u64s(a, amount.wrapping_neg())
	}
	fn rotate_right_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s;
	fn rotate_right_c64s(self, a: Self::c64s, amount: usize) -> Self::c64s;
	#[inline(always)]
	fn rotate_right_f32s(self, a: Self::f32s, amount: usize) -> Self::f32s {
		cast(self.rotate_right_u32s(cast(a), amount))
	}
	#[inline(always)]
	fn rotate_right_f64s(self, a: Self::f64s, amount: usize) -> Self::f64s {
		cast(self.rotate_right_u64s(cast(a), amount))
	}
	#[inline(always)]
	fn rotate_right_i32s(self, a: Self::i32s, amount: usize) -> Self::i32s {
		cast(self.rotate_right_u32s(cast(a), amount))
	}
	#[inline(always)]
	fn rotate_right_i64s(self, a: Self::i64s, amount: usize) -> Self::i64s {
		cast(self.rotate_right_u64s(cast(a), amount))
	}
	fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s;
	fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s;

	#[inline]
	fn select_f32s(
		self,
		mask: Self::m32s,
		if_true: Self::f32s,
		if_false: Self::f32s,
	) -> Self::f32s {
		self.transmute_f32s_u32s(self.select_u32s(
			mask,
			self.transmute_u32s_f32s(if_true),
			self.transmute_u32s_f32s(if_false),
		))
	}
	#[inline]
	fn select_f64s(
		self,
		mask: Self::m64s,
		if_true: Self::f64s,
		if_false: Self::f64s,
	) -> Self::f64s {
		self.transmute_f64s_u64s(self.select_u64s(
			mask,
			self.transmute_u64s_f64s(if_true),
			self.transmute_u64s_f64s(if_false),
		))
	}
	#[inline]
	fn select_i32s(
		self,
		mask: Self::m32s,
		if_true: Self::i32s,
		if_false: Self::i32s,
	) -> Self::i32s {
		self.transmute_i32s_u32s(self.select_u32s(
			mask,
			self.transmute_u32s_i32s(if_true),
			self.transmute_u32s_i32s(if_false),
		))
	}
	#[inline]
	fn select_i64s(
		self,
		mask: Self::m64s,
		if_true: Self::i64s,
		if_false: Self::i64s,
	) -> Self::i64s {
		self.transmute_i64s_u64s(self.select_u64s(
			mask,
			self.transmute_u64s_i64s(if_true),
			self.transmute_u64s_i64s(if_false),
		))
	}
	fn select_u32s(self, mask: Self::m32s, if_true: Self::u32s, if_false: Self::u32s)
	-> Self::u32s;
	fn select_u64s(self, mask: Self::m64s, if_true: Self::u64s, if_false: Self::u64s)
	-> Self::u64s;

	fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s;
	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s;

	#[inline]
	fn transmute_f32s_i32s(self, a: Self::i32s) -> Self::f32s {
		cast(a)
	}
	#[inline]
	fn transmute_f32s_u32s(self, a: Self::u32s) -> Self::f32s {
		cast(a)
	}

	#[inline]
	fn transmute_f64s_i64s(self, a: Self::i64s) -> Self::f64s {
		cast(a)
	}
	#[inline]
	fn transmute_f64s_u64s(self, a: Self::u64s) -> Self::f64s {
		cast(a)
	}
	#[inline]
	fn transmute_i32s_f32s(self, a: Self::f32s) -> Self::i32s {
		cast(a)
	}
	#[inline]
	fn transmute_m8s_u8s(self, a: Self::u8s) -> Self::m8s {
		checked::cast(a)
	}
	#[inline]
	fn transmute_u8s_m8s(self, a: Self::m8s) -> Self::u8s {
		cast(a)
	}
	#[inline]
	fn transmute_m16s_u16s(self, a: Self::u16s) -> Self::m16s {
		checked::cast(a)
	}
	#[inline]
	fn transmute_u16s_m16s(self, a: Self::m16s) -> Self::u16s {
		cast(a)
	}
	#[inline]
	fn transmute_m32s_u32s(self, a: Self::u32s) -> Self::m32s {
		checked::cast(a)
	}
	#[inline]
	fn transmute_u32s_m32s(self, a: Self::m32s) -> Self::u32s {
		cast(a)
	}
	#[inline]
	fn transmute_m64s_u64s(self, a: Self::u64s) -> Self::m64s {
		checked::cast(a)
	}
	#[inline]
	fn transmute_u64s_m64s(self, a: Self::m64s) -> Self::u64s {
		cast(a)
	}
	#[inline]
	fn transmute_i8s_u8s(self, a: Self::u8s) -> Self::i8s {
		cast(a)
	}
	#[inline]
	fn transmute_u8s_i8s(self, a: Self::i8s) -> Self::u8s {
		cast(a)
	}
	#[inline]
	fn transmute_u16s_i16s(self, a: Self::i16s) -> Self::u16s {
		cast(a)
	}
	#[inline]
	fn transmute_i16s_u16s(self, a: Self::u16s) -> Self::i16s {
		cast(a)
	}
	#[inline]
	fn transmute_i32s_u32s(self, a: Self::u32s) -> Self::i32s {
		cast(a)
	}
	#[inline]
	fn transmute_i64s_f64s(self, a: Self::f64s) -> Self::i64s {
		cast(a)
	}
	#[inline]
	fn transmute_i64s_u64s(self, a: Self::u64s) -> Self::i64s {
		cast(a)
	}

	#[inline]
	fn transmute_u32s_f32s(self, a: Self::f32s) -> Self::u32s {
		cast(a)
	}
	#[inline]
	fn transmute_u32s_i32s(self, a: Self::i32s) -> Self::u32s {
		cast(a)
	}
	#[inline]
	fn transmute_u64s_f64s(self, a: Self::f64s) -> Self::u64s {
		cast(a)
	}
	#[inline]
	fn transmute_u64s_i64s(self, a: Self::i64s) -> Self::u64s {
		cast(a)
	}

	fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output;
	fn widening_mul_u32s(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s);
	fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s;
	fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s;
}

pub trait PortableSimd: Simd {}

impl PortableSimd for Scalar {}
impl PortableSimd for Scalar128b {}
impl PortableSimd for Scalar256b {}
impl PortableSimd for Scalar512b {}

#[derive(Debug, Copy, Clone)]
pub struct Scalar;

#[derive(Debug, Copy, Clone)]
pub struct Scalar128b;
#[derive(Debug, Copy, Clone)]
pub struct Scalar256b;
#[derive(Debug, Copy, Clone)]
pub struct Scalar512b;

macro_rules! scalar_simd_binop_impl {
	($func: ident, $op: ident, $ty: ty) => {
		paste! {
			#[inline]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>],) -> Self::[<$ty s>] {
				let mut out = [<$ty as Default>::default(); Self::[<$ty:upper _LANES>]];
				let a: [$ty; Self::[<$ty:upper _LANES>]] = cast(a);
				let b: [$ty; Self::[<$ty:upper _LANES>]] = cast(b);

				for i in 0..Self::[<$ty:upper _LANES>] {
					out[i] = a[i].$op(b[i]);
				}

				cast(out)
			}
		}
	};
}

macro_rules! scalar_simd_binop {
	($func: ident, op $op: ident, $($ty: ty),*) => {
		$(scalar_simd_binop_impl!($func, $op, $ty);)*
	};
	($func: ident, $($ty: ty),*) => {
		$(scalar_simd_binop_impl!($func, $func, $ty);)*
	};
}

macro_rules! scalar_simd_unop_impl {
	($func: ident, $op: ident, $ty: ty) => {
		paste! {
			#[inline]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>]) -> Self::[<$ty s>] {
				let mut out = [<$ty as Default>::default(); Self::[<$ty:upper _LANES>]];
				let a: [$ty; Self::[<$ty:upper _LANES>]] = cast(a);

				for i in 0..Self::[<$ty:upper _LANES>] {
					out[i] = a[i].$op();
				}

				cast(out)
			}
		}
	};
}

macro_rules! scalar_simd_unop {
	($func: ident, $($ty: ty),*) => {
		$(scalar_simd_unop_impl!($func, $func, $ty);)*
	};
}

macro_rules! scalar_simd_cmp {
	($func: ident, $op: ident, $ty: ty, $mask: ty) => {
		paste! {
			#[inline]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$mask s>] {
				let mut out = [$mask::new(false); Self::[<$ty:upper _LANES>]];
				let a: [$ty; Self::[<$ty:upper _LANES>]] = cast(a);
				let b: [$ty; Self::[<$ty:upper _LANES>]] = cast(b);
				for i in 0..Self::[<$ty:upper _LANES>] {
					out[i] = $mask::new(a[i].$op(&b[i]));
				}
				cast(out)
			}
		}
	};
	($func: ident, op $op: ident, $($ty: ty => $mask: ty),*) => {
		$(scalar_simd_cmp!($func, $op, $ty, $mask);)*
	};
	($func: ident, $($ty: ty => $mask: ty),*) => {
		$(scalar_simd_cmp!($func, $func, $ty, $mask);)*
	};
}

macro_rules! scalar_splat {
	($ty: ident) => {
		paste! {
			#[inline]
			fn [<splat_ $ty s>](self, value: $ty) -> Self::[<$ty s>] {
				cast([value; Self::[<$ty:upper _LANES>]])
			}
		}
	};
	($($ty: ident),*) => {
		$(scalar_splat!($ty);)*
	};
}

macro_rules! scalar_partial_load {
	($ty: ident) => {
		paste! {
			#[inline]
			fn [<partial_load_ $ty s>](self, slice: &[$ty]) -> Self::[<$ty s>] {
				let mut values = [<$ty as Default>::default(); Self::[<$ty:upper _LANES>]];
				for i in 0..Ord::min(values.len(), slice.len()) {
					values[i] = slice[i];
				}
				cast(values)
			}
		}
	};
	($($ty: ident),*) => {
		$(scalar_partial_load!($ty);)*
	};
}

macro_rules! scalar_partial_store {
	($ty: ident) => {
		paste! {
			#[inline]
			fn [<partial_store_ $ty s>](self, slice: &mut [$ty], values: Self::[<$ty s>]) {
				let values: [$ty; Self::[<$ty:upper _LANES>]] = cast(values);
				for i in 0..Ord::min(values.len(), slice.len()) {
					slice[i] = values[i];
				}
			}
		}
	};
	($($ty: ident),*) => {
		$(scalar_partial_store!($ty);)*
	};
}

macro_rules! mask_load_ptr {
	($ty: ident, $mask: ident) => {
		paste! {
			#[inline]
			unsafe fn [<mask_load_ptr_ $ty s>](
				self,
				mask: MemMask<Self::[<$mask s>]>,
				ptr: *const $ty,
			) -> Self::[<$ty s>] {
				let mut values = [<$ty as Default>::default(); Self::[<$ty:upper _LANES>]];
				let mask: [$mask; Self::[<$ty:upper _LANES>]] = cast(mask.mask());
				for i in 0..Self::[<$ty:upper _LANES>] {
					if mask[i].is_set() {
						values[i] = *ptr.add(i);
					}
				}
				cast(values)
			}
		}
	};
	(cast $ty: ident, $to: ident, $mask: ident) => {
		paste! {
			#[inline]
			unsafe fn [<mask_load_ptr_ $ty s>](
				self,
				mask: MemMask<Self::[<$mask s>]>,
				ptr: *const $ty,
			) -> Self::[<$ty s>] {
				cast(self.[<mask_load_ptr_ $to s>](mask, ptr as *const $to))
			}
		}
	};
	($($ty: ident: $mask: ident),*) => {
		$(mask_load_ptr!($ty, $mask);)*
	};
	(cast $($ty: ident: $mask: ident => $to: ident),*) => {
		$(mask_load_ptr!(cast $ty, $to, $mask);)*
	};
}

macro_rules! mask_store_ptr {
	($ty: ident, $mask: ident) => {
		paste! {
			#[inline]
			unsafe fn [<mask_store_ptr_ $ty s>](
				self,
				mask: MemMask<Self::[<$mask s>]>,
				ptr: *mut $ty,
				values: Self::[<$ty s>],
			) {
				let mask: [$mask; Self::[<$ty:upper _LANES>]] = cast(mask.mask());
				let values: [$ty; Self::[<$ty:upper _LANES>]] = cast(values);
				for i in 0..Self::[<$ty:upper _LANES>] {
					if mask[i].is_set() {
						*ptr.add(i) = values[i];
					}
				}
			}
		}
	};
	(cast $ty: ident, $to: ident, $mask: ident) => {
		paste! {
			#[inline]
			unsafe fn [<mask_store_ptr_ $ty s>](
				self,
				mask: MemMask<Self::[<$mask s>]>,
				ptr: *mut $ty,
				values: Self::[<$ty s>],
			) {
				self.[<mask_store_ptr_ $to s>](mask, ptr as *mut $to, cast(values));
			}
		}
	};
	($($ty: ident: $mask: ident),*) => {
		$(mask_store_ptr!($ty, $mask);)*
	};
	(cast $($ty: ident: $mask: ident => $to: ident),*) => {
		$(mask_store_ptr!(cast $ty, $to, $mask);)*
	};
}

macro_rules! scalar_simd {
	($ty: ty, $register_count: expr, $m8s: ty, $i8s: ty, $u8s: ty, $m16s: ty, $i16s: ty, $u16s: ty, $m32s: ty, $f32s: ty, $i32s: ty, $u32s: ty, $m64s: ty, $f64s: ty, $i64s: ty, $u64s: ty $(,)?) => {
		impl Seal for $ty {}
		impl Simd for $ty {
			type m8s = $m8s;
			type m16s = $m16s;
			type c32s = $f32s;
			type c64s = $f64s;
			type f32s = $f32s;
			type f64s = $f64s;
			type i16s = $i16s;
			type i32s = $i32s;
			type i64s = $i64s;
			type i8s = $i8s;
			type m32s = $m32s;
			type m64s = $m64s;
			type u16s = $u16s;
			type u32s = $u32s;
			type u64s = $u64s;
			type u8s = $u8s;

			const REGISTER_COUNT: usize = $register_count;

			scalar_simd_binop!(min, u8, i8, u16, i16, u32, i32, u64, i64, f32, f64);

			scalar_simd_binop!(max, u8, i8, u16, i16, u32, i32, u64, i64, f32, f64);

			scalar_simd_binop!(add, c32, f32, c64, f64);
			scalar_simd_binop!(add, op wrapping_add, u8, i8, u16, i16, u32, i32, u64, i64);
			scalar_simd_binop!(sub, c32, f32, c64, f64);
			scalar_simd_binop!(sub, op wrapping_sub, u8, i8, u16, i16, u32, i32, u64, i64);
			scalar_simd_binop!(mul, c32, f32, c64, f64);
			scalar_simd_binop!(mul, op wrapping_mul, u16, i16, u32, i32, u64, i64);
			scalar_simd_binop!(div, f32, f64);

			scalar_simd_binop!(and, op bitand, u8, u16, u32, u64);
			scalar_simd_binop!(or,  op bitor, u8, u16, u32, u64);
			scalar_simd_binop!(xor, op bitxor, u8, u16, u32, u64);

			scalar_simd_cmp!(equal, op eq, u8 => m8, u16 => m16, u32 => m32, u64 => m64, c32 => m32, f32 => m32, c64 => m64, f64 => m64);
			scalar_simd_cmp!(greater_than, op gt, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);
			scalar_simd_cmp!(greater_than_or_equal, op ge, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);
			scalar_simd_cmp!(less_than_or_equal, op le, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);
			scalar_simd_cmp!(less_than, op lt, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);

			scalar_simd_unop!(not, m8, u8, m16, u16, m32, u32, m64, u64);

			scalar_splat!(u8, i8, u16, i16, u32, i32, u64, i64, f32, f64);

			scalar_partial_load!(u8, i8, u16, i16, u32, i32, u64, i64, f32, f64);
			scalar_partial_store!(u8, i8, u16, i16, u32, i32, u64, i64, f32, f64);

			mask_load_ptr!(u8: m8, u16: m16, u32: m32, u64: m64);
			mask_load_ptr!(cast i8: m8 => u8, i16: m16 => u16, i32: m32 => u32, i64: m64 => u64, c32: m32 => u32, f32: m32 => u32, c64: m64 => u64, f64: m64 => u64);
			mask_store_ptr!(u8: m8, u16: m16, u32: m32, u64: m64);
			mask_store_ptr!(cast i8: m8 => u8, i16: m16 => u16, i32: m32 => u32, i64: m64 => u64, c32: m32 => u32, f32: m32 => u32, c64: m64 => u64, f64: m64 => u64);

			#[inline]
			fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
				op.with_simd(self)
			}

			#[inline]
			fn and_m32s(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
				let mut out = [m32::new(false); Self::F32_LANES];
				let a: [m32; Self::F32_LANES] = cast(a);
				let b: [m32; Self::F32_LANES] = cast(b);
				for i in 0..Self::F32_LANES {
					out[i] = a[i] & b[i];
				}
				cast(out)
			}

			#[inline]
			fn or_m32s(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
				let mut out = [m32::new(false); Self::F32_LANES];
				let a: [m32; Self::F32_LANES] = cast(a);
				let b: [m32; Self::F32_LANES] = cast(b);
				for i in 0..Self::F32_LANES {
					out[i] = a[i] | b[i];
				}
				cast(out)
			}

			#[inline]
			fn xor_m32s(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
				let mut out = [m32::new(false); Self::F32_LANES];
				let a: [m32; Self::F32_LANES] = cast(a);
				let b: [m32; Self::F32_LANES] = cast(b);
				for i in 0..Self::F32_LANES {
					out[i] = a[i] ^ b[i];
				}
				cast(out)
			}

			#[inline]
			fn and_m64s(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
				let mut out = [m64::new(false); Self::F64_LANES];
				let a: [m64; Self::F64_LANES] = cast(a);
				let b: [m64; Self::F64_LANES] = cast(b);
				for i in 0..Self::F64_LANES {
					out[i] = a[i] & b[i];
				}
				cast(out)
			}

			#[inline]
			fn or_m64s(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
				let mut out = [m64::new(false); Self::F64_LANES];
				let a: [m64; Self::F64_LANES] = cast(a);
				let b: [m64; Self::F64_LANES] = cast(b);
				for i in 0..Self::F64_LANES {
					out[i] = a[i] | b[i];
				}
				cast(out)
			}

			#[inline]
			fn xor_m64s(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
				let mut out = [m64::new(false); Self::F64_LANES];
				let a: [m64; Self::F64_LANES] = cast(a);
				let b: [m64; Self::F64_LANES] = cast(b);
				for i in 0..Self::F64_LANES {
					out[i] = a[i] ^ b[i];
				}
				cast(out)
			}

			#[inline]
			fn select_u32s(
				self,
				mask: Self::m32s,
				if_true: Self::u32s,
				if_false: Self::u32s,
			) -> Self::u32s {
				let mut out = [0u32; Self::F32_LANES];
				let mask: [m32; Self::F32_LANES] = cast(mask);
				let if_true: [u32; Self::F32_LANES] = cast(if_true);
				let if_false: [u32; Self::F32_LANES] = cast(if_false);

				for i in 0..Self::F32_LANES {
					out[i] = if mask[i].is_set() {
						if_true[i]
					} else {
						if_false[i]
					};
				}

				cast(out)
			}

			#[inline]
			fn select_u64s(
				self,
				mask: Self::m64s,
				if_true: Self::u64s,
				if_false: Self::u64s,
			) -> Self::u64s {
				let mut out = [0u64; Self::F64_LANES];
				let mask: [m64; Self::F64_LANES] = cast(mask);
				let if_true: [u64; Self::F64_LANES] = cast(if_true);
				let if_false: [u64; Self::F64_LANES] = cast(if_false);

				for i in 0..Self::F64_LANES {
					out[i] = if mask[i].is_set() {
						if_true[i]
					} else {
						if_false[i]
					};
				}

				cast(out)
			}

			#[inline]
			fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
				let mut out = [0u32; Self::F32_LANES];
				let a: [u32; Self::F32_LANES] = cast(a);
				let b: [u32; Self::F32_LANES] = cast(amount);
				for i in 0..Self::F32_LANES {
					out[i] = a[i].wrapping_shl(b[i]);
				}
				cast(out)
			}

			#[inline]
			fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
				let mut out = [0u32; Self::F32_LANES];
				let a: [u32; Self::F32_LANES] = cast(a);
				let b: [u32; Self::F32_LANES] = cast(amount);
				for i in 0..Self::F32_LANES {
					out[i] = a[i].wrapping_shr(b[i]);
				}
				cast(out)
			}

			#[inline]
			fn widening_mul_u32s(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
				let mut lo = [0u32; Self::F32_LANES];
				let mut hi = [0u32; Self::F32_LANES];
				let a: [u32; Self::F32_LANES] = cast(a);
				let b: [u32; Self::F32_LANES] = cast(b);
				for i in 0..Self::F32_LANES {
					let m = a[i] as u64 * b[i] as u64;

					(lo[i], hi[i]) = (m as u32, (m >> 32) as u32);
				}
				(cast(lo), cast(hi))
			}

			#[inline]
			fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
				let mut out = [0.0f32; Self::F32_LANES];
				let a: [f32; Self::F32_LANES] = cast(a);
				let b: [f32; Self::F32_LANES] = cast(b);
				let c: [f32; Self::F32_LANES] = cast(c);

				for i in 0..Self::F32_LANES {
					out[i] = fma_f32(a[i], b[i], c[i]);
				}

				cast(out)
			}

			#[inline]
			fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
				let mut a: [f32; Self::F32_LANES] = cast(a);

				let mut n = Self::F32_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i] += a[i + n];
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
				let mut a: [f32; Self::F32_LANES] = cast(a);

				let mut n = Self::F32_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i] *= a[i + n];
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
				let mut a: [f32; Self::F32_LANES] = cast(a);

				let mut n = Self::F32_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i] = f32::min(a[i], a[i + n]);
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
				let mut a: [f32; Self::F32_LANES] = cast(a);

				let mut n = Self::F32_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i] = f32::max(a[i], a[i + n]);
					}
				}

				a[0]
			}

			#[inline]
			fn splat_c32s(self, value: c32) -> Self::c32s {
				cast([value; Self::C32_LANES])
			}

			#[inline]
			fn conj_c32s(self, a: Self::c32s) -> Self::c32s {
				let mut out = [c32::ZERO; Self::C32_LANES];
				let a: [c32; Self::C32_LANES] = cast(a);

				for i in 0..Self::C32_LANES {
					out[i] = c32::new(a[i].re, -a[i].im);
				}

				cast(out)
			}

			#[inline]
			fn neg_c32s(self, a: Self::c32s) -> Self::c32s {
				let mut out = [c32::ZERO; Self::C32_LANES];
				let a: [c32; Self::C32_LANES] = cast(a);

				for i in 0..Self::C32_LANES {
					out[i] = c32::new(-a[i].re, -a[i].im);
				}

				cast(out)
			}

			#[inline]
			fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s {
				let mut out = [c32::ZERO; Self::C32_LANES];
				let a: [c32; Self::C32_LANES] = cast(a);

				for i in 0..Self::C32_LANES {
					out[i] = c32::new(a[i].im, a[i].re);
				}

				cast(out)
			}

			#[inline]
			fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
				let mut out = [c32::ZERO; Self::C32_LANES];
				let a: [c32; Self::C32_LANES] = cast(a);
				let b: [c32; Self::C32_LANES] = cast(b);

				for i in 0..Self::C32_LANES {
					out[i].re = fma_f32(a[i].re, b[i].re, a[i].im * b[i].im);
					out[i].im = fma_f32(a[i].re, b[i].im, -(a[i].im * b[i].re));
				}

				cast(out)
			}

			#[inline]
			fn mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
				let mut out = [c32::ZERO; Self::C32_LANES];
				let a: [c32; Self::C32_LANES] = cast(a);
				let b: [c32; Self::C32_LANES] = cast(b);
				let c: [c32; Self::C32_LANES] = cast(c);

				for i in 0..Self::C32_LANES {
					out[i].re = fma_f32(a[i].re, b[i].re, -fma_f32(a[i].im, b[i].im, -c[i].re));
					out[i].im = fma_f32(a[i].re, b[i].im, fma_f32(a[i].im, b[i].re, c[i].im));
				}

				cast(out)
			}

			#[inline]
			fn conj_mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
				let mut out = [c32::ZERO; Self::C32_LANES];
				let a: [c32; Self::C32_LANES] = cast(a);
				let b: [c32; Self::C32_LANES] = cast(b);
				let c: [c32; Self::C32_LANES] = cast(c);

				for i in 0..Self::C32_LANES {
					out[i].re = fma_f32(a[i].re, b[i].re, fma_f32(a[i].im, b[i].im, c[i].re));
					out[i].im = fma_f32(a[i].re, b[i].im, -fma_f32(a[i].im, b[i].re, -c[i].im));
				}

				cast(out)
			}

			#[inline]
			fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
				let mut out = [c32::ZERO; Self::C32_LANES];
				let a: [c32; Self::C32_LANES] = cast(a);

				for i in 0..Self::C32_LANES {
					let x = a[i].re * a[i].re + a[i].im * a[i].im;
					out[i].re = x;
					out[i].im = x;
				}

				cast(out)
			}

			#[inline]
			fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
				let mut out = [c32::ZERO; Self::C32_LANES];
				let a: [c32; Self::C32_LANES] = cast(self.abs_f32s(a));

				for i in 0..Self::C32_LANES {
					let x = f32::max(a[i].re, a[i].im);
					out[i].re = x;
					out[i].im = x;
				}

				cast(out)
			}

			#[inline]
			fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
				let mut a: [c32; Self::C32_LANES] = cast(a);

				let mut n = Self::C32_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i].re += a[i + n].re;
						a[i].im += a[i + n].im;
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
				let mut a: [c32; Self::C32_LANES] = cast(a);

				let mut n = Self::C32_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i].re = f32::min(a[i].re, a[i + n].re);
						a[i].im = f32::min(a[i].im, a[i + n].im);
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_max_c32s(self, a: Self::c32s) -> c32 {
				let mut a: [c32; Self::C32_LANES] = cast(a);

				let mut n = Self::C32_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i].re = f32::max(a[i].re, a[i + n].re);
						a[i].im = f32::max(a[i].im, a[i + n].im);
					}
				}

				a[0]
			}

			#[inline]
			fn rotate_right_u32s(self, a: Self::u32s, amount: usize) -> Self::u32s {
				let mut a: [u32; Self::F32_LANES] = cast(a);
				let amount = amount % Self::F32_LANES;
				a.rotate_right(amount);
				cast(a)
			}

			#[inline]
			fn rotate_right_c32s(self, a: Self::c32s, amount: usize) -> Self::c32s {
				let mut a: [c32; Self::C32_LANES] = cast(a);
				let amount = amount % Self::C32_LANES;
				a.rotate_right(amount);
				cast(a)
			}

			#[inline]
			fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
				let mut out = [0.0f64; Self::F64_LANES];
				let a: [f64; Self::F64_LANES] = cast(a);
				let b: [f64; Self::F64_LANES] = cast(b);
				let c: [f64; Self::F64_LANES] = cast(c);

				for i in 0..Self::F64_LANES {
					out[i] = fma_f64(a[i], b[i], c[i]);
				}

				cast(out)
			}

			#[inline]
			fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
				let mut a: [f64; Self::F64_LANES] = cast(a);

				let mut n = Self::F64_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i] += a[i + n];
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
				let mut a: [f64; Self::F64_LANES] = cast(a);

				let mut n = Self::F64_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i] *= a[i + n];
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
				let mut a: [f64; Self::F64_LANES] = cast(a);

				let mut n = Self::F64_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i] = f64::min(a[i], a[i + n]);
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
				let mut a: [f64; Self::F64_LANES] = cast(a);

				let mut n = Self::F64_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i] = f64::max(a[i], a[i + n]);
					}
				}

				a[0]
			}

			#[inline]
			fn splat_c64s(self, value: c64) -> Self::c64s {
				cast([value; Self::C64_LANES])
			}

			#[inline]
			fn conj_c64s(self, a: Self::c64s) -> Self::c64s {
				let mut out = [c64::ZERO; Self::C64_LANES];
				let a: [c64; Self::C64_LANES] = cast(a);

				for i in 0..Self::C64_LANES {
					out[i] = c64::new(a[i].re, -a[i].im);
				}

				cast(out)
			}

			#[inline]
			fn neg_c64s(self, a: Self::c64s) -> Self::c64s {
				let mut out = [c64::ZERO; Self::C64_LANES];
				let a: [c64; Self::C64_LANES] = cast(a);

				for i in 0..Self::C64_LANES {
					out[i] = c64::new(-a[i].re, -a[i].im);
				}

				cast(out)
			}

			#[inline]
			fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
				let mut out = [c64::ZERO; Self::C64_LANES];
				let a: [c64; Self::C64_LANES] = cast(a);

				for i in 0..Self::C64_LANES {
					out[i] = c64::new(a[i].im, a[i].re);
				}

				cast(out)
			}

			#[inline]
			fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
				let mut out = [c64::ZERO; Self::C64_LANES];
				let a: [c64; Self::C64_LANES] = cast(a);
				let b: [c64; Self::C64_LANES] = cast(b);

				for i in 0..Self::C64_LANES {
					out[i].re = fma_f64(a[i].re, b[i].re, a[i].im * b[i].im);
					out[i].im = fma_f64(a[i].re, b[i].im, -(a[i].im * b[i].re));
				}

				cast(out)
			}

			#[inline]
			fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
				let mut out = [c64::ZERO; Self::C64_LANES];
				let a: [c64; Self::C64_LANES] = cast(a);
				let b: [c64; Self::C64_LANES] = cast(b);
				let c: [c64; Self::C64_LANES] = cast(c);

				for i in 0..Self::C64_LANES {
					out[i].re = fma_f64(a[i].re, b[i].re, -fma_f64(a[i].im, b[i].im, -c[i].re));
					out[i].im = fma_f64(a[i].re, b[i].im, fma_f64(a[i].im, b[i].re, c[i].im));
				}

				cast(out)
			}

			#[inline]
			fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
				let mut out = [c64::ZERO; Self::C64_LANES];
				let a: [c64; Self::C64_LANES] = cast(a);
				let b: [c64; Self::C64_LANES] = cast(b);
				let c: [c64; Self::C64_LANES] = cast(c);

				for i in 0..Self::C64_LANES {
					out[i].re = fma_f64(a[i].re, b[i].re, fma_f64(a[i].im, b[i].im, c[i].re));
					out[i].im = fma_f64(a[i].re, b[i].im, -fma_f64(a[i].im, b[i].re, -c[i].im));
				}

				cast(out)
			}

			#[inline]
			fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
				let mut out = [c64::ZERO; Self::C64_LANES];
				let a: [c64; Self::C64_LANES] = cast(a);

				for i in 0..Self::C64_LANES {
					let x = a[i].re * a[i].re + a[i].im * a[i].im;
					out[i].re = x;
					out[i].im = x;
				}

				cast(out)
			}

			#[inline]
			fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
				let mut out = [c64::ZERO; Self::C64_LANES];
				let a: [c64; Self::C64_LANES] = cast(self.abs_f64s(a));

				for i in 0..Self::C64_LANES {
					let x = f64::max(a[i].re, a[i].im);
					out[i].re = x;
					out[i].im = x;
				}

				cast(out)
			}

			#[inline]
			fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
				let mut a: [c64; Self::C64_LANES] = cast(a);

				let mut n = Self::C64_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i].re += a[i + n].re;
						a[i].im += a[i + n].im;
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
				let mut a: [c64; Self::C64_LANES] = cast(a);

				let mut n = Self::C64_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i].re = f64::min(a[i].re, a[i + n].re);
						a[i].im = f64::min(a[i].im, a[i + n].im);
					}
				}

				a[0]
			}

			#[inline]
			fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
				let mut a: [c64; Self::C64_LANES] = cast(a);

				let mut n = Self::C64_LANES;
				while n > 1 {
					n /= 2;
					for i in 0..n {
						a[i].re = f64::max(a[i].re, a[i + n].re);
						a[i].im = f64::max(a[i].im, a[i + n].im);
					}
				}

				a[0]
			}

			#[inline]
			fn rotate_right_u64s(self, a: Self::u64s, amount: usize) -> Self::u64s {
				let mut a: [u64; Self::F64_LANES] = cast(a);
				let amount = amount % Self::F64_LANES;
				a.rotate_right(amount);
				cast(a)
			}

			#[inline]
			fn rotate_right_c64s(self, a: Self::c64s, amount: usize) -> Self::c64s {
				let mut a: [c64; Self::C64_LANES] = cast(a);
				let amount = amount % Self::C64_LANES;
				a.rotate_right(amount);
				cast(a)
			}

			#[inline]
			fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
				self.mul_add_f32s(a, b, c)
			}

			#[inline]
			fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
				self.mul_add_f64s(a, b, c)
			}

			#[inline(always)]
			fn sqrt_f32s(self, a: Self::f32s) -> Self::f32s {
				let mut out = [0.0_f32; Self::F32_LANES];
				let a: [f32; Self::F32_LANES] = cast(a);

				for i in 0..Self::F32_LANES {
					out[i] = sqrt_f32(a[i]);
				}

				cast(out)
			}
			#[inline(always)]
			fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
				let mut out = [0.0_f64; Self::F64_LANES];
				let a: [f64; Self::F64_LANES] = cast(a);

				for i in 0..Self::F64_LANES {
					out[i] = sqrt_f64(a[i]);
				}

				cast(out)
			}
		}
	};
}

scalar_simd!(
	Scalar128b, 16, m8x16, i8x16, u8x16, m16x8, i16x8, u16x8, m32x4, f32x4, i32x4, u32x4, m64x2,
	f64x2, i64x2, u64x2
);
scalar_simd!(
	Scalar256b, 16, m8x32, i8x32, u8x32, m16x16, i16x16, u16x16, m32x8, f32x8, i32x8, u32x8, m64x4,
	f64x4, i64x4, u64x4
);
scalar_simd!(
	Scalar512b, 8, m8x64, i8x64, u8x64, m16x32, i16x32, u16x32, m32x16, f32x16, i32x16, u32x16,
	m64x8, f64x8, i64x8, u64x8
);

impl Default for Scalar {
	#[inline]
	fn default() -> Self {
		Self::new()
	}
}

impl Scalar {
	#[inline]
	pub fn new() -> Self {
		Self
	}
}

macro_rules! impl_primitive_binop {
	($func: ident, $op: ident, $ty: ident, $out: ty) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$out s>] {
				a.$op(b)
			}
		}
	};
	(ref $func: ident, $op: ident, $ty: ident, $out: ty) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>], b: Self::[<$ty s>]) -> Self::[<$out s>] {
				a.$op(&b)
			}
		}
	};
}

macro_rules! primitive_binop {
	(ref $func: ident, op $op: ident, $($ty: ident => $out: ty),*) => {
		$(impl_primitive_binop!(ref $func, $op, $ty, $out);)*
	};
	($func: ident, $($ty: ident => $out: ty),*) => {
		$(impl_primitive_binop!($func, $func, $ty, $out);)*
	};
	($func: ident, op $op: ident, $($ty: ident),*) => {
		$(impl_primitive_binop!($func, $op, $ty, $ty);)*
	};
	($func: ident, $($ty: ident),*) => {
		$(impl_primitive_binop!($func, $func, $ty, $ty);)*
	};
}

macro_rules! impl_primitive_unop {
	($func: ident, $op: ident, $ty: ident, $out: ty) => {
		paste! {
			#[inline(always)]
			fn [<$func _ $ty s>](self, a: Self::[<$ty s>]) -> Self::[<$out s>] {
				a.$op()
			}
		}
	};
}

macro_rules! primitive_unop {
	($func: ident, $($ty: ident),*) => {
		$(impl_primitive_unop!($func, $func, $ty, $ty);)*
	};
}

macro_rules! splat_primitive {
	($ty: ty) => {
		paste! {
			#[inline]
			fn [<splat_ $ty s>](self, value: $ty) -> Self::[<$ty s>] {
				value
			}
		}
	};
	($($ty: ty),*) => {
		$(splat_primitive!($ty);)*
	}
}

impl Seal for Scalar {}
impl Simd for Scalar {
	type c32s = c32;
	type c64s = c64;
	type f32s = f32;
	type f64s = f64;
	type i16s = i16;
	type i32s = i32;
	type i64s = i64;
	type i8s = i8;
	type m16s = bool;
	type m32s = bool;
	type m64s = bool;
	type m8s = bool;
	type u16s = u16;
	type u32s = u32;
	type u64s = u64;
	type u8s = u8;

	const IS_SCALAR: bool = true;
	const REGISTER_COUNT: usize = 16;

	primitive_binop!(add, c32, f32, c64, f64);

	primitive_binop!(add, op wrapping_add, u8, i8, u16, i16, u32, i32, u64, i64);

	primitive_binop!(sub, c32, f32, c64, f64);

	primitive_binop!(sub, op wrapping_sub, u8, i8, u16, i16, u32, i32, u64, i64);

	primitive_binop!(mul, f32, f64);

	primitive_binop!(mul, op wrapping_mul, u16, i16, u32, i32, u64, i64);

	primitive_binop!(div, f32, f64);

	primitive_binop!(and, op bitand, m8, u8, m16, u16, m32, u32, m64, u64);

	primitive_binop!(or, op bitor, m8, u8, m16, u16, m32, u32, m64, u64);

	primitive_binop!(xor, op bitxor, m8, u8, m16, u16, m32, u32, m64, u64);

	primitive_binop!(ref equal, op eq, m8 => m8, u8 => m8, m16 => m16, u16 => m16, m32 => m32, u32 => m32, m64 => m64, u64 => m64, c32 => m32, f32 => m32, c64 => m64, f64 => m64);

	primitive_binop!(ref greater_than, op gt, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);

	primitive_binop!(ref greater_than_or_equal, op ge, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);

	primitive_binop!(ref less_than, op lt, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);

	primitive_binop!(ref less_than_or_equal, op le, u8 => m8, i8 => m8, u16 => m16, i16 => m16, u32 => m32, i32 => m32, u64 => m64, i64 => m64, f32 => m32, f64 => m64);

	primitive_binop!(min, u8, i8, u16, i16, u32, i32, u64, i64, f32, f64);

	primitive_binop!(max, u8, i8, u16, i16, u32, i32, u64, i64, f32, f64);

	primitive_unop!(neg, c32, c64, f32, f64);

	primitive_unop!(not, m8, u8, m16, u16, m32, u32, m64, u64);

	splat_primitive!(u8, i8, u16, i16, u32, i32, u64, i64, c32, f32, c64, f64);

	#[inline]
	fn abs2_c32s(self, a: Self::c32s) -> Self::c32s {
		let norm2 = a.re * a.re + a.im * a.im;
		c32::new(norm2, norm2)
	}

	#[inline]
	fn abs2_c64s(self, a: Self::c64s) -> Self::c64s {
		let norm2 = a.re * a.re + a.im * a.im;
		c64::new(norm2, norm2)
	}

	#[inline(always)]
	fn abs_max_c32s(self, a: Self::c32s) -> Self::c32s {
		let re = if a.re > a.im { a.re } else { a.im };
		let im = re;
		Complex { re, im }
	}

	#[inline(always)]
	fn abs_max_c64s(self, a: Self::c64s) -> Self::c64s {
		let re = if a.re > a.im { a.re } else { a.im };
		let im = re;
		Complex { re, im }
	}

	#[inline]
	fn conj_c32s(self, a: Self::c32s) -> Self::c32s {
		a.conj()
	}

	#[inline]
	fn conj_c64s(self, a: Self::c64s) -> Self::c64s {
		a.conj()
	}

	#[inline]
	fn conj_mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		let re = fma_f32(a.re, b.re, fma_f32(a.im, b.im, c.re));
		let im = fma_f32(a.re, b.im, -fma_f32(a.im, b.re, -c.im));
		Complex { re, im }
	}

	#[inline]
	fn conj_mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let re = fma_f64(a.re, b.re, fma_f64(a.im, b.im, c.re));
		let im = fma_f64(a.re, b.im, -fma_f64(a.im, b.re, -c.im));
		Complex { re, im }
	}

	#[inline]
	fn conj_mul_add_e_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		a.conj() * b + c
	}

	#[inline]
	fn conj_mul_add_e_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		a.conj() * b + c
	}

	#[inline]
	fn conj_mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let re = fma_f32(a.re, b.re, a.im * b.im);
		let im = fma_f32(a.re, b.im, -(a.im * b.re));
		Complex { re, im }
	}

	#[inline]
	fn conj_mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let re = fma_f64(a.re, b.re, a.im * b.im);
		let im = fma_f64(a.re, b.im, -(a.im * b.re));
		Complex { re, im }
	}

	#[inline]
	fn conj_mul_e_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		a.conj() * b
	}

	#[inline]
	fn conj_mul_e_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		a.conj() * b
	}

	#[inline(always)]
	fn first_true_m32s(self, mask: Self::m32s) -> usize {
		if mask { 0 } else { 1 }
	}

	#[inline(always)]
	fn first_true_m64s(self, mask: Self::m64s) -> usize {
		if mask { 0 } else { 1 }
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_c32s(self, mask: MemMask<Self::m32s>, ptr: *const c32) -> Self::c32s {
		if mask.mask { *ptr } else { core::mem::zeroed() }
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_c64s(self, mask: MemMask<Self::m64s>, ptr: *const c64) -> Self::c64s {
		if mask.mask { *ptr } else { core::mem::zeroed() }
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_u32s(self, mask: MemMask<Self::m32s>, ptr: *const u32) -> Self::u32s {
		if mask.mask { *ptr } else { 0 }
	}

	#[inline(always)]
	unsafe fn mask_load_ptr_u64s(self, mask: MemMask<Self::m64s>, ptr: *const u64) -> Self::u64s {
		if mask.mask { *ptr } else { 0 }
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_c32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut c32,
		values: Self::c32s,
	) {
		if mask.mask {
			*ptr = values
		}
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_c64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut c64,
		values: Self::c64s,
	) {
		if mask.mask {
			*ptr = values
		}
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *mut u8, values: Self::u8s) {
		if mask.mask {
			*ptr = values
		}
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_u16s(
		self,
		mask: MemMask<Self::m16s>,
		ptr: *mut u16,
		values: Self::u16s,
	) {
		if mask.mask {
			*ptr = values
		}
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_u32s(
		self,
		mask: MemMask<Self::m32s>,
		ptr: *mut u32,
		values: Self::u32s,
	) {
		if mask.mask {
			*ptr = values
		}
	}

	#[inline(always)]
	unsafe fn mask_store_ptr_u64s(
		self,
		mask: MemMask<Self::m64s>,
		ptr: *mut u64,
		values: Self::u64s,
	) {
		if mask.mask {
			*ptr = values
		}
	}

	#[inline]
	fn mul_add_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		let re = fma_f32(a.re, b.re, -fma_f32(a.im, b.im, -c.re));
		let im = fma_f32(a.re, b.im, fma_f32(a.im, b.re, c.im));
		Complex { re, im }
	}

	#[inline]
	fn mul_add_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		let re = fma_f64(a.re, b.re, -fma_f64(a.im, b.im, -c.re));
		let im = fma_f64(a.re, b.im, fma_f64(a.im, b.re, c.im));
		Complex { re, im }
	}

	#[inline]
	fn mul_add_e_c32s(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
		a * b + c
	}

	#[inline]
	fn mul_add_e_c64s(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
		a * b + c
	}

	#[inline(always)]
	fn mul_add_e_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		a * b + c
	}

	#[inline(always)]
	fn mul_add_e_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		a * b + c
	}

	#[inline]
	fn mul_add_f32s(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
		fma_f32(a, b, c)
	}

	#[inline]
	fn mul_add_f64s(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
		fma_f64(a, b, c)
	}

	#[inline]
	fn mul_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		let re = fma_f32(a.re, b.re, -(a.im * b.im));
		let im = fma_f32(a.re, b.im, a.im * b.re);
		Complex { re, im }
	}

	#[inline]
	fn mul_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		let re = fma_f64(a.re, b.re, -(a.im * b.im));
		let im = fma_f64(a.re, b.im, a.im * b.re);
		Complex { re, im }
	}

	#[inline]
	fn mul_e_c32s(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
		a * b
	}

	#[inline]
	fn mul_e_c64s(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
		a * b
	}

	#[inline]
	fn partial_load_c64s(self, slice: &[c64]) -> Self::c64s {
		if let Some((head, _)) = slice.split_first() {
			*head
		} else {
			c64 { re: 0.0, im: 0.0 }
		}
	}

	#[inline]
	fn partial_load_u32s(self, slice: &[u32]) -> Self::u32s {
		if let Some((head, _)) = slice.split_first() {
			*head
		} else {
			0
		}
	}

	#[inline]
	fn partial_load_u64s(self, slice: &[u64]) -> Self::u64s {
		if let Some((head, _)) = slice.split_first() {
			*head
		} else {
			0
		}
	}

	#[inline]
	fn partial_store_c64s(self, slice: &mut [c64], values: Self::c64s) {
		if let Some((head, _)) = slice.split_first_mut() {
			*head = values;
		}
	}

	#[inline]
	fn partial_store_u32s(self, slice: &mut [u32], values: Self::u32s) {
		if let Some((head, _)) = slice.split_first_mut() {
			*head = values;
		}
	}

	#[inline]
	fn partial_store_u64s(self, slice: &mut [u64], values: Self::u64s) {
		if let Some((head, _)) = slice.split_first_mut() {
			*head = values;
		}
	}

	#[inline(always)]
	fn reduce_max_c32s(self, a: Self::c32s) -> c32 {
		a
	}

	#[inline(always)]
	fn reduce_max_c64s(self, a: Self::c64s) -> c64 {
		a
	}

	#[inline]
	fn reduce_max_f32s(self, a: Self::f32s) -> f32 {
		a
	}

	#[inline]
	fn reduce_max_f64s(self, a: Self::f64s) -> f64 {
		a
	}

	#[inline(always)]
	fn reduce_min_c32s(self, a: Self::c32s) -> c32 {
		a
	}

	#[inline(always)]
	fn reduce_min_c64s(self, a: Self::c64s) -> c64 {
		a
	}

	#[inline]
	fn reduce_min_f32s(self, a: Self::f32s) -> f32 {
		a
	}

	#[inline]
	fn reduce_min_f64s(self, a: Self::f64s) -> f64 {
		a
	}

	#[inline]
	fn reduce_product_f32s(self, a: Self::f32s) -> f32 {
		a
	}

	#[inline]
	fn reduce_product_f64s(self, a: Self::f64s) -> f64 {
		a
	}

	#[inline]
	fn reduce_sum_c32s(self, a: Self::c32s) -> c32 {
		a
	}

	#[inline]
	fn reduce_sum_c64s(self, a: Self::c64s) -> c64 {
		a
	}

	#[inline]
	fn reduce_sum_f32s(self, a: Self::f32s) -> f32 {
		a
	}

	#[inline]
	fn reduce_sum_f64s(self, a: Self::f64s) -> f64 {
		a
	}

	#[inline(always)]
	fn rotate_right_c32s(self, a: Self::c32s, _amount: usize) -> Self::c32s {
		a
	}

	#[inline(always)]
	fn rotate_right_c64s(self, a: Self::c64s, _amount: usize) -> Self::c64s {
		a
	}

	#[inline(always)]
	fn rotate_right_u32s(self, a: Self::u32s, _amount: usize) -> Self::u32s {
		a
	}

	#[inline(always)]
	fn rotate_right_u64s(self, a: Self::u64s, _amount: usize) -> Self::u64s {
		a
	}

	#[inline]
	fn select_u32s(
		self,
		mask: Self::m32s,
		if_true: Self::u32s,
		if_false: Self::u32s,
	) -> Self::u32s {
		if mask { if_true } else { if_false }
	}

	#[inline]
	fn select_u64s(
		self,
		mask: Self::m64s,
		if_true: Self::u64s,
		if_false: Self::u64s,
	) -> Self::u64s {
		if mask { if_true } else { if_false }
	}

	#[inline]
	fn swap_re_im_c32s(self, a: Self::c32s) -> Self::c32s {
		c32 { re: a.im, im: a.re }
	}

	fn swap_re_im_c64s(self, a: Self::c64s) -> Self::c64s {
		c64 { re: a.im, im: a.re }
	}

	#[inline]
	fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
		op.with_simd(self)
	}

	#[inline]
	fn widening_mul_u32s(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
		let c = a as u64 * b as u64;
		let lo = c as u32;
		let hi = (c >> 32) as u32;
		(lo, hi)
	}

	#[inline]
	fn wrapping_dyn_shl_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		a.wrapping_shl(amount)
	}

	#[inline]
	fn wrapping_dyn_shr_u32s(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
		a.wrapping_shr(amount)
	}

	unsafe fn mask_load_ptr_u8s(self, mask: MemMask<Self::m8s>, ptr: *const u8) -> Self::u8s {
		if mask.mask { *ptr } else { 0 }
	}

	unsafe fn mask_load_ptr_u16s(self, mask: MemMask<Self::m16s>, ptr: *const u16) -> Self::u16s {
		if mask.mask { *ptr } else { 0 }
	}

	#[inline(always)]
	fn sqrt_f32s(self, a: Self::f32s) -> Self::f32s {
		sqrt_f32(a)
	}

	#[inline(always)]
	fn sqrt_f64s(self, a: Self::f64s) -> Self::f64s {
		sqrt_f64(a)
	}
}

#[inline(always)]
unsafe fn split_slice<T, U>(slice: &[T]) -> (&[U], &[T]) {
	assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
	assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

	let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

	let len = slice.len();
	let data = slice.as_ptr();

	let div = len / chunk_size;
	let rem = len % chunk_size;
	(
		from_raw_parts(data as *const U, div),
		from_raw_parts(data.add(len - rem), rem),
	)
}

#[inline(always)]
unsafe fn split_mut_slice<T, U>(slice: &mut [T]) -> (&mut [U], &mut [T]) {
	assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
	assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

	let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

	let len = slice.len();
	let data = slice.as_mut_ptr();

	let div = len / chunk_size;
	let rem = len % chunk_size;
	(
		from_raw_parts_mut(data as *mut U, div),
		from_raw_parts_mut(data.add(len - rem), rem),
	)
}

#[inline(always)]
unsafe fn rsplit_slice<T, U>(slice: &[T]) -> (&[T], &[U]) {
	assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
	assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

	let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

	let len = slice.len();
	let data = slice.as_ptr();

	let div = len / chunk_size;
	let rem = len % chunk_size;
	(
		from_raw_parts(data, rem),
		from_raw_parts(data.add(rem) as *const U, div),
	)
}

#[inline(always)]
unsafe fn rsplit_mut_slice<T, U>(slice: &mut [T]) -> (&mut [T], &mut [U]) {
	assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
	assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

	let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

	let len = slice.len();
	let data = slice.as_mut_ptr();

	let div = len / chunk_size;
	let rem = len % chunk_size;
	(
		from_raw_parts_mut(data, rem),
		from_raw_parts_mut(data.add(rem) as *mut U, div),
	)
}

match_cfg!(
	item,
	match cfg!() {
		const { any(target_arch = "x86", target_arch = "x86_64") } => {
			pub use x86::Arch;
		},
		const { target_arch = "aarch64" } => {
			pub use aarch64::Arch;
		},
		const { target_arch = "wasm32" } => {
			pub use wasm::Arch;
		},
		_ => {
			#[derive(Debug, Clone, Copy)]
			#[non_exhaustive]
			pub enum Arch {
				Scalar,
			}

			impl Arch {
				#[inline(always)]
				pub fn new() -> Self {
					Self::Scalar
				}

				#[inline(always)]
				pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
					op.with_simd(Scalar)
				}
			}
			impl Default for Arch {
				#[inline]
				fn default() -> Self {
					Self::new()
				}
			}
		},
	}
);

#[doc(hidden)]
pub struct CheckSameSize<T, U>(PhantomData<(T, U)>);
impl<T, U> CheckSameSize<T, U> {
	pub const VALID: () = {
		assert!(core::mem::size_of::<T>() == core::mem::size_of::<U>());
	};
}

#[doc(hidden)]
pub struct CheckSizeLessThanOrEqual<T, U>(PhantomData<(T, U)>);
impl<T, U> CheckSizeLessThanOrEqual<T, U> {
	pub const VALID: () = {
		assert!(core::mem::size_of::<T>() <= core::mem::size_of::<U>());
	};
}

#[macro_export]
macro_rules! static_assert_same_size {
	($t: ty, $u: ty) => {
		let _ = $crate::CheckSameSize::<$t, $u>::VALID;
	};
}
#[macro_export]
macro_rules! static_assert_size_less_than_or_equal {
	($t: ty, $u: ty) => {
		let _ = $crate::CheckSizeLessThanOrEqual::<$t, $u>::VALID;
	};
}

/// Safe transmute function.
///
/// This function asserts at compile time that the two types have the same size.
#[inline(always)]
pub const fn cast<T: NoUninit, U: AnyBitPattern>(value: T) -> U {
	static_assert_same_size!(T, U);
	let ptr = &raw const value as *const U;
	unsafe { ptr.read_unaligned() }
}

/// Safe lossy transmute function, where the destination type may be smaller than the source type.
///
/// This property is checked at compile time.
#[inline(always)]
pub const fn cast_lossy<T: NoUninit, U: AnyBitPattern>(value: T) -> U {
	static_assert_size_less_than_or_equal!(U, T);
	let value = core::mem::ManuallyDrop::new(value);
	let ptr = &raw const value as *const U;
	unsafe { ptr.read_unaligned() }
}

/// Splits a slice into chunks of equal size (known at compile time).
///
/// Returns the chunks and the remaining section of the input slice.
#[inline(always)]
pub fn as_arrays<const N: usize, T>(slice: &[T]) -> (&[[T; N]], &[T]) {
	let n = slice.len();
	let mid_div_n = n / N;
	let mid = mid_div_n * N;
	let ptr = slice.as_ptr();
	unsafe {
		(
			from_raw_parts(ptr as *const [T; N], mid_div_n),
			from_raw_parts(ptr.add(mid), n - mid),
		)
	}
}

/// Splits a slice into chunks of equal size (known at compile time).
///
/// Returns the chunks and the remaining section of the input slice.
#[inline(always)]
pub fn as_arrays_mut<const N: usize, T>(slice: &mut [T]) -> (&mut [[T; N]], &mut [T]) {
	let n = slice.len();
	let mid_div_n = n / N;
	let mid = mid_div_n * N;
	let ptr = slice.as_mut_ptr();
	unsafe {
		(
			from_raw_parts_mut(ptr as *mut [T; N], mid_div_n),
			from_raw_parts_mut(ptr.add(mid), n - mid),
		)
	}
}

/// Platform dependent intrinsics.
pub mod core_arch;

#[allow(unused_macros)]
macro_rules! inherit {
    ({$(
        $(#[$attr: meta])*
        $(unsafe $($placeholder: lifetime)?)?
        fn $func: ident(self
            $(,$arg: ident: $ty: ty)* $(,)?
        ) $(-> $ret: ty)?;
    )*}) => {
        $(
            $(#[$attr])*
            #[inline(always)]
            $(unsafe $($placeholder)?)? fn $func (self, $($arg: $ty,)*) $(-> $ret)? {
                (*self).$func ($($arg,)*)
            }
        )*
    };
}

#[allow(unused_macros)]
macro_rules! inherit_x2 {
    ($base: expr, {$(
        $(#[$attr: meta])*
        $(unsafe $($placeholder: lifetime)?)?
        fn $func: ident ($self: ident
            $(,$arg: ident: $ty: ty)* $(,)?
        ) $(-> $ret: ty)?;
    )*}) => {
        $(
            $(#[$attr])*
            #[inline(always)]
            $(unsafe $($placeholder)?)? fn $func ($self, $($arg: $ty,)*) $(-> $ret)? {
            	$(let $arg: [_; 2] = cast!($arg);)*
                cast!([($base).$func ($($arg[0],)*), ($base).$func ($($arg[1],)*)])
            }
        )*
    };

    ($base: expr, splat, {$(
        $(#[$attr: meta])*
        $(unsafe $($placeholder: lifetime)?)?
        fn $func: ident ($self: ident
            $(,$arg: ident: $ty: ty)* $(,)?
        ) $(-> $ret: ty)?;
    )*}) => {
        $(
            $(#[$attr])*
            #[inline(always)]
            $(unsafe $($placeholder)?)? fn $func ($self, $($arg: $ty,)*) $(-> $ret)? {
                cast!([($base).$func ($($arg,)*), ($base).$func ($($arg,)*)])
            }
        )*
    };

    ($base: expr, wide, {$(
        $(#[$attr: meta])*
        $(unsafe $($placeholder: lifetime)?)?
        fn $func: ident ($self: ident
            $(,$arg: ident: $ty: ty)* $(,)?
        ) $(-> $ret: ty)?;
    )*}) => {
        $(
            $(#[$attr])*
            #[inline(always)]
            $(unsafe $($placeholder)?)? fn $func ($self, $($arg: $ty,)*) $(-> $ret)? {
            	$(let $arg: [_; 2] = cast!($arg);)*
                let (r0, r1) = ($base).$func ($($arg[0],)*); let (s0, s1) = ($base).$func ($($arg[1],)*);
                (cast!([r0, s0]), cast!([r1, s1]))
            }
        )*
    };
}

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[cfg_attr(docsrs, doc(cfg(any(target_arch = "x86", target_arch = "x86_64"))))]
/// Low level x86 API.
pub mod x86;

#[cfg(target_arch = "wasm32")]
#[cfg_attr(docsrs, doc(cfg(target_arch = "wasm32")))]
/// Low level wasm API.
pub mod wasm;

#[cfg(target_arch = "aarch64")]
#[cfg_attr(docsrs, doc(cfg(target_arch = "aarch64")))]
/// Low level aarch64 API.
pub mod aarch64;

/// Mask type with 8 bits. Its bit pattern is either all ones or all zeros. Unsafe code must not
/// depend on this, however.
#[derive(Copy, Clone, PartialEq, Eq, Default)]
#[repr(transparent)]
pub struct m8(u8);
/// Mask type with 16 bits. Its bit pattern is either all ones or all zeros. Unsafe code must not
/// depend on this, however.
#[derive(Copy, Clone, PartialEq, Eq, Default)]
#[repr(transparent)]
pub struct m16(u16);
/// Mask type with 32 bits. Its bit pattern is either all ones or all zeros. Unsafe code must not
/// depend on this, however.
#[derive(Copy, Clone, PartialEq, Eq, Default)]
#[repr(transparent)]
pub struct m32(u32);
/// Mask type with 64 bits. Its bit pattern is either all ones or all zeros. Unsafe code must not
/// depend on this, however.
#[derive(Copy, Clone, PartialEq, Eq, Default)]
#[repr(transparent)]
pub struct m64(u64);

/// Bitmask type for 8 elements, used for mask operations on AVX512.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct b8(pub u8);
/// Bitmask type for 16 elements, used for mask operations on AVX512.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct b16(pub u16);
/// Bitmask type for 32 elements, used for mask operations on AVX512.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct b32(pub u32);
/// Bitmask type for 64 elements, used for mask operations on AVX512.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct b64(pub u64);

impl core::ops::Not for b8 {
	type Output = b8;

	#[inline(always)]
	fn not(self) -> Self::Output {
		b8(!self.0)
	}
}
impl core::ops::BitAnd for b8 {
	type Output = b8;

	#[inline(always)]
	fn bitand(self, rhs: Self) -> Self::Output {
		b8(self.0 & rhs.0)
	}
}
impl core::ops::BitOr for b8 {
	type Output = b8;

	#[inline(always)]
	fn bitor(self, rhs: Self) -> Self::Output {
		b8(self.0 | rhs.0)
	}
}
impl core::ops::BitXor for b8 {
	type Output = b8;

	#[inline(always)]
	fn bitxor(self, rhs: Self) -> Self::Output {
		b8(self.0 ^ rhs.0)
	}
}

impl core::ops::Not for m8 {
	type Output = m8;

	#[inline(always)]
	fn not(self) -> Self::Output {
		m8(!self.0)
	}
}
impl core::ops::BitAnd for m8 {
	type Output = m8;

	#[inline(always)]
	fn bitand(self, rhs: Self) -> Self::Output {
		m8(self.0 & rhs.0)
	}
}
impl core::ops::BitOr for m8 {
	type Output = m8;

	#[inline(always)]
	fn bitor(self, rhs: Self) -> Self::Output {
		m8(self.0 | rhs.0)
	}
}
impl core::ops::BitXor for m8 {
	type Output = m8;

	#[inline(always)]
	fn bitxor(self, rhs: Self) -> Self::Output {
		m8(self.0 ^ rhs.0)
	}
}

impl core::ops::Not for m16 {
	type Output = m16;

	#[inline(always)]
	fn not(self) -> Self::Output {
		m16(!self.0)
	}
}
impl core::ops::BitAnd for m16 {
	type Output = m16;

	#[inline(always)]
	fn bitand(self, rhs: Self) -> Self::Output {
		m16(self.0 & rhs.0)
	}
}
impl core::ops::BitOr for m16 {
	type Output = m16;

	#[inline(always)]
	fn bitor(self, rhs: Self) -> Self::Output {
		m16(self.0 | rhs.0)
	}
}
impl core::ops::BitXor for m16 {
	type Output = m16;

	#[inline(always)]
	fn bitxor(self, rhs: Self) -> Self::Output {
		m16(self.0 ^ rhs.0)
	}
}

impl core::ops::Not for m32 {
	type Output = m32;

	#[inline(always)]
	fn not(self) -> Self::Output {
		m32(!self.0)
	}
}
impl core::ops::BitAnd for m32 {
	type Output = m32;

	#[inline(always)]
	fn bitand(self, rhs: Self) -> Self::Output {
		m32(self.0 & rhs.0)
	}
}
impl core::ops::BitOr for m32 {
	type Output = m32;

	#[inline(always)]
	fn bitor(self, rhs: Self) -> Self::Output {
		m32(self.0 | rhs.0)
	}
}
impl core::ops::BitXor for m32 {
	type Output = m32;

	#[inline(always)]
	fn bitxor(self, rhs: Self) -> Self::Output {
		m32(self.0 ^ rhs.0)
	}
}

impl core::ops::Not for m64 {
	type Output = m64;

	#[inline(always)]
	fn not(self) -> Self::Output {
		m64(!self.0)
	}
}
impl core::ops::BitAnd for m64 {
	type Output = m64;

	#[inline(always)]
	fn bitand(self, rhs: Self) -> Self::Output {
		m64(self.0 & rhs.0)
	}
}
impl core::ops::BitOr for m64 {
	type Output = m64;

	#[inline(always)]
	fn bitor(self, rhs: Self) -> Self::Output {
		m64(self.0 | rhs.0)
	}
}
impl core::ops::BitXor for m64 {
	type Output = m64;

	#[inline(always)]
	fn bitxor(self, rhs: Self) -> Self::Output {
		m64(self.0 ^ rhs.0)
	}
}

impl core::ops::Not for b16 {
	type Output = b16;

	#[inline(always)]
	fn not(self) -> Self::Output {
		b16(!self.0)
	}
}
impl core::ops::BitAnd for b16 {
	type Output = b16;

	#[inline(always)]
	fn bitand(self, rhs: Self) -> Self::Output {
		b16(self.0 & rhs.0)
	}
}
impl core::ops::BitOr for b16 {
	type Output = b16;

	#[inline(always)]
	fn bitor(self, rhs: Self) -> Self::Output {
		b16(self.0 | rhs.0)
	}
}
impl core::ops::BitXor for b16 {
	type Output = b16;

	#[inline(always)]
	fn bitxor(self, rhs: Self) -> Self::Output {
		b16(self.0 ^ rhs.0)
	}
}

impl core::ops::Not for b32 {
	type Output = b32;

	#[inline(always)]
	fn not(self) -> Self::Output {
		b32(!self.0)
	}
}
impl core::ops::BitAnd for b32 {
	type Output = b32;

	#[inline(always)]
	fn bitand(self, rhs: Self) -> Self::Output {
		b32(self.0 & rhs.0)
	}
}
impl core::ops::BitOr for b32 {
	type Output = b32;

	#[inline(always)]
	fn bitor(self, rhs: Self) -> Self::Output {
		b32(self.0 | rhs.0)
	}
}
impl core::ops::BitXor for b32 {
	type Output = b32;

	#[inline(always)]
	fn bitxor(self, rhs: Self) -> Self::Output {
		b32(self.0 ^ rhs.0)
	}
}

impl core::ops::Not for b64 {
	type Output = b64;

	#[inline(always)]
	fn not(self) -> Self::Output {
		b64(!self.0)
	}
}
impl core::ops::BitAnd for b64 {
	type Output = b64;

	#[inline(always)]
	fn bitand(self, rhs: Self) -> Self::Output {
		b64(self.0 & rhs.0)
	}
}
impl core::ops::BitOr for b64 {
	type Output = b64;

	#[inline(always)]
	fn bitor(self, rhs: Self) -> Self::Output {
		b64(self.0 | rhs.0)
	}
}
impl core::ops::BitXor for b64 {
	type Output = b64;

	#[inline(always)]
	fn bitxor(self, rhs: Self) -> Self::Output {
		b64(self.0 ^ rhs.0)
	}
}

impl Debug for b8 {
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
		#[allow(dead_code)]
		#[derive(Copy, Clone, Debug)]
		struct b8(bool, bool, bool, bool, bool, bool, bool, bool);
		b8(
			((self.0 >> 0) & 1) == 1,
			((self.0 >> 1) & 1) == 1,
			((self.0 >> 2) & 1) == 1,
			((self.0 >> 3) & 1) == 1,
			((self.0 >> 4) & 1) == 1,
			((self.0 >> 5) & 1) == 1,
			((self.0 >> 6) & 1) == 1,
			((self.0 >> 7) & 1) == 1,
		)
		.fmt(f)
	}
}
impl Debug for b16 {
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
		#[allow(dead_code)]
		#[derive(Copy, Clone, Debug)]
		struct b16(
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
		);
		b16(
			((self.0 >> 00) & 1) == 1,
			((self.0 >> 01) & 1) == 1,
			((self.0 >> 02) & 1) == 1,
			((self.0 >> 03) & 1) == 1,
			((self.0 >> 04) & 1) == 1,
			((self.0 >> 05) & 1) == 1,
			((self.0 >> 06) & 1) == 1,
			((self.0 >> 07) & 1) == 1,
			((self.0 >> 08) & 1) == 1,
			((self.0 >> 09) & 1) == 1,
			((self.0 >> 10) & 1) == 1,
			((self.0 >> 11) & 1) == 1,
			((self.0 >> 12) & 1) == 1,
			((self.0 >> 13) & 1) == 1,
			((self.0 >> 14) & 1) == 1,
			((self.0 >> 15) & 1) == 1,
		)
		.fmt(f)
	}
}
impl Debug for b32 {
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
		#[allow(dead_code)]
		#[derive(Copy, Clone, Debug)]
		struct b32(
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
		);
		b32(
			((self.0 >> 00) & 1) == 1,
			((self.0 >> 01) & 1) == 1,
			((self.0 >> 02) & 1) == 1,
			((self.0 >> 03) & 1) == 1,
			((self.0 >> 04) & 1) == 1,
			((self.0 >> 05) & 1) == 1,
			((self.0 >> 06) & 1) == 1,
			((self.0 >> 07) & 1) == 1,
			((self.0 >> 08) & 1) == 1,
			((self.0 >> 09) & 1) == 1,
			((self.0 >> 10) & 1) == 1,
			((self.0 >> 11) & 1) == 1,
			((self.0 >> 12) & 1) == 1,
			((self.0 >> 13) & 1) == 1,
			((self.0 >> 14) & 1) == 1,
			((self.0 >> 15) & 1) == 1,
			((self.0 >> 16) & 1) == 1,
			((self.0 >> 17) & 1) == 1,
			((self.0 >> 18) & 1) == 1,
			((self.0 >> 19) & 1) == 1,
			((self.0 >> 20) & 1) == 1,
			((self.0 >> 21) & 1) == 1,
			((self.0 >> 22) & 1) == 1,
			((self.0 >> 23) & 1) == 1,
			((self.0 >> 24) & 1) == 1,
			((self.0 >> 25) & 1) == 1,
			((self.0 >> 26) & 1) == 1,
			((self.0 >> 27) & 1) == 1,
			((self.0 >> 28) & 1) == 1,
			((self.0 >> 29) & 1) == 1,
			((self.0 >> 30) & 1) == 1,
			((self.0 >> 31) & 1) == 1,
		)
		.fmt(f)
	}
}
impl Debug for b64 {
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
		#[allow(dead_code)]
		#[derive(Copy, Clone, Debug)]
		struct b64(
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
			bool,
		);
		b64(
			((self.0 >> 00) & 1) == 1,
			((self.0 >> 01) & 1) == 1,
			((self.0 >> 02) & 1) == 1,
			((self.0 >> 03) & 1) == 1,
			((self.0 >> 04) & 1) == 1,
			((self.0 >> 05) & 1) == 1,
			((self.0 >> 06) & 1) == 1,
			((self.0 >> 07) & 1) == 1,
			((self.0 >> 08) & 1) == 1,
			((self.0 >> 09) & 1) == 1,
			((self.0 >> 10) & 1) == 1,
			((self.0 >> 11) & 1) == 1,
			((self.0 >> 12) & 1) == 1,
			((self.0 >> 13) & 1) == 1,
			((self.0 >> 14) & 1) == 1,
			((self.0 >> 15) & 1) == 1,
			((self.0 >> 16) & 1) == 1,
			((self.0 >> 17) & 1) == 1,
			((self.0 >> 18) & 1) == 1,
			((self.0 >> 19) & 1) == 1,
			((self.0 >> 20) & 1) == 1,
			((self.0 >> 21) & 1) == 1,
			((self.0 >> 22) & 1) == 1,
			((self.0 >> 23) & 1) == 1,
			((self.0 >> 24) & 1) == 1,
			((self.0 >> 25) & 1) == 1,
			((self.0 >> 26) & 1) == 1,
			((self.0 >> 27) & 1) == 1,
			((self.0 >> 28) & 1) == 1,
			((self.0 >> 29) & 1) == 1,
			((self.0 >> 30) & 1) == 1,
			((self.0 >> 31) & 1) == 1,
			((self.0 >> 32) & 1) == 1,
			((self.0 >> 33) & 1) == 1,
			((self.0 >> 34) & 1) == 1,
			((self.0 >> 35) & 1) == 1,
			((self.0 >> 36) & 1) == 1,
			((self.0 >> 37) & 1) == 1,
			((self.0 >> 38) & 1) == 1,
			((self.0 >> 39) & 1) == 1,
			((self.0 >> 40) & 1) == 1,
			((self.0 >> 41) & 1) == 1,
			((self.0 >> 42) & 1) == 1,
			((self.0 >> 43) & 1) == 1,
			((self.0 >> 44) & 1) == 1,
			((self.0 >> 45) & 1) == 1,
			((self.0 >> 46) & 1) == 1,
			((self.0 >> 47) & 1) == 1,
			((self.0 >> 48) & 1) == 1,
			((self.0 >> 49) & 1) == 1,
			((self.0 >> 50) & 1) == 1,
			((self.0 >> 51) & 1) == 1,
			((self.0 >> 52) & 1) == 1,
			((self.0 >> 53) & 1) == 1,
			((self.0 >> 54) & 1) == 1,
			((self.0 >> 55) & 1) == 1,
			((self.0 >> 56) & 1) == 1,
			((self.0 >> 57) & 1) == 1,
			((self.0 >> 58) & 1) == 1,
			((self.0 >> 59) & 1) == 1,
			((self.0 >> 60) & 1) == 1,
			((self.0 >> 61) & 1) == 1,
			((self.0 >> 62) & 1) == 1,
			((self.0 >> 63) & 1) == 1,
		)
		.fmt(f)
	}
}

impl Debug for m8 {
	#[inline]
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
		self.is_set().fmt(f)
	}
}
impl Debug for m16 {
	#[inline]
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
		self.is_set().fmt(f)
	}
}
impl Debug for m32 {
	#[inline]
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
		self.is_set().fmt(f)
	}
}
impl Debug for m64 {
	#[inline]
	fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
		self.is_set().fmt(f)
	}
}

impl m8 {
	/// Returns a mask with all bits set one, if `flag` is true, otherwise returns a mask with all
	/// bits set to zero.
	#[inline(always)]
	pub const fn new(flag: bool) -> Self {
		Self(if flag { u8::MAX } else { 0 })
	}

	/// Returns `false` if the mask bits are all zero, otherwise returns `true`.
	#[inline(always)]
	pub const fn is_set(self) -> bool {
		self.0 != 0
	}
}
impl m16 {
	/// Returns a mask with all bits set one, if `flag` is true, otherwise returns a mask with all
	/// bits set to zero.
	#[inline(always)]
	pub const fn new(flag: bool) -> Self {
		Self(if flag { u16::MAX } else { 0 })
	}

	/// Returns `false` if the mask bits are all zero, otherwise returns `true`.
	#[inline(always)]
	pub const fn is_set(self) -> bool {
		self.0 != 0
	}
}
impl m32 {
	/// Returns a mask with all bits set one, if `flag` is true, otherwise returns a mask with all
	/// bits set to zero.
	#[inline(always)]
	pub const fn new(flag: bool) -> Self {
		Self(if flag { u32::MAX } else { 0 })
	}

	/// Returns `false` if the mask bits are all zero, otherwise returns `true`.
	#[inline(always)]
	pub const fn is_set(self) -> bool {
		self.0 != 0
	}
}
impl m64 {
	/// Returns a mask with all bits set one, if `flag` is true, otherwise returns a mask with all
	/// bits set to zero.
	#[inline(always)]
	pub const fn new(flag: bool) -> Self {
		Self(if flag { u64::MAX } else { 0 })
	}

	/// Returns `false` if the mask bits are all zero, otherwise returns `true`.
	#[inline(always)]
	pub const fn is_set(self) -> bool {
		self.0 != 0
	}
}

/// A 128-bit SIMD vector with 16 elements of type [`i8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i8x16(
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
);
/// A 256-bit SIMD vector with 32 elements of type [`i8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i8x32(
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
);
/// A 512-bit SIMD vector with 64 elements of type [`i8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i8x64(
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
	pub i8,
);

/// A 128-bit SIMD vector with 16 elements of type [`u8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u8x16(
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
);
/// A 256-bit SIMD vector with 32 elements of type [`u8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u8x32(
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
);
/// A 512-bit SIMD vector with 64 elements of type [`u8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u8x64(
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
	pub u8,
);

/// A 128-bit SIMD vector with 16 elements of type [`m8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m8x16(
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
);
/// A 256-bit SIMD vector with 32 elements of type [`m8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m8x32(
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
);

/// A 512-bit SIMD vector with 64 elements of type [`m8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m8x64(
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
	pub m8,
);

/// A 128-bit SIMD vector with 8 elements of type [`i16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i16x8(
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
);
/// A 256-bit SIMD vector with 16 elements of type [`i16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i16x16(
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
);
/// A 512-bit SIMD vector with 32 elements of type [`i16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i16x32(
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
	pub i16,
);

/// A 128-bit SIMD vector with 8 elements of type [`u16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u16x8(
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
);
/// A 256-bit SIMD vector with 16 elements of type [`u16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u16x16(
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
);
/// A 512-bit SIMD vector with 32 elements of type [`u16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u16x32(
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
	pub u16,
);

/// A 128-bit SIMD vector with 8 elements of type [`m16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m16x8(
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
);
/// A 256-bit SIMD vector with 16 elements of type [`m16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m16x16(
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
);
/// A 512-bit SIMD vector with 32 elements of type [`m16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m16x32(
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
	pub m16,
);

/// A 128-bit SIMD vector with 4 elements of type [`f32`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f32x4(pub f32, pub f32, pub f32, pub f32);

/// A 256-bit SIMD vector with 8 elements of type [`f32`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f32x8(
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
);
/// A 512-bit SIMD vector with 16 elements of type [`f32`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f32x16(
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
	pub f32,
);

/// A 128-bit SIMD vector with 2 elements of type [`c32`].
#[derive(Copy, Clone, PartialEq)]
#[repr(C)]
pub struct c32x2(pub c32, pub c32);

impl Debug for c32x2 {
	fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
		#[derive(Copy, Clone, Debug)]
		#[repr(C)]
		pub struct c32x2(pub DebugCplx<c32>, pub DebugCplx<c32>);
		unsafe impl Zeroable for c32x2 {}
		unsafe impl Pod for c32x2 {}

		let this: c32x2 = cast!(*self);
		this.fmt(f)
	}
}

/// A 256-bit SIMD vector with 4 elements of type [`c32`].
#[derive(Copy, Clone, PartialEq)]
#[repr(C)]
pub struct c32x4(pub c32, pub c32, pub c32, pub c32);

impl Debug for c32x4 {
	fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
		#[derive(Copy, Clone, Debug)]
		#[repr(C)]
		pub struct c32x4(
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
		);
		unsafe impl Zeroable for c32x4 {}
		unsafe impl Pod for c32x4 {}

		let this: c32x4 = cast!(*self);
		this.fmt(f)
	}
}

/// A 512-bit SIMD vector with 8 elements of type [`c32`].
#[derive(Copy, Clone, PartialEq)]
#[repr(C)]
pub struct c32x8(
	pub c32,
	pub c32,
	pub c32,
	pub c32,
	pub c32,
	pub c32,
	pub c32,
	pub c32,
);

impl Debug for c32x8 {
	fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
		#[derive(Copy, Clone, Debug)]
		#[repr(C)]
		pub struct c32x8(
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
			pub DebugCplx<c32>,
		);
		unsafe impl Zeroable for c32x8 {}
		unsafe impl Pod for c32x8 {}

		let this: c32x8 = cast!(*self);
		this.fmt(f)
	}
}
/// A 128-bit SIMD vector with 4 elements of type [`i32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i32x4(pub i32, pub i32, pub i32, pub i32);
/// A 256-bit SIMD vector with 8 elements of type [`i32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i32x8(
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
);
/// A 512-bit SIMD vector with 16 elements of type [`i32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i32x16(
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
	pub i32,
);

/// A 128-bit SIMD vector with 4 elements of type [`u32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u32x4(pub u32, pub u32, pub u32, pub u32);
/// A 256-bit SIMD vector with 8 elements of type [`u32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u32x8(
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
);
/// A 512-bit SIMD vector with 16 elements of type [`u32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u32x16(
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
	pub u32,
);

/// A 128-bit SIMD vector with 4 elements of type [`m32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m32x4(pub m32, pub m32, pub m32, pub m32);
/// A 256-bit SIMD vector with 8 elements of type [`m32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m32x8(
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
);
/// A 512-bit SIMD vector with 16 elements of type [`m32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m32x16(
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
	pub m32,
);

/// A 128-bit SIMD vector with 2 elements of type [`f64`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f64x2(pub f64, pub f64);
/// A 256-bit SIMD vector with 4 elements of type [`f64`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f64x4(pub f64, pub f64, pub f64, pub f64);
/// A 512-bit SIMD vector with 8 elements of type [`f64`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f64x8(
	pub f64,
	pub f64,
	pub f64,
	pub f64,
	pub f64,
	pub f64,
	pub f64,
	pub f64,
);

/// A 128-bit SIMD vector with 1 elements of type [`c64`].
#[derive(Copy, Clone, PartialEq)]
#[repr(C)]
pub struct c64x1(pub c64);

impl Debug for c64x1 {
	fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
		#[derive(Copy, Clone, Debug)]
		#[repr(C)]
		pub struct c64x1(pub DebugCplx<c64>);
		unsafe impl Zeroable for c64x1 {}
		unsafe impl Pod for c64x1 {}

		let this: c64x1 = cast!(*self);
		this.fmt(f)
	}
}

/// A 256-bit SIMD vector with 2 elements of type [`c64`].
#[derive(Copy, Clone, PartialEq)]
#[repr(C)]
pub struct c64x2(pub c64, pub c64);

impl Debug for c64x2 {
	fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
		#[derive(Copy, Clone, Debug)]
		#[repr(C)]
		pub struct c64x2(pub DebugCplx<c64>, pub DebugCplx<c64>);
		unsafe impl Zeroable for c64x2 {}
		unsafe impl Pod for c64x2 {}

		let this: c64x2 = cast!(*self);
		this.fmt(f)
	}
}

/// A 512-bit SIMD vector with 4 elements of type [`c64`].
#[derive(Copy, Clone, PartialEq)]
#[repr(C)]
pub struct c64x4(pub c64, pub c64, pub c64, pub c64);

impl Debug for c64x4 {
	fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
		#[derive(Copy, Clone, Debug)]
		#[repr(C)]
		pub struct c64x4(
			pub DebugCplx<c64>,
			pub DebugCplx<c64>,
			pub DebugCplx<c64>,
			pub DebugCplx<c64>,
		);
		unsafe impl Zeroable for c64x4 {}
		unsafe impl Pod for c64x4 {}

		let this: c64x4 = cast!(*self);
		this.fmt(f)
	}
}

/// A 128-bit SIMD vector with 2 elements of type [`i64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i64x2(pub i64, pub i64);
/// A 256-bit SIMD vector with 4 elements of type [`i64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i64x4(pub i64, pub i64, pub i64, pub i64);
/// A 512-bit SIMD vector with 8 elements of type [`i64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i64x8(
	pub i64,
	pub i64,
	pub i64,
	pub i64,
	pub i64,
	pub i64,
	pub i64,
	pub i64,
);

/// A 128-bit SIMD vector with 2 elements of type [`u64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u64x2(pub u64, pub u64);
/// A 256-bit SIMD vector with 4 elements of type [`u64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u64x4(pub u64, pub u64, pub u64, pub u64);
/// A 512-bit SIMD vector with 8 elements of type [`u64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u64x8(
	pub u64,
	pub u64,
	pub u64,
	pub u64,
	pub u64,
	pub u64,
	pub u64,
	pub u64,
);

/// A 128-bit SIMD vector with 2 elements of type [`m64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m64x2(pub m64, pub m64);
/// A 256-bit SIMD vector with 4 elements of type [`m64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m64x4(pub m64, pub m64, pub m64, pub m64);
/// A 512-bit SIMD vector with 8 elements of type [`m64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m64x8(
	pub m64,
	pub m64,
	pub m64,
	pub m64,
	pub m64,
	pub m64,
	pub m64,
	pub m64,
);

unsafe impl Zeroable for m8 {}
unsafe impl Zeroable for m16 {}
unsafe impl Zeroable for m32 {}
unsafe impl Zeroable for m64 {}
unsafe impl Pod for m8 {}
unsafe impl Pod for m16 {}
unsafe impl Pod for m32 {}
unsafe impl Pod for m64 {}

unsafe impl Zeroable for b8 {}
unsafe impl Pod for b8 {}
unsafe impl Zeroable for b16 {}
unsafe impl Pod for b16 {}
unsafe impl Zeroable for b32 {}
unsafe impl Pod for b32 {}
unsafe impl Zeroable for b64 {}
unsafe impl Pod for b64 {}

unsafe impl Zeroable for i8x16 {}
unsafe impl Zeroable for i8x32 {}
unsafe impl Zeroable for i8x64 {}
unsafe impl Pod for i8x16 {}
unsafe impl Pod for i8x32 {}
unsafe impl Pod for i8x64 {}
unsafe impl Zeroable for u8x16 {}
unsafe impl Zeroable for u8x32 {}
unsafe impl Zeroable for u8x64 {}
unsafe impl Pod for u8x16 {}
unsafe impl Pod for u8x32 {}
unsafe impl Pod for u8x64 {}
unsafe impl Zeroable for m8x16 {}
unsafe impl Zeroable for m8x32 {}
unsafe impl Zeroable for m8x64 {}
unsafe impl Pod for m8x16 {}
unsafe impl Pod for m8x32 {}
unsafe impl Pod for m8x64 {}

unsafe impl Zeroable for i16x8 {}
unsafe impl Zeroable for i16x16 {}
unsafe impl Zeroable for i16x32 {}
unsafe impl Pod for i16x8 {}
unsafe impl Pod for i16x16 {}
unsafe impl Pod for i16x32 {}
unsafe impl Zeroable for u16x8 {}
unsafe impl Zeroable for u16x16 {}
unsafe impl Zeroable for u16x32 {}
unsafe impl Pod for u16x8 {}
unsafe impl Pod for u16x16 {}
unsafe impl Pod for u16x32 {}
unsafe impl Zeroable for m16x8 {}
unsafe impl Zeroable for m16x16 {}
unsafe impl Zeroable for m16x32 {}
unsafe impl Pod for m16x8 {}
unsafe impl Pod for m16x16 {}
unsafe impl Pod for m16x32 {}

unsafe impl Zeroable for f32x4 {}
unsafe impl Zeroable for f32x8 {}
unsafe impl Zeroable for f32x16 {}
unsafe impl Pod for f32x4 {}
unsafe impl Pod for f32x8 {}
unsafe impl Pod for f32x16 {}
unsafe impl Zeroable for c32x2 {}
unsafe impl Zeroable for c32x4 {}
unsafe impl Zeroable for c32x8 {}
unsafe impl Pod for c32x2 {}
unsafe impl Pod for c32x4 {}
unsafe impl Pod for c32x8 {}
unsafe impl Zeroable for i32x4 {}
unsafe impl Zeroable for i32x8 {}
unsafe impl Zeroable for i32x16 {}
unsafe impl Pod for i32x4 {}
unsafe impl Pod for i32x8 {}
unsafe impl Pod for i32x16 {}
unsafe impl Zeroable for u32x4 {}
unsafe impl Zeroable for u32x8 {}
unsafe impl Zeroable for u32x16 {}
unsafe impl Pod for u32x4 {}
unsafe impl Pod for u32x8 {}
unsafe impl Pod for u32x16 {}
unsafe impl Zeroable for m32x4 {}
unsafe impl Zeroable for m32x8 {}
unsafe impl Zeroable for m32x16 {}
unsafe impl Pod for m32x4 {}
unsafe impl Pod for m32x8 {}
unsafe impl Pod for m32x16 {}

unsafe impl Zeroable for f64x2 {}
unsafe impl Zeroable for f64x4 {}
unsafe impl Zeroable for f64x8 {}
unsafe impl Pod for f64x2 {}
unsafe impl Pod for f64x4 {}
unsafe impl Pod for f64x8 {}
unsafe impl Zeroable for c64x1 {}
unsafe impl Zeroable for c64x2 {}
unsafe impl Zeroable for c64x4 {}
unsafe impl Pod for c64x1 {}
unsafe impl Pod for c64x2 {}
unsafe impl Pod for c64x4 {}
unsafe impl Zeroable for i64x2 {}
unsafe impl Zeroable for i64x4 {}
unsafe impl Zeroable for i64x8 {}
unsafe impl Pod for i64x2 {}
unsafe impl Pod for i64x4 {}
unsafe impl Pod for i64x8 {}
unsafe impl Zeroable for u64x2 {}
unsafe impl Zeroable for u64x4 {}
unsafe impl Zeroable for u64x8 {}
unsafe impl Pod for u64x2 {}
unsafe impl Pod for u64x4 {}
unsafe impl Pod for u64x8 {}
unsafe impl Zeroable for m64x2 {}
unsafe impl Zeroable for m64x4 {}
unsafe impl Zeroable for m64x8 {}
unsafe impl Pod for m64x2 {}
unsafe impl Pod for m64x4 {}
unsafe impl Pod for m64x8 {}

macro_rules! iota {
	($T: ty, $N: expr, $int: ty) => {
		const {
			unsafe {
				let mut iota = [const { core::mem::MaybeUninit::uninit() }; $N];
				{
					let mut i = 0;
					while i < $N {
						let v = (&raw mut iota[i]) as *mut $int;

						let mut j = 0;
						while j < core::mem::size_of::<$T>() / core::mem::size_of::<$int>() {
							v.add(j).write_unaligned(i as $int);
							j += 1;
						}

						i += 1;
					}
				}
				iota
			}
		}
	};
}

pub const fn iota_8<T: Interleave, const N: usize>() -> [MaybeUninit<T>; N] {
	iota!(T, N, u8)
}
pub const fn iota_16<T: Interleave, const N: usize>() -> [MaybeUninit<T>; N] {
	iota!(T, N, u16)
}
pub const fn iota_32<T: Interleave, const N: usize>() -> [MaybeUninit<T>; N] {
	iota!(T, N, u32)
}
pub const fn iota_64<T: Interleave, const N: usize>() -> [MaybeUninit<T>; N] {
	iota!(T, N, u64)
}

#[cfg(target_arch = "x86_64")]
#[cfg(test)]
mod tests {
	use super::*;

	#[test]
	fn test_interleave() {
		if let Some(simd) = x86::V3::try_new() {
			{
				let src = [f64x4(0.0, 0.1, 1.0, 1.1), f64x4(2.0, 2.1, 3.0, 3.1)];
				let dst = unsafe { deinterleave_fallback::<f64, f64x4, [f64x4; 2]>(src) };
				assert_eq!(dst[1], simd.add_f64x4(dst[0], simd.splat_f64x4(0.1)));
				assert_eq!(src, unsafe {
					interleave_fallback::<f64, f64x4, [f64x4; 2]>(dst)
				});
			}
			{
				let src = [
					f64x4(0.0, 0.1, 0.2, 0.3),
					f64x4(1.0, 1.1, 1.2, 1.3),
					f64x4(2.0, 2.1, 2.2, 2.3),
					f64x4(3.0, 3.1, 3.2, 3.3),
				];
				let dst = unsafe { deinterleave_fallback::<f64, f64x4, [f64x4; 4]>(src) };
				assert_eq!(dst[1], simd.add_f64x4(dst[0], simd.splat_f64x4(0.1)));
				assert_eq!(dst[2], simd.add_f64x4(dst[0], simd.splat_f64x4(0.2)));
				assert_eq!(dst[3], simd.add_f64x4(dst[0], simd.splat_f64x4(0.3)));
				assert_eq!(src, unsafe {
					interleave_fallback::<f64, f64x4, [f64x4; 4]>(dst)
				});
			}
		}
	}
}
