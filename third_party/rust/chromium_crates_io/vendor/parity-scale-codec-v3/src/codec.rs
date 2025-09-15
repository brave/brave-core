// Copyright 2017-2018 Parity Technologies
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

//! Serialization.

use core::{
	convert::TryFrom,
	fmt,
	iter::FromIterator,
	marker::PhantomData,
	mem,
	mem::MaybeUninit,
	num::{
		NonZeroI128, NonZeroI16, NonZeroI32, NonZeroI64, NonZeroI8, NonZeroU128, NonZeroU16,
		NonZeroU32, NonZeroU64, NonZeroU8,
	},
	ops::{Deref, Range, RangeInclusive},
	time::Duration,
};

use byte_slice_cast::{AsByteSlice, AsMutByteSlice, ToMutByteSlice};

#[cfg(target_has_atomic = "ptr")]
use crate::alloc::sync::Arc;
use crate::{
	alloc::{
		borrow::{Cow, ToOwned},
		boxed::Box,
		collections::{BTreeMap, BTreeSet, BinaryHeap, LinkedList, VecDeque},
		rc::Rc,
		string::String,
		vec::Vec,
	},
	compact::Compact,
	encode_like::EncodeLike,
	mem_tracking::DecodeWithMemTracking,
	DecodeFinished, Error,
};

pub(crate) const INITIAL_PREALLOCATION: usize = 16 * 1024;
const A_BILLION: u32 = 1_000_000_000;

/// Trait that allows reading of data into a slice.
pub trait Input {
	/// Should return the remaining length of the input data. If no information about the input
	/// length is available, `None` should be returned.
	///
	/// The length is used to constrain the preallocation while decoding. Returning a garbage
	/// length can open the doors for a denial of service attack to your application.
	/// Otherwise, returning `None` can decrease the performance of your application.
	fn remaining_len(&mut self) -> Result<Option<usize>, Error>;

	/// Read the exact number of bytes required to fill the given buffer.
	///
	/// Note that this function is similar to `std::io::Read::read_exact` and not
	/// `std::io::Read::read`.
	fn read(&mut self, into: &mut [u8]) -> Result<(), Error>;

	/// Read a single byte from the input.
	fn read_byte(&mut self) -> Result<u8, Error> {
		let mut buf = [0u8];
		self.read(&mut buf[..])?;
		Ok(buf[0])
	}

	/// Descend into nested reference when decoding.
	/// This is called when decoding a new refence-based instance,
	/// such as `Vec` or `Box`. Currently, all such types are
	/// allocated on the heap.
	fn descend_ref(&mut self) -> Result<(), Error> {
		Ok(())
	}

	/// Ascend to previous structure level when decoding.
	/// This is called when decoding reference-based type is finished.
	fn ascend_ref(&mut self) {}

	/// Hook that is called before allocating memory on the heap.
	///
	/// The aim is to get a reasonable approximation of memory usage, especially with variably
	/// sized types like `Vec`s. Depending on the structure, it is acceptable to be off by a bit.
	/// In some cases we might not track the memory used by internal sub-structures, and
	/// also we don't take alignment or memory layouts into account.
	/// But we should always track the memory used by the decoded data inside the type.
	fn on_before_alloc_mem(&mut self, _size: usize) -> Result<(), Error> {
		Ok(())
	}

	/// !INTERNAL USE ONLY!
	///
	/// Decodes a `bytes::Bytes`.
	#[cfg(feature = "bytes")]
	#[doc(hidden)]
	fn scale_internal_decode_bytes(&mut self) -> Result<bytes::Bytes, Error>
	where
		Self: Sized,
	{
		Vec::<u8>::decode(self).map(bytes::Bytes::from)
	}
}

impl Input for &[u8] {
	fn remaining_len(&mut self) -> Result<Option<usize>, Error> {
		Ok(Some(self.len()))
	}

	fn read(&mut self, into: &mut [u8]) -> Result<(), Error> {
		if into.len() > self.len() {
			return Err("Not enough data to fill buffer".into());
		}
		let len = into.len();
		into.copy_from_slice(&self[..len]);
		*self = &self[len..];
		Ok(())
	}
}

#[cfg(feature = "std")]
impl From<std::io::Error> for Error {
	fn from(err: std::io::Error) -> Self {
		use std::io::ErrorKind::*;
		match err.kind() {
			NotFound => "io error: NotFound".into(),
			PermissionDenied => "io error: PermissionDenied".into(),
			ConnectionRefused => "io error: ConnectionRefused".into(),
			ConnectionReset => "io error: ConnectionReset".into(),
			ConnectionAborted => "io error: ConnectionAborted".into(),
			NotConnected => "io error: NotConnected".into(),
			AddrInUse => "io error: AddrInUse".into(),
			AddrNotAvailable => "io error: AddrNotAvailable".into(),
			BrokenPipe => "io error: BrokenPipe".into(),
			AlreadyExists => "io error: AlreadyExists".into(),
			WouldBlock => "io error: WouldBlock".into(),
			InvalidInput => "io error: InvalidInput".into(),
			InvalidData => "io error: InvalidData".into(),
			TimedOut => "io error: TimedOut".into(),
			WriteZero => "io error: WriteZero".into(),
			Interrupted => "io error: Interrupted".into(),
			Other => "io error: Other".into(),
			UnexpectedEof => "io error: UnexpectedEof".into(),
			_ => "io error: Unknown".into(),
		}
	}
}

/// Wrapper that implements Input for any `Read` type.
#[cfg(feature = "std")]
pub struct IoReader<R: std::io::Read>(pub R);

#[cfg(feature = "std")]
impl<R: std::io::Read> Input for IoReader<R> {
	fn remaining_len(&mut self) -> Result<Option<usize>, Error> {
		Ok(None)
	}

	fn read(&mut self, into: &mut [u8]) -> Result<(), Error> {
		self.0.read_exact(into).map_err(Into::into)
	}
}

/// Trait that allows writing of data.
pub trait Output {
	/// Write to the output.
	fn write(&mut self, bytes: &[u8]);

	/// Write a single byte to the output.
	fn push_byte(&mut self, byte: u8) {
		self.write(&[byte]);
	}
}

#[cfg(not(feature = "std"))]
impl Output for Vec<u8> {
	fn write(&mut self, bytes: &[u8]) {
		self.extend_from_slice(bytes)
	}
}

#[cfg(feature = "std")]
impl<W: std::io::Write> Output for W {
	fn write(&mut self, bytes: &[u8]) {
		(self as &mut dyn std::io::Write)
			.write_all(bytes)
			.expect("Codec outputs are infallible");
	}
}

/// !INTERNAL USE ONLY!
///
/// This enum provides type information to optimize encoding/decoding by doing fake specialization.
#[doc(hidden)]
#[non_exhaustive]
pub enum TypeInfo {
	/// Default value of [`Encode::TYPE_INFO`] to not require implementors to set this value in the
	/// trait.
	Unknown,
	U8,
	I8,
	U16,
	I16,
	U32,
	I32,
	U64,
	I64,
	U128,
	I128,
	F32,
	F64,
}

/// Trait that allows zero-copy write of value-references to slices in LE format.
///
/// Implementations should override `using_encoded` for value types and `encode_to` and `size_hint`
/// for allocating types. Wrapper types should override all methods.
pub trait Encode {
	// !INTERNAL USE ONLY!
	// This const helps SCALE to optimize the encoding/decoding by doing fake specialization.
	#[doc(hidden)]
	const TYPE_INFO: TypeInfo = TypeInfo::Unknown;

	/// If possible give a hint of expected size of the encoding.
	///
	/// This method is used inside default implementation of `encode`
	/// to avoid re-allocations.
	fn size_hint(&self) -> usize {
		0
	}

	/// Convert self to a slice and append it to the destination.
	fn encode_to<T: Output + ?Sized>(&self, dest: &mut T) {
		self.using_encoded(|buf| dest.write(buf));
	}

	/// Convert self to an owned vector.
	fn encode(&self) -> Vec<u8> {
		let mut r = Vec::with_capacity(self.size_hint());
		self.encode_to(&mut r);
		r
	}

	/// Convert self to a slice and then invoke the given closure with it.
	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		f(&self.encode())
	}

	/// Calculates the encoded size.
	///
	/// Should be used when the encoded data isn't required.
	///
	/// # Note
	///
	/// This works by using a special [`Output`] that only tracks the size. So, there are no
	/// allocations inside the output. However, this can not prevent allocations that some types are
	/// doing inside their own encoding.
	fn encoded_size(&self) -> usize {
		let mut size_tracker = SizeTracker { written: 0 };
		self.encode_to(&mut size_tracker);
		size_tracker.written
	}
}

// Implements `Output` and only keeps track of the number of written bytes
struct SizeTracker {
	written: usize,
}

impl Output for SizeTracker {
	fn write(&mut self, bytes: &[u8]) {
		self.written += bytes.len();
	}

	fn push_byte(&mut self, _byte: u8) {
		self.written += 1;
	}
}

/// Trait that allows the length of a collection to be read, without having
/// to read and decode the entire elements.
pub trait DecodeLength {
	/// Return the number of elements in `self_encoded`.
	fn len(self_encoded: &[u8]) -> Result<usize, Error>;
}

