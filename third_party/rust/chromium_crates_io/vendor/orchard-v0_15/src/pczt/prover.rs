use core::fmt;

use alloc::vec::Vec;

use halo2_proofs::plonk;
use rand::{CryptoRng, RngCore};

use crate::{
    builder::SpendInfo,
    circuit::{Circuit, Instance, ProvingKey},
    note::Rho,
    Note, Proof,
};

impl super::Bundle {
    /// Adds a proof to this PCZT bundle.
    ///
    /// The Action circuits are built for `pk`'s circuit version; the caller selects the
    /// proving key matching the transaction format the PCZT targets. If the PCZT
    /// bundle disables cross-address transfers, the key must be an
    /// [`OrchardCircuitVersion::PostNu6_3`] proving key.
    ///
    /// # Errors
    ///
    /// Returns [`ProverError::DisallowedCrossAddressTransfer`] if the bundle
    /// disables cross-address transfers, and any action's output
    /// is addressed differently than its spent note.
    ///
    /// Returns [`ProverError::ProofFailed`] containing
    /// [`plonk::Error::InvalidInstances`] if the bundle disables
    /// cross-address transfers, and `pk` is not an
    /// [`OrchardCircuitVersion::PostNu6_3`] proving key.
    ///
    /// Also returns an error if required Prover-role fields are missing or invalid,
    /// or if proof creation fails.
    ///
    /// [`OrchardCircuitVersion::PostNu6_3`]: crate::circuit::OrchardCircuitVersion::PostNu6_3
    pub fn create_proof<R: RngCore + CryptoRng>(
        &mut self,
        pk: &ProvingKey,
        rng: R,
    ) -> Result<(), ProverError> {
        // If we have no actions, we don't need a proof (and if we still have no actions
        // by the time we come to transaction extraction, we will end up with a `None`
        // bundle that doesn't even hold a proof field).
        if self.actions.is_empty() {
            return Ok(());
        }

        // Check the restriction structurally before synthesizing any circuit, for a
        // clear error instead of an unsatisfiable-constraint failure.
        self.verify_cross_address_restriction()
            .map_err(|e| match e {
                super::VerifyError::MissingRecipient => ProverError::MissingRecipient,
                // `e` will normally be `VerifyError::DisallowedCrossAddressTransfer`,
                // but `VerifyError` is `#[non_exhaustive]`. Any other error returned
                // by `verify_cross_address_restriction` would by definition disallow
                // a cross-address transfer.
                e => ProverError::DisallowedCrossAddressTransfer(e),
            })?;

        let circuits = self
            .actions
            .iter()
            .map(|action| {
                let fvk = action
                    .spend
                    .fvk
                    .clone()
                    .ok_or(ProverError::MissingFullViewingKey)?;

                let note = Note::from_parts(
                    action
                        .spend
                        .recipient
                        .ok_or(ProverError::MissingRecipient)?,
                    action.spend.value.ok_or(ProverError::MissingValue)?,
                    action.spend.rho.ok_or(ProverError::MissingRho)?,
                    action.spend.rseed.ok_or(ProverError::MissingRandomSeed)?,
                    action.spend.note_version,
                )
                .into_option()
                .ok_or(ProverError::InvalidSpendNote)?;

                let merkle_path = action
                    .spend
                    .witness
                    .clone()
                    .ok_or(ProverError::MissingWitness)?;

                let spend =
                    SpendInfo::new(fvk, note, merkle_path).ok_or(ProverError::WrongFvkForNote)?;

                let output_note = Note::from_parts(
                    action
                        .output
                        .recipient
                        .ok_or(ProverError::MissingRecipient)?,
                    action.output.value.ok_or(ProverError::MissingValue)?,
                    Rho::from_nf_old(action.spend.nullifier),
                    action.output.rseed.ok_or(ProverError::MissingRandomSeed)?,
                    action.output.note_version,
                )
                .into_option()
                .ok_or(ProverError::InvalidOutputNote)?;

                let alpha = action
                    .spend
                    .alpha
                    .ok_or(ProverError::MissingSpendAuthRandomizer)?;
                let rcv = action
                    .rcv
                    .clone()
                    .ok_or(ProverError::MissingValueCommitTrapdoor)?;

                Circuit::from_action_context(spend, output_note, alpha, rcv, pk.circuit_version())
                    .ok_or(ProverError::RhoMismatch)
            })
            .collect::<Result<Vec<_>, ProverError>>()?;

        let instances = self
            .actions
            .iter()
            .map(|action| {
                Instance::from_parts(
                    self.anchor,
                    action.cv_net.clone(),
                    action.spend.nullifier,
                    action.spend.rk.clone(),
                    action.output.cmx,
                    self.flags,
                )
                .ok_or(ProverError::IdentityRk)
            })
            .collect::<Result<Vec<_>, ProverError>>()?;

        let proof =
            Proof::create(pk, &circuits, &instances, rng).map_err(ProverError::ProofFailed)?;

        self.zkproof = Some(proof);

        Ok(())
    }
}

