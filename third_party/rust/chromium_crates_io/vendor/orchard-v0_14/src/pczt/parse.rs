use core::fmt;

use alloc::collections::BTreeMap;
use alloc::string::String;
use alloc::vec::Vec;

use ff::PrimeField;
use incrementalmerkletree::Hashable;
use pasta_curves::pallas;
use zcash_note_encryption::OutgoingCipherKey;
use zip32::ChildIndex;

use super::{Action, Bundle, Output, Spend, Zip32Derivation};
use crate::{
    bundle::Flags,
    keys::{FullViewingKey, SpendingKey},
    note::{ExtractedNoteCommitment, Nullifier, RandomSeed, Rho, TransmittedNoteCiphertext},
    primitives::redpallas::{self, SpendAuth},
    tree::{MerkleHashOrchard, MerklePath},
    value::{NoteValue, Sign, ValueCommitTrapdoor, ValueCommitment, ValueSum},
    Address, Anchor, Proof, NOTE_COMMITMENT_TREE_DEPTH,
};

impl Bundle {
    /// Parses a PCZT bundle from its component parts.
    /// `value_sum` is represented as `(magnitude, is_negative)`.
    pub fn parse(
        actions: Vec<Action>,
        flags: u8,
        value_sum: (u64, bool),
        anchor: [u8; 32],
        zkproof: Option<Vec<u8>>,
        bsk: Option<[u8; 32]>,
    ) -> Result<Self, ParseError> {
        let flags = Flags::from_byte(flags).ok_or(ParseError::UnexpectedFlagBitsSet)?;

        let value_sum = {
            let (magnitude, is_negative) = value_sum;
            ValueSum::from_magnitude_sign(
                magnitude,
                if is_negative {
                    Sign::Negative
                } else {
                    Sign::Positive
                },
            )
        };

        let anchor = Anchor::from_bytes(anchor)
            .into_option()
            .ok_or(ParseError::InvalidAnchor)?;

        let zkproof = zkproof.map(Proof::new);

        let bsk = bsk
            .map(redpallas::SigningKey::try_from)
            .transpose()
            .map_err(|_| ParseError::InvalidBindingSignatureSigningKey)?;

        Ok(Self {
            actions,
            flags,
            value_sum,
            anchor,
            zkproof,
            bsk,
        })
    }
}

impl Action {
    /// Parses a PCZT action from its component parts.
    pub fn parse(
        cv_net: [u8; 32],
        spend: Spend,
        output: Output,
        rcv: Option<[u8; 32]>,
    ) -> Result<Self, ParseError> {
        let cv_net = ValueCommitment::from_bytes(&cv_net)
            .into_option()
            .ok_or(ParseError::InvalidValueCommitment)?;

        let rcv = rcv
            .map(ValueCommitTrapdoor::from_bytes)
            .map(|rcv| {
                rcv.into_option()
                    .ok_or(ParseError::InvalidValueCommitTrapdoor)
            })
            .transpose()?;

        Ok(Self {
            cv_net,
            spend,
            output,
            rcv,
        })
    }
}

