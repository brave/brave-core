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

//! [Compact encoding](https://docs.substrate.io/v3/advanced/scale-codec/#compactgeneral-integers)

use arrayvec::ArrayVec;

use crate::{
	alloc::vec::Vec,
	codec::{Decode, Encode, EncodeAsRef, Input, Output},
	encode_like::EncodeLike,
	DecodeWithMemTracking, Error,
};

#[cfg(feature = "fuzz")]
use arbitrary::Arbitrary;

struct ArrayVecWrapper<const N: usize>(ArrayVec<u8, N>);

impl<const N: usize> Output for ArrayVecWrapper<N> {
	fn write(&mut self, bytes: &[u8]) {
		let old_len = self.0.len();
		let new_len = old_len + bytes.len();

		assert!(new_len <= self.0.capacity());
		unsafe {
			self.0.set_len(new_len);
		}

		self.0[old_len..new_len].copy_from_slice(bytes);
	}

	fn push_byte(&mut self, byte: u8) {
		self.0.push(byte);
	}
}

/// Prefix another input with a byte.
struct PrefixInput<'a, T> {
	prefix: Option<u8>,
	input: &'a mut T,
}

impl<'a, T: 'a + Input> Input for PrefixInput<'a, T> {
	fn remaining_len(&mut self) -> Result<Option<usize>, Error> {
		let len = if let Some(len) = self.input.remaining_len()? {
			Some(len.saturating_add(self.prefix.iter().count()))
		} else {
			None
		};
		Ok(len)
	}

	fn read(&mut self, buffer: &mut [u8]) -> Result<(), Error> {
		if buffer.is_empty() {
			return Ok(());
		}
		match self.prefix.take() {
			Some(v) => {
				buffer[0] = v;
				self.input.read(&mut buffer[1..])
			},
			_ => self.input.read(buffer),
		}
	}
}

/// Something that can return the compact encoded length for a given value.
pub trait CompactLen<T> {
	/// Returns the compact encoded length for the given value.
	fn compact_len(val: &T) -> usize;
}

/// Compact-encoded variant of T. This is more space-efficient but less compute-efficient.
#[derive(Eq, PartialEq, Clone, Copy, Ord, PartialOrd)]
#[cfg_attr(feature = "fuzz", derive(Arbitrary))]
pub struct Compact<T>(pub T);

impl<T> From<T> for Compact<T> {
	fn from(x: T) -> Compact<T> {
		Compact(x)
	}
}

impl<'a, T: Copy> From<&'a T> for Compact<T> {
	fn from(x: &'a T) -> Compact<T> {
		Compact(*x)
	}
}

/// Allow foreign structs to be wrap in Compact
pub trait CompactAs: From<Compact<Self>> {
	/// A compact-encodable type that should be used as the encoding.
	type As;

	/// Returns the compact-encodable type.
	fn encode_as(&self) -> &Self::As;

	/// Decode `Self` from the compact-decoded type.
	fn decode_from(_: Self::As) -> Result<Self, Error>;
}

impl<T> EncodeLike for Compact<T> where for<'a> CompactRef<'a, T>: Encode {}

impl<T> Encode for Compact<T>
where
	for<'a> CompactRef<'a, T>: Encode,
{
	fn size_hint(&self) -> usize {
		CompactRef(&self.0).size_hint()
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		CompactRef(&self.0).encode_to(dest)
	}

	fn encode(&self) -> Vec<u8> {
		CompactRef(&self.0).encode()
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		CompactRef(&self.0).using_encoded(f)
	}
}

impl<T> EncodeLike for CompactRef<'_, T>
where
	T: CompactAs,
	for<'b> CompactRef<'b, T::As>: Encode,
{
}

impl<T> Encode for CompactRef<'_, T>
where
	T: CompactAs,
	for<'b> CompactRef<'b, T::As>: Encode,
{
	fn size_hint(&self) -> usize {
		CompactRef(self.0.encode_as()).size_hint()
	}

	fn encode_to<Out: Output + ?Sized>(&self, dest: &mut Out) {
		CompactRef(self.0.encode_as()).encode_to(dest)
	}

	fn encode(&self) -> Vec<u8> {
		CompactRef(self.0.encode_as()).encode()
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		CompactRef(self.0.encode_as()).using_encoded(f)
	}
}

