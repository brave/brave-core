// Copyright 2020 Parity Technologies
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

// Code derived from original work by Andrew Poelstra <apoelstra@wpsoftware.net>

// Rust Bitcoin Library
// Written in 2014 by
//	   Andrew Poelstra <apoelstra@wpsoftware.net>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to
// the public domain worldwide. This software is distributed without
// any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//

//! Big unsigned integer types.
//!
//! Implementation of a various large-but-fixed sized unsigned integer types.
//! The functions here are designed to be fast. There are optional `x86_64`
//! implementations for even more speed, hidden behind the `x64_arithmetic`
//! feature flag.

use core::fmt;

/// A list of error categories encountered when parsing numbers.
#[derive(Debug, PartialEq, Eq, Clone, Copy, Hash)]
#[non_exhaustive]
pub enum FromStrRadixErrKind {
	/// A character in the input string is not valid for the given radix.
	InvalidCharacter,

	/// The input length is not valid for the given radix.
	InvalidLength,

	/// The given radix is not supported.
	UnsupportedRadix,
}

#[derive(Debug)]
enum FromStrRadixErrSrc {
	Hex(FromHexError),
	Dec(FromDecStrErr),
}

impl fmt::Display for FromStrRadixErrSrc {
	fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
		match self {
			FromStrRadixErrSrc::Dec(d) => write!(f, "{}", d),
			FromStrRadixErrSrc::Hex(h) => write!(f, "{}", h),
		}
	}
}

/// The error type for parsing numbers from strings.
#[derive(Debug)]
pub struct FromStrRadixErr {
	kind: FromStrRadixErrKind,
	source: Option<FromStrRadixErrSrc>,
}

impl FromStrRadixErr {
	#[doc(hidden)]
	pub fn unsupported() -> Self {
		Self { kind: FromStrRadixErrKind::UnsupportedRadix, source: None }
	}

	/// Returns the corresponding `FromStrRadixErrKind` for this error.
	pub fn kind(&self) -> FromStrRadixErrKind {
		self.kind
	}
}

impl fmt::Display for FromStrRadixErr {
	fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
		if let Some(ref src) = self.source {
			return write!(f, "{}", src);
		}

		match self.kind {
			FromStrRadixErrKind::UnsupportedRadix => write!(f, "the given radix is not supported"),
			FromStrRadixErrKind::InvalidCharacter => write!(f, "input contains an invalid character"),
			FromStrRadixErrKind::InvalidLength => write!(f, "length not supported for radix or type"),
		}
	}
}

#[cfg(feature = "std")]
impl std::error::Error for FromStrRadixErr {
	fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
		match self.source {
			Some(FromStrRadixErrSrc::Dec(ref d)) => Some(d),
			Some(FromStrRadixErrSrc::Hex(ref h)) => Some(h),
			None => None,
		}
	}
}

impl From<FromDecStrErr> for FromStrRadixErr {
	fn from(e: FromDecStrErr) -> Self {
		let kind = match e {
			FromDecStrErr::InvalidCharacter => FromStrRadixErrKind::InvalidCharacter,
			FromDecStrErr::InvalidLength => FromStrRadixErrKind::InvalidLength,
		};

		Self { kind, source: Some(FromStrRadixErrSrc::Dec(e)) }
	}
}

impl From<FromHexError> for FromStrRadixErr {
	fn from(e: FromHexError) -> Self {
		let kind = match e.inner {
			hex::FromHexError::InvalidHexCharacter { .. } => FromStrRadixErrKind::InvalidCharacter,
			hex::FromHexError::InvalidStringLength => FromStrRadixErrKind::InvalidLength,
			hex::FromHexError::OddLength => FromStrRadixErrKind::InvalidLength,
		};

		Self { kind, source: Some(FromStrRadixErrSrc::Hex(e)) }
	}
}

/// Conversion from decimal string error
#[derive(Debug, PartialEq)]
pub enum FromDecStrErr {
	/// Char not from range 0-9
	InvalidCharacter,
	/// Value does not fit into type
	InvalidLength,
}

impl fmt::Display for FromDecStrErr {
	fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
		write!(
			f,
			"{}",
			match self {
				FromDecStrErr::InvalidCharacter => "a character is not in the range 0-9",
				FromDecStrErr::InvalidLength => "the number is too large for the type",
			}
		)
	}
}

#[cfg(feature = "std")]
impl std::error::Error for FromDecStrErr {}

#[derive(Debug)]
pub struct FromHexError {
	inner: hex::FromHexError,
}

impl fmt::Display for FromHexError {
	fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
		write!(f, "{}", self.inner)
	}
}

#[cfg(feature = "std")]
impl std::error::Error for FromHexError {
	fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
		Some(&self.inner)
	}
}

#[doc(hidden)]
impl From<hex::FromHexError> for FromHexError {
	fn from(inner: hex::FromHexError) -> Self {
		Self { inner }
	}
}

#[macro_export]
#[doc(hidden)]
macro_rules! impl_map_from {
	($thing:ident, $from:ty, $to:ty) => {
		impl From<$from> for $thing {
			fn from(value: $from) -> $thing {
				From::from(value as $to)
			}
		}
	};
}

#[macro_export]
#[doc(hidden)]
macro_rules! impl_try_from_for_primitive {
	($from:ident, $to:ty) => {
		impl $crate::core_::convert::TryFrom<$from> for $to {
			type Error = &'static str;

			#[inline]
			fn try_from(u: $from) -> $crate::core_::result::Result<$to, &'static str> {
				let $from(arr) = u;
				if !u.fits_word() || arr[0] > <$to>::max_value() as u64 {
					Err(concat!("integer overflow when casting to ", stringify!($to)))
				} else {
					Ok(arr[0] as $to)
				}
			}
		}
	};
}

#[macro_export]
#[doc(hidden)]
macro_rules! uint_overflowing_binop {
	($name:ident, $n_words: tt, $self_expr: expr, $other: expr, $fn:expr) => {{
		use $crate::core_ as core;
		let $name(ref me) = $self_expr;
		let $name(ref you) = $other;

		let mut ret = [0u64; $n_words];
		let mut carry = 0u64;
		$crate::static_assertions::const_assert!(core::isize::MAX as usize / core::mem::size_of::<u64>() > $n_words);

		// `unroll!` is recursive, but doesn’t use `$crate::unroll`, so we need to ensure that it
		// is in scope unqualified.
		use $crate::unroll;
		unroll! {
			for i in 0..$n_words {
				use core::ptr;

				if carry != 0 {
					let (res1, overflow1) = ($fn)(me[i], you[i]);
					let (res2, overflow2) = ($fn)(res1, carry);

					ret[i] = res2;
					carry = (overflow1 as u8 + overflow2 as u8) as u64;
				} else {
					let (res, overflow) = ($fn)(me[i], you[i]);

					ret[i] = res;
					carry = overflow as u64;
				}
			}
		}

		($name(ret), carry > 0)
	}};
}

