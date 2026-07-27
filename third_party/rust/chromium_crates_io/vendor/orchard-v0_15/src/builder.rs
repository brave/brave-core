//! Logic for building Orchard components of transactions.

use alloc::collections::BTreeMap;
use alloc::vec::Vec;
use core::fmt;
use core::iter;

use ff::Field;
use pasta_curves::pallas;
use rand::{prelude::SliceRandom, CryptoRng, RngCore};
use zcash_note_encryption::ENC_CIPHERTEXT_SIZE;

use crate::{
    address::Address,
    bundle::{Authorization, Authorized, Bundle, BundleVersion, Flags},
    keys::{
        FullViewingKey, OutgoingViewingKey, Scope, SpendAuthorizingKey, SpendValidatingKey,
        SpendingKey,
    },
    note::{ExtractedNoteCommitment, Note, NoteVersion, Nullifier, Rho, TransmittedNoteCiphertext},
    note_encryption::OrchardNoteEncryption,
    primitives::redpallas::{self, Binding, SpendAuth},
    tree::{Anchor, MerklePath},
    value::{self, BalanceError, NoteValue, ValueCommitTrapdoor, ValueCommitment, ValueSum},
    Proof,
};

#[cfg(feature = "circuit")]
use {
    crate::{
        action::Action,
        circuit::{Circuit, Instance, OrchardCircuitVersion, ProvingKey},
    },
    nonempty::NonEmpty,
};

const DEFAULT_MIN_ACTIONS: u8 = 2;

/// An enumeration of rules for Orchard bundle construction.
///
/// This selects only the construction discipline; the bundle's [`Flags`] are
/// supplied separately to the builder (see [`Builder::new`] and
/// [`BundleVersion::default_flags`]).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BundleType {
    /// A transactional bundle is padded to contain at least the configured
    /// `pad_to_minimum` actions, defaulting to 2 actions when
    /// `pad_to_minimum` is [`None`].
    Transactional {
        /// A flag that, when set to `true`, indicates that a bundle should be
        /// produced even if no spends or outputs have been added to the bundle;
        /// in such a circumstance, all of the actions in the resulting bundle
        /// will be dummies.
        bundle_required: bool,
        /// The minimum number of actions to pad the transaction to.
        ///
        /// If this is [`None`], the default 2-action minimum is used. Set this
        /// to `Some(1)` for transactions whose shape is already public: the
        /// bundle then contains exactly the requested actions (at least one, if
        /// `bundle_required` is set). One-action bundles are consensus-valid
        /// ([`BundleType::Coinbase`] bundles are never padded); the default minimum
        /// exists to improve indistinguishability, so reducing it lets the action
        /// count reveal the transaction shape.
        pad_to_minimum: Option<u8>,
    },
    /// A coinbase bundle performs no padding and requires the bundle's flags to
    /// disable spends.
    ///
    /// Since coinbase transactions have `enableSpends = 0`, every spend must be
    /// a dummy. Coinbase transactions are not otherwise any different wrt
    /// cross-address restrictions from other transactions that have dummy
    /// inputs.
    Coinbase,
}

impl BundleType {
    /// The default bundle type: a transactional bundle padded to the default
    /// minimum action count, and not required to be produced if no spends or
    /// outputs have been added.
    pub const DEFAULT: BundleType = BundleType::Transactional {
        bundle_required: false,
        pad_to_minimum: Some(DEFAULT_MIN_ACTIONS),
    };

    /// Unpadded transactional bundle: the bundle is padded only to the
    /// one-action consensus minimum, so it contains exactly the requested
    /// actions. Intended for transactions whose shape is already public — such
    /// as pool migrations, where the per-pool value balances reveal the
    /// transfer — since the action count reveals the transaction shape (see
    /// [`BundleType::Transactional::pad_to_minimum`]).
    pub const UNPADDED: BundleType = BundleType::Transactional {
        bundle_required: false,
        pad_to_minimum: Some(1),
    };

    /// Returns the number of logical actions that the builder will produce in constructing a bundle
    /// of this type with the given `flags`, given the specified numbers of spends and outputs.
    ///
    /// In the current implementation, for a bundle (regardless of type) that disables
    /// cross-address transfers, a requested spend and a requested output do not share an
    /// action (each is paired with a fabricated zero-valued counterpart), so the number of
    /// requested actions is `num_spends + num_outputs` rather than `max(num_spends, num_outputs)`.
    /// Wallets estimating fees (e.g. per [ZIP 317]) must account for this larger action
    /// count.
    ///
    /// Returns an error if the specified number of spends and outputs is incompatible with
    /// this bundle type and flags.
    ///
    /// [ZIP 317]: https://zips.z.cash/zip-0317
    pub fn num_actions(
        &self,
        flags: Flags,
        num_spends: usize,
        num_outputs: usize,
    ) -> Result<usize, &'static str> {
        match self {
            BundleType::Transactional {
                bundle_required,
                pad_to_minimum,
            } => {
                // When cross-address transfers are disabled, every action's output is
                // addressed to the note it spends. For this implementation, a requested
                // spend and a requested output never share an action: each is paired with
                // a fabricated zero-valued counterpart instead.
                let num_requested_actions = if !flags.cross_address_enabled() {
                    num_spends
                        .checked_add(num_outputs)
                        .ok_or("num_spends + num_outputs overflowed")?
                } else {
                    core::cmp::max(num_spends, num_outputs)
                };

                if !flags.spends_enabled() && num_spends > 0 {
                    Err("Spends are disabled, so num_spends must be zero")
                } else if !flags.outputs_enabled() && num_outputs > 0 {
                    Err("Outputs are disabled, so num_outputs must be zero")
                } else {
                    let mut min_actions =
                        usize::from(pad_to_minimum.unwrap_or(DEFAULT_MIN_ACTIONS));
                    if *bundle_required {
                        min_actions = core::cmp::max(min_actions, 1);
                    }

                    Ok(if *bundle_required || num_requested_actions > 0 {
                        core::cmp::max(num_requested_actions, min_actions)
                    } else {
                        0
                    })
                }
            }
            BundleType::Coinbase => {
                if num_spends > 0 {
                    Err("Coinbase bundles have spends disabled, so num_spends must be zero")
                } else {
                    Ok(num_outputs)
                }
            }
        }
    }
}

/// An error type for the kinds of errors that can occur during bundle construction.
#[derive(Debug)]
#[non_exhaustive]
pub enum BuildError {
    /// Spends are disabled for the provided bundle type.
    SpendsDisabled,
    /// Outputs are disabled for the provided bundle type.
    OutputsDisabled,
    /// The anchor provided to this builder doesn't match the Merkle path used to add a spend.
    AnchorMismatch,
    /// A bundle could not be built because required signatures were missing.
    MissingSignatures,
    /// An error occurred in the process of producing a proof for a bundle.
    #[cfg(feature = "circuit")]
    Proof(halo2_proofs::plonk::Error),
    /// An overflow error occurred while attempting to construct the value
    /// for a bundle.
    ValueSum(value::BalanceError),
    /// External signature is not valid.
    InvalidExternalSignature,
    /// A signature is valid for more than one input. This should never happen if `alpha`
    /// is sampled correctly, and indicates a critical failure in randomness generation.
    DuplicateSignature,
    /// The bundle being constructed violated the construction rules for the requested bundle type.
    BundleTypeNotSatisfiable,
    /// Cross-address transfers are disabled for the bundle being constructed, and an
    /// output is not a wallet-controlled change output.
    CrossAddressDisabled,
    /// A supplied output or change output has a note version that is
    /// inconsistent with the bundle version.
    InvalidNoteVersion,
    /// The builder's flags cannot be encoded under its [`BundleVersion`].
    UnrepresentableFlags,
    /// A coinbase bundle was requested with flags that enable spends.
    CoinbaseSpendsEnabled,
}

impl fmt::Display for BuildError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use BuildError::*;
        match self {
            MissingSignatures => f.write_str("Required signatures were missing during build"),
            #[cfg(feature = "circuit")]
            Proof(halo2_proofs::plonk::Error::InvalidInstances) => f.write_str(
                "Could not create proof: provided instances do not match the circuit, or \
                     the cross-address restriction is not supported by the proving key's \
                     circuit version",
            ),
            #[cfg(feature = "circuit")]
            Proof(e) => write!(f, "Could not create proof: {e}"),
            ValueSum(_) => f.write_str("Overflow occurred during value construction"),
            InvalidExternalSignature => f.write_str("External signature was invalid"),
            DuplicateSignature => f.write_str("Signature valid for more than one input"),
            BundleTypeNotSatisfiable => {
                f.write_str("Bundle structure did not conform to requested bundle type.")
            }
            SpendsDisabled => f.write_str("Spends are not enabled for the requested bundle type."),
            OutputsDisabled => {
                f.write_str("Outputs are not enabled for the requested bundle type.")
            }
            AnchorMismatch => {
                f.write_str("All spends must share the anchor requested for the transaction.")
            }
            CrossAddressDisabled => f.write_str(
                "Cross-address transfers are disabled for this bundle: every output must \
                 be a wallet-controlled change output.",
            ),
            InvalidNoteVersion => f.write_str(
                "A supplied output or change output has a note version that does not match \
                 the bundle version.",
            ),
            UnrepresentableFlags => f.write_str(
                "The requested flags cannot be encoded under the requested bundle version.",
            ),
            CoinbaseSpendsEnabled => {
                f.write_str("A coinbase bundle was requested with flags that enable spends.")
            }
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for BuildError {}

#[cfg(feature = "circuit")]
impl From<halo2_proofs::plonk::Error> for BuildError {
    fn from(e: halo2_proofs::plonk::Error) -> Self {
        BuildError::Proof(e)
    }
}

impl From<value::BalanceError> for BuildError {
    fn from(e: value::BalanceError) -> Self {
        BuildError::ValueSum(e)
    }
}

/// An error type for adding a spend to the builder.
#[derive(Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum SpendError {
    /// Spends aren't enabled for this builder.
    SpendsDisabled,
    /// The anchor provided to this builder doesn't match the merkle path used to add a spend.
    AnchorMismatch,
    /// The full viewing key provided didn't match the note provided
    FvkMismatch,
}

impl fmt::Display for SpendError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use SpendError::*;
        f.write_str(match self {
            SpendsDisabled => "Spends are not enabled for this builder",
            AnchorMismatch => "All anchors must be equal.",
            FvkMismatch => "FullViewingKey does not correspond to the given note",
        })
    }
}

#[cfg(feature = "std")]
impl std::error::Error for SpendError {}

/// An error type for adding an output to the builder.
#[derive(Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum OutputError {
    /// Outputs aren't enabled for this builder.
    OutputsDisabled,
    /// Spends aren't enabled for this builder. A wallet-controlled change output in a bundle
    /// that disables cross-address transfers pairs with a fabricated spend, so we do not support
    /// change outputs when both spends and cross-address transfers are disabled.
    SpendsDisabled,
    /// Cross-address transfers are disabled for this builder, so ordinary outputs cannot
    /// be added; use [`Builder::add_change_output`] for wallet-controlled change.
    CrossAddressDisabled,
    /// The full viewing key provided does not own the recipient address.
    RecipientNotOwned,
}

impl fmt::Display for OutputError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use OutputError::*;
        f.write_str(match self {
            OutputsDisabled => "Outputs are not enabled for this builder",
            SpendsDisabled => "Spends are not enabled for this builder",
            CrossAddressDisabled => {
                "Cross-address transfers are disabled for this builder; use \
                 add_change_output for wallet-controlled change"
            }
            RecipientNotOwned => "FullViewingKey does not own the recipient address",
        })
    }
}

#[cfg(feature = "std")]
impl std::error::Error for OutputError {}

/// Information about a specific note to be spent in an [`Action`].
#[derive(Debug)]
pub struct SpendInfo {
    pub(crate) dummy_sk: Option<SpendingKey>,
    pub(crate) fvk: FullViewingKey,
    pub(crate) scope: Scope,
    pub(crate) note: Note,
    pub(crate) merkle_path: MerklePath,
}

