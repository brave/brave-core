// Copyright 2019 Parity Technologies
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use core::iter::ExactSizeIterator;

use crate::{
	alloc::vec::Vec,
	compact::{Compact, CompactLen},
	encode_like::EncodeLike,
	Decode, Encode, Error,
};

/// Trait that allows to append items to an encoded representation without
/// decoding all previous added items.
pub trait EncodeAppend {
	/// The item that will be appended.
	type Item: Encode;

	/// Append all items in `iter` to the given `self_encoded` representation
	/// or if `self_encoded` value is empty, `iter` is encoded to the `Self` representation.
	///
	/// # Example
	///
	/// ```
	/// # use parity_scale_codec::EncodeAppend;
	///
	/// // Some encoded data
	/// let data = Vec::new();
	///
	/// let item = 8u32;
	/// let encoded = <Vec<u32> as EncodeAppend>::append_or_new(data, std::iter::once(&item)).expect("Adds new element");
	///
	/// // Add multiple element
	/// <Vec<u32> as EncodeAppend>::append_or_new(encoded, &[700u32, 800u32, 10u32]).expect("Adds new elements");
	/// ```
	fn append_or_new<EncodeLikeItem, I>(self_encoded: Vec<u8>, iter: I) -> Result<Vec<u8>, Error>
	where
		I: IntoIterator<Item = EncodeLikeItem>,
		EncodeLikeItem: EncodeLike<Self::Item>,
		I::IntoIter: ExactSizeIterator;
}

impl<T: Encode> EncodeAppend for Vec<T> {
	type Item = T;

	fn append_or_new<EncodeLikeItem, I>(self_encoded: Vec<u8>, iter: I) -> Result<Vec<u8>, Error>
	where
		I: IntoIterator<Item = EncodeLikeItem>,
		EncodeLikeItem: EncodeLike<Self::Item>,
		I::IntoIter: ExactSizeIterator,
	{
		append_or_new_impl(self_encoded, iter)
	}
}

impl<T: Encode> EncodeAppend for crate::alloc::collections::VecDeque<T> {
	type Item = T;

	fn append_or_new<EncodeLikeItem, I>(self_encoded: Vec<u8>, iter: I) -> Result<Vec<u8>, Error>
	where
		I: IntoIterator<Item = EncodeLikeItem>,
		EncodeLikeItem: EncodeLike<Self::Item>,
		I::IntoIter: ExactSizeIterator,
	{
		append_or_new_impl(self_encoded, iter)
	}
}

/// Extends a SCALE-encoded vector with elements from the given `iter`.
///
/// `vec` must either be empty, or contain a valid SCALE-encoded `Vec<Item>` payload.
fn append_or_new_impl<Item, I>(mut vec: Vec<u8>, iter: I) -> Result<Vec<u8>, Error>
where
	Item: Encode,
	I: IntoIterator<Item = Item>,
	I::IntoIter: ExactSizeIterator,
{
	let iter = iter.into_iter();
	let items_to_append = iter.len();

	if vec.is_empty() {
		crate::codec::compact_encode_len_to(&mut vec, items_to_append)?;
	} else {
		let old_item_count = u32::from(Compact::<u32>::decode(&mut &vec[..])?);
		let new_item_count = old_item_count
			.checked_add(items_to_append as u32)
			.ok_or("cannot append new items into a SCALE-encoded vector: length overflow due to too many items")?;

		let old_item_count_encoded_bytesize = Compact::<u32>::compact_len(&old_item_count);
		let new_item_count_encoded_bytesize = Compact::<u32>::compact_len(&new_item_count);

		if old_item_count_encoded_bytesize == new_item_count_encoded_bytesize {
			// The size of the length as encoded by SCALE didn't change, so we can just
			// keep the old buffer as-is. We just need to update the length prefix.
			Compact(new_item_count).using_encoded(|length_encoded| {
				vec[..old_item_count_encoded_bytesize].copy_from_slice(length_encoded)
			});
		} else {
			// We can't update the length as the new length prefix will take up more
			// space when encoded, so we need to move our data to make space for it.

			// If this overflows then it means that `vec` is bigger that half of the
			// total address space, which means that it will be impossible to allocate
			// enough memory for another vector of at least the same size.
			//
			// So let's just immediately bail with an error if this happens.
			let new_capacity = vec.len().checked_mul(2)
				.ok_or("cannot append new items into a SCALE-encoded vector: new vector won't fit in memory")?;
			let mut new_vec = Vec::with_capacity(new_capacity);

			crate::codec::compact_encode_len_to(&mut new_vec, new_item_count as usize)?;
			new_vec.extend_from_slice(&vec[old_item_count_encoded_bytesize..]);
			vec = new_vec;
		}
	}

	// And now we just need to append the new items.
	iter.for_each(|e| e.encode_to(&mut vec));
	Ok(vec)
}

