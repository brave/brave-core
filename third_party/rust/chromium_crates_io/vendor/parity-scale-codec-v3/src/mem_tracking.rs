// Copyright (C) Parity Technologies (UK) Ltd.
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

use crate::{Decode, Error, Input};
use impl_trait_for_tuples::impl_for_tuples;

/// Marker trait used for identifying types that call the [`Input::on_before_alloc_mem`] hook
/// while decoding.
pub trait DecodeWithMemTracking: Decode {}

const DECODE_OOM_MSG: &str = "Heap memory limit exceeded while decoding";

#[impl_for_tuples(18)]
impl DecodeWithMemTracking for Tuple {}

/// `Input` implementation that can be used for limiting the heap memory usage while decoding.
pub struct MemTrackingInput<'a, I> {
	input: &'a mut I,
	used_mem: usize,
	mem_limit: usize,
}

impl<'a, I: Input> MemTrackingInput<'a, I> {
	/// Create a new instance of `MemTrackingInput`.
	pub fn new(input: &'a mut I, mem_limit: usize) -> Self {
		Self { input, used_mem: 0, mem_limit }
	}

	/// Get the `used_mem` field.
	pub fn used_mem(&self) -> usize {
		self.used_mem
	}
}

impl<I: Input> Input for MemTrackingInput<'_, I> {
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
		self.input.descend_ref()
	}

	fn ascend_ref(&mut self) {
		self.input.ascend_ref()
	}

	fn on_before_alloc_mem(&mut self, size: usize) -> Result<(), Error> {
		self.input.on_before_alloc_mem(size)?;

		self.used_mem = self.used_mem.saturating_add(size);
		if self.used_mem >= self.mem_limit {
			return Err(DECODE_OOM_MSG.into());
		}

		Ok(())
	}
}

/// Extension trait to [`Decode`] for decoding with a maximum memory limit.
pub trait DecodeWithMemLimit: DecodeWithMemTracking {
	/// Decode `Self` with the given maximum memory limit and advance `input` by the number of
	/// bytes consumed.
	///
	/// If `mem_limit` is hit, an error is returned.
	fn decode_with_mem_limit<I: Input>(input: &mut I, mem_limit: usize) -> Result<Self, Error>;
}

impl<T> DecodeWithMemLimit for T
where
	T: DecodeWithMemTracking,
{
	fn decode_with_mem_limit<I: Input>(input: &mut I, mem_limit: usize) -> Result<Self, Error> {
		let mut input = MemTrackingInput::new(input, mem_limit);
		T::decode(&mut input)
	}
}
