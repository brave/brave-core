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

use crate::codec::Encode;

/// A marker trait that tells the compiler that a type encode to the same representation as another
/// type.
///
/// E.g. `Vec<u8>` has the same encoded representation as `&[u8]`.
///
/// # Example
///
/// ```
/// # use parity_scale_codec::{EncodeLike, Encode};
/// fn encode_like<T: Encode, R: EncodeLike<T>>(data: &R) {
///     data.encode(); // Valid `T` encoded value.
/// }
///
/// fn main() {
///     // Just pass the a reference to the normal tuple.
///     encode_like::<(u32, u32), _>(&(1u32, 2u32));
///     // Pass a tuple of references
///     encode_like::<(u32, u32), _>(&(&1u32, &2u32));
///     // Pass a tuple of a reference and a value.
///     encode_like::<(u32, u32), _>(&(&1u32, 2u32));
/// }
/// ```
///
/// # Warning
///
/// The relation is not symetric, `T` implements `EncodeLike<U>` does not mean `U` has same
/// representation as `T`.
/// For instance we could imaging a non zero integer to be encoded to the same representation as
/// the said integer but not the other way around.
///
/// # Limitation
///
/// Not all possible implementations of EncodeLike are implemented (for instance `Box<Box<u32>>`
/// does not implement `EncodeLike<u32>`). To bypass this issue either open a PR to add the new
/// combination or use [`Ref`](./struct.Ref.html) reference wrapper or define your own wrapper
/// and implement `EncodeLike` on it as such:
/// ```
/// # use parity_scale_codec::{EncodeLike, Encode, WrapperTypeEncode};
/// fn encode_like<T: Encode, R: EncodeLike<T>>(data: &R) {
///     data.encode(); // Valid `T` encoded value.
/// }
///
/// struct MyWrapper<'a>(&'a (Box<Box<u32>>, u32));
/// impl<'a> core::ops::Deref for MyWrapper<'a> { // Or use derive_deref crate
///     type Target = (Box<Box<u32>>, u32);
///     fn deref(&self) -> &Self::Target { &self.0 }
/// }
///
/// impl<'a> parity_scale_codec::WrapperTypeEncode for MyWrapper<'a> {}
/// impl<'a> parity_scale_codec::EncodeLike<(u32, u32)> for MyWrapper<'a> {}
///
/// fn main() {
///     let v = (Box::new(Box::new(0)), 0);
///     encode_like::<(u32, u32), _>(&MyWrapper(&v));
/// }
/// ```
pub trait EncodeLike<T: Encode = Self>: Sized + Encode {}

/// Reference wrapper that implement encode like any type that is encoded like its inner type.
///
/// # Example
///
/// ```rust
/// # use parity_scale_codec::{EncodeLike, Ref};
/// fn foo<T: EncodeLike<u8>>(t: T) -> T {
///     store_t(Ref::from(&t)); // Store t without moving it, but only using a reference.
///     t
/// }
///
/// fn store_t<T: EncodeLike<u8>>(t: T) {
/// }
/// ```
pub struct Ref<'a, T: EncodeLike<U>, U: Encode>(&'a T, core::marker::PhantomData<U>);
impl<T: EncodeLike<U>, U: Encode> core::ops::Deref for Ref<'_, T, U> {
	type Target = T;
	fn deref(&self) -> &Self::Target {
		self.0
	}
}

impl<'a, T: EncodeLike<U>, U: Encode> From<&'a T> for Ref<'a, T, U> {
	fn from(x: &'a T) -> Self {
		Ref(x, Default::default())
	}
}
impl<T: EncodeLike<U>, U: Encode> crate::WrapperTypeEncode for Ref<'_, T, U> {}
impl<T: EncodeLike<U>, U: Encode> EncodeLike<U> for Ref<'_, T, U> {}
impl<T: EncodeLike<U>, U: Encode> EncodeLike<U> for &Ref<'_, T, U> {}

#[cfg(test)]
mod tests {
	use super::*;
	use std::collections::BTreeMap;

	struct ComplexStuff<T>(T);

	impl<T: Encode> ComplexStuff<T> {
		fn complex_method<R: Encode>(value: &R) -> Vec<u8>
		where
			T: EncodeLike<R>,
		{
			value.encode()
		}
	}

	#[test]
	fn vec_and_slice_are_working() {
		let slice: &[u8] = &[1, 2, 3, 4];
		let data: Vec<u8> = slice.to_vec();

		let data_encoded = data.encode();
		let slice_encoded = ComplexStuff::<Vec<u8>>::complex_method(&slice);

		assert_eq!(slice_encoded, data_encoded);
	}

	#[test]
	fn btreemap_and_slice_are_working() {
		let slice: &[(u32, u32)] = &[(1, 2), (23, 24), (28, 30), (45, 80)];
		let data: BTreeMap<u32, u32> = slice.iter().copied().collect();

		let data_encoded = data.encode();
		let slice_encoded = ComplexStuff::<BTreeMap<u32, u32>>::complex_method(&slice);

		assert_eq!(slice_encoded, data_encoded);
	}

	#[test]
	fn interface_testing() {
		let value = 10u32;
		let data = (value, value, value);
		let encoded = ComplexStuff::<(u32, u32, u32)>::complex_method(&data);
		assert_eq!(data.encode(), encoded);
		let data = (&value, &value, &value);
		let encoded = ComplexStuff::<(u32, u32, u32)>::complex_method(&data);
		assert_eq!(data.encode(), encoded);
		let data = (&value, value, &value);
		let encoded = ComplexStuff::<(u32, u32, u32)>::complex_method(&data);
		assert_eq!(data.encode(), encoded);

		let vec_data: Vec<u8> = vec![1, 2, 3];
		ComplexStuff::<Vec<u8>>::complex_method(&vec_data);
		ComplexStuff::<&'static str>::complex_method(&String::from("test"));
		ComplexStuff::<&'static str>::complex_method(&"test");

		let slice: &[u8] = &vec_data;
		assert_eq!(
			ComplexStuff::<(u32, Vec<u8>)>::complex_method(&(1u32, slice.to_vec())),
			ComplexStuff::<(u32, Vec<u8>)>::complex_method(&(1u32, slice))
		);
	}
}
