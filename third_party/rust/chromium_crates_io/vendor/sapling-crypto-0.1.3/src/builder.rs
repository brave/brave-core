//! Types and functions for building Sapling transaction components.

use core::fmt;
use std::{iter, marker::PhantomData};

use group::ff::Field;
use incrementalmerkletree::Position;
use rand::{seq::SliceRandom, RngCore};
use rand_core::CryptoRng;
use redjubjub::{Binding, SpendAuth};

use crate::{
    bundle::{
        Authorization, Authorized, Bundle, GrothProofBytes, OutputDescription, SpendDescription,
    },
    circuit,
    keys::{OutgoingViewingKey, SpendAuthorizingKey, SpendValidatingKey},
    note_encryption::{sapling_note_encryption, Zip212Enforcement},
    prover::{OutputProver, SpendProver},
    util::generate_random_rseed_internal,
    value::{
        CommitmentSum, NoteValue, TrapdoorSum, ValueCommitTrapdoor, ValueCommitment, ValueSum,
    },
    zip32::ExtendedSpendingKey,
    Anchor, Diversifier, MerklePath, Node, Note, PaymentAddress, ProofGenerationKey, SaplingIvk,
    NOTE_COMMITMENT_TREE_DEPTH,
};

/// If there are any shielded inputs, always have at least two shielded outputs, padding
/// with dummy outputs if necessary. See <https://github.com/zcash/zcash/issues/3615>.
const MIN_SHIELDED_OUTPUTS: usize = 2;

/// An enumeration of rules for Sapling bundle construction.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BundleType {
    /// A transactional bundle will be padded if necessary to contain at least 2 outputs,
    /// irrespective of whether any genuine outputs are required.
    Transactional {
        /// A flag that, when set to `true`, indicates that the resulting bundle should be
        /// produced with the minimum required number of spends (1) and outputs (2 with
        /// padding) to be usable on its own in a transaction, irrespective of whether any
        /// spends or outputs have been requested. If no explicit spends or outputs have
        /// been added, all of the spends and outputs in the resulting bundle will be
        /// dummies.
        bundle_required: bool,
    },
    /// A coinbase bundle is required to have no spends. No output padding is performed.
    Coinbase,
}

impl BundleType {
    /// The default bundle type allows both spends and outputs, and does not require a
    /// bundle to be produced if no spends or outputs have been added to the bundle.
    pub const DEFAULT: BundleType = BundleType::Transactional {
        bundle_required: false,
    };

    /// Returns the number of logical spends that a builder will produce in constructing a bundle
    /// of this type, given the specified numbers of spends and outputs.
    ///
    /// Returns an error if the specified number of spends and outputs is incompatible with
    /// this bundle type.
    pub fn num_spends(&self, requested_spends: usize) -> Result<usize, &'static str> {
        match self {
            BundleType::Transactional { bundle_required } => {
                Ok(if *bundle_required || requested_spends > 0 {
                    core::cmp::max(requested_spends, 1)
                } else {
                    0
                })
            }
            BundleType::Coinbase => {
                if requested_spends == 0 {
                    Ok(0)
                } else {
                    Err("Spends not allowed in coinbase bundles")
                }
            }
        }
    }

    /// Returns the number of logical outputs that a builder will produce in constructing a bundle
    /// of this type, given the specified numbers of spends and outputs.
    ///
    /// Returns an error if the specified number of spends and outputs is incompatible with
    /// this bundle type.
    pub fn num_outputs(
        &self,
        requested_spends: usize,
        requested_outputs: usize,
    ) -> Result<usize, &'static str> {
        match self {
            BundleType::Transactional { bundle_required } => Ok(
                if *bundle_required || requested_spends > 0 || requested_outputs > 0 {
                    core::cmp::max(requested_outputs, MIN_SHIELDED_OUTPUTS)
                } else {
                    0
                },
            ),
            BundleType::Coinbase => {
                if requested_spends == 0 {
                    Ok(requested_outputs)
                } else {
                    Err("Spends not allowed in coinbase bundles")
                }
            }
        }
    }
}