/// Trait that allows zero-copy read of value-references from slices in LE format.
pub trait Decode: Sized {
	// !INTERNAL USE ONLY!
	// This const helps SCALE to optimize the encoding/decoding by doing fake specialization.
	#[doc(hidden)]
	const TYPE_INFO: TypeInfo = TypeInfo::Unknown;

	/// Attempt to deserialise the value from input.
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error>;

	/// Attempt to deserialize the value from input into a pre-allocated piece of memory.
	///
	/// The default implementation will just call [`Decode::decode`].
	///
	/// # Safety
	///
	/// If this function returns `Ok` then `dst` **must** be properly initialized.
	///
	/// This is enforced by requiring the implementation to return a [`DecodeFinished`]
	/// which can only be created by calling [`DecodeFinished::assert_decoding_finished`] which is
	/// `unsafe`.
	fn decode_into<I: Input>(
		input: &mut I,
		dst: &mut MaybeUninit<Self>,
	) -> Result<DecodeFinished, Error> {
		let value = Self::decode(input)?;
		dst.write(value);

		// SAFETY: We've written the decoded value to `dst` so calling this is safe.
		unsafe { Ok(DecodeFinished::assert_decoding_finished()) }
	}

	/// Attempt to skip the encoded value from input.
	///
	/// The default implementation of this function is just calling [`Decode::decode`].
	/// When possible, an implementation should provide a specialized implementation.
	fn skip<I: Input>(input: &mut I) -> Result<(), Error> {
		Self::decode(input).map(|_| ())
	}

	/// Returns the fixed encoded size of the type.
	///
	/// If it returns `Some(size)` then all possible values of this
	/// type have the given size (in bytes) when encoded.
	///
	/// NOTE: A type with a fixed encoded size may return `None`.
	fn encoded_fixed_size() -> Option<usize> {
		None
	}
}

/// Trait that allows zero-copy read/write of value-references to/from slices in LE format.
pub trait Codec: Decode + Encode {}
impl<S: Decode + Encode> Codec for S {}

/// Trait that bound `EncodeLike` along with `Encode`. Usefull for generic being used in function
/// with `EncodeLike` parameters.
pub trait FullEncode: Encode + EncodeLike {}
impl<S: Encode + EncodeLike> FullEncode for S {}

/// Trait that bound `EncodeLike` along with `Codec`. Usefull for generic being used in function
/// with `EncodeLike` parameters.
pub trait FullCodec: Decode + FullEncode {}
impl<S: Decode + FullEncode> FullCodec for S {}

/// A marker trait for types that wrap other encodable type.
///
/// Such types should not carry any additional information
/// that would require to be encoded, because the encoding
/// is assumed to be the same as the wrapped type.
///
/// The wrapped type that is referred to is the [`Deref::Target`].
pub trait WrapperTypeEncode: Deref {}

impl<T: ?Sized> WrapperTypeEncode for Box<T> {}
impl<T: ?Sized + Encode> EncodeLike for Box<T> {}
impl<T: Encode> EncodeLike<T> for Box<T> {}
impl<T: Encode> EncodeLike<Box<T>> for T {}

impl<T: ?Sized> WrapperTypeEncode for &T {}
impl<T: ?Sized + Encode> EncodeLike for &T {}
impl<T: Encode> EncodeLike<T> for &T {}
impl<T: Encode> EncodeLike<&T> for T {}
impl<T: Encode> EncodeLike<T> for &&T {}
impl<T: Encode> EncodeLike<&&T> for T {}

impl<T: ?Sized> WrapperTypeEncode for &mut T {}
impl<T: ?Sized + Encode> EncodeLike for &mut T {}
impl<T: Encode> EncodeLike<T> for &mut T {}
impl<T: Encode> EncodeLike<&mut T> for T {}

impl<T: ToOwned + ?Sized> WrapperTypeEncode for Cow<'_, T> {}
impl<T: ToOwned + Encode + ?Sized> EncodeLike for Cow<'_, T> {}
impl<T: ToOwned + Encode> EncodeLike<T> for Cow<'_, T> {}
impl<T: ToOwned + Encode> EncodeLike<Cow<'_, T>> for T {}

impl<T: ?Sized> WrapperTypeEncode for Rc<T> {}
impl<T: ?Sized + Encode> EncodeLike for Rc<T> {}
impl<T: Encode> EncodeLike<T> for Rc<T> {}
impl<T: Encode> EncodeLike<Rc<T>> for T {}

impl WrapperTypeEncode for String {}
impl EncodeLike for String {}
impl EncodeLike<&str> for String {}
impl EncodeLike<String> for &str {}

#[cfg(target_has_atomic = "ptr")]
mod atomic_ptr_targets {
	use super::*;
	impl<T: ?Sized> WrapperTypeEncode for Arc<T> {}
	impl<T: ?Sized + Encode> EncodeLike for Arc<T> {}
	impl<T: Encode> EncodeLike<T> for Arc<T> {}
	impl<T: Encode> EncodeLike<Arc<T>> for T {}
}

#[cfg(feature = "bytes")]
mod feature_wrapper_bytes {
	use super::*;
	use bytes::Bytes;

	impl WrapperTypeEncode for Bytes {}
	impl EncodeLike for Bytes {}
	impl EncodeLike<&[u8]> for Bytes {}
	impl EncodeLike<Vec<u8>> for Bytes {}
	impl EncodeLike<Bytes> for &[u8] {}
	impl EncodeLike<Bytes> for Vec<u8> {}
}

#[cfg(feature = "bytes")]
struct BytesCursor {
	bytes: bytes::Bytes,
	position: usize,
}

#[cfg(feature = "bytes")]
impl Input for BytesCursor {
	fn remaining_len(&mut self) -> Result<Option<usize>, Error> {
		Ok(Some(self.bytes.len() - self.position))
	}

	fn read(&mut self, into: &mut [u8]) -> Result<(), Error> {
		if into.len() > self.bytes.len() - self.position {
			return Err("Not enough data to fill buffer".into());
		}

		into.copy_from_slice(&self.bytes[self.position..self.position + into.len()]);
		self.position += into.len();
		Ok(())
	}

	fn scale_internal_decode_bytes(&mut self) -> Result<bytes::Bytes, Error> {
		let length = <Compact<u32>>::decode(self)?.0 as usize;

		bytes::Buf::advance(&mut self.bytes, self.position);
		self.position = 0;

		if length > self.bytes.len() {
			return Err("Not enough data to fill buffer".into());
		}

		self.on_before_alloc_mem(length)?;
		Ok(self.bytes.split_to(length))
	}
}

/// Decodes a given `T` from `Bytes`.
#[cfg(feature = "bytes")]
pub fn decode_from_bytes<T>(bytes: bytes::Bytes) -> Result<T, Error>
where
	T: Decode,
{
	// We could just use implement `Input` for `Bytes` and use `Bytes::split_to`
	// to move the cursor, however doing it this way allows us to prevent an
	// unnecessary allocation when the `T` which is being deserialized doesn't
	// take advantage of the fact that it's being deserialized from `Bytes`.
	//
	// `Bytes` can be cheaply created from a `Vec<u8>`. It is both zero-copy
	// *and* zero-allocation. However once you `.clone()` it or call `split_to()`
	// an extra one-time allocation is triggered where the `Bytes` changes it's internal
	// representation from essentially being a `Box<[u8]>` into being an `Arc<Box<[u8]>>`.
	//
	// If the `T` is `Bytes` or is a structure which contains `Bytes` in it then
	// we don't really care, because this allocation will have to be made anyway.
	//
	// However, if `T` doesn't contain any `Bytes` then this extra allocation is
	// technically unnecessary, and we can avoid it by tracking the position ourselves
	// and treating the underlying `Bytes` as a fancy `&[u8]`.
	let mut input = BytesCursor { bytes, position: 0 };
	T::decode(&mut input)
}

#[cfg(feature = "bytes")]
impl Decode for bytes::Bytes {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		input.scale_internal_decode_bytes()
	}
}

#[cfg(feature = "bytes")]
impl DecodeWithMemTracking for bytes::Bytes {}

impl<T, X> Encode for X
where
	T: Encode + ?Sized,
	X: WrapperTypeEncode<Target = T>,
{
	fn size_hint(&self) -> usize {
		(**self).size_hint()
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		(**self).using_encoded(f)
	}

	fn encode(&self) -> Vec<u8> {
		(**self).encode()
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		(**self).encode_to(dest)
	}
}

/// A marker trait for types that can be created solely from other decodable types.
///
/// The decoding of such type is assumed to be the same as the wrapped type.
pub trait WrapperTypeDecode: Sized {
	/// A wrapped type.
	type Wrapped: Into<Self>;

	// !INTERNAL USE ONLY!
	// This is a used to specialize `decode` for the wrapped type.
	#[doc(hidden)]
	#[inline]
	fn decode_wrapped<I: Input>(input: &mut I) -> Result<Self, Error>
	where
		Self::Wrapped: Decode,
	{
		input.descend_ref()?;
		let result = Ok(Self::Wrapped::decode(input)?.into());
		input.ascend_ref();
		result
	}
}

impl<T> WrapperTypeDecode for Box<T> {
	type Wrapped = T;