impl<T> Decode for Compact<T>
where
	T: CompactAs,
	Compact<T::As>: Decode,
{
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let as_ = Compact::<T::As>::decode(input)?;
		Ok(Compact(<T as CompactAs>::decode_from(as_.0)?))
	}
}

impl<T> DecodeWithMemTracking for Compact<T>
where
	T: CompactAs,
	Compact<T::As>: DecodeWithMemTracking,
{
}

macro_rules! impl_from_compact {
	( $( $ty:ty ),* ) => {
		$(
			impl From<Compact<$ty>> for $ty {
				fn from(x: Compact<$ty>) -> $ty { x.0 }
			}
		)*
	}
}

impl_from_compact! { (), u8, u16, u32, u64, u128 }

/// Compact-encoded variant of &'a T. This is more space-efficient but less compute-efficient.
#[derive(Eq, PartialEq, Clone, Copy)]
pub struct CompactRef<'a, T>(pub &'a T);

impl<'a, T> From<&'a T> for CompactRef<'a, T> {
	fn from(x: &'a T) -> Self {
		CompactRef(x)
	}
}

impl<T> core::fmt::Debug for Compact<T>
where
	T: core::fmt::Debug,
{
	fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
		self.0.fmt(f)
	}
}

#[cfg(feature = "serde")]
impl<T> serde::Serialize for Compact<T>
where
	T: serde::Serialize,
{
	fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
	where
		S: serde::Serializer,
	{
		T::serialize(&self.0, serializer)
	}
}

#[cfg(feature = "serde")]
impl<'de, T> serde::Deserialize<'de> for Compact<T>
where
	T: serde::Deserialize<'de>,
{
	fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
	where
		D: serde::Deserializer<'de>,
	{
		T::deserialize(deserializer).map(Compact)
	}
}

/// Trait that tells you if a given type can be encoded/decoded in a compact way.
pub trait HasCompact: Sized {
	/// The compact type; this can be
	type Type: for<'a> EncodeAsRef<'a, Self> + Decode + From<Self> + Into<Self>;
}

impl<'a, T: 'a> EncodeAsRef<'a, T> for Compact<T>
where
	CompactRef<'a, T>: Encode + From<&'a T>,
{
	type RefType = CompactRef<'a, T>;
}

impl<T: 'static> HasCompact for T
where
	Compact<T>: for<'a> EncodeAsRef<'a, T> + Decode + From<Self> + Into<Self>,
{
	type Type = Compact<T>;
}

impl Encode for CompactRef<'_, ()> {
	fn encode_to<W: Output + ?Sized>(&self, _dest: &mut W) {}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		f(&[])
	}

	fn encode(&self) -> Vec<u8> {
		Vec::new()
	}
}

impl Encode for CompactRef<'_, u8> {
	fn size_hint(&self) -> usize {
		Compact::compact_len(self.0)
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		match self.0 {
			0..=0b0011_1111 => dest.push_byte(self.0 << 2),
			_ => ((u16::from(*self.0) << 2) | 0b01).encode_to(dest),
		}
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		let mut r = ArrayVecWrapper(ArrayVec::<u8, 2>::new());
		self.encode_to(&mut r);
		f(&r.0)
	}
}

impl CompactLen<u8> for Compact<u8> {
	fn compact_len(val: &u8) -> usize {
		match val {
			0..=0b0011_1111 => 1,
			_ => 2,
		}
	}
}

impl Encode for CompactRef<'_, u16> {
	fn size_hint(&self) -> usize {
		Compact::compact_len(self.0)
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		match self.0 {
			0..=0b0011_1111 => dest.push_byte((*self.0 as u8) << 2),
			0..=0b0011_1111_1111_1111 => ((*self.0 << 2) | 0b01).encode_to(dest),
			_ => ((u32::from(*self.0) << 2) | 0b10).encode_to(dest),
		}
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		let mut r = ArrayVecWrapper(ArrayVec::<u8, 4>::new());
		self.encode_to(&mut r);
		f(&r.0)
	}
}

impl CompactLen<u16> for Compact<u16> {
	fn compact_len(val: &u16) -> usize {
		match val {
			0..=0b0011_1111 => 1,
			0..=0b0011_1111_1111_1111 => 2,
			_ => 4,
		}
	}
}

