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

//! `BitVec` specific serialization.

use crate::{
	codec::decode_vec_with_len, Compact, Decode, DecodeWithMemTracking, Encode, EncodeLike, Error,
	Input, Output,
};
use bitvec::{
	boxed::BitBox, order::BitOrder, slice::BitSlice, store::BitStore, vec::BitVec, view::BitView,
};

impl<O: BitOrder, T: BitStore + Encode> Encode for BitSlice<T, O> {
	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		let bits = self.len();
		assert!(
			bits <= ARCH32BIT_BITSLICE_MAX_BITS,
			"Attempted to encode a BitSlice with too many bits.",
		);
		Compact(bits as u32).encode_to(dest);

		// Iterate over chunks
		for chunk in self.chunks(core::mem::size_of::<T>() * 8) {
			let mut element = T::ZERO;
			element.view_bits_mut::<O>()[..chunk.len()].copy_from_bitslice(chunk);
			element.encode_to(dest);
		}
	}
}

impl<O: BitOrder, T: BitStore + Encode> Encode for BitVec<T, O> {
	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		self.as_bitslice().encode_to(dest)
	}
}

impl<O: BitOrder, T: BitStore + Encode> EncodeLike for BitVec<T, O> {}

/// Equivalent of `BitStore::MAX_BITS` on 32bit machine.
const ARCH32BIT_BITSLICE_MAX_BITS: usize = 0x1fff_ffff;

impl<O: BitOrder, T: BitStore + Decode> Decode for BitVec<T, O> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		<Compact<u32>>::decode(input).and_then(move |Compact(bits)| {
			// Otherwise it is impossible to store it on 32bit machine.
			if bits as usize > ARCH32BIT_BITSLICE_MAX_BITS {
				return Err("Attempt to decode a BitVec with too many bits".into());
			}
			let vec = decode_vec_with_len(input, bitvec::mem::elts::<T>(bits as usize))?;

			let mut result = Self::try_from_vec(vec).map_err(|_| {
				Error::from(
					"UNEXPECTED ERROR: `bits` is less or equal to
					`ARCH32BIT_BITSLICE_MAX_BITS`; So BitVec must be able to handle the number of
					segment needed for `bits` to be represented; qed",
				)
			})?;

			assert!(bits as usize <= result.len());
			result.truncate(bits as usize);
			Ok(result)
		})
	}
}

impl<O: BitOrder, T: BitStore + Decode> DecodeWithMemTracking for BitVec<T, O> {}

impl<O: BitOrder, T: BitStore + Encode> Encode for BitBox<T, O> {
	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		self.as_bitslice().encode_to(dest)
	}
}

impl<O: BitOrder, T: BitStore + Encode> EncodeLike for BitBox<T, O> {}

impl<O: BitOrder, T: BitStore + Decode> Decode for BitBox<T, O> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		Ok(BitVec::<T, O>::decode(input)?.into())
	}
}

impl<O: BitOrder, T: BitStore + Decode> DecodeWithMemTracking for BitBox<T, O> {}

#[cfg(test)]
mod tests {
	use super::*;
	use crate::{codec::INITIAL_PREALLOCATION, CompactLen};
	use bitvec::{
		bitvec,
		order::{Lsb0, Msb0},
	};

	macro_rules! test_data {
		($inner_type:ident) => (
			[
				BitVec::<$inner_type, Msb0>::new(),
				bitvec![$inner_type, Msb0; 0],
				bitvec![$inner_type, Msb0; 1],
				bitvec![$inner_type, Msb0; 0, 0],
				bitvec![$inner_type, Msb0; 1, 0],
				bitvec![$inner_type, Msb0; 0, 1],
				bitvec![$inner_type, Msb0; 1, 1],
				bitvec![$inner_type, Msb0; 1, 0, 1],
				bitvec![$inner_type, Msb0; 0, 1, 0, 1, 0, 1, 1],
				bitvec![$inner_type, Msb0; 0, 1, 0, 1, 0, 1, 1, 0],
				bitvec![$inner_type, Msb0; 1, 1, 0, 1, 0, 1, 1, 0, 1],
				bitvec![$inner_type, Msb0; 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0],
				bitvec![$inner_type, Msb0; 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0],
				bitvec![$inner_type, Msb0; 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0],
				bitvec![$inner_type, Msb0; 0; 15],
				bitvec![$inner_type, Msb0; 1; 16],
				bitvec![$inner_type, Msb0; 0; 17],
				bitvec![$inner_type, Msb0; 1; 31],
				bitvec![$inner_type, Msb0; 0; 32],
				bitvec![$inner_type, Msb0; 1; 33],
				bitvec![$inner_type, Msb0; 0; 63],
				bitvec![$inner_type, Msb0; 1; 64],
				bitvec![$inner_type, Msb0; 0; 65],
				bitvec![$inner_type, Msb0; 1; INITIAL_PREALLOCATION * 8 + 1],
				bitvec![$inner_type, Msb0; 0; INITIAL_PREALLOCATION * 9],
				bitvec![$inner_type, Msb0; 1; INITIAL_PREALLOCATION * 32 + 1],
				bitvec![$inner_type, Msb0; 0; INITIAL_PREALLOCATION * 33],
			]
		)
	}

