//! PCZT support for Orchard.

use alloc::collections::BTreeMap;
use alloc::string::String;
use alloc::vec::Vec;
use core::fmt;

use getset::Getters;
use pasta_curves::pallas;
use zcash_note_encryption::OutgoingCipherKey;
use zip32::ChildIndex;

use crate::{
    bundle::{BundleVersion, Flags},
    keys::{FullViewingKey, SpendingKey},
    note::{ExtractedNoteCommitment, Nullifier, RandomSeed, Rho, TransmittedNoteCiphertext},
    primitives::redpallas::{self, Binding, SpendAuth},
    tree::MerklePath,
    value::{NoteValue, ValueCommitTrapdoor, ValueCommitment, ValueSum},
    Address, Anchor, NoteVersion, Proof,
};

mod parse;
pub use parse::ParseError;

mod verify;
pub use verify::VerifyError;

mod io_finalizer;
pub use io_finalizer::IoFinalizerError;

mod updater;
pub use updater::{ActionUpdater, Updater, UpdaterError};

#[cfg(feature = "circuit")]
mod prover;
#[cfg(feature = "circuit")]
pub use prover::ProverError;

mod signer;
pub use signer::SignerError;

mod tx_extractor;
pub use tx_extractor::{TxExtractorError, Unbound};

/// PCZT fields that are specific to producing the transaction's Orchard bundle (if any).
///
/// This struct is for representing Orchard in a partially-created transaction. If you
/// have a fully-created transaction, use [the regular `Bundle` struct].
///
/// [the regular `Bundle` struct]: crate::Bundle
#[derive(Debug, Getters)]
#[getset(get = "pub")]
pub struct Bundle {
    /// The Orchard actions in this bundle.
    ///
    /// Entries are added by the Constructor, and modified by an Updater, IO Finalizer,
    /// Signer, Combiner, or Spend Finalizer.
    pub(crate) actions: Vec<Action>,

    /// The flags for the Orchard bundle.
    ///
    /// This is set by the Creator. The Constructor MUST only add spends and outputs that
    /// are consistent with these flags (i.e. are dummies as appropriate).
    pub(crate) flags: Flags,

    /// The value pool and protocol version this bundle is encoded under.
    ///
    /// This is set by the Creator, and determines how the bundle's flags are interpreted and
    /// which [`crate::Bundle`] the Transaction Extractor produces. The flags are always
    /// consistent with this version (by parsing or construction).
    pub(crate) bundle_version: BundleVersion,

    /// The sum of the values of all `actions`.
    ///
    /// This is initialized by the Creator, and updated by the Constructor as spends or
    /// outputs are added to the PCZT. It enables per-spend and per-output values to be
    /// redacted from the PCZT after they are no longer necessary.
    pub(crate) value_sum: ValueSum,

    /// The Orchard anchor for this transaction.
    ///
    /// Set by the Creator.
    pub(crate) anchor: Anchor,

    /// The Orchard bundle proof.
    ///
    /// This is `None` until it is set by the Prover.
    pub(crate) zkproof: Option<Proof>,

    /// The Orchard binding signature signing key.
    ///
    /// - This is `None` until it is set by the IO Finalizer.
    /// - The Transaction Extractor uses this to produce the binding signature.
    pub(crate) bsk: Option<redpallas::SigningKey<Binding>>,
}

impl Bundle {
    /// Returns a mutable reference to the actions in this bundle.
    ///
    /// This is used by Signers to apply signatures with [`Action::sign`].
    ///
    /// Note: updating the `Action`s via the returned slice will not update other
    /// fields of the bundle dependent on them, such as `value_sum` and `bsk`.
    pub fn actions_mut(&mut self) -> &mut [Action] {
        &mut self.actions
    }

    /// Returns the byte encoding of this bundle's flags under its own [`BundleVersion`].
    ///
    /// This is infallible: a PCZT bundle is only ever constructed (by parsing or by the builder)
    /// with flags that are representable under its version.
    pub fn flag_byte(&self) -> u8 {
        self.flags
            .to_byte(self.bundle_version)
            .expect("flags are validated against the bundle version at construction")
    }
}

/// PCZT fields that are specific to producing an Orchard action within a transaction.
///
/// This struct is for representing Orchard actions in a partially-created transaction.
/// If you have a fully-created transaction, use [the regular `Action` struct].
///
/// [the regular `Action` struct]: crate::Action
#[derive(Debug, Getters)]
#[getset(get = "pub")]
pub struct Action {
    /// A commitment to the net value created or consumed by this action.
    pub(crate) cv_net: ValueCommitment,

    /// The spend half of this action.
    pub(crate) spend: Spend,

    /// The output half of this action.
    pub(crate) output: Output,

    /// The value commitment randomness.
    ///
    /// - This is set by the Constructor.
    /// - The IO Finalizer compresses it into the bsk.
    /// - This is required by the Prover.
    /// - This may be used by Signers to verify that the value correctly matches `cv`.
    ///
    /// This opens `cv` for all participants. For Signers who don't need this information,
    /// or after proofs / signatures have been applied, this can be redacted.
    pub(crate) rcv: Option<ValueCommitTrapdoor>,
}

/// Information about an Orchard spend within a transaction.
#[derive(Debug, Getters)]
#[getset(get = "pub")]
pub struct Spend {
    /// The nullifier of the note being spent.
    pub(crate) nullifier: Nullifier,

    /// The randomized verification key for the note being spent.
    pub(crate) rk: redpallas::VerificationKey<SpendAuth>,

    /// The spend authorization signature.
    ///
    /// This is set by the Signer.
    pub(crate) spend_auth_sig: Option<redpallas::Signature<SpendAuth>>,

    /// The address that received the note being spent.
    ///
    /// - This is set by the Constructor (or Updater?).
    /// - This is required by the Prover.
    pub(crate) recipient: Option<Address>,

    /// The value of the input being spent.
    ///
    /// - This is required by the Prover.
    /// - This may be used by Signers to verify that the value matches `cv`, and to
    ///   confirm the values and change involved in the transaction.
    ///
    /// This exposes the input value to all participants. For Signers who don't need this
    /// information, or after signatures have been applied, this can be redacted.
    pub(crate) value: Option<NoteValue>,

    /// The rho value for the note being spent.
    ///
    /// - This is set by the Constructor.
    /// - This is required by the Prover.
    //
    // TODO: This could be merged with `rseed` into a tuple. `recipient` and `value` are
    // separate because they might need to be independently redacted. (For which role?)
    pub(crate) rho: Option<Rho>,

    /// The seed randomness for the note being spent.
    ///
    /// - This is set by the Constructor.
    /// - This is required by the Prover.
    pub(crate) rseed: Option<RandomSeed>,