impl Encode for CompactRef<'_, u32> {
	fn size_hint(&self) -> usize {
		Compact::compact_len(self.0)
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		match self.0 {
			0..=0b0011_1111 => dest.push_byte((*self.0 as u8) << 2),
			0..=0b0011_1111_1111_1111 => (((*self.0 as u16) << 2) | 0b01).encode_to(dest),
			0..=0b0011_1111_1111_1111_1111_1111_1111_1111 =>
				((*self.0 << 2) | 0b10).encode_to(dest),
			_ => {
				dest.push_byte(0b11);
				self.0.encode_to(dest);
			},
		}
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		let mut r = ArrayVecWrapper(ArrayVec::<u8, 5>::new());
		self.encode_to(&mut r);
		f(&r.0)
	}
}

impl CompactLen<u32> for Compact<u32> {
	fn compact_len(val: &u32) -> usize {
		match val {
			0..=0b0011_1111 => 1,
			0..=0b0011_1111_1111_1111 => 2,
			0..=0b0011_1111_1111_1111_1111_1111_1111_1111 => 4,
			_ => 5,
		}
	}
}

impl Encode for CompactRef<'_, u64> {
	fn size_hint(&self) -> usize {
		Compact::compact_len(self.0)
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		match self.0 {
			0..=0b0011_1111 => dest.push_byte((*self.0 as u8) << 2),
			0..=0b0011_1111_1111_1111 => (((*self.0 as u16) << 2) | 0b01).encode_to(dest),
			0..=0b0011_1111_1111_1111_1111_1111_1111_1111 =>
				(((*self.0 as u32) << 2) | 0b10).encode_to(dest),
			_ => {
				let bytes_needed = 8 - self.0.leading_zeros() / 8;
				assert!(
					bytes_needed >= 4,
					"Previous match arm matches anyting less than 2^30; qed"
				);
				dest.push_byte(0b11 + ((bytes_needed - 4) << 2) as u8);
				let mut v = *self.0;
				for _ in 0..bytes_needed {
					dest.push_byte(v as u8);
					v >>= 8;
				}
				assert_eq!(v, 0, "shifted sufficient bits right to lead only leading zeros; qed")
			},
		}
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		let mut r = ArrayVecWrapper(ArrayVec::<u8, 9>::new());
		self.encode_to(&mut r);
		f(&r.0)
	}
}

impl CompactLen<u64> for Compact<u64> {
	fn compact_len(val: &u64) -> usize {
		match val {
			0..=0b0011_1111 => 1,
			0..=0b0011_1111_1111_1111 => 2,
			0..=0b0011_1111_1111_1111_1111_1111_1111_1111 => 4,
			_ => (8 - val.leading_zeros() / 8) as usize + 1,
		}
	}
}

impl Encode for CompactRef<'_, u128> {
	fn size_hint(&self) -> usize {
		Compact::compact_len(self.0)
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		match self.0 {
			0..=0b0011_1111 => dest.push_byte((*self.0 as u8) << 2),
			0..=0b0011_1111_1111_1111 => (((*self.0 as u16) << 2) | 0b01).encode_to(dest),
			0..=0b0011_1111_1111_1111_1111_1111_1111_1111 =>
				(((*self.0 as u32) << 2) | 0b10).encode_to(dest),
			_ => {
				let bytes_needed = 16 - self.0.leading_zeros() / 8;
				assert!(
					bytes_needed >= 4,
					"Previous match arm matches anyting less than 2^30; qed"
				);
				dest.push_byte(0b11 + ((bytes_needed - 4) << 2) as u8);
				let mut v = *self.0;
				for _ in 0..bytes_needed {
					dest.push_byte(v as u8);
					v >>= 8;
				}
				assert_eq!(v, 0, "shifted sufficient bits right to lead only leading zeros; qed")
			},
		}
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		let mut r = ArrayVecWrapper(ArrayVec::<u8, 17>::new());
		self.encode_to(&mut r);
		f(&r.0)
	}
}