	fn decode_wrapped<I: Input>(input: &mut I) -> Result<Self, Error>
	where
		Self::Wrapped: Decode,
	{
		input.descend_ref()?;

		// Placement new is not yet stable, but we can just manually allocate a chunk of memory
		// and convert it to a `Box` ourselves.
		//
		// The explicit types here are written out for clarity.
		//
		// TODO: Use `Box::new_uninit` once that's stable.
		let layout = core::alloc::Layout::new::<MaybeUninit<T>>();

		input.on_before_alloc_mem(layout.size())?;
		let ptr: *mut MaybeUninit<T> = if layout.size() == 0 {
			core::ptr::NonNull::dangling().as_ptr()
		} else {
			// SAFETY: Layout has a non-zero size so calling this is safe.
			let ptr: *mut u8 = unsafe { crate::alloc::alloc::alloc(layout) };

			if ptr.is_null() {
				crate::alloc::alloc::handle_alloc_error(layout);
			}

			ptr.cast()
		};

		// SAFETY: Constructing a `Box` from a piece of memory allocated with `std::alloc::alloc`
		//         is explicitly allowed as long as it was allocated with the global allocator
		//         and the memory layout matches.
		//
		//         Constructing a `Box` from `NonNull::dangling` is also always safe as long
		//         as the underlying type is zero-sized.
		let mut boxed: Box<MaybeUninit<T>> = unsafe { Box::from_raw(ptr) };

		T::decode_into(input, &mut boxed)?;

		// Decoding succeeded, so let's get rid of `MaybeUninit`.
		//
		// TODO: Use `Box::assume_init` once that's stable.
		let ptr: *mut MaybeUninit<T> = Box::into_raw(boxed);
		let ptr: *mut T = ptr.cast();

		// SAFETY: `MaybeUninit` doesn't affect the memory layout, so casting the pointer back
		//         into a `Box` is safe.
		let boxed: Box<T> = unsafe { Box::from_raw(ptr) };

		input.ascend_ref();
		Ok(boxed)
	}
}

impl<T: DecodeWithMemTracking> DecodeWithMemTracking for Box<T> {}

impl<T> WrapperTypeDecode for Rc<T> {
	type Wrapped = T;

	fn decode_wrapped<I: Input>(input: &mut I) -> Result<Self, Error>
	where
		Self::Wrapped: Decode,
	{
		// TODO: This is inefficient; use `Rc::new_uninit` once that's stable.
		Box::<T>::decode(input).map(|output| output.into())
	}
}

// `Rc<T>` uses `Box::<T>::decode()` internally, so it supports `DecodeWithMemTracking`.
impl<T: DecodeWithMemTracking> DecodeWithMemTracking for Rc<T> {}

#[cfg(target_has_atomic = "ptr")]
impl<T> WrapperTypeDecode for Arc<T> {
	type Wrapped = T;

	fn decode_wrapped<I: Input>(input: &mut I) -> Result<Self, Error>
	where
		Self::Wrapped: Decode,
	{
		// TODO: This is inefficient; use `Arc::new_uninit` once that's stable.
		Box::<T>::decode(input).map(|output| output.into())
	}
}

// `Arc<T>` uses `Box::<T>::decode()` internally, so it supports `DecodeWithMemTracking`.
impl<T: DecodeWithMemTracking> DecodeWithMemTracking for Arc<T> {}

impl<T, X> Decode for X
where
	T: Decode + Into<X>,
	X: WrapperTypeDecode<Wrapped = T>,
{
	#[inline]
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		Self::decode_wrapped(input)
	}
}

/// A macro that matches on a [`TypeInfo`] and expands a given macro per variant.
///
/// The first parameter to the given macro will be the type of variant (e.g. `u8`, `u32`, etc.) and
/// other parameters given to this macro.
///
/// The last parameter is the code that should be executed for the `Unknown` type info.
macro_rules! with_type_info {
	( $type_info:expr, $macro:ident $( ( $( $params:ident ),* ) )?, { $( $unknown_variant:tt )* }, ) => {
		match $type_info {
			TypeInfo::U8 => { $macro!(u8 $( $( , $params )* )? ) },
			TypeInfo::I8 => { $macro!(i8 $( $( , $params )* )? ) },
			TypeInfo::U16 => { $macro!(u16 $( $( , $params )* )? ) },
			TypeInfo::I16 => { $macro!(i16 $( $( , $params )* )? ) },
			TypeInfo::U32 => { $macro!(u32 $( $( , $params )* )? ) },
			TypeInfo::I32 => { $macro!(i32 $( $( , $params )* )? ) },
			TypeInfo::U64 => { $macro!(u64 $( $( , $params )* )? ) },
			TypeInfo::I64 => { $macro!(i64 $( $( , $params )* )? ) },
			TypeInfo::U128 => { $macro!(u128 $( $( , $params )* )? ) },
			TypeInfo::I128 => { $macro!(i128 $( $( , $params )* )? ) },
			TypeInfo::Unknown => { $( $unknown_variant )* },
			TypeInfo::F32 => { $macro!(f32 $( $( , $params )* )? ) },
			TypeInfo::F64 => { $macro!(f64 $( $( , $params )* )? ) },
		}
	};
}

/// Something that can be encoded as a reference.
pub trait EncodeAsRef<'a, T: 'a> {
	/// The reference type that is used for encoding.
	type RefType: Encode + From<&'a T>;
}

impl<T: Encode, E: Encode> Encode for Result<T, E> {
	fn size_hint(&self) -> usize {
		1 + match *self {
			Ok(ref t) => t.size_hint(),
			Err(ref t) => t.size_hint(),
		}
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		match *self {
			Ok(ref t) => {
				dest.push_byte(0);
				t.encode_to(dest);
			},
			Err(ref e) => {
				dest.push_byte(1);
				e.encode_to(dest);
			},
		}
	}
}

impl<T, LikeT, E, LikeE> EncodeLike<Result<LikeT, LikeE>> for Result<T, E>
where
	T: EncodeLike<LikeT>,
	LikeT: Encode,
	E: EncodeLike<LikeE>,
	LikeE: Encode,
{
}

impl<T: Decode, E: Decode> Decode for Result<T, E> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		match input
			.read_byte()
			.map_err(|e| e.chain("Could not result variant byte for `Result`"))?
		{
			0 => Ok(Ok(T::decode(input).map_err(|e| e.chain("Could not Decode `Result::Ok(T)`"))?)),
			1 => Ok(Err(
				E::decode(input).map_err(|e| e.chain("Could not decode `Result::Error(E)`"))?
			)),
			_ => Err("unexpected first byte decoding Result".into()),
		}
	}
}

impl<T: DecodeWithMemTracking, E: DecodeWithMemTracking> DecodeWithMemTracking for Result<T, E> {}

/// Shim type because we can't do a specialised implementation for `Option<bool>` directly.
#[derive(Eq, PartialEq, Clone, Copy)]
pub struct OptionBool(pub Option<bool>);

impl fmt::Debug for OptionBool {
	fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
		self.0.fmt(f)
	}
}

impl Encode for OptionBool {
	fn size_hint(&self) -> usize {
		1
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		f(&[match *self {
			OptionBool(None) => 0u8,
			OptionBool(Some(true)) => 1u8,
			OptionBool(Some(false)) => 2u8,
		}])
	}
}

impl EncodeLike for OptionBool {}

impl Decode for OptionBool {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		match input.read_byte()? {
			0 => Ok(OptionBool(None)),
			1 => Ok(OptionBool(Some(true))),
			2 => Ok(OptionBool(Some(false))),
			_ => Err("unexpected first byte decoding OptionBool".into()),
		}
	}
}

impl DecodeWithMemTracking for OptionBool {}

impl<T: EncodeLike<U>, U: Encode> EncodeLike<Option<U>> for Option<T> {}

impl<T: Encode> Encode for Option<T> {
	fn size_hint(&self) -> usize {
		1 + match *self {
			Some(ref t) => t.size_hint(),
			None => 0,
		}
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		match *self {
			Some(ref t) => {
				dest.push_byte(1);
				t.encode_to(dest);
			},
			None => dest.push_byte(0),
		}
	}
}

impl<T: Decode> Decode for Option<T> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		match input
			.read_byte()
			.map_err(|e| e.chain("Could not decode variant byte for `Option`"))?
		{
			0 => Ok(None),
			1 => Ok(Some(
				T::decode(input).map_err(|e| e.chain("Could not decode `Option::Some(T)`"))?,
			)),
			_ => Err("unexpected first byte decoding Option".into()),
		}
	}
}

impl<T: DecodeWithMemTracking> DecodeWithMemTracking for Option<T> {}

macro_rules! impl_for_non_zero {
	( $( $name:ty ),* $(,)? ) => {
		$(
			impl Encode for $name {
				fn size_hint(&self) -> usize {
					self.get().size_hint()
				}

				fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
					self.get().encode_to(dest)
				}

				fn encode(&self) -> Vec<u8> {
					self.get().encode()
				}

				fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
					self.get().using_encoded(f)
				}
			}

			impl EncodeLike for $name {}

			impl Decode for $name {
				fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
					Self::new(Decode::decode(input)?)
						.ok_or_else(|| Error::from("cannot create non-zero number from 0"))
				}
			}

			impl DecodeWithMemTracking for $name {}
		)*
	}
}