#[derive(Debug, PartialEq, Eq)]
pub enum Error {
    AnchorMismatch,
    BindingSig,
    /// A signature is valid for more than one input. This should never happen if `alpha`
    /// is sampled correctly, and indicates a critical failure in randomness generation.
    DuplicateSignature,
    InvalidAddress,
    InvalidAmount,
    /// External signature is not valid.
    InvalidExternalSignature,
    /// A bundle could not be built because required signatures were missing.
    MissingSignatures,
    SpendProof,
    /// The bundle being constructed violated the construction rules for the requested bundle type.
    BundleTypeNotSatisfiable,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Error::AnchorMismatch => {
                write!(f, "Anchor mismatch (anchors for all spends must be equal)")
            }
            Error::BindingSig => write!(f, "Failed to create bindingSig"),
            Error::DuplicateSignature => write!(f, "Signature valid for more than one input"),
            Error::InvalidAddress => write!(f, "Invalid address"),
            Error::InvalidAmount => write!(f, "Invalid amount"),
            Error::InvalidExternalSignature => write!(f, "External signature was invalid"),
            Error::MissingSignatures => write!(f, "Required signatures were missing during build"),
            Error::SpendProof => write!(f, "Failed to create Sapling spend proof"),
            Error::BundleTypeNotSatisfiable => {
                f.write_str("Bundle structure did not conform to requested bundle type.")
            }
        }
    }
}

/// A struct containing the information necessary to add a spend to a bundle.
#[derive(Debug, Clone)]
pub struct SpendInfo {
    proof_generation_key: ProofGenerationKey,
    note: Note,
    merkle_path: MerklePath,
    dummy_ask: Option<SpendAuthorizingKey>,
}

impl SpendInfo {
    /// Constructs a [`SpendInfo`] from its constituent parts.
    pub fn new(
        proof_generation_key: ProofGenerationKey,
        note: Note,
        merkle_path: MerklePath,
    ) -> Self {
        Self {
            proof_generation_key,
            note,
            merkle_path,
            dummy_ask: None,
        }
    }

    /// Returns the value of the note to be spent.
    pub fn value(&self) -> NoteValue {
        self.note.value()
    }

    /// Defined in [Zcash Protocol Spec § 4.8.2: Dummy Notes (Sapling)][saplingdummynotes].
    ///
    /// [saplingdummynotes]: https://zips.z.cash/protocol/protocol.pdf#saplingdummynotes
    fn dummy<R: RngCore>(mut rng: R) -> Self {
        let (sk, _, note) = Note::dummy(&mut rng);
        let merkle_path = MerklePath::from_parts(
            iter::repeat_with(|| Node::from_scalar(jubjub::Base::random(&mut rng)))
                .take(NOTE_COMMITMENT_TREE_DEPTH.into())
                .collect(),
            Position::from(0),
        )
        .expect("The path length corresponds to the length of the generated vector.");

        SpendInfo {
            proof_generation_key: sk.proof_generation_key(),
            note,
            merkle_path,
            dummy_ask: Some(sk.ask),
        }
    }

    fn has_matching_anchor(&self, anchor: &Anchor) -> bool {
        if self.note.value() == NoteValue::ZERO {
            true
        } else {
            let node = Node::from_cmu(&self.note.cmu());
            &Anchor::from(self.merkle_path.root(node)) == anchor
        }
    }

    fn prepare<R: RngCore>(self, rng: R) -> PreparedSpendInfo {
        PreparedSpendInfo {
            proof_generation_key: self.proof_generation_key,
            note: self.note,
            merkle_path: self.merkle_path,
            rcv: ValueCommitTrapdoor::random(rng),
            dummy_ask: self.dummy_ask,
        }
    }
}

#[derive(Debug, Clone)]
struct PreparedSpendInfo {
    proof_generation_key: ProofGenerationKey,
    note: Note,
    merkle_path: MerklePath,
    rcv: ValueCommitTrapdoor,
    dummy_ask: Option<SpendAuthorizingKey>,
}

impl PreparedSpendInfo {
    fn build<Pr: SpendProver, R: RngCore>(
        self,
        mut rng: R,
    ) -> Result<SpendDescription<InProgress<Unproven, Unsigned>>, Error> {
        // Construct the value commitment.
        let alpha = jubjub::Fr::random(&mut rng);
        let cv = ValueCommitment::derive(self.note.value(), self.rcv.clone());
        let node = Node::from_cmu(&self.note.cmu());
        let anchor = *self.merkle_path.root(node).inner();

        let ak = self.proof_generation_key.ak.clone();

        // This is the result of the re-randomization, we compute it for the caller
        let rk = ak.randomize(&alpha);

        let nullifier = self.note.nf(
            &self.proof_generation_key.to_viewing_key().nk,
            u64::try_from(self.merkle_path.position())
                .expect("Sapling note commitment tree position must fit into a u64"),
        );

        let zkproof = Pr::prepare_circuit(
            self.proof_generation_key,
            *self.note.recipient().diversifier(),
            *self.note.rseed(),
            self.note.value(),
            alpha,
            self.rcv,
            anchor,
            self.merkle_path.clone(),
        )
        .ok_or(Error::SpendProof)?;

        Ok(SpendDescription::from_parts(
            cv,
            anchor,
            nullifier,
            rk,
            zkproof,
            SigningMetadata {
                dummy_ask: self.dummy_ask,
                parts: SigningParts { ak, alpha },
            },
        ))
    }
}

