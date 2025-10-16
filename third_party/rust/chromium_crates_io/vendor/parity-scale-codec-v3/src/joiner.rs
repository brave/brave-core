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

//! Trait

use core::iter::Extend;

use crate::codec::Codec;

/// Trait to allow itself to be serialised into a value which can be extended
/// by bytes.
pub trait Joiner {
	/// Append encoding of value to `Self`.
	fn and<V: Codec + Sized>(self, value: &V) -> Self;
}

impl<T> Joiner for T
where
	T: for<'a> Extend<&'a u8>,
{
	fn and<V: Codec + Sized>(mut self, value: &V) -> Self {
		value.using_encoded(|s| self.extend(s));
		self
	}
}