impl SpendInfo {
    /// This constructor is public to enable creation of custom builders.
    /// If you are not creating a custom builder, use [`Builder::add_spend`] instead.
    ///
    /// Creates a `SpendInfo` from note, full viewing key owning the note,
    /// and merkle path witness of the note.
    ///
    /// Returns `None` if the `fvk` does not own the `note`.
    ///
    /// [`Builder::add_spend`]: Builder::add_spend
    pub fn new(fvk: FullViewingKey, note: Note, merkle_path: MerklePath) -> Option<Self> {
        let scope = fvk.scope_for_address(&note.recipient())?;
        Some(SpendInfo {
            dummy_sk: None,
            fvk,
            scope,
            note,
            merkle_path,
        })
    }

    /// Defined in [Zcash Protocol Spec § 4.8.3: Dummy Notes (Orchard)][orcharddummynotes].
    ///
    /// [orcharddummynotes]: https://zips.z.cash/protocol/nu5.pdf#orcharddummynotes
    fn dummy(note_version: NoteVersion, rng: &mut impl RngCore) -> Self {
        let (sk, fvk, note) = Note::dummy(rng, None, note_version);
        let merkle_path = MerklePath::dummy(rng);

        SpendInfo {
            dummy_sk: Some(sk),
            fvk,
            // We use external scope to avoid unnecessary derivations, because the dummy
            // note's spending key is random and thus scoping is irrelevant.
            scope: Scope::External,
            note,
            merkle_path,
        }
    }

    fn has_matching_anchor(&self, anchor: &Anchor) -> bool {
        if self.note.value() == NoteValue::ZERO {
            true
        } else {
            let cm = self.note.commitment();
            let path_root = self.merkle_path.root(cm.into());
            &path_root == anchor
        }
    }

    /// Builds the spend half of an action.
    ///
    /// The returned values are chosen as in [Zcash Protocol Spec § 4.7.3: Sending Notes (Orchard)][orchardsend].
    ///
    /// [orchardsend]: https://zips.z.cash/protocol/nu5.pdf#orchardsend
    fn build(
        &self,
        mut rng: impl RngCore,
    ) -> (
        Nullifier,
        SpendValidatingKey,
        pallas::Scalar,
        redpallas::VerificationKey<SpendAuth>,
    ) {
        let nf_old = self.note.nullifier(&self.fvk);
        let ak: SpendValidatingKey = self.fvk.clone().into();
        let alpha = pallas::Scalar::random(&mut rng);
        let rk = ak.randomize(&alpha);

        (nf_old, ak, alpha, rk)
    }

    fn into_pczt(self, rng: impl RngCore) -> crate::pczt::Spend {
        let (nf_old, _, alpha, rk) = self.build(rng);

        crate::pczt::Spend {
            nullifier: nf_old,
            rk,
            spend_auth_sig: None,
            recipient: Some(self.note.recipient()),
            value: Some(self.note.value()),
            rho: Some(self.note.rho()),
            rseed: Some(*self.note.rseed()),
            note_version: self.note.version(),
            fvk: Some(self.fvk),
            witness: Some(self.merkle_path),
            alpha: Some(alpha),
            zip32_derivation: None,
            dummy_sk: self.dummy_sk,
            proprietary: BTreeMap::new(),
        }
    }
}

/// Information about a specific output to receive funds in an [`Action`].
///
/// This carries a plain output to an arbitrary recipient. For wallet-controlled change,
/// which additionally records the owning full viewing key, see [`ChangeInfo`].
#[derive(Debug)]
pub struct OutputInfo {
    ovk: Option<OutgoingViewingKey>,
    recipient: Address,
    value: NoteValue,
    memo: [u8; 512],
    note_version: NoteVersion,
    /// When set, `build` fills `enc_ciphertext` with random bytes instead of encrypting the
    /// note to `recipient`. This is used only for the zero-valued output that a builder with
    /// cross-address actions disabled pairs with a real external-scope spend, which is addressed
    /// to the spent note's own receiver. A real ciphertext there would trial-decrypt under that
    /// receiver's incoming viewing key in the same action that carries the spend's nullifier,
    /// letting anyone who holds the ivk -- including a quantum adversary who recovered it from
    /// the published address -- detect the spend. The note and its commitment are unchanged.
    ///
    /// In a PCZT the output still carries its explicit `recipient`, `value`, and `rseed` with no
    /// `user_address`, so a signer fails to recover anything from the ciphertext, reads the zero
    /// value, and classifies it as a dummy output it tolerates -- rather than reconstructing the
    /// expected ciphertext and rejecting the mismatch.
    randomized_ciphertext: bool,
}

impl OutputInfo {
    /// Constructs a new OutputInfo from its constituent parts.
    pub fn new(
        ovk: Option<OutgoingViewingKey>,
        recipient: Address,
        value: NoteValue,
        note_version: NoteVersion,
        memo: [u8; 512],
    ) -> Self {
        Self {
            ovk,
            recipient,
            value,
            memo,
            note_version,
            randomized_ciphertext: false,
        }
    }

    /// Constructs the zero-valued output that a builder with cross-address actions
    /// disabled pairs with a real spend.
    ///
    /// `recipient` is the spent note's own receiver. For external-scope spends, the output's
    /// `enc_ciphertext` is randomized rather than encrypted to it (see the
    /// `randomized_ciphertext` field). Internal-scope spends do not have the same external-address
    /// exposure, so the deterministic ciphertext remains decryptable.
    fn fabricated_for_spend(
        note_version: NoteVersion,
        recipient: Address,
        spent_scope: Scope,
    ) -> Self {
        Self {
            ovk: None,
            recipient,
            value: NoteValue::ZERO,
            memo: [0u8; 512],
            note_version,
            randomized_ciphertext: matches!(spent_scope, Scope::External),
        }
    }

    /// Defined in [Zcash Protocol Spec § 4.8.3: Dummy Notes (Orchard)][orcharddummynotes].
    ///
    /// [orcharddummynotes]: https://zips.z.cash/protocol/nu5.pdf#orcharddummynotes
    pub fn dummy(note_version: NoteVersion, rng: &mut impl RngCore) -> Self {
        let fvk: FullViewingKey = (&SpendingKey::random(rng)).into();
        let recipient = fvk.address_at(0u32, Scope::External);

        Self::new(None, recipient, NoteValue::ZERO, note_version, [0u8; 512])
    }

    /// Builds the output half of an action.
    ///
    /// Defined in [Zcash Protocol Spec § 4.7.3: Sending Notes (Orchard)][orchardsend].
    ///
    /// [orchardsend]: https://zips.z.cash/protocol/nu5.pdf#orchardsend
    fn build(
        &self,
        cv_net: &ValueCommitment,
        nf_old: Nullifier,
        mut rng: impl RngCore,
    ) -> (Note, ExtractedNoteCommitment, TransmittedNoteCiphertext) {
        let rho = Rho::from_nf_old(nf_old);
        let note = Note::new(self.recipient, self.value, rho, self.note_version, &mut rng);
        let cm_new = note.commitment();
        let cmx = cm_new.into();

        // The Orchard and Ironwood encryptor aliases share encryption behavior;
        // `Note::version()` selects the note plaintext lead byte.
        let encryptor = OrchardNoteEncryption::new(self.ovk.clone(), note, self.memo);

        // `encryptor` still supplies a valid non-identity `epk` and, because these outputs use
        // `ovk = None`, a random `out_ciphertext`. Only `enc_ciphertext` is replaced.
        let enc_ciphertext = if self.randomized_ciphertext {
            assert_eq!(
                self.value,
                NoteValue::ZERO,
                "a randomized note ciphertext must never stand in for a nonzero-valued note",
            );
            let mut enc_ciphertext = [0u8; ENC_CIPHERTEXT_SIZE];
            rng.fill_bytes(&mut enc_ciphertext);
            enc_ciphertext
        } else {
            encryptor.encrypt_note_plaintext()
        };

        let encrypted_note = TransmittedNoteCiphertext {
            epk_bytes: encryptor.epk().to_bytes().0,
            enc_ciphertext,
            out_ciphertext: encryptor.encrypt_outgoing_plaintext(cv_net, &cmx, &mut rng),
        };

        (note, cmx, encrypted_note)
    }

    fn into_pczt(
        self,
        cv_net: &ValueCommitment,
        nf_old: Nullifier,
        rng: impl RngCore,
    ) -> crate::pczt::Output {
        let (note, cmx, encrypted_note) = self.build(cv_net, nf_old, rng);

        crate::pczt::Output {
            cmx,
            note_version: self.note_version,
            encrypted_note,
            recipient: Some(self.recipient),
            value: Some(self.value),
            rseed: Some(*note.rseed()),
            // TODO: Extract ock from the encryptor and save it so
            // Signers can check `out_ciphertext`.
            ock: None,
            zip32_derivation: None,
            user_address: None,
            proprietary: BTreeMap::new(),
        }
    }
}

/// Information about a wallet-controlled change output.
///
/// This is an [`OutputInfo`] to a `recipient` owned by `fvk`, with that ownership recorded.
/// In a bundle that disables cross-address transfers it is the only way to retain shielded
/// value: the builder pairs it with a fabricated zero-valued spend controlled by `fvk` at
/// `recipient`, in the same action. In a bundle that permits cross-address transfers it is
/// equivalent to the underlying [`OutputInfo`] (the ownership is validated when the
/// `ChangeInfo` is constructed, then plays no further role).
#[derive(Debug)]
pub struct ChangeInfo {
    output: OutputInfo,
    fvk: FullViewingKey,
    scope: Scope,
}

impl ChangeInfo {
    /// Constructs a wallet-controlled change output to `recipient`, owned by `fvk`.
    ///
    /// Returns [`OutputError::RecipientNotOwned`] if `fvk` does not own `recipient`. The
    /// recorded `fvk`/scope are used only to fabricate the paired same-expanded-receiver spend in a
    /// bundle that disables cross-address transfers; they do not affect the output's note,
    /// commitment, or ciphertext.
    pub fn new(
        fvk: FullViewingKey,
        ovk: Option<OutgoingViewingKey>,
        recipient: Address,
        value: NoteValue,
        note_version: NoteVersion,
        memo: [u8; 512],
    ) -> Result<Self, OutputError> {
        let scope = fvk
            .scope_for_address(&recipient)
            .ok_or(OutputError::RecipientNotOwned)?;
        Ok(Self {
            output: OutputInfo::new(ovk, recipient, value, note_version, memo),
            fvk,
            scope,
        })
    }

    /// Discards the wallet-ownership information, yielding the underlying plain output.
    ///
    /// Used on the build path for bundles that permit cross-address transfers, where the
    /// ownership has already served its purpose (validation at construction).
    fn into_output(self) -> OutputInfo {
        self.output
    }
}

/// Information about a specific [`Action`] we plan to build.
#[derive(Debug)]
struct ActionInfo {
    spend: SpendInfo,
    output: OutputInfo,
    rcv: ValueCommitTrapdoor,
}

impl ActionInfo {
    fn new(spend: SpendInfo, output: OutputInfo, rng: impl RngCore) -> Self {
        ActionInfo {
            spend,
            output,
            rcv: ValueCommitTrapdoor::random(rng),
        }
    }

    /// Returns the value sum for this action.
    fn value_sum(&self) -> ValueSum {
        self.spend.note.value() - self.output.value
    }

    /// Builds the action for a given circuit version.
    ///
    /// Defined in [Zcash Protocol Spec § 4.7.3: Sending Notes (Orchard)][orchardsend].
    ///
    /// The circuit version must be consistent between actions in a bundle.
    ///
    /// [orchardsend]: https://zips.z.cash/protocol/nu5.pdf#orchardsend
    #[cfg(feature = "circuit")]
    fn build(
        self,
        mut rng: impl RngCore,
        circuit_version: OrchardCircuitVersion,
    ) -> (Action<SigningMetadata>, Circuit) {
        let v_net = self.value_sum();
        let cv_net = ValueCommitment::derive(v_net, self.rcv.clone());

        let (nf_old, ak, alpha, rk) = self.spend.build(&mut rng);
        let (note, cmx, encrypted_note) = self.output.build(&cv_net, nf_old, &mut rng);

        (
            Action::from_parts(
                nf_old,
                rk,
                cmx,
                encrypted_note,
                cv_net,
                SigningMetadata {
                    dummy_ask: self.spend.dummy_sk.as_ref().map(SpendAuthorizingKey::from),
                    parts: SigningParts { ak, alpha },
                },
            )
            .expect(
                "rk is non-identity (α was generated randomly) and epk is a \
                 valid non-identity point by construction",
            ),
            Circuit::from_action_context_unchecked(
                self.spend,
                note,
                alpha,
                self.rcv,
                circuit_version,
            ),
        )
    }