    /// The full viewing key that received the note being spent.
    ///
    /// - This is set by the Updater.
    /// - This is required by the Prover.
    pub(crate) fvk: Option<FullViewingKey>,

    /// The plaintext version of the note being spent.
    ///
    /// This is set by the Constructor, and is required by Verifiers and
    /// Provers to reconstruct the note commitment.
    pub(crate) note_version: NoteVersion,

    /// A witness from the note to the bundle's anchor.
    ///
    /// - This is set by the Updater.
    /// - This is required by the Prover.
    pub(crate) witness: Option<MerklePath>,

    /// The spend authorization randomizer.
    ///
    /// - This is chosen by the Constructor.
    /// - This is required by the Signer for creating `spend_auth_sig`, and may be used to
    ///   validate `rk`.
    /// - After`zkproof` / `spend_auth_sig` has been set, this can be redacted.
    pub(crate) alpha: Option<pallas::Scalar>,

    /// The ZIP 32 derivation path at which the spending key can be found for the note
    /// being spent.
    pub(crate) zip32_derivation: Option<Zip32Derivation>,

    /// The spending key for this spent note, if it is a dummy note.
    ///
    /// - This is chosen by the Constructor.
    /// - This is required by the IO Finalizer, and is cleared by it once used.
    /// - Signers MUST reject PCZTs that contain `dummy_sk` values.
    pub(crate) dummy_sk: Option<SpendingKey>,

    /// Proprietary fields related to the note being spent.
    pub(crate) proprietary: BTreeMap<String, Vec<u8>>,
}

/// Information about an Orchard output within a transaction.
#[derive(Getters)]
#[getset(get = "pub")]
pub struct Output {
    /// A commitment to the new note being created.
    pub(crate) cmx: ExtractedNoteCommitment,

    /// The plaintext version of the new note being created.
    ///
    /// This is set by the Constructor, and is required by Verifiers and
    /// Provers to reconstruct the note commitment.
    pub(crate) note_version: NoteVersion,

    /// The transmitted note ciphertext.
    ///
    /// This contains the following PCZT fields:
    /// - `ephemeral_key`
    /// - `enc_ciphertext`
    /// - `out_ciphertext`
    pub(crate) encrypted_note: TransmittedNoteCiphertext,

    /// The address that will receive the output.
    ///
    /// - This is set by the Constructor.
    /// - This is required by the Prover.
    /// - The Signer can use `recipient` and `rseed` (if present) to verify that
    ///   `enc_ciphertext` is correctly encrypted (and contains a note plaintext matching
    ///   the public commitments), and to confirm the value of the memo. This does not apply
    ///   to the restricted builder's zero-valued output paired with a real external-scope spend,
    ///   whose `enc_ciphertext` is deliberately randomized; its note commitment remains
    ///   verifiable.
    pub(crate) recipient: Option<Address>,

    /// The value of the output.
    ///
    /// This may be used by Signers to verify that the value matches `cv`, and to confirm
    /// the values and change involved in the transaction.
    ///
    /// This exposes the value to all participants. For Signers who don't need this
    /// information, we can drop the values and compress the rcvs into the bsk global.
    pub(crate) value: Option<NoteValue>,

    /// The seed randomness for the output.
    ///
    /// - This is set by the Constructor.
    /// - This is required by the Prover.
    /// - The Signer can use `recipient` and `rseed` (if present) to verify that
    ///   `enc_ciphertext` is correctly encrypted (and contains a note plaintext matching
    ///   the public commitments), and to confirm the value of the memo. This does not apply
    ///   to the restricted builder's zero-valued output paired with a real external-scope spend,
    ///   whose `enc_ciphertext` is deliberately randomized; its note commitment remains
    ///   verifiable.
    pub(crate) rseed: Option<RandomSeed>,

    /// The `ock` value used to encrypt `out_ciphertext`.
    ///
    /// This enables Signers to verify that `out_ciphertext` is correctly encrypted.
    ///
    /// This may be `None` if the Constructor added the output using an OVK policy of
    /// "None", to make the output unrecoverable from the chain by the sender.
    pub(crate) ock: Option<OutgoingCipherKey>,

    /// The ZIP 32 derivation path at which the spending key can be found for the output.
    pub(crate) zip32_derivation: Option<Zip32Derivation>,

    /// The user-facing address to which this output is being sent, if any.
    ///
    /// - This is set by an Updater.
    /// - Signers must parse this address (if present) and confirm that it contains
    ///   `recipient` (either directly, or e.g. as a receiver within a Unified Address).
    pub(crate) user_address: Option<String>,

    /// Proprietary fields related to the note being created.
    pub(crate) proprietary: BTreeMap<String, Vec<u8>>,
}

impl fmt::Debug for Output {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Output")
            .field("cmx", &self.cmx)
            .field("note_version", &self.note_version)
            .field("encrypted_note", &self.encrypted_note)
            .field("recipient", &self.recipient)
            .field("value", &self.value)
            .field("rseed", &self.rseed)
            .field("zip32_derivation", &self.zip32_derivation)
            .field("user_address", &self.user_address)
            .field("proprietary", &self.proprietary)
            .finish_non_exhaustive()
    }
}

/// The ZIP 32 derivation path at which a key can be found.
#[derive(Debug, Getters, PartialEq, Eq)]
#[getset(get = "pub")]
pub struct Zip32Derivation {
    /// The [ZIP 32 seed fingerprint](https://zips.z.cash/zip-0032#seed-fingerprints).
    seed_fingerprint: [u8; 32],

    /// The sequence of indices corresponding to the shielded HD path.
    derivation_path: Vec<ChildIndex>,
}

impl Zip32Derivation {
    /// Extracts the ZIP 32 account index from this derivation path.
    ///
    /// Returns `None` if the seed fingerprints don't match, or if this is a non-standard
    /// derivation path.
    pub fn extract_account_index(
        &self,
        seed_fp: &zip32::fingerprint::SeedFingerprint,
        expected_coin_type: zip32::ChildIndex,
    ) -> Option<zip32::AccountId> {
        if self.seed_fingerprint == seed_fp.to_bytes() {
            match &self.derivation_path[..] {
                [purpose, coin_type, account_index]
                    if purpose == &zip32::ChildIndex::hardened(32)
                        && coin_type == &expected_coin_type =>
                {
                    Some(
                        zip32::AccountId::try_from(account_index.index() - (1 << 31))
                            .expect("zip32::ChildIndex only supports hardened"),
                    )
                }
                _ => None,
            }
        } else {
            None
        }
    }
}

#[cfg(all(test, feature = "circuit"))]
mod tests {
    use ff::{Field, PrimeField};
    use incrementalmerkletree::{Marking, Retention};
    use pasta_curves::pallas;
    use rand::rngs::OsRng;
    use shardtree::{store::memory::MemoryShardStore, ShardTree};