impl CompactLen<u128> for Compact<u128> {
	fn compact_len(val: &u128) -> usize {
		match val {
			0..=0b0011_1111 => 1,
			0..=0b0011_1111_1111_1111 => 2,
			0..=0b0011_1111_1111_1111_1111_1111_1111_1111 => 4,
			_ => (16 - val.leading_zeros() / 8) as usize + 1,
		}
	}
}

impl Decode for Compact<()> {
	fn decode<I: Input>(_input: &mut I) -> Result<Self, Error> {
		Ok(Compact(()))
	}
}

impl DecodeWithMemTracking for Compact<()> {}

const U8_OUT_OF_RANGE: &str = "out of range decoding Compact<u8>";
const U16_OUT_OF_RANGE: &str = "out of range decoding Compact<u16>";
const U32_OUT_OF_RANGE: &str = "out of range decoding Compact<u32>";
const U64_OUT_OF_RANGE: &str = "out of range decoding Compact<u64>";
const U128_OUT_OF_RANGE: &str = "out of range decoding Compact<u128>";

impl Decode for Compact<u8> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let prefix = input.read_byte()?;
		Ok(Compact(match prefix % 4 {
			0 => prefix >> 2,
			1 => {
				let x = u16::decode(&mut PrefixInput { prefix: Some(prefix), input })? >> 2;
				if x > 0b0011_1111 && x <= 255 {
					x as u8
				} else {
					return Err(U8_OUT_OF_RANGE.into());
				}
			},
			_ => return Err("unexpected prefix decoding Compact<u8>".into()),
		}))
	}
}

impl DecodeWithMemTracking for Compact<u8> {}

impl Decode for Compact<u16> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let prefix = input.read_byte()?;
		Ok(Compact(match prefix % 4 {
			0 => u16::from(prefix) >> 2,
			1 => {
				let x = u16::decode(&mut PrefixInput { prefix: Some(prefix), input })? >> 2;
				if x > 0b0011_1111 && x <= 0b0011_1111_1111_1111 {
					x
				} else {
					return Err(U16_OUT_OF_RANGE.into());
				}
			},
			2 => {
				let x = u32::decode(&mut PrefixInput { prefix: Some(prefix), input })? >> 2;
				if x > 0b0011_1111_1111_1111 && x < 65536 {
					x as u16
				} else {
					return Err(U16_OUT_OF_RANGE.into());
				}
			},
			_ => return Err("unexpected prefix decoding Compact<u16>".into()),
		}))
	}
}

impl DecodeWithMemTracking for Compact<u16> {}

impl Decode for Compact<u32> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let prefix = input.read_byte()?;
		Ok(Compact(match prefix % 4 {
			0 => u32::from(prefix) >> 2,
			1 => {
				let x = u16::decode(&mut PrefixInput { prefix: Some(prefix), input })? >> 2;
				if x > 0b0011_1111 && x <= 0b0011_1111_1111_1111 {
					u32::from(x)
				} else {
					return Err(U32_OUT_OF_RANGE.into());
				}
			},
			2 => {
				let x = u32::decode(&mut PrefixInput { prefix: Some(prefix), input })? >> 2;
				if x > 0b0011_1111_1111_1111 && x <= u32::MAX >> 2 {
					x
				} else {
					return Err(U32_OUT_OF_RANGE.into());
				}
			},
			3 => {
				if prefix >> 2 == 0 {
					// just 4 bytes. ok.
					let x = u32::decode(input)?;
					if x > u32::MAX >> 2 {
						x
					} else {
						return Err(U32_OUT_OF_RANGE.into());
					}
				} else {
					// Out of range for a 32-bit quantity.
					return Err(U32_OUT_OF_RANGE.into());
				}
			},
			_ => unreachable!(),
		}))
	}
}

impl DecodeWithMemTracking for Compact<u32> {}