/// A struct containing the information required in order to construct a
/// Sapling output to a transaction.
#[derive(Clone)]
pub struct OutputInfo {
    /// `None` represents the `ovk = ⊥` case.
    ovk: Option<OutgoingViewingKey>,
    to: PaymentAddress,
    value: NoteValue,
    memo: [u8; 512],
}

impl OutputInfo {
    /// Constructs a new [`OutputInfo`] from its constituent parts.
    pub fn new(
        ovk: Option<OutgoingViewingKey>,
        to: PaymentAddress,
        value: NoteValue,
        memo: Option<[u8; 512]>,
    ) -> Self {
        Self {
            ovk,
            to,
            value,
            memo: memo.unwrap_or_else(|| {
                let mut memo = [0; 512];
                memo[0] = 0xf6;
                memo
            }),
        }
    }

    /// Returns the recipient of the new output.
    pub fn recipient(&self) -> PaymentAddress {
        self.to
    }

    /// Returns the value of the output.
    pub fn value(&self) -> NoteValue {
        self.value
    }

    /// Constructs a new dummy Sapling output.
    pub fn dummy<R: RngCore>(mut rng: &mut R) -> Self {
        // This is a dummy output
        let dummy_to = {
            let mut diversifier = Diversifier([0; 11]);
            loop {
                rng.fill_bytes(&mut diversifier.0);
                let dummy_ivk = SaplingIvk(jubjub::Fr::random(&mut rng));
                if let Some(addr) = dummy_ivk.to_payment_address(diversifier) {
                    break addr;
                }
            }
        };

        Self::new(None, dummy_to, NoteValue::ZERO, None)
    }

    fn prepare<R: RngCore>(
        self,
        rng: &mut R,
        zip212_enforcement: Zip212Enforcement,
    ) -> PreparedOutputInfo {
        let rseed = generate_random_rseed_internal(zip212_enforcement, rng);

        let note = Note::from_parts(self.to, self.value, rseed);

        PreparedOutputInfo {
            ovk: self.ovk,
            note,
            memo: self.memo,
            rcv: ValueCommitTrapdoor::random(rng),
        }
    }
}

struct PreparedOutputInfo {
    /// `None` represents the `ovk = ⊥` case.
    ovk: Option<OutgoingViewingKey>,
    note: Note,
    memo: [u8; 512],
    rcv: ValueCommitTrapdoor,
}

impl PreparedOutputInfo {
    fn build<Pr: OutputProver, R: RngCore>(
        self,
        rng: &mut R,
    ) -> OutputDescription<circuit::Output> {
        let encryptor = sapling_note_encryption::<R>(self.ovk, self.note.clone(), self.memo, rng);

        // Construct the value commitment.
        let cv = ValueCommitment::derive(self.note.value(), self.rcv.clone());

        // Prepare the circuit that will be used to construct the proof.
        let zkproof = Pr::prepare_circuit(
            encryptor.esk().0,
            self.note.recipient(),
            self.note.rcm(),
            self.note.value(),
            self.rcv,
        );

        let cmu = self.note.cmu();

        let enc_ciphertext = encryptor.encrypt_note_plaintext();
        let out_ciphertext = encryptor.encrypt_outgoing_plaintext(&cv, &cmu, rng);

        let epk = encryptor.epk();

        OutputDescription::from_parts(
            cv,
            cmu,
            epk.to_bytes(),
            enc_ciphertext,
            out_ciphertext,
            zkproof,
        )
    }
}

/// Metadata about a transaction created by a [`Builder`].
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SaplingMetadata {
    spend_indices: Vec<usize>,
    output_indices: Vec<usize>,
}

impl SaplingMetadata {
    pub fn empty() -> Self {
        SaplingMetadata {
            spend_indices: vec![],
            output_indices: vec![],
        }
    }

    /// Returns the index within the transaction of the [`SpendDescription`] corresponding
    /// to the `n`-th call to [`Builder::add_spend`].
    ///
    /// Note positions are randomized when building transactions for indistinguishability.
    /// This means that the transaction consumer cannot assume that e.g. the first spend
    /// they added (via the first call to [`Builder::add_spend`]) is the first
    /// [`SpendDescription`] in the transaction.
    pub fn spend_index(&self, n: usize) -> Option<usize> {
        self.spend_indices.get(n).copied()
    }

    /// Returns the index within the transaction of the [`OutputDescription`] corresponding
    /// to the `n`-th call to [`Builder::add_output`].
    ///
    /// Note positions are randomized when building transactions for indistinguishability.
    /// This means that the transaction consumer cannot assume that e.g. the first output
    /// they added (via the first call to [`Builder::add_output`]) is the first
    /// [`OutputDescription`] in the transaction.
    pub fn output_index(&self, n: usize) -> Option<usize> {
        self.output_indices.get(n).copied()
    }
}