    fn build_for_pczt(self, mut rng: impl RngCore) -> crate::pczt::Action {
        let v_net = self.value_sum();
        let cv_net = ValueCommitment::derive(v_net, self.rcv.clone());

        let spend = self.spend.into_pczt(&mut rng);
        let output = self.output.into_pczt(&cv_net, spend.nullifier, &mut rng);

        crate::pczt::Action {
            cv_net,
            spend,
            output,
            rcv: Some(self.rcv),
        }
    }
}

/// Type alias for an in-progress bundle that has no proofs or signatures.
///
/// This is returned by [`Builder::build`].
#[cfg(feature = "circuit")]
pub type UnauthorizedBundle<V> = Bundle<InProgress<Unproven, Unauthorized>, V>;

/// Metadata about a bundle created by [`bundle`] or [`Builder::build`] that is not necessarily
/// recoverable from the bundle itself.
///
/// This includes information about how [`Action`]s within the bundle are ordered (after
/// padding and randomization) relative to the order in which spends and outputs were provided
/// (to [`bundle`]), or the order in which [`Builder`] mutations were performed.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BundleMetadata {
    spend_indices: Vec<usize>,
    output_indices: Vec<usize>,
}

impl BundleMetadata {
    fn new(num_requested_spends: usize, num_requested_outputs: usize) -> Self {
        BundleMetadata {
            spend_indices: vec![0; num_requested_spends],
            output_indices: vec![0; num_requested_outputs],
        }
    }

    /// Returns the metadata for a [`Bundle`] that contains only dummy actions, if any.
    pub fn empty() -> Self {
        Self::new(0, 0)
    }

    /// Returns the index within the bundle of the [`Action`] corresponding to the `n`-th
    /// spend specified in bundle construction. If a [`Builder`] was used, this refers to
    /// the spend added by the `n`-th call to [`Builder::add_spend`].
    ///
    /// For the purpose of improving indistinguishability, actions are padded and note
    /// positions are randomized when building bundles. This means that the bundle
    /// consumer cannot assume that e.g. the first spend they added corresponds to the
    /// first action in the bundle. In a bundle that disables cross-address transfers,
    /// each spend's action contains a fabricated zero-valued output (to the spent note's
    /// own address), so no requested output shares the returned action index.
    pub fn spend_action_index(&self, n: usize) -> Option<usize> {
        self.spend_indices.get(n).copied()
    }

    /// Returns the index within the bundle of the [`Action`] corresponding to the `n`-th
    /// requested output. Requested outputs are numbered as:
    /// * all plain outputs (in the order they were added with [`Builder::add_output`], or their
    ///   order in the `outputs` vector passed to [`bundle`]);
    /// * followed by all wallet-controlled change outputs (in the order they were added with
    ///   [`Builder::add_change_output`], or their order in the `changes` vector passed to [`bundle`]).
    ///
    /// For the purpose of improving indistinguishability, actions are padded and note
    /// positions are randomized when building bundles. This means that the bundle
    /// consumer cannot assume that e.g. the first output they added corresponds to the
    /// first action in the bundle. In a bundle that disables cross-address transfers,
    /// each output's action contains a fabricated wallet-controlled zero-valued spend (at
    /// the change address), so no requested spend shares the returned action index.
    pub fn output_action_index(&self, n: usize) -> Option<usize> {
        self.output_indices.get(n).copied()
    }
}

/// A builder that constructs a [`Bundle`] from a set of notes to be spent, and outputs
/// to receive funds.
#[derive(Debug)]
pub struct Builder {
    bundle_type: BundleType,
    bundle_version: BundleVersion,
    flags: Flags,
    spends: Vec<SpendInfo>,
    outputs: Vec<OutputInfo>,
    changes: Vec<ChangeInfo>,
    anchor: Anchor,
}

impl Builder {
    /// Constructs a new empty builder for an Orchard bundle following `bundle_version` with the
    /// given `flags`.
    ///
    /// `bundle_version` is the [`ValuePool`](crate::ValuePool) (Orchard or Ironwood) and
    /// [`ProtocolVersion`](crate::ProtocolVersion) of the bundles created by this builder. It
    /// influences the circuit version, the flag-byte format, and the cross-address policy, and is
    /// threaded into building, committing, and parsing. See [`BundleVersion`].
    ///
    /// `flags` are the bundle's flags; [`BundleVersion::default_flags`] provides a suitable default
    /// that a caller may restrict further.
    ///
    /// # Errors
    ///
    /// Returns [`BuildError::UnrepresentableFlags`] if `flags` cannot be encoded under
    /// `bundle_version`, or [`BuildError::CoinbaseSpendsEnabled`] if `bundle_type` is
    /// [`BundleType::Coinbase`] but `flags` enable spends.
    pub fn new(
        bundle_type: BundleType,
        bundle_version: BundleVersion,
        flags: Flags,
        anchor: Anchor,
    ) -> Result<Self, BuildError> {
        if flags.to_byte(bundle_version).is_none() {
            return Err(BuildError::UnrepresentableFlags);
        }
        if matches!(bundle_type, BundleType::Coinbase) && flags.spends_enabled() {
            return Err(BuildError::CoinbaseSpendsEnabled);
        }
        Ok(Builder {
            bundle_type,
            bundle_version,
            flags,
            spends: vec![],
            outputs: vec![],
            changes: vec![],
            anchor,
        })
    }

    /// Returns the bundle type this builder was constructed with.
    ///
    /// Lets callers that must predict the builder's action count (e.g. fee
    /// calculation) read back the exact [`BundleType`] the bundle will be built
    /// with, so the two cannot drift.
    pub fn bundle_type(&self) -> BundleType {
        self.bundle_type
    }

    /// Returns the note version associated with this builder's bundle version.
    fn note_version(&self) -> NoteVersion {
        self.bundle_version.note_version()
    }

    /// Adds a note to be spent in this transaction.
    ///
    /// - `note` is a spendable note, obtained by trial-decrypting an [`Action`]
    ///   under the bundle's version.
    /// - `merkle_path` can be obtained using the [`incrementalmerkletree`] crate
    ///   instantiated with [`MerkleHashOrchard`].
    ///
    /// Returns an error if the given Merkle path does not have the required anchor for
    /// the given note.
    ///
    /// In a bundle that disables cross-address transfers, each spend is paired with a
    /// fabricated zero-valued output addressed to the spent note's own receiver. For
    /// external-scope spends, that output's note ciphertext is randomized rather than encrypted
    /// to the receiver, so it is undecryptable: the owning wallet does not see it when scanning,
    /// and no holder of the receiver's incoming viewing key -- including a quantum adversary who
    /// recovered it from the published address -- can use it to detect the spend. Internal-scope
    /// spends do not need this randomization.
    ///
    /// [`MerkleHashOrchard`]: crate::tree::MerkleHashOrchard
    pub fn add_spend(
        &mut self,
        fvk: FullViewingKey,
        note: Note,
        merkle_path: MerklePath,
    ) -> Result<(), SpendError> {
        if !self.flags.spends_enabled() {
            return Err(SpendError::SpendsDisabled);
        }

        let spend = SpendInfo::new(fvk, note, merkle_path).ok_or(SpendError::FvkMismatch)?;

        // Consistency check: all anchors must be equal.
        if !spend.has_matching_anchor(&self.anchor) {
            return Err(SpendError::AnchorMismatch);
        }

        self.spends.push(spend);

        Ok(())
    }

    /// Adds an address which will receive funds in this transaction.
    ///
    /// In a bundle that disables cross-address transfers, ordinary outputs cannot be
    /// constructed (each action's output is addressed to the note it spends); retained
    /// value must be added with [`Builder::add_change_output`] instead.
    pub fn add_output(
        &mut self,
        ovk: Option<OutgoingViewingKey>,
        recipient: Address,
        value: NoteValue,
        memo: [u8; 512],
    ) -> Result<(), OutputError> {
        if !self.flags.outputs_enabled() {
            return Err(OutputError::OutputsDisabled);
        }
        if !self.flags.cross_address_enabled() {
            return Err(OutputError::CrossAddressDisabled);
        }

        self.outputs.push(OutputInfo::new(
            ovk,
            recipient,
            value,
            self.note_version(),
            memo,
        ));

        Ok(())
    }

    /// Adds a wallet-controlled change output, to an address owned by `fvk`.
    ///
    /// This is the only way to retain shielded value in a bundle that disables
    /// cross-address transfers: the builder pairs the change output with a fabricated
    /// zero-valued spend at `recipient`, controlled by `fvk`, in the same action.
    /// (Withdrawals leave such a bundle through its positive value balance; its real
    /// spends are each paired with a fabricated zero-valued output to the spent note's
    /// own address.) The fabricated spend's authorization is produced by the normal
    /// signing flow -- [`Bundle::apply_signatures`] with the [`SpendAuthorizingKey`]
    /// matching `fvk` -- exactly like the bundle's real spends.
    ///
    /// This may also be used in bundles that permit cross-address transfers, where it
    /// behaves like [`Builder::add_output`] plus an ownership check, so wallet change
    /// logic can be uniform across bundle kinds.
    ///
    /// Note that the builder does not constrain the sign of the bundle's value balance:
    /// a bundle that disables cross-address transfers can still have a negative balance
    /// (value entering the Orchard pool from the rest of the transaction). Whether such
    /// bundles are acceptable is a transaction-level concern outside this crate.
    ///
    /// Returns an error if outputs are disabled for this builder's bundle type, if the
    /// bundle disables cross-address transfers but has spends disabled (the paired
    /// fabricated spend could not be created), or if `fvk` does not own `recipient`.
    pub fn add_change_output(
        &mut self,
        fvk: FullViewingKey,
        ovk: Option<OutgoingViewingKey>,
        recipient: Address,
        value: NoteValue,
        memo: [u8; 512],
    ) -> Result<(), OutputError> {
        if !self.flags.outputs_enabled() {
            return Err(OutputError::OutputsDisabled);
        }
        // In a bundle that disables cross-address transfers, every change output pairs with
        // a fabricated wallet-controlled spend, so spends must be enabled. (In a bundle that
        // permits cross-address transfers, a change output is just an owned output and does
        // not require spends.)
        if !self.flags.cross_address_enabled() && !self.flags.spends_enabled() {
            return Err(OutputError::SpendsDisabled);
        }

        let change = ChangeInfo::new(fvk, ovk, recipient, value, self.note_version(), memo)?;
        self.changes.push(change);

        Ok(())
    }

    /// Returns the action spend components that will be produced by the
    /// transaction being constructed
    pub fn spends(&self) -> &Vec<impl InputView<()>> {
        &self.spends
    }

    /// Returns the action output components that will be produced by the
    /// transaction being constructed
    pub fn outputs(&self) -> &Vec<impl OutputView> {
        &self.outputs
    }

    /// Returns the wallet-controlled change outputs that will be produced by the
    /// transaction being constructed.
    pub fn changes(&self) -> &Vec<impl OutputView> {
        &self.changes
    }