#[macro_export]
#[doc(hidden)]
macro_rules! uint_full_mul_reg {
	($name:ident, 8, $self_expr:expr, $other:expr) => {
		$crate::uint_full_mul_reg!($name, 8, $self_expr, $other, |a, b| a != 0 || b != 0);
	};
	($name:ident, $n_words:tt, $self_expr:expr, $other:expr) => {
		$crate::uint_full_mul_reg!($name, $n_words, $self_expr, $other, |_, _| true);
	};
	($name:ident, $n_words:tt, $self_expr:expr, $other:expr, $check:expr) => {{
		{
			#![allow(unused_assignments)]

			let $name(ref me) = $self_expr;
			let $name(ref you) = $other;
			let mut ret = [0u64; $n_words * 2];

			use $crate::unroll;
			unroll! {
				for i in 0..$n_words {
					let mut carry = 0u64;
					let b = you[i];

					unroll! {
						for j in 0..$n_words {
							if $check(me[j], carry) {
								let a = me[j];

								let (hi, low) = Self::split_u128(a as u128 * b as u128);

								let overflow = {
									let existing_low = &mut ret[i + j];
									let (low, o) = low.overflowing_add(*existing_low);
									*existing_low = low;
									o
								};

								carry = {
									let existing_hi = &mut ret[i + j + 1];
									let hi = hi + overflow as u64;
									let (hi, o0) = hi.overflowing_add(carry);
									let (hi, o1) = hi.overflowing_add(*existing_hi);
									*existing_hi = hi;

									(o0 | o1) as u64
								}
							}
						}
					}
				}
			}

			ret
		}
	}};
}

#[macro_export]
#[doc(hidden)]
macro_rules! uint_overflowing_mul {
	($name:ident, $n_words: tt, $self_expr: expr, $other: expr) => {{
		let ret: [u64; $n_words * 2] = $crate::uint_full_mul_reg!($name, $n_words, $self_expr, $other);

		// The safety of this is enforced by the compiler
		let ret: [[u64; $n_words]; 2] = unsafe { $crate::core_::mem::transmute(ret) };

		// The compiler WILL NOT inline this if you remove this annotation.
		#[inline(always)]
		fn any_nonzero(arr: &[u64; $n_words]) -> bool {
			use $crate::unroll;
			unroll! {
				for i in 0..$n_words {
					if arr[i] != 0 {
						return true;
					}
				}
			}

			false
		}

		($name(ret[0]), any_nonzero(&ret[1]))
	}};
}

#[macro_export]
#[doc(hidden)]
macro_rules! overflowing {
	($op: expr, $overflow: expr) => {{
		let (overflow_x, overflow_overflow) = $op;
		$overflow |= overflow_overflow;
		overflow_x
	}};
	($op: expr) => {{
		let (overflow_x, _overflow_overflow) = $op;
		overflow_x
	}};
}

#[macro_export]
#[doc(hidden)]
macro_rules! panic_on_overflow {
	($name: expr) => {
		if $name {
			panic!("arithmetic operation overflow")
		}
	};
}

#[macro_export]
#[doc(hidden)]
macro_rules! impl_mul_from {
	($name: ty, $other: ident) => {
		impl $crate::core_::ops::Mul<$other> for $name {
			type Output = $name;

			fn mul(self, other: $other) -> $name {
				let bignum: $name = other.into();
				let (result, overflow) = self.overflowing_mul(bignum);
				$crate::panic_on_overflow!(overflow);
				result
			}
		}

		impl<'a> $crate::core_::ops::Mul<&'a $other> for $name {
			type Output = $name;

			fn mul(self, other: &'a $other) -> $name {
				let bignum: $name = (*other).into();
				let (result, overflow) = self.overflowing_mul(bignum);
				$crate::panic_on_overflow!(overflow);
				result
			}
		}

		impl<'a> $crate::core_::ops::Mul<&'a $other> for &'a $name {
			type Output = $name;

			fn mul(self, other: &'a $other) -> $name {
				let bignum: $name = (*other).into();
				let (result, overflow) = self.overflowing_mul(bignum);
				$crate::panic_on_overflow!(overflow);
				result
			}
		}

		impl<'a> $crate::core_::ops::Mul<$other> for &'a $name {
			type Output = $name;

			fn mul(self, other: $other) -> $name {
				let bignum: $name = other.into();
				let (result, overflow) = self.overflowing_mul(bignum);
				$crate::panic_on_overflow!(overflow);
				result
			}
		}

		impl $crate::core_::ops::MulAssign<$other> for $name {
			fn mul_assign(&mut self, other: $other) {
				let result = *self * other;
				*self = result
			}
		}
	};
}

#[macro_export]
#[doc(hidden)]
macro_rules! impl_mul_for_primitive {
	($name: ty, $other: ident) => {
		impl $crate::core_::ops::Mul<$other> for $name {
			type Output = $name;

			fn mul(self, other: $other) -> $name {
				let (result, carry) = self.overflowing_mul_u64(other as u64);
				$crate::panic_on_overflow!(carry > 0);
				result
			}
		}

		impl<'a> $crate::core_::ops::Mul<&'a $other> for $name {
			type Output = $name;

			fn mul(self, other: &'a $other) -> $name {
				let (result, carry) = self.overflowing_mul_u64(*other as u64);
				$crate::panic_on_overflow!(carry > 0);
				result
			}
		}

		impl<'a> $crate::core_::ops::Mul<&'a $other> for &'a $name {
			type Output = $name;

			fn mul(self, other: &'a $other) -> $name {
				let (result, carry) = self.overflowing_mul_u64(*other as u64);
				$crate::panic_on_overflow!(carry > 0);
				result
			}
		}

		impl<'a> $crate::core_::ops::Mul<$other> for &'a $name {
			type Output = $name;

			fn mul(self, other: $other) -> $name {
				let (result, carry) = self.overflowing_mul_u64(other as u64);
				$crate::panic_on_overflow!(carry > 0);
				result
			}
		}

		impl $crate::core_::ops::MulAssign<$other> for $name {
			fn mul_assign(&mut self, other: $other) {
				let result = *self * (other as u64);
				*self = result
			}
		}
	};
}

