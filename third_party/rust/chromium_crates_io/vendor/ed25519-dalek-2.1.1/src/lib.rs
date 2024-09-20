// -*- mode: rust; -*-
//
// This file is part of ed25519-dalek.
// Copyright (c) 2017-2019 isis lovecruft
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>

//! A Rust implementation of ed25519 key generation, signing, and verification.
//!
//! # Example
//!
//! Creating an ed25519 signature on a message is simple.
//!
//! First, we need to generate a `SigningKey`, which includes both public and
//! secret halves of an asymmetric key.  To do so, we need a cryptographically
//! secure pseudorandom number generator (CSPRNG). For this example, we'll use
//! the operating system's builtin PRNG:
//!
#![cfg_attr(feature = "rand_core", doc = "```")]
#![cfg_attr(not(feature = "rand_core"), doc = "```ignore")]
//! # fn main() {
//! use rand::rngs::OsRng;
//! use ed25519_dalek::SigningKey;
//! use ed25519_dalek::Signature;
//!
//! let mut csprng = OsRng;
//! let signing_key: SigningKey = SigningKey::generate(&mut csprng);
//! # }
//! ```
//!
//! We can now use this `signing_key` to sign a message:
//!
#![cfg_attr(feature = "rand_core", doc = "```")]
#![cfg_attr(not(feature = "rand_core"), doc = "```ignore")]
//! # fn main() {
//! # use rand::rngs::OsRng;
//! # use ed25519_dalek::SigningKey;
//! # let mut csprng = OsRng;
//! # let signing_key: SigningKey = SigningKey::generate(&mut csprng);
//! use ed25519_dalek::{Signature, Signer};
//! let message: &[u8] = b"This is a test of the tsunami alert system.";
//! let signature: Signature = signing_key.sign(message);
//! # }
//! ```
//!
//! As well as to verify that this is, indeed, a valid signature on
//! that `message`:
//!
#![cfg_attr(feature = "rand_core", doc = "```")]
#![cfg_attr(not(feature = "rand_core"), doc = "```ignore")]
//! # fn main() {
//! # use rand::rngs::OsRng;
//! # use ed25519_dalek::{SigningKey, Signature, Signer};
//! # let mut csprng = OsRng;
//! # let signing_key: SigningKey = SigningKey::generate(&mut csprng);
//! # let message: &[u8] = b"This is a test of the tsunami alert system.";
//! # let signature: Signature = signing_key.sign(message);
//! use ed25519_dalek::Verifier;
//! assert!(signing_key.verify(message, &signature).is_ok());
//! # }
//! ```
//!
//! Anyone else, given the `public` half of the `signing_key` can also easily
//! verify this signature:
//!
#![cfg_attr(feature = "rand_core", doc = "```")]
#![cfg_attr(not(feature = "rand_core"), doc = "```ignore")]
//! # fn main() {
//! # use rand::rngs::OsRng;
//! # use ed25519_dalek::SigningKey;
//! # use ed25519_dalek::Signature;
//! # use ed25519_dalek::Signer;
//! use ed25519_dalek::{VerifyingKey, Verifier};
//! # let mut csprng = OsRng;
//! # let signing_key: SigningKey = SigningKey::generate(&mut csprng);
//! # let message: &[u8] = b"This is a test of the tsunami alert system.";
//! # let signature: Signature = signing_key.sign(message);
//!
//! let verifying_key: VerifyingKey = signing_key.verifying_key();
//! assert!(verifying_key.verify(message, &signature).is_ok());
//! # }
//! ```
//!
//! ## Serialisation
//!
//! `VerifyingKey`s, `SecretKey`s, `SigningKey`s, and `Signature`s can be serialised
//! into byte-arrays by calling `.to_bytes()`.  It's perfectly acceptable and
//! safe to transfer and/or store those bytes.  (Of course, never transfer your
//! secret key to anyone else, since they will only need the public key to
//! verify your signatures!)
//!
#![cfg_attr(feature = "rand_core", doc = "```")]
#![cfg_attr(not(feature = "rand_core"), doc = "```ignore")]
//! # fn main() {
//! # use rand::rngs::OsRng;
//! # use ed25519_dalek::{SigningKey, Signature, Signer, VerifyingKey};
//! use ed25519_dalek::{PUBLIC_KEY_LENGTH, SECRET_KEY_LENGTH, KEYPAIR_LENGTH, SIGNATURE_LENGTH};
//! # let mut csprng = OsRng;
//! # let signing_key: SigningKey = SigningKey::generate(&mut csprng);
//! # let message: &[u8] = b"This is a test of the tsunami alert system.";
//! # let signature: Signature = signing_key.sign(message);
//!
//! let verifying_key_bytes: [u8; PUBLIC_KEY_LENGTH] = signing_key.verifying_key().to_bytes();
//! let secret_key_bytes: [u8; SECRET_KEY_LENGTH] = signing_key.to_bytes();
//! let signing_key_bytes:    [u8; KEYPAIR_LENGTH]    = signing_key.to_keypair_bytes();
//! let signature_bytes:  [u8; SIGNATURE_LENGTH]  = signature.to_bytes();
//! # }
//! ```
//!
//! And similarly, decoded from bytes with `::from_bytes()`:
//!
#![cfg_attr(feature = "rand_core", doc = "```")]
#![cfg_attr(not(feature = "rand_core"), doc = "```ignore")]
//! # use core::convert::{TryFrom, TryInto};
//! # use rand::rngs::OsRng;
//! # use ed25519_dalek::{SigningKey, Signature, Signer, VerifyingKey, SecretKey, SignatureError};
//! # use ed25519_dalek::{PUBLIC_KEY_LENGTH, SECRET_KEY_LENGTH, KEYPAIR_LENGTH, SIGNATURE_LENGTH};
//! # fn do_test() -> Result<(SigningKey, VerifyingKey, Signature), SignatureError> {
//! # let mut csprng = OsRng;
//! # let signing_key_orig: SigningKey = SigningKey::generate(&mut csprng);
//! # let message: &[u8] = b"This is a test of the tsunami alert system.";
//! # let signature_orig: Signature = signing_key_orig.sign(message);
//! # let verifying_key_bytes: [u8; PUBLIC_KEY_LENGTH] = signing_key_orig.verifying_key().to_bytes();
//! # let signing_key_bytes: [u8; SECRET_KEY_LENGTH] = signing_key_orig.to_bytes();
//! # let signature_bytes:  [u8; SIGNATURE_LENGTH]  = signature_orig.to_bytes();
//! #
//! let verifying_key: VerifyingKey = VerifyingKey::from_bytes(&verifying_key_bytes)?;
//! let signing_key: SigningKey = SigningKey::from_bytes(&signing_key_bytes);
//! let signature: Signature = Signature::try_from(&signature_bytes[..])?;
//! #
//! # Ok((signing_key, verifying_key, signature))
//! # }
//! # fn main() {
//! #     do_test();
//! # }
//! ```
//!
//! ### PKCS#8 Key Encoding
//!
//! PKCS#8 is a private key format with support for multiple algorithms.
//! It can be encoded as binary (DER) or text (PEM).
//!
//! You can recognize PEM-encoded PKCS#8 keys by the following:
//!
//! ```text
//! -----BEGIN PRIVATE KEY-----
//! ```
//!
//! To use PKCS#8, you need to enable the `pkcs8` crate feature.
//!
//! The following traits can be used to decode/encode [`SigningKey`] and
//! [`VerifyingKey`] as PKCS#8. Note that [`pkcs8`] is re-exported from the
//! toplevel of the crate:
//!
//! - [`pkcs8::DecodePrivateKey`]: decode private keys from PKCS#8
//! - [`pkcs8::EncodePrivateKey`]: encode private keys to PKCS#8
//! - [`pkcs8::DecodePublicKey`]: decode public keys from PKCS#8
//! - [`pkcs8::EncodePublicKey`]: encode public keys to PKCS#8
//!
//! #### Example
//!
//! NOTE: this requires the `pem` crate feature.
//!
#![cfg_attr(feature = "pem", doc = "```")]
#![cfg_attr(not(feature = "pem"), doc = "```ignore")]
//! use ed25519_dalek::{VerifyingKey, pkcs8::DecodePublicKey};
//!
//! let pem = "-----BEGIN PUBLIC KEY-----
//! MCowBQYDK2VwAyEAGb9ECWmEzf6FQbrBZ9w7lshQhqowtrbLDFw4rXAxZuE=
//! -----END PUBLIC KEY-----";
//!
//! let verifying_key = VerifyingKey::from_public_key_pem(pem)
//!     .expect("invalid public key PEM");
//! ```
//!
//! ### Using Serde
//!
//! If you prefer the bytes to be wrapped in another serialisation format, all
//! types additionally come with built-in [serde](https://serde.rs) support by
//! building `ed25519-dalek` via:
//!
//! ```bash
//! $ cargo build --features="serde"
//! ```
//!
//! They can be then serialised into any of the wire formats which serde supports.
//! For example, using [bincode](https://github.com/TyOverby/bincode):
//!
#![cfg_attr(all(feature = "rand_core", feature = "serde"), doc = "```")]
#![cfg_attr(not(all(feature = "rand_core", feature = "serde")), doc = "```ignore")]
//! # fn main() {
//! # use rand::rngs::OsRng;
//! # use ed25519_dalek::{SigningKey, Signature, Signer, Verifier, VerifyingKey};
//! use bincode::serialize;
//! # let mut csprng = OsRng;
//! # let signing_key: SigningKey = SigningKey::generate(&mut csprng);
//! # let message: &[u8] = b"This is a test of the tsunami alert system.";
//! # let signature: Signature = signing_key.sign(message);
//! # let verifying_key: VerifyingKey = signing_key.verifying_key();
//! # let verified: bool = verifying_key.verify(message, &signature).is_ok();
//!
//! let encoded_verifying_key: Vec<u8> = serialize(&verifying_key).unwrap();
//! let encoded_signature: Vec<u8> = serialize(&signature).unwrap();
//! # }
//! ```
//!
//! After sending the `encoded_verifying_key` and `encoded_signature`, the
//! recipient may deserialise them and verify:
//!
#![cfg_attr(all(feature = "rand_core", feature = "serde"), doc = "```")]
#![cfg_attr(not(all(feature = "rand_core", feature = "serde")), doc = "```ignore")]
//! # fn main() {
//! # use rand::rngs::OsRng;
//! # use ed25519_dalek::{SigningKey, Signature, Signer, Verifier, VerifyingKey};
//! # use bincode::serialize;
//! use bincode::deserialize;
//!
//! # let mut csprng = OsRng;
//! # let signing_key: SigningKey = SigningKey::generate(&mut csprng);
//! let message: &[u8] = b"This is a test of the tsunami alert system.";
//! # let signature: Signature = signing_key.sign(message);
//! # let verifying_key: VerifyingKey = signing_key.verifying_key();
//! # let verified: bool = verifying_key.verify(message, &signature).is_ok();
//! # let encoded_verifying_key: Vec<u8> = serialize(&verifying_key).unwrap();
//! # let encoded_signature: Vec<u8> = serialize(&signature).unwrap();
//! let decoded_verifying_key: VerifyingKey = deserialize(&encoded_verifying_key).unwrap();
//! let decoded_signature: Signature = deserialize(&encoded_signature).unwrap();
//!
//! # assert_eq!(verifying_key, decoded_verifying_key);
//! # assert_eq!(signature, decoded_signature);
//! #
//! let verified: bool = decoded_verifying_key.verify(&message, &decoded_signature).is_ok();
//!
//! assert!(verified);
//! # }
//! ```

