//! [Authenticated Encryption with Associated Data] (AEAD) traits
//!
//! This crate provides an abstract interface for AEAD ciphers, which guarantee
//! both confidentiality and integrity, even from a powerful attacker who is
//! able to execute [chosen-ciphertext attacks]. The resulting security property,
//! [ciphertext indistinguishability], is considered a basic requirement for
//! modern cryptographic implementations.
//!
//! See [RustCrypto/AEADs] for cipher implementations which use this trait.
//!
//! [Authenticated Encryption with Associated Data]: https://en.wikipedia.org/wiki/Authenticated_encryption
//! [chosen-ciphertext attacks]: https://en.wikipedia.org/wiki/Chosen-ciphertext_attack
//! [ciphertext indistinguishability]: https://en.wikipedia.org/wiki/Ciphertext_indistinguishability
//! [RustCrypto/AEADs]: https://github.com/RustCrypto/AEADs

#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/logo.svg",
    html_favicon_url = "https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/logo.svg"
)]
#![forbid(unsafe_code)]
#![warn(clippy::unwrap_used, missing_docs, rust_2018_idioms)]

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(feature = "std")]
extern crate std;

#[cfg(feature = "dev")]
#[cfg_attr(docsrs, doc(cfg(feature = "dev")))]
pub mod dev;

#[cfg(feature = "stream")]
#[cfg_attr(docsrs, doc(cfg(feature = "stream")))]
pub mod stream;

pub use crypto_common::{Key, KeyInit, KeySizeUser};
pub use generic_array::{self, typenum::consts};

#[cfg(feature = "arrayvec")]
#[cfg_attr(docsrs, doc(cfg(feature = "arrayvec")))]
pub use arrayvec;

#[cfg(feature = "bytes")]
#[cfg_attr(docsrs, doc(cfg(feature = "bytes")))]
pub use bytes;

#[cfg(feature = "getrandom")]
#[cfg_attr(docsrs, doc(cfg(feature = "getrandom")))]
pub use crypto_common::rand_core::OsRng;

#[cfg(feature = "heapless")]
#[cfg_attr(docsrs, doc(cfg(feature = "heapless")))]
pub use heapless;

#[cfg(feature = "rand_core")]
#[cfg_attr(docsrs, doc(cfg(feature = "rand_core")))]
pub use crypto_common::rand_core;

use core::fmt;
use generic_array::{typenum::Unsigned, ArrayLength, GenericArray};

#[cfg(feature = "alloc")]
use alloc::vec::Vec;

#[cfg(feature = "bytes")]
use bytes::BytesMut;

#[cfg(feature = "rand_core")]
use rand_core::{CryptoRng, RngCore};

/// Error type.
///
/// This type is deliberately opaque as to avoid potential side-channel
/// leakage (e.g. padding oracle).
#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct Error;

/// Result type alias with [`Error`].
pub type Result<T> = core::result::Result<T, Error>;

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("aead::Error")
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}

/// Nonce: single-use value for ensuring ciphertexts are unique
pub type Nonce<A> = GenericArray<u8, <A as AeadCore>::NonceSize>;

/// Tag: authentication code which ensures ciphertexts are authentic
pub type Tag<A> = GenericArray<u8, <A as AeadCore>::TagSize>;

/// Authenticated Encryption with Associated Data (AEAD) algorithm core trait.
///
/// Defines nonce, tag, and overhead sizes that are consumed by various other
/// `Aead*` traits.
pub trait AeadCore {
    /// The length of a nonce.
    type NonceSize: ArrayLength<u8>;

    /// The maximum length of the nonce.
    type TagSize: ArrayLength<u8>;

    /// The upper bound amount of additional space required to support a
    /// ciphertext vs. a plaintext.
    type CiphertextOverhead: ArrayLength<u8> + Unsigned;

