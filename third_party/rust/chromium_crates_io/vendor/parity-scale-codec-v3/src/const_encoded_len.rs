// Copyright (C) 2023 Parity Technologies (UK) Ltd.
// SPDX-License-Identifier: Apache-2.0

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// 	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Contains the [`ConstEncodedLen`] trait.

use crate::{alloc::boxed::Box, MaxEncodedLen};
use core::{
	marker::PhantomData,
	num::*,
	ops::{Range, RangeInclusive},
	time::Duration,
};
use impl_trait_for_tuples::impl_for_tuples;

/// Types that have a constant encoded length. This implies [`MaxEncodedLen`].
///
/// No derive macros is provided; instead use an empty implementation like for a marker trait.
pub trait ConstEncodedLen: MaxEncodedLen {}

#[impl_for_tuples(18)]
impl ConstEncodedLen for Tuple {}

impl<T: ConstEncodedLen, const N: usize> ConstEncodedLen for [T; N] {}

/// Mark `T` or `T<S>` as `CEL`.
macro_rules! mark_cel {
	( $($n:ident <$t:ident>),+ ) => {
		$(
			impl<$t: ConstEncodedLen> ConstEncodedLen for $n<$t> { }
		)+
	};
	( $($t:ty),+ ) => {
		$(
			impl ConstEncodedLen for $t { }
		)+
	};
}

mark_cel!(u8, u16, u32, u64, u128, i8, i16, i32, i64, i128, bool);
mark_cel!(
	NonZeroU8,
	NonZeroU16,
	NonZeroU32,
	NonZeroU64,
	NonZeroU128,
	NonZeroI8,
	NonZeroI16,
	NonZeroI32,
	NonZeroI64,
	NonZeroI128
);

mark_cel!(Duration);
mark_cel!(PhantomData<T>);
mark_cel!(Box<T>);
mark_cel!(Range<T>, RangeInclusive<T>);

// `Option`, `Result` and `Compact` are sum types, therefore not `CEL`.

#[cfg(test)]
mod tests {
	use super::*;
	use crate::Encode;
	use proptest::prelude::*;

	/// Test that some random instances of `T` have encoded len `T::max_encoded_len()`.
	macro_rules! test_cel_compliance {
		( $( $t:ty ),+ ) => {
			$(
				paste::paste! {
					proptest::proptest! {
						#[test]
						fn [< cel_compliance_ $t:snake >](x: $t) {
							prop_assert_eq!(x.encode().len(), $t::max_encoded_len());
						}
					}
				}
			)*
		};
	}

	type Void = ();
	test_cel_compliance!(Void);

	test_cel_compliance!(u8, u16, u32, u64, u128, i8, i16, i32, i64, i128, bool);

	type TupleArithmetic = (u8, u16, u32, u64, u128, i8, i16, i32, i64, i128);
	test_cel_compliance!(TupleArithmetic);

	test_cel_compliance!(
		NonZeroU8,
		NonZeroU16,
		NonZeroU32,
		NonZeroU64,
		NonZeroU128,
		NonZeroI8,
		NonZeroI16,
		NonZeroI32,
		NonZeroI64,
		NonZeroI128
	);

	type TupleNonZero = (
		NonZeroU8,
		NonZeroU16,
		NonZeroU32,
		NonZeroU64,
		NonZeroU128,
		NonZeroI8,
		NonZeroI16,
		NonZeroI32,
		NonZeroI64,
		NonZeroI128,
	);
	test_cel_compliance!(TupleNonZero);

	type ArrayArithmetic = [(u8, u16, u32, u64, u128, i8, i16, i32, i64, i128); 10];
	test_cel_compliance!(ArrayArithmetic);

	test_cel_compliance!(Duration);

	type BoxedArithmetic = Box<(u8, u16, u32, u64, u128, i8, i16, i32, i64, i128)>;
	test_cel_compliance!(BoxedArithmetic);

	type PhantomArithmetic = PhantomData<(u8, u16, u32, u64, u128, i8, i16, i32, i64, i128)>;
	test_cel_compliance!(PhantomArithmetic);

	type Ranges = (Range<u8>, Range<u16>, Range<u32>, Range<u64>, Range<u128>);
	test_cel_compliance!(Ranges);

	type Ranges2D = (
		Range<(u8, u8)>,
		Range<(u16, u16)>,
		Range<(u32, u32)>,
		Range<(u64, u64)>,
		Range<(u128, u128)>,
	);
	test_cel_compliance!(Ranges2D);

	type RangesInc = (
		RangeInclusive<u8>,
		RangeInclusive<u16>,
		RangeInclusive<u32>,
		RangeInclusive<u64>,
		RangeInclusive<u128>,
	);
	test_cel_compliance!(RangesInc);

	type RangesInc2D = (
		RangeInclusive<(u8, u8)>,
		RangeInclusive<(u16, u16)>,
		RangeInclusive<(u32, u32)>,
		RangeInclusive<(u64, u64)>,
		RangeInclusive<(u128, u128)>,
	);
	test_cel_compliance!(RangesInc2D);
}
