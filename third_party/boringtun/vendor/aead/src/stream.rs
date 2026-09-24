//! Streaming AEAD support.
//!
//! Implementation of the STREAM online authenticated encryption construction
//! as described in the paper
//! [Online Authenticated-Encryption and its Nonce-Reuse Misuse-Resistance][1].
//!
//! ## About
//!
//! The STREAM construction supports encrypting/decrypting sequences of AEAD
//! message segments, which is useful in cases where the overall message is too
//! large to fit in a single buffer and needs to be processed incrementally.
//!
//! STREAM defends against reordering and truncation attacks which are common
//! in naive schemes which attempt to provide these properties, and is proven
//! to meet the security definition of "nonce-based online authenticated
//! encryption" (nOAE) as given in the aforementioned paper.
//!
//! ## Diagram
//!
//! ![STREAM Diagram](https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/img/AEADs/rogaway-stream.svg)
//!
//! Legend:
//!
//! - 𝐄k: AEAD encryption under key `k`
//! - 𝐌: message
//! - 𝐍: nonce
//! - 𝐀: additional associated data
//! - 𝐂: ciphertext
//! - 𝜏: MAC tag
//!
//! [1]: https://eprint.iacr.org/2015/189.pdf

#![allow(clippy::upper_case_acronyms)]

use crate::{AeadCore, AeadInPlace, Buffer, Error, Key, KeyInit, Result};
use core::ops::{AddAssign, Sub};
use generic_array::{
    typenum::{Unsigned, U4, U5},
    ArrayLength, GenericArray,
};

#[cfg(feature = "alloc")]
use {crate::Payload, alloc::vec::Vec};

/// Nonce as used by a given AEAD construction and STREAM primitive.
pub type Nonce<A, S> = GenericArray<u8, NonceSize<A, S>>;

/// Size of a nonce as used by a STREAM construction, sans the overhead of
/// the STREAM protocol itself.
pub type NonceSize<A, S> =
    <<A as AeadCore>::NonceSize as Sub<<S as StreamPrimitive<A>>::NonceOverhead>>::Output;

/// STREAM encryptor instantiated with [`StreamBE32`] as the underlying
/// STREAM primitive.
pub type EncryptorBE32<A> = Encryptor<A, StreamBE32<A>>;

/// STREAM decryptor instantiated with [`StreamBE32`] as the underlying
/// STREAM primitive.
pub type DecryptorBE32<A> = Decryptor<A, StreamBE32<A>>;

/// STREAM encryptor instantiated with [`StreamLE31`] as the underlying
/// STREAM primitive.
pub type EncryptorLE31<A> = Encryptor<A, StreamLE31<A>>;

/// STREAM decryptor instantiated with [`StreamLE31`] as the underlying
/// STREAM primitive.
pub type DecryptorLE31<A> = Decryptor<A, StreamLE31<A>>;

/// Create a new STREAM from the provided AEAD.
pub trait NewStream<A>: StreamPrimitive<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<Self::NonceOverhead>,
    NonceSize<A, Self>: ArrayLength<u8>,
{
    /// Create a new STREAM with the given key and nonce.
    fn new(key: &Key<A>, nonce: &Nonce<A, Self>) -> Self
    where
        A: KeyInit,
        Self: Sized,
    {
        Self::from_aead(A::new(key), nonce)
    }

    /// Create a new STREAM from the given AEAD cipher.
    fn from_aead(aead: A, nonce: &Nonce<A, Self>) -> Self;
}

