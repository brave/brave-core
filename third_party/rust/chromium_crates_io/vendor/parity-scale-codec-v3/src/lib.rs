// Copyright 2017-2021 Parity Technologies
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

#![cfg_attr(all(test, nightly), feature(allocator_api))]
#![cfg_attr(all(test, nightly), feature(btreemap_alloc))]
#![doc = include_str!("../README.md")]
#![warn(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
#[macro_use]
#[doc(hidden)]
pub extern crate alloc;

#[cfg(feature = "derive")]
#[allow(unused_imports)]
#[macro_use]
extern crate parity_scale_codec_derive;

#[cfg(all(feature = "std", test))]
#[macro_use]
extern crate serde_derive;

#[cfg(feature = "derive")]
pub use parity_scale_codec_derive::*;

#[cfg(feature = "std")]
#[doc(hidden)]
pub mod alloc {
	pub use std::{alloc, borrow, boxed, collections, rc, string, sync, vec};
}

/// Private module to reexport items used by derive macros.
// We don't feature gate this module with `derive` to avoid compilation error when
// `parity-scale-codec-derive` is used on its own and this crate doesn't have the feature enabled.
#[doc(hidden)]
pub mod __private {
	pub use const_format::concatcp;
}

#[cfg(feature = "bit-vec")]
mod bit_vec;
mod btree_utils;
mod codec;
mod compact;
#[cfg(feature = "max-encoded-len")]
mod const_encoded_len;
mod counted_input;
mod decode_all;
mod decode_finished;
mod depth_limit;
mod encode_append;
mod encode_like;
mod error;
#[cfg(feature = "generic-array")]
mod generic_array;
mod joiner;
mod keyedvec;
#[cfg(feature = "max-encoded-len")]
mod max_encoded_len;
mod mem_tracking;

#[cfg(feature = "std")]
pub use self::codec::IoReader;
pub use self::{
	codec::{
		decode_vec_with_len, Codec, Decode, DecodeLength, Encode, EncodeAsRef, FullCodec,
		FullEncode, Input, OptionBool, Output, WrapperTypeDecode, WrapperTypeEncode,
	},
	compact::{Compact, CompactAs, CompactLen, CompactRef, HasCompact},
	counted_input::CountedInput,
	decode_all::DecodeAll,
	decode_finished::DecodeFinished,
	depth_limit::DecodeLimit,
	encode_append::EncodeAppend,
	encode_like::{EncodeLike, Ref},
	error::Error,
	joiner::Joiner,
	keyedvec::KeyedVec,
	mem_tracking::{DecodeWithMemLimit, DecodeWithMemTracking, MemTrackingInput},
};
#[cfg(feature = "max-encoded-len")]
pub use const_encoded_len::ConstEncodedLen;
#[cfg(feature = "max-encoded-len")]
pub use max_encoded_len::MaxEncodedLen;

/// Derive macro for [`MaxEncodedLen`][max_encoded_len::MaxEncodedLen].
///
/// # Examples
///
/// ```
/// # use parity_scale_codec::{Encode, MaxEncodedLen};
/// #[derive(Encode, MaxEncodedLen)]
/// struct Example;
/// ```
///
/// ```
/// # use parity_scale_codec::{Encode, MaxEncodedLen};
/// #[derive(Encode, MaxEncodedLen)]
/// struct TupleStruct(u8, u32);
///
/// assert_eq!(TupleStruct::max_encoded_len(), u8::max_encoded_len() + u32::max_encoded_len());
/// ```
///
/// ```
/// # use parity_scale_codec::{Encode, MaxEncodedLen};
/// #[derive(Encode, MaxEncodedLen)]
/// enum GenericEnum<T> {
///     A,
///     B(T),
/// }
///
/// assert_eq!(GenericEnum::<u8>::max_encoded_len(), u8::max_encoded_len() + u8::max_encoded_len());
/// assert_eq!(GenericEnum::<u128>::max_encoded_len(), u8::max_encoded_len() + u128::max_encoded_len());
/// ```
///
/// # Within other macros
///
/// Sometimes the `MaxEncodedLen` trait and macro are used within another macro, and it can't
/// be guaranteed that the `parity_scale_codec` module is available at the call site. In that
/// case, the macro should reexport the `parity_scale_codec` module and specify the path to the
/// reexport:
///
/// ```ignore
/// pub use parity_scale_codec as codec;
///
/// #[derive(Encode, MaxEncodedLen)]
/// #[codec(crate = $crate::codec)]
/// struct Example;
/// ```
#[cfg(all(feature = "derive", feature = "max-encoded-len"))]
pub use parity_scale_codec_derive::MaxEncodedLen;

#[cfg(feature = "bytes")]
pub use self::codec::decode_from_bytes;
