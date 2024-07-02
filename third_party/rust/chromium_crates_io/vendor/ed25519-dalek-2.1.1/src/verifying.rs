// -*- mode: rust; -*-
//
// This file is part of ed25519-dalek.
// Copyright (c) 2017-2019 isis lovecruft
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>

//! ed25519 public keys.

use core::convert::TryFrom;
use core::fmt::Debug;
use core::hash::{Hash, Hasher};

use curve25519_dalek::{
    digest::{generic_array::typenum::U64, Digest},
    edwards::{CompressedEdwardsY, EdwardsPoint},
    montgomery::MontgomeryPoint,
    scalar::Scalar,
};

use ed25519::signature::Verifier;

use sha2::Sha512;

#[cfg(feature = "pkcs8")]
use ed25519::pkcs8;

#[cfg(feature = "serde")]
use serde::{Deserialize, Deserializer, Serialize, Serializer};

#[cfg(feature = "digest")]
use crate::context::Context;
#[cfg(feature = "digest")]
use signature::DigestVerifier;

use crate::{
    constants::PUBLIC_KEY_LENGTH,
    errors::{InternalError, SignatureError},
    hazmat::ExpandedSecretKey,
    signature::InternalSignature,
    signing::SigningKey,
};

/// An ed25519 public key.
///
/// # Note
///
/// The `Eq` and `Hash` impls here use the compressed Edwards y encoding, _not_ the algebraic
/// representation. This means if this `VerifyingKey` is non-canonically encoded, it will be
/// considered unequal to the other equivalent encoding, despite the two representing the same
/// point. More encoding details can be found
/// [here](https://hdevalence.ca/blog/2020-10-04-its-25519am).
/// If you want to make sure that signatures produced with respect to those sorts of public keys
/// are rejected, use [`VerifyingKey::verify_strict`].
// Invariant: VerifyingKey.1 is always the decompression of VerifyingKey.0
#[derive(Copy, Clone, Default, Eq)]
pub struct VerifyingKey {
    /// Serialized compressed Edwards-y point.
    pub(crate) compressed: CompressedEdwardsY,

    /// Decompressed Edwards point used for curve arithmetic operations.
    pub(crate) point: EdwardsPoint,
}

impl Debug for VerifyingKey {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "VerifyingKey({:?}), {:?})", self.compressed, self.point)
    }
}

impl AsRef<[u8]> for VerifyingKey {
    fn as_ref(&self) -> &[u8] {
        self.as_bytes()
    }
}

impl Hash for VerifyingKey {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.as_bytes().hash(state);
    }
}

impl PartialEq<VerifyingKey> for VerifyingKey {
    fn eq(&self, other: &VerifyingKey) -> bool {
        self.as_bytes() == other.as_bytes()
    }
}

impl From<&ExpandedSecretKey> for VerifyingKey {
    /// Derive this public key from its corresponding `ExpandedSecretKey`.
    fn from(expanded_secret_key: &ExpandedSecretKey) -> VerifyingKey {
        VerifyingKey::from(EdwardsPoint::mul_base(&expanded_secret_key.scalar))
    }
}

impl From<&SigningKey> for VerifyingKey {
    fn from(signing_key: &SigningKey) -> VerifyingKey {
        signing_key.verifying_key()
    }
}

impl From<EdwardsPoint> for VerifyingKey {
    fn from(point: EdwardsPoint) -> VerifyingKey {
        VerifyingKey {
            point,
            compressed: point.compress(),
        }
    }
}

impl VerifyingKey {
    /// Convert this public key to a byte array.
    #[inline]
    pub fn to_bytes(&self) -> [u8; PUBLIC_KEY_LENGTH] {
        self.compressed.to_bytes()
    }

    /// View this public key as a byte array.
    #[inline]
    pub fn as_bytes(&self) -> &[u8; PUBLIC_KEY_LENGTH] {
        &(self.compressed).0
    }