/// A mutable builder type for constructing Sapling bundles.
pub struct Builder {
    value_balance: ValueSum,
    spends: Vec<SpendInfo>,
    outputs: Vec<OutputInfo>,
    zip212_enforcement: Zip212Enforcement,
    bundle_type: BundleType,
    anchor: Anchor,
}

impl Builder {
    pub fn new(
        zip212_enforcement: Zip212Enforcement,
        bundle_type: BundleType,
        anchor: Anchor,
    ) -> Self {
        Builder {
            value_balance: ValueSum::zero(),
            spends: vec![],
            outputs: vec![],
            zip212_enforcement,
            bundle_type,
            anchor,
        }
    }

    /// Returns the list of Sapling inputs that have been added to the builder.
    pub fn inputs(&self) -> &[SpendInfo] {
        &self.spends
    }

    /// Returns the Sapling outputs that have been added to the builder.
    pub fn outputs(&self) -> &[OutputInfo] {
        &self.outputs
    }

    /// Returns the net value represented by the spends and outputs added to this builder,
    /// or an error if the values added to this builder overflow the range of a Zcash
    /// monetary amount.
    fn try_value_balance<V: TryFrom<i64>>(&self) -> Result<V, Error> {
        self.value_balance
            .try_into()
            .map_err(|_| ())
            .and_then(|vb| V::try_from(vb).map_err(|_| ()))
            .map_err(|()| Error::InvalidAmount)
    }

    /// Returns the net value represented by the spends and outputs added to this builder.
    pub fn value_balance<V: TryFrom<i64>>(&self) -> V {
        self.try_value_balance()
            .expect("we check this when mutating self.value_balance")
    }

    /// Adds a Sapling note to be spent in this transaction.
    ///
    /// Returns an error if the given Merkle path does not have the same anchor as the
    /// paths for previous Sapling notes.
    pub fn add_spend(
        &mut self,
        extsk: &ExtendedSpendingKey,
        note: Note,
        merkle_path: MerklePath,
    ) -> Result<(), Error> {
        let spend = SpendInfo::new(extsk.expsk.proof_generation_key(), note, merkle_path);

        // Consistency check: all anchors must equal the first one
        match self.bundle_type {
            BundleType::Transactional { .. } => {
                if !spend.has_matching_anchor(&self.anchor) {
                    return Err(Error::AnchorMismatch);
                }
            }
            BundleType::Coinbase => {
                return Err(Error::BundleTypeNotSatisfiable);
            }
        }

        self.value_balance = (self.value_balance + spend.value()).ok_or(Error::InvalidAmount)?;
        self.try_value_balance::<i64>()?;

        self.spends.push(spend);

        Ok(())
    }

    /// Adds a Sapling address to send funds to.
    pub fn add_output(
        &mut self,
        ovk: Option<OutgoingViewingKey>,
        to: PaymentAddress,
        value: NoteValue,
        memo: Option<[u8; 512]>,
    ) -> Result<(), Error> {
        let output = OutputInfo::new(ovk, to, value, memo);

        self.value_balance = (self.value_balance - value).ok_or(Error::InvalidAddress)?;
        self.try_value_balance::<i64>()?;

        self.outputs.push(output);

        Ok(())
    }

    /// Constructs the Sapling bundle from the builder's accumulated state.
    pub fn build<SP: SpendProver, OP: OutputProver, R: RngCore, V: TryFrom<i64>>(
        self,
        rng: R,
    ) -> Result<Option<(UnauthorizedBundle<V>, SaplingMetadata)>, Error> {
        bundle::<SP, OP, _, _>(
            rng,
            self.bundle_type,
            self.zip212_enforcement,
            self.anchor,
            self.spends,
            self.outputs,
        )
    }
}