    use crate::{
        builder::{Builder, BundleMetadata, BundleType},
        bundle::{BundleVersion, Flags},
        circuit::{OrchardCircuitVersion, ProvingKey, VerifyingKey},
        constants::MERKLE_DEPTH_ORCHARD,
        keys::{FullViewingKey, Scope, SpendAuthorizingKey, SpendingKey},
        note::{ExtractedNoteCommitment, NoteVersion, Nullifier, RandomSeed, Rho},
        pczt::{
            IoFinalizerError, ParseError, ProverError, SignerError, TxExtractorError, VerifyError,
            Zip32Derivation,
        },
        primitives::redpallas::{self, SpendAuth},
        tree::{MerkleHashOrchard, MerklePath, EMPTY_ROOTS},
        value::NoteValue,
        Note,
    };

    /// Builds a cross-address-restricted pczt bundle with one real spend (15_000 at an
    /// external address) and one wallet-controlled change output (5_000 at a different
    /// wallet's internal address), without finalizing IO.
    ///
    /// Returns the bundle, its metadata, and the spend authorizing keys for the spend
    /// and the change output respectively.
    fn restricted_pczt_bundle(
        mut rng: OsRng,
    ) -> (
        super::Bundle,
        BundleMetadata,
        SpendAuthorizingKey,
        SpendAuthorizingKey,
    ) {
        let spend_sk = SpendingKey::random(&mut rng);
        let spend_fvk = FullViewingKey::from(&spend_sk);
        let spend_recipient = spend_fvk.address_at(0u32, Scope::External);
        let change_sk = SpendingKey::random(&mut rng);
        let change_fvk = FullViewingKey::from(&change_sk);
        let change_recipient = change_fvk.address_at(0u32, Scope::Internal);
        let bundle_version = BundleVersion::orchard_v3();
        let note_version = bundle_version.note_version();

        let rho = Rho::from_nf_old(Nullifier::dummy(&mut rng));
        let note = Note::new(
            spend_recipient,
            NoteValue::from_raw(15_000),
            rho,
            note_version,
            &mut rng,
        );
        let merkle_path = MerklePath::dummy(&mut rng);
        let anchor = merkle_path.root(note.commitment().into());

        let mut builder = Builder::new(
            BundleType::DEFAULT,
            bundle_version,
            bundle_version.default_flags(),
            anchor,
        )
        .unwrap();
        builder.add_spend(spend_fvk, note, merkle_path).unwrap();
        builder
            .add_change_output(
                change_fvk,
                None,
                change_recipient,
                NoteValue::from_raw(5_000),
                [0u8; 512],
            )
            .unwrap();

        let (pczt_bundle, bundle_meta) = builder.build_for_pczt(&mut rng).unwrap();
        (
            pczt_bundle,
            bundle_meta,
            SpendAuthorizingKey::from(&spend_sk),
            SpendAuthorizingKey::from(&change_sk),
        )
    }

    /// Builds a minimal shielding-style pczt bundle, finalizes IO, and returns it ready for
    /// tests that exercise `create_proof` and `extract`.
    fn minimal_finalized_pczt_bundle(mut rng: OsRng) -> super::Bundle {
        let sk = SpendingKey::random(&mut rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);
        let mut builder = Builder::new(
            BundleType::DEFAULT,
            BundleVersion::orchard_v2(),
            BundleVersion::orchard_v2().default_flags(),
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
        )
        .unwrap();
        builder
            .add_output(None, recipient, NoteValue::from_raw(5000), [0u8; 512])
            .unwrap();
        let mut pczt_bundle = builder.build_for_pczt(&mut rng).unwrap().0;

        let sighash = [0; 32];
        pczt_bundle.finalize_io(sighash, rng).unwrap();
        pczt_bundle
    }

    fn ironwood_output_pczt_bundle(mut rng: OsRng) -> super::Bundle {
        let sk = SpendingKey::random(&mut rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);
        let mut builder = Builder::new(
            BundleType::DEFAULT,
            BundleVersion::ironwood_v3(),
            BundleVersion::ironwood_v3().default_flags(),
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
        )
        .unwrap();
        builder
            .add_output(None, recipient, NoteValue::from_raw(5000), [0u8; 512])
            .unwrap();
        builder.build_for_pczt(&mut rng).unwrap().0
    }

    fn identity_rk() -> redpallas::VerificationKey<SpendAuth> {
        redpallas::VerificationKey::<SpendAuth>::try_from([0u8; 32])
            .expect("plain redpallas accepts the identity encoding")
    }

    #[test]
    fn shielding_bundle() {
        let bundle_version = BundleVersion::orchard_v2();
        let pk = ProvingKey::build(bundle_version.circuit_version());
        let mut rng = OsRng;

        let sk = SpendingKey::random(&mut rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);

        // Run the Creator and Constructor roles.
        let mut builder = Builder::new(
            BundleType::DEFAULT,
            bundle_version,
            bundle_version.default_flags(),
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
        )
        .unwrap();
        builder
            .add_output(None, recipient, NoteValue::from_raw(5000), [0u8; 512])
            .unwrap();
        let balance: i64 = builder.value_balance().unwrap();
        assert_eq!(balance, -5000);
        let mut pczt_bundle = builder.build_for_pczt(&mut rng).unwrap().0;

        // Run the IO Finalizer role.
        let sighash = [0; 32];
        pczt_bundle.finalize_io(sighash, rng).unwrap();

        // Run the Prover role.
        pczt_bundle.create_proof(&pk, rng).unwrap();

        // Run the Transaction Extractor role.
        let bundle = pczt_bundle.extract::<i64>().unwrap().unwrap();

        assert_eq!(bundle.value_balance(), &(-5000));
        // We can successfully bind the bundle.
        bundle.apply_binding_signature(sighash, rng).unwrap();
    }

    #[test]
    fn create_proof_uses_proving_key_circuit_version() {
        let pk = ProvingKey::build(OrchardCircuitVersion::PostNu6_3);
        let vk = VerifyingKey::build(OrchardCircuitVersion::PostNu6_3);
        let rng = OsRng;

        let mut pczt_bundle = minimal_finalized_pczt_bundle(rng);
        let sighash = [0; 32];
        // This is the load-bearing assertion: if PCZT proving still built FixedPostNu6_2
        // circuits unconditionally, `Proof::create` would reject them for this post-NU 6.3 key.
        pczt_bundle.create_proof(&pk, rng).unwrap();

        let bundle = pczt_bundle
            .extract::<i64>()
            .unwrap()
            .unwrap()
            .apply_binding_signature(sighash, rng)
            .unwrap();

        assert!(bundle.verify_proof(&vk).is_ok());
    }