    /// Construct a `VerifyingKey` from a slice of bytes.
    ///
    /// # Warning
    ///
    /// The caller is responsible for ensuring that the bytes passed into this
    /// method actually represent a `curve25519_dalek::curve::CompressedEdwardsY`
    /// and that said compressed point is actually a point on the curve.
    ///
    /// # Example
    ///
    /// ```
    /// use ed25519_dalek::VerifyingKey;
    /// use ed25519_dalek::PUBLIC_KEY_LENGTH;
    /// use ed25519_dalek::SignatureError;
    ///
    /// # fn doctest() -> Result<VerifyingKey, SignatureError> {
    /// let public_key_bytes: [u8; PUBLIC_KEY_LENGTH] = [
    ///    215,  90, 152,   1, 130, 177,  10, 183, 213,  75, 254, 211, 201, 100,   7,  58,
    ///     14, 225, 114, 243, 218, 166,  35,  37, 175,   2,  26, 104, 247,   7,   81, 26];
    ///
    /// let public_key = VerifyingKey::from_bytes(&public_key_bytes)?;
    /// #
    /// # Ok(public_key)
    /// # }
    /// #
    /// # fn main() {
    /// #     doctest();
    /// # }
    /// ```
    ///
    /// # Returns
    ///
    /// A `Result` whose okay value is an EdDSA `VerifyingKey` or whose error value
    /// is a `SignatureError` describing the error that occurred.
    #[inline]
    pub fn from_bytes(bytes: &[u8; PUBLIC_KEY_LENGTH]) -> Result<VerifyingKey, SignatureError> {
        let compressed = CompressedEdwardsY(*bytes);
        let point = compressed
            .decompress()
            .ok_or(InternalError::PointDecompression)?;

        // Invariant: VerifyingKey.1 is always the decompression of VerifyingKey.0
        Ok(VerifyingKey { compressed, point })
    }

    /// Create a verifying context that can be used for Ed25519ph with
    /// [`DigestVerifier`].
    #[cfg(feature = "digest")]
    pub fn with_context<'k, 'v>(
        &'k self,
        context_value: &'v [u8],
    ) -> Result<Context<'k, 'v, Self>, SignatureError> {
        Context::new(self, context_value)
    }

    /// Returns whether this is a _weak_ public key, i.e., if this public key has low order.
    ///
    /// A weak public key can be used to generate a signature that's valid for almost every
    /// message. [`Self::verify_strict`] denies weak keys, but if you want to check for this
    /// property before verification, then use this method.
    pub fn is_weak(&self) -> bool {
        self.point.is_small_order()
    }

    // A helper function that computes `H(R || A || M)` where `H` is the 512-bit hash function
    // given by `CtxDigest` (this is SHA-512 in spec-compliant Ed25519). If `context.is_some()`,
    // this does the prehashed variant of the computation using its contents.
    #[allow(non_snake_case)]
    fn compute_challenge<CtxDigest>(
        context: Option<&[u8]>,
        R: &CompressedEdwardsY,
        A: &CompressedEdwardsY,
        M: &[u8],
    ) -> Scalar
    where
        CtxDigest: Digest<OutputSize = U64>,
    {
        let mut h = CtxDigest::new();
        if let Some(c) = context {
            h.update(b"SigEd25519 no Ed25519 collisions");
            h.update([1]); // Ed25519ph
            h.update([c.len() as u8]);
            h.update(c);
        }
        h.update(R.as_bytes());
        h.update(A.as_bytes());
        h.update(M);

        Scalar::from_hash(h)
    }

    // Helper function for verification. Computes the _expected_ R component of the signature. The
    // caller compares this to the real R component.  If `context.is_some()`, this does the
    // prehashed variant of the computation using its contents.
    // Note that this returns the compressed form of R and the caller does a byte comparison. This
    // means that all our verification functions do not accept non-canonically encoded R values.
    // See the validation criteria blog post for more details:
    //     https://hdevalence.ca/blog/2020-10-04-its-25519am
    #[allow(non_snake_case)]
    fn recompute_R<CtxDigest>(
        &self,
        context: Option<&[u8]>,
        signature: &InternalSignature,
        M: &[u8],
    ) -> CompressedEdwardsY
    where
        CtxDigest: Digest<OutputSize = U64>,
    {
        let k = Self::compute_challenge::<CtxDigest>(context, &signature.R, &self.compressed, M);
        let minus_A: EdwardsPoint = -self.point;
        // Recall the (non-batched) verification equation: -[k]A + [s]B = R
        EdwardsPoint::vartime_double_scalar_mul_basepoint(&k, &(minus_A), &signature.s).compress()
    }