    /// The net value of the bundle to be built. The value of all spends,
    /// minus the value of all outputs.
    ///
    /// Useful for balancing a transaction, as the value balance of an individual bundle
    /// can be non-zero. Each bundle's value balance is [added] to the transparent
    /// transaction value pool, which [must not have a negative value]. (If it were
    /// negative, the transaction would output more value than it receives in inputs.)
    ///
    /// [added]: https://zips.z.cash/protocol/protocol.pdf#orchardbalance
    /// [must not have a negative value]: https://zips.z.cash/protocol/protocol.pdf#transactions
    pub fn value_balance<V: TryFrom<i64>>(&self) -> Result<V, value::BalanceError> {
        let value_balance = self
            .spends
            .iter()
            .map(|spend| spend.note.value() - NoteValue::ZERO)
            .chain(
                self.outputs
                    .iter()
                    .map(|output| NoteValue::ZERO - output.value),
            )
            .chain(
                self.changes
                    .iter()
                    .map(|change| NoteValue::ZERO - change.output.value),
            )
            .try_fold(ValueSum::zero(), |acc, note_value| acc + note_value)
            .ok_or(BalanceError::Overflow)?;
        i64::try_from(value_balance)
            .and_then(|i| V::try_from(i).map_err(|_| value::BalanceError::Overflow))
    }

    /// Builds a bundle containing the given spent notes and outputs, under this builder's
    /// [`BundleVersion`].
    ///
    /// The returned bundle will have no proof or signatures; these can be applied with
    /// [`Bundle::create_proof`] and [`Bundle::apply_signatures`] respectively. The proof must be
    /// created with a [`ProvingKey`] for the circuit version consistent with the builder's
    /// bundle version.
    #[cfg(feature = "circuit")]
    pub fn build<V: TryFrom<i64>>(
        self,
        rng: impl RngCore,
    ) -> Result<Option<(UnauthorizedBundle<V>, BundleMetadata)>, BuildError> {
        bundle(
            rng,
            self.bundle_type,
            self.bundle_version,
            self.flags,
            self.anchor,
            self.spends,
            self.outputs,
            self.changes,
        )
    }

    /// Builds a bundle containing the given spent notes and outputs along with their
    /// metadata, for inclusion in a PCZT.
    pub fn build_for_pczt(
        self,
        rng: impl RngCore,
    ) -> Result<(crate::pczt::Bundle, BundleMetadata), BuildError> {
        build_bundle(
            rng,
            self.bundle_version,
            self.flags,
            self.anchor,
            self.bundle_type,
            self.spends,
            self.outputs,
            self.changes,
            |pre_actions, flags, value_sum, bundle_meta, mut rng| {
                // Create the actions.
                let actions = pre_actions
                    .into_iter()
                    .map(|a| a.build_for_pczt(&mut rng))
                    .collect::<Vec<_>>();

                Ok((
                    crate::pczt::Bundle {
                        actions,
                        flags,
                        bundle_version: self.bundle_version,
                        value_sum,
                        anchor: self.anchor,
                        zkproof: None,
                        bsk: None,
                    },
                    bundle_meta,
                ))
            },
        )
    }
}

/// Builds a bundle containing the given spent notes, outputs, and wallet-controlled change
/// outputs, under the given [`BundleVersion`] (which selects the Action circuit
/// version, the flag-byte format, and the cross-address policy).
///
/// In a bundle that disables cross-address transfers, `outputs` must be empty (every output
/// is addressed to the note it spends); retained value must be supplied as `changes`.
///
/// # Errors
///
/// Returns [`BuildError::UnrepresentableFlags`] if `flags` cannot be encoded under
/// `bundle_version`, or [`BuildError::CoinbaseSpendsEnabled`] if `bundle_type` is
/// [`BundleType::Coinbase`] but `flags` enable spends.
#[allow(clippy::too_many_arguments)]
#[cfg(feature = "circuit")]
pub fn bundle<V: TryFrom<i64>>(
    rng: impl RngCore,
    bundle_type: BundleType,
    bundle_version: BundleVersion,
    flags: Flags,
    anchor: Anchor,
    spends: Vec<SpendInfo>,
    outputs: Vec<OutputInfo>,
    changes: Vec<ChangeInfo>,
) -> Result<Option<(UnauthorizedBundle<V>, BundleMetadata)>, BuildError> {
    build_bundle(
        rng,
        bundle_version,
        flags,
        anchor,
        bundle_type,
        spends,
        outputs,
        changes,
        |pre_actions, flags, value_balance, bundle_meta, rng| {
            finish_unauthorized_bundle(
                pre_actions,
                flags,
                value_balance,
                bundle_meta,
                rng,
                anchor,
                bundle_version,
            )
        },
    )
}

#[cfg(feature = "circuit")]
fn finish_unauthorized_bundle<V: TryFrom<i64>, R: RngCore>(
    pre_actions: Vec<ActionInfo>,
    flags: Flags,
    value_balance: ValueSum,
    bundle_meta: BundleMetadata,
    mut rng: R,
    anchor: Anchor,
    bundle_version: BundleVersion,
) -> Result<Option<(UnauthorizedBundle<V>, BundleMetadata)>, BuildError> {
    let circuit_version = bundle_version.circuit_version();
    let result_value_balance: V = i64::try_from(value_balance)
        .map_err(BuildError::ValueSum)
        .and_then(|i| {
            V::try_from(i).map_err(|_| BuildError::ValueSum(value::BalanceError::Overflow))
        })?;

    // Compute the transaction binding signing key.
    let bsk = pre_actions
        .iter()
        .map(|a| &a.rcv)
        .sum::<ValueCommitTrapdoor>()
        .into_bsk();

    // Create the actions.
    let (actions, circuits): (Vec<_>, Vec<_>) = pre_actions
        .into_iter()
        .map(|a| a.build(&mut rng, circuit_version))
        .unzip();

    // Verify that bsk and bvk are consistent.
    let bvk = (actions.iter().map(|a| a.cv_net()).sum::<ValueCommitment>()
        - ValueCommitment::derive(value_balance, ValueCommitTrapdoor::zero()))
    .into_bvk();
    assert_eq!(redpallas::VerificationKey::from(&bsk), bvk);

    Ok(NonEmpty::from_vec(actions).map(|actions| {
        (
            Bundle::from_parts_unchecked(
                actions,
                flags,
                result_value_balance,
                anchor,
                InProgress {
                    proof: Unproven {
                        circuits,
                        circuit_version,
                    },
                    sigs: Unauthorized { bsk },
                },
                bundle_version,
            ),
            bundle_meta,
        )
    }))
}

#[allow(clippy::too_many_arguments)]
fn build_bundle<B, R: RngCore>(
    mut rng: R,
    bundle_version: BundleVersion,
    flags: Flags,
    anchor: Anchor,
    bundle_type: BundleType,
    spends: Vec<SpendInfo>,
    outputs: Vec<OutputInfo>,
    changes: Vec<ChangeInfo>,
    finisher: impl FnOnce(Vec<ActionInfo>, Flags, ValueSum, BundleMetadata, R) -> Result<B, BuildError>,
) -> Result<B, BuildError> {
    // Every build path funnels through here (the free `bundle` function, `Builder::build`, and
    // `Builder::build_for_pczt`), so validate the version-dependent invariants here rather than
    // trusting each caller: the flags must be encodable under the bundle version, and a coinbase
    // bundle must not enable spends. `Builder::new` also enforces both up front, for fail-fast
    // construction.
    if flags.to_byte(bundle_version).is_none() {
        return Err(BuildError::UnrepresentableFlags);
    }
    if matches!(bundle_type, BundleType::Coinbase) && flags.spends_enabled() {
        return Err(BuildError::CoinbaseSpendsEnabled);
    }
    let note_version = bundle_version.note_version();

    let num_requested_spends = spends.len();
    if !flags.spends_enabled() && num_requested_spends > 0 {
        return Err(BuildError::SpendsDisabled);
    }

    for spend in &spends {
        if !spend.has_matching_anchor(&anchor) {
            return Err(BuildError::AnchorMismatch);
        }
    }

    // Requested outputs are numbered for `BundleMetadata` as the plain `outputs` in order,
    // followed by the wallet-controlled `changes` in order.
    let num_plain_outputs = outputs.len();
    let num_requested_outputs = num_plain_outputs + changes.len();
    if !flags.outputs_enabled() && num_requested_outputs > 0 {
        return Err(BuildError::OutputsDisabled);
    }
    if outputs
        .iter()
        .any(|output| output.note_version != note_version)
        || changes
            .iter()
            .any(|change| change.output.note_version != note_version)
    {
        return Err(BuildError::InvalidNoteVersion);
    }
    if !flags.spends_enabled() && !flags.cross_address_enabled() && num_requested_outputs > 0 {
        return Err(BuildError::BundleTypeNotSatisfiable);
    }
    // When cross-address transfers are disabled, every action's output is addressed to the
    // note it spends, so a plain output cannot be built; retained value must arrive as a
    // wallet-controlled change output (which carries the owning fvk needed to fabricate the
    // paired same-expanded-receiver spend).
    if !flags.cross_address_enabled() && num_plain_outputs > 0 {
        return Err(BuildError::CrossAddressDisabled);
    }

    let num_actions = bundle_type
        .num_actions(flags, num_requested_spends, num_requested_outputs)
        .map_err(|_| BuildError::BundleTypeNotSatisfiable)?;

    let (pre_actions, bundle_meta) = if !flags.cross_address_enabled() {
        // Every action's output must be addressed to the note it spends, so the
        // spend/output pairing within each action is intentional:
        //
        // - each requested spend is paired with a fabricated zero-valued output to the
        //   spent note's own address;
        // - each requested change output is paired with a fabricated zero-valued spend
        //   controlled by the wallet at the change address, because withdrawn value leaves
        //   the bundle through its value balance, and retained value is exactly the
        //   wallet's change;
        // - padding actions pair a dummy spend with a zero-valued output to the dummy's
        //   own address, since the cross-address checks apply to dummy actions too.
        //
        // `outputs` is empty here (a plain output was rejected above), so the only requested
        // outputs are the changes, numbered `0..changes.len()`.
        //
        // Only complete pairs are shuffled.
        let mut pairs = Vec::with_capacity(num_actions);

        for (spend_idx, spend) in spends.into_iter().enumerate() {
            let output =
                OutputInfo::fabricated_for_spend(note_version, spend.note.recipient(), spend.scope);
            pairs.push((Some(spend_idx), None, spend, output));
        }

        for (chg_idx, change) in changes.into_iter().enumerate() {
            let ChangeInfo { output, fvk, scope } = change;
            let rho = Rho::from_nf_old(Nullifier::dummy(&mut rng));
            let note = Note::new(
                output.recipient,
                NoteValue::ZERO,
                rho,
                note_version,
                &mut rng,
            );
            let spend = SpendInfo {
                // The wallet controls this spend: it is signed through the normal
                // signing flow, by the spend authorizing key matching `fvk`.
                dummy_sk: None,
                fvk,
                scope,
                note,
                merkle_path: MerklePath::dummy(&mut rng),
            };
            pairs.push((None, Some(chg_idx), spend, output));
        }

        while pairs.len() < num_actions {
            let spend = SpendInfo::dummy(note_version, &mut rng);
            let output = OutputInfo::new(
                None,
                spend.note.recipient(),
                NoteValue::ZERO,
                note_version,
                [0u8; 512],
            );
            pairs.push((None, None, spend, output));
        }

        // Shuffle the action pairs, so that learning the position of a specific action
        // doesn't reveal anything on its own about its meaning in the transaction
        // context. The spend/output pairing inside each action is intentional.
        pairs.shuffle(&mut rng);

        let mut bundle_meta = BundleMetadata::new(num_requested_spends, num_requested_outputs);
        let pre_actions = pairs
            .into_iter()
            .enumerate()
            .map(|(action_idx, (spend_idx, out_idx, spend, output))| {
                // Record the post-randomization spend location
                if let Some(spend_idx) = spend_idx {
                    bundle_meta.spend_indices[spend_idx] = action_idx;
                }

                // Record the post-randomization output location
                if let Some(out_idx) = out_idx {
                    bundle_meta.output_indices[out_idx] = action_idx;
                }

                assert!(
                    spend
                        .note
                        .recipient()
                        .same_expanded_receiver(&output.recipient),
                    "cross-address-disabled actions pair a spend with an output to the \
                     same expanded receiver by construction",
                );

                ActionInfo::new(spend, output, &mut rng)
            })
            .collect::<Vec<_>>();

        (pre_actions, bundle_meta)
    } else {
        // Pair up the spends and outputs, extending with dummy values as necessary.
        let mut indexed_spends = spends
            .into_iter()
            .chain(iter::repeat_with(|| {
                SpendInfo::dummy(note_version, &mut rng)
            }))
            .enumerate()
            .take(num_actions)
            .collect::<Vec<_>>();

        // Plain outputs first, then change outputs collapsed to plain outputs (their
        // ownership was validated when each `ChangeInfo` was constructed and plays no
        // further role when cross-address transfers are permitted). This ordering matches
        // the `BundleMetadata` output numbering.
        let mut indexed_outputs = outputs
            .into_iter()
            .chain(changes.into_iter().map(ChangeInfo::into_output))
            .chain(iter::repeat_with(|| {
                OutputInfo::dummy(note_version, &mut rng)
            }))
            .enumerate()
            .take(num_actions)
            .collect::<Vec<_>>();

        // Shuffle the spends and outputs, so that learning the position of a
        // specific spent note or output note doesn't reveal anything on its own about
        // the meaning of that note in the transaction context.
        indexed_spends.shuffle(&mut rng);
        indexed_outputs.shuffle(&mut rng);

        let mut bundle_meta = BundleMetadata::new(num_requested_spends, num_requested_outputs);
        let pre_actions = indexed_spends
            .into_iter()
            .zip(indexed_outputs)
            .enumerate()
            .map(|(action_idx, ((spend_idx, spend), (out_idx, output)))| {
                // Record the post-randomization spend location
                if spend_idx < num_requested_spends {
                    bundle_meta.spend_indices[spend_idx] = action_idx;
                }

                // Record the post-randomization output location
                if out_idx < num_requested_outputs {
                    bundle_meta.output_indices[out_idx] = action_idx;
                }

                ActionInfo::new(spend, output, &mut rng)
            })
            .collect::<Vec<_>>();

        (pre_actions, bundle_meta)
    };

    // Determine the value balance for this bundle, ensuring it is valid.
    let value_balance = pre_actions
        .iter()
        .try_fold(ValueSum::zero(), |acc, action| acc + action.value_sum())
        .ok_or(BalanceError::Overflow)?;

    finisher(pre_actions, flags, value_balance, bundle_meta, rng)
}