/// Constructs a new Sapling transaction bundle of the given type from the specified set of spends
/// and outputs.
pub fn bundle<SP: SpendProver, OP: OutputProver, R: RngCore, V: TryFrom<i64>>(
    mut rng: R,
    bundle_type: BundleType,
    zip212_enforcement: Zip212Enforcement,
    anchor: Anchor,
    spends: Vec<SpendInfo>,
    outputs: Vec<OutputInfo>,
) -> Result<Option<(UnauthorizedBundle<V>, SaplingMetadata)>, Error> {
    match bundle_type {
        BundleType::Transactional { .. } => {
            for spend in &spends {
                if !spend.has_matching_anchor(&anchor) {
                    return Err(Error::AnchorMismatch);
                }
            }
        }
        BundleType::Coinbase => {
            if !spends.is_empty() {
                return Err(Error::BundleTypeNotSatisfiable);
            }
        }
    }

    let requested_spend_count = spends.len();
    let bundle_spend_count = bundle_type
        .num_spends(requested_spend_count)
        .map_err(|_| Error::BundleTypeNotSatisfiable)?;

    let requested_output_count = outputs.len();
    let bundle_output_count = bundle_type
        .num_outputs(spends.len(), requested_output_count)
        .map_err(|_| Error::BundleTypeNotSatisfiable)?;
    assert!(requested_output_count <= bundle_output_count);

    // Set up the transaction metadata that will be used to record how
    // inputs and outputs are shuffled.
    let mut tx_metadata = SaplingMetadata::empty();
    tx_metadata.spend_indices.resize(spends.len(), 0);
    tx_metadata.output_indices.resize(requested_output_count, 0);

    // Create any required dummy spends and record initial spend positions
    let mut indexed_spends: Vec<_> = spends
        .into_iter()
        .chain(iter::repeat_with(|| SpendInfo::dummy(&mut rng)))
        .enumerate()
        .take(bundle_spend_count)
        .collect();

    // Create any required dummy outputs and record initial output positions
    let mut indexed_outputs: Vec<_> = outputs
        .into_iter()
        .chain(iter::repeat_with(|| OutputInfo::dummy(&mut rng)))
        .enumerate()
        .take(bundle_output_count)
        .collect();

    // Randomize order of inputs and outputs
    indexed_spends.shuffle(&mut rng);
    indexed_outputs.shuffle(&mut rng);

    // Record the transaction metadata and create prepared spends and outputs.
    let spend_infos = indexed_spends
        .into_iter()
        .enumerate()
        .map(|(i, (pos, spend))| {
            // Record the post-randomized spend location
            if pos < requested_spend_count {
                tx_metadata.spend_indices[pos] = i;
            }

            spend.prepare(&mut rng)
        })
        .collect::<Vec<_>>();
    let output_infos = indexed_outputs
        .into_iter()
        .enumerate()
        .map(|(i, (pos, output))| {
            // Record the post-randomized output location. Due to how `indexed_outputs` is
            // constructed, all non-dummy positions will be less than requested_output_count
            if pos < requested_output_count {
                tx_metadata.output_indices[pos] = i;
            }

            output.prepare(&mut rng, zip212_enforcement)
        })
        .collect::<Vec<_>>();

    // Compute the transaction binding signing key.
    let bsk = {
        let spends: TrapdoorSum = spend_infos.iter().map(|spend| &spend.rcv).sum();
        let outputs: TrapdoorSum = output_infos.iter().map(|output| &output.rcv).sum();
        (spends - outputs).into_bsk()
    };

    // Compute the Sapling value balance of the bundle for comparison to `bvk` and `bsk`
    let input_total = spend_infos
        .iter()
        .try_fold(ValueSum::zero(), |balance, spend| {
            (balance + spend.note.value()).ok_or(Error::InvalidAmount)
        })?;
    let value_balance = output_infos
        .iter()
        .try_fold(input_total, |balance, output| {
            (balance - output.note.value()).ok_or(Error::InvalidAmount)
        })?;
    let value_balance_i64 = i64::try_from(value_balance).map_err(|_| Error::InvalidAmount)?;

    // Create the unauthorized Spend and Output descriptions.
    let shielded_spends = spend_infos
        .into_iter()
        .map(|a| a.build::<SP, _>(&mut rng))
        .collect::<Result<Vec<_>, _>>()?;
    let shielded_outputs = output_infos
        .into_iter()
        .map(|a| a.build::<OP, _>(&mut rng))
        .collect::<Vec<_>>();

    // Verify that bsk and bvk are consistent.
    let bvk = {
        let spends = shielded_spends
            .iter()
            .map(|spend| spend.cv())
            .sum::<CommitmentSum>();
        let outputs = shielded_outputs
            .iter()
            .map(|output| output.cv())
            .sum::<CommitmentSum>();
        (spends - outputs).into_bvk(value_balance_i64)
    };
    assert_eq!(redjubjub::VerificationKey::from(&bsk), bvk);

    Ok(Bundle::from_parts(
        shielded_spends,
        shielded_outputs,
        V::try_from(value_balance_i64).map_err(|_| Error::InvalidAmount)?,
        InProgress {
            sigs: Unsigned { bsk },
            _proof_state: PhantomData::default(),
        },
    )
    .map(|b| (b, tx_metadata)))
}

/// Type alias for an in-progress bundle that has no proofs or signatures.
///
/// This is returned by [`Builder::build`].
pub type UnauthorizedBundle<V> = Bundle<InProgress<Unproven, Unsigned>, V>;