    /// Generate a random nonce for this AEAD algorithm.
    ///
    /// AEAD algorithms accept a parameter to encryption/decryption called
    /// a "nonce" which must be unique every time encryption is performed and
    /// never repeated for the same key. The nonce is often prepended to the
    /// ciphertext. The nonce used to produce a given ciphertext must be passed
    /// to the decryption function in order for it to decrypt correctly.
    ///
    /// Nonces don't necessarily have to be random, but it is one strategy
    /// which is implemented by this function.
    ///
    /// # ⚠️Security Warning
    ///
    /// AEAD algorithms often fail catastrophically if nonces are ever repeated
    /// (with SIV modes being an exception).
    ///
    /// Using random nonces runs the risk of repeating them unless the nonce
    /// size is particularly large (e.g. 192-bit extended nonces used by the
    /// `XChaCha20Poly1305` and `XSalsa20Poly1305` constructions.
    ///
    /// [NIST SP 800-38D] recommends the following:
    ///
    /// > The total number of invocations of the authenticated encryption
    /// > function shall not exceed 2^32, including all IV lengths and all
    /// > instances of the authenticated encryption function with the given key.
    ///
    /// Following this guideline, only 4,294,967,296 messages with random
    /// nonces can be encrypted under a given key. While this bound is high,
    /// it's possible to encounter in practice, and systems which might
    /// reach it should consider alternatives to purely random nonces, like
    /// a counter or a combination of a random nonce + counter.
    ///
    /// See the [`stream`] module for a ready-made implementation of the latter.
    ///
    /// [NIST SP 800-38D]: https://csrc.nist.gov/publications/detail/sp/800-38d/final
    #[cfg(feature = "rand_core")]
    #[cfg_attr(docsrs, doc(cfg(feature = "rand_core")))]
    fn generate_nonce(mut rng: impl CryptoRng + RngCore) -> Nonce<Self>
    where
        Nonce<Self>: Default,
    {
        let mut nonce = Nonce::<Self>::default();
        rng.fill_bytes(&mut nonce);
        nonce
    }
}