/// Encode the slice without prepending the len.
///
/// This is equivalent to encoding all the element one by one, but it is optimized for some types.
pub(crate) fn encode_slice_no_len<T: Encode, W: Output + ?Sized>(slice: &[T], dest: &mut W) {
	macro_rules! encode_to {
		( u8, $slice:ident, $dest:ident ) => {{
			let typed = unsafe { mem::transmute::<&[T], &[u8]>(&$slice[..]) };
			$dest.write(&typed)
		}};
		( i8, $slice:ident, $dest:ident ) => {{
			// `i8` has the same size as `u8`. We can just convert it here and write to the
			// dest buffer directly.
			let typed = unsafe { mem::transmute::<&[T], &[u8]>(&$slice[..]) };
			$dest.write(&typed)
		}};
		( $ty:ty, $slice:ident, $dest:ident ) => {{
			if cfg!(target_endian = "little") {
				let typed = unsafe { mem::transmute::<&[T], &[$ty]>(&$slice[..]) };
				$dest.write(<[$ty] as AsByteSlice<$ty>>::as_byte_slice(typed))
			} else {
				for item in $slice.iter() {
					item.encode_to(dest);
				}
			}
		}};
	}

	with_type_info! {
		<T as Encode>::TYPE_INFO,
		encode_to(slice, dest),
		{
			for item in slice.iter() {
				item.encode_to(dest);
			}
		},
	}
}

impl_for_non_zero! {
	NonZeroI8,
	NonZeroI16,
	NonZeroI32,
	NonZeroI64,
	NonZeroI128,
	NonZeroU8,
	NonZeroU16,
	NonZeroU32,
	NonZeroU64,
	NonZeroU128,
}

impl<T: Encode, const N: usize> Encode for [T; N] {
	fn size_hint(&self) -> usize {
		mem::size_of::<T>() * N
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		encode_slice_no_len(&self[..], dest)
	}
}

const fn calculate_array_bytesize<T, const N: usize>() -> usize {
	struct AssertNotOverflow<T, const N: usize>(PhantomData<T>);

	impl<T, const N: usize> AssertNotOverflow<T, N> {
		const OK: () = assert!(mem::size_of::<T>().checked_mul(N).is_some(), "array size overflow");
	}

	#[allow(clippy::let_unit_value)]
	let () = AssertNotOverflow::<T, N>::OK;
	mem::size_of::<T>() * N
}

impl<T: Decode, const N: usize> Decode for [T; N] {
	#[inline(always)]
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let mut array = MaybeUninit::uninit();
		Self::decode_into(input, &mut array)?;

		// SAFETY: `decode_into` succeeded, so the array is initialized.
		unsafe { Ok(array.assume_init()) }
	}

	fn decode_into<I: Input>(
		input: &mut I,
		dst: &mut MaybeUninit<Self>,
	) -> Result<DecodeFinished, Error> {
		let is_primitive = match <T as Decode>::TYPE_INFO {
			| TypeInfo::U8 | TypeInfo::I8 => true,
			| TypeInfo::U16 |
			TypeInfo::I16 |
			TypeInfo::U32 |
			TypeInfo::I32 |
			TypeInfo::U64 |
			TypeInfo::I64 |
			TypeInfo::U128 |
			TypeInfo::I128 |
			TypeInfo::F32 |
			TypeInfo::F64 => cfg!(target_endian = "little"),
			TypeInfo::Unknown => false,
		};

		if is_primitive {
			// Let's read the array in bulk as that's going to be a lot
			// faster than just reading each element one-by-one.

			let ptr: *mut [T; N] = dst.as_mut_ptr();
			let ptr: *mut u8 = ptr.cast();

			let bytesize = calculate_array_bytesize::<T, N>();

			// TODO: This is potentially slow; it'd be better if `Input` supported
			//       reading directly into uninitialized memory.
			//
			// SAFETY: The pointer is valid and points to a memory `bytesize` bytes big.
			unsafe {
				ptr.write_bytes(0, bytesize);
			}

			// SAFETY: We've zero-initialized everything so creating a slice here is safe.
			let slice: &mut [u8] = unsafe { core::slice::from_raw_parts_mut(ptr, bytesize) };

			input.read(slice)?;

			// SAFETY: We've initialized the whole slice so calling this is safe.
			unsafe {
				return Ok(DecodeFinished::assert_decoding_finished());
			}
		}

		let slice: &mut [MaybeUninit<T>; N] = {
			let ptr: *mut [T; N] = dst.as_mut_ptr();
			let ptr: *mut [MaybeUninit<T>; N] = ptr.cast();
			// SAFETY: Casting `&mut MaybeUninit<[T; N]>` into `&mut [MaybeUninit<T>; N]` is safe.
			unsafe { &mut *ptr }
		};

		/// A wrapper type to make sure the partially read elements are always
		/// dropped in case an error occurs or the underlying `decode` implementation panics.
		struct State<'a, T, const N: usize> {
			count: usize,
			slice: &'a mut [MaybeUninit<T>; N],
		}

		impl<T, const N: usize> Drop for State<'_, T, N> {
			fn drop(&mut self) {
				if !mem::needs_drop::<T>() {
					// If the types don't actually need to be dropped then don't even
					// try to run the loop below.
					//
					// Most likely won't make a difference in release mode, but will
					// make a difference in debug mode.
					return;
				}

				// TODO: Use `MaybeUninit::slice_assume_init_mut` + `core::ptr::drop_in_place`
				//       once `slice_assume_init_mut` is stable.
				for item in &mut self.slice[..self.count] {
					// SAFETY: Each time we've read a new element we incremented `count`,
					//         and we only drop at most `count` elements here,
					//         so all of the elements we drop here are valid.
					unsafe {
						item.assume_init_drop();
					}
				}
			}
		}

		let mut state = State { count: 0, slice };

		while state.count < state.slice.len() {
			T::decode_into(input, &mut state.slice[state.count])?;
			state.count += 1;
		}

		// We've successfully read everything, so disarm the `Drop` impl.
		mem::forget(state);

		// SAFETY: We've initialized the whole slice so calling this is safe.
		unsafe { Ok(DecodeFinished::assert_decoding_finished()) }
	}

	fn skip<I: Input>(input: &mut I) -> Result<(), Error> {
		if Self::encoded_fixed_size().is_some() {
			// Should skip the bytes, but Input does not support skip.
			for _ in 0..N {
				T::skip(input)?;
			}
		} else {
			Self::decode(input)?;
		}
		Ok(())
	}

	fn encoded_fixed_size() -> Option<usize> {
		Some(<T as Decode>::encoded_fixed_size()? * N)
	}
}

impl<T: DecodeWithMemTracking, const N: usize> DecodeWithMemTracking for [T; N] {}

impl<T: EncodeLike<U>, U: Encode, const N: usize> EncodeLike<[U; N]> for [T; N] {}

impl Encode for str {
	fn size_hint(&self) -> usize {
		self.as_bytes().size_hint()
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		self.as_bytes().encode_to(dest)
	}

	fn encode(&self) -> Vec<u8> {
		self.as_bytes().encode()
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		self.as_bytes().using_encoded(f)
	}
}

impl<T: ToOwned + ?Sized> Decode for Cow<'_, T>
where
	<T as ToOwned>::Owned: Decode,
{
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		Ok(Cow::Owned(Decode::decode(input)?))
	}
}

impl<'a, T: ToOwned + ?Sized> DecodeWithMemTracking for Cow<'a, T>
where
	Cow<'a, T>: Decode,
	T::Owned: DecodeWithMemTracking,
{
}

impl<T> EncodeLike for PhantomData<T> {}

impl<T> Encode for PhantomData<T> {
	fn encode_to<W: Output + ?Sized>(&self, _dest: &mut W) {}
}

impl<T> Decode for PhantomData<T> {
	fn decode<I: Input>(_input: &mut I) -> Result<Self, Error> {
		Ok(PhantomData)
	}
}

impl<T> DecodeWithMemTracking for PhantomData<T> where PhantomData<T>: Decode {}

impl Decode for String {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		Self::from_utf8(Vec::decode(input)?).map_err(|_| "Invalid utf8 sequence".into())
	}
}

impl DecodeWithMemTracking for String {}

/// Writes the compact encoding of `len` do `dest`.
pub(crate) fn compact_encode_len_to<W: Output + ?Sized>(
	dest: &mut W,
	len: usize,
) -> Result<(), Error> {
	if len > u32::MAX as usize {
		return Err("Attempted to serialize a collection with too many elements.".into());
	}

	Compact(len as u32).encode_to(dest);
	Ok(())
}

impl<T: Encode> Encode for [T] {
	fn size_hint(&self) -> usize {
		mem::size_of::<u32>() + mem::size_of_val(self)
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		compact_encode_len_to(dest, self.len()).expect("Compact encodes length");

		encode_slice_no_len(self, dest)
	}
}

fn decode_vec_chunked<T, I: Input, F>(
	input: &mut I,
	len: usize,
	mut decode_chunk: F,
) -> Result<Vec<T>, Error>
where
	F: FnMut(&mut I, &mut Vec<T>, usize) -> Result<(), Error>,
{
	const { assert!(INITIAL_PREALLOCATION >= mem::size_of::<T>()) }
	// we have to account for the fact that `mem::size_of::<T>` can be 0 for types like `()`
	// for example.
	let mut chunk_len = INITIAL_PREALLOCATION.checked_div(mem::size_of::<T>()).unwrap_or(1);

	let mut decoded_vec = vec![];
	let mut num_undecoded_items = len;
	while num_undecoded_items > 0 {
		let bounded_chunk_len = chunk_len.min(num_undecoded_items);
		input.on_before_alloc_mem(bounded_chunk_len.saturating_mul(mem::size_of::<T>()))?;
		decoded_vec.reserve_exact(bounded_chunk_len);

		decode_chunk(input, &mut decoded_vec, bounded_chunk_len)?;

		num_undecoded_items -= bounded_chunk_len;
		chunk_len = decoded_vec.len();
	}

	Ok(decoded_vec)
}