/// Marker trait representing bundle signatures in the process of being created.
pub trait InProgressSignatures: fmt::Debug {
    /// The authorization type of an Orchard action in the process of being authorized.
    type SpendAuth: fmt::Debug;
}

/// Marker for a bundle in the process of being built.
#[derive(Clone, Debug)]
pub struct InProgress<P, S: InProgressSignatures> {
    proof: P,
    sigs: S,
}

impl<P: fmt::Debug, S: InProgressSignatures> Authorization for InProgress<P, S> {
    type SpendAuth = S::SpendAuth;
}

/// Marker for a bundle without a proof.
///
/// This struct contains the private data needed to create a [`Proof`] for a [`Bundle`].
#[cfg(feature = "circuit")]
#[derive(Clone, Debug)]
pub struct Unproven {
    circuits: Vec<Circuit>,
    circuit_version: OrchardCircuitVersion,
}

#[cfg(feature = "circuit")]
impl<S: InProgressSignatures> InProgress<Unproven, S> {
    /// Creates the proof for this bundle.
    ///
    /// # Errors
    ///
    /// Returns [`halo2_proofs::plonk::Error::InvalidInstances`] if any provided
    /// instance has `disableCrossAddress = 1` and `pk` is not an
    /// [`OrchardCircuitVersion::PostNu6_3`] proving key.
    ///
    /// Also returns an error if `pk` does not match the circuit version this
    /// bundle's actions were built for, or if proof creation fails.
    pub fn create_proof(
        &self,
        pk: &ProvingKey,
        instances: &[Instance],
        rng: impl RngCore,
    ) -> Result<Proof, halo2_proofs::plonk::Error> {
        Proof::create(pk, &self.proof.circuits, instances, rng)
    }
}

#[cfg(feature = "circuit")]
impl<S: InProgressSignatures, V> Bundle<InProgress<Unproven, S>, V> {
    /// The circuit version this bundle's actions were built for, and that its proof must
    /// therefore be created against (with a matching [`ProvingKey`]).
    pub fn circuit_version(&self) -> OrchardCircuitVersion {
        self.authorization().proof.circuit_version
    }

    /// Creates the proof for this bundle.
    ///
    /// # Errors
    ///
    /// Returns [`BuildError::Proof`] containing
    /// [`halo2_proofs::plonk::Error::InvalidInstances`] if this bundle disables
    /// cross-address transfers and `pk` is not an
    /// [`OrchardCircuitVersion::PostNu6_3`] proving key.
    ///
    /// Also returns an error if `pk` does not match this bundle's
    /// [`circuit_version`](Self::circuit_version), or if proof creation fails.
    pub fn create_proof(
        self,
        pk: &ProvingKey,
        mut rng: impl RngCore,
    ) -> Result<Bundle<InProgress<Proof, S>, V>, BuildError> {
        let instances: Vec<_> = self
            .actions()
            .iter()
            .map(|a| a.to_instance(*self.flags(), *self.anchor()))
            .collect();
        self.try_map_authorization(
            &mut (),
            |_, _, a| Ok(a),
            |_, auth| {
                let proof = auth.create_proof(pk, &instances, &mut rng)?;
                Ok(InProgress {
                    proof,
                    sigs: auth.sigs,
                })
            },
        )
    }
}

/// The parts needed to sign an [`Action`].
#[derive(Clone, Debug)]
pub struct SigningParts {
    /// The spend validating key for this action. Used to match spend authorizing keys to
    /// actions they can create signatures for.
    ak: SpendValidatingKey,
    /// The randomization needed to derive the actual signing key for this note.
    alpha: pallas::Scalar,
}

/// Marker for an unauthorized bundle with no signatures.
#[derive(Clone, Debug)]
pub struct Unauthorized {
    bsk: redpallas::SigningKey<Binding>,
}

impl InProgressSignatures for Unauthorized {
    type SpendAuth = SigningMetadata;
}

/// Container for metadata needed to sign an [`Action`].
#[derive(Clone, Debug)]
pub struct SigningMetadata {
    /// If this action is spending a dummy note, this field holds that note's spend
    /// authorizing key.
    ///
    /// These keys are used automatically in [`Bundle<Unauthorized>::prepare`] or
    /// [`Bundle<Unauthorized>::apply_signatures`] to sign dummy spends.
    dummy_ask: Option<SpendAuthorizingKey>,
    parts: SigningParts,
}

/// Marker for a partially-authorized bundle, in the process of being signed.
#[derive(Debug)]
pub struct PartiallyAuthorized {
    binding_signature: redpallas::Signature<Binding>,
    sighash: [u8; 32],
}

impl InProgressSignatures for PartiallyAuthorized {
    type SpendAuth = MaybeSigned;
}

/// A heisen[`Signature`] for a particular [`Action`].
///
/// [`Signature`]: redpallas::Signature
#[derive(Debug)]
pub enum MaybeSigned {
    /// The information needed to sign this [`Action`].
    SigningMetadata(SigningParts),
    /// The signature for this [`Action`].
    Signature(redpallas::Signature<SpendAuth>),
}

impl MaybeSigned {
    fn finalize(self) -> Result<redpallas::Signature<SpendAuth>, BuildError> {
        match self {
            Self::Signature(sig) => Ok(sig),
            _ => Err(BuildError::MissingSignatures),
        }
    }
}

impl<P: fmt::Debug, V> Bundle<InProgress<P, Unauthorized>, V> {
    /// Loads the sighash into this bundle, preparing it for signing.
    ///
    /// This API ensures that all signatures are created over the same sighash.
    pub fn prepare<R: RngCore + CryptoRng>(
        self,
        mut rng: R,
        sighash: [u8; 32],
    ) -> Bundle<InProgress<P, PartiallyAuthorized>, V> {
        self.map_authorization(
            &mut rng,
            |rng, _, SigningMetadata { dummy_ask, parts }| {
                // We can create signatures for dummy spends immediately.
                dummy_ask
                    .map(|ask| ask.randomize(&parts.alpha).sign(rng, &sighash))
                    .map(MaybeSigned::Signature)
                    .unwrap_or(MaybeSigned::SigningMetadata(parts))
            },
            |rng, auth| InProgress {
                proof: auth.proof,
                sigs: PartiallyAuthorized {
                    binding_signature: auth.sigs.bsk.sign(rng, &sighash),
                    sighash,
                },
            },
        )
    }
}

impl<V> Bundle<InProgress<Proof, Unauthorized>, V> {
    /// Applies signatures to this bundle, in order to authorize it.
    ///
    /// This is a helper method that wraps [`Bundle::prepare`], [`Bundle::sign`], and
    /// [`Bundle::finalize`].
    pub fn apply_signatures<R: RngCore + CryptoRng>(
        self,
        mut rng: R,
        sighash: [u8; 32],
        signing_keys: &[SpendAuthorizingKey],
    ) -> Result<Bundle<Authorized, V>, BuildError> {
        signing_keys
            .iter()
            .fold(self.prepare(&mut rng, sighash), |partial, ask| {
                partial.sign(&mut rng, ask)
            })
            .finalize()
    }
}

impl<P: fmt::Debug, V> Bundle<InProgress<P, PartiallyAuthorized>, V> {
    /// Signs this bundle with the given [`SpendAuthorizingKey`].
    ///
    /// This will apply signatures for all notes controlled by this spending key.
    pub fn sign<R: RngCore + CryptoRng>(self, mut rng: R, ask: &SpendAuthorizingKey) -> Self {
        let expected_ak = ask.into();
        self.map_authorization(
            &mut rng,
            |rng, partial, maybe| match maybe {
                MaybeSigned::SigningMetadata(parts) if parts.ak == expected_ak => {
                    MaybeSigned::Signature(
                        ask.randomize(&parts.alpha).sign(rng, &partial.sigs.sighash),
                    )
                }
                s => s,
            },
            |_, partial| partial,
        )
    }
    /// Appends externally computed [`Signature`]s.
    ///
    /// Each signature will be applied to the one input for which it is valid. An error
    /// will be returned if the signature is not valid for any inputs, or if it is valid
    /// for more than one input.
    ///
    /// [`Signature`]: redpallas::Signature
    pub fn append_signatures(
        self,
        signatures: &[redpallas::Signature<SpendAuth>],
    ) -> Result<Self, BuildError> {
        signatures.iter().try_fold(self, Self::append_signature)
    }

    fn append_signature(
        self,
        signature: &redpallas::Signature<SpendAuth>,
    ) -> Result<Self, BuildError> {
        let mut signature_valid_for = 0usize;
        let bundle = self.map_authorization(
            &mut signature_valid_for,
            |valid_for, partial, maybe| match maybe {
                MaybeSigned::SigningMetadata(parts) => {
                    let rk = parts.ak.randomize(&parts.alpha);
                    if rk.verify(&partial.sigs.sighash[..], signature).is_ok() {
                        *valid_for += 1;
                        MaybeSigned::Signature(signature.clone())
                    } else {
                        // Signature isn't for this input.
                        MaybeSigned::SigningMetadata(parts)
                    }
                }
                s => s,
            },
            |_, partial| partial,
        );
        match signature_valid_for {
            0 => Err(BuildError::InvalidExternalSignature),
            1 => Ok(bundle),
            _ => Err(BuildError::DuplicateSignature),
        }
    }
}

impl<V> Bundle<InProgress<Proof, PartiallyAuthorized>, V> {
    /// Finalizes this bundle, enabling it to be included in a transaction.
    ///
    /// Returns an error if any signatures are missing.
    pub fn finalize(self) -> Result<Bundle<Authorized, V>, BuildError> {
        self.try_map_authorization(
            &mut (),
            |_, _, maybe| maybe.finalize(),
            |_, partial| {
                Ok(Authorized::from_parts(
                    partial.proof,
                    partial.sigs.binding_signature,
                ))
            },
        )
    }
}

/// A trait that provides a minimized view of an Orchard input suitable for use in
/// fee and change calculation.
pub trait InputView<NoteRef> {
    /// An identifier for the input being spent.
    fn note_id(&self) -> &NoteRef;
    /// The value of the input being spent.
    fn value<V: From<u64>>(&self) -> V;
}

