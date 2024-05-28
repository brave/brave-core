use blake2b_simd::Hash as Blake2bHash;

use super::{
    components::{amount::NonNegativeAmount, transparent},
    sighash_v4::v4_signature_hash,
    sighash_v5::v5_signature_hash,
    Authorization, TransactionData, TxDigests, TxVersion,
};
use crate::{
    legacy::Script,
    sapling::{self, bundle::GrothProofBytes},
};

#[cfg(zcash_unstable = "zfuture")]
use {super::components::Amount, crate::extensions::transparent::Precondition};

pub const SIGHASH_ALL: u8 = 0x01;
pub const SIGHASH_NONE: u8 = 0x02;
pub const SIGHASH_SINGLE: u8 = 0x03;
pub const SIGHASH_MASK: u8 = 0x1f;
pub const SIGHASH_ANYONECANPAY: u8 = 0x80;

pub enum SignableInput<'a> {
    Shielded,
    Transparent {
        hash_type: u8,
        index: usize,
        script_code: &'a Script,
        script_pubkey: &'a Script,
        value: NonNegativeAmount,
    },
    #[cfg(zcash_unstable = "zfuture")]
    Tze {
        index: usize,
        precondition: &'a Precondition,
        value: Amount,
    },
}

impl<'a> SignableInput<'a> {
    pub fn hash_type(&self) -> u8 {
        match self {
            SignableInput::Shielded => SIGHASH_ALL,
            SignableInput::Transparent { hash_type, .. } => *hash_type,
            #[cfg(zcash_unstable = "zfuture")]
            SignableInput::Tze { .. } => SIGHASH_ALL,
        }
    }
}

pub struct SignatureHash(Blake2bHash);

impl AsRef<[u8; 32]> for SignatureHash {
    fn as_ref(&self) -> &[u8; 32] {
        self.0.as_ref().try_into().unwrap()
    }
}

/// Additional context that is needed to compute signature hashes
/// for transactions that include transparent inputs or outputs.
pub trait TransparentAuthorizingContext: transparent::Authorization {
    /// Returns the list of all transparent input amounts, provided
    /// so that wallets can commit to the transparent input breakdown
    /// without requiring the full data of the previous transactions
    /// providing these inputs.
    fn input_amounts(&self) -> Vec<NonNegativeAmount>;
    /// Returns the list of all transparent input scriptPubKeys, provided
    /// so that wallets can commit to the transparent input breakdown
    /// without requiring the full data of the previous transactions
    /// providing these inputs.
    fn input_scriptpubkeys(&self) -> Vec<Script>;
}

/// Computes the signature hash for an input to a transaction, given
/// the full data of the transaction, the input being signed, and the
/// set of precomputed hashes produced in the construction of the
/// transaction ID.
pub fn signature_hash<
    'a,
    TA: TransparentAuthorizingContext,
    SA: sapling::bundle::Authorization<SpendProof = GrothProofBytes, OutputProof = GrothProofBytes>,
    A: Authorization<SaplingAuth = SA, TransparentAuth = TA>,
>(
    tx: &TransactionData<A>,
    signable_input: &SignableInput<'a>,
    txid_parts: &TxDigests<Blake2bHash>,
) -> SignatureHash {
    SignatureHash(match tx.version {
        TxVersion::Sprout(_) | TxVersion::Overwinter | TxVersion::Sapling => {
            v4_signature_hash(tx, signable_input)
        }

        TxVersion::Zip225 => v5_signature_hash(tx, signable_input, txid_parts),

        #[cfg(zcash_unstable = "zfuture")]
        TxVersion::ZFuture => v5_signature_hash(tx, signable_input, txid_parts),
    })
}