/// Create a `Vec<T>` by casting directly from a buffer of read `u8`s
///
/// The encoding of `T` must be equal to its binary representation, and size of `T` must be less
/// or equal to [`INITIAL_PREALLOCATION`].
fn read_vec_from_u8s<T, I>(input: &mut I, len: usize) -> Result<Vec<T>, Error>
where
	T: ToMutByteSlice + Default + Clone,
	I: Input,
{
	let byte_len = len
		.checked_mul(mem::size_of::<T>())
		.ok_or("Item is too big and cannot be allocated")?;

	// Check if there is enough data in the input buffer.
	if let Some(input_len) = input.remaining_len()? {
		if input_len < byte_len {
			return Err("Not enough data to decode vector".into());
		}
	}

	decode_vec_chunked(input, len, |input, decoded_vec, chunk_len| {
		let decoded_vec_len = decoded_vec.len();
		let decoded_vec_size = decoded_vec_len * mem::size_of::<T>();
		unsafe {
			decoded_vec.set_len(decoded_vec_len + chunk_len);
		}

		let bytes_slice = decoded_vec.as_mut_byte_slice();
		input.read(&mut bytes_slice[decoded_vec_size..])
	})
}

fn decode_vec_from_items<T, I>(input: &mut I, len: usize) -> Result<Vec<T>, Error>
where
	T: Decode,
	I: Input,
{
	input.descend_ref()?;
	let vec = decode_vec_chunked(input, len, |input, decoded_vec, chunk_len| {
		for _ in 0..chunk_len {
			decoded_vec.push(T::decode(input)?);
		}

		Ok(())
	})?;
	input.ascend_ref();

	Ok(vec)
}

/// Decode the vec (without a prepended len).
///
/// This is equivalent to decode all elements one by one, but it is optimized in some
/// situation.
pub fn decode_vec_with_len<T: Decode, I: Input>(
	input: &mut I,
	len: usize,
) -> Result<Vec<T>, Error> {
	macro_rules! decode {
		( $ty:ty, $input:ident, $len:ident ) => {{
			if cfg!(target_endian = "little") || mem::size_of::<T>() == 1 {
				let vec = read_vec_from_u8s::<$ty, _>($input, $len)?;
				Ok(unsafe { mem::transmute::<Vec<$ty>, Vec<T>>(vec) })
			} else {
				decode_vec_from_items::<T, _>($input, $len)
			}
		}};
	}

	with_type_info! {
		<T as Decode>::TYPE_INFO,
		decode(input, len),
		{
			decode_vec_from_items::<T, _>(input, len)
		},
	}
}

impl<T> WrapperTypeEncode for Vec<T> {}
impl<T: EncodeLike<U>, U: Encode> EncodeLike<Vec<U>> for Vec<T> {}
impl<T: EncodeLike<U>, U: Encode> EncodeLike<&[U]> for Vec<T> {}
impl<T: EncodeLike<U>, U: Encode> EncodeLike<Vec<U>> for &[T] {}

impl<T: Decode> Decode for Vec<T> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		<Compact<u32>>::decode(input)
			.and_then(move |Compact(len)| decode_vec_with_len(input, len as usize))
	}
}

impl<T: DecodeWithMemTracking> DecodeWithMemTracking for Vec<T> {}

macro_rules! impl_encode_for_collection {
	($(
		$type:ident
		{ $( $generics:ident $( : $decode_additional:ident )? ),* }
		{ $( $type_like_generics:ident ),* }
		{ $( $impl_like_generics:tt )* }
	)*) => {$(
		impl<$( $generics: Encode ),*> Encode for $type<$( $generics, )*> {
			fn size_hint(&self) -> usize {
				mem::size_of::<u32>() + mem::size_of::<($($generics,)*)>().saturating_mul(self.len())
			}

			fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
				compact_encode_len_to(dest, self.len()).expect("Compact encodes length");

				for i in self.iter() {
					i.encode_to(dest);
				}
			}
		}

		impl<$( $impl_like_generics )*> EncodeLike<$type<$( $type_like_generics ),*>>
			for $type<$( $generics ),*> {}
		impl<$( $impl_like_generics )*> EncodeLike<&[( $( $type_like_generics, )* )]>
			for $type<$( $generics ),*> {}
		impl<$( $impl_like_generics )*> EncodeLike<$type<$( $type_like_generics ),*>>
			for &[( $( $generics, )* )] {}
	)*}
}

impl_encode_for_collection! {
	BTreeMap { K: Ord, V } { LikeK, LikeV}
		{ K: EncodeLike<LikeK>, LikeK: Encode, V: EncodeLike<LikeV>, LikeV: Encode }
}

impl<K: Decode + Ord, V: Decode> Decode for BTreeMap<K, V> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		<Compact<u32>>::decode(input).and_then(move |Compact(len)| {
			input.descend_ref()?;
			input.on_before_alloc_mem(super::btree_utils::mem_size_of_btree::<(K, V)>(len))?;
			let result = Result::from_iter((0..len).map(|_| Decode::decode(input)));
			input.ascend_ref();
			result
		})
	}
}

impl<K: DecodeWithMemTracking, V: DecodeWithMemTracking> DecodeWithMemTracking for BTreeMap<K, V> where
	BTreeMap<K, V>: Decode
{
}

impl_encode_for_collection! {
	BTreeSet { T: Ord } { LikeT }
		{ T: EncodeLike<LikeT>, LikeT: Encode }
}

impl<T: Decode + Ord> Decode for BTreeSet<T> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		<Compact<u32>>::decode(input).and_then(move |Compact(len)| {
			input.descend_ref()?;
			input.on_before_alloc_mem(super::btree_utils::mem_size_of_btree::<T>(len))?;
			let result = Result::from_iter((0..len).map(|_| Decode::decode(input)));
			input.ascend_ref();
			result
		})
	}
}
impl<T: DecodeWithMemTracking> DecodeWithMemTracking for BTreeSet<T> where BTreeSet<T>: Decode {}

impl_encode_for_collection! {
	LinkedList { T } { LikeT }
		{ T: EncodeLike<LikeT>, LikeT: Encode }
}

impl<T: Decode> Decode for LinkedList<T> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		<Compact<u32>>::decode(input).and_then(move |Compact(len)| {
			input.descend_ref()?;
			// We account for the size of the `prev` and `next` pointers of each list node,
			// plus the decoded element.
			input.on_before_alloc_mem((len as usize).saturating_mul(mem::size_of::<(
				usize,
				usize,
				T,
			)>()))?;
			let result = Result::from_iter((0..len).map(|_| Decode::decode(input)));
			input.ascend_ref();
			result
		})
	}
}

impl<T: DecodeWithMemTracking> DecodeWithMemTracking for LinkedList<T> where LinkedList<T>: Decode {}

impl_encode_for_collection! {
	BinaryHeap { T: Ord } { LikeT }
		{ T: EncodeLike<LikeT>, LikeT: Encode }
}

impl<T: Decode + Ord> Decode for BinaryHeap<T> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		Ok(Vec::decode(input)?.into())
	}
}
impl<T: DecodeWithMemTracking> DecodeWithMemTracking for BinaryHeap<T> where BinaryHeap<T>: Decode {}

impl<T: Encode> EncodeLike for VecDeque<T> {}
impl<T: EncodeLike<U>, U: Encode> EncodeLike<&[U]> for VecDeque<T> {}
impl<T: EncodeLike<U>, U: Encode> EncodeLike<VecDeque<U>> for &[T] {}
impl<T: EncodeLike<U>, U: Encode> EncodeLike<Vec<U>> for VecDeque<T> {}
impl<T: EncodeLike<U>, U: Encode> EncodeLike<VecDeque<U>> for Vec<T> {}

impl<T: Encode> Encode for VecDeque<T> {
	fn size_hint(&self) -> usize {
		mem::size_of::<u32>() + mem::size_of::<T>() * self.len()
	}

	fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
		compact_encode_len_to(dest, self.len()).expect("Compact encodes length");

		let slices = self.as_slices();
		encode_slice_no_len(slices.0, dest);
		encode_slice_no_len(slices.1, dest);
	}
}

impl<T: Decode> Decode for VecDeque<T> {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		Ok(<Vec<T>>::decode(input)?.into())
	}
}

impl<T: DecodeWithMemTracking> DecodeWithMemTracking for VecDeque<T> {}

impl EncodeLike for () {}

impl Encode for () {
	fn encode_to<W: Output + ?Sized>(&self, _dest: &mut W) {}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		f(&[])
	}

	fn encode(&self) -> Vec<u8> {
		Vec::new()
	}
}

impl Decode for () {
	fn decode<I: Input>(_: &mut I) -> Result<(), Error> {
		Ok(())
	}
}