/// Low-level STREAM implementation.
///
/// This trait provides a particular "flavor" of STREAM, as there are
/// different ways the specifics of the construction can be implemented.
///
/// Deliberately immutable and stateless to permit parallel operation.
pub trait StreamPrimitive<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<Self::NonceOverhead>,
    NonceSize<A, Self>: ArrayLength<u8>,
{
    /// Number of bytes this STREAM primitive requires from the nonce.
    type NonceOverhead: ArrayLength<u8>;

    /// Type used as the STREAM counter.
    type Counter: AddAssign + Copy + Default + Eq;

    /// Value to use when incrementing the STREAM counter (i.e. one)
    const COUNTER_INCR: Self::Counter;

    /// Maximum value of the STREAM counter.
    const COUNTER_MAX: Self::Counter;

    /// Encrypt an AEAD message in-place at the given position in the STREAM.
    fn encrypt_in_place(
        &self,
        position: Self::Counter,
        last_block: bool,
        associated_data: &[u8],
        buffer: &mut dyn Buffer,
    ) -> Result<()>;

    /// Decrypt an AEAD message in-place at the given position in the STREAM.
    fn decrypt_in_place(
        &self,
        position: Self::Counter,
        last_block: bool,
        associated_data: &[u8],
        buffer: &mut dyn Buffer,
    ) -> Result<()>;

    /// Encrypt the given plaintext payload, and return the resulting
    /// ciphertext as a vector of bytes.
    #[cfg(feature = "alloc")]
    #[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
    fn encrypt<'msg, 'aad>(
        &self,
        position: Self::Counter,
        last_block: bool,
        plaintext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>> {
        let payload = plaintext.into();
        let mut buffer = Vec::with_capacity(payload.msg.len() + A::TagSize::to_usize());
        buffer.extend_from_slice(payload.msg);
        self.encrypt_in_place(position, last_block, payload.aad, &mut buffer)?;
        Ok(buffer)
    }

    /// Decrypt the given ciphertext slice, and return the resulting plaintext
    /// as a vector of bytes.
    #[cfg(feature = "alloc")]
    #[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
    fn decrypt<'msg, 'aad>(
        &self,
        position: Self::Counter,
        last_block: bool,
        ciphertext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>> {
        let payload = ciphertext.into();
        let mut buffer = Vec::from(payload.msg);
        self.decrypt_in_place(position, last_block, payload.aad, &mut buffer)?;
        Ok(buffer)
    }

    /// Obtain [`Encryptor`] for this [`StreamPrimitive`].
    fn encryptor(self) -> Encryptor<A, Self>
    where
        Self: Sized,
    {
        Encryptor::from_stream_primitive(self)
    }

    /// Obtain [`Decryptor`] for this [`StreamPrimitive`].
    fn decryptor(self) -> Decryptor<A, Self>
    where
        Self: Sized,
    {
        Decryptor::from_stream_primitive(self)
    }
}

/// Implement a stateful STREAM object (i.e. encryptor or decryptor)
macro_rules! impl_stream_object {
    (
        $name:ident,
        $next_method:tt,
        $next_in_place_method:tt,
        $last_method:tt,
        $last_in_place_method:tt,
        $op:tt,
        $in_place_op:tt,
        $op_desc:expr,
        $obj_desc:expr
    ) => {
        #[doc = "Stateful STREAM object which can"]
        #[doc = $op_desc]
        #[doc = "AEAD messages one-at-a-time."]
        #[doc = ""]
        #[doc = "This corresponds to the "]
        #[doc = $obj_desc]
        #[doc = "object as defined in the paper"]
        #[doc = "[Online Authenticated-Encryption and its Nonce-Reuse Misuse-Resistance][1]."]
        #[doc = ""]
        #[doc = "[1]: https://eprint.iacr.org/2015/189.pdf"]
        pub struct $name<A, S>
        where
            A: AeadInPlace,
            S: StreamPrimitive<A>,
            A::NonceSize: Sub<<S as StreamPrimitive<A>>::NonceOverhead>,
            NonceSize<A, S>: ArrayLength<u8>,
        {
            /// Underlying STREAM primitive.
            stream: S,

            /// Current position in the STREAM.
            position: S::Counter,
        }

        impl<A, S> $name<A, S>
        where
            A: AeadInPlace,
            S: StreamPrimitive<A>,
            A::NonceSize: Sub<<S as StreamPrimitive<A>>::NonceOverhead>,
            NonceSize<A, S>: ArrayLength<u8>,
        {
            #[doc = "Create a"]
            #[doc = $obj_desc]
            #[doc = "object from the given AEAD key and nonce."]
            pub fn new(key: &Key<A>, nonce: &Nonce<A, S>) -> Self
            where
                A: KeyInit,
                S: NewStream<A>,
            {
                Self::from_stream_primitive(S::new(key, nonce))
            }

            #[doc = "Create a"]
            #[doc = $obj_desc]
            #[doc = "object from the given AEAD primitive."]
            pub fn from_aead(aead: A, nonce: &Nonce<A, S>) -> Self
            where
                A: KeyInit,
                S: NewStream<A>,
            {
                Self::from_stream_primitive(S::from_aead(aead, nonce))
            }

            #[doc = "Create a"]
            #[doc = $obj_desc]
            #[doc = "object from the given STREAM primitive."]
            pub fn from_stream_primitive(stream: S) -> Self {
                Self {
                    stream,
                    position: Default::default(),
                }
            }

            #[doc = "Use the underlying AEAD to"]
            #[doc = $op_desc]
            #[doc = "the next AEAD message in this STREAM, returning the"]
            #[doc = "result as a [`Vec`]."]
            #[cfg(feature = "alloc")]
            #[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
            pub fn $next_method<'msg, 'aad>(
                &mut self,
                payload: impl Into<Payload<'msg, 'aad>>,
            ) -> Result<Vec<u8>> {
                if self.position == S::COUNTER_MAX {
                    // Counter overflow. Note that the maximum counter value is
                    // deliberately disallowed, as it would preclude being able
                    // to encrypt a last block (i.e. with `$last_in_place_method`)
                    return Err(Error);
                }

                let result = self.stream.$op(self.position, false, payload)?;

                // Note: overflow checked above
                self.position += S::COUNTER_INCR;
                Ok(result)
            }

            #[doc = "Use the underlying AEAD to"]
            #[doc = $op_desc]
            #[doc = "the next AEAD message in this STREAM in-place."]
            pub fn $next_in_place_method(
                &mut self,
                associated_data: &[u8],
                buffer: &mut dyn Buffer,
            ) -> Result<()> {
                if self.position == S::COUNTER_MAX {
                    // Counter overflow. Note that the maximum counter value is
                    // deliberately disallowed, as it would preclude being able
                    // to encrypt a last block (i.e. with `$last_in_place_method`)
                    return Err(Error);
                }

                self.stream
                    .$in_place_op(self.position, false, associated_data, buffer)?;

                // Note: overflow checked above
                self.position += S::COUNTER_INCR;
                Ok(())
            }

            #[doc = "Use the underlying AEAD to"]
            #[doc = $op_desc]
            #[doc = "the last AEAD message in this STREAM,"]
            #[doc = "consuming the "]
            #[doc = $obj_desc]
            #[doc = "object in order to prevent further use."]
            #[cfg(feature = "alloc")]
            #[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
            pub fn $last_method<'msg, 'aad>(
                self,
                payload: impl Into<Payload<'msg, 'aad>>,
            ) -> Result<Vec<u8>> {
                self.stream.$op(self.position, true, payload)
            }

            #[doc = "Use the underlying AEAD to"]
            #[doc = $op_desc]
            #[doc = "the last AEAD message in this STREAM in-place,"]
            #[doc = "consuming the "]
            #[doc = $obj_desc]
            #[doc = "object in order to prevent further use."]
            pub fn $last_in_place_method(
                self,
                associated_data: &[u8],
                buffer: &mut dyn Buffer,
            ) -> Result<()> {
                self.stream
                    .$in_place_op(self.position, true, associated_data, buffer)
            }
        }
    };
}

impl_stream_object!(
    Encryptor,
    encrypt_next,
    encrypt_next_in_place,
    encrypt_last,
    encrypt_last_in_place,
    encrypt,
    encrypt_in_place,
    "encrypt",
    "ℰ STREAM encryptor"
);

impl_stream_object!(
    Decryptor,
    decrypt_next,
    decrypt_next_in_place,
    decrypt_last,
    decrypt_last_in_place,
    decrypt,
    decrypt_in_place,
    "decrypt",
    "𝒟 STREAM decryptor"
);

/// The original "Rogaway-flavored" STREAM as described in the paper
/// [Online Authenticated-Encryption and its Nonce-Reuse Misuse-Resistance][1].
///
/// Uses a 32-bit big endian counter and 1-byte "last block" flag stored as
/// the last 5-bytes of the AEAD nonce.
///
/// [1]: https://eprint.iacr.org/2015/189.pdf
pub struct StreamBE32<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<U5>,
    <<A as AeadCore>::NonceSize as Sub<U5>>::Output: ArrayLength<u8>,
{
    /// Underlying AEAD cipher
    aead: A,

    /// Nonce (sans STREAM overhead)
    nonce: Nonce<A, Self>,
}

impl<A> NewStream<A> for StreamBE32<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<U5>,
    <<A as AeadCore>::NonceSize as Sub<U5>>::Output: ArrayLength<u8>,
{
    fn from_aead(aead: A, nonce: &Nonce<A, Self>) -> Self {
        Self {
            aead,
            nonce: nonce.clone(),
        }
    }
}

impl<A> StreamPrimitive<A> for StreamBE32<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<U5>,
    <<A as AeadCore>::NonceSize as Sub<U5>>::Output: ArrayLength<u8>,
{
    type NonceOverhead = U5;
    type Counter = u32;
    const COUNTER_INCR: u32 = 1;
    const COUNTER_MAX: u32 = core::u32::MAX;

    fn encrypt_in_place(
        &self,
        position: u32,
        last_block: bool,
        associated_data: &[u8],
        buffer: &mut dyn Buffer,
    ) -> Result<()> {
        let nonce = self.aead_nonce(position, last_block);
        self.aead.encrypt_in_place(&nonce, associated_data, buffer)
    }

    fn decrypt_in_place(
        &self,
        position: Self::Counter,
        last_block: bool,
        associated_data: &[u8],
        buffer: &mut dyn Buffer,
    ) -> Result<()> {
        let nonce = self.aead_nonce(position, last_block);
        self.aead.decrypt_in_place(&nonce, associated_data, buffer)
    }
}

impl<A> StreamBE32<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<U5>,
    <<A as AeadCore>::NonceSize as Sub<U5>>::Output: ArrayLength<u8>,
{
    /// Compute the full AEAD nonce including the STREAM counter and last
    /// block flag.
    fn aead_nonce(&self, position: u32, last_block: bool) -> crate::Nonce<A> {
        let mut result = GenericArray::default();

        // TODO(tarcieri): use `generic_array::sequence::Concat` (or const generics)
        let (prefix, tail) = result.split_at_mut(NonceSize::<A, Self>::to_usize());
        prefix.copy_from_slice(&self.nonce);

        let (counter, flag) = tail.split_at_mut(4);
        counter.copy_from_slice(&position.to_be_bytes());
        flag[0] = last_block as u8;

        result
    }
}

/// STREAM as instantiated with a 31-bit little endian counter and 1-bit
/// "last block" flag stored as the most significant bit of the counter
/// when interpreted as a 32-bit integer.
///
/// The 31-bit + 1-bit value is stored as the last 4 bytes of the AEAD nonce.
pub struct StreamLE31<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<U4>,
    <<A as AeadCore>::NonceSize as Sub<U4>>::Output: ArrayLength<u8>,
{
    /// Underlying AEAD cipher
    aead: A,

    /// Nonce (sans STREAM overhead)
    nonce: Nonce<A, Self>,
}

impl<A> NewStream<A> for StreamLE31<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<U4>,
    <<A as AeadCore>::NonceSize as Sub<U4>>::Output: ArrayLength<u8>,
{
    fn from_aead(aead: A, nonce: &Nonce<A, Self>) -> Self {
        Self {
            aead,
            nonce: nonce.clone(),
        }
    }
}

impl<A> StreamPrimitive<A> for StreamLE31<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<U4>,
    <<A as AeadCore>::NonceSize as Sub<U4>>::Output: ArrayLength<u8>,
{
    type NonceOverhead = U4;
    type Counter = u32;
    const COUNTER_INCR: u32 = 1;
    const COUNTER_MAX: u32 = 0xfff_ffff;

    fn encrypt_in_place(
        &self,
        position: u32,
        last_block: bool,
        associated_data: &[u8],
        buffer: &mut dyn Buffer,
    ) -> Result<()> {
        let nonce = self.aead_nonce(position, last_block)?;
        self.aead.encrypt_in_place(&nonce, associated_data, buffer)
    }

    fn decrypt_in_place(
        &self,
        position: Self::Counter,
        last_block: bool,
        associated_data: &[u8],
        buffer: &mut dyn Buffer,
    ) -> Result<()> {
        let nonce = self.aead_nonce(position, last_block)?;
        self.aead.decrypt_in_place(&nonce, associated_data, buffer)
    }
}

impl<A> StreamLE31<A>
where
    A: AeadInPlace,
    A::NonceSize: Sub<U4>,
    <<A as AeadCore>::NonceSize as Sub<U4>>::Output: ArrayLength<u8>,
{
    /// Compute the full AEAD nonce including the STREAM counter and last
    /// block flag.
    fn aead_nonce(&self, position: u32, last_block: bool) -> Result<crate::Nonce<A>> {
        if position > Self::COUNTER_MAX {
            return Err(Error);
        }

        let mut result = GenericArray::default();

        // TODO(tarcieri): use `generic_array::sequence::Concat` (or const generics)
        let (prefix, tail) = result.split_at_mut(NonceSize::<A, Self>::to_usize());
        prefix.copy_from_slice(&self.nonce);

        let position_with_flag = position | ((last_block as u32) << 31);
        tail.copy_from_slice(&position_with_flag.to_le_bytes());

        Ok(result)
    }
}