impl InputView<()> for SpendInfo {
    fn note_id(&self) -> &() {
        // The builder does not make use of note identifiers, so we can just return the unit value.
        &()
    }

    fn value<V: From<u64>>(&self) -> V {
        V::from(self.note.value().inner())
    }
}

/// A trait that provides a minimized view of an Orchard output suitable for use in
/// fee and change calculation.
pub trait OutputView {
    /// The value of the output being produced.
    fn value<V: From<u64>>(&self) -> V;
}

impl OutputView for OutputInfo {
    fn value<V: From<u64>>(&self) -> V {
        V::from(self.value.inner())
    }
}

impl OutputView for ChangeInfo {
    fn value<V: From<u64>>(&self) -> V {
        self.output.value()
    }
}

/// Generators for property testing.
#[cfg(all(feature = "circuit", any(test, feature = "test-dependencies")))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub mod testing {
    use alloc::vec::Vec;
    use core::fmt::Debug;

    use incrementalmerkletree::{frontier::Frontier, Hashable};
    use rand::{rngs::StdRng, CryptoRng, SeedableRng};

    use proptest::collection::vec;
    use proptest::prelude::*;

    use crate::{
        address::testing::arb_address,
        bundle::{Authorized, Bundle, BundleVersion},
        circuit::{OrchardCircuitVersion, ProvingKey},
        keys::{testing::arb_spending_key, FullViewingKey, SpendAuthorizingKey, SpendingKey},
        note::testing::arb_note,
        tree::{Anchor, MerkleHashOrchard, MerklePath},
        value::{testing::arb_positive_note_value, NoteValue, MAX_NOTE_VALUE},
        Address, Note, NoteVersion,
    };

    use super::{Builder, BundleType};

    /// An intermediate type used for construction of arbitrary
    /// bundle values. This type is required because of a limitation
    /// of the proptest prop_compose! macro which does not correctly
    /// handle polymorphic generator functions. Instead of generating
    /// a bundle directly, we generate the bundle inputs, and then
    /// are able to use the `build` function to construct the bundle
    /// from these inputs, but using a `ValueBalance` implementation that
    /// is defined by the end user.
    #[derive(Debug)]
    struct ArbitraryBundleInputs<R> {
        rng: R,
        sk: SpendingKey,
        anchor: Anchor,
        notes: Vec<(Note, MerklePath)>,
        output_amounts: Vec<(Address, NoteValue)>,
    }

    impl<R: RngCore + CryptoRng> ArbitraryBundleInputs<R> {
        /// Create a bundle from the set of arbitrary bundle inputs.
        fn into_bundle<V: TryFrom<i64>>(mut self) -> Bundle<Authorized, V> {
            let fvk = FullViewingKey::from(&self.sk);
            let bundle_version = BundleVersion::orchard_v2();
            let mut builder = Builder::new(
                BundleType::DEFAULT,
                bundle_version,
                bundle_version.default_flags(),
                self.anchor,
            )
            .unwrap();

            for (note, path) in self.notes.into_iter() {
                builder.add_spend(fvk.clone(), note, path).unwrap();
            }

            for (addr, value) in self.output_amounts.into_iter() {
                let scope = fvk.scope_for_address(&addr).unwrap();
                let ovk = fvk.to_ovk(scope);

                builder
                    .add_output(Some(ovk.clone()), addr, value, [0u8; 512])
                    .unwrap();
            }

            let pk = ProvingKey::build(OrchardCircuitVersion::FixedPostNu6_2);
            builder
                .build(&mut self.rng)
                .unwrap()
                .unwrap()
                .0
                .create_proof(&pk, &mut self.rng)
                .unwrap()
                .prepare(&mut self.rng, [0; 32])
                .sign(&mut self.rng, &SpendAuthorizingKey::from(&self.sk))
                .finalize()
                .unwrap()
        }
    }

    prop_compose! {
        /// Produce a random valid Orchard bundle.
        fn arb_bundle_inputs(sk: SpendingKey)
        (
            n_notes in 1usize..30,
            n_outputs in 1..30,

        )
        (
            // generate note values that we're certain won't exceed MAX_NOTE_VALUE in total
            notes in vec(
                arb_positive_note_value(MAX_NOTE_VALUE / n_notes as u64)
                    .prop_flat_map(|value| arb_note(value, NoteVersion::V2)),
                n_notes
            ),
            output_amounts in vec(
                arb_address().prop_flat_map(move |a| {
                    arb_positive_note_value(MAX_NOTE_VALUE / n_outputs as u64)
                        .prop_map(move |v| (a, v))
                }),
                n_outputs as usize
            ),
            rng_seed in prop::array::uniform32(prop::num::u8::ANY)
        ) -> ArbitraryBundleInputs<StdRng> {
            use crate::constants::MERKLE_DEPTH_ORCHARD;
            let mut frontier = Frontier::<MerkleHashOrchard, { MERKLE_DEPTH_ORCHARD as u8 }>::empty();
            let mut notes_and_auth_paths: Vec<(Note, MerklePath)> = Vec::new();

            for note in notes.iter() {
                let leaf = MerkleHashOrchard::from_cmx(&note.commitment().into());
                frontier.append(leaf);

                let path = frontier
                    .witness(|addr| Some(<MerkleHashOrchard as Hashable>::empty_root(addr.level())))
                    .ok()
                    .flatten()
                    .expect("we can always construct a correct Merkle path");
                notes_and_auth_paths.push((*note, path.into()));
            }

            ArbitraryBundleInputs {
                rng: StdRng::from_seed(rng_seed),
                sk,
                anchor: frontier.root().into(),
                notes: notes_and_auth_paths,
                output_amounts
            }
        }
    }

    /// Produce an arbitrary valid Orchard bundle using a random spending key.
    pub fn arb_bundle<V: TryFrom<i64> + Debug>() -> impl Strategy<Value = Bundle<Authorized, V>> {
        arb_spending_key()
            .prop_flat_map(arb_bundle_inputs)
            .prop_map(|inputs| inputs.into_bundle::<V>())
    }

    /// Produce an arbitrary valid Orchard bundle using a specified spending key.
    pub fn arb_bundle_with_key<V: TryFrom<i64> + Debug>(
        k: SpendingKey,
    ) -> impl Strategy<Value = Bundle<Authorized, V>> {
        arb_bundle_inputs(k).prop_map(|inputs| inputs.into_bundle::<V>())
    }
}

#[cfg(all(test, feature = "circuit"))]
mod tests {
    use rand::rngs::OsRng;
    use rand::RngCore;

    use super::{
        bundle, BuildError, Builder, ChangeInfo, MaybeSigned, OutputError, OutputInfo, SpendInfo,
        DEFAULT_MIN_ACTIONS,
    };
    use crate::{
        builder::BundleType,
        bundle::{Authorized, Bundle, BundleVersion, Flags},
        circuit::{OrchardCircuitVersion, ProvingKey},
        constants::MERKLE_DEPTH_ORCHARD,
        keys::{
            FullViewingKey, PreparedIncomingViewingKey, Scope, SpendAuthorizingKey, SpendingKey,
        },
        note::{NoteVersion, Nullifier, Rho},
        note_encryption::OrchardDomain,
        pczt::{ProverError, VerifyError},
        tree::{MerklePath, EMPTY_ROOTS},
        value::NoteValue,
        Address, Anchor, Note,
    };
    use zcash_note_encryption::try_note_decryption;

    fn note_with_path(
        rng: &mut impl RngCore,
        recipient: Address,
        value: NoteValue,
        note_version: NoteVersion,
    ) -> (Note, MerklePath, Anchor) {
        let rho = Rho::from_nf_old(Nullifier::dummy(rng));
        let note = Note::new(recipient, value, rho, note_version, &mut *rng);
        let merkle_path = MerklePath::dummy(rng);
        let anchor = merkle_path.root(note.commitment().into());

        (note, merkle_path, anchor)
    }

    fn transactional(bundle_required: bool) -> BundleType {
        BundleType::Transactional {
            bundle_required,
            pad_to_minimum: None,
        }
    }

    #[test]
    fn num_actions_respects_pad_to_minimum() {
        let padded = transactional(false);
        let explicitly_padded = BundleType::Transactional {
            bundle_required: false,
            pad_to_minimum: Some(DEFAULT_MIN_ACTIONS),
        };
        let padded_to_three = BundleType::Transactional {
            bundle_required: false,
            pad_to_minimum: Some(3),
        };
        let unpadded = BundleType::Transactional {
            bundle_required: false,
            pad_to_minimum: Some(1),
        };
        let required_no_padding = BundleType::Transactional {
            bundle_required: true,
            pad_to_minimum: Some(0),
        };
        let required_unpadded = BundleType::Transactional {
            bundle_required: true,
            pad_to_minimum: Some(1),
        };

        // Cross-address transfers enabled: requested actions = max(spends, outputs).
        let flags = BundleVersion::ironwood_v3().default_flags();
        assert_eq!(padded.num_actions(flags, 0, 1), Ok(2));
        assert_eq!(explicitly_padded.num_actions(flags, 0, 1), Ok(2));
        assert_eq!(padded_to_three.num_actions(flags, 0, 1), Ok(3));
        assert_eq!(unpadded.num_actions(flags, 0, 1), Ok(1));
        assert_eq!(BundleType::DEFAULT.num_actions(flags, 0, 1), Ok(2));
        assert_eq!(BundleType::UNPADDED.num_actions(flags, 0, 1), Ok(1));
        assert_eq!(unpadded.num_actions(flags, 2, 3), Ok(3));
        assert_eq!(unpadded.num_actions(flags, 0, 0), Ok(0));
        // `bundle_required` still guarantees a bundle exists: a single all-dummy action.
        assert_eq!(required_no_padding.num_actions(flags, 0, 0), Ok(1));
        assert_eq!(required_unpadded.num_actions(flags, 0, 0), Ok(1));

        let flags = BundleVersion::orchard_v2().default_flags();
        assert_eq!(BundleType::DEFAULT.num_actions(flags, 0, 1), Ok(2));
        assert_eq!(BundleType::UNPADDED.num_actions(flags, 0, 1), Ok(1));

        // Cross-address transfers disabled: requested actions = spends + outputs.
        let flags = BundleVersion::orchard_v3().default_flags();
        assert_eq!(unpadded.num_actions(flags, 1, 0), Ok(1));
        assert_eq!(unpadded.num_actions(flags, 1, 1), Ok(2));
    }

    #[test]
    fn default_flags_match_pool_policy() {
        // The builder's default flags enable spends and outputs and leave cross-address transfers
        // as permissive as consensus allows. The bundle version leaves the cross-address choice free
        // everywhere except Orchard from NU6.3 onward, where it is mandatorily disabled.
        for bundle_version in [
            BundleVersion::orchard_insecure_v1(),
            BundleVersion::orchard_v2(),
            BundleVersion::ironwood_v3(),
        ] {
            let flags = bundle_version.default_flags();
            assert!(flags.spends_enabled());
            assert!(flags.outputs_enabled());
            assert!(flags.cross_address_enabled());
        }

        // Orchard from NU6.3 onward mandates the cross-address restriction.
        let flags = BundleVersion::orchard_v3().default_flags();
        assert!(flags.spends_enabled());
        assert!(flags.outputs_enabled());
        assert!(!flags.cross_address_enabled());

        // The default flag bytes follow from the settings above.
        assert_eq!(
            BundleVersion::orchard_v3()
                .default_flags()
                .to_byte(BundleVersion::orchard_v3()),
            Some(0b011),
        );
        assert_eq!(
            BundleVersion::ironwood_v3()
                .default_flags()
                .to_byte(BundleVersion::ironwood_v3()),
            Some(0b111),
        );
    }

