//! Rerandomized FROST with Pallas curve.
#![allow(non_snake_case)]
#![deny(missing_docs)]

use std::collections::HashMap;

use group::GroupEncoding;
#[cfg(feature = "alloc")]
use group::{ff::Field as FFField, ff::PrimeField, Group as FFGroup};
use pasta_curves::pallas;

use frost_rerandomized::{
    frost_core::{frost, Ciphersuite, Field, FieldError, Group, GroupError},
    RandomizedParams,
};

use rand_core::{CryptoRng, RngCore};

use crate::{hash::HStar, orchard, private::Sealed};

/// An error type for the FROST(Pallas, BLAKE2b-512) ciphersuite.
pub type Error = frost_rerandomized::frost_core::Error<PallasBlake2b512>;

/// An implementation of the FROST(Pallas, BLAKE2b-512) ciphersuite scalar field.
#[derive(Clone, Copy)]
pub struct PallasScalarField;

impl Field for PallasScalarField {
    type Scalar = pallas::Scalar;

    type Serialization = [u8; 32];

    fn zero() -> Self::Scalar {
        Self::Scalar::zero()
    }

    fn one() -> Self::Scalar {
        Self::Scalar::one()
    }

    fn invert(scalar: &Self::Scalar) -> Result<Self::Scalar, FieldError> {
        // [`pallas::Scalar`]'s Eq/PartialEq does a constant-time comparison using
        // `ConstantTimeEq`
        if *scalar == <Self as Field>::zero() {
            Err(FieldError::InvalidZeroScalar)
        } else {
            Ok(Self::Scalar::invert(scalar).unwrap())
        }
    }

    fn random<R: RngCore + CryptoRng>(rng: &mut R) -> Self::Scalar {
        Self::Scalar::random(rng)
    }

    fn serialize(scalar: &Self::Scalar) -> Self::Serialization {
        // to_repr() endianess is implementation-specific, but this is OK since
        // it is specific to [`pallas::Scalar`] which uses little-endian and that
        // is what we want.
        scalar.to_repr()
    }

    fn little_endian_serialize(scalar: &Self::Scalar) -> Self::Serialization {
        Self::serialize(scalar)
    }

    fn deserialize(buf: &Self::Serialization) -> Result<Self::Scalar, FieldError> {
        match pallas::Scalar::from_repr(*buf).into() {
            Some(s) => Ok(s),
            None => Err(FieldError::MalformedScalar),
        }
    }
}

/// An implementation of the FROST(Pallas, BLAKE2b-512) ciphersuite group.
#[derive(Clone, Copy, PartialEq, Eq)]
pub struct PallasGroup;

impl Group for PallasGroup {
    type Field = PallasScalarField;

    type Element = pallas::Point;

    type Serialization = [u8; 32];

    fn cofactor() -> <Self::Field as Field>::Scalar {
        Self::Field::one()
    }

    fn identity() -> Self::Element {
        Self::Element::identity()
    }

    fn generator() -> Self::Element {
        orchard::SpendAuth::basepoint()
    }

    fn serialize(element: &Self::Element) -> Self::Serialization {
        element.to_bytes()
    }

    fn deserialize(buf: &Self::Serialization) -> Result<Self::Element, GroupError> {
        let point = Self::Element::from_bytes(buf);

        match Option::<Self::Element>::from(point) {
            Some(point) => {
                if point == Self::identity() {
                    Err(GroupError::InvalidIdentityElement)
                } else {
                    Ok(point)
                }
            }
            None => Err(GroupError::MalformedElement),
        }
    }
}

/// An implementation of the FROST(Pallas, BLAKE2b-512) ciphersuite.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct PallasBlake2b512;

impl Ciphersuite for PallasBlake2b512 {
    const ID: &'static str = "FROST(Pallas, BLAKE2b-512)";

    type Group = PallasGroup;

    type HashOutput = [u8; 64];

    type SignatureSerialization = [u8; 64];