	#[test]
	fn bitvec_u8() {
		for v in &test_data!(u8) {
			let encoded = v.encode();
			assert_eq!(*v, BitVec::<u8, Msb0>::decode(&mut &encoded[..]).unwrap());

			let elements = bitvec::mem::elts::<u8>(v.len());
			let compact_len = Compact::compact_len(&(v.len() as u32));
			assert_eq!(compact_len + elements, encoded.len(), "{}", v);
		}
	}

	#[test]
	fn bitvec_u16() {
		for v in &test_data!(u16) {
			let encoded = v.encode();
			assert_eq!(*v, BitVec::<u16, Msb0>::decode(&mut &encoded[..]).unwrap());

			let elements = bitvec::mem::elts::<u16>(v.len());
			let compact_len = Compact::compact_len(&(v.len() as u32));
			assert_eq!(compact_len + elements * 2, encoded.len(), "{}", v);
		}
	}

	#[test]
	fn bitvec_u32() {
		for v in &test_data!(u32) {
			let encoded = v.encode();
			assert_eq!(*v, BitVec::<u32, Msb0>::decode(&mut &encoded[..]).unwrap());

			let elements = bitvec::mem::elts::<u32>(v.len());
			let compact_len = Compact::compact_len(&(v.len() as u32));
			assert_eq!(compact_len + elements * 4, encoded.len(), "{}", v);
		}
	}

	#[test]
	fn bitvec_u64() {
		for v in &test_data!(u64) {
			let encoded = v.encode();
			assert_eq!(*v, BitVec::<u64, Msb0>::decode(&mut &encoded[..]).unwrap());

			let elements = bitvec::mem::elts::<u64>(v.len());
			let compact_len = Compact::compact_len(&(v.len() as u32));
			assert_eq!(compact_len + elements * 8, encoded.len(), "{}", v);
		}
	}

	#[test]
	fn bitslice() {
		let data: &[u8] = &[0x69];
		let slice = BitSlice::<u8, Msb0>::from_slice(data);
		let encoded = slice.encode();
		let decoded = BitVec::<u8, Msb0>::decode(&mut &encoded[..]).unwrap();
		assert_eq!(slice, decoded.as_bitslice());
	}

	#[test]
	fn bitbox() {
		let data: &[u8] = &[5, 10];
		let slice = BitSlice::<u8, Msb0>::from_slice(data);
		let bb = BitBox::<u8, Msb0>::from_bitslice(slice);
		let encoded = bb.encode();
		let decoded = BitBox::<u8, Msb0>::decode(&mut &encoded[..]).unwrap();
		assert_eq!(bb, decoded);
	}

	#[test]
	fn bitvec_u8_encodes_as_expected() {
		let cases = vec![
			(bitvec![u8, Lsb0; 0, 0, 1, 1].encode(), (Compact(4u32), 0b00001100u8).encode()),
			(bitvec![u8, Lsb0; 0, 1, 1, 1].encode(), (Compact(4u32), 0b00001110u8).encode()),
			(bitvec![u8, Lsb0; 1, 1, 1, 1].encode(), (Compact(4u32), 0b00001111u8).encode()),
			(bitvec![u8, Lsb0; 1, 1, 1, 1, 1].encode(), (Compact(5u32), 0b00011111u8).encode()),
			(bitvec![u8, Lsb0; 1, 1, 1, 1, 1, 0].encode(), (Compact(6u32), 0b00011111u8).encode()),
			(
				bitvec![u8, Lsb0; 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1].encode(),
				(Compact(12u32), 0b00011111u8, 0b00001011u8).encode(),
			),
		];

		for (idx, (actual, expected)) in cases.into_iter().enumerate() {
			assert_eq!(actual, expected, "case at index {} failed; encodings differ", idx);
		}
	}
}