macro_rules! impl_len {
	( $( $type:ident< $($g:ident),* > ),* ) => { $(
		impl<$($g),*> DecodeLength for $type<$($g),*> {
			fn len(mut self_encoded: &[u8]) -> Result<usize, Error> {
				usize::try_from(u32::from(Compact::<u32>::decode(&mut self_encoded)?))
					.map_err(|_| "Failed convert decoded size into usize.".into())
			}
		}
	)*}
}

// Collection types that support compact decode length.
impl_len!(Vec<T>, BTreeSet<T>, BTreeMap<K, V>, VecDeque<T>, BinaryHeap<T>, LinkedList<T>);

macro_rules! tuple_impl {
	(
		($one:ident, $extra:ident),
	) => {
		impl<$one: Encode> Encode for ($one,) {
			fn size_hint(&self) -> usize {
				self.0.size_hint()
			}

			fn encode_to<T: Output + ?Sized>(&self, dest: &mut T) {
				self.0.encode_to(dest);
			}

			fn encode(&self) -> Vec<u8> {
				self.0.encode()
			}

			fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
				self.0.using_encoded(f)
			}
		}

		impl<$one: Decode> Decode for ($one,) {
			fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
				match $one::decode(input) {
					Err(e) => Err(e),
					Ok($one) => Ok(($one,)),
				}
			}
		}

		impl<$one: DecodeLength> DecodeLength for ($one,) {
			fn len(self_encoded: &[u8]) -> Result<usize, Error> {
				$one::len(self_encoded)
			}
		}

		impl<$one: EncodeLike<$extra>, $extra: Encode> crate::EncodeLike<($extra,)> for ($one,) {}
	};
	(($first:ident, $fextra:ident), $( ( $rest:ident, $rextra:ident ), )+) => {
		impl<$first: Encode, $($rest: Encode),+> Encode for ($first, $($rest),+) {
			fn size_hint(&self) -> usize {
				let (
					ref $first,
					$(ref $rest),+
				) = *self;
				$first.size_hint()
				$( + $rest.size_hint() )+
			}

			fn encode_to<T: Output + ?Sized>(&self, dest: &mut T) {
				let (
					ref $first,
					$(ref $rest),+
				) = *self;

				$first.encode_to(dest);
				$($rest.encode_to(dest);)+
			}
		}

		impl<$first: Decode, $($rest: Decode),+> Decode for ($first, $($rest),+) {
			fn decode<INPUT: Input>(input: &mut INPUT) -> Result<Self, super::Error> {
				Ok((
					match $first::decode(input) {
						Ok(x) => x,
						Err(e) => return Err(e),
					},
					$(match $rest::decode(input) {
						Ok(x) => x,
						Err(e) => return Err(e),
					},)+
				))
			}
		}

		impl<$first: EncodeLike<$fextra>, $fextra: Encode,
			$($rest: EncodeLike<$rextra>, $rextra: Encode),+> crate::EncodeLike<($fextra, $( $rextra ),+)>
			for ($first, $($rest),+) {}

		impl<$first: DecodeLength, $($rest),+> DecodeLength for ($first, $($rest),+) {
			fn len(self_encoded: &[u8]) -> Result<usize, Error> {
				$first::len(self_encoded)
			}
		}

		tuple_impl!( $( ($rest, $rextra), )+ );
	}
}

#[allow(non_snake_case)]
mod inner_tuple_impl {
	use super::*;

	tuple_impl!(
		(A0, A1),
		(B0, B1),
		(C0, C1),
		(D0, D1),
		(E0, E1),
		(F0, F1),
		(G0, G1),
		(H0, H1),
		(I0, I1),
		(J0, J1),
		(K0, K1),
		(L0, L1),
		(M0, M1),
		(N0, N1),
		(O0, O1),
		(P0, P1),
		(Q0, Q1),
		(R0, R1),
	);
}

macro_rules! impl_endians {
	( $( $t:ty; $ty_info:ident ),* ) => { $(
		impl EncodeLike for $t {}

		impl Encode for $t {
			const TYPE_INFO: TypeInfo = TypeInfo::$ty_info;

			fn size_hint(&self) -> usize {
				mem::size_of::<$t>()
			}

			fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
				let buf = self.to_le_bytes();
				f(&buf[..])
			}
		}

		impl Decode for $t {
			const TYPE_INFO: TypeInfo = TypeInfo::$ty_info;

			fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
				let mut buf = [0u8; mem::size_of::<$t>()];
				input.read(&mut buf)?;
				Ok(<$t>::from_le_bytes(buf))
			}

			fn encoded_fixed_size() -> Option<usize> {
				Some(mem::size_of::<$t>())
			}
		}

		impl DecodeWithMemTracking for $t {}
	)* }
}
macro_rules! impl_one_byte {
	( $( $t:ty; $ty_info:ident ),* ) => { $(
		impl EncodeLike for $t {}

		impl Encode for $t {
			const TYPE_INFO: TypeInfo = TypeInfo::$ty_info;

			fn size_hint(&self) -> usize {
				mem::size_of::<$t>()
			}

			fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
				f(&[*self as u8][..])
			}
		}

		impl Decode for $t {
			const TYPE_INFO: TypeInfo = TypeInfo::$ty_info;

			fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
				Ok(input.read_byte()? as $t)
			}
		}

		impl DecodeWithMemTracking for $t {}
	)* }
}

impl_endians!(u16; U16, u32; U32, u64; U64, u128; U128, i16; I16, i32; I32, i64; I64, i128; I128);
impl_one_byte!(u8; U8, i8; I8);

impl_endians!(f32; F32, f64; F64);

impl EncodeLike for bool {}

impl Encode for bool {
	fn size_hint(&self) -> usize {
		mem::size_of::<bool>()
	}

	fn using_encoded<R, F: FnOnce(&[u8]) -> R>(&self, f: F) -> R {
		f(&[*self as u8][..])
	}
}

impl Decode for bool {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let byte = input.read_byte()?;
		match byte {
			0 => Ok(false),
			1 => Ok(true),
			_ => Err("Invalid boolean representation".into()),
		}
	}

	fn encoded_fixed_size() -> Option<usize> {
		Some(1)
	}
}

impl DecodeWithMemTracking for bool {}

impl Encode for Duration {
	fn size_hint(&self) -> usize {
		mem::size_of::<u64>() + mem::size_of::<u32>()
	}

	fn encode(&self) -> Vec<u8> {
		let secs = self.as_secs();
		let nanos = self.subsec_nanos();
		(secs, nanos).encode()
	}
}

impl Decode for Duration {
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let (secs, nanos) = <(u64, u32)>::decode(input)
			.map_err(|e| e.chain("Could not decode `Duration(u64, u32)`"))?;
		if nanos >= A_BILLION {
			Err("Could not decode `Duration`: Number of nanoseconds should not be higher than 10^9.".into())
		} else {
			Ok(Duration::new(secs, nanos))
		}
	}
}

impl DecodeWithMemTracking for Duration {}

impl EncodeLike for Duration {}

impl<T> Encode for Range<T>
where
	T: Encode,
{
	fn size_hint(&self) -> usize {
		2 * mem::size_of::<T>()
	}

	fn encode(&self) -> Vec<u8> {
		(&self.start, &self.end).encode()
	}
}

impl<T> Decode for Range<T>
where
	T: Decode,
{
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let (start, end) =
			<(T, T)>::decode(input).map_err(|e| e.chain("Could not decode `Range<T>`"))?;
		Ok(Range { start, end })
	}
}

impl<T: DecodeWithMemTracking> DecodeWithMemTracking for Range<T> {}

impl<T> Encode for RangeInclusive<T>
where
	T: Encode,
{
	fn size_hint(&self) -> usize {
		2 * mem::size_of::<T>()
	}

	fn encode(&self) -> Vec<u8> {
		(self.start(), self.end()).encode()
	}
}

impl<T> Decode for RangeInclusive<T>
where
	T: Decode,
{
	fn decode<I: Input>(input: &mut I) -> Result<Self, Error> {
		let (start, end) =
			<(T, T)>::decode(input).map_err(|e| e.chain("Could not decode `RangeInclusive<T>`"))?;
		Ok(RangeInclusive::new(start, end))
	}
}

impl<T: DecodeWithMemTracking> DecodeWithMemTracking for RangeInclusive<T> {}

#[cfg(test)]
mod tests {
	use super::*;
	use std::borrow::Cow;

	#[test]
	fn vec_is_sliceable() {
		let v = b"Hello world".to_vec();
		v.using_encoded(|ref slice| assert_eq!(slice, &b"\x2cHello world"));
	}

	#[test]
	fn encode_borrowed_tuple() {
		let x = vec![1u8, 2, 3, 4];
		let y = 128i64;

		let encoded = (&x, &y).encode();

		assert_eq!((x, y), Decode::decode(&mut &encoded[..]).unwrap());
	}

	#[test]
	fn cow_works() {
		let x = &[1u32, 2, 3, 4, 5, 6][..];
		let y = Cow::Borrowed(&x);
		assert_eq!(x.encode(), y.encode());

		let z: Cow<'_, [u32]> = Cow::decode(&mut &x.encode()[..]).unwrap();
		assert_eq!(*z, *x);
	}