    #[test]
    fn qr_output_version_checks_note_commitment() {
        let mut rng = OsRng;
        let pk = ProvingKey::build(OrchardCircuitVersion::PostNu6_3);
        let vk = VerifyingKey::build(OrchardCircuitVersion::PostNu6_3);

        let sk = SpendingKey::random(&mut rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);

        let mut builder = Builder::new(
            BundleType::DEFAULT,
            BundleVersion::ironwood_v3(),
            BundleVersion::ironwood_v3().default_flags(),
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
        )
        .unwrap();
        builder
            .add_output(None, recipient, NoteValue::from_raw(5000), [0u8; 512])
            .unwrap();
        let (mut pczt_bundle, bundle_meta) = builder.build_for_pczt(&mut rng).unwrap();
        let output_action_index = bundle_meta.output_action_index(0).unwrap();

        let action = &pczt_bundle.actions()[output_action_index];
        assert_eq!(action.output.note_version(), &NoteVersion::V3);
        action
            .output
            .verify_note_commitment(&action.spend)
            .expect("V3 output version verifies the QR note commitment");

        let sighash = [0; 32];
        pczt_bundle.finalize_io(sighash, rng).unwrap();
        pczt_bundle
            .create_proof(&pk, rng)
            .expect("V3 output version reconstructs the QR note for proving");

        pczt_bundle.actions_mut()[output_action_index]
            .output
            .note_version = NoteVersion::V2;
        let action = &pczt_bundle.actions()[output_action_index];
        assert!(matches!(
            action.output.verify_note_commitment(&action.spend),
            Err(VerifyError::InvalidExtractedNoteCommitment)
        ));
        pczt_bundle.create_proof(&pk, rng).unwrap();
        let bundle = pczt_bundle
            .extract::<i64>()
            .unwrap()
            .unwrap()
            .apply_binding_signature(sighash, rng)
            .unwrap();
        assert!(bundle.verify_proof(&vk).is_err());
    }

    #[test]
    fn qr_spend_version_checks_nullifier_and_proves() {
        let pk = ProvingKey::build(OrchardCircuitVersion::PostNu6_3);
        let mut rng = OsRng;

        let sk = SpendingKey::random(&mut rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);

        let value = NoteValue::from_raw(15_000);
        let note = {
            let rho = Rho::from_bytes(&pallas::Base::random(&mut rng).to_repr()).unwrap();
            loop {
                if let Some(note) = Note::from_parts(
                    recipient,
                    value,
                    rho,
                    RandomSeed::random(&mut rng, &rho),
                    NoteVersion::V3,
                )
                .into_option()
                {
                    break note;
                }
            }
        };

        let (anchor, merkle_path) = {
            let cmx: ExtractedNoteCommitment = note.commitment().into();
            let leaf = MerkleHashOrchard::from_cmx(&cmx);
            let mut tree: ShardTree<MemoryShardStore<MerkleHashOrchard, u32>, 32, 16> =
                ShardTree::new(MemoryShardStore::empty(), 100);
            tree.append(
                leaf,
                Retention::Checkpoint {
                    id: 0,
                    marking: Marking::Marked,
                },
            )
            .unwrap();
            let root = tree.root_at_checkpoint_id(&0).unwrap().unwrap();
            let position = tree.max_leaf_position(None).unwrap().unwrap();
            let merkle_path = tree
                .witness_at_checkpoint_id(position, &0)
                .unwrap()
                .unwrap();
            assert_eq!(root, merkle_path.root(MerkleHashOrchard::from_cmx(&cmx)));
            (root.into(), merkle_path)
        };

        let bundle_version = BundleVersion::ironwood_v3();
        let mut builder = Builder::new(
            BundleType::DEFAULT,
            bundle_version,
            bundle_version.default_flags(),
            anchor,
        )
        .unwrap();
        builder
            .add_spend(fvk.clone(), note, merkle_path.into())
            .unwrap();
        builder
            .add_output(None, recipient, NoteValue::from_raw(10_000), [0u8; 512])
            .unwrap();
        let (mut pczt_bundle, bundle_meta) = builder.build_for_pczt(&mut rng).unwrap();
        let spend_action_index = bundle_meta.spend_action_index(0).unwrap();

        let action = &pczt_bundle.actions()[spend_action_index];
        assert_eq!(action.spend.note_version(), &NoteVersion::V3);
        action
            .spend
            .verify_nullifier(None)
            .expect("V3 spend version verifies the QR note nullifier");

        pczt_bundle.finalize_io([0; 32], rng).unwrap();
        pczt_bundle
            .create_proof(&pk, rng)
            .expect("V3 spend version reconstructs the QR note for proving");

        pczt_bundle.actions_mut()[spend_action_index]
            .spend
            .note_version = NoteVersion::V2;
        let action = &pczt_bundle.actions()[spend_action_index];
        assert!(matches!(
            action.spend.verify_nullifier(None),
            Err(VerifyError::InvalidNullifier)
        ));
        assert!(matches!(
            pczt_bundle.create_proof(&pk, rng),
            Err(ProverError::RhoMismatch)
        ));
    }

    #[test]
    fn shielded_bundle() {
        let bundle_version = BundleVersion::orchard_v2();
        let pk = ProvingKey::build(bundle_version.circuit_version());
        let mut rng = OsRng;

        // Pretend we derived the spending key via ZIP 32.
        let zip32_derivation = Zip32Derivation::parse([1; 32], vec![]).unwrap();
        let sk = SpendingKey::random(&mut rng);
        let ask = SpendAuthorizingKey::from(&sk);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);

        // Pretend we already received a note.
        let value = NoteValue::from_raw(15_000);
        let note = {
            let rho = Rho::from_bytes(&pallas::Base::random(&mut rng).to_repr()).unwrap();
            loop {
                if let Some(note) = Note::from_parts(
                    recipient,
                    value,
                    rho,
                    RandomSeed::random(&mut rng, &rho),
                    bundle_version.note_version(),
                )
                .into_option()
                {
                    break note;
                }
            }
        };

        // Use the tree with a single leaf.
        let (anchor, merkle_path) = {
            let cmx: ExtractedNoteCommitment = note.commitment().into();
            let leaf = MerkleHashOrchard::from_cmx(&cmx);
            let mut tree: ShardTree<MemoryShardStore<MerkleHashOrchard, u32>, 32, 16> =
                ShardTree::new(MemoryShardStore::empty(), 100);
            tree.append(
                leaf,
                Retention::Checkpoint {
                    id: 0,
                    marking: Marking::Marked,
                },
            )
            .unwrap();
            let root = tree.root_at_checkpoint_id(&0).unwrap().unwrap();
            let position = tree.max_leaf_position(None).unwrap().unwrap();
            let merkle_path = tree
                .witness_at_checkpoint_id(position, &0)
                .unwrap()
                .unwrap();
            assert_eq!(root, merkle_path.root(MerkleHashOrchard::from_cmx(&cmx)));
            (root.into(), merkle_path)
        };