/// Marker trait representing bundle proofs in the process of being created.
pub trait InProgressProofs: fmt::Debug {
    /// The proof type of a Sapling spend in the process of being proven.
    type SpendProof: Clone + fmt::Debug;
    /// The proof type of a Sapling output in the process of being proven.
    type OutputProof: Clone + fmt::Debug;
}

/// Marker trait representing bundle signatures in the process of being created.
pub trait InProgressSignatures: fmt::Debug {
    /// The authorization type of a Sapling spend or output in the process of being
    /// authorized.
    type AuthSig: Clone + fmt::Debug;
}

/// Marker for a bundle in the process of being built.
#[derive(Clone, Debug)]
pub struct InProgress<P: InProgressProofs, S: InProgressSignatures> {
    sigs: S,
    _proof_state: PhantomData<P>,
}

impl<P: InProgressProofs, S: InProgressSignatures> Authorization for InProgress<P, S> {
    type SpendProof = P::SpendProof;
    type OutputProof = P::OutputProof;
    type AuthSig = S::AuthSig;
}

/// Marker for a [`Bundle`] without proofs.
///
/// The [`SpendDescription`]s and [`OutputDescription`]s within the bundle contain the
/// private data needed to create proofs.
#[derive(Clone, Copy, Debug)]
pub struct Unproven;

impl InProgressProofs for Unproven {
    type SpendProof = circuit::Spend;
    type OutputProof = circuit::Output;
}

/// Marker for a [`Bundle`] with proofs.
#[derive(Clone, Copy, Debug)]
pub struct Proven;

impl InProgressProofs for Proven {
    type SpendProof = GrothProofBytes;
    type OutputProof = GrothProofBytes;
}

/// Reports on the progress made towards creating proofs for a bundle.
pub trait ProverProgress {
    /// Updates the progress instance with the number of steps completed and the total
    /// number of steps.
    fn update(&mut self, cur: u32, end: u32);
}

impl ProverProgress for () {
    fn update(&mut self, _: u32, _: u32) {}
}

impl<U: From<(u32, u32)>> ProverProgress for std::sync::mpsc::Sender<U> {
    fn update(&mut self, cur: u32, end: u32) {
        // If the send fails, we should ignore the error, not crash.
        self.send(U::from((cur, end))).unwrap_or(());
    }
}

impl<U: ProverProgress> ProverProgress for &mut U {
    fn update(&mut self, cur: u32, end: u32) {
        (*self).update(cur, end);
    }
}

struct CreateProofs<'a, SP: SpendProver, OP: OutputProver, R: RngCore, U: ProverProgress> {
    spend_prover: &'a SP,
    output_prover: &'a OP,
    rng: R,
    progress_notifier: U,
    total_progress: u32,
    progress: u32,
}

impl<'a, SP: SpendProver, OP: OutputProver, R: RngCore, U: ProverProgress>
    CreateProofs<'a, SP, OP, R, U>
{
    fn new(
        spend_prover: &'a SP,
        output_prover: &'a OP,
        rng: R,
        progress_notifier: U,
        total_progress: u32,
    ) -> Self {
        // Keep track of the total number of steps computed
        Self {
            spend_prover,
            output_prover,
            rng,
            progress_notifier,
            total_progress,
            progress: 0u32,
        }
    }

    fn update_progress(&mut self) {
        // Update progress and send a notification on the channel
        self.progress += 1;
        self.progress_notifier
            .update(self.progress, self.total_progress);
    }

    fn map_spend_proof(&mut self, spend: circuit::Spend) -> GrothProofBytes {
        let proof = self.spend_prover.create_proof(spend, &mut self.rng);
        self.update_progress();
        SP::encode_proof(proof)
    }

    fn map_output_proof(&mut self, output: circuit::Output) -> GrothProofBytes {
        let proof = self.output_prover.create_proof(output, &mut self.rng);
        self.update_progress();
        OP::encode_proof(proof)
    }

    fn map_authorization<S: InProgressSignatures>(
        &mut self,
        a: InProgress<Unproven, S>,
    ) -> InProgress<Proven, S> {
        InProgress {
            sigs: a.sigs,
            _proof_state: PhantomData::default(),
        }
    }
}

impl<S: InProgressSignatures, V> Bundle<InProgress<Unproven, S>, V> {
    /// Creates the proofs for this bundle.
    pub fn create_proofs<SP: SpendProver, OP: OutputProver>(
        self,
        spend_prover: &SP,
        output_prover: &OP,
        rng: impl RngCore,
        progress_notifier: impl ProverProgress,
    ) -> Bundle<InProgress<Proven, S>, V> {
        let total_progress =
            self.shielded_spends().len() as u32 + self.shielded_outputs().len() as u32;
        let mut cp = CreateProofs::new(
            spend_prover,
            output_prover,
            rng,
            progress_notifier,
            total_progress,
        );

        self.map_authorization(
            &mut cp,
            |cp, spend| cp.map_spend_proof(spend),
            |cp, output| cp.map_output_proof(output),
            |_cp, sig| sig,
            |cp, auth| cp.map_authorization(auth),
        )
    }
}

