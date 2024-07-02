// -*- mode: rust; -*-
//
// This file is part of ed25519-dalek.
// Copyright (c) 2017-2019 isis lovecruft
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>

//! An ed25519 signature.

use core::convert::TryFrom;
use core::fmt::Debug;

use curve25519_dalek::edwards::CompressedEdwardsY;
use curve25519_dalek::scalar::Scalar;

use crate::constants::*;
use crate::errors::*;

/// An ed25519 signature.
///
/// # Note
///
/// These signatures, unlike the ed25519 signature reference implementation, are
/// "detached"—that is, they do **not** include a copy of the message which has
/// been signed.
#[allow(non_snake_case)]
#[derive(Copy, Eq, PartialEq)]
pub(crate) struct InternalSignature {
    /// `R` is an `EdwardsPoint`, formed by using an hash function with
    /// 512-bits output to produce the digest of:
    ///
    /// - the nonce half of the `ExpandedSecretKey`, and
    /// - the message to be signed.
    ///
    /// This digest is then interpreted as a `Scalar` and reduced into an
    /// element in ℤ/lℤ.  The scalar is then multiplied by the distinguished
    /// basepoint to produce `R`, and `EdwardsPoint`.
    pub(crate) R: CompressedEdwardsY,

    /// `s` is a `Scalar`, formed by using an hash function with 512-bits output
    /// to produce the digest of:
    ///
    /// - the `r` portion of this `Signature`,
    /// - the `PublicKey` which should be used to verify this `Signature`, and
    /// - the message to be signed.
    ///
    /// This digest is then interpreted as a `Scalar` and reduced into an
    /// element in ℤ/lℤ.
    pub(crate) s: Scalar,
}

impl Clone for InternalSignature {
    fn clone(&self) -> Self {
        *self
    }
}

impl Debug for InternalSignature {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        write!(f, "Signature( R: {:?}, s: {:?} )", &self.R, &self.s)
    }
}

/// Ensures that the scalar `s` of a signature is within the bounds [0, 2^253).
///
/// **Unsafe**: This version of `check_scalar` permits signature malleability. See README.
#[cfg(feature = "legacy_compatibility")]
#[inline(always)]
fn check_scalar(bytes: [u8; 32]) -> Result<Scalar, SignatureError> {
    // The highest 3 bits must not be set.  No other checking for the
    // remaining 2^253 - 2^252 + 27742317777372353535851937790883648493
    // potential non-reduced scalars is performed.
    //
    // This is compatible with ed25519-donna and libsodium when
    // -DED25519_COMPAT is NOT specified.
    if bytes[31] & 224 != 0 {
        return Err(InternalError::ScalarFormat.into());
    }

    // You cannot do arithmetic with scalars construct with Scalar::from_bits. We only use this
    // scalar for EdwardsPoint::vartime_double_scalar_mul_basepoint, which is an accepted usecase.
    // The `from_bits` method is deprecated because it's unsafe. We know this.
    #[allow(deprecated)]
    Ok(Scalar::from_bits(bytes))
}

/// Ensures that the scalar `s` of a signature is within the bounds [0, ℓ)
#[cfg(not(feature = "legacy_compatibility"))]
#[inline(always)]
fn check_scalar(bytes: [u8; 32]) -> Result<Scalar, SignatureError> {
    match Scalar::from_canonical_bytes(bytes).into() {
        None => Err(InternalError::ScalarFormat.into()),
        Some(x) => Ok(x),
    }
}

impl InternalSignature {
    /// Construct a `Signature` from a slice of bytes.
    ///
    /// # Scalar Malleability Checking
    ///
    /// As originally specified in the ed25519 paper (cf. the "Malleability"
    /// section of the README in this repo), no checks whatsoever were performed
    /// for signature malleability.
    ///
    /// Later, a semi-functional, hacky check was added to most libraries to
    /// "ensure" that the scalar portion, `s`, of the signature was reduced `mod
    /// \ell`, the order of the basepoint:
    ///
    /// ```ignore
    /// if signature.s[31] & 224 != 0 {
    ///     return Err();
    /// }
    /// ```
    ///
    /// This bit-twiddling ensures that the most significant three bits of the
    /// scalar are not set:
    ///
    /// ```python,ignore
    /// >>> 0b00010000 & 224
    /// 0
    /// >>> 0b00100000 & 224
    /// 32
    /// >>> 0b01000000 & 224
    /// 64
    /// >>> 0b10000000 & 224
    /// 128
    /// ```
    ///
    /// However, this check is hacky and insufficient to check that the scalar is
    /// fully reduced `mod \ell = 2^252 + 27742317777372353535851937790883648493` as
    /// it leaves us with a guanteed bound of 253 bits.  This means that there are
    /// `2^253 - 2^252 + 2774231777737235353585193779088364849311` remaining scalars
    /// which could cause malleabilllity.
    ///
    /// RFC8032 [states](https://tools.ietf.org/html/rfc8032#section-5.1.7):
    ///
    /// > To verify a signature on a message M using public key A, [...]
    /// > first split the signature into two 32-octet halves.  Decode the first
    /// > half as a point R, and the second half as an integer S, in the range
    /// > 0 <= s < L.  Decode the public key A as point A'.  If any of the
    /// > decodings fail (including S being out of range), the signature is
    /// > invalid.
    ///
    /// However, by the time this was standardised, most libraries in use were
    /// only checking the most significant three bits.  (See also the
    /// documentation for [`crate::VerifyingKey::verify_strict`].)
    #[inline]
    #[allow(non_snake_case)]
    pub fn from_bytes(bytes: &[u8; SIGNATURE_LENGTH]) -> Result<InternalSignature, SignatureError> {
        // TODO: Use bytes.split_array_ref once it’s in MSRV.
        let mut R_bytes: [u8; 32] = [0u8; 32];
        let mut s_bytes: [u8; 32] = [0u8; 32];
        R_bytes.copy_from_slice(&bytes[00..32]);
        s_bytes.copy_from_slice(&bytes[32..64]);

        Ok(InternalSignature {
            R: CompressedEdwardsY(R_bytes),
            s: check_scalar(s_bytes)?,
        })
    }
}

impl TryFrom<&ed25519::Signature> for InternalSignature {
    type Error = SignatureError;

    fn try_from(sig: &ed25519::Signature) -> Result<InternalSignature, SignatureError> {
        InternalSignature::from_bytes(&sig.to_bytes())
    }
}

impl From<InternalSignature> for ed25519::Signature {
    fn from(sig: InternalSignature) -> ed25519::Signature {
        ed25519::Signature::from_components(*sig.R.as_bytes(), *sig.s.as_bytes())
    }
}