        // Run the Creator and Constructor roles.
        let bundle_version = BundleVersion::orchard_v2();
        let mut builder = Builder::new(
            BundleType::DEFAULT,
            bundle_version,
            bundle_version.default_flags(),
            anchor,
        )
        .unwrap();
        builder
            .add_spend(fvk.clone(), note, merkle_path.into())
            .unwrap();
        builder
            .add_output(None, recipient, NoteValue::from_raw(10_000), [0u8; 512])
            .unwrap();
        builder
            .add_output(
                Some(fvk.to_ovk(Scope::Internal)),
                fvk.address_at(0u32, Scope::Internal),
                NoteValue::from_raw(5_000),
                [0u8; 512],
            )
            .unwrap();
        let balance: i64 = builder.value_balance().unwrap();
        assert_eq!(balance, 0);
        let mut pczt_bundle = builder.build_for_pczt(&mut rng).unwrap().0;

        // Run the IO Finalizer role.
        let sighash = [0; 32];
        pczt_bundle.finalize_io(sighash, rng).unwrap();

        // Run the Updater role.
        for action in pczt_bundle.actions_mut() {
            if action.spend.value() == &Some(value) {
                action.spend.zip32_derivation = Some(Zip32Derivation {
                    seed_fingerprint: zip32_derivation.seed_fingerprint,
                    derivation_path: zip32_derivation.derivation_path.clone(),
                });
            }
        }

        // Run the Prover role.
        pczt_bundle.create_proof(&pk, rng).unwrap();

        // TODO: Verify that the PCZT contains sufficient information to decrypt and check
        // `enc_ciphertext`.

        // Run the Signer role.
        for action in pczt_bundle.actions_mut() {
            if action.spend.zip32_derivation.as_ref() == Some(&zip32_derivation) {
                action.sign(sighash, &ask, rng).unwrap();

                // We can also apply the signature as an external signature.
                let signature = action.spend().spend_auth_sig().clone().expect("signed");
                action.apply_signature(sighash, signature).unwrap();
            }
        }

        // Run the Transaction Extractor role.
        let bundle = pczt_bundle.extract::<i64>().unwrap().unwrap();

