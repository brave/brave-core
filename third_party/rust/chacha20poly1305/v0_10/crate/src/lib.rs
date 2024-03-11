#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![doc = include_str!("../README.md")]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/RustCrypto/meta/master/logo.svg",
    html_favicon_url = "https://raw.githubusercontent.com/RustCrypto/meta/master/logo.svg"
)]
#![warn(missing_docs, rust_2018_idioms)]

//! ## Supported Algorithms
//!
//! This crate contains pure Rust implementations of [`ChaCha20Poly1305`]
//! (with optional AVX2 acceleration) as well as the following variants thereof:
//!
//! - [`XChaCha20Poly1305`] - ChaCha20Poly1305 variant with an extended 192-bit (24-byte) nonce.
//! - [`ChaCha8Poly1305`] / [`ChaCha12Poly1305`] - non-standard, reduced-round variants
//!   (gated under the `reduced-round` Cargo feature). See the [Too Much Crypto][5]
//!   paper for background and rationale on when these constructions could be used.
//!   When in doubt, prefer [`ChaCha20Poly1305`].
//! - [`XChaCha8Poly1305`] / [`XChaCha12Poly1305`] - same as above,
//!   but with an extended 192-bit (24-byte) nonce.
//!
//! # Usage
//!
#![cfg_attr(all(feature = "getrandom", feature = "std"), doc = "```")]
#![cfg_attr(not(all(feature = "getrandom", feature = "std")), doc = "```ignore")]
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! use chacha20poly1305::{
//!     aead::{Aead, AeadCore, KeyInit, OsRng},
//!     ChaCha20Poly1305, Nonce
//! };
//!
//! let key = ChaCha20Poly1305::generate_key(&mut OsRng);
//! let cipher = ChaCha20Poly1305::new(&key);
//! let nonce = ChaCha20Poly1305::generate_nonce(&mut OsRng); // 96-bits; unique per message
//! let ciphertext = cipher.encrypt(&nonce, b"plaintext message".as_ref())?;
//! let plaintext = cipher.decrypt(&nonce, ciphertext.as_ref())?;
//! assert_eq!(&plaintext, b"plaintext message");
//! # Ok(())
//! # }
//! ```
//!
//! ## In-place Usage (eliminates `alloc` requirement)
//!
//! This crate has an optional `alloc` feature which can be disabled in e.g.
//! microcontroller environments that don't have a heap.
//!
//! The [`AeadInPlace::encrypt_in_place`] and [`AeadInPlace::decrypt_in_place`]
//! methods accept any type that impls the [`aead::Buffer`] trait which
//! contains the plaintext for encryption or ciphertext for decryption.
//!
//! Note that if you enable the `heapless` feature of this crate,
//! you will receive an impl of [`aead::Buffer`] for `heapless::Vec`
//! (re-exported from the [`aead`] crate as [`aead::heapless::Vec`]),
//! which can then be passed as the `buffer` parameter to the in-place encrypt
//! and decrypt methods:
//!
#![cfg_attr(
    all(feature = "getrandom", feature = "heapless", feature = "std"),
    doc = "```"
)]
#![cfg_attr(
    not(all(feature = "getrandom", feature = "heapless", feature = "std")),
    doc = "```ignore"
)]
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! use chacha20poly1305::{
//!     aead::{AeadCore, AeadInPlace, KeyInit, OsRng, heapless::Vec},
//!     ChaCha20Poly1305, Nonce,
//! };
//!
//! let key = ChaCha20Poly1305::generate_key(&mut OsRng);
//! let cipher = ChaCha20Poly1305::new(&key);
//! let nonce = ChaCha20Poly1305::generate_nonce(&mut OsRng); // 96-bits; unique per message
//!
//! let mut buffer: Vec<u8, 128> = Vec::new(); // Note: buffer needs 16-bytes overhead for auth tag
//! buffer.extend_from_slice(b"plaintext message");
//!
//! // Encrypt `buffer` in-place, replacing the plaintext contents with ciphertext
//! cipher.encrypt_in_place(&nonce, b"", &mut buffer)?;
//!
//! // `buffer` now contains the message ciphertext
//! assert_ne!(&buffer, b"plaintext message");
//!
//! // Decrypt `buffer` in-place, replacing its ciphertext context with the original plaintext
//! cipher.decrypt_in_place(&nonce, b"", &mut buffer)?;
//! assert_eq!(&buffer, b"plaintext message");
//! # Ok(())
//! # }
//! ```
//!
//! ## [`XChaCha20Poly1305`]
//!
//! ChaCha20Poly1305 variant with an extended 192-bit (24-byte) nonce.
//!
//! The construction is an adaptation of the same techniques used by
//! XSalsa20 as described in the paper "Extending the Salsa20 Nonce"
//! to the 96-bit nonce variant of ChaCha20, which derive a
//! separate subkey/nonce for each extended nonce:
//!
//! <https://cr.yp.to/snuffle/xsalsa-20081128.pdf>
//!
//! No authoritative specification exists for XChaCha20Poly1305, however the
//! construction has "rough consensus and running code" in the form of
//! several interoperable libraries and protocols (e.g. libsodium, WireGuard)
//! and is documented in an (expired) IETF draft, which also applies the
//! proof from the XSalsa20 paper to the construction in order to demonstrate
//! that XChaCha20 is secure if ChaCha20 is secure (see Section 3.1):
//!
//! <https://tools.ietf.org/html/draft-arciszewski-xchacha-03>
//!
//! It is worth noting that NaCl/libsodium's default "secretbox" algorithm is
//! XSalsa20Poly1305, not XChaCha20Poly1305, and thus not compatible with
//! this library. If you are interested in that construction, please see the
//! `xsalsa20poly1305` crate:
//!
//! <https://docs.rs/xsalsa20poly1305/>
//!
//! # Usage
//!
#![cfg_attr(all(feature = "getrandom", feature = "std"), doc = "```")]
#![cfg_attr(not(all(feature = "getrandom", feature = "std")), doc = "```ignore")]
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! use chacha20poly1305::{
//!     aead::{Aead, AeadCore, KeyInit, OsRng},
//!     XChaCha20Poly1305, XNonce
//! };
//!
//! let key = XChaCha20Poly1305::generate_key(&mut OsRng);
//! let cipher = XChaCha20Poly1305::new(&key);
//! let nonce = XChaCha20Poly1305::generate_nonce(&mut OsRng); // 192-bits; unique per message
//! let ciphertext = cipher.encrypt(&nonce, b"plaintext message".as_ref())?;
//! let plaintext = cipher.decrypt(&nonce, ciphertext.as_ref())?;
//! assert_eq!(&plaintext, b"plaintext message");
//! # Ok(())
//! # }
//! ```

mod cipher;

pub use aead::{self, consts, AeadCore, AeadInPlace, Error, KeyInit, KeySizeUser};

use self::cipher::Cipher;
use ::cipher::{KeyIvInit, StreamCipher, StreamCipherSeek};
use aead::{
    consts::{U0, U12, U16, U24, U32},
    generic_array::{ArrayLength, GenericArray},
};
use core::marker::PhantomData;
use zeroize::{Zeroize, ZeroizeOnDrop};

use chacha20::{ChaCha20, XChaCha20};

#[cfg(feature = "reduced-round")]
use chacha20::{ChaCha12, ChaCha8, XChaCha12, XChaCha8};

/// Key type (256-bits/32-bytes).
///
/// Implemented as an alias for [`GenericArray`].
///
/// All [`ChaChaPoly1305`] variants (including `XChaCha20Poly1305`) use this
/// key type.
pub type Key = GenericArray<u8, U32>;

/// Nonce type (96-bits/12-bytes).
///
/// Implemented as an alias for [`GenericArray`].
pub type Nonce = GenericArray<u8, U12>;

/// XNonce type (192-bits/24-bytes).
///
/// Implemented as an alias for [`GenericArray`].
pub type XNonce = GenericArray<u8, U24>;

/// Poly1305 tag.
///
/// Implemented as an alias for [`GenericArray`].
pub type Tag = GenericArray<u8, U16>;

/// ChaCha20Poly1305 Authenticated Encryption with Additional Data (AEAD).
pub type ChaCha20Poly1305 = ChaChaPoly1305<ChaCha20, U12>;

/// XChaCha20Poly1305 Authenticated Encryption with Additional Data (AEAD).
pub type XChaCha20Poly1305 = ChaChaPoly1305<XChaCha20, U24>;

/// ChaCha8Poly1305 (reduced round variant) Authenticated Encryption with Additional Data (AEAD).
#[cfg(feature = "reduced-round")]
#[cfg_attr(docsrs, doc(cfg(feature = "reduced-round")))]
pub type ChaCha8Poly1305 = ChaChaPoly1305<ChaCha8, U12>;

/// ChaCha12Poly1305 (reduced round variant) Authenticated Encryption with Additional Data (AEAD).
#[cfg(feature = "reduced-round")]
#[cfg_attr(docsrs, doc(cfg(feature = "reduced-round")))]
pub type ChaCha12Poly1305 = ChaChaPoly1305<ChaCha12, U12>;

/// XChaCha8Poly1305 (reduced round variant) Authenticated Encryption with Additional Data (AEAD).
#[cfg(feature = "reduced-round")]
#[cfg_attr(docsrs, doc(cfg(feature = "reduced-round")))]
pub type XChaCha8Poly1305 = ChaChaPoly1305<XChaCha8, U24>;

/// XChaCha12Poly1305 (reduced round variant) Authenticated Encryption with Additional Data (AEAD).
#[cfg(feature = "reduced-round")]
#[cfg_attr(docsrs, doc(cfg(feature = "reduced-round")))]
pub type XChaCha12Poly1305 = ChaChaPoly1305<XChaCha12, U24>;

/// Generic ChaCha+Poly1305 Authenticated Encryption with Additional Data (AEAD) construction.
///
/// See the [toplevel documentation](index.html) for a usage example.
pub struct ChaChaPoly1305<C, N: ArrayLength<u8> = U12> {
    /// Secret key.
    key: Key,

    /// ChaCha stream cipher.
    stream_cipher: PhantomData<C>,

    /// Nonce size.
    nonce_size: PhantomData<N>,
}

impl<C, N> KeySizeUser for ChaChaPoly1305<C, N>
where
    N: ArrayLength<u8>,
{
    type KeySize = U32;
}

impl<C, N> KeyInit for ChaChaPoly1305<C, N>
where
    N: ArrayLength<u8>,
{
    #[inline]
    fn new(key: &Key) -> Self {
        Self {
            key: *key,
            stream_cipher: PhantomData,
            nonce_size: PhantomData,
        }
    }
}

impl<C, N> AeadCore for ChaChaPoly1305<C, N>
where
    N: ArrayLength<u8>,
{
    type NonceSize = N;
    type TagSize = U16;
    type CiphertextOverhead = U0;
}

impl<C, N> AeadInPlace for ChaChaPoly1305<C, N>
where
    C: KeyIvInit<KeySize = U32, IvSize = N> + StreamCipher + StreamCipherSeek,
    N: ArrayLength<u8>,
{
    fn encrypt_in_place_detached(
        &self,
        nonce: &aead::Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut [u8],
    ) -> Result<Tag, Error> {
        Cipher::new(C::new(&self.key, nonce)).encrypt_in_place_detached(associated_data, buffer)
    }

    fn decrypt_in_place_detached(
        &self,
        nonce: &aead::Nonce<Self>,
        associated_data: &[u8],
        buffer: &mut [u8],
        tag: &Tag,
    ) -> Result<(), Error> {
        Cipher::new(C::new(&self.key, nonce)).decrypt_in_place_detached(
            associated_data,
            buffer,
            tag,
        )
    }
}

impl<C, N> Clone for ChaChaPoly1305<C, N>
where
    N: ArrayLength<u8>,
{
    fn clone(&self) -> Self {
        Self {
            key: self.key,
            stream_cipher: PhantomData,
            nonce_size: PhantomData,
        }
    }
}

impl<C, N> Drop for ChaChaPoly1305<C, N>
where
    N: ArrayLength<u8>,
{
    fn drop(&mut self) {
        self.key.as_mut_slice().zeroize();
    }
}

impl<C, N: ArrayLength<u8>> ZeroizeOnDrop for ChaChaPoly1305<C, N> {}
