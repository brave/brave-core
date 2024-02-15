//! The FROST communication messages specified in [RFC-001]
//!
//! [RFC-001]: https://github.com/ZcashFoundation/redjubjub/blob/main/rfcs/0001-messages.md

use crate::{frost, signature, verification_key, SpendAuth};
use group::GroupEncoding;
use serde::{Deserialize, Serialize};

use alloc::vec::Vec;
use std::{collections::BTreeMap, convert::TryInto};

#[cfg(test)]
use proptest_derive::Arbitrary;

#[cfg(test)]
mod arbitrary;
mod constants;
mod serialize;
#[cfg(test)]
mod tests;
mod validate;

/// Define our own `Secret` type instead of using [`frost::Secret`].
///
/// The serialization design specifies that `Secret` is a [`jubjub::Scalar`] that uses:
/// "a 32-byte little-endian canonical representation".
#[derive(Serialize, Deserialize, Debug, PartialEq, Clone, Copy)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct Secret([u8; 32]);

/// Define our own `Commitment` type instead of using [`frost::Commitment`].
///
/// The serialization design specifies that `Commitment` is an [`jubjub::AffinePoint`] that uses:
/// "a 32-byte little-endian canonical representation".
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Copy)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct Commitment([u8; 32]);

impl<S: SpendAuth> From<frost::Commitment<S>> for Commitment {
    fn from(value: frost::Commitment<S>) -> Commitment {
        // TODO(str4d): We need to either enforce somewhere that these messages are only
        // used with curves that have 32-byte encodings, or make the curve a parameter of
        // the encoding. This will be easier once const_evaluatable_checked stabilises.
        Commitment(value.0.to_bytes().as_ref().try_into().unwrap())
    }
}

/// Define our own `GroupCommitment` type instead of using [`frost::GroupCommitment`].
///
/// The serialization design specifies that `GroupCommitment` is an [`jubjub::AffinePoint`] that uses:
/// "a 32-byte little-endian canonical representation".
#[derive(Serialize, Deserialize, Debug, PartialEq, Clone, Copy)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct GroupCommitment([u8; 32]);

/// Define our own `SignatureResponse` type instead of using [`frost::SignatureResponse`].
///
/// The serialization design specifies that `SignatureResponse` is a [`jubjub::Scalar`] that uses:
/// "a 32-byte little-endian canonical representation".
#[derive(Serialize, Deserialize, Debug, PartialEq, Clone, Copy)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct SignatureResponse([u8; 32]);

impl<S: SpendAuth> From<signature::Signature<S>> for SignatureResponse {
    fn from(value: signature::Signature<S>) -> SignatureResponse {
        SignatureResponse(value.s_bytes)
    }
}

impl<S: SpendAuth> From<signature::Signature<S>> for GroupCommitment {
    fn from(value: signature::Signature<S>) -> GroupCommitment {
        GroupCommitment(value.r_bytes)
    }
}

/// Define our own `VerificationKey` type instead of using [`verification_key::VerificationKey<SpendAuth>`].
///
/// The serialization design specifies that `VerificationKey` is a [`verification_key::VerificationKeyBytes`] that uses:
/// "a 32-byte little-endian canonical representation".
#[derive(Serialize, Deserialize, PartialEq, Debug, Copy, Clone)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct VerificationKey([u8; 32]);

impl<S: SpendAuth> From<verification_key::VerificationKey<S>> for VerificationKey {
    fn from(value: verification_key::VerificationKey<S>) -> VerificationKey {
        VerificationKey(<[u8; 32]>::from(value))
    }
}

/// The data required to serialize a frost message.
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct Message {
    header: Header,
    payload: Payload,
}

/// The data required to serialize the common header fields for every message.
///
/// Note: the `msg_type` is derived from the `payload` enum variant.
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone, Copy)]
pub struct Header {
    version: MsgVersion,
    sender: ParticipantId,
    receiver: ParticipantId,
}

/// The data required to serialize the payload for a message.
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
#[cfg_attr(test, derive(Arbitrary))]
pub enum Payload {
    SharePackage(SharePackage),
    SigningCommitments(SigningCommitments),
    SigningPackage(SigningPackage),
    SignatureShare(SignatureShare),
    AggregateSignature(AggregateSignature),
}

/// The numeric values used to identify each [`Payload`] variant during serialization.
// TODO: spec says `#[repr(u8)]` but it is incompatible with `bincode`
// manual serialization and deserialization is needed.
#[repr(u32)]
#[non_exhaustive]
#[derive(Serialize, Deserialize, Debug, PartialEq)]
enum MsgType {
    SharePackage,
    SigningCommitments,
    SigningPackage,
    SignatureShare,
    AggregateSignature,
}

/// The numeric values used to identify the protocol version during serialization.
#[derive(PartialEq, Serialize, Deserialize, Debug, Clone, Copy)]
pub struct MsgVersion(u8);