    /// The ordinary non-batched Ed25519 verification check, rejecting non-canonical R values. (see
    /// [`Self::recompute_R`]). `CtxDigest` is the digest used to calculate the pseudorandomness
    /// needed for signing. According to the spec, `CtxDigest = Sha512`.
    ///
    /// This definition is loose in its parameters so that end-users of the `hazmat` module can
    /// change how the `ExpandedSecretKey` is calculated and which hash function to use.
    #[allow(non_snake_case)]
    pub(crate) fn raw_verify<CtxDigest>(
        &self,
        message: &[u8],
        signature: &ed25519::Signature,
    ) -> Result<(), SignatureError>
    where
        CtxDigest: Digest<OutputSize = U64>,
    {
        let signature = InternalSignature::try_from(signature)?;

        let expected_R = self.recompute_R::<CtxDigest>(None, &signature, message);
        if expected_R == signature.R {
            Ok(())
        } else {
            Err(InternalError::Verify.into())
        }
    }

    /// The prehashed non-batched Ed25519 verification check, rejecting non-canonical R values.
    /// (see [`Self::recompute_R`]). `CtxDigest` is the digest used to calculate the
    /// pseudorandomness needed for signing. `MsgDigest` is the digest used to hash the signed
    /// message. According to the spec, `MsgDigest = CtxDigest = Sha512`.
    ///
    /// This definition is loose in its parameters so that end-users of the `hazmat` module can
    /// change how the `ExpandedSecretKey` is calculated and which hash function to use.
    #[cfg(feature = "digest")]
    #[allow(non_snake_case)]
    pub(crate) fn raw_verify_prehashed<CtxDigest, MsgDigest>(
        &self,
        prehashed_message: MsgDigest,
        context: Option<&[u8]>,
        signature: &ed25519::Signature,
    ) -> Result<(), SignatureError>
    where
        CtxDigest: Digest<OutputSize = U64>,
        MsgDigest: Digest<OutputSize = U64>,
    {
        let signature = InternalSignature::try_from(signature)?;

        let ctx: &[u8] = context.unwrap_or(b"");
        debug_assert!(
            ctx.len() <= 255,
            "The context must not be longer than 255 octets."
        );

        let message = prehashed_message.finalize();
        let expected_R = self.recompute_R::<CtxDigest>(Some(ctx), &signature, &message);

        if expected_R == signature.R {
            Ok(())
        } else {
            Err(InternalError::Verify.into())
        }
    }

    /// Verify a `signature` on a `prehashed_message` using the Ed25519ph algorithm.
    ///
    /// # Inputs
    ///
    /// * `prehashed_message` is an instantiated hash digest with 512-bits of
    ///   output which has had the message to be signed previously fed into its
    ///   state.
    /// * `context` is an optional context string, up to 255 bytes inclusive,
    ///   which may be used to provide additional domain separation.  If not
    ///   set, this will default to an empty string.
    /// * `signature` is a purported Ed25519ph signature on the `prehashed_message`.
    ///
    /// # Returns
    ///
    /// Returns `true` if the `signature` was a valid signature created by this
    /// [`SigningKey`] on the `prehashed_message`.
    ///
    /// # Note
    ///
    /// The RFC only permits SHA-512 to be used for prehashing, i.e., `MsgDigest = Sha512`. This
    /// function technically works, and is probably safe to use, with any secure hash function with
    /// 512-bit digests, but anything outside of SHA-512 is NOT specification-compliant. We expose
    /// [`crate::Sha512`] for user convenience.
    #[cfg(feature = "digest")]
    #[allow(non_snake_case)]
    pub fn verify_prehashed<MsgDigest>(
        &self,
        prehashed_message: MsgDigest,
        context: Option<&[u8]>,
        signature: &ed25519::Signature,
    ) -> Result<(), SignatureError>
    where
        MsgDigest: Digest<OutputSize = U64>,
    {
        self.raw_verify_prehashed::<Sha512, MsgDigest>(prehashed_message, context, signature)
    }