/// Marker for an unauthorized bundle with no signatures.
#[derive(Clone)]
pub struct Unsigned {
    bsk: redjubjub::SigningKey<Binding>,
}

impl fmt::Debug for Unsigned {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Unsigned").finish_non_exhaustive()
    }
}

impl InProgressSignatures for Unsigned {
    type AuthSig = SigningMetadata;
}

/// The parts needed to sign a [`SpendDescription`].
#[derive(Clone, Debug)]
pub struct SigningParts {
    /// The spend validating key for this spend description. Used to match spend
    /// authorizing keys to spend descriptions they can create signatures for.
    ak: SpendValidatingKey,
    /// The randomization needed to derive the actual signing key for this note.
    alpha: jubjub::Scalar,
}

/// Marker for a partially-authorized bundle, in the process of being signed.
#[derive(Clone, Debug)]
pub struct PartiallyAuthorized {
    binding_signature: redjubjub::Signature<Binding>,
    sighash: [u8; 32],
}

/// Container for metadata needed to sign a Sapling input.
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

impl InProgressSignatures for PartiallyAuthorized {
    type AuthSig = MaybeSigned;
}

/// A heisen[`Signature`] for a particular [`SpendDescription`].
///
/// [`Signature`]: redjubjub::Signature
#[derive(Clone, Debug)]
pub enum MaybeSigned {
    /// The information needed to sign this [`SpendDescription`].
    SigningParts(SigningParts),
    /// The signature for this [`SpendDescription`].
    Signature(redjubjub::Signature<SpendAuth>),
}

impl MaybeSigned {
    fn finalize(self) -> Result<redjubjub::Signature<SpendAuth>, Error> {
        match self {
            Self::Signature(sig) => Ok(sig),
            _ => Err(Error::MissingSignatures),
        }
    }
}

impl<P: InProgressProofs, V> Bundle<InProgress<P, Unsigned>, V> {
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
            |_, proof| proof,
            |_, proof| proof,
            |rng, SigningMetadata { dummy_ask, parts }| match dummy_ask {
                None => MaybeSigned::SigningParts(parts),
                Some(ask) => {
                    MaybeSigned::Signature(ask.randomize(&parts.alpha).sign(rng, &sighash))
                }
            },
            |rng, auth: InProgress<P, Unsigned>| InProgress {
                sigs: PartiallyAuthorized {
                    binding_signature: auth.sigs.bsk.sign(rng, &sighash),
                    sighash,
                },
                _proof_state: PhantomData::default(),
            },
        )
    }
}

impl<V> Bundle<InProgress<Proven, Unsigned>, V> {
    /// Applies signatures to this bundle, in order to authorize it.
    ///
    /// This is a helper method that wraps [`Bundle::prepare`], [`Bundle::sign`], and
    /// [`Bundle::finalize`].
    pub fn apply_signatures<R: RngCore + CryptoRng>(
        self,
        mut rng: R,
        sighash: [u8; 32],
        signing_keys: &[SpendAuthorizingKey],
    ) -> Result<Bundle<Authorized, V>, Error> {
        signing_keys
            .iter()
            .fold(self.prepare(&mut rng, sighash), |partial, ask| {
                partial.sign(&mut rng, ask)
            })
            .finalize()
    }
}

impl<P: InProgressProofs, V> Bundle<InProgress<P, PartiallyAuthorized>, V> {
    /// Signs this bundle with the given [`redjubjub::SigningKey`].
    ///
    /// This will apply signatures for all notes controlled by this spending key.
    pub fn sign<R: RngCore + CryptoRng>(self, mut rng: R, ask: &SpendAuthorizingKey) -> Self {
        let expected_ak = ask.into();
        let sighash = self.authorization().sigs.sighash;
        self.map_authorization(
            &mut rng,
            |_, proof| proof,
            |_, proof| proof,
            |rng, maybe| match maybe {
                MaybeSigned::SigningParts(parts) if parts.ak == expected_ak => {
                    MaybeSigned::Signature(ask.randomize(&parts.alpha).sign(rng, &sighash))
                }
                s => s,
            },
            |_, partial| partial,
        )
    }