/// Authenticated Encryption with Associated Data (AEAD) algorithm.
///
/// This trait is intended for use with stateless AEAD algorithms. The
/// [`AeadMut`] trait provides a stateful interface.
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub trait Aead: AeadCore {
    /// Encrypt the given plaintext payload, and return the resulting
    /// ciphertext as a vector of bytes.
    ///
    /// The [`Payload`] type can be used to provide Additional Associated Data
    /// (AAD) along with the message: this is an optional bytestring which is
    /// not encrypted, but *is* authenticated along with the message. Failure
    /// to pass the same AAD that was used during encryption will cause
    /// decryption to fail, which is useful if you would like to "bind" the
    /// ciphertext to some other identifier, like a digital signature key
    /// or other identifier.
    ///
    /// If you don't care about AAD and just want to encrypt a plaintext
    /// message, `&[u8]` will automatically be coerced into a `Payload`:
    ///
    /// ```nobuild
    /// let plaintext = b"Top secret message, handle with care";
    /// let ciphertext = cipher.encrypt(nonce, plaintext);
    /// ```
    ///
    /// The default implementation assumes a postfix tag (ala AES-GCM,
    /// AES-GCM-SIV, ChaCha20Poly1305). [`Aead`] implementations which do not
    /// use a postfix tag will need to override this to correctly assemble the
    /// ciphertext message.
    fn encrypt<'msg, 'aad>(
        &self,
        nonce: &Nonce<Self>,
        plaintext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>>;

    /// Decrypt the given ciphertext slice, and return the resulting plaintext
    /// as a vector of bytes.
    ///
    /// See notes on [`Aead::encrypt()`] about allowable message payloads and
    /// Associated Additional Data (AAD).
    ///
    /// If you have no AAD, you can call this as follows:
    ///
    /// ```nobuild
    /// let ciphertext = b"...";
    /// let plaintext = cipher.decrypt(nonce, ciphertext)?;
    /// ```
    ///
    /// The default implementation assumes a postfix tag (ala AES-GCM,
    /// AES-GCM-SIV, ChaCha20Poly1305). [`Aead`] implementations which do not
    /// use a postfix tag will need to override this to correctly parse the
    /// ciphertext message.
    fn decrypt<'msg, 'aad>(
        &self,
        nonce: &Nonce<Self>,
        ciphertext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>>;
}

/// Stateful Authenticated Encryption with Associated Data algorithm.
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub trait AeadMut: AeadCore {
    /// Encrypt the given plaintext slice, and return the resulting ciphertext
    /// as a vector of bytes.
    ///
    /// See notes on [`Aead::encrypt()`] about allowable message payloads and
    /// Associated Additional Data (AAD).
    fn encrypt<'msg, 'aad>(
        &mut self,
        nonce: &Nonce<Self>,
        plaintext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>>;

    /// Decrypt the given ciphertext slice, and return the resulting plaintext
    /// as a vector of bytes.
    ///
    /// See notes on [`Aead::encrypt()`] and [`Aead::decrypt()`] about allowable
    /// message payloads and Associated Additional Data (AAD).
    fn decrypt<'msg, 'aad>(
        &mut self,
        nonce: &Nonce<Self>,
        ciphertext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>>;
}

/// Implement the `decrypt_in_place` method on [`AeadInPlace`] and
/// [`AeadMutInPlace]`, using a macro to gloss over the `&self` vs `&mut self`.
///
/// Assumes a postfix authentication tag. AEAD ciphers which do not use a
/// postfix authentication tag will need to define their own implementation.
macro_rules! impl_decrypt_in_place {
    ($aead:expr, $nonce:expr, $aad:expr, $buffer:expr) => {{
        if $buffer.len() < Self::TagSize::to_usize() {
            return Err(Error);
        }

        let tag_pos = $buffer.len() - Self::TagSize::to_usize();
        let (msg, tag) = $buffer.as_mut().split_at_mut(tag_pos);
        $aead.decrypt_in_place_detached($nonce, $aad, msg, Tag::<Self>::from_slice(tag))?;
        $buffer.truncate(tag_pos);
        Ok(())
    }};
}

/// In-place stateless AEAD trait.
///
/// This trait is both object safe and has no dependencies on `alloc` or `std`.
pub trait AeadInPlace: AeadCore {
    /// Encrypt the given buffer containing a plaintext message in-place.
    ///
    /// The buffer must have sufficient capacity to store the ciphertext
    /// message, which will always be larger than the original plaintext.
    /// The exact size needed is cipher-dependent, but generally includes
    /// the size of an authentication tag.
    ///
    /// Returns an error if the buffer has insufficient capacity to store the
    /// resulting ciphertext message.
    fn encrypt_in_place(
        &self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut dyn Buffer,
    ) -> Result<()> {
        let tag = self.encrypt_in_place_detached(nonce, associated_data, buffer.as_mut())?;
        buffer.extend_from_slice(tag.as_slice())?;
        Ok(())
    }

    /// Encrypt the data in-place, returning the authentication tag
    fn encrypt_in_place_detached(
        &self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut [u8],
    ) -> Result<Tag<Self>>;

    /// Decrypt the message in-place, returning an error in the event the
    /// provided authentication tag does not match the given ciphertext.
    ///
    /// The buffer will be truncated to the length of the original plaintext
    /// message upon success.
    fn decrypt_in_place(
        &self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut dyn Buffer,
    ) -> Result<()> {
        impl_decrypt_in_place!(self, nonce, associated_data, buffer)
    }

    /// Decrypt the message in-place, returning an error in the event the provided
    /// authentication tag does not match the given ciphertext (i.e. ciphertext
    /// is modified/unauthentic)
    fn decrypt_in_place_detached(
        &self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut [u8],
        tag: &Tag<Self>,
    ) -> Result<()>;
}

/// In-place stateful AEAD trait.
///
/// This trait is both object safe and has no dependencies on `alloc` or `std`.
pub trait AeadMutInPlace: AeadCore {
    /// Encrypt the given buffer containing a plaintext message in-place.
    ///
    /// The buffer must have sufficient capacity to store the ciphertext
    /// message, which will always be larger than the original plaintext.
    /// The exact size needed is cipher-dependent, but generally includes
    /// the size of an authentication tag.
    ///
    /// Returns an error if the buffer has insufficient capacity to store the
    /// resulting ciphertext message.
    fn encrypt_in_place(
        &mut self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut impl Buffer,
    ) -> Result<()> {
        let tag = self.encrypt_in_place_detached(nonce, associated_data, buffer.as_mut())?;
        buffer.extend_from_slice(tag.as_slice())?;
        Ok(())
    }

    /// Encrypt the data in-place, returning the authentication tag
    fn encrypt_in_place_detached(
        &mut self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut [u8],
    ) -> Result<Tag<Self>>;

    /// Decrypt the message in-place, returning an error in the event the
    /// provided authentication tag does not match the given ciphertext.
    ///
    /// The buffer will be truncated to the length of the original plaintext
    /// message upon success.
    fn decrypt_in_place(
        &mut self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut impl Buffer,
    ) -> Result<()> {
        impl_decrypt_in_place!(self, nonce, associated_data, buffer)
    }

    /// Decrypt the data in-place, returning an error in the event the provided
    /// authentication tag does not match the given ciphertext (i.e. ciphertext
    /// is modified/unauthentic)
    fn decrypt_in_place_detached(
        &mut self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut [u8],
        tag: &Tag<Self>,
    ) -> Result<()>;
}

#[cfg(feature = "alloc")]
impl<Alg: AeadInPlace> Aead for Alg {
    fn encrypt<'msg, 'aad>(
        &self,
        nonce: &Nonce<Self>,
        plaintext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>> {
        let payload = plaintext.into();
        let mut buffer = Vec::with_capacity(payload.msg.len() + Self::TagSize::to_usize());
        buffer.extend_from_slice(payload.msg);
        self.encrypt_in_place(nonce, payload.aad, &mut buffer)?;
        Ok(buffer)
    }

    fn decrypt<'msg, 'aad>(
        &self,
        nonce: &Nonce<Self>,
        ciphertext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>> {
        let payload = ciphertext.into();
        let mut buffer = Vec::from(payload.msg);
        self.decrypt_in_place(nonce, payload.aad, &mut buffer)?;
        Ok(buffer)
    }
}

#[cfg(feature = "alloc")]
impl<Alg: AeadMutInPlace> AeadMut for Alg {
    fn encrypt<'msg, 'aad>(
        &mut self,
        nonce: &Nonce<Self>,
        plaintext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>> {
        let payload = plaintext.into();
        let mut buffer = Vec::with_capacity(payload.msg.len() + Self::TagSize::to_usize());
        buffer.extend_from_slice(payload.msg);
        self.encrypt_in_place(nonce, payload.aad, &mut buffer)?;
        Ok(buffer)
    }

    fn decrypt<'msg, 'aad>(
        &mut self,
        nonce: &Nonce<Self>,
        ciphertext: impl Into<Payload<'msg, 'aad>>,
    ) -> Result<Vec<u8>> {
        let payload = ciphertext.into();
        let mut buffer = Vec::from(payload.msg);
        self.decrypt_in_place(nonce, payload.aad, &mut buffer)?;
        Ok(buffer)
    }
}

impl<Alg: AeadInPlace> AeadMutInPlace for Alg {
    fn encrypt_in_place(
        &mut self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut impl Buffer,
    ) -> Result<()> {
        <Self as AeadInPlace>::encrypt_in_place(self, nonce, associated_data, buffer)
    }

    fn encrypt_in_place_detached(
        &mut self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut [u8],
    ) -> Result<Tag<Self>> {
        <Self as AeadInPlace>::encrypt_in_place_detached(self, nonce, associated_data, buffer)
    }

    fn decrypt_in_place(
        &mut self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut impl Buffer,
    ) -> Result<()> {
        <Self as AeadInPlace>::decrypt_in_place(self, nonce, associated_data, buffer)
    }

    fn decrypt_in_place_detached(
        &mut self,
        nonce: &Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut [u8],
        tag: &Tag<Self>,
    ) -> Result<()> {
        <Self as AeadInPlace>::decrypt_in_place_detached(self, nonce, associated_data, buffer, tag)
    }
}

/// AEAD payloads (message + AAD).
///
/// Combination of a message (plaintext or ciphertext) and
/// "additional associated data" (AAD) to be authenticated (in cleartext)
/// along with the message.
///
/// If you don't care about AAD, you can pass a `&[u8]` as the payload to
/// `encrypt`/`decrypt` and it will automatically be coerced to this type.
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub struct Payload<'msg, 'aad> {
    /// Message to be encrypted/decrypted
    pub msg: &'msg [u8],

    /// Optional "additional associated data" to authenticate along with
    /// this message. If AAD is provided at the time the message is encrypted,
    /// the same AAD *MUST* be provided at the time the message is decrypted,
    /// or decryption will fail.
    pub aad: &'aad [u8],
}

#[cfg(feature = "alloc")]
impl<'msg, 'aad> From<&'msg [u8]> for Payload<'msg, 'aad> {
    fn from(msg: &'msg [u8]) -> Self {
        Self { msg, aad: b"" }
    }
}

/// In-place encryption/decryption byte buffers.
///
/// This trait defines the set of methods needed to support in-place operations
/// on a `Vec`-like data type.
pub trait Buffer: AsRef<[u8]> + AsMut<[u8]> {
    /// Get the length of the buffer
    fn len(&self) -> usize {
        self.as_ref().len()
    }

    /// Is the buffer empty?
    fn is_empty(&self) -> bool {
        self.as_ref().is_empty()
    }

    /// Extend this buffer from the given slice
    fn extend_from_slice(&mut self, other: &[u8]) -> Result<()>;

    /// Truncate this buffer to the given size
    fn truncate(&mut self, len: usize);
}

#[cfg(feature = "alloc")]
impl Buffer for Vec<u8> {
    fn extend_from_slice(&mut self, other: &[u8]) -> Result<()> {
        Vec::extend_from_slice(self, other);
        Ok(())
    }

    fn truncate(&mut self, len: usize) {
        Vec::truncate(self, len);
    }
}

#[cfg(feature = "bytes")]
impl Buffer for BytesMut {
    fn len(&self) -> usize {
        BytesMut::len(self)
    }

    fn is_empty(&self) -> bool {
        BytesMut::is_empty(self)
    }

    fn extend_from_slice(&mut self, other: &[u8]) -> Result<()> {
        BytesMut::extend_from_slice(self, other);
        Ok(())
    }

    fn truncate(&mut self, len: usize) {
        BytesMut::truncate(self, len);
    }
}

#[cfg(feature = "arrayvec")]
impl<const N: usize> Buffer for arrayvec::ArrayVec<u8, N> {
    fn extend_from_slice(&mut self, other: &[u8]) -> Result<()> {
        arrayvec::ArrayVec::try_extend_from_slice(self, other).map_err(|_| Error)
    }

    fn truncate(&mut self, len: usize) {
        arrayvec::ArrayVec::truncate(self, len);
    }
}

#[cfg(feature = "heapless")]
impl<const N: usize> Buffer for heapless::Vec<u8, N> {
    fn extend_from_slice(&mut self, other: &[u8]) -> Result<()> {
        heapless::Vec::extend_from_slice(self, other).map_err(|_| Error)
    }

    fn truncate(&mut self, len: usize) {
        heapless::Vec::truncate(self, len);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Ensure that `AeadInPlace` is object-safe
    #[allow(dead_code)]
    type DynAeadInPlace<N, T, O> =
        dyn AeadInPlace<NonceSize = N, TagSize = T, CiphertextOverhead = O>;

    /// Ensure that `AeadMutInPlace` is object-safe
    #[allow(dead_code)]
    type DynAeadMutInPlace<N, T, O> =
        dyn AeadMutInPlace<NonceSize = N, TagSize = T, CiphertextOverhead = O>;
}
