// Copyright 2017, 2018 Parity Technologies
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

use crate::{Decode, Error, Input};

/// The error message returned when depth limit is reached.
const DECODE_MAX_DEPTH_MSG: &str = "Maximum recursion depth reached when decoding";

/// Extension trait to [`Decode`] for decoding with a maximum recursion depth.
pub trait DecodeLimit: Sized {
	/// Decode `Self` with the given maximum recursion depth and advance `input` by the number of
	/// bytes consumed.
	///
	/// If `limit` is hit, an error is returned.
	fn decode_with_depth_limit<I: Input>(limit: u32, input: &mut I) -> Result<Self, Error>;

	/// Decode `Self` and consume all of the given input data.
	///
	/// If not all data is consumed or `limit` is hit, an error is returned.
	fn decode_all_with_depth_limit(limit: u32, input: &mut &[u8]) -> Result<Self, Error>;
}

struct DepthTrackingInput<'a, I> {
	input: &'a mut I,
	depth: u32,
	max_depth: u32,
}

impl<I: Input> Input for DepthTrackingInput<'_, I> {
	fn remaining_len(&mut self) -> Result<Option<usize>, Error> {
		self.input.remaining_len()
	}

	fn read(&mut self, into: &mut [u8]) -> Result<(), Error> {
		self.input.read(into)
	}

	fn read_byte(&mut self) -> Result<u8, Error> {
		self.input.read_byte()
	}

	fn descend_ref(&mut self) -> Result<(), Error> {
		self.input.descend_ref()?;
		self.depth += 1;
		if self.depth > self.max_depth {
			Err(DECODE_MAX_DEPTH_MSG.into())
		} else {
			Ok(())
		}
	}

	fn ascend_ref(&mut self) {
		self.input.ascend_ref();
		self.depth -= 1;
	}

	fn on_before_alloc_mem(&mut self, size: usize) -> Result<(), Error> {
		self.input.on_before_alloc_mem(size)
	}
}

impl<T: Decode> DecodeLimit for T {
	fn decode_all_with_depth_limit(limit: u32, input: &mut &[u8]) -> Result<Self, Error> {
		let t = <Self as DecodeLimit>::decode_with_depth_limit(limit, input)?;

		if input.is_empty() {
			Ok(t)
		} else {
			Err(crate::decode_all::DECODE_ALL_ERR_MSG.into())
		}
	}

	fn decode_with_depth_limit<I: Input>(limit: u32, input: &mut I) -> Result<Self, Error> {
		let mut input = DepthTrackingInput { input, depth: 0, max_depth: limit };
		T::decode(&mut input)
	}
}

#[cfg(test)]
mod tests {
	use super::*;
	use crate::Encode;

	#[test]
	fn decode_limit_works() {
		type NestedVec = Vec<Vec<Vec<Vec<u8>>>>;
		let nested: NestedVec = vec![vec![vec![vec![1]]]];
		let encoded = nested.encode();

		let decoded = NestedVec::decode_with_depth_limit(3, &mut encoded.as_slice()).unwrap();
		assert_eq!(decoded, nested);
		assert!(NestedVec::decode_with_depth_limit(2, &mut encoded.as_slice()).is_err());
	}

	#[test]
	fn decode_limit_advances_input() {
		type NestedVec = Vec<Vec<Vec<Vec<u8>>>>;
		let nested: NestedVec = vec![vec![vec![vec![1]]]];
		let encoded = nested.encode();
		let encoded_slice = &mut encoded.as_slice();

		let decoded = Vec::<u8>::decode_with_depth_limit(1, encoded_slice).unwrap();
		assert_eq!(decoded, vec![4]);
		assert!(NestedVec::decode_with_depth_limit(3, encoded_slice).is_err());
	}

	#[test]
	fn decode_all_with_limit_advances_input() {
		type NestedVec = Vec<Vec<Vec<Vec<u8>>>>;
		let nested: NestedVec = vec![vec![vec![vec![1]]]];
		let mut encoded = NestedVec::encode(&nested);

		let decoded = NestedVec::decode_all_with_depth_limit(3, &mut encoded.as_slice()).unwrap();
		assert_eq!(decoded, nested);

		encoded.extend(&[1, 2, 3, 4, 5, 6]);
		assert_eq!(
			NestedVec::decode_all_with_depth_limit(3, &mut encoded.as_slice())
				.unwrap_err()
				.to_string(),
			"Input buffer has still data left after decoding!",
		);
	}
}