    /// H1 for FROST(Pallas, BLAKE2b-512)
    fn H1(m: &[u8]) -> <<Self::Group as Group>::Field as Field>::Scalar {
        HStar::<orchard::SpendAuth>::new(b"FROST_RedPallasR")
            .update(m)
            .finalize()
    }

    /// H2 for FROST(Pallas, BLAKE2b-512)
    fn H2(m: &[u8]) -> <<Self::Group as Group>::Field as Field>::Scalar {
        HStar::<orchard::SpendAuth>::default().update(m).finalize()
    }

    /// H3 for FROST(Pallas, BLAKE2b-512)
    fn H3(m: &[u8]) -> <<Self::Group as Group>::Field as Field>::Scalar {
        HStar::<orchard::SpendAuth>::new(b"FROST_RedPallasN")
            .update(m)
            .finalize()
    }

    /// H4 for FROST(Pallas, BLAKE2b-512)
    fn H4(m: &[u8]) -> Self::HashOutput {
        let mut state = blake2b_simd::Params::new()
            .hash_length(64)
            .personal(b"FROST_RedPallasM")
            .to_state();
        *state.update(m).finalize().as_array()
    }

    /// H5 for FROST(Pallas, BLAKE2b-512)
    fn H5(m: &[u8]) -> Self::HashOutput {
        let mut state = blake2b_simd::Params::new()
            .hash_length(64)
            .personal(b"FROST_RedPallasC")
            .to_state();
        *state.update(m).finalize().as_array()
    }

    /// HDKG for FROST(Pallas, BLAKE2b-512)
    fn HDKG(m: &[u8]) -> Option<<<Self::Group as Group>::Field as Field>::Scalar> {
        Some(
            HStar::<orchard::SpendAuth>::new(b"FROST_RedPallasD")
                .update(m)
                .finalize(),
        )
    }
}

// Shorthand alias for the ciphersuite
type P = PallasBlake2b512;

/// A FROST(Pallas, BLAKE2b-512) participant identifier.
pub type Identifier = frost::Identifier<P>;

/// FROST(Pallas, BLAKE2b-512) keys, key generation, key shares.
pub mod keys {
    use std::collections::HashMap;

    use super::*;

    /// The identifier list to use when generating key shares.
    pub type IdentifierList<'a> = frost::keys::IdentifierList<'a, P>;

    /// Allows all participants' keys to be generated using a central, trusted
    /// dealer.
    pub fn generate_with_dealer<RNG: RngCore + CryptoRng>(
        max_signers: u16,
        min_signers: u16,
        identifiers: IdentifierList,
        mut rng: RNG,
    ) -> Result<(HashMap<Identifier, SecretShare>, PublicKeyPackage), Error> {
        frost::keys::generate_with_dealer(max_signers, min_signers, identifiers, &mut rng)
    }

    /// Secret and public key material generated by a dealer performing
    /// [`keygen_with_dealer`].
    ///
    /// # Security
    ///
    /// To derive a FROST(Pallas, BLAKE2b-512) keypair, the receiver of the [`SecretShare`] *must* call
    /// .into(), which under the hood also performs validation.
    pub type SecretShare = frost::keys::SecretShare<P>;

    /// A FROST(Pallas, BLAKE2b-512) keypair, which can be generated either by a trusted dealer or using
    /// a DKG.
    ///
    /// When using a central dealer, [`SecretShare`]s are distributed to
    /// participants, who then perform verification, before deriving
    /// [`KeyPackage`]s, which they store to later use during signing.
    pub type KeyPackage = frost::keys::KeyPackage<P>;

    /// Public data that contains all the signers' public keys as well as the
    /// group public key.
    ///
    /// Used for verification purposes before publishing a signature.
    pub type PublicKeyPackage = frost::keys::PublicKeyPackage<P>;
}

/// FROST(Pallas, BLAKE2b-512) Round 1 functionality and types.
pub mod round1 {
    use frost_rerandomized::frost_core::frost::keys::SigningShare;