#[cfg(test)]
mod tests {
	use super::*;
	use crate::{Encode, EncodeLike, Input};
	use std::collections::VecDeque;

	const TEST_VALUE: u32 = {
		#[cfg(not(miri))]
		{
			1_000_000
		}
		#[cfg(miri)]
		{
			1_000
		}
	};

	#[test]
	fn vec_encode_append_works() {
		let encoded = (0..TEST_VALUE).fold(Vec::new(), |encoded, v| {
			<Vec<u32> as EncodeAppend>::append_or_new(encoded, std::iter::once(&v)).unwrap()
		});

		let decoded = Vec::<u32>::decode(&mut &encoded[..]).unwrap();
		assert_eq!(decoded, (0..TEST_VALUE).collect::<Vec<_>>());
	}

	#[test]
	fn vec_encode_append_multiple_items_works() {
		let encoded = (0..TEST_VALUE).fold(Vec::new(), |encoded, v| {
			<Vec<u32> as EncodeAppend>::append_or_new(encoded, [v, v, v, v]).unwrap()
		});

		let decoded = Vec::<u32>::decode(&mut &encoded[..]).unwrap();
		let expected = (0..TEST_VALUE).fold(Vec::new(), |mut vec, i| {
			vec.append(&mut vec![i, i, i, i]);
			vec
		});
		assert_eq!(decoded, expected);
	}

	#[test]
	fn vecdeque_encode_append_works() {
		let encoded = (0..TEST_VALUE).fold(Vec::new(), |encoded, v| {
			<VecDeque<u32> as EncodeAppend>::append_or_new(encoded, std::iter::once(&v)).unwrap()
		});

		let decoded = VecDeque::<u32>::decode(&mut &encoded[..]).unwrap();
		assert_eq!(decoded, (0..TEST_VALUE).collect::<Vec<_>>());
	}

	#[test]
	fn vecdeque_encode_append_multiple_items_works() {
		let encoded = (0..TEST_VALUE).fold(Vec::new(), |encoded, v| {
			<VecDeque<u32> as EncodeAppend>::append_or_new(encoded, [v, v, v, v]).unwrap()
		});

		let decoded = VecDeque::<u32>::decode(&mut &encoded[..]).unwrap();
		let expected = (0..TEST_VALUE).fold(Vec::new(), |mut vec, i| {
			vec.append(&mut vec![i, i, i, i]);
			vec
		});
		assert_eq!(decoded, expected);
	}

	#[test]
	fn append_non_copyable() {
		#[derive(Eq, PartialEq, Debug)]
		struct NoCopy {
			data: u32,
		}

		impl EncodeLike for NoCopy {}

		impl Encode for NoCopy {
			fn encode(&self) -> Vec<u8> {
				self.data.encode()
			}
		}

		impl Decode for NoCopy {
			fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
				u32::decode(input).map(|data| Self { data })
			}
		}

		let append = NoCopy { data: 100 };
		let data = Vec::new();
		let encoded =
			<Vec<NoCopy> as EncodeAppend>::append_or_new(data, std::iter::once(&append)).unwrap();

		let decoded = <Vec<NoCopy>>::decode(&mut &encoded[..]).unwrap();
		assert_eq!(vec![append], decoded);
	}

	#[test]
	fn vec_encode_like_append_works() {
		let encoded = (0..TEST_VALUE).fold(Vec::new(), |encoded, v| {
			<Vec<u32> as EncodeAppend>::append_or_new(encoded, std::iter::once(Box::new(v)))
				.unwrap()
		});

		let decoded = Vec::<u32>::decode(&mut &encoded[..]).unwrap();
		assert_eq!(decoded, (0..TEST_VALUE).collect::<Vec<_>>());
	}
}