    /// Appends externally computed [`redjubjub::Signature`]s.
    ///
    /// Each signature will be applied to the one input for which it is valid. An error
    /// will be returned if the signature is not valid for any inputs, or if it is valid
    /// for more than one input.
    pub fn append_signatures(
        self,
        signatures: &[redjubjub::Signature<SpendAuth>],
    ) -> Result<Self, Error> {
        signatures.iter().try_fold(self, Self::append_signature)
    }

    fn append_signature(self, signature: &redjubjub::Signature<SpendAuth>) -> Result<Self, Error> {
        let sighash = self.authorization().sigs.sighash;
        let mut signature_valid_for = 0usize;
        let bundle = self.map_authorization(
            &mut signature_valid_for,
            |_, proof| proof,
            |_, proof| proof,
            |ctx, maybe| match maybe {
                MaybeSigned::SigningParts(parts) => {
                    let rk = parts.ak.randomize(&parts.alpha);
                    if rk.verify(&sighash, signature).is_ok() {
                        **ctx += 1;
                        MaybeSigned::Signature(*signature)
                    } else {
                        // Signature isn't for this input.
                        MaybeSigned::SigningParts(parts)
                    }
                }
                s => s,
            },
            |_, partial| partial,
        );
        match signature_valid_for {
            0 => Err(Error::InvalidExternalSignature),
            1 => Ok(bundle),
            _ => Err(Error::DuplicateSignature),
        }
    }
}

impl<V> Bundle<InProgress<Proven, PartiallyAuthorized>, V> {
    /// Finalizes this bundle, enabling it to be included in a transaction.
    ///
    /// Returns an error if any signatures are missing.
    pub fn finalize(self) -> Result<Bundle<Authorized, V>, Error> {
        self.try_map_authorization(
            (),
            |_, v| Ok(v),
            |_, v| Ok(v),
            |_, maybe: MaybeSigned| maybe.finalize(),
            |_, partial: InProgress<Proven, PartiallyAuthorized>| {
                Ok(Authorized {
                    binding_sig: partial.sigs.binding_signature,
                })
            },
        )
    }
}

#[cfg(any(test, feature = "test-dependencies"))]
pub(crate) mod testing {
    use std::fmt;

    use proptest::collection::vec;
    use proptest::prelude::*;
    use rand::{rngs::StdRng, SeedableRng};

    use crate::{
        bundle::{Authorized, Bundle},
        note_encryption::Zip212Enforcement,
        prover::mock::{MockOutputProver, MockSpendProver},
        testing::{arb_node, arb_note},
        value::testing::arb_positive_note_value,
        zip32::testing::arb_extended_spending_key,
        Anchor, Node,
    };
    use incrementalmerkletree::{
        frontier::testing::arb_commitment_tree, witness::IncrementalWitness,
    };

    use super::{Builder, BundleType};

    #[allow(dead_code)]
    fn arb_bundle<V: fmt::Debug + From<i64>>(
        max_money: u64,
        zip212_enforcement: Zip212Enforcement,
    ) -> impl Strategy<Value = Bundle<Authorized, V>> {
        (1..30usize)
            .prop_flat_map(move |n_notes| {
                (
                    arb_extended_spending_key(),
                    vec(
                        arb_positive_note_value(max_money / 10000).prop_flat_map(arb_note),
                        n_notes,
                    ),
                    vec(
                        arb_commitment_tree::<_, _, 32>(n_notes, arb_node())
                            .prop_map(|t| IncrementalWitness::from_tree(t).path().unwrap()),
                        n_notes,
                    ),
                    prop::array::uniform32(any::<u8>()),
                    prop::array::uniform32(any::<u8>()),
                )
            })
            .prop_map(
                move |(extsk, spendable_notes, commitment_trees, rng_seed, fake_sighash_bytes)| {
                    let anchor = spendable_notes
                        .first()
                        .zip(commitment_trees.first())
                        .map_or_else(Anchor::empty_tree, |(note, tree)| {
                            let node = Node::from_cmu(&note.cmu());
                            Anchor::from(*tree.root(node).inner())
                        });
                    let mut builder = Builder::new(zip212_enforcement, BundleType::DEFAULT, anchor);
                    let mut rng = StdRng::from_seed(rng_seed);

                    for (note, path) in spendable_notes
                        .into_iter()
                        .zip(commitment_trees.into_iter())
                    {
                        builder.add_spend(&extsk, note, path).unwrap();
                    }

                    let (bundle, _) = builder
                        .build::<MockSpendProver, MockOutputProver, _, _>(&mut rng)
                        .unwrap()
                        .unwrap();

                    let bundle =
                        bundle.create_proofs(&MockSpendProver, &MockOutputProver, &mut rng, ());

                    bundle
                        .apply_signatures(&mut rng, fake_sighash_bytes, &[extsk.expsk.ask])
                        .unwrap()
                },
            )
    }
}