    /// Creates a builder with the given `bundle_version` and `bundle_type` over the
    /// empty-tree anchor, with a single 5000-zat output to a freshly derived external address.
    fn output_only_builder(
        rng: &mut impl RngCore,
        bundle_version: BundleVersion,
        bundle_type: BundleType,
    ) -> Builder {
        let sk = SpendingKey::random(rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);

        // Coinbase bundles must disable spends; transactional bundles use the version's defaults.
        let flags = if matches!(bundle_type, BundleType::Coinbase) {
            Flags::from_parts(
                false,
                true,
                bundle_version.permits_cross_address_transfers(),
            )
        } else {
            bundle_version.default_flags()
        };

        let mut builder = Builder::new(
            bundle_type,
            bundle_version,
            flags,
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
        )
        .expect("flags are valid for the bundle version");
        builder
            .add_output(None, recipient, NoteValue::from_raw(5000), [0u8; 512])
            .expect("output-only builders accept ordinary outputs");
        builder
    }

    #[test]
    fn shielding_bundle() {
        let pk = ProvingKey::build(OrchardCircuitVersion::FixedPostNu6_2);
        let mut rng = OsRng;

        let builder =
            output_only_builder(&mut rng, BundleVersion::orchard_v2(), BundleType::DEFAULT);
        let balance: i64 = builder.value_balance().unwrap();
        assert_eq!(balance, -5000);

        let bundle: Bundle<Authorized, i64> = builder
            .build(&mut rng)
            .unwrap()
            .unwrap()
            .0
            .create_proof(&pk, &mut rng)
            .unwrap()
            .prepare(rng, [0; 32])
            .finalize()
            .unwrap();
        assert_eq!(bundle.value_balance(), &(-5000))
    }

    #[test]
    fn coinbase_bundle_builds_for_post_nu6_3() {
        let mut rng = OsRng;

        // Coinbase bundles disable nonzero-valued spends. From NU6.3, consensus requires
        // nActionsOrchard = 0 in a v5+ coinbase transaction (v4, still valid after NU6.3,
        // has no Orchard bundle). So a post-NU6.3 coinbase bundle built by this crate must
        // be an Ironwood bundle. There the builder leaves cross-address enabled by default,
        // and therefore ordinary outputs build normally.
        let builder =
            output_only_builder(&mut rng, BundleVersion::ironwood_v3(), BundleType::Coinbase);

        let (bundle, _) = builder
            .build::<i64>(&mut rng)
            .expect("coinbase bundles build under the post-NU 6.3 circuit version")
            .expect("a bundle is produced for the requested output");
        assert_eq!(bundle.actions().len(), 1);
        assert_eq!(bundle.circuit_version(), OrchardCircuitVersion::PostNu6_3);
        assert!(!bundle.flags().spends_enabled());
        assert!(bundle.flags().outputs_enabled());
        assert!(bundle.flags().cross_address_enabled());
    }

    #[test]
    fn coinbase_rejects_spends_enabled_flags() {
        let anchor = EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into();
        let bundle_version = BundleVersion::ironwood_v3();

        // A coinbase bundle must disable spends; the builder rejects spends-enabled flags at
        // construction rather than silently producing an invalid bundle.
        assert!(matches!(
            Builder::new(
                BundleType::Coinbase,
                bundle_version,
                bundle_version.default_flags(),
                anchor,
            ),
            Err(BuildError::CoinbaseSpendsEnabled)
        ));

        // Spends-disabled flags are accepted.
        assert!(Builder::new(
            BundleType::Coinbase,
            bundle_version,
            Flags::from_parts(
                false,
                true,
                bundle_version.permits_cross_address_transfers()
            ),
            anchor,
        )
        .is_ok());
    }

    #[test]
    fn free_bundle_rejects_coinbase_spends_enabled() {
        let mut rng = OsRng;
        let anchor: Anchor = EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into();
        let bundle_version = BundleVersion::ironwood_v3();

        // The coinbase-spends invariant is enforced on every build path, not just at
        // `Builder::new`: a direct caller of the free `bundle` function cannot silently produce a
        // coinbase bundle with `enableSpends` set.
        let result = bundle::<i64>(
            &mut rng,
            BundleType::Coinbase,
            bundle_version,
            bundle_version.default_flags(), // spends enabled
            anchor,
            vec![],
            vec![],
            vec![],
        );
        assert!(matches!(result, Err(BuildError::CoinbaseSpendsEnabled)));
    }

    #[test]
    fn new_rejects_unrepresentable_flags() {
        // Orchard from NU6.3 onward cannot encode cross-address-enabled flags.
        let bundle_version = BundleVersion::orchard_v3();
        assert!(matches!(
            Builder::new(
                BundleType::DEFAULT,
                bundle_version,
                Flags::ENABLED,
                EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
            ),
            Err(BuildError::UnrepresentableFlags)
        ));
    }

    #[test]
    fn cross_address_disabled_builder_pairs_actions() {
        let mut rng = OsRng;
        let spend_sk = SpendingKey::random(&mut rng);
        let spend_fvk = FullViewingKey::from(&spend_sk);
        let spend_recipient = spend_fvk.address_at(0u32, Scope::External);
        let change_sk = SpendingKey::random(&mut rng);
        let change_fvk = FullViewingKey::from(&change_sk);
        let change_recipient = change_fvk.address_at(0u32, Scope::Internal);
        let bundle_version = BundleVersion::orchard_v3();
        let (note, merkle_path, anchor) = note_with_path(
            &mut rng,
            spend_recipient,
            NoteValue::from_raw(15_000),
            bundle_version.note_version(),
        );

        let mut builder = Builder::new(
            transactional(false),
            bundle_version,
            bundle_version.default_flags(),
            anchor,
        )
        .unwrap();
        assert_eq!(
            builder.add_output(
                None,
                change_recipient,
                NoteValue::from_raw(5_000),
                [0u8; 512]
            ),
            Err(OutputError::CrossAddressDisabled)
        );
        assert_eq!(
            builder.add_change_output(
                FullViewingKey::from(&SpendingKey::random(&mut rng)),
                None,
                change_recipient,
                NoteValue::from_raw(5_000),
                [0u8; 512],
            ),
            Err(OutputError::RecipientNotOwned)
        );

        builder
            .add_spend(spend_fvk.clone(), note, merkle_path)
            .unwrap();
        builder
            .add_change_output(
                change_fvk.clone(),
                None,
                change_recipient,
                NoteValue::from_raw(5_000),
                [0u8; 512],
            )
            .unwrap();
        let balance: i64 = builder.value_balance().unwrap();
        assert_eq!(balance, 10_000);

        let (pczt_bundle, bundle_meta) = builder.build_for_pczt(&mut rng).unwrap();
        assert!(!pczt_bundle.flags().cross_address_enabled());
        assert_eq!(pczt_bundle.actions().len(), 2);
        assert_eq!(i64::try_from(pczt_bundle.value_sum).unwrap(), 10_000);
        pczt_bundle.verify_cross_address_restriction().unwrap();

        let spend_action_index = bundle_meta.spend_action_index(0).unwrap();
        let change_action_index = bundle_meta.output_action_index(0).unwrap();
        assert_ne!(spend_action_index, change_action_index);

        let spend_action = &pczt_bundle.actions()[spend_action_index];
        assert_eq!(
            spend_action.spend.recipient,
            Some(spend_recipient),
            "the real spend remains at the spent note's address"
        );
        assert_eq!(spend_action.spend.value, Some(NoteValue::from_raw(15_000)));
        assert!(spend_action.spend.dummy_sk.is_none());
        assert_eq!(spend_action.output.recipient, Some(spend_recipient));
        assert_eq!(spend_action.output.value, Some(NoteValue::ZERO));

        // The spend-paired output's ciphertext is randomized, so signers (e.g. Keystone) must be
        // able to classify it as a zero-valued dummy and tolerate the undecryptable ciphertext
        // rather than reconstructing and rejecting it. That requires the explicit note fields to
        // be present (so the commitment verifies), no `user_address`, and a zero value.
        assert!(spend_action.output.user_address.is_none());
        assert!(spend_action.output.rseed.is_some());
        assert!(spend_action
            .output
            .verify_note_commitment(&spend_action.spend)
            .is_ok());

        let change_action = &pczt_bundle.actions()[change_action_index];
        assert_eq!(change_action.spend.recipient, Some(change_recipient));
        assert_eq!(change_action.spend.value, Some(NoteValue::ZERO));
        assert!(change_action.spend.dummy_sk.is_none());
        assert_eq!(change_action.spend.fvk.as_ref(), Some(&change_fvk));
        assert_eq!(change_action.output.recipient, Some(change_recipient));
        assert_eq!(change_action.output.value, Some(NoteValue::from_raw(5_000)));

        for action in pczt_bundle.actions() {
            assert!(action
                .spend
                .recipient
                .as_ref()
                .unwrap()
                .same_expanded_receiver(action.output.recipient.as_ref().unwrap()));
        }
    }

    #[test]
    fn cross_address_disabled_spend_output_randomization_depends_on_scope() {
        fn spend_paired_output_decrypts(spend_scope: Scope) -> bool {
            let mut rng = OsRng;
            let spend_sk = SpendingKey::random(&mut rng);
            let spend_fvk = FullViewingKey::from(&spend_sk);
            let spend_recipient = spend_fvk.address_at(0u32, spend_scope);
            let bundle_version = BundleVersion::orchard_v3();
            let (note, merkle_path, anchor) = note_with_path(
                &mut rng,
                spend_recipient,
                NoteValue::from_raw(15_000),
                bundle_version.note_version(),
            );

            let mut builder = Builder::new(
                transactional(true),
                bundle_version,
                bundle_version.default_flags(),
                anchor,
            )
            .unwrap();
            builder
                .add_spend(spend_fvk.clone(), note, merkle_path)
                .unwrap();

            let (pczt_bundle, bundle_meta) = builder.build_for_pczt(&mut rng).unwrap();
            let spend_action = &pczt_bundle.actions()[bundle_meta.spend_action_index(0).unwrap()];
            let domain = OrchardDomain::for_pczt_action(spend_action);

            let ivk = PreparedIncomingViewingKey::new(&spend_fvk.to_ivk(spend_scope));
            try_note_decryption(&domain, &ivk, spend_action).is_some()
        }

        assert!(!spend_paired_output_decrypts(Scope::External));
        assert!(spend_paired_output_decrypts(Scope::Internal));
    }

    #[test]
    fn cross_address_disabled_padding_pairs_dummy_addresses() {
        let mut rng = OsRng;
        let sk = SpendingKey::random(&mut rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::Internal);
        let mut builder = Builder::new(
            transactional(true),
            BundleVersion::orchard_v3(),
            BundleVersion::orchard_v3().default_flags(),
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
        )
        .unwrap();

        builder
            .add_change_output(fvk, None, recipient, NoteValue::ZERO, [0u8; 512])
            .unwrap();

        let (pczt_bundle, bundle_meta) = builder.build_for_pczt(&mut rng).unwrap();
        assert_eq!(pczt_bundle.actions().len(), 2);
        pczt_bundle.verify_cross_address_restriction().unwrap();

        let change_action_index = bundle_meta.output_action_index(0).unwrap();
        let (_, padding_action) = pczt_bundle
            .actions()
            .iter()
            .enumerate()
            .find(|(idx, action)| *idx != change_action_index && action.spend.dummy_sk.is_some())
            .unwrap();

        assert_eq!(padding_action.spend.value, Some(NoteValue::ZERO));
        assert_eq!(padding_action.output.value, Some(NoteValue::ZERO));
        assert!(padding_action
            .spend
            .recipient
            .as_ref()
            .unwrap()
            .same_expanded_receiver(padding_action.output.recipient.as_ref().unwrap()));
    }

    #[test]
    fn cross_address_disabled_rejects_non_change_outputs() {
        let mut rng = OsRng;
        let sk = SpendingKey::random(&mut rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);
        let bundle_version = BundleVersion::orchard_v3();

        assert!(matches!(
            bundle::<i64>(
                &mut rng,
                transactional(false),
                bundle_version,
                bundle_version.default_flags(),
                EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
                vec![],
                vec![OutputInfo::new(
                    None,
                    recipient,
                    NoteValue::from_raw(5_000),
                    bundle_version.note_version(),
                    [0u8; 512],
                )],
                vec![],
            ),
            Err(BuildError::CrossAddressDisabled)
        ));

        let change_output = ChangeInfo::new(
            fvk,
            None,
            recipient,
            NoteValue::from_raw(5_000),
            bundle_version.note_version(),
            [0u8; 512],
        )
        .unwrap();
        let (bundle, bundle_meta) = bundle::<i64>(
            &mut rng,
            transactional(false),
            bundle_version,
            bundle_version.default_flags(),
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
            vec![],
            vec![],
            vec![change_output],
        )
        .unwrap()
        .unwrap();

        assert!(!bundle.flags().cross_address_enabled());
        assert!(bundle_meta.output_action_index(0).is_some());
    }

