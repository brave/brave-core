// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

use crate::aws_lc::EVP_PKEY;
use crate::buffer::Buffer;
use crate::digest::Digest;
use crate::encoding::{AsDer, PublicKeyX509Der};
use crate::error::Unspecified;
use crate::evp_pkey::No_EVP_PKEY_CTX_consumer;
use crate::pqdsa::{parse_pqdsa_public_key, AlgorithmID};
use crate::ptr::LcPtr;
use crate::signature::{ParsedPublicKey, ParsedVerificationAlgorithm, VerificationAlgorithm};
use crate::{digest, sealed};
use core::fmt;
use core::fmt::{Debug, Formatter};
#[cfg(feature = "ring-sig-verify")]
use untrusted::Input;

/// A PQDSA verification algorithm.
#[derive(Debug, Eq, PartialEq)]
pub struct PqdsaVerificationAlgorithm {
    pub(crate) id: &'static AlgorithmID,
}

impl sealed::Sealed for PqdsaVerificationAlgorithm {}

/// A PQDSA signing algorithm.
#[derive(Debug, Eq, PartialEq)]
pub struct PqdsaSigningAlgorithm(pub(crate) &'static PqdsaVerificationAlgorithm);

impl PqdsaSigningAlgorithm {
    /// Returns the size of the signature in bytes.
    #[must_use]
    pub fn signature_len(&self) -> usize {
        self.0.id.signature_size_bytes()
    }

    /// Returns the size of the raw public key in bytes.
    #[must_use]
    pub fn public_key_len(&self) -> usize {
        self.0.id.pub_key_size_bytes()
    }

    /// Returns the size of the seed in bytes.
    ///
    /// See [`crate::signature::PqdsaKeyPair::from_seed`].
    #[must_use]
    pub fn seed_len(&self) -> usize {
        self.0.id.seed_size_bytes()
    }
}

/// A PQDSA public key.
#[derive(Clone)]
pub struct PublicKey {
    evp_pkey: LcPtr<EVP_PKEY>,
    pub(crate) octets: Box<[u8]>,
}
unsafe impl Send for PublicKey {}

unsafe impl Sync for PublicKey {}

impl PublicKey {
    pub(crate) fn from_private_evp_pkey(evp_pkey: &LcPtr<EVP_PKEY>) -> Result<Self, Unspecified> {
        let octets = evp_pkey.as_const().marshal_raw_public_key()?;
        Ok(Self {
            evp_pkey: evp_pkey.clone(),
            octets: octets.into_boxed_slice(),
        })
    }
}

impl ParsedVerificationAlgorithm for PqdsaVerificationAlgorithm {
    fn parsed_verify_sig(
        &self,
        public_key: &ParsedPublicKey,
        msg: &[u8],
        signature: &[u8],
    ) -> Result<(), Unspecified> {
        let evp_pkey = public_key.key();
        evp_pkey.verify(msg, None, No_EVP_PKEY_CTX_consumer, signature)
    }

    fn parsed_verify_digest_sig(
        &self,
        _public_key: &ParsedPublicKey,
        _digest: &Digest,
        _signature: &[u8],
    ) -> Result<(), Unspecified> {
        // This API cannot be used with ML-DSA, because a `Digest` cannot represent the
        // input that ML-DSA's digest-then-verify flow ("external mu") requires. That flow
        // verifies against the 64-byte message representative
        // mu = SHAKE256(SHAKE256(public_key) || 0x00 || len(ctx) || ctx || message),
        // which is a key-dependent XOF output rather than a fixed-output hash of the
        // message alone. Previously this path verified the digest bytes as if they were
        // a pure ML-DSA message, which is not a construction defined by FIPS 204.
        Err(Unspecified)
    }
}

impl VerificationAlgorithm for PqdsaVerificationAlgorithm {
    /// Verifies the signature of `msg` using the public key `public_key`.
    ///
    /// # Errors
    /// `error::Unspecified` if the signature is invalid.
    #[cfg(feature = "ring-sig-verify")]
    fn verify(
        &self,
        public_key: Input<'_>,
        msg: Input<'_>,
        signature: Input<'_>,
    ) -> Result<(), Unspecified> {
        self.verify_sig(
            public_key.as_slice_less_safe(),
            msg.as_slice_less_safe(),
            signature.as_slice_less_safe(),
        )
    }

    /// Verifies the signature for `msg` using the `public_key`.
    ///
    /// # Errors
    /// `error::Unspecified` if the signature is invalid.
    //
    // # FIPS
    // Approved for all supported algorithms: ML-DSA-44, ML-DSA-65, ML-DSA-87.
    fn verify_sig(
        &self,
        public_key: &[u8],
        msg: &[u8],
        signature: &[u8],
    ) -> Result<(), Unspecified> {
        let evp_pkey = parse_pqdsa_public_key(public_key, self.id)?;

        evp_pkey.verify(msg, None, No_EVP_PKEY_CTX_consumer, signature)
    }

    /// DO NOT USE. This function is required by `VerificationAlgorithm` but cannot be used
    /// with ML-DSA: a `Digest` cannot represent `mu`, the key-dependent "message
    /// representative" that ML-DSA's digest-then-verify flow ("external mu") operates on.
    /// See `parsed_verify_digest_sig` for details.
    ///
    /// # Errors
    /// Always returns `Unspecified`.
    fn verify_digest_sig(
        &self,
        _public_key: &[u8],
        _digest: &digest::Digest,
        _signature: &[u8],
    ) -> Result<(), Unspecified> {
        Err(Unspecified)
    }
}

impl AsRef<[u8]> for PublicKey {
    /// Serializes the public key as a raw byte string.
    fn as_ref(&self) -> &[u8] {
        self.octets.as_ref()
    }
}

impl AsDer<PublicKeyX509Der<'static>> for PublicKey {
    /// Provides the public key as a DER-encoded (X.509) `SubjectPublicKeyInfo` structure.
    /// # Errors
    /// Returns an error if the public key fails to marshal to X.509.
    fn as_der(&self) -> Result<PublicKeyX509Der<'static>, crate::error::Unspecified> {
        let der = self.evp_pkey.as_const().marshal_rfc5280_public_key()?;
        Ok(PublicKeyX509Der::from(Buffer::new(der)))
    }
}

impl Debug for PublicKey {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result<(), fmt::Error> {
        f.write_str(&format!(
            "PqdsaPublicKey(\"{}\")",
            crate::hex::encode(self.octets.as_ref())
        ))
    }
}