impl Decode for Compact<u64> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let prefix = input.read_byte()?;
		Ok(Compact(match prefix % 4 {
			0 => u64::from(prefix) >> 2,
			1 => {
				let x = u16::decode(&mut PrefixInput { prefix: Some(prefix), input })? >> 2;
				if x > 0b0011_1111 && x <= 0b0011_1111_1111_1111 {
					u64::from(x)
				} else {
					return Err(U64_OUT_OF_RANGE.into());
				}
			},
			2 => {
				let x = u32::decode(&mut PrefixInput { prefix: Some(prefix), input })? >> 2;
				if x > 0b0011_1111_1111_1111 && x <= u32::MAX >> 2 {
					u64::from(x)
				} else {
					return Err(U64_OUT_OF_RANGE.into());
				}
			},
			3 => match (prefix >> 2) + 4 {
				4 => {
					let x = u32::decode(input)?;
					if x > u32::MAX >> 2 {
						u64::from(x)
					} else {
						return Err(U64_OUT_OF_RANGE.into());
					}
				},
				8 => {
					let x = u64::decode(input)?;
					if x > u64::MAX >> 8 {
						x
					} else {
						return Err(U64_OUT_OF_RANGE.into());
					}
				},
				x if x > 8 => return Err("unexpected prefix decoding Compact<u64>".into()),
				bytes_needed => {
					let mut res = 0;
					for i in 0..bytes_needed {
						res |= u64::from(input.read_byte()?) << (i * 8);
					}
					if res > u64::MAX >> ((8 - bytes_needed + 1) * 8) {
						res
					} else {
						return Err(U64_OUT_OF_RANGE.into());
					}
				},
			},
			_ => unreachable!(),
		}))
	}
}

impl DecodeWithMemTracking for Compact<u64> {}

impl Decode for Compact<u128> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let prefix = input.read_byte()?;
		Ok(Compact(match prefix % 4 {
			0 => u128::from(prefix) >> 2,
			1 => {
				let x = u16::decode(&mut PrefixInput { prefix: Some(prefix), input })? >> 2;
				if x > 0b0011_1111 && x <= 0b0011_1111_1111_1111 {
					u128::from(x)
				} else {
					return Err(U128_OUT_OF_RANGE.into());
				}
			},
			2 => {
				let x = u32::decode(&mut PrefixInput { prefix: Some(prefix), input })? >> 2;
				if x > 0b0011_1111_1111_1111 && x <= u32::MAX >> 2 {
					u128::from(x)
				} else {
					return Err(U128_OUT_OF_RANGE.into());
				}
			},
			3 => match (prefix >> 2) + 4 {
				4 => {
					let x = u32::decode(input)?;
					if x > u32::MAX >> 2 {
						u128::from(x)
					} else {
						return Err(U128_OUT_OF_RANGE.into());
					}
				},
				8 => {
					let x = u64::decode(input)?;
					if x > u64::MAX >> 8 {
						u128::from(x)
					} else {
						return Err(U128_OUT_OF_RANGE.into());
					}
				},
				16 => {
					let x = u128::decode(input)?;
					if x > u128::MAX >> 8 {
						x
					} else {
						return Err(U128_OUT_OF_RANGE.into());
					}
				},
				x if x > 16 => return Err("unexpected prefix decoding Compact<u128>".into()),
				bytes_needed => {
					let mut res = 0;
					for i in 0..bytes_needed {
						res |= u128::from(input.read_byte()?) << (i * 8);
					}
					if res > u128::MAX >> ((16 - bytes_needed + 1) * 8) {
						res
					} else {
						return Err(U128_OUT_OF_RANGE.into());
					}
				},
			},
			_ => unreachable!(),
		}))
	}
}

impl DecodeWithMemTracking for Compact<u128> {}

#[cfg(test)]
mod tests {
	use super::*;

	#[test]
	fn prefix_input_empty_read_unchanged() {
		let mut input = PrefixInput { prefix: Some(1), input: &mut &vec![2, 3, 4][..] };
		assert_eq!(input.remaining_len(), Ok(Some(4)));
		let mut empty_buf = [];
		assert_eq!(input.read(&mut empty_buf[..]), Ok(()));
		assert_eq!(input.remaining_len(), Ok(Some(4)));
		assert_eq!(input.read_byte(), Ok(1));
	}