#![no_std]
#![warn(future_incompatible, rust_2018_idioms)]
#![deny(missing_docs)] // refuse to compile if documentation is missing
#![deny(clippy::unwrap_used)] // don't allow unwrap
#![cfg_attr(not(test), forbid(unsafe_code))]
#![cfg_attr(docsrs, feature(doc_auto_cfg, doc_cfg, doc_cfg_hide))]
#![cfg_attr(docsrs, doc(cfg_hide(docsrs)))]

#[cfg(feature = "batch")]
extern crate alloc;

#[cfg(any(feature = "std", test))]
#[macro_use]
extern crate std;

pub use ed25519;

#[cfg(feature = "batch")]
mod batch;
mod constants;
#[cfg(feature = "digest")]
mod context;
mod errors;
mod signature;
mod signing;
mod verifying;

#[cfg(feature = "hazmat")]
pub mod hazmat;
#[cfg(not(feature = "hazmat"))]
mod hazmat;

#[cfg(feature = "digest")]
pub use curve25519_dalek::digest::Digest;
#[cfg(feature = "digest")]
pub use sha2::Sha512;

#[cfg(feature = "batch")]
pub use crate::batch::*;
pub use crate::constants::*;
#[cfg(feature = "digest")]
pub use crate::context::Context;
pub use crate::errors::*;
pub use crate::signing::*;
pub use crate::verifying::*;

// Re-export the `Signer` and `Verifier` traits from the `signature` crate
#[cfg(feature = "digest")]
pub use ed25519::signature::{DigestSigner, DigestVerifier};
pub use ed25519::signature::{Signer, Verifier};
pub use ed25519::Signature;

#[cfg(feature = "pkcs8")]
pub use ed25519::pkcs8;
