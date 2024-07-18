#![no_std]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]
#![doc = include_str!("../README.md")]
#![doc(html_logo_url = "https://raw.githubusercontent.com/RustCrypto/meta/master/logo_small.png")]
#![allow(non_snake_case)]
#![forbid(unsafe_code)]
#![warn(
    clippy::unwrap_used,
    missing_docs,
    rust_2018_idioms,
    unused_lifetimes,
    unused_qualifications
)]

//! # Using Ed25519 generically over algorithm implementations/providers
//!
//! By using the `ed25519` crate, you can write code which signs and verifies
//! messages using the Ed25519 signature algorithm generically over any
//! supported Ed25519 implementation (see the next section for available
//! providers).
//!
//! This allows consumers of your code to plug in whatever implementation they
//! want to use without having to add all potential Ed25519 libraries you'd
//! like to support as optional dependencies.
//!
//! ## Example
//!
//! ```
//! use ed25519::signature::{Signer, Verifier};
//!
//! pub struct HelloSigner<S>
//! where
//!     S: Signer<ed25519::Signature>
//! {
//!     pub signing_key: S
//! }
//!
//! impl<S> HelloSigner<S>
//! where
//!     S: Signer<ed25519::Signature>
//! {
//!     pub fn sign(&self, person: &str) -> ed25519::Signature {
//!         // NOTE: use `try_sign` if you'd like to be able to handle
//!         // errors from external signing services/devices (e.g. HSM/KMS)
//!         // <https://docs.rs/signature/latest/signature/trait.Signer.html#tymethod.try_sign>
//!         self.signing_key.sign(format_message(person).as_bytes())
//!     }
//! }
//!
//! pub struct HelloVerifier<V> {
//!     pub verifying_key: V
//! }
//!
//! impl<V> HelloVerifier<V>
//! where
//!     V: Verifier<ed25519::Signature>
//! {
//!     pub fn verify(
//!         &self,
//!         person: &str,
//!         signature: &ed25519::Signature
//!     ) -> Result<(), ed25519::Error> {
//!         self.verifying_key.verify(format_message(person).as_bytes(), signature)
//!     }
//! }
//!
//! fn format_message(person: &str) -> String {
//!     format!("Hello, {}!", person)
//! }
//! ```
//!
//! ## Using above example with `ed25519-dalek`
//!
//! The [`ed25519-dalek`] crate natively supports the [`ed25519::Signature`][`Signature`]
//! type defined in this crate along with the [`signature::Signer`] and
//! [`signature::Verifier`] traits.
//!
//! Below is an example of how a hypothetical consumer of the code above can
//! instantiate and use the previously defined `HelloSigner` and `HelloVerifier`
//! types with [`ed25519-dalek`] as the signing/verification provider:
//!
//! *NOTE: requires [`ed25519-dalek`] v2 or newer for compatibility with
//! `ed25519` v2.2+*.
//!
//! ```
//! use ed25519_dalek::{Signer, Verifier, Signature};
//! #
//! # pub struct HelloSigner<S>
//! # where
//! #     S: Signer<Signature>
//! # {
//! #     pub signing_key: S
//! # }
//! #
//! # impl<S> HelloSigner<S>
//! # where
//! #     S: Signer<Signature>
//! # {
//! #     pub fn sign(&self, person: &str) -> Signature {
//! #         // NOTE: use `try_sign` if you'd like to be able to handle
//! #         // errors from external signing services/devices (e.g. HSM/KMS)
//! #         // <https://docs.rs/signature/latest/signature/trait.Signer.html#tymethod.try_sign>
//! #         self.signing_key.sign(format_message(person).as_bytes())
//! #     }
//! # }
//! #
//! # pub struct HelloVerifier<V> {
//! #     pub verifying_key: V
//! # }
//! #
//! # impl<V> HelloVerifier<V>
//! # where
//! #     V: Verifier<Signature>
//! # {
//! #     pub fn verify(
//! #         &self,
//! #         person: &str,
//! #         signature: &Signature
//! #     ) -> Result<(), ed25519::Error> {
//! #         self.verifying_key.verify(format_message(person).as_bytes(), signature)
//! #     }
//! # }
//! #
//! # fn format_message(person: &str) -> String {
//! #     format!("Hello, {}!", person)
//! # }
//! use rand_core::OsRng; // Requires the `std` feature of `rand_core`
//!
//! /// `HelloSigner` defined above instantiated with `ed25519-dalek` as
//! /// the signing provider.
//! pub type DalekHelloSigner = HelloSigner<ed25519_dalek::SigningKey>;
//!
//! let signing_key = ed25519_dalek::SigningKey::generate(&mut OsRng);
//! let signer = DalekHelloSigner { signing_key };
//! let person = "Joe"; // Message to sign
//! let signature = signer.sign(person);
//!
//! /// `HelloVerifier` defined above instantiated with `ed25519-dalek`
//! /// as the signature verification provider.
//! pub type DalekHelloVerifier = HelloVerifier<ed25519_dalek::VerifyingKey>;
//!
//! let verifying_key: ed25519_dalek::VerifyingKey = signer.signing_key.verifying_key();
//! let verifier = DalekHelloVerifier { verifying_key };
//! assert!(verifier.verify(person, &signature).is_ok());
//! ```
//!
//! ## Using above example with `ring-compat`
//!
//! The [`ring-compat`] crate provides wrappers for [*ring*] which implement
//! the [`signature::Signer`] and [`signature::Verifier`] traits for
//! [`ed25519::Signature`][`Signature`].
//!
//! Below is an example of how a hypothetical consumer of the code above can
//! instantiate and use the previously defined `HelloSigner` and `HelloVerifier`
//! types with [`ring-compat`] as the signing/verification provider:
//!
//! ```
//! use ring_compat::signature::{
//!     ed25519::{Signature, SigningKey, VerifyingKey},
//!     Signer, Verifier
//! };
//! #
//! # pub struct HelloSigner<S>
//! # where
//! #     S: Signer<Signature>
//! # {
//! #     pub signing_key: S
//! # }
//! #
//! # impl<S> HelloSigner<S>
//! # where
//! #     S: Signer<Signature>
//! # {
//! #     pub fn sign(&self, person: &str) -> Signature {
//! #         // NOTE: use `try_sign` if you'd like to be able to handle
//! #         // errors from external signing services/devices (e.g. HSM/KMS)
//! #         // <https://docs.rs/signature/latest/signature/trait.Signer.html#tymethod.try_sign>
//! #         self.signing_key.sign(format_message(person).as_bytes())
//! #     }
//! # }
//! #
//! # pub struct HelloVerifier<V> {
//! #     pub verifying_key: V
//! # }
//! #
//! # impl<V> HelloVerifier<V>
//! # where
//! #     V: Verifier<Signature>
//! # {
//! #     pub fn verify(
//! #         &self,
//! #         person: &str,
//! #         signature: &Signature
//! #     ) -> Result<(), ed25519::Error> {
//! #         self.verifying_key.verify(format_message(person).as_bytes(), signature)
//! #     }
//! # }
//! #
//! # fn format_message(person: &str) -> String {
//! #     format!("Hello, {}!", person)
//! # }
//! use rand_core::{OsRng, RngCore}; // Requires the `std` feature of `rand_core`
//!
//! /// `HelloSigner` defined above instantiated with *ring* as
//! /// the signing provider.
//! pub type RingHelloSigner = HelloSigner<SigningKey>;
//!
//! let mut ed25519_seed = [0u8; 32];
//! OsRng.fill_bytes(&mut ed25519_seed);
//!
//! let signing_key = SigningKey::from_bytes(&ed25519_seed);
//! let verifying_key = signing_key.verifying_key();
//!
//! let signer = RingHelloSigner { signing_key };
//! let person = "Joe"; // Message to sign
//! let signature = signer.sign(person);
//!
//! /// `HelloVerifier` defined above instantiated with *ring*
//! /// as the signature verification provider.
//! pub type RingHelloVerifier = HelloVerifier<VerifyingKey>;
//!
//! let verifier = RingHelloVerifier { verifying_key };
//! assert!(verifier.verify(person, &signature).is_ok());
//! ```
//!
//! # Available Ed25519 providers
//!
//! The following libraries support the types/traits from the `ed25519` crate:
//!
//! - [`ed25519-dalek`] - mature pure Rust implementation of Ed25519
//! - [`ring-compat`] - compatibility wrapper for [*ring*]
//! - [`yubihsm`] - host-side client library for YubiHSM2 devices from Yubico
//!
//! [`ed25519-dalek`]: https://docs.rs/ed25519-dalek
//! [`ring-compat`]: https://docs.rs/ring-compat
//! [*ring*]: https://github.com/briansmith/ring
//! [`yubihsm`]: https://github.com/iqlusioninc/yubihsm.rs/blob/develop/README.md
//!
//! # Features
//!
//! The following features are presently supported:
//!
//! - `pkcs8`: support for decoding/encoding PKCS#8-formatted private keys using the
//!   [`KeypairBytes`] type.
//! - `std` *(default)*: Enable `std` support in [`signature`], which currently only affects whether
//!   [`signature::Error`] implements `std::error::Error`.
//! - `serde`: Implement `serde::Deserialize` and `serde::Serialize` for [`Signature`]. Signatures
//!   are serialized as their bytes.
//! - `serde_bytes`: Implement `serde_bytes::Deserialize` and `serde_bytes::Serialize` for
//!   [`Signature`]. This enables more compact representations for formats with an efficient byte
//!   array representation. As per the `serde_bytes` documentation, this can most easily be realised
//!   using the `#[serde(with = "serde_bytes")]` annotation, e.g.:
//!
//!   ```ignore
//!   # use ed25519::Signature;
//!   # use serde::{Deserialize, Serialize};
//!   #[derive(Deserialize, Serialize)]
//!   #[serde(transparent)]
//!   struct SignatureAsBytes(#[serde(with = "serde_bytes")] Signature);
//!   ```