    use super::*;
    /// Comprised of FROST(Pallas, BLAKE2b-512) hiding and binding nonces.
    ///
    /// Note that [`SigningNonces`] must be used *only once* for a signing
    /// operation; re-using nonces will result in leakage of a signer's long-lived
    /// signing key.
    pub type SigningNonces = frost::round1::SigningNonces<P>;

    /// Published by each participant in the first round of the signing protocol.
    ///
    /// This step can be batched if desired by the implementation. Each
    /// SigningCommitment can be used for exactly *one* signature.
    pub type SigningCommitments = frost::round1::SigningCommitments<P>;

    /// Performed once by each participant selected for the signing operation.
    ///
    /// Generates the signing nonces and commitments to be used in the signing
    /// operation.
    pub fn commit<RNG>(
        secret: &SigningShare<P>,
        rng: &mut RNG,
    ) -> (SigningNonces, SigningCommitments)
    where
        RNG: CryptoRng + RngCore,
    {
        frost::round1::commit::<P, RNG>(secret, rng)
    }
}

/// Generated by the coordinator of the signing operation and distributed to
/// each signing party.
pub type SigningPackage = frost::SigningPackage<P>;

/// FROST(Pallas, BLAKE2b-512) Round 2 functionality and types, for signature share generation.
pub mod round2 {
    use super::*;

    /// A FROST(Pallas, BLAKE2b-512) participant's signature share, which the Coordinator will aggregate with all other signer's
    /// shares into the joint signature.
    pub type SignatureShare = frost::round2::SignatureShare<P>;

    /// Generated by the coordinator of the signing operation and distributed to
    /// each signing party
    pub type SigningPackage = frost::SigningPackage<P>;

    /// Performed once by each participant selected for the signing operation.
    ///
    /// Receives the message to be signed and a set of signing commitments and a set
    /// of randomizing commitments to be used in that signing operation, including
    /// that for this participant.
    ///
    /// Assumes the participant has already determined which nonce corresponds with
    /// the commitment that was assigned by the coordinator in the SigningPackage.
    pub fn sign(
        signing_package: &SigningPackage,
        signer_nonces: &round1::SigningNonces,
        key_package: &keys::KeyPackage,
        randomizer_point: &<<P as Ciphersuite>::Group as Group>::Element,
    ) -> Result<SignatureShare, Error> {
        frost_rerandomized::sign(
            signing_package,
            signer_nonces,
            key_package,
            randomizer_point,
        )
    }
}

/// A Schnorr signature on FROST(Pallas, BLAKE2b-512).
pub type Signature = frost_rerandomized::frost_core::Signature<P>;

/// Verifies each FROST(Pallas, BLAKE2b-512) participant's signature share, and if all are valid,
/// aggregates the shares into a signature to publish.
///
/// Resulting signature is compatible with verification of a plain Schnorr
/// signature.
///
/// This operation is performed by a coordinator that can communicate with all
/// the signing participants before publishing the final signature. The
/// coordinator can be one of the participants or a semi-trusted third party
/// (who is trusted to not perform denial of service attacks, but does not learn
/// any secret information). Note that because the coordinator is trusted to
/// report misbehaving parties in order to avoid publishing an invalid
/// signature, if the coordinator themselves is a signer and misbehaves, they
/// can avoid that step. However, at worst, this results in a denial of
/// service attack due to publishing an invalid signature.
pub fn aggregate(
    signing_package: &round2::SigningPackage,
    signature_shares: &HashMap<Identifier, round2::SignatureShare>,
    pubkeys: &keys::PublicKeyPackage,
    randomized_params: &RandomizedParams<P>,
) -> Result<Signature, Error> {
    frost_rerandomized::aggregate(
        signing_package,
        signature_shares,
        pubkeys,
        randomized_params,
    )
}

/// A signing key for a Schnorr signature on FROST(Pallas, BLAKE2b-512).
pub type SigningKey = frost_rerandomized::frost_core::SigningKey<P>;

/// A valid verifying key for Schnorr signatures on FROST(Pallas, BLAKE2b-512).
pub type VerifyingKey = frost_rerandomized::frost_core::VerifyingKey<P>;