#[macro_export]
macro_rules! construct_uint {
	( $(#[$attr:meta])* $visibility:vis struct $name:ident (1); ) => {
		$crate::construct_uint!{ @construct $(#[$attr])* $visibility struct $name (1); }
	};

	( $(#[$attr:meta])* $visibility:vis struct $name:ident ( $n_words:tt ); ) => {
			$crate::construct_uint! { @construct $(#[$attr])* $visibility struct $name ($n_words); }

			impl $crate::core_::convert::From<u128> for $name {
				fn from(value: u128) -> $name {
					let mut ret = [0; $n_words];
					ret[0] = value as u64;
					ret[1] = (value >> 64) as u64;
					$name(ret)
				}
			}

			impl $crate::core_::convert::From<i128> for $name {
				fn from(value: i128) -> $name {
					match value >= 0 {
						true => From::from(value as u128),
						false => { panic!("Unsigned integer can't be created from negative value"); }
					}
				}
			}

			impl $name {
				/// Low 2 words (u128)
				#[inline]
				pub const fn low_u128(&self) -> u128 {
					let &$name(ref arr) = self;
					((arr[1] as u128) << 64) + arr[0] as u128
				}

				/// Conversion to u128 with overflow checking
				///
				/// # Panics
				///
				/// Panics if the number is larger than 2^128.
				#[inline]
				pub fn as_u128(&self) -> u128 {
					let &$name(ref arr) = self;
					for i in 2..$n_words {
						if arr[i] != 0 {
							panic!("Integer overflow when casting to u128")
						}

					}
					self.low_u128()
				}
			}

			impl $crate::core_::convert::TryFrom<$name> for u128 {
				type Error = &'static str;

				#[inline]
				fn try_from(u: $name) -> $crate::core_::result::Result<u128, &'static str> {
					let $name(arr) = u;
					for i in 2..$n_words {
						if arr[i] != 0 {
							return Err("integer overflow when casting to u128");
						}
					}
					Ok(((arr[1] as u128) << 64) + arr[0] as u128)
				}
			}

			impl $crate::core_::convert::TryFrom<$name> for i128 {
				type Error = &'static str;

				#[inline]
				fn try_from(u: $name) -> $crate::core_::result::Result<i128, &'static str> {
					let err_str = "integer overflow when casting to i128";
					let i = u128::try_from(u).map_err(|_| err_str)?;
					if i > i128::max_value() as u128 {
						Err(err_str)
					} else {
						Ok(i as i128)
					}
				}
			}
	};
	( @construct $(#[$attr:meta])* $visibility:vis struct $name:ident ( $n_words:tt ); ) => {
		/// Little-endian large integer type
		#[repr(C)]
		$(#[$attr])*
		#[derive(Copy, Clone, Eq, PartialEq, Hash)]
		$visibility struct $name (pub [u64; $n_words]);

		/// Get a reference to the underlying little-endian words.
		impl AsRef<[u64]> for $name {
			#[inline]
			fn as_ref(&self) -> &[u64] {
				&self.0
			}
		}

		impl<'a> From<&'a $name> for $name {
			fn from(x: &'a $name) -> $name {
				*x
			}
		}

		impl $name {
			const WORD_BITS: usize = 64;
			/// Maximum value.
			pub const MAX: $name = $name([u64::max_value(); $n_words]);

			/// Converts a string slice in a given base to an integer. Only supports radixes of 10
			/// and 16.
			pub fn from_str_radix(txt: &str, radix: u32) -> Result<Self, $crate::FromStrRadixErr> {
				let parsed = match radix {
					10 => Self::from_dec_str(txt)?,
					16 => core::str::FromStr::from_str(txt)?,
					_ => return Err($crate::FromStrRadixErr::unsupported()),
				};

				Ok(parsed)
			}

			/// Convert from a decimal string.
			pub fn from_dec_str(value: &str) -> $crate::core_::result::Result<Self, $crate::FromDecStrErr> {
				let mut res = Self::default();
				for b in value.bytes().map(|b| b.wrapping_sub(b'0')) {
					if b > 9 {
						return Err($crate::FromDecStrErr::InvalidCharacter)
					}
					let (r, overflow) = res.overflowing_mul_u64(10);
					if overflow > 0 {
						return Err($crate::FromDecStrErr::InvalidLength);
					}
					let (r, overflow) = r.overflowing_add(b.into());
					if overflow {
						return Err($crate::FromDecStrErr::InvalidLength);
					}
					res = r;
				}
				Ok(res)
			}

			/// Conversion to u32
			#[inline]
			pub const fn low_u32(&self) -> u32 {
				let &$name(ref arr) = self;
				arr[0] as u32
			}

			/// Low word (u64)
			#[inline]
			pub const fn low_u64(&self) -> u64 {
				let &$name(ref arr) = self;
				arr[0]
			}

			/// Conversion to u32 with overflow checking
			///
			/// # Panics
			///
			/// Panics if the number is larger than 2^32.
			#[inline]
			pub fn as_u32(&self) -> u32 {
				let &$name(ref arr) = self;
				if !self.fits_word() ||  arr[0] > u32::max_value() as u64 {
					panic!("Integer overflow when casting to u32")
				}
				self.as_u64() as u32
			}

			/// Conversion to u64 with overflow checking
			///
			/// # Panics
			///
			/// Panics if the number is larger than u64::max_value().
			#[inline]
			pub fn as_u64(&self) -> u64 {
				let &$name(ref arr) = self;
				if !self.fits_word() {
					panic!("Integer overflow when casting to u64")
				}
				arr[0]
			}

			/// Conversion to usize with overflow checking
			///
			/// # Panics
			///
			/// Panics if the number is larger than usize::max_value().
			#[inline]
			pub fn as_usize(&self) -> usize {
				let &$name(ref arr) = self;
				if !self.fits_word() || arr[0] > usize::max_value() as u64 {
					panic!("Integer overflow when casting to usize")
				}
				arr[0] as usize
			}

			/// Whether this is zero.
			#[inline]
			pub const fn is_zero(&self) -> bool {
				let &$name(ref arr) = self;
				let mut i = 0;
				while i < $n_words { if arr[i] != 0 { return false; } else { i += 1; } }
				return true;
			}

			// Whether this fits u64.
			#[inline]
			fn fits_word(&self) -> bool {
				let &$name(ref arr) = self;
				for i in 1..$n_words { if arr[i] != 0 { return false; } }
				return true;
			}


			/// Return the least number of bits needed to represent the number
			#[inline]
			pub fn bits(&self) -> usize {
				let &$name(ref arr) = self;
				for i in 1..$n_words {
					if arr[$n_words - i] > 0 { return (0x40 * ($n_words - i + 1)) - arr[$n_words - i].leading_zeros() as usize; }
				}
				0x40 - arr[0].leading_zeros() as usize
			}

			/// Return if specific bit is set.
			///
			/// # Panics
			///
			/// Panics if `index` exceeds the bit width of the number.
			#[inline]
			pub const fn bit(&self, index: usize) -> bool {
				let &$name(ref arr) = self;
				arr[index / 64] & (1 << (index % 64)) != 0
			}

			/// Returns the number of leading zeros in the binary representation of self.
			pub fn leading_zeros(&self) -> u32 {
				let mut r = 0;
				for i in 0..$n_words {
					let w = self.0[$n_words - i - 1];
					if w == 0 {
						r += 64;
					} else {
						r += w.leading_zeros();
						break;
					}
				}
				r
			}

			/// Returns the number of trailing zeros in the binary representation of self.
			pub fn trailing_zeros(&self) -> u32 {
				let mut r = 0;
				for i in 0..$n_words {
					let w = self.0[i];
					if w == 0 {
						r += 64;
					} else {
						r += w.trailing_zeros();
						break;
					}
				}
				r
			}

			/// Return specific byte.
			///
			/// # Panics
			///
			/// Panics if `index` exceeds the byte width of the number.
			#[inline]
			pub const fn byte(&self, index: usize) -> u8 {
				let &$name(ref arr) = self;
				(arr[index / 8] >> (((index % 8)) * 8)) as u8
			}

			/// Write to the slice in big-endian format.
			#[inline]
			pub fn to_big_endian(&self, bytes: &mut [u8]) {
				use $crate::byteorder::{ByteOrder, BigEndian};
				debug_assert!($n_words * 8 == bytes.len());
				for i in 0..$n_words {
					BigEndian::write_u64(&mut bytes[8 * i..], self.0[$n_words - i - 1]);
				}
			}

			/// Write to the slice in little-endian format.
			#[inline]
			pub fn to_little_endian(&self, bytes: &mut [u8]) {
				use $crate::byteorder::{ByteOrder, LittleEndian};
				debug_assert!($n_words * 8 == bytes.len());
				for i in 0..$n_words {
					LittleEndian::write_u64(&mut bytes[8 * i..], self.0[i]);
				}
			}


			/// Create `10**n` as this type.
			///
			/// # Panics
			///
			/// Panics if the result overflows the type.
			#[inline]
			pub fn exp10(n: usize) -> Self {
				match n {
					0 => Self::from(1u64),
					_ => Self::exp10(n - 1) * 10u32
				}
			}

			/// Zero (additive identity) of this type.
			#[inline]
			pub const fn zero() -> Self {
				Self([0; $n_words])
			}

			/// One (multiplicative identity) of this type.
			#[inline]
			pub const fn one() -> Self {
				let mut words = [0; $n_words];
				words[0] = 1u64;
				Self(words)
			}

			/// The maximum value which can be inhabited by this type.
			#[inline]
			pub const fn max_value() -> Self {
				Self::MAX
			}

			fn full_shl(self, shift: u32) -> [u64; $n_words + 1] {
				debug_assert!(shift < Self::WORD_BITS as u32);
				let mut u = [0u64; $n_words + 1];
				let u_lo = self.0[0] << shift;
				let u_hi = self >> (Self::WORD_BITS as u32 - shift);
				u[0] = u_lo;
				u[1..].copy_from_slice(&u_hi.0[..]);
				u
			}

			fn full_shr(u: [u64; $n_words + 1], shift: u32) -> Self {
				debug_assert!(shift < Self::WORD_BITS as u32);
				let mut res = Self::zero();
				for i in 0..$n_words {
					res.0[i] = u[i] >> shift;
				}
				// carry
				if shift > 0 {
					for i in 1..=$n_words {
						res.0[i - 1] |= u[i] << (Self::WORD_BITS as u32 - shift);
					}
				}
				res
			}

			fn full_mul_u64(self, by: u64) -> [u64; $n_words + 1] {
				let (prod, carry) = self.overflowing_mul_u64(by);
				let mut res = [0u64; $n_words + 1];
				res[..$n_words].copy_from_slice(&prod.0[..]);
				res[$n_words] = carry;
				res
			}

			fn div_mod_small(mut self, other: u64) -> (Self, Self) {
				let mut rem = 0u64;
				self.0.iter_mut().rev().for_each(|d| {
					let (q, r) = Self::div_mod_word(rem, *d, other);
					*d = q;
					rem = r;
				});
				(self, rem.into())
			}

			// See Knuth, TAOCP, Volume 2, section 4.3.1, Algorithm D.
			fn div_mod_knuth(self, mut v: Self, n: usize, m: usize) -> (Self, Self) {
				debug_assert!(self.bits() >= v.bits() && !v.fits_word());
				debug_assert!(n + m <= $n_words);
				// D1.
				// Make sure 64th bit in v's highest word is set.
				// If we shift both self and v, it won't affect the quotient
				// and the remainder will only need to be shifted back.
				let shift = v.0[n - 1].leading_zeros();
				v <<= shift;
				// u will store the remainder (shifted)
				let mut u = self.full_shl(shift);

				// quotient
				let mut q = Self::zero();
				let v_n_1 = v.0[n - 1];
				let v_n_2 = v.0[n - 2];

				// D2. D7.
				// iterate from m downto 0
				for j in (0..=m).rev() {
					let u_jn = u[j + n];

					// D3.
					// q_hat is our guess for the j-th quotient digit
					// q_hat = min(b - 1, (u_{j+n} * b + u_{j+n-1}) / v_{n-1})
					// b = 1 << WORD_BITS
					// Theorem B: q_hat >= q_j >= q_hat - 2
					let mut q_hat = if u_jn < v_n_1 {
						let (mut q_hat, mut r_hat) = Self::div_mod_word(u_jn, u[j + n - 1], v_n_1);
						// this loop takes at most 2 iterations
						loop {
							// check if q_hat * v_{n-2} > b * r_hat + u_{j+n-2}
							let (hi, lo) = Self::split_u128(u128::from(q_hat) * u128::from(v_n_2));
							if (hi, lo) <= (r_hat, u[j + n - 2]) {
								break;
							}
							// then iterate till it doesn't hold
							q_hat -= 1;
							let (new_r_hat, overflow) = r_hat.overflowing_add(v_n_1);
							r_hat = new_r_hat;
							// if r_hat overflowed, we're done
							if overflow {
								break;
							}
						}
						q_hat
					} else {
						// here q_hat >= q_j >= q_hat - 1
						u64::max_value()
					};

					// ex. 20:
					// since q_hat * v_{n-2} <= b * r_hat + u_{j+n-2},
					// either q_hat == q_j, or q_hat == q_j + 1

					// D4.
					// let's assume optimistically q_hat == q_j
					// subtract (q_hat * v) from u[j..]
					let q_hat_v = v.full_mul_u64(q_hat);
					// u[j..] -= q_hat_v;
					let c = Self::sub_slice(&mut u[j..], &q_hat_v[..n + 1]);

					// D6.
					// actually, q_hat == q_j + 1 and u[j..] has overflowed
					// highly unlikely ~ (1 / 2^63)
					if c {
						q_hat -= 1;
						// add v to u[j..]
						let c = Self::add_slice(&mut u[j..], &v.0[..n]);
						u[j + n] = u[j + n].wrapping_add(u64::from(c));
					}

					// D5.
					q.0[j] = q_hat;
				}

				// D8.
				let remainder = Self::full_shr(u, shift);

				(q, remainder)
			}

			// Returns the least number of words needed to represent the nonzero number
			fn words(bits: usize) -> usize {
				debug_assert!(bits > 0);
				1 + (bits - 1) / Self::WORD_BITS
			}

			/// Returns a pair `(self / other, self % other)`.
			///
			/// # Panics
			///
			/// Panics if `other` is zero.
			pub fn div_mod(mut self, mut other: Self) -> (Self, Self) {
				use $crate::core_::cmp::Ordering;

				let my_bits = self.bits();
				let your_bits = other.bits();

				assert!(your_bits != 0, "division by zero");

				// Early return in case we are dividing by a larger number than us
				if my_bits < your_bits {
					return (Self::zero(), self);
				}

				if your_bits <= Self::WORD_BITS {
					return self.div_mod_small(other.low_u64());
				}

				let (n, m) = {
					let my_words = Self::words(my_bits);
					let your_words = Self::words(your_bits);
					(your_words, my_words - your_words)
				};

				self.div_mod_knuth(other, n, m)
			}

			/// Compute the highest `n` such that `n * n <= self`.
			pub fn integer_sqrt(&self) -> Self {
				let one = Self::one();
				if self <= &one {
					return *self;
				}

				// the implementation is based on:
				// https://en.wikipedia.org/wiki/Integer_square_root#Using_only_integer_division

				// Set the initial guess to something higher than √self.
				let shift: u32 = (self.bits() as u32 + 1) / 2;
				let mut x_prev = one << shift;
				loop {
					let x = (x_prev + self / x_prev) >> 1;
					if x >= x_prev {
						return x_prev;
					}
					x_prev = x;
				}
			}

			/// Fast exponentiation by squaring
			/// https://en.wikipedia.org/wiki/Exponentiation_by_squaring
			///
			/// # Panics
			///
			/// Panics if the result overflows the type.
			pub fn pow(self, expon: Self) -> Self {
				if expon.is_zero() {
					return Self::one()
				}
				let is_even = |x : &Self| x.low_u64() & 1 == 0;

				let u_one = Self::one();
				let mut y = u_one;
				let mut n = expon;
				let mut x = self;
				while n > u_one {
					if is_even(&n) {
						x = x * x;
						n = n >> 1usize;
					} else {
						y = x * y;
						x = x * x;
						// to reduce odd number by 1 we should just clear the last bit
						n.0[$n_words-1] = n.0[$n_words-1] & ((!0u64)>>1);
						n = n >> 1usize;
					}
				}
				x * y
			}

			/// Fast exponentiation by squaring. Returns result and overflow flag.
			pub fn overflowing_pow(self, expon: Self) -> (Self, bool) {
				if expon.is_zero() { return (Self::one(), false) }

				let is_even = |x : &Self| x.low_u64() & 1 == 0;

				let u_one = Self::one();
				let mut y = u_one;
				let mut n = expon;
				let mut x = self;
				let mut overflow = false;

				while n > u_one {
					if is_even(&n) {
						x = $crate::overflowing!(x.overflowing_mul(x), overflow);
						n = n >> 1usize;
					} else {
						y = $crate::overflowing!(x.overflowing_mul(y), overflow);
						x = $crate::overflowing!(x.overflowing_mul(x), overflow);
						n = (n - u_one) >> 1usize;
					}
				}
				let res = $crate::overflowing!(x.overflowing_mul(y), overflow);
				(res, overflow)
			}

			/// Checked exponentiation. Returns `None` if overflow occurred.
			pub fn checked_pow(self, expon: $name) -> Option<$name> {
				match self.overflowing_pow(expon) {
					(_, true) => None,
					(val, _) => Some(val),
				}
			}

			/// Addition which overflows and returns a flag if it does.
			#[inline(always)]
			pub fn overflowing_add(self, other: $name) -> ($name, bool) {
				$crate::uint_overflowing_binop!(
					$name,
					$n_words,
					self,
					other,
					u64::overflowing_add
				)
			}

			/// Addition which saturates at the maximum value (Self::MAX).
			pub fn saturating_add(self, other: $name) -> $name {
				match self.overflowing_add(other) {
					(_, true) => $name::MAX,
					(val, false) => val,
				}
			}

			/// Checked addition. Returns `None` if overflow occurred.
			pub fn checked_add(self, other: $name) -> Option<$name> {
				match self.overflowing_add(other) {
					(_, true) => None,
					(val, _) => Some(val),
				}
			}

			/// Subtraction which underflows and returns a flag if it does.
			#[inline(always)]
			pub fn overflowing_sub(self, other: $name) -> ($name, bool) {
				$crate::uint_overflowing_binop!(
					$name,
					$n_words,
					self,
					other,
					u64::overflowing_sub
				)
			}

			/// Subtraction which saturates at zero.
			pub fn saturating_sub(self, other: $name) -> $name {
				match self.overflowing_sub(other) {
					(_, true) => $name::zero(),
					(val, false) => val,
				}
			}

			/// Checked subtraction. Returns `None` if overflow occurred.
			pub fn checked_sub(self, other: $name) -> Option<$name> {
				match self.overflowing_sub(other) {
					(_, true) => None,
					(val, _) => Some(val),
				}
			}

			/// Computes the absolute difference between self and other.
			pub fn abs_diff(self, other: $name) -> $name {
				if self > other {
					self.overflowing_sub(other).0
				} else {
					other.overflowing_sub(self).0
				}
			}

			/// Multiply with overflow, returning a flag if it does.
			#[inline(always)]
			pub fn overflowing_mul(self, other: $name) -> ($name, bool) {
				$crate::uint_overflowing_mul!($name, $n_words, self, other)
			}

			/// Multiplication which saturates at the maximum value..
			pub fn saturating_mul(self, other: $name) -> $name {
				match self.overflowing_mul(other) {
					(_, true) => $name::MAX,
					(val, false) => val,
				}
			}

			/// Checked multiplication. Returns `None` if overflow occurred.
			pub fn checked_mul(self, other: $name) -> Option<$name> {
				match self.overflowing_mul(other) {
					(_, true) => None,
					(val, _) => Some(val),
				}
			}

			/// Checked division. Returns `None` if `other == 0`.
			pub fn checked_div(self, other: $name) -> Option<$name> {
				if other.is_zero() {
					None
				} else {
					Some(self / other)
				}
			}

			/// Checked modulus. Returns `None` if `other == 0`.
			pub fn checked_rem(self, other: $name) -> Option<$name> {
				if other.is_zero() {
					None
				} else {
					Some(self % other)
				}
			}

			/// Negation with overflow.
			pub fn overflowing_neg(self) -> ($name, bool) {
				if self.is_zero() {
					(self, false)
				} else {
					(!self + 1, true)
				}
			}

			/// Checked negation. Returns `None` unless `self == 0`.
			pub fn checked_neg(self) -> Option<$name> {
				match self.overflowing_neg() {
					(_, true) => None,
					(zero, false) => Some(zero),
				}
			}

			#[inline(always)]
			fn div_mod_word(hi: u64, lo: u64, y: u64) -> (u64, u64) {
				debug_assert!(hi < y);
				let x = (u128::from(hi) << 64) + u128::from(lo);
				let y = u128::from(y);
				((x / y) as u64, (x % y) as u64)
			}

			#[inline(always)]
			fn add_slice(a: &mut [u64], b: &[u64]) -> bool {
				Self::binop_slice(a, b, u64::overflowing_add)
			}

			#[inline(always)]
			fn sub_slice(a: &mut [u64], b: &[u64]) -> bool {
				Self::binop_slice(a, b, u64::overflowing_sub)
			}

			#[inline(always)]
			fn binop_slice(a: &mut [u64], b: &[u64], binop: impl Fn(u64, u64) -> (u64, bool) + Copy) -> bool {
				let mut c = false;
				a.iter_mut().zip(b.iter()).for_each(|(x, y)| {
					let (res, carry) = Self::binop_carry(*x, *y, c, binop);
					*x = res;
					c = carry;
				});
				c
			}

			#[inline(always)]
			fn binop_carry(a: u64, b: u64, c: bool, binop: impl Fn(u64, u64) -> (u64, bool)) -> (u64, bool) {
				let (res1, overflow1) = b.overflowing_add(u64::from(c));
				let (res2, overflow2) = binop(a, res1);
				(res2, overflow1 || overflow2)
			}

			#[inline(always)]
			const fn mul_u64(a: u64, b: u64, carry: u64) -> (u64, u64) {
				let (hi, lo) = Self::split_u128(a as u128 * b as u128 + carry as u128);
				(lo, hi)
			}

			#[inline(always)]
			const fn split(a: u64) -> (u64, u64) {
				(a >> 32, a & 0xFFFF_FFFF)
			}

			#[inline(always)]
			const fn split_u128(a: u128) -> (u64, u64) {
				((a >> 64) as _, (a & 0xFFFFFFFFFFFFFFFF) as _)
			}


			/// Overflowing multiplication by u64.
			/// Returns the result and carry.
			fn overflowing_mul_u64(mut self, other: u64) -> (Self, u64) {
				let mut carry = 0u64;

				for d in self.0.iter_mut() {
					let (res, c) = Self::mul_u64(*d, other, carry);
					*d = res;
					carry = c;
				}

				(self, carry)
			}

			/// Converts from big endian representation bytes in memory.
			pub fn from_big_endian(slice: &[u8]) -> Self {
				use $crate::byteorder::{ByteOrder, BigEndian};
				assert!($n_words * 8 >= slice.len());

				let mut padded = [0u8; $n_words * 8];
				padded[$n_words * 8 - slice.len() .. $n_words * 8].copy_from_slice(&slice);

				let mut ret = [0; $n_words];
				for i in 0..$n_words {
					ret[$n_words - i - 1] = BigEndian::read_u64(&padded[8 * i..]);
				}

				$name(ret)
			}

			/// Converts from little endian representation bytes in memory.
			pub fn from_little_endian(slice: &[u8]) -> Self {
				use $crate::byteorder::{ByteOrder, LittleEndian};
				assert!($n_words * 8 >= slice.len());

				let mut padded = [0u8; $n_words * 8];
				padded[0..slice.len()].copy_from_slice(&slice);

				let mut ret = [0; $n_words];
				for i in 0..$n_words {
					ret[i] = LittleEndian::read_u64(&padded[8 * i..]);
				}

				$name(ret)
			}

			fn fmt_hex(&self, f: &mut $crate::core_::fmt::Formatter, is_lower: bool) -> $crate::core_::fmt::Result {
				let &$name(ref data) = self;
				// special case.
				if self.is_zero() {
					return f.pad_integral(true, "0x", "0");
				}

				let mut latch = false;
				let mut buf = [0_u8; $n_words * 16];
				let mut i = 0;
				for ch in data.iter().rev() {
					for x in 0..16 {
						// nibble < 16
						let nibble = (ch & (15u64 << ((15 - x) * 4) as u64)) >> (((15 - x) * 4) as u64);
						if !latch {
							latch = nibble != 0;
						}

						if latch {
							// nibble is `'0'..'9' 'a'..'f' 'A'..'F'` because nibble < 16
							let nibble = match nibble {
								0..=9 => nibble as u8 + b'0',
								_ if is_lower => nibble as u8 - 10 + b'a',
								_ => nibble as u8 - 10 + b'A',
							};
							buf[i] = nibble;
							i += 1;
						}
					}
				}

				// sequence of `'0'..'9' 'a'..'f' 'A'..'F'` chars is guaranteed to be a valid UTF8 string
				let s = unsafe {
					$crate::core_::str::from_utf8_unchecked(&buf[0..i])
				};
				f.pad_integral(true, "0x", s)
			}
		}

		impl $crate::core_::convert::From<$name> for [u8; $n_words * 8] {
			fn from(number: $name) -> Self {
				let mut arr = [0u8; $n_words * 8];
				number.to_big_endian(&mut arr);
				arr
			}
		}

		impl $crate::core_::convert::From<[u8; $n_words * 8]> for $name {
			fn from(bytes: [u8; $n_words * 8]) -> Self {
				Self::from(&bytes)
			}
		}

		impl<'a> $crate::core_::convert::From<&'a [u8; $n_words * 8]> for $name {
			fn from(bytes: &[u8; $n_words * 8]) -> Self {
				Self::from(&bytes[..])
			}
		}

		impl $crate::core_::default::Default for $name {
			fn default() -> Self {
				$name::zero()
			}
		}

		impl $crate::core_::convert::From<u64> for $name {
			fn from(value: u64) -> $name {
				let mut ret = [0; $n_words];
				ret[0] = value;
				$name(ret)
			}
		}

		$crate::impl_map_from!($name, u8, u64);
		$crate::impl_map_from!($name, u16, u64);
		$crate::impl_map_from!($name, u32, u64);
		$crate::impl_map_from!($name, usize, u64);

		impl $crate::core_::convert::From<i64> for $name {
			fn from(value: i64) -> $name {
				match value >= 0 {
					true => From::from(value as u64),
					false => { panic!("Unsigned integer can't be created from negative value"); }
				}
			}
		}

		$crate::impl_map_from!($name, i8, i64);
		$crate::impl_map_from!($name, i16, i64);
		$crate::impl_map_from!($name, i32, i64);
		$crate::impl_map_from!($name, isize, i64);

		// Converts from big endian representation.
		impl<'a> $crate::core_::convert::From<&'a [u8]> for $name {
			fn from(bytes: &[u8]) -> $name {
				Self::from_big_endian(bytes)
			}
		}

		$crate::impl_try_from_for_primitive!($name, u8);
		$crate::impl_try_from_for_primitive!($name, u16);
		$crate::impl_try_from_for_primitive!($name, u32);
		$crate::impl_try_from_for_primitive!($name, usize);
		$crate::impl_try_from_for_primitive!($name, u64);
		$crate::impl_try_from_for_primitive!($name, i8);
		$crate::impl_try_from_for_primitive!($name, i16);
		$crate::impl_try_from_for_primitive!($name, i32);
		$crate::impl_try_from_for_primitive!($name, isize);
		$crate::impl_try_from_for_primitive!($name, i64);

		impl<T> $crate::core_::ops::Add<T> for $name where T: Into<$name> {
			type Output = $name;

			fn add(self, other: T) -> $name {
				let (result, overflow) = self.overflowing_add(other.into());
				$crate::panic_on_overflow!(overflow);
				result
			}
		}

		impl<'a, T> $crate::core_::ops::Add<T> for &'a $name where T: Into<$name> {
			type Output = $name;

			fn add(self, other: T) -> $name {
				*self + other
			}
		}

		impl $crate::core_::ops::AddAssign<$name> for $name {
			fn add_assign(&mut self, other: $name) {
				let (result, overflow) = self.overflowing_add(other);
				$crate::panic_on_overflow!(overflow);
				*self = result
			}
		}

		impl<T> $crate::core_::ops::Sub<T> for $name where T: Into<$name> {
			type Output = $name;

			#[inline]
			fn sub(self, other: T) -> $name {
				let (result, overflow) = self.overflowing_sub(other.into());
				$crate::panic_on_overflow!(overflow);
				result
			}
		}

		impl<'a, T> $crate::core_::ops::Sub<T> for &'a $name where T: Into<$name> {
			type Output = $name;

			fn sub(self, other: T) -> $name {
				*self - other
			}
		}

		impl $crate::core_::ops::SubAssign<$name> for $name {
			fn sub_assign(&mut self, other: $name) {
				let (result, overflow) = self.overflowing_sub(other);
				$crate::panic_on_overflow!(overflow);
				*self = result
			}
		}

		// all other impls
		$crate::impl_mul_from!($name, $name);
		$crate::impl_mul_for_primitive!($name, u8);
		$crate::impl_mul_for_primitive!($name, u16);
		$crate::impl_mul_for_primitive!($name, u32);
		$crate::impl_mul_for_primitive!($name, u64);
		$crate::impl_mul_for_primitive!($name, usize);
		$crate::impl_mul_for_primitive!($name, i8);
		$crate::impl_mul_for_primitive!($name, i16);
		$crate::impl_mul_for_primitive!($name, i32);
		$crate::impl_mul_for_primitive!($name, i64);
		$crate::impl_mul_for_primitive!($name, isize);

		impl<T> $crate::core_::ops::Div<T> for $name where T: Into<$name> {
			type Output = $name;

			fn div(self, other: T) -> $name {
				let other: Self = other.into();
				self.div_mod(other).0
			}
		}

		impl<'a, T> $crate::core_::ops::Div<T> for &'a $name where T: Into<$name> {
			type Output = $name;

			fn div(self, other: T) -> $name {
				*self / other
			}
		}

		impl<T> $crate::core_::ops::DivAssign<T> for $name where T: Into<$name> {
			fn div_assign(&mut self, other: T) {
				*self = *self / other.into();
			}
		}

		impl<T> $crate::core_::ops::Rem<T> for $name where T: Into<$name> + Copy {
			type Output = $name;

			fn rem(self, other: T) -> $name {
				let mut sub_copy = self;
				sub_copy %= other;
				sub_copy
			}
		}

		impl<'a, T> $crate::core_::ops::Rem<T> for &'a $name where T: Into<$name>  + Copy {
			type Output = $name;

			fn rem(self, other: T) -> $name {
				*self % other
			}
		}

		impl<T> $crate::core_::ops::RemAssign<T> for $name where T: Into<$name> + Copy {
			fn rem_assign(&mut self, other: T) {
				let other: Self = other.into();
				let rem = self.div_mod(other).1;
				*self = rem;
			}
		}

		impl $crate::core_::ops::BitAnd<$name> for $name {
			type Output = $name;

			#[inline]
			fn bitand(self, other: $name) -> $name {
				let $name(ref arr1) = self;
				let $name(ref arr2) = other;
				let mut ret = [0u64; $n_words];
				for i in 0..$n_words {
					ret[i] = arr1[i] & arr2[i];
				}
				$name(ret)
			}
		}

		impl $crate::core_::ops::BitAndAssign<$name> for $name {
			fn bitand_assign(&mut self, rhs: $name) {
				*self = *self & rhs;
			}
		}

		impl $crate::core_::ops::BitXor<$name> for $name {
			type Output = $name;

			#[inline]
			fn bitxor(self, other: $name) -> $name {
				let $name(ref arr1) = self;
				let $name(ref arr2) = other;
				let mut ret = [0u64; $n_words];
				for i in 0..$n_words {
					ret[i] = arr1[i] ^ arr2[i];
				}
				$name(ret)
			}
		}

		impl $crate::core_::ops::BitXorAssign<$name> for $name {
			fn bitxor_assign(&mut self, rhs: $name) {
				*self = *self ^ rhs;
			}
		}

		impl $crate::core_::ops::BitOr<$name> for $name {
			type Output = $name;

			#[inline]
			fn bitor(self, other: $name) -> $name {
				let $name(ref arr1) = self;
				let $name(ref arr2) = other;
				let mut ret = [0u64; $n_words];
				for i in 0..$n_words {
					ret[i] = arr1[i] | arr2[i];
				}
				$name(ret)
			}
		}

		impl $crate::core_::ops::BitOrAssign<$name> for $name {
			fn bitor_assign(&mut self, rhs: $name) {
				*self = *self | rhs;
			}
		}

		impl $crate::core_::ops::Not for $name {
			type Output = $name;

			#[inline]
			fn not(self) -> $name {
				let $name(ref arr) = self;
				let mut ret = [0u64; $n_words];
				for i in 0..$n_words {
					ret[i] = !arr[i];
				}
				$name(ret)
			}
		}

		impl<T> $crate::core_::ops::Shl<T> for $name where T: Into<$name> {
			type Output = $name;

			fn shl(self, shift: T) -> $name {
				let shift = shift.into().as_usize();
				let $name(ref original) = self;
				let mut ret = [0u64; $n_words];
				let word_shift = shift / 64;
				let bit_shift = shift % 64;

				// shift
				for i in word_shift..$n_words {
					ret[i] = original[i - word_shift] << bit_shift;
				}
				// carry
				if bit_shift > 0 {
					for i in word_shift+1..$n_words {
						ret[i] += original[i - 1 - word_shift] >> (64 - bit_shift);
					}
				}
				$name(ret)
			}
		}

		impl<'a, T> $crate::core_::ops::Shl<T> for &'a $name where T: Into<$name> {
			type Output = $name;
			fn shl(self, shift: T) -> $name {
				*self << shift
			}
		}

		impl<T> $crate::core_::ops::ShlAssign<T> for $name where T: Into<$name> {
			fn shl_assign(&mut self, shift: T) {
				*self = *self << shift;
			}
		}

		impl<T> $crate::core_::ops::Shr<T> for $name where T: Into<$name> {
			type Output = $name;

			fn shr(self, shift: T) -> $name {
				let shift = shift.into().as_usize();
				let $name(ref original) = self;
				let mut ret = [0u64; $n_words];
				let word_shift = shift / 64;
				let bit_shift = shift % 64;

				// shift
				for i in word_shift..$n_words {
					ret[i - word_shift] = original[i] >> bit_shift;
				}

				// Carry
				if bit_shift > 0 {
					for i in word_shift+1..$n_words {
						ret[i - word_shift - 1] += original[i] << (64 - bit_shift);
					}
				}

				$name(ret)
			}
		}

		impl<'a, T> $crate::core_::ops::Shr<T> for &'a $name where T: Into<$name> {
			type Output = $name;
			fn shr(self, shift: T) -> $name {
				*self >> shift
			}
		}

		impl<T> $crate::core_::ops::ShrAssign<T> for $name where T: Into<$name> {
			fn shr_assign(&mut self, shift: T) {
				*self = *self >> shift;
			}
		}

		impl $crate::core_::cmp::Ord for $name {
			fn cmp(&self, other: &$name) -> $crate::core_::cmp::Ordering {
				self.as_ref().iter().rev().cmp(other.as_ref().iter().rev())
			}
		}

		impl $crate::core_::cmp::PartialOrd for $name {
			fn partial_cmp(&self, other: &$name) -> Option<$crate::core_::cmp::Ordering> {
				Some(self.cmp(other))
			}
		}

		impl $crate::core_::fmt::Debug for $name {
			fn fmt(&self, f: &mut $crate::core_::fmt::Formatter) -> $crate::core_::fmt::Result {
				$crate::core_::fmt::Display::fmt(self, f)
			}
		}

		impl $crate::core_::fmt::Display for $name {
			fn fmt(&self, f: &mut $crate::core_::fmt::Formatter) -> $crate::core_::fmt::Result {
				if self.is_zero() {
					return $crate::core_::write!(f, "0");
				}

				let mut buf = [0_u8; $n_words*20];
				let mut i = buf.len() - 1;
				let mut current = *self;
				let ten = $name::from(10);

				loop {
					let digit = (current % ten).low_u64() as u8;
					buf[i] = digit + b'0';
					current = current / ten;
					if current.is_zero() {
						break;
					}
					i -= 1;
				}

				// sequence of `'0'..'9'` chars is guaranteed to be a valid UTF8 string
				let s = unsafe {
					$crate::core_::str::from_utf8_unchecked(&buf[i..])
				};
				f.pad_integral(true, "", s)
			}
		}

		impl $crate::core_::fmt::LowerHex for $name {
			fn fmt(&self, f: &mut $crate::core_::fmt::Formatter) -> $crate::core_::fmt::Result {
				self.fmt_hex(f, true)
			}
		}

		impl $crate::core_::fmt::UpperHex for $name {
			fn fmt(&self, f: &mut $crate::core_::fmt::Formatter) -> $crate::core_::fmt::Result {
				self.fmt_hex(f, false)
			}
		}

		impl $crate::core_::str::FromStr for $name {
			type Err = $crate::FromHexError;

			fn from_str(value: &str) -> $crate::core_::result::Result<$name, Self::Err> {
				let value = value.strip_prefix("0x").unwrap_or(value);
				const BYTES_LEN: usize = $n_words * 8;
				const MAX_ENCODED_LEN: usize = BYTES_LEN * 2;

				let mut bytes = [0_u8; BYTES_LEN];

				let encoded = value.as_bytes();

				if encoded.len() > MAX_ENCODED_LEN {
					return Err($crate::hex::FromHexError::InvalidStringLength.into());
				}

				if encoded.len() % 2 == 0 {
					let out = &mut bytes[BYTES_LEN - encoded.len() / 2..];

					$crate::hex::decode_to_slice(encoded, out).map_err(Self::Err::from)?;
				} else {
					// Prepend '0' by overlaying our value on a scratch buffer filled with '0' characters.
					let mut s = [b'0'; MAX_ENCODED_LEN];
					s[MAX_ENCODED_LEN - encoded.len()..].copy_from_slice(encoded);
					let encoded = &s[MAX_ENCODED_LEN - encoded.len() - 1..];

					let out = &mut bytes[BYTES_LEN - encoded.len() / 2..];

					$crate::hex::decode_to_slice(encoded, out).map_err(Self::Err::from)?;
				}

				let bytes_ref: &[u8] = &bytes;
				Ok(From::from(bytes_ref))
			}
		}

		impl $crate::core_::convert::From<&'static str> for $name {
			fn from(s: &'static str) -> Self {
				s.parse().unwrap()
			}
		}

		// `$n_words * 8` because macro expects bytes and
		// uints use 64 bit (8 byte) words
		$crate::impl_quickcheck_arbitrary_for_uint!($name, ($n_words * 8));
		$crate::impl_arbitrary_for_uint!($name, ($n_words * 8));
	}
}

#[cfg(feature = "quickcheck")]
#[macro_export]
#[doc(hidden)]
macro_rules! impl_quickcheck_arbitrary_for_uint {
	($uint: ty, $n_bytes: tt) => {
		impl $crate::quickcheck::Arbitrary for $uint {
			fn arbitrary(g: &mut $crate::quickcheck::Gen) -> Self {
				let p = usize::arbitrary(g) % 100;
				// make it more likely to generate smaller numbers that
				// don't use up the full $n_bytes
				let range =
					// 10% chance to generate number that uses up to $n_bytes
					if p < 10 {
						$n_bytes
					// 10% chance to generate number that uses up to $n_bytes / 2
					} else if p < 20 {
						$n_bytes / 2
					// 80% chance to generate number that uses up to $n_bytes / 5
					} else {
						$n_bytes / 5
					};

				let range = $crate::core_::cmp::max(range, 1);
				let size: usize = usize::arbitrary(g) % range;

				let res: [u8; $n_bytes] = $crate::core_::array::from_fn(|i| {
					if i > size {
						0
					} else {
						u8::arbitrary(g)
					}
				});

				res.as_ref().into()
			}
		}
	};
}

#[cfg(not(feature = "quickcheck"))]
#[macro_export]
#[doc(hidden)]
macro_rules! impl_quickcheck_arbitrary_for_uint {
	($uint: ty, $n_bytes: tt) => {};
}

#[cfg(feature = "arbitrary")]
#[macro_export]
#[doc(hidden)]
macro_rules! impl_arbitrary_for_uint {
	($uint: ty, $n_bytes: tt) => {
		impl $crate::arbitrary::Arbitrary<'_> for $uint {
			fn arbitrary(u: &mut $crate::arbitrary::Unstructured<'_>) -> $crate::arbitrary::Result<Self> {
				let mut res = [0u8; $n_bytes];
				u.fill_buffer(&mut res)?;
				Ok(Self::from(res))
			}
		}
	};
}

#[cfg(not(feature = "arbitrary"))]
#[macro_export]
#[doc(hidden)]
macro_rules! impl_arbitrary_for_uint {
	($uint: ty, $n_bytes: tt) => {};
}
