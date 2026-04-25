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

use crate::{alloc::vec::Vec, encode_like::EncodeLike, Decode, Encode, Error, Input, Output};

impl<T: Encode, L: generic_array::ArrayLength<T>> Encode for generic_array::GenericArray<T, L> {
	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		for item in self.iter() {
			item.encode_to(dest);
		}
	}
}

impl<T: Encode, L: generic_array::ArrayLength<T>> EncodeLike for generic_array::GenericArray<T, L> {}

impl<T: Decode, L: generic_array::ArrayLength<T>> Decode for generic_array::GenericArray<T, L> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let mut r = Vec::with_capacity(L::to_usize());
		for _ in 0..L::to_usize() {
			r.push(T::decode(input)?);
		}
		let i = generic_array::GenericArray::from_exact_iter(r);

		match i {
			Some(a) => Ok(a),
			None => Err("array length does not match definition".into()),
		}
	}
}

#[cfg(test)]
mod tests {
	use super::*;
	use generic_array::{arr, typenum, GenericArray};

	#[test]
	fn generic_array() {
		let test = arr![u8; 3, 4, 5];
		let encoded = test.encode();
		assert_eq!(test, GenericArray::<u8, typenum::U3>::decode(&mut &encoded[..]).unwrap());

		let test = arr![u16; 3, 4, 5, 6, 7, 8, 0];
		let encoded = test.encode();
		assert_eq!(test, GenericArray::<u16, typenum::U7>::decode(&mut &encoded[..]).unwrap());

		let test = arr![u32; 3, 4, 5, 0, 1];
		let encoded = test.encode();
		assert_eq!(test, GenericArray::<u32, typenum::U5>::decode(&mut &encoded[..]).unwrap());

		let test = arr![u64; 3];
		let encoded = test.encode();
		assert_eq!(test, GenericArray::<u64, typenum::U1>::decode(&mut &encoded[..]).unwrap());
	}
}