    #[test]
    fn cross_address_disabled_rejects_spends_disabled_outputs() {
        let mut rng = OsRng;
        let sk = SpendingKey::random(&mut rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::Internal);
        let bundle_type = transactional(false);
        let bundle_version = BundleVersion::orchard_v3();
        // Under Orchard from NU6.3 onward this is spends-disabled and cross-address-disabled.
        let flags = Flags::from_parts(
            false,
            true,
            bundle_version.permits_cross_address_transfers(),
        );
        assert!(!flags.spends_enabled());
        assert!(flags.outputs_enabled());
        assert!(!flags.cross_address_enabled());

        let change_output = ChangeInfo::new(
            fvk,
            None,
            recipient,
            NoteValue::from_raw(5_000),
            bundle_version.note_version(),
            [0u8; 512],
        )
        .unwrap();

        assert!(matches!(
            bundle::<i64>(
                &mut rng,
                bundle_type,
                bundle_version,
                flags,
                EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
                vec![],
                vec![],
                vec![change_output],
            ),
            Err(BuildError::BundleTypeNotSatisfiable)
        ));
    }

    #[test]
    fn builder_note_version_checks_apply_to_outputs_only() {
        let mut rng = OsRng;
        let sk = SpendingKey::random(&mut rng);
        let fvk = FullViewingKey::from(&sk);
        let recipient = fvk.address_at(0u32, Scope::External);
        let bundle_version = BundleVersion::ironwood_v3();
        let mismatched_note_version = NoteVersion::V2;

        let (note, merkle_path, anchor) = note_with_path(
            &mut rng,
            recipient,
            NoteValue::from_raw(15_000),
            mismatched_note_version,
        );
        let mut builder = Builder::new(
            BundleType::DEFAULT,
            bundle_version,
            bundle_version.default_flags(),
            anchor,
        )
        .unwrap();
        assert_eq!(builder.add_spend(fvk.clone(), note, merkle_path), Ok(()));

        let (note, merkle_path, anchor) = note_with_path(
            &mut rng,
            recipient,
            NoteValue::from_raw(15_000),
            mismatched_note_version,
        );
        let spend = SpendInfo::new(fvk.clone(), note, merkle_path).unwrap();
        assert!(bundle::<i64>(
            &mut rng,
            BundleType::DEFAULT,
            bundle_version,
            bundle_version.default_flags(),
            anchor,
            vec![spend],
            vec![],
            vec![],
        )
        .is_ok());

        let output = OutputInfo::new(
            None,
            recipient,
            NoteValue::from_raw(5_000),
            mismatched_note_version,
            [0u8; 512],
        );
        assert!(matches!(
            bundle::<i64>(
                &mut rng,
                BundleType::DEFAULT,
                bundle_version,
                bundle_version.default_flags(),
                EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
                vec![],
                vec![output],
                vec![],
            ),
            Err(BuildError::InvalidNoteVersion)
        ));

        let change = ChangeInfo::new(
            fvk,
            None,
            recipient,
            NoteValue::from_raw(5_000),
            mismatched_note_version,
            [0u8; 512],
        )
        .unwrap();
        assert!(matches!(
            bundle::<i64>(
                &mut rng,
                BundleType::DEFAULT,
                bundle_version,
                bundle_version.default_flags(),
                EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
                vec![],
                vec![],
                vec![change],
            ),
            Err(BuildError::InvalidNoteVersion)
        ));
    }

    #[test]
    fn add_change_output_validates_ownership_when_unrestricted() {
        let mut rng = OsRng;
        let fvk = FullViewingKey::from(&SpendingKey::random(&mut rng));
        let owned = fvk.address_at(0u32, Scope::Internal);
        let foreign =
            FullViewingKey::from(&SpendingKey::random(&mut rng)).address_at(0u32, Scope::External);
        let bundle_version = BundleVersion::orchard_v2();
        let mut builder = Builder::new(
            BundleType::DEFAULT,
            bundle_version,
            bundle_version.default_flags(),
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
        )
        .unwrap();

        // Even in a bundle that permits cross-address transfers, a change output's ownership
        // is validated eagerly (the fvk is no longer dead weight).
        assert_eq!(
            builder.add_change_output(
                fvk.clone(),
                None,
                foreign,
                NoteValue::from_raw(5_000),
                [0u8; 512]
            ),
            Err(OutputError::RecipientNotOwned)
        );
        // An owned recipient is accepted and counts as one of the bundle's outputs.
        builder
            .add_change_output(fvk, None, owned, NoteValue::from_raw(5_000), [0u8; 512])
            .unwrap();
        assert_eq!(builder.changes().len(), 1);
    }

    #[test]
    fn add_change_output_rejects_spends_disabled_eagerly() {
        let mut rng = OsRng;
        let fvk = FullViewingKey::from(&SpendingKey::random(&mut rng));
        let recipient = fvk.address_at(0u32, Scope::Internal);

        // For a cross-address-disabled bundle with spends disabled and outputs enabled,
        // the change output is rejected eagerly.
        //
        // The builder wouldn't be prevented by consensus from supporting this (fabricating
        // a spend with the same expanded receiver), but there is no point: `enableSpends = 0`
        // is only enforced for coinbase transactions, and coinbase transactions do not allow
        // Orchard actions at all post NU6.3. So the case unsupported by the builder would only
        // happen by voluntarily disabling `enableSpends` and/or `enableCrossAddress` when
        // consensus does not require it.
        let bundle_version = BundleVersion::orchard_v3();
        let mut builder = Builder::new(
            transactional(false),
            bundle_version,
            Flags::from_parts(
                false,
                true,
                bundle_version.permits_cross_address_transfers(),
            ),
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
        )
        .unwrap();

        assert_eq!(
            builder.add_change_output(fvk, None, recipient, NoteValue::from_raw(5_000), [0u8; 512]),
            Err(OutputError::SpendsDisabled)
        );
    }

    #[test]
    fn cross_address_disabled_non_pczt_signing_flow() {
        let mut rng = OsRng;
        let spend_sk = SpendingKey::random(&mut rng);
        let spend_fvk = FullViewingKey::from(&spend_sk);
        let spend_recipient = spend_fvk.address_at(0u32, Scope::External);
        let change_sk = SpendingKey::random(&mut rng);
        let change_fvk = FullViewingKey::from(&change_sk);
        let change_recipient = change_fvk.address_at(0u32, Scope::Internal);
        let bundle_version = BundleVersion::orchard_v3();
        let (note, merkle_path, anchor) = note_with_path(
            &mut rng,
            spend_recipient,
            NoteValue::from_raw(15_000),
            bundle_version.note_version(),
        );

        let mut builder = Builder::new(
            transactional(false),
            bundle_version,
            bundle_version.default_flags(),
            anchor,
        )
        .unwrap();
        builder.add_spend(spend_fvk, note, merkle_path).unwrap();
        builder
            .add_change_output(
                change_fvk.clone(),
                None,
                change_recipient,
                NoteValue::from_raw(5_000),
                [0u8; 512],
            )
            .unwrap();

        let bundle = builder.build::<i64>(&mut rng).unwrap().unwrap().0;

        fn num_unsigned<P: core::fmt::Debug>(
            bundle: &Bundle<super::InProgress<P, super::PartiallyAuthorized>, i64>,
        ) -> usize {
            bundle
                .actions()
                .iter()
                .filter(|a| matches!(a.authorization(), MaybeSigned::SigningMetadata(_)))
                .count()
        }

        // Both the real spend and the fabricated change spend require real signatures.
        let bundle = bundle.prepare(rng, [0; 32]);
        assert_eq!(num_unsigned(&bundle), 2);

        let bundle = bundle.sign(rng, &SpendAuthorizingKey::from(&spend_sk));
        assert_eq!(num_unsigned(&bundle), 1);

        let bundle = bundle.sign(rng, &SpendAuthorizingKey::from(&change_sk));
        assert_eq!(num_unsigned(&bundle), 0);

        // A change-only bundle: the padding dummy spend is signed during `prepare`, so
        // a single `sign` call with the change key completes the actions.
        let mut builder = Builder::new(
            transactional(false),
            BundleVersion::orchard_v3(),
            BundleVersion::orchard_v3().default_flags(),
            EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
        )
        .unwrap();
        builder
            .add_change_output(
                change_fvk,
                None,
                change_recipient,
                NoteValue::from_raw(5_000),
                [0u8; 512],
            )
            .unwrap();

        let bundle = builder
            .build::<i64>(&mut rng)
            .unwrap()
            .unwrap()
            .0
            .prepare(rng, [0; 32]);
        assert_eq!(bundle.actions().len(), 2);
        assert_eq!(num_unsigned(&bundle), 1);

        let bundle = bundle.sign(rng, &SpendAuthorizingKey::from(&change_sk));
        assert_eq!(num_unsigned(&bundle), 0);
    }

    #[test]
    fn restricted_pczt_structural_checks_reject_tampering() {
        let pk = ProvingKey::build(OrchardCircuitVersion::PostNu6_3);
        let mut rng = OsRng;
        let spend_sk = SpendingKey::random(&mut rng);
        let spend_fvk = FullViewingKey::from(&spend_sk);
        let spend_recipient = spend_fvk.address_at(0u32, Scope::External);
        let change_sk = SpendingKey::random(&mut rng);
        let change_fvk = FullViewingKey::from(&change_sk);
        let change_recipient = change_fvk.address_at(0u32, Scope::Internal);
        let bundle_version = BundleVersion::orchard_v3();
        let (note, merkle_path, anchor) = note_with_path(
            &mut rng,
            spend_recipient,
            NoteValue::from_raw(15_000),
            bundle_version.note_version(),
        );

        let mut builder = Builder::new(
            transactional(false),
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

        let (mut pczt_bundle, _) = builder.build_for_pczt(&mut rng).unwrap();
        pczt_bundle.verify_cross_address_restriction().unwrap();
        pczt_bundle.create_proof(&pk, rng).unwrap();

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
            pczt_bundle.verify_cross_address_restriction(),
            Err(VerifyError::DisallowedCrossAddressTransfer)
        ));
        assert!(matches!(
            pczt_bundle.create_proof(&pk, rng),
            Err(ProverError::DisallowedCrossAddressTransfer(_))
        ));
    }

    #[test]
    fn create_proof_supports_cross_address_disabled_only_for_post_nu6_3() {
        // A cross-address-disabled bundle can only be built under `BundleVersion::orchard_v3()`
        // (`BundleVersion` owns the cross-address policy), which builds post-NU6.3
        // circuits. Proving therefore requires a matching post-NU6.3 key; a pre-NU6.3 key
        // is rejected as a circuit-version mismatch. The lower-level interlock that rejects
        // a restricted *instance* under an unsupporting key is covered by
        // `circuit::tests::restricted_statement_requires_supporting_key`.
        let build_restricted = |rng: &mut OsRng| {
            Builder::new(
                transactional(true),
                BundleVersion::orchard_v3(),
                BundleVersion::orchard_v3().default_flags(),
                EMPTY_ROOTS[MERKLE_DEPTH_ORCHARD].into(),
            )
            .unwrap()
            .build::<i64>(rng)
            .unwrap()
            .unwrap()
            .0
        };

        let mut rng = OsRng;
        let pk = ProvingKey::build(OrchardCircuitVersion::FixedPostNu6_2);
        let bundle = build_restricted(&mut rng);
        assert!(matches!(
            bundle.create_proof(&pk, &mut rng),
            Err(BuildError::Proof(halo2_proofs::plonk::Error::Synthesis)),
        ));

        let pk = ProvingKey::build(OrchardCircuitVersion::PostNu6_3);
        let bundle = build_restricted(&mut rng);
        bundle.create_proof(&pk, &mut rng).unwrap();
    }
}