/// Errors that can occur while creating Orchard proofs for a PCZT.
#[derive(Debug)]
#[non_exhaustive]
pub enum ProverError {
    /// An action's output is addressed differently than its spent note, but the bundle's pool
    /// restrictions disable cross-address transfers.
    DisallowedCrossAddressTransfer(super::VerifyError),
    /// The output note's components do not produce a valid note commitment.
    InvalidOutputNote,
    /// The spent note's components do not produce a valid note commitment.
    InvalidSpendNote,
    /// The Prover role requires `fvk` to be set.
    MissingFullViewingKey,
    /// The Prover role requires all `rseed` fields to be set.
    MissingRandomSeed,
    /// The Prover role requires all `recipient` fields to be set.
    MissingRecipient,
    /// The Prover role requires `rho` to be set.
    MissingRho,
    /// The Prover role requires `alpha` to be set.
    MissingSpendAuthRandomizer,
    /// The Prover role requires all `value` fields to be set.
    MissingValue,
    /// The Prover role requires `rcv` to be set.
    MissingValueCommitTrapdoor,
    /// The Prover role requires `witness` to be set.
    MissingWitness,
    /// An error occurred while creating the proof.
    ProofFailed(plonk::Error),
    /// The `rho` of the `output_note` is not equal to the nullifier of the spent note.
    RhoMismatch,
    /// An action has an identity `rk`, which is forbidden by the consensus
    /// rule introduced in zcashd v6.12.1 and Zebra 4.3.1.
    IdentityRk,
    /// The provided `fvk` does not own the spent note.
    WrongFvkForNote,
}

impl fmt::Display for ProverError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ProverError::DisallowedCrossAddressTransfer(e) => match e {
                super::VerifyError::DisallowedCrossAddressTransfer => write!(
                    f,
                    "an action outputs to a different expanded receiver than it spends from, but the \
                     bundle disables cross-address transfers"
                ),
                e => write!(f, "cross-address restriction verification failed: {e}"),
            },
            ProverError::InvalidOutputNote => write!(f, "output note is invalid"),
            ProverError::InvalidSpendNote => write!(f, "spent note is invalid"),
            ProverError::MissingFullViewingKey => {
                write!(f, "`fvk` must be set for the Prover role")
            }
            ProverError::MissingRandomSeed => {
                write!(f, "`rseed` fields must be set for the Prover role")
            }
            ProverError::MissingRecipient => {
                write!(f, "`recipient` fields must be set for the Prover role")
            }
            ProverError::MissingRho => write!(f, "`rho` must be set for the Prover role"),
            ProverError::MissingSpendAuthRandomizer => {
                write!(f, "`alpha` must be set for the Prover role")
            }
            ProverError::MissingValue => {
                write!(f, "`value` fields must be set for the Prover role")
            }
            ProverError::MissingValueCommitTrapdoor => {
                write!(f, "`rcv` must be set for the Prover role")
            }
            ProverError::MissingWitness => write!(f, "`witness` must be set for the Prover role"),
            ProverError::ProofFailed(halo2_proofs::plonk::Error::InvalidInstances) => {
                write!(
                    f,
                    "Failed to create proof: provided instances do not match the circuit, or \
                     the cross-address restriction is not supported by the proving key's \
                     circuit version",
                )
            }
            ProverError::ProofFailed(e) => write!(f, "Failed to create proof: {e}"),
            ProverError::RhoMismatch => {
                write!(f, "output's `rho` does not match spent note's nullifier")
            }
            ProverError::IdentityRk => {
                write!(f, "an Orchard action with identity `rk` is not valid")
            }
            ProverError::WrongFvkForNote => write!(f, "`fvk` does not own the action's spent note"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ProverError {}