	#[test]
	fn cow_string_works() {
		let x = "Hello world!";
		let y = Cow::Borrowed(&x);
		assert_eq!(x.encode(), y.encode());

		let z: Cow<'_, str> = Cow::decode(&mut &x.encode()[..]).unwrap();
		assert_eq!(*z, *x);
	}

	fn hexify(bytes: &[u8]) -> String {
		bytes
			.iter()
			.map(|ref b| format!("{:02x}", b))
			.collect::<Vec<String>>()
			.join(" ")
	}

	#[test]
	fn string_encoded_as_expected() {
		let value = String::from("Hello, World!");
		let encoded = value.encode();
		assert_eq!(hexify(&encoded), "34 48 65 6c 6c 6f 2c 20 57 6f 72 6c 64 21");
		assert_eq!(<String>::decode(&mut &encoded[..]).unwrap(), value);
	}

	#[test]
	fn vec_of_u8_encoded_as_expected() {
		let value = vec![0u8, 1, 1, 2, 3, 5, 8, 13, 21, 34];
		let encoded = value.encode();
		assert_eq!(hexify(&encoded), "28 00 01 01 02 03 05 08 0d 15 22");
		assert_eq!(<Vec<u8>>::decode(&mut &encoded[..]).unwrap(), value);
	}

	#[test]
	fn vec_of_i16_encoded_as_expected() {
		let value = vec![0i16, 1, -1, 2, -2, 3, -3];
		let encoded = value.encode();
		assert_eq!(hexify(&encoded), "1c 00 00 01 00 ff ff 02 00 fe ff 03 00 fd ff");
		assert_eq!(<Vec<i16>>::decode(&mut &encoded[..]).unwrap(), value);
	}

	#[test]
	fn vec_of_option_int_encoded_as_expected() {
		let value = vec![Some(1i8), Some(-1), None];
		let encoded = value.encode();
		assert_eq!(hexify(&encoded), "0c 01 01 01 ff 00");
		assert_eq!(<Vec<Option<i8>>>::decode(&mut &encoded[..]).unwrap(), value);
	}

	#[test]
	fn vec_of_option_bool_encoded_as_expected() {
		let value = vec![OptionBool(Some(true)), OptionBool(Some(false)), OptionBool(None)];
		let encoded = value.encode();
		assert_eq!(hexify(&encoded), "0c 01 02 00");
		assert_eq!(<Vec<OptionBool>>::decode(&mut &encoded[..]).unwrap(), value);
	}

	#[test]
	fn vec_of_empty_tuples_encoded_as_expected() {
		let value = vec![(), (), (), (), ()];
		let encoded = value.encode();
		assert_eq!(hexify(&encoded), "14");
		assert_eq!(<Vec<()>>::decode(&mut &encoded[..]).unwrap(), value);
	}

	#[cfg(feature = "bytes")]
	#[test]
	fn bytes_works_as_expected() {
		let input = bytes::Bytes::from_static(b"hello");
		let encoded = Encode::encode(&input);
		let encoded_vec = input.to_vec().encode();
		assert_eq!(encoded, encoded_vec);

		assert_eq!(&b"hello"[..], bytes::Bytes::decode(&mut &encoded[..]).unwrap(),);
	}

	#[cfg(feature = "bytes")]
	#[test]
	fn bytes_deserialized_from_bytes_is_zero_copy() {
		let encoded = bytes::Bytes::from(Encode::encode(&b"hello".to_vec()));
		let decoded = decode_from_bytes::<bytes::Bytes>(encoded.clone()).unwrap();
		assert_eq!(decoded, &b"hello"[..]);

		// The `slice_ref` will panic if the `decoded` is not a subslice of `encoded`.
		assert_eq!(encoded.slice_ref(&decoded), &b"hello"[..]);
	}

	#[cfg(feature = "bytes")]
	#[test]
	fn nested_bytes_deserialized_from_bytes_is_zero_copy() {
		let encoded = bytes::Bytes::from(Encode::encode(&Some(b"hello".to_vec())));
		let decoded = decode_from_bytes::<Option<bytes::Bytes>>(encoded.clone()).unwrap();
		let decoded = decoded.as_ref().unwrap();
		assert_eq!(decoded, &b"hello"[..]);

		// The `slice_ref` will panic if the `decoded` is not a subslice of `encoded`.
		assert_eq!(encoded.slice_ref(decoded), &b"hello"[..]);
	}

	fn test_encode_length<T: Encode + Decode + DecodeLength>(thing: &T, len: usize) {
		assert_eq!(<T as DecodeLength>::len(&thing.encode()[..]).unwrap(), len);
	}

	#[test]
	fn len_works_for_decode_collection_types() {
		let vector = vec![10; 10];
		let mut btree_map: BTreeMap<u32, u32> = BTreeMap::new();
		btree_map.insert(1, 1);
		btree_map.insert(2, 2);
		let mut btree_set: BTreeSet<u32> = BTreeSet::new();
		btree_set.insert(1);
		btree_set.insert(2);
		let mut vd = VecDeque::new();
		vd.push_front(1);
		vd.push_front(2);
		let mut bh = BinaryHeap::new();
		bh.push(1);
		bh.push(2);
		let mut ll = LinkedList::new();
		ll.push_back(1);
		ll.push_back(2);
		let t1: (Vec<_>,) = (vector.clone(),);
		let t2: (Vec<_>, u32) = (vector.clone(), 3u32);

		test_encode_length(&vector, 10);
		test_encode_length(&btree_map, 2);
		test_encode_length(&btree_set, 2);
		test_encode_length(&vd, 2);
		test_encode_length(&bh, 2);
		test_encode_length(&ll, 2);
		test_encode_length(&t1, 10);
		test_encode_length(&t2, 10);
	}

	#[test]
	fn vec_of_string_encoded_as_expected() {
		let value = vec![
			"Hamlet".to_owned(),
			"Война и мир".to_owned(),
			"三国演义".to_owned(),
			"أَلْف لَيْلَة وَلَيْلَة‎".to_owned(),
		];
		let encoded = value.encode();
		assert_eq!(
			hexify(&encoded),
			"10 18 48 61 6d 6c 65 74 50 d0 92 d0 be d0 b9 d0 bd d0 b0 20 d0 \
			b8 20 d0 bc d0 b8 d1 80 30 e4 b8 89 e5 9b bd e6 bc 94 e4 b9 89 bc d8 a3 d9 8e d9 84 d9 92 \
			d9 81 20 d9 84 d9 8e d9 8a d9 92 d9 84 d9 8e d8 a9 20 d9 88 d9 8e d9 84 d9 8e d9 8a d9 92 \
			d9 84 d9 8e d8 a9 e2 80 8e"
		);
		assert_eq!(<Vec<String>>::decode(&mut &encoded[..]).unwrap(), value);
	}

	#[derive(Debug, PartialEq)]
	struct MyWrapper(Compact<u32>);
	impl Deref for MyWrapper {
		type Target = Compact<u32>;
		fn deref(&self) -> &Self::Target {
			&self.0
		}
	}
	impl WrapperTypeEncode for MyWrapper {}

	impl From<Compact<u32>> for MyWrapper {
		fn from(c: Compact<u32>) -> Self {
			MyWrapper(c)
		}
	}
	impl WrapperTypeDecode for MyWrapper {
		type Wrapped = Compact<u32>;
	}

	#[test]
	fn should_work_for_wrapper_types() {
		let result = vec![0b1100];

		assert_eq!(MyWrapper(3u32.into()).encode(), result);
		assert_eq!(MyWrapper::decode(&mut &*result).unwrap(), MyWrapper(3_u32.into()));
	}

	#[test]
	fn codec_vec_deque_u8_and_u16() {
		let mut v_u8 = VecDeque::new();
		let mut v_u16 = VecDeque::new();

		for i in 0..50 {
			v_u8.push_front(i as u8);
			v_u16.push_front(i as u16);
		}
		for i in 50..100 {
			v_u8.push_back(i as u8);
			v_u16.push_back(i as u16);
		}

		assert_eq!(Decode::decode(&mut &v_u8.encode()[..]), Ok(v_u8));
		assert_eq!(Decode::decode(&mut &v_u16.encode()[..]), Ok(v_u16));
	}

	#[test]
	fn codec_iterator() {
		let t1: BTreeSet<u32> = FromIterator::from_iter((0..10).flat_map(|i| 0..i));
		let t2: LinkedList<u32> = FromIterator::from_iter((0..10).flat_map(|i| 0..i));
		let t3: BinaryHeap<u32> = FromIterator::from_iter((0..10).flat_map(|i| 0..i));
		let t4: BTreeMap<u16, u32> =
			FromIterator::from_iter((0..10).flat_map(|i| 0..i).map(|i| (i as u16, i + 10)));
		let t5: BTreeSet<Vec<u8>> = FromIterator::from_iter((0..10).map(|i| Vec::from_iter(0..i)));
		let t6: LinkedList<Vec<u8>> =
			FromIterator::from_iter((0..10).map(|i| Vec::from_iter(0..i)));
		let t7: BinaryHeap<Vec<u8>> =
			FromIterator::from_iter((0..10).map(|i| Vec::from_iter(0..i)));
		let t8: BTreeMap<Vec<u8>, u32> = FromIterator::from_iter(
			(0..10).map(|i| Vec::from_iter(0..i)).map(|i| (i.clone(), i.len() as u32)),
		);

		assert_eq!(Decode::decode(&mut &t1.encode()[..]), Ok(t1));
		assert_eq!(Decode::decode(&mut &t2.encode()[..]), Ok(t2));
		assert_eq!(
			Decode::decode(&mut &t3.encode()[..]).map(BinaryHeap::into_sorted_vec),
			Ok(t3.into_sorted_vec()),
		);
		assert_eq!(Decode::decode(&mut &t4.encode()[..]), Ok(t4));
		assert_eq!(Decode::decode(&mut &t5.encode()[..]), Ok(t5));
		assert_eq!(Decode::decode(&mut &t6.encode()[..]), Ok(t6));
		assert_eq!(
			Decode::decode(&mut &t7.encode()[..]).map(BinaryHeap::into_sorted_vec),
			Ok(t7.into_sorted_vec()),
		);
		assert_eq!(Decode::decode(&mut &t8.encode()[..]), Ok(t8));
	}

