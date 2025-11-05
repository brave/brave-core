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

//! Contains the [`DecodeFinished`] type, sequestered into its own module
//! to prevent its direct construction in the whole crate.

use core::marker::PhantomData;

/// A zero-sized type signifying that the decoding finished.
///
/// To be used in [`Decode::decode_into`] to allow the implementation to explicitly
/// assert that the `MaybeUninit` passed into that function was properly initialized.
pub struct DecodeFinished(PhantomData<*const ()>);

impl DecodeFinished {
	/// Assert that the decoding has finished.
	///
	/// # Safety
	///
	/// Should be used in [`Decode::decode_into`] to signify that
	/// the `MaybeUninit` passed into that function was properly initialized.
	#[inline]
	pub unsafe fn assert_decoding_finished() -> DecodeFinished {
		DecodeFinished(PhantomData)
	}
}
