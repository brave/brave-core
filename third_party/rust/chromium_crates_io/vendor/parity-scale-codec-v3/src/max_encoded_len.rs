// Copyright (C) 2021 Parity Technologies (UK) Ltd.
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

//! `trait MaxEncodedLen` bounds the maximum encoded length of items.

use crate::{alloc::boxed::Box, Compact, Encode};
use core::{
	marker::PhantomData,
	mem,
	num::*,
	ops::{Range, RangeInclusive},
	time::Duration,
};
use impl_trait_for_tuples::impl_for_tuples;

#[cfg(target_has_atomic = "ptr")]
use crate::alloc::sync::Arc;

/// Items implementing `MaxEncodedLen` have a statically known maximum encoded size.
///
/// Some containers, such as `BoundedVec`, have enforced size limits and this trait
/// can be implemented accurately. Other containers, such as `StorageMap`, do not have enforced size
/// limits. For those containers, it is necessary to make a documented assumption about the maximum
/// usage, and compute the max encoded length based on that assumption.
pub trait MaxEncodedLen: Encode {
	/// Upper bound, in bytes, of the maximum encoded size of this item.
	fn max_encoded_len() -> usize;
}

macro_rules! impl_primitives {
	( $($t:ty),+ ) => {
		$(
			impl MaxEncodedLen for $t {
				fn max_encoded_len() -> usize {
					mem::size_of::<$t>()
				}
			}
		)+
	};
}

impl_primitives!(u8, u16, u32, u64, u128, i8, i16, i32, i64, i128, bool);

impl_primitives!(
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

macro_rules! impl_compact {
	($( $t:ty => $e:expr; )*) => {
		$(
			impl MaxEncodedLen for Compact<$t> {
				fn max_encoded_len() -> usize {
					$e
				}
			}
		)*
	};
}

impl_compact!(
	() => 0;
	// github.com/paritytech/parity-scale-codec/blob/f0341dabb01aa9ff0548558abb6dcc5c31c669a1/src/compact.rs#L261
	u8 => 2;
	// github.com/paritytech/parity-scale-codec/blob/f0341dabb01aa9ff0548558abb6dcc5c31c669a1/src/compact.rs#L291
	u16 => 4;
	// github.com/paritytech/parity-scale-codec/blob/f0341dabb01aa9ff0548558abb6dcc5c31c669a1/src/compact.rs#L326
	u32 => 5;
	// github.com/paritytech/parity-scale-codec/blob/f0341dabb01aa9ff0548558abb6dcc5c31c669a1/src/compact.rs#L369
	u64 => 9;
	// github.com/paritytech/parity-scale-codec/blob/f0341dabb01aa9ff0548558abb6dcc5c31c669a1/src/compact.rs#L413
	u128 => 17;
);

// impl_for_tuples for values 19 and higher fails because that's where the WrapperTypeEncode impl
// stops.
#[impl_for_tuples(18)]
impl MaxEncodedLen for Tuple {
	fn max_encoded_len() -> usize {
		let mut len: usize = 0;
		for_tuples!( #( len = len.saturating_add(Tuple::max_encoded_len()); )* );
		len
	}
}

impl<T: MaxEncodedLen, const N: usize> MaxEncodedLen for [T; N] {
	fn max_encoded_len() -> usize {
		T::max_encoded_len().saturating_mul(N)
	}
}

impl<T: MaxEncodedLen> MaxEncodedLen for Box<T> {
	fn max_encoded_len() -> usize {
		T::max_encoded_len()
	}
}

#[cfg(target_has_atomic = "ptr")]
impl<T: MaxEncodedLen> MaxEncodedLen for Arc<T> {
	fn max_encoded_len() -> usize {
		T::max_encoded_len()
	}
}

impl<T: MaxEncodedLen> MaxEncodedLen for Option<T> {
	fn max_encoded_len() -> usize {
		T::max_encoded_len().saturating_add(1)
	}
}

impl<T, E> MaxEncodedLen for Result<T, E>
where
	T: MaxEncodedLen,
	E: MaxEncodedLen,
{
	fn max_encoded_len() -> usize {
		T::max_encoded_len().max(E::max_encoded_len()).saturating_add(1)
	}
}

impl<T> MaxEncodedLen for PhantomData<T> {
	fn max_encoded_len() -> usize {
		0
	}
}

impl MaxEncodedLen for Duration {
	fn max_encoded_len() -> usize {
		u64::max_encoded_len() + u32::max_encoded_len()
	}
}

impl<T: MaxEncodedLen> MaxEncodedLen for Range<T> {
	fn max_encoded_len() -> usize {
		T::max_encoded_len().saturating_mul(2)
	}
}

impl<T: MaxEncodedLen> MaxEncodedLen for RangeInclusive<T> {
	fn max_encoded_len() -> usize {
		T::max_encoded_len().saturating_mul(2)
	}
}

#[cfg(test)]
mod tests {
	use super::*;

	macro_rules! test_compact_length {
		($(fn $name:ident($t:ty);)*) => {
			$(
				#[test]
				fn $name() {
					assert_eq!(Compact(<$t>::MAX).encode().len(), Compact::<$t>::max_encoded_len());
				}
			)*
		};
	}

	test_compact_length!(
		fn compact_u8(u8);
		fn compact_u16(u16);
		fn compact_u32(u32);
		fn compact_u64(u64);
		fn compact_u128(u128);
	);
}
