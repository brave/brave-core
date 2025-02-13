// Copyright 2015 Brian Smith.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

//! webpki: Web PKI X.509 Certificate Validation.
//!
//! See `EndEntityCert`'s documentation for a description of the certificate
//! processing steps necessary for a TLS connection.
//!
//! # Features
//!
//! | Feature | Description |
//! | ------- | ----------- |
//! | `alloc` | Enable features that require use of the heap. Currently all RSA signature algorithms require this feature. |
//! | `std` | Enable features that require libstd. Implies `alloc`. |
//! | `ring` | Enable use of the *ring* crate for cryptography. |
//! | `aws_lc_rs` | Enable use of the aws-lc-rs crate for cryptography. |

#![no_std]
#![warn(elided_lifetimes_in_paths, unreachable_pub, clippy::use_self)]
#![deny(missing_docs, clippy::as_conversions)]
#![allow(
    clippy::len_without_is_empty,
    clippy::new_without_default,
    clippy::single_match,
    clippy::single_match_else,
    clippy::type_complexity,
    clippy::upper_case_acronyms
)]
// Enable documentation for all features on docs.rs
#![cfg_attr(docsrs, feature(doc_cfg, doc_auto_cfg))]

#[cfg(any(feature = "std", test))]
extern crate std;

#[cfg(any(test, feature = "alloc"))]
#[cfg_attr(test, macro_use)]
extern crate alloc;

#[macro_use]
mod der;

#[cfg(feature = "aws_lc_rs")]
mod aws_lc_rs_algs;
mod cert;
mod end_entity;
mod error;
#[cfg(feature = "ring")]
mod ring_algs;
mod rpk_entity;
mod signed_data;
mod subject_name;
mod time;
mod trust_anchor;

mod crl;
mod verify_cert;
mod x509;

#[cfg(test)]
pub(crate) mod test_utils;

pub use {
    cert::Cert,
    crl::{
        BorrowedCertRevocationList, BorrowedRevokedCert, CertRevocationList, ExpirationPolicy,
        RevocationCheckDepth, RevocationOptions, RevocationOptionsBuilder, RevocationReason,
        UnknownStatusPolicy,
    },
    end_entity::EndEntityCert,
    error::{DerTypeId, Error},
    rpk_entity::RawPublicKeyEntity,
    signed_data::alg_id,
    trust_anchor::anchor_from_trusted_cert,
    verify_cert::KeyUsage,
    verify_cert::VerifiedPath,
};

pub use pki_types as types;

#[cfg(feature = "alloc")]
pub use crl::{OwnedCertRevocationList, OwnedRevokedCert};

#[cfg(feature = "ring")]
/// Signature verification algorithm implementations using the *ring* crypto library.
pub mod ring {
    pub use super::ring_algs::{
        ECDSA_P256_SHA256, ECDSA_P256_SHA384, ECDSA_P384_SHA256, ECDSA_P384_SHA384, ED25519,
    };

    #[cfg(feature = "alloc")]
    pub use super::ring_algs::{
        RSA_PKCS1_2048_8192_SHA256, RSA_PKCS1_2048_8192_SHA384, RSA_PKCS1_2048_8192_SHA512,
        RSA_PKCS1_3072_8192_SHA384, RSA_PSS_2048_8192_SHA256_LEGACY_KEY,
        RSA_PSS_2048_8192_SHA384_LEGACY_KEY, RSA_PSS_2048_8192_SHA512_LEGACY_KEY,
    };
}

#[cfg(feature = "aws_lc_rs")]
/// Signature verification algorithm implementations using the aws-lc-rs crypto library.
pub mod aws_lc_rs {
    pub use super::aws_lc_rs_algs::{
        ECDSA_P256_SHA256, ECDSA_P256_SHA384, ECDSA_P384_SHA256, ECDSA_P384_SHA384,
        ECDSA_P521_SHA256, ECDSA_P521_SHA384, ECDSA_P521_SHA512, ED25519,
        RSA_PKCS1_2048_8192_SHA256, RSA_PKCS1_2048_8192_SHA384, RSA_PKCS1_2048_8192_SHA512,
        RSA_PKCS1_3072_8192_SHA384, RSA_PSS_2048_8192_SHA256_LEGACY_KEY,
        RSA_PSS_2048_8192_SHA384_LEGACY_KEY, RSA_PSS_2048_8192_SHA512_LEGACY_KEY,
    };
}

/// An array of all the verification algorithms exported by this crate.
///
/// This will be empty if the crate is built without the `ring` and `aws_lc_rs` features.
pub static ALL_VERIFICATION_ALGS: &[&dyn types::SignatureVerificationAlgorithm] = &[
    #[cfg(feature = "ring")]
    ring::ECDSA_P256_SHA256,
    #[cfg(feature = "ring")]
    ring::ECDSA_P256_SHA384,
    #[cfg(feature = "ring")]
    ring::ECDSA_P384_SHA256,
    #[cfg(feature = "ring")]
    ring::ECDSA_P384_SHA384,
    #[cfg(feature = "ring")]
    ring::ED25519,
    #[cfg(all(feature = "ring", feature = "alloc"))]
    ring::RSA_PKCS1_2048_8192_SHA256,
    #[cfg(all(feature = "ring", feature = "alloc"))]
    ring::RSA_PKCS1_2048_8192_SHA384,
    #[cfg(all(feature = "ring", feature = "alloc"))]
    ring::RSA_PKCS1_2048_8192_SHA512,
    #[cfg(all(feature = "ring", feature = "alloc"))]
    ring::RSA_PKCS1_3072_8192_SHA384,
    #[cfg(all(feature = "ring", feature = "alloc"))]
    ring::RSA_PSS_2048_8192_SHA256_LEGACY_KEY,
    #[cfg(all(feature = "ring", feature = "alloc"))]
    ring::RSA_PSS_2048_8192_SHA384_LEGACY_KEY,
    #[cfg(all(feature = "ring", feature = "alloc"))]
    ring::RSA_PSS_2048_8192_SHA512_LEGACY_KEY,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::ECDSA_P256_SHA256,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::ECDSA_P256_SHA384,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::ECDSA_P384_SHA256,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::ECDSA_P384_SHA384,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::ECDSA_P521_SHA256,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::ECDSA_P521_SHA384,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::ECDSA_P521_SHA512,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::ED25519,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::RSA_PKCS1_2048_8192_SHA256,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::RSA_PKCS1_2048_8192_SHA384,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::RSA_PKCS1_2048_8192_SHA512,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::RSA_PKCS1_3072_8192_SHA384,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::RSA_PSS_2048_8192_SHA256_LEGACY_KEY,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::RSA_PSS_2048_8192_SHA384_LEGACY_KEY,
    #[cfg(feature = "aws_lc_rs")]
    aws_lc_rs::RSA_PSS_2048_8192_SHA512_LEGACY_KEY,
];

fn public_values_eq(a: untrusted::Input<'_>, b: untrusted::Input<'_>) -> bool {
    a.as_slice_less_safe() == b.as_slice_less_safe()
}