	#[test]
	fn compact_128_encoding_works() {
		let tests = [
			(0u128, 1usize),
			(63, 1),
			(64, 2),
			(16383, 2),
			(16384, 4),
			(1073741823, 4),
			(1073741824, 5),
			((1 << 32) - 1, 5),
			(1 << 32, 6),
			(1 << 40, 7),
			(1 << 48, 8),
			((1 << 56) - 1, 8),
			(1 << 56, 9),
			((1 << 64) - 1, 9),
			(1 << 64, 10),
			(1 << 72, 11),
			(1 << 80, 12),
			(1 << 88, 13),
			(1 << 96, 14),
			(1 << 104, 15),
			(1 << 112, 16),
			((1 << 120) - 1, 16),
			(1 << 120, 17),
			(u128::MAX, 17),
		];
		for &(n, l) in &tests {
			let encoded = Compact(n).encode();
			assert_eq!(encoded.len(), l);
			assert_eq!(Compact::compact_len(&n), l);
			assert_eq!(<Compact<u128>>::decode(&mut &encoded[..]).unwrap().0, n);
		}
	}

	#[test]
	fn compact_64_encoding_works() {
		let tests = [
			(0u64, 1usize),
			(63, 1),
			(64, 2),
			(16383, 2),
			(16384, 4),
			(1073741823, 4),
			(1073741824, 5),
			((1 << 32) - 1, 5),
			(1 << 32, 6),
			(1 << 40, 7),
			(1 << 48, 8),
			((1 << 56) - 1, 8),
			(1 << 56, 9),
			(u64::MAX, 9),
		];
		for &(n, l) in &tests {
			let encoded = Compact(n).encode();
			assert_eq!(encoded.len(), l);
			assert_eq!(Compact::compact_len(&n), l);
			assert_eq!(<Compact<u64>>::decode(&mut &encoded[..]).unwrap().0, n);
		}
	}

	#[test]
	fn compact_32_encoding_works() {
		let tests = [
			(0u32, 1usize),
			(63, 1),
			(64, 2),
			(16383, 2),
			(16384, 4),
			(1073741823, 4),
			(1073741824, 5),
			(u32::MAX, 5),
		];
		for &(n, l) in &tests {
			let encoded = Compact(n).encode();
			assert_eq!(encoded.len(), l);
			assert_eq!(Compact::compact_len(&n), l);
			assert_eq!(<Compact<u32>>::decode(&mut &encoded[..]).unwrap().0, n);
		}
	}

	#[test]
	fn compact_16_encoding_works() {
		let tests = [(0u16, 1usize), (63, 1), (64, 2), (16383, 2), (16384, 4), (65535, 4)];
		for &(n, l) in &tests {
			let encoded = Compact(n).encode();
			assert_eq!(encoded.len(), l);
			assert_eq!(Compact::compact_len(&n), l);
			assert_eq!(<Compact<u16>>::decode(&mut &encoded[..]).unwrap().0, n);
		}
		assert!(<Compact<u16>>::decode(&mut &Compact(65536u32).encode()[..]).is_err());
	}

	#[test]
	fn compact_8_encoding_works() {
		let tests = [(0u8, 1usize), (63, 1), (64, 2), (255, 2)];
		for &(n, l) in &tests {
			let encoded = Compact(n).encode();
			assert_eq!(encoded.len(), l);
			assert_eq!(Compact::compact_len(&n), l);
			assert_eq!(<Compact<u8>>::decode(&mut &encoded[..]).unwrap().0, n);
		}
		assert!(<Compact<u8>>::decode(&mut &Compact(256u32).encode()[..]).is_err());
	}

	fn hexify(bytes: &[u8]) -> String {
		bytes
			.iter()
			.map(|ref b| format!("{:02x}", b))
			.collect::<Vec<String>>()
			.join(" ")
	}