        assert_eq!(bundle.value_balance(), &0);
        // We can successfully bind the bundle.
        bundle.apply_binding_signature(sighash, rng).unwrap();
    }

    /// Proves the preverified signing parse produces a byte-identical `spend_auth_sig` to
    /// the full parse.
    ///
    /// One real Orchard action's on-the-wire spend bytes (including the `fvk`) are re-parsed
    /// twice — once with the full parse (which derives the FVK) and once with the preverified
    /// signing parse (which skips it) — then signed with the same `ask`, `sighash`, and an
    /// identically-seeded RNG. The signatures must match, since a `spend_auth_sig` depends
    /// only on `alpha`, `rk`, and `ask`, never on `fvk`.
    #[test]
    fn preverified_parse_signs_identically_to_full_parse() {
        use super::{Action, Spend};
        use rand::{rngs::StdRng, SeedableRng};

        let bundle_version = BundleVersion::orchard_v2();
        let mut rng = OsRng;

        // Derive the spending key material (the seed-derived `ask` the signer uses).
        let sk = SpendingKey::random(&mut rng);
        let ask = SpendAuthorizingKey::from(&sk);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);

        // Pretend we already received a note.
        let value = NoteValue::from_raw(15_000);
        let note = {
            let rho = Rho::from_bytes(&pallas::Base::random(&mut rng).to_repr()).unwrap();
            loop {
                if let Some(note) = Note::from_parts(
                    recipient,
                    value,
                    rho,
                    RandomSeed::random(&mut rng, &rho),
                    bundle_version.note_version(),
                )
                .into_option()
                {
                    break note;
                }
            }
        };

        // Single-leaf tree, for the witness/anchor.
        let (anchor, merkle_path) = {
            let cmx: ExtractedNoteCommitment = note.commitment().into();
            let leaf = MerkleHashOrchard::from_cmx(&cmx);
            let mut tree: ShardTree<MemoryShardStore<MerkleHashOrchard, u32>, 32, 16> =
                ShardTree::new(MemoryShardStore::empty(), 100);
            tree.append(
                leaf,
                Retention::Checkpoint {
                    id: 0,
                    marking: Marking::Marked,
                },
            )
            .unwrap();
            let root = tree.root_at_checkpoint_id(&0).unwrap().unwrap();
            let position = tree.max_leaf_position(None).unwrap().unwrap();
            let merkle_path = tree
                .witness_at_checkpoint_id(position, &0)
                .unwrap()
                .unwrap();
            assert_eq!(root, merkle_path.root(MerkleHashOrchard::from_cmx(&cmx)));
            (root.into(), merkle_path)
        };

        // Creator + Constructor.
        let mut builder = Builder::new(
            BundleType::DEFAULT,
            bundle_version,
            bundle_version.default_flags(),
            anchor,
        )
        .unwrap();
        builder
            .add_spend(fvk.clone(), note, merkle_path.into())
            .unwrap();
        builder
            .add_output(None, recipient, NoteValue::from_raw(10_000), [0u8; 512])
            .unwrap();
        builder
            .add_output(
                Some(fvk.to_ovk(Scope::Internal)),
                fvk.address_at(0u32, Scope::Internal),
                NoteValue::from_raw(5_000),
                [0u8; 512],
            )
            .unwrap();
        let (mut pczt_bundle, _meta) = builder.build_for_pczt(&mut rng).unwrap();

        // IO Finalizer sets the `alpha`/`rk` consistency needed for signing.
        let sighash = [0u8; 32];
        pczt_bundle.finalize_io(sighash, rng).unwrap();

        // Find the real spend action (the one whose `ask` we hold) and capture the raw
        // on-the-wire component bytes of its spend. The `fvk` IS present on the wire here,
        // so the full parse will do the FVK derivation and the preverified signing parse
        // will skip it.
        let action = pczt_bundle
            .actions()
            .iter()
            .find(|a| a.spend().value() == &Some(value))
            .expect("the real spend is present");
        let spend = action.spend();

        // Sanity: this spend really carries an `fvk` on the wire (otherwise the test would
        // be vacuous because both parses would skip the derivation).
        assert!(
            spend.fvk().is_some(),
            "test precondition: the captured spend must carry an fvk on the wire",
        );
        // Sanity: `alpha` and a real (non-`None`) signature randomizer are present.
        assert!(
            spend.alpha().is_some(),
            "alpha must be set after IO finalize"
        );

        // Raw component bytes (the same encoding `pczt::Bundle::serialize_from` produces).
        let cv_net_bytes: [u8; 32] = action.cv_net().to_bytes();
        let nf_bytes: [u8; 32] = spend.nullifier().to_bytes();
        let rk_bytes: [u8; 32] = spend.rk().into();
        let recipient_bytes = spend.recipient().map(|r| r.to_raw_address_bytes());
        let value_raw = spend.value().map(|v| v.inner());
        let rho_bytes = spend.rho().map(|rho| rho.to_bytes());
        let rseed_bytes = spend.rseed().map(|rseed| *rseed.as_bytes());
        let fvk_bytes = spend.fvk().as_ref().map(|fvk| fvk.to_bytes());
        let witness_bytes = spend.witness().as_ref().map(|witness| {
            (
                u32::try_from(u64::from(witness.position())).unwrap(),
                witness
                    .auth_path()
                    .iter()
                    .map(|node| node.to_bytes())
                    .collect::<alloc::vec::Vec<_>>()[..]
                    .try_into()
                    .expect("path is length 32"),
            )
        });
        let alpha_bytes = spend.alpha().map(|alpha| alpha.to_repr());
        let note_version = *spend.note_version();

        // Build the spend's component bytes once, and a helper to parse with either path.
        // The output half is irrelevant to signing but is required to assemble an `Action`;
        // reuse the real output's bytes through the full output parse for both branches.
        let output = action.output();
        let build_output = || {
            super::Output::parse(
                *spend.nullifier(),
                output.cmx().to_bytes(),
                output.encrypted_note().epk_bytes,
                output.encrypted_note().enc_ciphertext.to_vec(),
                output.encrypted_note().out_ciphertext.to_vec(),
                output.recipient().map(|r| r.to_raw_address_bytes()),
                output.value().map(|v| v.inner()),
                output.rseed().map(|rseed| *rseed.as_bytes()),
                output.ock().as_ref().map(|ock| ock.0),
                None,
                output.user_address().clone(),
                *output.note_version(),
                output.proprietary().clone(),
            )
            .expect("output re-parses")
        };
        let rcv_bytes = action.rcv().as_ref().map(|rcv| rcv.to_bytes());

        // === FULL parse path (derives the FVK) ===
        let full_spend = Spend::parse(
            nf_bytes,
            rk_bytes,
            None,
            recipient_bytes,
            value_raw,
            rho_bytes,
            rseed_bytes,
            fvk_bytes,
            witness_bytes,
            alpha_bytes,
            None,
            None,
            note_version,
            alloc::collections::BTreeMap::new(),
        )
        .expect("full spend parse");
        assert!(full_spend.fvk().is_some(), "full parse must derive the fvk",);
        let mut full_action =
            Action::parse(cv_net_bytes, full_spend, build_output(), rcv_bytes).unwrap();

        // === PREVERIFIED SIGNING parse path (skips the FVK) ===
        let preverified_spend = Spend::parse_preverified_for_signing(
            nf_bytes,
            rk_bytes,
            None,
            recipient_bytes,
            value_raw,
            rho_bytes,
            rseed_bytes,
            fvk_bytes,
            witness_bytes,
            alpha_bytes,
            None,
            None,
            note_version,
            alloc::collections::BTreeMap::new(),
        )
        .expect("preverified signing spend parse");
        assert!(
            preverified_spend.fvk().is_none(),
            "preverified signing parse must omit the fvk",
        );
        let mut preverified_action =
            Action::parse(cv_net_bytes, preverified_spend, build_output(), rcv_bytes).unwrap();

        // Sign both with the SAME ask, sighash, and an identically-seeded RNG. RedPallas
        // signing is randomized, so the RNGs must match for the signatures to be comparable.
        let seed = [7u8; 32];
        full_action
            .sign(sighash, &ask, StdRng::from_seed(seed))
            .expect("full-parsed action signs");
        preverified_action
            .sign(sighash, &ask, StdRng::from_seed(seed))
            .expect("preverified signing action signs");

        let full_sig: [u8; 64] = (&full_action
            .spend()
            .spend_auth_sig()
            .clone()
            .expect("full action signed"))
            .into();
        let preverified_sig: [u8; 64] = (&preverified_action
            .spend()
            .spend_auth_sig()
            .clone()
            .expect("preverified signing action signed"))
            .into();

        assert_eq!(
            full_sig, preverified_sig,
            "preverified signing signature must be byte-identical to the full-parsed signature",
        );

        // And both must verify against `rk` (defends against two matching-but-wrong sigs).
        full_action
            .apply_signature(
                sighash,
                redpallas::Signature::<SpendAuth>::from(preverified_sig),
            )
            .expect("the preverified signing signature verifies against the full action's rk");

        // The one behavioural difference: the full parse rejects a malformed `fvk`, the
        // preverified parse accepts it as `fvk: None` (sound per the invariant on that method).
        let bad_fvk = Some([0xFFu8; 96]);
        assert!(
            matches!(
                Spend::parse(
                    nf_bytes,
                    rk_bytes,
                    None,
                    recipient_bytes,
                    value_raw,
                    rho_bytes,
                    rseed_bytes,
                    bad_fvk,
                    witness_bytes,
                    alpha_bytes,
                    None,
                    None,
                    note_version,
                    alloc::collections::BTreeMap::new(),
                ),
                Err(super::ParseError::InvalidFullViewingKey)
            ),
            "the full parse must reject malformed fvk bytes",
        );
        let preverified_bad_fvk = Spend::parse_preverified_for_signing(
            nf_bytes,
            rk_bytes,
            None,
            recipient_bytes,
            value_raw,
            rho_bytes,
            rseed_bytes,
            bad_fvk,
            witness_bytes,
            alpha_bytes,
            None,
            None,
            note_version,
            alloc::collections::BTreeMap::new(),
        )
        .expect("the preverified signing parse ignores malformed fvk bytes");
        assert!(
            preverified_bad_fvk.fvk().is_none(),
            "the preverified signing parse must leave fvk None even when the wire fvk bytes are malformed",
        );
    }

    #[test]
    fn create_proof_rejects_identity_rk() {
        let pk = ProvingKey::build(OrchardCircuitVersion::FixedPostNu6_2);
        let rng = OsRng;

        let mut pczt_bundle = minimal_finalized_pczt_bundle(rng);
        pczt_bundle.actions_mut()[0].spend.rk = identity_rk();

        assert!(matches!(
            pczt_bundle.create_proof(&pk, rng),
            Err(ProverError::IdentityRk),
        ));
    }

    #[test]
    fn extract_rejects_identity_rk() {
        let pk = ProvingKey::build(OrchardCircuitVersion::FixedPostNu6_2);
        let rng = OsRng;

        let mut pczt_bundle = minimal_finalized_pczt_bundle(rng);
        pczt_bundle.create_proof(&pk, rng).unwrap();

        // Inject identity rk after a valid proof has been produced. Extract
        // should reject at the `Action::from_parts` step, before any proof or
        // signature check.
        pczt_bundle.actions_mut()[0].spend.rk = identity_rk();

        assert!(matches!(
            pczt_bundle.extract::<i64>(),
            Err(TxExtractorError::IdentityRk),
        ));
    }

    #[test]
    fn extract_rejects_non_canonical_proof() {
        let pk = ProvingKey::build(OrchardCircuitVersion::FixedPostNu6_2);
        let rng = OsRng;

        let mut pczt_bundle = minimal_finalized_pczt_bundle(rng);
        pczt_bundle.create_proof(&pk, rng).unwrap();

        // Pad the proof with a trailing byte after it was produced. Extraction must reject the
        // non-canonical proof rather than carry it into the extracted (and later authorized)
        // bundle.
        let padded = {
            let mut bytes = pczt_bundle.zkproof.as_ref().unwrap().as_ref().to_vec();
            bytes.push(0);
            crate::Proof::new(bytes)
        };
        pczt_bundle.zkproof = Some(padded);

        assert!(matches!(
            pczt_bundle.extract::<i64>(),
            Err(TxExtractorError::NonCanonicalProofSize { .. }),
        ));
    }

    #[test]
    fn parse_uses_bundle_version_for_flags() {
        let anchor: crate::Anchor = EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into();

        // Bit 2 is reserved pre-NU6.3, and rejected for Orchard post-NU6.3 (which mandates
        // the cross-address restriction); only Ironwood may set it.
        for pr in [
            BundleVersion::orchard_insecure_v1(),
            BundleVersion::orchard_v2(),
            BundleVersion::orchard_v3(),
        ] {
            assert!(matches!(
                super::Bundle::parse(
                    vec![],
                    0b0000_0100,
                    pr,
                    (0, false),
                    anchor.to_bytes(),
                    None,
                    None,
                ),
                Err(ParseError::UnexpectedFlagBitsSet),
            ));
        }

        let parsed = super::Bundle::parse(
            vec![],
            0b0000_0100,
            BundleVersion::ironwood_v3(),
            (0, false),
            anchor.to_bytes(),
            None,
            None,
        )
        .unwrap();

        assert!(parsed.flags().cross_address_enabled());
        assert_eq!(
            parsed.flags().to_byte(BundleVersion::ironwood_v3()),
            Some(0b0000_0100)
        );
        assert_eq!(
            parsed.flags().to_byte(BundleVersion::orchard_v2()),
            Some(0b0000_0000)
        );

        let restricted = super::Bundle::parse(
            vec![],
            0b0000_0011,
            BundleVersion::orchard_v3(),
            (0, false),
            anchor.to_bytes(),
            None,
            None,
        )
        .unwrap();

        assert!(!restricted.flags().cross_address_enabled());
        assert_eq!(
            restricted.flags().to_byte(BundleVersion::orchard_v3()),
            Some(0b0000_0011)
        );
        assert_eq!(
            restricted.flags().to_byte(BundleVersion::orchard_v2()),
            None
        );
    }

    #[test]
    fn parse_preserves_note_versions() {
        let bundle_version = BundleVersion::ironwood_v3();
        let pczt_bundle = ironwood_output_pczt_bundle(OsRng);
        let flags = pczt_bundle.flags.to_byte(bundle_version).unwrap();
        let anchor = pczt_bundle.anchor.to_bytes();
        let actions = pczt_bundle.actions;

        let parsed = super::Bundle::parse(
            actions,
            flags,
            bundle_version,
            (5000, true),
            anchor,
            None,
            None,
        )
        .unwrap();
        let action = &parsed.actions()[0];

        assert_eq!(action.spend().note_version(), &NoteVersion::V3);
        assert_eq!(action.output().note_version(), &NoteVersion::V3);
    }

    #[test]
    fn parse_rejects_output_note_version_mismatch() {
        let bundle_version = BundleVersion::ironwood_v3();
        let pczt_bundle = ironwood_output_pczt_bundle(OsRng);
        let flags = pczt_bundle.flags.to_byte(bundle_version).unwrap();
        let anchor = pczt_bundle.anchor.to_bytes();
        let mut actions = pczt_bundle.actions;
        actions[0].output.note_version = NoteVersion::V2;

        assert!(matches!(
            super::Bundle::parse(
                actions,
                flags,
                bundle_version,
                (5000, true),
                anchor,
                None,
                None,
            ),
            Err(ParseError::InvalidNoteVersion)
        ));
    }

    #[test]
    fn create_proof_supports_cross_address_disabled_only_for_post_nu6_3() {
        let rng = OsRng;
        let sighash = [0; 32];

        // Structural same-expanded-receiver violations are rejected before any key-capability
        // check, for every circuit version.
        for circuit_version in [
            OrchardCircuitVersion::FixedPostNu6_2,
            OrchardCircuitVersion::PostNu6_3,
        ] {
            let pk = ProvingKey::build(circuit_version);

            let mut mismatched_pczt_bundle = minimal_finalized_pczt_bundle(rng);
            mismatched_pczt_bundle.flags = Flags::CROSS_ADDRESS_DISABLED;
            assert!(matches!(
                mismatched_pczt_bundle.create_proof(&pk, rng),
                Err(ProverError::DisallowedCrossAddressTransfer(_)),
            ));
        }

        let (mut pczt_bundle, bundle_meta, spend_ask, change_ask) = restricted_pczt_bundle(rng);
        pczt_bundle.finalize_io(sighash, rng).unwrap();

        // A pre-NU 6.3 proving key rejects the structurally-conforming restricted
        // statement at the instance check, leaving the bundle unmodified.
        let pk = ProvingKey::build(OrchardCircuitVersion::FixedPostNu6_2);
        assert!(matches!(
            pczt_bundle.create_proof(&pk, rng),
            Err(ProverError::ProofFailed(
                halo2_proofs::plonk::Error::InvalidInstances
            )),
        ));
        assert!(pczt_bundle.zkproof.is_none());

        // A post-NU 6.3 proving key proves the same statement, and the proof verifies
        // in the extracted bundle under the post-NU 6.3 verifying key.
        let pk = ProvingKey::build(OrchardCircuitVersion::PostNu6_3);
        pczt_bundle.create_proof(&pk, rng).unwrap();

        pczt_bundle.actions_mut()[bundle_meta.spend_action_index(0).unwrap()]
            .sign(sighash, &spend_ask, rng)
            .unwrap();
        pczt_bundle.actions_mut()[bundle_meta.output_action_index(0).unwrap()]
            .sign(sighash, &change_ask, rng)
            .unwrap();

        let bundle = pczt_bundle
            .extract::<i64>()
            .unwrap()
            .unwrap()
            .apply_binding_signature(sighash, rng)
            .unwrap();
        bundle
            .verify_proof(&VerifyingKey::build(OrchardCircuitVersion::PostNu6_3))
            .unwrap();
    }

    #[test]
    fn restricted_pczt_signing_flow() {
        let rng = OsRng;
        let (mut pczt_bundle, bundle_meta, spend_ask, change_ask) = restricted_pczt_bundle(rng);

        let sighash = [0; 32];
        pczt_bundle.finalize_io(sighash, rng).unwrap();
        pczt_bundle.verify_cross_address_restriction().unwrap();

        let spend_action_index = bundle_meta.spend_action_index(0).unwrap();
        let change_action_index = bundle_meta.output_action_index(0).unwrap();
        assert_ne!(spend_action_index, change_action_index);

        // The fabricated change spend is wallet-controlled: it is signed through the
        // normal Signer flow, and only by the matching spend authorizing key.
        assert!(matches!(
            pczt_bundle.actions_mut()[change_action_index].sign(sighash, &spend_ask, rng),
            Err(SignerError::WrongSpendAuthorizingKey),
        ));
        pczt_bundle.actions_mut()[change_action_index]
            .sign(sighash, &change_ask, rng)
            .unwrap();
        pczt_bundle.actions_mut()[spend_action_index]
            .sign(sighash, &spend_ask, rng)
            .unwrap();

        for action in pczt_bundle.actions() {
            assert!(action.spend.spend_auth_sig.is_some());
            assert!(action.spend.dummy_sk.is_none());
        }
    }

    #[test]
    fn restricted_pczt_io_finalizer_signs_padding_dummy() {
        let mut rng = OsRng;
        let spend_sk = SpendingKey::random(&mut rng);
        let spend_fvk = FullViewingKey::from(&spend_sk);
        let spend_recipient = spend_fvk.address_at(0u32, Scope::External);
        let note_version = NoteVersion::V2;

        let rho = Rho::from_nf_old(Nullifier::dummy(&mut rng));
        let note = Note::new(
            spend_recipient,
            NoteValue::from_raw(15_000),
            rho,
            note_version,
            &mut rng,
        );
        let merkle_path = MerklePath::dummy(&mut rng);
        let anchor = merkle_path.root(note.commitment().into());

        let bundle_version = BundleVersion::orchard_v3();
        let mut builder = Builder::new(
            BundleType::DEFAULT,
            bundle_version,
            bundle_version.default_flags(),
            anchor,
        )
        .unwrap();
        builder.add_spend(spend_fvk, note, merkle_path).unwrap();

        let (mut pczt_bundle, bundle_meta) = builder.build_for_pczt(&mut rng).unwrap();
        assert_eq!(pczt_bundle.actions().len(), 2);

        let spend_action_index = bundle_meta.spend_action_index(0).unwrap();
        let padding_action_index = 1 - spend_action_index;
        assert!(pczt_bundle.actions()[padding_action_index]
            .spend
            .dummy_sk
            .is_some());

        let sighash = [0; 32];
        pczt_bundle.finalize_io(sighash, rng).unwrap();

        // The IO Finalizer signed the padding dummy spend and cleared its `dummy_sk`;
        // the real spend still needs its signature.
        let padding_action = &pczt_bundle.actions()[padding_action_index];
        assert!(padding_action.spend.dummy_sk.is_none());
        assert!(padding_action.spend.spend_auth_sig.is_some());
        assert!(pczt_bundle.actions()[spend_action_index]
            .spend
            .spend_auth_sig
            .is_none());

        pczt_bundle.actions_mut()[spend_action_index]
            .sign(sighash, &SpendAuthorizingKey::from(&spend_sk), rng)
            .unwrap();
    }

    #[test]
    fn finalize_io_rejects_cross_address_violation() {
        let mut rng = OsRng;
        let (mut pczt_bundle, _, _, _) = restricted_pczt_bundle(rng);

        let spend_recipient = pczt_bundle.actions()[0].spend.recipient.unwrap();
        let other_recipient = loop {
            let fvk = FullViewingKey::from(&SpendingKey::random(&mut rng));
            let recipient = fvk.address_at(0u32, Scope::External);
            if !spend_recipient.same_expanded_receiver(&recipient) {
                break recipient;
            }
        };
        pczt_bundle.actions_mut()[0].output.recipient = Some(other_recipient);

        assert!(matches!(
            pczt_bundle.finalize_io([0; 32], rng),
            Err(IoFinalizerError::CrossAddressRestriction(
                VerifyError::DisallowedCrossAddressTransfer
            )),
        ));
        // The failed call left the bundle unmodified.
        assert!(pczt_bundle.bsk.is_none());
    }

    #[test]
    fn verify_cross_address_restriction_requires_recipients() {
        let mut pczt_bundle = minimal_finalized_pczt_bundle(OsRng);
        pczt_bundle.flags = Flags::CROSS_ADDRESS_DISABLED;
        for action in pczt_bundle.actions_mut() {
            action.output.recipient = action.spend.recipient;
        }
        pczt_bundle.verify_cross_address_restriction().unwrap();

        let original = pczt_bundle.actions()[0].spend.recipient;
        pczt_bundle.actions_mut()[0].spend.recipient = None;
        assert!(matches!(
            pczt_bundle.verify_cross_address_restriction(),
            Err(VerifyError::MissingRecipient),
        ));

        pczt_bundle.actions_mut()[0].spend.recipient = original;
        pczt_bundle.actions_mut()[0].output.recipient = None;
        assert!(matches!(
            pczt_bundle.verify_cross_address_restriction(),
            Err(VerifyError::MissingRecipient),
        ));
    }

    #[test]
    fn extract_preserves_cross_address_disabled() {
        let rng = OsRng;

        let mut pczt_bundle = minimal_finalized_pczt_bundle(rng);
        pczt_bundle.zkproof = Some(crate::Proof::new(vec![
            0;
            crate::Proof::expected_proof_size(
                pczt_bundle.actions.len()
            )
        ]));
        // Cross-address-disabled flags are only representable from NU6.3 onward, and the Orchard
        // pool at NU6.3 mandates the restriction; that is the version under which an extracted
        // bundle can legitimately carry these flags.
        pczt_bundle.bundle_version = BundleVersion::orchard_v3();
        pczt_bundle.flags = Flags::CROSS_ADDRESS_DISABLED;

        let bundle = pczt_bundle.extract::<i64>().unwrap().unwrap();
        assert!(!bundle.flags().cross_address_enabled());
    }
}
