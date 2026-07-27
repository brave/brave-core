//! Utility functions for computing bundle commitments

use blake2b_simd::{Hash as Blake2bHash, Params, State};

use crate::{
    bundle::{Authorization, Authorized, Bundle, CommitmentError, TxVersion},
    ValuePool,
};

const ZCASH_ORCHARD_V5_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxIdOrchardHash";
const ZCASH_ORCHARD_V6_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxIdOrchardH_v6";
const ZCASH_ORCHARD_ACTIONS_COMPACT_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxIdOrcActCHash";
const ZCASH_ORCHARD_ACTIONS_MEMOS_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxIdOrcActMHash";
const ZCASH_ORCHARD_ACTIONS_NONCOMPACT_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxIdOrcActNHash";
const ZCASH_ORCHARD_V5_SIGS_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxAuthOrchaHash";
const ZCASH_ORCHARD_V6_SIGS_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxAuthOrchaH_v6";
const ZCASH_IRONWOOD_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxIdIronwd_H_v6";
const ZCASH_IRONWOOD_ACTIONS_COMPACT_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxIdIrnActCH_v6";
const ZCASH_IRONWOOD_ACTIONS_MEMOS_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxIdIrnActMH_v6";
const ZCASH_IRONWOOD_ACTIONS_NONCOMPACT_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxIdIrnActNH_v6";
const ZCASH_IRONWOOD_SIGS_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxAuthIrnwdH_v6";

#[derive(Clone, Copy, Debug)]
struct BundleCommitmentPersonalizations {
    bundle: &'static [u8; 16],
    actions_compact: &'static [u8; 16],
    actions_memos: &'static [u8; 16],
    actions_noncompact: &'static [u8; 16],
    auth: &'static [u8; 16],
}

const ORCHARD_V5_PERSONALIZATIONS: BundleCommitmentPersonalizations =
    BundleCommitmentPersonalizations {
        bundle: ZCASH_ORCHARD_V5_HASH_PERSONALIZATION,
        actions_compact: ZCASH_ORCHARD_ACTIONS_COMPACT_HASH_PERSONALIZATION,
        actions_memos: ZCASH_ORCHARD_ACTIONS_MEMOS_HASH_PERSONALIZATION,
        actions_noncompact: ZCASH_ORCHARD_ACTIONS_NONCOMPACT_HASH_PERSONALIZATION,
        auth: ZCASH_ORCHARD_V5_SIGS_HASH_PERSONALIZATION,
    };

// Orchard v6 deliberately reuses the v5 action-level personalizations
// (compact/memos/noncompact); only the top-level `bundle` and `auth` strings gain `_v6`.
// Ironwood instead uses fresh `_v6` strings throughout. Either way the top-level digest is
// domain-separated by its `bundle`/`auth` personalization, so reusing the action-level ones
// cannot collide across formats.
const ORCHARD_V6_PERSONALIZATIONS: BundleCommitmentPersonalizations =
    BundleCommitmentPersonalizations {
        bundle: ZCASH_ORCHARD_V6_HASH_PERSONALIZATION,
        actions_compact: ZCASH_ORCHARD_ACTIONS_COMPACT_HASH_PERSONALIZATION,
        actions_memos: ZCASH_ORCHARD_ACTIONS_MEMOS_HASH_PERSONALIZATION,
        actions_noncompact: ZCASH_ORCHARD_ACTIONS_NONCOMPACT_HASH_PERSONALIZATION,
        auth: ZCASH_ORCHARD_V6_SIGS_HASH_PERSONALIZATION,
    };

const IRONWOOD_V6_PERSONALIZATIONS: BundleCommitmentPersonalizations =
    BundleCommitmentPersonalizations {
        bundle: ZCASH_IRONWOOD_HASH_PERSONALIZATION,
        actions_compact: ZCASH_IRONWOOD_ACTIONS_COMPACT_HASH_PERSONALIZATION,
        actions_memos: ZCASH_IRONWOOD_ACTIONS_MEMOS_HASH_PERSONALIZATION,
        actions_noncompact: ZCASH_IRONWOOD_ACTIONS_NONCOMPACT_HASH_PERSONALIZATION,
        auth: ZCASH_IRONWOOD_SIGS_HASH_PERSONALIZATION,
    };

/// The hash format used to compute a bundle's transaction-ID and authorizing digests,
/// selected from the bundle's pool and the version of the transaction it is encoded in.
/// Orchard bundles use the v5 or v6 format according to the transaction; Ironwood bundles
/// exist only in v6 transactions.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum BundleCommitmentFormat {
    OrchardV5,
    OrchardV6,
    IronwoodV6,
}

impl ValuePool {
    fn commitment_format(
        self,
        tx_version: TxVersion,
    ) -> Result<BundleCommitmentFormat, CommitmentError> {
        match (self, tx_version) {
            (ValuePool::Orchard, TxVersion::V5) => Ok(BundleCommitmentFormat::OrchardV5),
            (ValuePool::Orchard, TxVersion::V6) => Ok(BundleCommitmentFormat::OrchardV6),
            (ValuePool::Ironwood, TxVersion::V5) => Err(CommitmentError::InvalidTransactionVersion),
            (ValuePool::Ironwood, TxVersion::V6) => Ok(BundleCommitmentFormat::IronwoodV6),
        }
    }
}