#[cfg(feature = "alloc")]
extern crate alloc;

mod hex;

#[cfg(feature = "pkcs8")]
pub mod pkcs8;

#[cfg(feature = "serde")]
mod serde;

pub use signature::{self, Error, SignatureEncoding};

#[cfg(feature = "pkcs8")]
pub use crate::pkcs8::{KeypairBytes, PublicKeyBytes};

use core::fmt;

#[cfg(feature = "alloc")]
use alloc::vec::Vec;

/// Size of a single component of an Ed25519 signature.
const COMPONENT_SIZE: usize = 32;

/// Size of an `R` or `s` component of an Ed25519 signature when serialized
/// as bytes.
pub type ComponentBytes = [u8; COMPONENT_SIZE];

/// Ed25519 signature serialized as a byte array.
pub type SignatureBytes = [u8; Signature::BYTE_SIZE];

/// Ed25519 signature.
///
/// This type represents a container for the byte serialization of an Ed25519
/// signature, and does not necessarily represent well-formed field or curve
/// elements.
///
/// Signature verification libraries are expected to reject invalid field
/// elements at the time a signature is verified.
#[derive(Copy, Clone, Eq, PartialEq)]
#[repr(C)]
pub struct Signature {
    R: ComponentBytes,
    s: ComponentBytes,
}