    /// Strictly verify a signature on a message with this keypair's public key.
    ///
    /// # On The (Multiple) Sources of Malleability in Ed25519 Signatures
    ///
    /// This version of verification is technically non-RFC8032 compliant.  The
    /// following explains why.
    ///
    /// 1. Scalar Malleability
    ///
    /// The authors of the RFC explicitly stated that verification of an ed25519
    /// signature must fail if the scalar `s` is not properly reduced mod $\ell$:
    ///
    /// > To verify a signature on a message M using public key A, with F
    /// > being 0 for Ed25519ctx, 1 for Ed25519ph, and if Ed25519ctx or
    /// > Ed25519ph is being used, C being the context, first split the
    /// > signature into two 32-octet halves.  Decode the first half as a
    /// > point R, and the second half as an integer S, in the range
    /// > 0 <= s < L.  Decode the public key A as point A'.  If any of the
    /// > decodings fail (including S being out of range), the signature is
    /// > invalid.)
    ///
    /// All `verify_*()` functions within ed25519-dalek perform this check.
    ///
    /// 2. Point malleability
    ///
    /// The authors of the RFC added in a malleability check to step #3 in
    /// §5.1.7, for small torsion components in the `R` value of the signature,
    /// *which is not strictly required*, as they state:
    ///
    /// > Check the group equation \[8\]\[S\]B = \[8\]R + \[8\]\[k\]A'.  It's
    /// > sufficient, but not required, to instead check \[S\]B = R + \[k\]A'.
    ///
    /// # History of Malleability Checks
    ///
    /// As originally defined (cf. the "Malleability" section in the README of
    /// this repo), ed25519 signatures didn't consider *any* form of
    /// malleability to be an issue.  Later the scalar malleability was
    /// considered important.  Still later, particularly with interests in
    /// cryptocurrency design and in unique identities (e.g. for Signal users,
    /// Tor onion services, etc.), the group element malleability became a
    /// concern.
    ///
    /// However, libraries had already been created to conform to the original
    /// definition.  One well-used library in particular even implemented the
    /// group element malleability check, *but only for batch verification*!
    /// Which meant that even using the same library, a single signature could
    /// verify fine individually, but suddenly, when verifying it with a bunch
    /// of other signatures, the whole batch would fail!
    ///
    /// # "Strict" Verification
    ///
    /// This method performs *both* of the above signature malleability checks.
    ///
    /// It must be done as a separate method because one doesn't simply get to
    /// change the definition of a cryptographic primitive ten years
    /// after-the-fact with zero consideration for backwards compatibility in
    /// hardware and protocols which have it already have the older definition
    /// baked in.
    ///
    /// # Return
    ///
    /// Returns `Ok(())` if the signature is valid, and `Err` otherwise.
    #[allow(non_snake_case)]
    pub fn verify_strict(
        &self,
        message: &[u8],
        signature: &ed25519::Signature,
    ) -> Result<(), SignatureError> {
        let signature = InternalSignature::try_from(signature)?;

        let signature_R = signature
            .R
            .decompress()
            .ok_or_else(|| SignatureError::from(InternalError::Verify))?;

        // Logical OR is fine here as we're not trying to be constant time.
        if signature_R.is_small_order() || self.point.is_small_order() {
            return Err(InternalError::Verify.into());
        }

        let expected_R = self.recompute_R::<Sha512>(None, &signature, message);
        if expected_R == signature.R {
            Ok(())
        } else {
            Err(InternalError::Verify.into())
        }
    }