impl Spend {
    /// Parses a PCZT spend from its component parts.
    #[allow(clippy::too_many_arguments)]
    pub fn parse(
        nullifier: [u8; 32],
        rk: [u8; 32],
        spend_auth_sig: Option<[u8; 64]>,
        recipient: Option<[u8; 43]>,
        value: Option<u64>,
        rho: Option<[u8; 32]>,
        rseed: Option<[u8; 32]>,
        fvk: Option<[u8; 96]>,
        witness: Option<(u32, [[u8; 32]; NOTE_COMMITMENT_TREE_DEPTH])>,
        alpha: Option<[u8; 32]>,
        zip32_derivation: Option<Zip32Derivation>,
        dummy_sk: Option<[u8; 32]>,
        proprietary: BTreeMap<String, Vec<u8>>,
    ) -> Result<Self, ParseError> {
        let nullifier = Nullifier::from_bytes(&nullifier)
            .into_option()
            .ok_or(ParseError::InvalidNullifier)?;

        let rk = redpallas::VerificationKey::try_from(rk)
            .map_err(|_| ParseError::InvalidRandomizedKey)?;

        let spend_auth_sig = spend_auth_sig.map(redpallas::Signature::<SpendAuth>::from);

        let recipient = recipient
            .as_ref()
            .map(|r| {
                Address::from_raw_address_bytes(r)
                    .into_option()
                    .ok_or(ParseError::InvalidRecipient)
            })
            .transpose()?;

        let value = value.map(NoteValue::from_raw);

        let rho = rho
            .map(|rho| {
                Rho::from_bytes(&rho)
                    .into_option()
                    .ok_or(ParseError::InvalidRho)
            })
            .transpose()?;

        let rseed = rseed
            .map(|rseed| {
                let rho = rho.as_ref().ok_or(ParseError::MissingRho)?;
                RandomSeed::from_bytes(rseed, rho)
                    .into_option()
                    .ok_or(ParseError::InvalidRandomSeed)
            })
            .transpose()?;

        let fvk = fvk
            .map(|fvk| FullViewingKey::from_bytes(&fvk).ok_or(ParseError::InvalidFullViewingKey))
            .transpose()?;

        let witness = witness
            .map(|(position, auth_path)| {
                Ok(MerklePath::from_parts(position, {
                    // Replace this with `array::try_map` if it ever stabilises.
                    let mut buf = [MerkleHashOrchard::empty_leaf(); NOTE_COMMITMENT_TREE_DEPTH];
                    for (from, to) in auth_path.into_iter().zip(&mut buf) {
                        *to = MerkleHashOrchard::from_bytes(&from)
                            .into_option()
                            .ok_or(ParseError::InvalidWitness)?;
                    }
                    buf
                }))
            })
            .transpose()?;

        let alpha = alpha
            .map(|alpha| {
                pallas::Scalar::from_repr(alpha)
                    .into_option()
                    .ok_or(ParseError::InvalidSpendAuthRandomizer)
            })
            .transpose()?;

        let dummy_sk = dummy_sk
            .map(|dummy_sk| {
                SpendingKey::from_bytes(dummy_sk)
                    .into_option()
                    .ok_or(ParseError::InvalidDummySpendingKey)
            })
            .transpose()?;

        Ok(Self {
            nullifier,
            rk,
            spend_auth_sig,
            recipient,
            value,
            rho,
            rseed,
            fvk,
            witness,
            alpha,
            zip32_derivation,
            dummy_sk,
            proprietary,
        })
    }
}

impl Output {
    /// Parses a PCZT output from its component parts, and the corresponding `Spend`'s
    /// nullifier.
    #[allow(clippy::too_many_arguments)]
    pub fn parse(
        spend_nullifier: Nullifier,
        cmx: [u8; 32],
        ephemeral_key: [u8; 32],
        enc_ciphertext: Vec<u8>,
        out_ciphertext: Vec<u8>,
        recipient: Option<[u8; 43]>,
        value: Option<u64>,
        rseed: Option<[u8; 32]>,
        ock: Option<[u8; 32]>,
        zip32_derivation: Option<Zip32Derivation>,
        user_address: Option<String>,
        proprietary: BTreeMap<String, Vec<u8>>,
    ) -> Result<Self, ParseError> {
        let cmx = ExtractedNoteCommitment::from_bytes(&cmx)
            .into_option()
            .ok_or(ParseError::InvalidExtractedNoteCommitment)?;

        let encrypted_note = TransmittedNoteCiphertext {
            epk_bytes: ephemeral_key,
            enc_ciphertext: enc_ciphertext
                .as_slice()
                .try_into()
                .map_err(|_| ParseError::InvalidEncCiphertext)?,
            out_ciphertext: out_ciphertext
                .as_slice()
                .try_into()
                .map_err(|_| ParseError::InvalidOutCiphertext)?,
        };

        let recipient = recipient
            .as_ref()
            .map(|r| {
                Address::from_raw_address_bytes(r)
                    .into_option()
                    .ok_or(ParseError::InvalidRecipient)
            })
            .transpose()?;

        let value = value.map(NoteValue::from_raw);

        let rseed = rseed
            .map(|rseed| {
                let rho = Rho::from_nf_old(spend_nullifier);
                RandomSeed::from_bytes(rseed, &rho)
                    .into_option()
                    .ok_or(ParseError::InvalidRandomSeed)
            })
            .transpose()?;

        let ock = ock.map(OutgoingCipherKey);

        Ok(Self {
            cmx,
            encrypted_note,
            recipient,
            value,
            rseed,
            ock,
            zip32_derivation,
            user_address,
            proprietary,
        })
    }
}