impl Signature {
    /// Size of an encoded Ed25519 signature in bytes.
    pub const BYTE_SIZE: usize = COMPONENT_SIZE * 2;

    /// Parse an Ed25519 signature from a byte slice.
    pub fn from_bytes(bytes: &SignatureBytes) -> Self {
        let mut R = ComponentBytes::default();
        let mut s = ComponentBytes::default();

        let components = bytes.split_at(COMPONENT_SIZE);
        R.copy_from_slice(components.0);
        s.copy_from_slice(components.1);

        Self { R, s }
    }

    /// Parse an Ed25519 signature from its `R` and `s` components.
    pub fn from_components(R: ComponentBytes, s: ComponentBytes) -> Self {
        Self { R, s }
    }

    /// Parse an Ed25519 signature from a byte slice.
    ///
    /// # Returns
    /// - `Ok` on success
    /// - `Err` if the input byte slice is not 64-bytes
    pub fn from_slice(bytes: &[u8]) -> signature::Result<Self> {
        SignatureBytes::try_from(bytes)
            .map(Into::into)
            .map_err(|_| Error::new())
    }

    /// Bytes for the `R` component of a signature.
    pub fn r_bytes(&self) -> &ComponentBytes {
        &self.R
    }

    /// Bytes for the `s` component of a signature.
    pub fn s_bytes(&self) -> &ComponentBytes {
        &self.s
    }

    /// Return the inner byte array.
    pub fn to_bytes(&self) -> SignatureBytes {
        let mut ret = [0u8; Self::BYTE_SIZE];
        let (R, s) = ret.split_at_mut(COMPONENT_SIZE);
        R.copy_from_slice(&self.R);
        s.copy_from_slice(&self.s);
        ret
    }

    /// Convert this signature into a byte vector.
    #[cfg(feature = "alloc")]
    pub fn to_vec(&self) -> Vec<u8> {
        self.to_bytes().to_vec()
    }
}

impl SignatureEncoding for Signature {
    type Repr = SignatureBytes;

    fn to_bytes(&self) -> SignatureBytes {
        self.to_bytes()
    }
}

impl From<Signature> for SignatureBytes {
    fn from(sig: Signature) -> SignatureBytes {
        sig.to_bytes()
    }
}

impl From<&Signature> for SignatureBytes {
    fn from(sig: &Signature) -> SignatureBytes {
        sig.to_bytes()
    }
}

impl From<SignatureBytes> for Signature {
    fn from(bytes: SignatureBytes) -> Self {
        Signature::from_bytes(&bytes)
    }
}

impl From<&SignatureBytes> for Signature {
    fn from(bytes: &SignatureBytes) -> Self {
        Signature::from_bytes(bytes)
    }
}

impl TryFrom<&[u8]> for Signature {
    type Error = Error;

    fn try_from(bytes: &[u8]) -> signature::Result<Self> {
        Self::from_slice(bytes)
    }
}

impl fmt::Debug for Signature {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ed25519::Signature")
            .field("R", &hex::ComponentFormatter(self.r_bytes()))
            .field("s", &hex::ComponentFormatter(self.s_bytes()))
            .finish()
    }
}

impl fmt::Display for Signature {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:X}", self)
    }
}