    /// Verify a `signature` on a `prehashed_message` using the Ed25519ph algorithm,
    /// using strict signture checking as defined by [`Self::verify_strict`].
    ///
    /// # Inputs
    ///
    /// * `prehashed_message` is an instantiated hash digest with 512-bits of
    ///   output which has had the message to be signed previously fed into its
    ///   state.
    /// * `context` is an optional context string, up to 255 bytes inclusive,
    ///   which may be used to provide additional domain separation.  If not
    ///   set, this will default to an empty string.
    /// * `signature` is a purported Ed25519ph signature on the `prehashed_message`.
    ///
    /// # Returns
    ///
    /// Returns `true` if the `signature` was a valid signature created by this
    /// [`SigningKey`] on the `prehashed_message`.
    ///
    /// # Note
    ///
    /// The RFC only permits SHA-512 to be used for prehashing, i.e., `MsgDigest = Sha512`. This
    /// function technically works, and is probably safe to use, with any secure hash function with
    /// 512-bit digests, but anything outside of SHA-512 is NOT specification-compliant. We expose
    /// [`crate::Sha512`] for user convenience.
    #[cfg(feature = "digest")]
    #[allow(non_snake_case)]
    pub fn verify_prehashed_strict<MsgDigest>(
        &self,
        prehashed_message: MsgDigest,
        context: Option<&[u8]>,
        signature: &ed25519::Signature,
    ) -> Result<(), SignatureError>
    where
        MsgDigest: Digest<OutputSize = U64>,
    {
        let signature = InternalSignature::try_from(signature)?;

        let ctx: &[u8] = context.unwrap_or(b"");
        debug_assert!(
            ctx.len() <= 255,
            "The context must not be longer than 255 octets."
        );

        let signature_R = signature
            .R
            .decompress()
            .ok_or_else(|| SignatureError::from(InternalError::Verify))?;

        // Logical OR is fine here as we're not trying to be constant time.
        if signature_R.is_small_order() || self.point.is_small_order() {
            return Err(InternalError::Verify.into());
        }

        let message = prehashed_message.finalize();
        let expected_R = self.recompute_R::<Sha512>(Some(ctx), &signature, &message);

        if expected_R == signature.R {
            Ok(())
        } else {
            Err(InternalError::Verify.into())
        }
    }

    /// Convert this verifying key into Montgomery form.
    ///
    /// This can be used for performing X25519 Diffie-Hellman using Ed25519 keys. The output of
    /// this function is a valid X25519 public key whose secret key is `sk.to_scalar_bytes()`,
    /// where `sk` is a valid signing key for this `VerifyingKey`.
    ///
    /// # Note
    ///
    /// We do NOT recommend this usage of a signing/verifying key. Signing keys are usually
    /// long-term keys, while keys used for key exchange should rather be ephemeral. If you can
    /// help it, use a separate key for encryption.
    ///
    /// For more information on the security of systems which use the same keys for both signing
    /// and Diffie-Hellman, see the paper
    /// [On using the same key pair for Ed25519 and an X25519 based KEM](https://eprint.iacr.org/2021/509).
    pub fn to_montgomery(&self) -> MontgomeryPoint {
        self.point.to_montgomery()
    }
}

impl Verifier<ed25519::Signature> for VerifyingKey {
    /// Verify a signature on a message with this keypair's public key.
    ///
    /// # Return
    ///
    /// Returns `Ok(())` if the signature is valid, and `Err` otherwise.
    fn verify(&self, message: &[u8], signature: &ed25519::Signature) -> Result<(), SignatureError> {
        self.raw_verify::<Sha512>(message, signature)
    }
}

/// Equivalent to [`VerifyingKey::verify_prehashed`] with `context` set to [`None`].
#[cfg(feature = "digest")]
impl<MsgDigest> DigestVerifier<MsgDigest, ed25519::Signature> for VerifyingKey
where
    MsgDigest: Digest<OutputSize = U64>,
{
    fn verify_digest(
        &self,
        msg_digest: MsgDigest,
        signature: &ed25519::Signature,
    ) -> Result<(), SignatureError> {
        self.verify_prehashed(msg_digest, None, signature)
    }
}

/// Equivalent to [`VerifyingKey::verify_prehashed`] with `context` set to [`Some`]
/// containing `self.value()`.
#[cfg(feature = "digest")]
impl<MsgDigest> DigestVerifier<MsgDigest, ed25519::Signature> for Context<'_, '_, VerifyingKey>
where
    MsgDigest: Digest<OutputSize = U64>,
{
    fn verify_digest(
        &self,
        msg_digest: MsgDigest,
        signature: &ed25519::Signature,
    ) -> Result<(), SignatureError> {
        self.key()
            .verify_prehashed(msg_digest, Some(self.value()), signature)
    }
}

impl TryFrom<&[u8]> for VerifyingKey {
    type Error = SignatureError;