impl BundleCommitmentFormat {
    fn personalizations(self) -> BundleCommitmentPersonalizations {
        match self {
            BundleCommitmentFormat::OrchardV5 => ORCHARD_V5_PERSONALIZATIONS,
            BundleCommitmentFormat::OrchardV6 => ORCHARD_V6_PERSONALIZATIONS,
            BundleCommitmentFormat::IronwoodV6 => IRONWOOD_V6_PERSONALIZATIONS,
        }
    }

    fn includes_anchor_in_txid_digest(self) -> bool {
        matches!(self, BundleCommitmentFormat::OrchardV5)
    }

    fn includes_anchor_in_authorizing_digest(self) -> bool {
        matches!(
            self,
            BundleCommitmentFormat::OrchardV6 | BundleCommitmentFormat::IronwoodV6
        )
    }
}

fn hasher(personal: &[u8; 16]) -> State {
    Params::new().hash_length(32).personal(personal).to_state()
}

/// Write disjoint parts of each bundle action as 3 separate hashes
/// as defined in [ZIP-244: Transaction Identifier Non-Malleability][zip244]:
/// * \[(nullifier, cmx, ephemeral_key, enc_ciphertext\[..52\])*\] personalized
///   with the format's compact-action personalization string
/// * \[enc_ciphertext\[52..564\]*\] (memo ciphertexts) personalized
///   with the format's action-memos personalization string
/// * \[(cv, rk, enc_ciphertext\[564..\], out_ciphertext)*\] personalized
///   with the format's non-compact-action personalization string
///
/// Then, hash these together along with (flags, value_balance_orchard, and — for the v5
/// transaction format only — anchor_orchard), personalized with the format's bundle
/// personalization string. In the v6 format the anchor is included by
/// `hash_bundle_auth_data` instead.
///
/// Returns [`CommitmentError::InvalidTransactionVersion`] if `tx_version` is not valid for the
/// bundle's [`BundleVersion`].
///
/// [zip244]: https://zips.z.cash/zip-0244
/// [`BundleVersion`]: crate::bundle::BundleVersion
pub(crate) fn hash_bundle_txid_data<A: Authorization, V: Copy + Into<i64>>(
    bundle: &Bundle<A, V>,
    tx_version: TxVersion,
) -> Result<Blake2bHash, CommitmentError> {
    let format = bundle
        .bundle_version()
        .value_pool()
        .commitment_format(tx_version)?;
    let personalizations = format.personalizations();
    let mut h = hasher(personalizations.bundle);
    let mut ch = hasher(personalizations.actions_compact);
    let mut mh = hasher(personalizations.actions_memos);
    let mut nh = hasher(personalizations.actions_noncompact);

    for action in bundle.actions().iter() {
        ch.update(&action.nullifier().to_bytes());
        ch.update(&action.cmx().to_bytes());
        ch.update(&action.encrypted_note().epk_bytes);
        ch.update(&action.encrypted_note().enc_ciphertext[..52]);

        mh.update(&action.encrypted_note().enc_ciphertext[52..564]);

        nh.update(&action.cv_net().to_bytes());
        nh.update(&<[u8; 32]>::from(action.rk()));
        nh.update(&action.encrypted_note().enc_ciphertext[564..]);
        nh.update(&action.encrypted_note().out_ciphertext);
    }

    h.update(ch.finalize().as_bytes());
    h.update(mh.finalize().as_bytes());
    h.update(nh.finalize().as_bytes());
    h.update(&[bundle.flag_byte()]);
    h.update(&(*bundle.value_balance()).into().to_le_bytes());
    if format.includes_anchor_in_txid_digest() {
        h.update(&bundle.anchor().to_bytes());
    }
    Ok(h.finalize())
}

/// Construct the commitment for the absent bundle as defined in
/// [ZIP-244: Transaction Identifier Non-Malleability][zip244]
///
/// [zip244]: https://zips.z.cash/zip-0244
pub fn hash_bundle_txid_empty(
    value_pool: ValuePool,
    tx_version: TxVersion,
) -> Result<Blake2bHash, CommitmentError> {
    Ok(hasher(
        value_pool
            .commitment_format(tx_version)?
            .personalizations()
            .bundle,
    )
    .finalize())
}

/// Construct the commitment to the authorizing data of an
/// authorized bundle as defined in [ZIP-244: Transaction
/// Identifier Non-Malleability][zip244]
///
/// [zip244]: https://zips.z.cash/zip-0244
pub(crate) fn hash_bundle_auth_data<V>(
    bundle: &Bundle<Authorized, V>,
    tx_version: TxVersion,
) -> Result<Blake2bHash, CommitmentError> {
    let format = bundle
        .bundle_version()
        .value_pool()
        .commitment_format(tx_version)?;
    let mut h = hasher(format.personalizations().auth);
    h.update(bundle.authorization().proof().as_ref());
    for action in bundle.actions().iter() {
        h.update(&<[u8; 64]>::from(action.authorization()));
    }
    h.update(&<[u8; 64]>::from(
        bundle.authorization().binding_signature(),
    ));
    if format.includes_anchor_in_authorizing_digest() {
        h.update(&bundle.anchor().to_bytes());
    }
    Ok(h.finalize())
}

/// Construct the commitment for an absent bundle as defined in
/// [ZIP-244: Transaction Identifier Non-Malleability][zip244]
///
/// [zip244]: https://zips.z.cash/zip-0244
pub fn hash_bundle_auth_empty(
    value_pool: ValuePool,
    tx_version: TxVersion,
) -> Result<Blake2bHash, CommitmentError> {
    Ok(hasher(
        value_pool
            .commitment_format(tx_version)?
            .personalizations()
            .auth,
    )
    .finalize())
}