/// The numeric values used to identify each participant during serialization.
///
/// In the `frost` module, participant ID `0` should be invalid.
/// But in serialization, we want participants to be indexed from `0..n`,
/// where `n` is the number of participants.
/// This helps us look up their shares and commitments in serialized arrays.
/// So in serialization, we assign the dealer and aggregator the highest IDs,
/// and mark those IDs as invalid for signers.
///
/// "When performing Shamir secret sharing, a polynomial `f(x)` is used to generate
/// each party’s share of the secret. The actual secret is `f(0)` and the party with
/// ID `i` will be given a share with value `f(i)`.
/// Since a DKG may be implemented in the future, we recommend that the ID `0` be declared invalid."
/// https://raw.githubusercontent.com/ZcashFoundation/redjubjub/main/zcash-frost-audit-report-20210323.pdf#d
#[derive(PartialEq, Eq, Hash, PartialOrd, Debug, Copy, Clone, Ord)]
pub enum ParticipantId {
    /// A serialized participant ID for a signer.
    ///
    /// Must be less than or equal to [`constants::MAX_SIGNER_PARTICIPANT_ID`].
    Signer(u64),
    /// The fixed participant ID for the dealer as defined in [`constants::DEALER_PARTICIPANT_ID`].
    Dealer,
    /// The fixed participant ID for the aggregator as defined in [`constants::AGGREGATOR_PARTICIPANT_ID`].
    Aggregator,
}

impl From<ParticipantId> for u64 {
    fn from(value: ParticipantId) -> u64 {
        match value {
            // An id of `0` is invalid in frost.
            ParticipantId::Signer(id) => id + 1,
            ParticipantId::Dealer => constants::DEALER_PARTICIPANT_ID,
            ParticipantId::Aggregator => constants::AGGREGATOR_PARTICIPANT_ID,
        }
    }
}

/// The data required to serialize [`frost::SharePackage`].
///
/// The dealer sends this message to each signer for this round.
/// With this, the signer should be able to build a [`SharePackage`] and use
/// the [`frost::sign()`] function.
///
/// Note: [`frost::SharePackage::public`] can be calculated from [`SharePackage::secret_share`].
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct SharePackage {
    /// The public signing key that represents the entire group:
    /// [`frost::SharePackage::group_public`].
    group_public: VerificationKey,
    /// This participant's secret key share: [`frost::SharePackage::share`].
    secret_share: Secret,
    /// The commitments to the coefficients for our secret polynomial _f_,
    /// used to generate participants' key shares. Participants use these to perform
    /// verifiable secret sharing.
    /// Share packages that contain duplicate or missing [`ParticipantId`]s are invalid.
    /// [`ParticipantId`]s must be serialized in ascending numeric order.
    share_commitment: BTreeMap<ParticipantId, Commitment>,
}

/// The data required to serialize [`frost::SigningCommitments`].
///
/// Each signer must send this message to the aggregator.
/// A signing commitment from the first round of the signing protocol.
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct SigningCommitments {
    /// The hiding point: [`frost::SigningCommitments::hiding`]
    hiding: Commitment,
    /// The binding point: [`frost::SigningCommitments::binding`]
    binding: Commitment,
}

/// The data required to serialize [`frost::SigningPackage`].
///
/// The aggregator decides what message is going to be signed and
/// sends it to each signer with all the commitments collected.
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct SigningPackage {
    /// The collected commitments for each signer as a hashmap of
    /// unique participant identifiers: [`frost::SigningPackage::signing_commitments`]
    ///
    /// Signing packages that contain duplicate or missing [`ParticipantId`]s are invalid.
    signing_commitments: BTreeMap<ParticipantId, SigningCommitments>,
    /// The message to be signed: [`frost::SigningPackage::message`].
    ///
    /// Each signer should perform protocol-specific verification on the message.
    message: Vec<u8>,
}

impl<S: SpendAuth> From<SigningPackage> for frost::SigningPackage<S> {
    fn from(value: SigningPackage) -> frost::SigningPackage<S> {
        let mut signing_commitments = Vec::new();
        for (participant_id, commitment) in &value.signing_commitments {
            // TODO(str4d): This will be so much nicer once const_evaluatable_checked
            // stabilises, and `GroupEncoding::from_bytes` can take the array directly.
            let mut hiding_repr = <S::Point as GroupEncoding>::Repr::default();
            let mut binding_repr = <S::Point as GroupEncoding>::Repr::default();
            hiding_repr.as_mut().copy_from_slice(&commitment.hiding.0);
            binding_repr.as_mut().copy_from_slice(&commitment.binding.0);

            let s = frost::SigningCommitments {
                index: u64::from(*participant_id),
                // TODO: The `from_bytes()` response is a `CtOption` so we have to `unwrap()`
                hiding: S::Point::from_bytes(&hiding_repr).unwrap(),
                binding: S::Point::from_bytes(&binding_repr).unwrap(),
            };
            signing_commitments.push(s);
        }

        frost::SigningPackage {
            signing_commitments,
            message: value.message,
        }
    }
}

/// The data required to serialize [`frost::SignatureShare`].
///
/// Each signer sends their signatures to the aggregator who is going to collect them
/// and generate a final spend signature.
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct SignatureShare {
    /// This participant's signature over the message: [`frost::SignatureShare::signature`]
    signature: SignatureResponse,
}

/// The data required to serialize a successful output from [`frost::aggregate()`].
///
/// The final signature is broadcasted by the aggregator to all signers.
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
#[cfg_attr(test, derive(Arbitrary))]
pub struct AggregateSignature {
    /// The aggregated group commitment: [`signature::Signature::r_bytes`] returned by [`frost::aggregate()`]
    group_commitment: GroupCommitment,
    /// A plain Schnorr signature created by summing all the signature shares:
    /// [`signature::Signature::s_bytes`] returned by [`frost::aggregate()`]
    schnorr_signature: SignatureResponse,
}