	#[test]
	fn compact_integers_encoded_as_expected() {
		let tests = [
			(0u64, "00"),
			(63, "fc"),
			(64, "01 01"),
			(16383, "fd ff"),
			(16384, "02 00 01 00"),
			(1073741823, "fe ff ff ff"),
			(1073741824, "03 00 00 00 40"),
			((1 << 32) - 1, "03 ff ff ff ff"),
			(1 << 32, "07 00 00 00 00 01"),
			(1 << 40, "0b 00 00 00 00 00 01"),
			(1 << 48, "0f 00 00 00 00 00 00 01"),
			((1 << 56) - 1, "0f ff ff ff ff ff ff ff"),
			(1 << 56, "13 00 00 00 00 00 00 00 01"),
			(u64::MAX, "13 ff ff ff ff ff ff ff ff"),
		];
		for &(n, s) in &tests {
			// Verify u64 encoding
			let encoded = Compact(n).encode();
			assert_eq!(hexify(&encoded), s);
			assert_eq!(<Compact<u64>>::decode(&mut &encoded[..]).unwrap().0, n);

			// Verify encodings for lower-size uints are compatible with u64 encoding
			if n <= u32::MAX as u64 {
				assert_eq!(<Compact<u32>>::decode(&mut &encoded[..]).unwrap().0, n as u32);
				let encoded = Compact(n as u32).encode();
				assert_eq!(hexify(&encoded), s);
				assert_eq!(<Compact<u64>>::decode(&mut &encoded[..]).unwrap().0, n);
			}
			if n <= u16::MAX as u64 {
				assert_eq!(<Compact<u16>>::decode(&mut &encoded[..]).unwrap().0, n as u16);
				let encoded = Compact(n as u16).encode();
				assert_eq!(hexify(&encoded), s);
				assert_eq!(<Compact<u64>>::decode(&mut &encoded[..]).unwrap().0, n);
			}
			if n <= u8::MAX as u64 {
				assert_eq!(<Compact<u8>>::decode(&mut &encoded[..]).unwrap().0, n as u8);
				let encoded = Compact(n as u8).encode();
				assert_eq!(hexify(&encoded), s);
				assert_eq!(<Compact<u64>>::decode(&mut &encoded[..]).unwrap().0, n);
			}
		}
	}

	#[cfg_attr(feature = "std", derive(Serialize, Deserialize, Debug))]
	#[derive(PartialEq, Eq, Clone)]
	struct Wrapper(u8);

	impl CompactAs for Wrapper {
		type As = u8;
		fn encode_as(&self) -> &u8 {
			&self.0
		}
		fn decode_from(x: u8) -> Result<Wrapper, Error> {
			Ok(Wrapper(x))
		}
	}

	impl From<Compact<Wrapper>> for Wrapper {
		fn from(x: Compact<Wrapper>) -> Wrapper {
			x.0
		}
	}

	#[test]
	fn compact_as_8_encoding_works() {
		let tests = [(0u8, 1usize), (63, 1), (64, 2), (255, 2)];
		for &(n, l) in &tests {
			let compact: Compact<Wrapper> = Wrapper(n).into();
			let encoded = compact.encode();
			assert_eq!(encoded.len(), l);
			assert_eq!(Compact::compact_len(&n), l);
			let decoded = <Compact<Wrapper>>::decode(&mut &encoded[..]).unwrap();
			let wrapper: Wrapper = decoded.into();
			assert_eq!(wrapper, Wrapper(n));
		}
	}

	struct WithCompact<T: HasCompact> {
		_data: T,
	}

	#[test]
	fn compact_as_has_compact() {
		let _data = WithCompact { _data: Wrapper(1) };
	}

	#[test]
	fn compact_using_encoded_arrayvec_size() {
		Compact(u8::MAX).using_encoded(|_| {});
		Compact(u16::MAX).using_encoded(|_| {});
		Compact(u32::MAX).using_encoded(|_| {});
		Compact(u64::MAX).using_encoded(|_| {});
		Compact(u128::MAX).using_encoded(|_| {});

		CompactRef(&u8::MAX).using_encoded(|_| {});
		CompactRef(&u16::MAX).using_encoded(|_| {});
		CompactRef(&u32::MAX).using_encoded(|_| {});
		CompactRef(&u64::MAX).using_encoded(|_| {});
		CompactRef(&u128::MAX).using_encoded(|_| {});
	}

	#[test]
	#[should_panic]
	fn array_vec_output_oob() {
		let mut v = ArrayVecWrapper(ArrayVec::<u8, 4>::new());
		v.write(&[1, 2, 3, 4, 5]);
	}

	#[test]
	fn array_vec_output() {
		let mut v = ArrayVecWrapper(ArrayVec::<u8, 4>::new());
		v.write(&[1, 2, 3, 4]);
	}