    #[inline]
    fn try_from(bytes: &[u8]) -> Result<Self, Self::Error> {
        let bytes = bytes.try_into().map_err(|_| InternalError::BytesLength {
            name: "VerifyingKey",
            length: PUBLIC_KEY_LENGTH,
        })?;
        Self::from_bytes(bytes)
    }
}

#[cfg(all(feature = "alloc", feature = "pkcs8"))]
impl pkcs8::EncodePublicKey for VerifyingKey {
    fn to_public_key_der(&self) -> pkcs8::spki::Result<pkcs8::Document> {
        pkcs8::PublicKeyBytes::from(self).to_public_key_der()
    }
}

#[cfg(feature = "pkcs8")]
impl TryFrom<pkcs8::PublicKeyBytes> for VerifyingKey {
    type Error = pkcs8::spki::Error;

    fn try_from(pkcs8_key: pkcs8::PublicKeyBytes) -> pkcs8::spki::Result<Self> {
        VerifyingKey::try_from(&pkcs8_key)
    }
}

#[cfg(feature = "pkcs8")]
impl TryFrom<&pkcs8::PublicKeyBytes> for VerifyingKey {
    type Error = pkcs8::spki::Error;

    fn try_from(pkcs8_key: &pkcs8::PublicKeyBytes) -> pkcs8::spki::Result<Self> {
        VerifyingKey::from_bytes(pkcs8_key.as_ref()).map_err(|_| pkcs8::spki::Error::KeyMalformed)
    }
}

#[cfg(feature = "pkcs8")]
impl From<VerifyingKey> for pkcs8::PublicKeyBytes {
    fn from(verifying_key: VerifyingKey) -> pkcs8::PublicKeyBytes {
        pkcs8::PublicKeyBytes::from(&verifying_key)
    }
}

#[cfg(feature = "pkcs8")]
impl From<&VerifyingKey> for pkcs8::PublicKeyBytes {
    fn from(verifying_key: &VerifyingKey) -> pkcs8::PublicKeyBytes {
        pkcs8::PublicKeyBytes(verifying_key.to_bytes())
    }
}

#[cfg(feature = "pkcs8")]
impl TryFrom<pkcs8::spki::SubjectPublicKeyInfoRef<'_>> for VerifyingKey {
    type Error = pkcs8::spki::Error;

    fn try_from(public_key: pkcs8::spki::SubjectPublicKeyInfoRef<'_>) -> pkcs8::spki::Result<Self> {
        pkcs8::PublicKeyBytes::try_from(public_key)?.try_into()
    }
}

#[cfg(feature = "serde")]
impl Serialize for VerifyingKey {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_bytes(&self.as_bytes()[..])
    }
}

#[cfg(feature = "serde")]
impl<'d> Deserialize<'d> for VerifyingKey {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'d>,
    {
        struct VerifyingKeyVisitor;

        impl<'de> serde::de::Visitor<'de> for VerifyingKeyVisitor {
            type Value = VerifyingKey;

            fn expecting(&self, formatter: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                write!(formatter, concat!("An ed25519 verifying (public) key"))
            }

            fn visit_bytes<E: serde::de::Error>(self, bytes: &[u8]) -> Result<Self::Value, E> {
                VerifyingKey::try_from(bytes).map_err(E::custom)
            }

            fn visit_seq<A>(self, mut seq: A) -> Result<Self::Value, A::Error>
            where
                A: serde::de::SeqAccess<'de>,
            {
                let mut bytes = [0u8; 32];

                #[allow(clippy::needless_range_loop)]
                for i in 0..32 {
                    bytes[i] = seq
                        .next_element()?
                        .ok_or_else(|| serde::de::Error::invalid_length(i, &"expected 32 bytes"))?;
                }

                let remaining = (0..)
                    .map(|_| seq.next_element::<u8>())
                    .take_while(|el| matches!(el, Ok(Some(_))))
                    .count();

                if remaining > 0 {
                    return Err(serde::de::Error::invalid_length(
                        32 + remaining,
                        &"expected 32 bytes",
                    ));
                }

                VerifyingKey::try_from(&bytes[..]).map_err(serde::de::Error::custom)
            }
        }

        deserializer.deserialize_bytes(VerifyingKeyVisitor)
    }
}