	#[test]
	fn io_reader() {
		let mut io_reader = IoReader(std::io::Cursor::new(&[1u8, 2, 3][..]));

		let mut v = [0; 2];
		io_reader.read(&mut v[..]).unwrap();
		assert_eq!(v, [1, 2]);

		assert_eq!(io_reader.read_byte().unwrap(), 3);

		assert_eq!(io_reader.read_byte(), Err("io error: UnexpectedEof".into()));
	}

	#[test]
	fn shared_references_implement_encode() {
		Arc::new(10u32).encode();
		Rc::new(10u32).encode();
	}

	#[test]
	fn not_limit_input_test() {
		use crate::Input;

		struct NoLimit<'a>(&'a [u8]);

		impl Input for NoLimit<'_> {
			fn remaining_len(&mut self) -> Result<Option<usize>, Error> {
				Ok(None)
			}

			fn read(&mut self, into: &mut [u8]) -> Result<(), Error> {
				self.0.read(into)
			}
		}

		let len = INITIAL_PREALLOCATION * 2 + 1;
		let mut i = Compact(len as u32).encode();
		i.resize(i.len() + len, 0);
		assert_eq!(<Vec<u8>>::decode(&mut NoLimit(&i[..])).unwrap(), vec![0u8; len]);

		let i = Compact(len as u32).encode();
		assert_eq!(
			<Vec<u8>>::decode(&mut NoLimit(&i[..])).err().unwrap().to_string(),
			"Not enough data to fill buffer",
		);

		let i = Compact(1000u32).encode();
		assert_eq!(
			<Vec<u8>>::decode(&mut NoLimit(&i[..])).err().unwrap().to_string(),
			"Not enough data to fill buffer",
		);
	}

	#[test]
	fn boolean() {
		assert_eq!(true.encode(), vec![1]);
		assert_eq!(false.encode(), vec![0]);
		assert!(bool::decode(&mut &[1][..]).unwrap());
		assert!(!bool::decode(&mut &[0][..]).unwrap());
	}

	#[test]
	fn some_encode_like() {
		fn t<B: EncodeLike>() {}
		t::<&[u8]>();
		t::<&str>();
		t::<NonZeroU32>();
	}

	#[test]
	fn vec_deque_encode_like_vec() {
		let data: VecDeque<u32> = vec![1, 2, 3, 4, 5, 6].into();
		let encoded = data.encode();

		let decoded = Vec::<u32>::decode(&mut &encoded[..]).unwrap();
		assert!(decoded.iter().all(|v| data.contains(v)));
		assert_eq!(data.len(), decoded.len());

		let encoded = decoded.encode();
		let decoded = VecDeque::<u32>::decode(&mut &encoded[..]).unwrap();
		assert_eq!(data, decoded);
	}

	#[test]
	fn vec_decode_right_capacity() {
		let data: Vec<u32> = vec![1, 2, 3];
		let mut encoded = data.encode();
		encoded.resize(encoded.len() * 2, 0);
		let decoded = Vec::<u32>::decode(&mut &encoded[..]).unwrap();
		assert_eq!(data, decoded);
		assert_eq!(decoded.capacity(), decoded.len());
		// Check with non-integer type
		let data: Vec<String> = vec!["1".into(), "2".into(), "3".into()];
		let mut encoded = data.encode();
		encoded.resize(65536, 0);
		let decoded = Vec::<String>::decode(&mut &encoded[..]).unwrap();
		assert_eq!(data, decoded);
		assert_eq!(decoded.capacity(), decoded.len());
	}

	#[test]
	fn duration() {
		let num_secs = 13;
		let num_nanos = 37;

		let duration = Duration::new(num_secs, num_nanos);
		let expected = (num_secs, num_nanos).encode();

		assert_eq!(duration.encode(), expected);
		assert_eq!(Duration::decode(&mut &expected[..]).unwrap(), duration);
	}

	#[test]
	fn malformed_duration_encoding_fails() {
		// This test should fail, as the number of nanoseconds encoded is exactly 10^9.
		let invalid_nanos = A_BILLION;
		let encoded = (0u64, invalid_nanos).encode();
		assert!(Duration::decode(&mut &encoded[..]).is_err());

		let num_secs = 1u64;
		let num_nanos = 37u32;
		let invalid_nanos = num_secs as u32 * A_BILLION + num_nanos;
		let encoded = (num_secs, invalid_nanos).encode();
		// This test should fail, as the number of nano seconds encoded is bigger than 10^9.
		assert!(Duration::decode(&mut &encoded[..]).is_err());

		// Now constructing a valid duration and encoding it. Those asserts should not fail.
		let duration = Duration::from_nanos(invalid_nanos as u64);
		let expected = (num_secs, num_nanos).encode();

		assert_eq!(duration.encode(), expected);
		assert_eq!(Duration::decode(&mut &expected[..]).unwrap(), duration);
	}

	#[test]
	fn u64_max() {
		let num_secs = u64::MAX;
		let num_nanos = 0;
		let duration = Duration::new(num_secs, num_nanos);
		let expected = (num_secs, num_nanos).encode();

		assert_eq!(duration.encode(), expected);
		assert_eq!(Duration::decode(&mut &expected[..]).unwrap(), duration);
	}

	#[test]
	fn decoding_does_not_overflow() {
		let num_secs = u64::MAX;
		let num_nanos = A_BILLION;

		// `num_nanos`' carry should make `num_secs` overflow if we were to call `Duration::new()`.
		// This test checks that the we do not call `Duration::new()`.
		let encoded = (num_secs, num_nanos).encode();
		assert!(Duration::decode(&mut &encoded[..]).is_err());
	}

	#[test]
	fn string_invalid_utf8() {
		// `167, 10` is not a valid utf8 sequence, so this should be an error.
		let mut bytes: &[u8] = &[20, 114, 167, 10, 20, 114];

		let obj = <String>::decode(&mut bytes);
		assert!(obj.is_err());
	}

	#[test]
	fn empty_array_encode_and_decode() {
		let data: [u32; 0] = [];
		let encoded = data.encode();
		assert!(encoded.is_empty());
		<[u32; 0]>::decode(&mut &encoded[..]).unwrap();
	}

	macro_rules! test_array_encode_and_decode {
		( $( $name:ty ),* $(,)? ) => {
			$(
				paste::item! {
					#[test]
					fn [<test_array_encode_and_decode _ $name>]() {
						let data: [$name; 32] = [123 as $name; 32];
						let encoded = data.encode();
						let decoded: [$name; 32] = Decode::decode(&mut &encoded[..]).unwrap();
						assert_eq!(decoded, data);
					}
				}
			)*
		}
	}

	test_array_encode_and_decode!(u8, i8, u16, i16, u32, i32, u64, i64, u128, i128);

	test_array_encode_and_decode!(f32, f64);

	fn test_encoded_size(val: impl Encode) {
		let length = val.using_encoded(|v| v.len());

		assert_eq!(length, val.encoded_size());
	}

	struct TestStruct {
		data: Vec<u32>,
		other: u8,
		compact: Compact<u128>,
	}

	impl Encode for TestStruct {
		fn encode_to<W: Output + ?Sized>(&self, dest: &mut W) {
			self.data.encode_to(dest);
			self.other.encode_to(dest);
			self.compact.encode_to(dest);
		}
	}

	#[test]
	fn encoded_size_works() {
		test_encoded_size(120u8);
		test_encoded_size(30u16);
		test_encoded_size(1u32);
		test_encoded_size(2343545u64);
		test_encoded_size(34358394245459854u128);
		test_encoded_size(vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10u32]);
		test_encoded_size(Compact(32445u32));
		test_encoded_size(Compact(34353454453545u128));
		test_encoded_size(TestStruct {
			data: vec![1, 2, 4, 5, 6],
			other: 45,
			compact: Compact(123234545),
		});
	}

	#[test]
	fn ranges() {
		let range = Range { start: 1, end: 100 };
		let range_bytes = (1, 100).encode();
		assert_eq!(range.encode(), range_bytes);
		assert_eq!(Range::decode(&mut &range_bytes[..]), Ok(range));

		let range_inclusive = RangeInclusive::new(1, 100);
		let range_inclusive_bytes = (1, 100).encode();
		assert_eq!(range_inclusive.encode(), range_inclusive_bytes);
		assert_eq!(RangeInclusive::decode(&mut &range_inclusive_bytes[..]), Ok(range_inclusive));
	}
}