impl Zip32Derivation {
    /// Parses a ZIP 32 derivation path from its component parts.
    ///
    /// Returns an error if any of the derivation path indices are non-hardened (which
    /// Orchard does not support).
    pub fn parse(
        seed_fingerprint: [u8; 32],
        derivation_path: Vec<u32>,
    ) -> Result<Self, ParseError> {
        Ok(Self {
            seed_fingerprint,
            derivation_path: derivation_path
                .into_iter()
                .map(|i| ChildIndex::from_index(i).ok_or(ParseError::InvalidZip32Derivation))
                .collect::<Result<_, _>>()?,
        })
    }
}

/// Errors that can occur while parsing a PCZT bundle.
#[derive(Debug)]
#[non_exhaustive]
pub enum ParseError {
    /// An invalid anchor was provided.
    InvalidAnchor,
    /// An invalid `bsk` was provided.
    InvalidBindingSignatureSigningKey,
    /// An invalid `dummy_sk` was provided.
    InvalidDummySpendingKey,
    /// An invalid `enc_ciphertext` was provided.
    InvalidEncCiphertext,
    /// An invalid `cmx` was provided.
    InvalidExtractedNoteCommitment,
    /// An invalid `fvk` was provided.
    InvalidFullViewingKey,
    /// An invalid `nullifier` was provided.
    InvalidNullifier,
    /// An invalid `out_ciphertext` was provided.
    InvalidOutCiphertext,
    /// An invalid `rk` was provided.
    InvalidRandomizedKey,
    /// An invalid `rseed` was provided.
    InvalidRandomSeed,
    /// An invalid `recipient` was provided.
    InvalidRecipient,
    /// An invalid `rho` was provided.
    InvalidRho,
    /// An invalid `alpha` was provided.
    InvalidSpendAuthRandomizer,
    /// An invalid `cv_net` was provided.
    InvalidValueCommitment,
    /// An invalid `rcv` was provided.
    InvalidValueCommitTrapdoor,
    /// An invalid `witness` was provided.
    InvalidWitness,
    /// An invalid `zip32_derivation` was provided.
    InvalidZip32Derivation,
    /// `rho` must be provided whenever `rseed` is provided.
    MissingRho,
    /// The provided `flags` field had unexpected bits set.
    UnexpectedFlagBitsSet,
}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ParseError::InvalidAnchor => write!(f, "invalid anchor"),
            ParseError::InvalidBindingSignatureSigningKey => write!(f, "invalid `bsk`"),
            ParseError::InvalidDummySpendingKey => write!(f, "invalid `dummy_sk`"),
            ParseError::InvalidEncCiphertext => write!(f, "invalid `enc_ciphertext`"),
            ParseError::InvalidExtractedNoteCommitment => write!(f, "invalid `cmx`"),
            ParseError::InvalidFullViewingKey => write!(f, "invalid `fvk`"),
            ParseError::InvalidNullifier => write!(f, "invalid `nullifier`"),
            ParseError::InvalidOutCiphertext => write!(f, "invalid `out_ciphertext`"),
            ParseError::InvalidRandomizedKey => write!(f, "invalid `rk`"),
            ParseError::InvalidRandomSeed => write!(f, "invalid `rseed`"),
            ParseError::InvalidRecipient => write!(f, "invalid `recipient`"),
            ParseError::InvalidRho => write!(f, "invalid `rho`"),
            ParseError::InvalidSpendAuthRandomizer => write!(f, "invalid `alpha`"),
            ParseError::InvalidValueCommitment => write!(f, "invalid `cv_net`"),
            ParseError::InvalidValueCommitTrapdoor => write!(f, "invalid `rcv`"),
            ParseError::InvalidWitness => write!(f, "invalid `witness`"),
            ParseError::InvalidZip32Derivation => write!(f, "invalid `zip32_derivation`"),
            ParseError::MissingRho => {
                write!(f, "`rho` must be provided whenever `rseed` is provided")
            }
            ParseError::UnexpectedFlagBitsSet => write!(f, "`flags` field had unexpected bits set"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ParseError {}