	macro_rules! check_bound {
		( $m:expr, $ty:ty, $typ1:ty, [ $(($ty2:ty, $ty2_err:expr)),* ]) => {
			$(
				check_bound!($m, $ty, $typ1, $ty2, $ty2_err);
			)*
		};
		( $m:expr, $ty:ty, $typ1:ty, $ty2:ty, $ty2_err:expr) => {
			let enc = ((<$ty>::MAX >> 2) as $typ1 << 2) | $m;
			assert_eq!(Compact::<$ty2>::decode(&mut &enc.to_le_bytes()[..]),
				Err($ty2_err.into()));
		};
	}
	macro_rules! check_bound_u32 {
		( [ $(($ty2:ty, $ty2_err:expr)),* ]) => {
			$(
				check_bound_u32!($ty2, $ty2_err);
			)*
		};
		( $ty2:ty, $ty2_err:expr ) => {
			assert_eq!(Compact::<$ty2>::decode(&mut &[0b11, 0xff, 0xff, 0xff, 0xff >> 2][..]),
				Err($ty2_err.into()));
		};
	}
	macro_rules! check_bound_high {
		( $m:expr, [ $(($ty2:ty, $ty2_err:expr)),* ]) => {
			$(
				check_bound_high!($m, $ty2, $ty2_err);
			)*
		};
		( $s:expr, $ty2:ty, $ty2_err:expr) => {
			let mut dest = Vec::new();
			dest.push(0b11 + (($s - 4) << 2) as u8);
			for _ in 0..($s - 1) {
				dest.push(u8::MAX);
			}
			dest.push(0);
			assert_eq!(Compact::<$ty2>::decode(&mut &dest[..]),
				Err($ty2_err.into()));
		};
	}

	#[test]
	fn compact_u64_test() {
		for a in [
			u64::MAX,
			u64::MAX - 1,
			u64::MAX << 8,
			(u64::MAX << 8) - 1,
			u64::MAX << 16,
			(u64::MAX << 16) - 1,
		]
		.iter()
		{
			let e = Compact::<u64>::encode(&Compact(*a));
			let d = Compact::<u64>::decode(&mut &e[..]).unwrap().0;
			assert_eq!(*a, d);
		}
	}

	#[test]
	fn compact_u128_test() {
		for a in [u64::MAX as u128, (u64::MAX - 10) as u128, u128::MAX, u128::MAX - 10].iter() {
			let e = Compact::<u128>::encode(&Compact(*a));
			let d = Compact::<u128>::decode(&mut &e[..]).unwrap().0;
			assert_eq!(*a, d);
		}
	}

	#[test]
	fn should_avoid_overlapping_definition() {
		check_bound!(
			0b01,
			u8,
			u16,
			[
				(u8, U8_OUT_OF_RANGE),
				(u16, U16_OUT_OF_RANGE),
				(u32, U32_OUT_OF_RANGE),
				(u64, U64_OUT_OF_RANGE),
				(u128, U128_OUT_OF_RANGE)
			]
		);
		check_bound!(
			0b10,
			u16,
			u32,
			[
				(u16, U16_OUT_OF_RANGE),
				(u32, U32_OUT_OF_RANGE),
				(u64, U64_OUT_OF_RANGE),
				(u128, U128_OUT_OF_RANGE)
			]
		);
		check_bound_u32!([
			(u32, U32_OUT_OF_RANGE),
			(u64, U64_OUT_OF_RANGE),
			(u128, U128_OUT_OF_RANGE)
		]);
		for i in 5..=8 {
			check_bound_high!(i, [(u64, U64_OUT_OF_RANGE), (u128, U128_OUT_OF_RANGE)]);
		}
		for i in 8..=16 {
			check_bound_high!(i, [(u128, U128_OUT_OF_RANGE)]);
		}
	}

	macro_rules! quick_check_roundtrip {
		( $( $ty:ty : $test:ident ),* ) => {
			$(
				quickcheck::quickcheck! {
					fn $test(v: $ty) -> bool {
						let encoded = Compact(v).encode();
						let deencoded = <Compact<$ty>>::decode(&mut &encoded[..]).unwrap().0;

						v == deencoded
					}
				}
			)*
		}
	}

	quick_check_roundtrip! {
		u8: u8_roundtrip,
		u16: u16_roundtrip,
		u32 : u32_roundtrip,
		u64 : u64_roundtrip,
		u128 : u128_roundtrip
	}
}
