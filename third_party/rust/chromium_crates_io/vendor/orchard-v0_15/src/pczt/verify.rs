use core::fmt;

use crate::{
    keys::{FullViewingKey, SpendValidatingKey},
    note::{ExtractedNoteCommitment, Rho},
    value::ValueCommitment,
    Note,
};

impl super::Bundle {
    /// If this bundle disables cross-address transfers, verifies that every action's
    /// output is addressed to the same expanded receiver (`(g_d, pk_d)`) as its spent
    /// note. This is a no-op for bundles that permit cross-address transfers.
    ///
    /// When the restriction applies, it requires `spend.recipient` and `output.recipient`
    /// to be set on every action. Signers should always call this before signing. The
    /// equivalent structural checks are also performed by [`Bundle::finalize_io`] and
    /// `Bundle::create_proof`.
    ///
    /// The post-NU6.3 circuit supports enforcing the restriction; older circuit versions
    /// do not. The prover and verifier APIs reject restricted bundles for those keys.
    /// (That is not a security restriction; for security, the consensus verifier must use
    /// the correct key for the epoch and pool.)
    ///
    /// [`Bundle::finalize_io`]: super::Bundle::finalize_io
    pub fn verify_cross_address_restriction(&self) -> Result<(), VerifyError> {
        if !self.flags.cross_address_enabled() {
            for action in &self.actions {
                let spend_recipient = action
                    .spend
                    .recipient
                    .ok_or(VerifyError::MissingRecipient)?;
                let output_recipient = action
                    .output
                    .recipient
                    .ok_or(VerifyError::MissingRecipient)?;

                if !spend_recipient.same_expanded_receiver(&output_recipient) {
                    return Err(VerifyError::DisallowedCrossAddressTransfer);
                }
            }
        }

        Ok(())
    }
}

impl super::Action {
    /// Verifies that the `cv_net` field is consistent with the note fields.
    ///
    /// Requires that the following optional fields are set:
    /// - `spend.value`
    /// - `output.value`
    /// - `rcv`
    pub fn verify_cv_net(&self) -> Result<(), VerifyError> {
        let spend_value = self.spend().value.ok_or(VerifyError::MissingValue)?;
        let output_value = self.output().value.ok_or(VerifyError::MissingValue)?;
        let rcv = self
            .rcv
            .clone()
            .ok_or(VerifyError::MissingValueCommitTrapdoor)?;

        let cv_net = ValueCommitment::derive(spend_value - output_value, rcv);
        if cv_net.to_bytes() == self.cv_net.to_bytes() {
            Ok(())
        } else {
            Err(VerifyError::InvalidValueCommitment)
        }
    }
}

impl super::Spend {
    /// Returns the [`FullViewingKey`] to use when validating this note.
    ///
    /// Handles dummy notes when the `value` field is set.
    fn fvk_for_validation<'a>(
        &'a self,
        expected_fvk: Option<&'a FullViewingKey>,
    ) -> Result<&'a FullViewingKey, VerifyError> {
        match (expected_fvk, self.fvk.as_ref(), self.value.as_ref()) {
            (Some(expected_fvk), Some(fvk), _) if fvk == expected_fvk => Ok(fvk),
            // `expected_fvk` is ignored if the spent note is a dummy note.
            (Some(_), Some(fvk), Some(value)) if value.inner() == 0 => Ok(fvk),
            (Some(_), Some(_), _) => Err(VerifyError::MismatchedFullViewingKey),
            (Some(expected_fvk), None, _) => Ok(expected_fvk),
            (None, Some(fvk), _) => Ok(fvk),
            (None, None, _) => Err(VerifyError::MissingFullViewingKey),
        }
    }

    /// Verifies that the `nullifier` field is consistent with the note fields.
    ///
    /// Requires that the following optional fields are set:
    /// - `recipient`
    /// - `value`
    /// - `rho`
    /// - `rseed`
    ///
    /// In addition, at least one of the `fvk` field or `expected_fvk` must be provided.
    ///
    /// The provided [`FullViewingKey`] is ignored if the spent note is a dummy note.
    /// Otherwise, it will be checked against the `fvk` field (if both are set).
    pub fn verify_nullifier(
        &self,
        expected_fvk: Option<&FullViewingKey>,
    ) -> Result<(), VerifyError> {
        let fvk = self.fvk_for_validation(expected_fvk)?;

        let note = Note::from_parts(
            self.recipient.ok_or(VerifyError::MissingRecipient)?,
            self.value.ok_or(VerifyError::MissingValue)?,
            self.rho.ok_or(VerifyError::MissingRho)?,
            self.rseed.ok_or(VerifyError::MissingRandomSeed)?,
            self.note_version,
        )
        .into_option()
        .ok_or(VerifyError::InvalidSpendNote)?;

        // We need both the note and the FVK to verify the nullifier; we have everything
        // needed to also verify that the correct FVK was provided (the nullifier check
        // itself only constrains `nk` within the FVK).
        fvk.scope_for_address(&note.recipient())
            .ok_or(VerifyError::WrongFvkForNote)?;

        if note.nullifier(fvk) == self.nullifier {
            Ok(())
        } else {
            Err(VerifyError::InvalidNullifier)
        }
    }

    /// Verifies that the `rk` field is consistent with the given FVK.
    ///
    /// Requires that the following optional fields are set:
    /// - `alpha`
    ///
    /// The provided [`FullViewingKey`] is ignored if the spent note is a dummy note
    /// (which can only be determined if the `value` field is set). Otherwise, it will be
    /// checked against the `fvk` field (if set).
    pub fn verify_rk(&self, expected_fvk: Option<&FullViewingKey>) -> Result<(), VerifyError> {
        let fvk = self.fvk_for_validation(expected_fvk)?;

        let ak = SpendValidatingKey::from(fvk.clone());

        let alpha = self
            .alpha
            .as_ref()
            .ok_or(VerifyError::MissingSpendAuthRandomizer)?;

        if ak.randomize(alpha) == self.rk {
            Ok(())
        } else {
            Err(VerifyError::InvalidRandomizedVerificationKey)
        }
    }
}

impl super::Output {
    /// Verifies that the `cmx` field is consistent with the note fields.
    ///
    /// Requires that the following optional fields are set:
    /// - `recipient`
    /// - `value`
    /// - `rseed`
    ///
    /// `spend` must be the Spend from the same Orchard action.
    pub fn verify_note_commitment(&self, spend: &super::Spend) -> Result<(), VerifyError> {
        let note = Note::from_parts(
            self.recipient.ok_or(VerifyError::MissingRecipient)?,
            self.value.ok_or(VerifyError::MissingValue)?,
            Rho::from_nf_old(spend.nullifier),
            self.rseed.ok_or(VerifyError::MissingRandomSeed)?,
            self.note_version,
        )
        .into_option()
        .ok_or(VerifyError::InvalidOutputNote)?;

        if ExtractedNoteCommitment::from(note.commitment()) == self.cmx {
            Ok(())
        } else {
            Err(VerifyError::InvalidExtractedNoteCommitment)
        }
    }
}

/// Errors that can occur while verifying a PCZT bundle.
#[derive(Debug)]
#[non_exhaustive]
pub enum VerifyError {
    /// An action's output is addressed differently than its spent note, but the bundle's pool
    /// restriction disables cross-address transfers.
    DisallowedCrossAddressTransfer,
    /// The output note's components do not produce the expected `cmx`.
    InvalidExtractedNoteCommitment,
    /// The spent note's components do not produce the expected `nullifier`.
    InvalidNullifier,
    /// The output note's components do not produce a valid note commitment.
    InvalidOutputNote,
    /// The Spend's FVK and `alpha` do not produce the expected `rk`.
    InvalidRandomizedVerificationKey,
    /// The spent note's components do not produce a valid note commitment.
    InvalidSpendNote,
    /// The action's `cv_net` does not match the provided note values and `rcv`.
    InvalidValueCommitment,
    /// The spend or output's `fvk` field does not match the provided FVK.
    MismatchedFullViewingKey,
    /// Dummy notes must have their `fvk` field set in order to be verified.
    MissingFullViewingKey,
    /// `nullifier` verification requires `rseed` to be set.
    MissingRandomSeed,
    /// Verification requires `recipient` to be set.
    MissingRecipient,
    /// `nullifier` verification requires `rho` to be set.
    MissingRho,
    /// `rk` verification requires `alpha` to be set.
    MissingSpendAuthRandomizer,
    /// Verification requires all `value` fields to be set.
    MissingValue,
    /// `cv_net` verification requires `rcv` to be set.
    MissingValueCommitTrapdoor,
    /// The provided `fvk` does not own the spent note.
    WrongFvkForNote,
}

impl fmt::Display for VerifyError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            VerifyError::DisallowedCrossAddressTransfer => write!(
                f,
                "an action outputs to a different expanded receiver than it spends from, but the \
                 bundle disables cross-address transfers"
            ),
            VerifyError::InvalidExtractedNoteCommitment => {
                write!(f, "output note doesn't match `cmx`")
            }
            VerifyError::InvalidNullifier => write!(f, "spent note doesn't match `nullifier`"),
            VerifyError::InvalidOutputNote => write!(f, "invalid output note"),
            VerifyError::InvalidRandomizedVerificationKey => {
                write!(f, "spend's `fvk` and `alpha` do not match `rk`")
            }
            VerifyError::InvalidSpendNote => write!(f, "invalid spent note"),
            VerifyError::InvalidValueCommitment => {
                write!(f, "`cv_net` doesn't match the note values and `rcv`")
            }
            VerifyError::MismatchedFullViewingKey => {
                write!(f, "Provided full viewing key doesn't match the `fvk` field")
            }
            VerifyError::MissingFullViewingKey => write!(f, "`fvk` missing for dummy note"),
            VerifyError::MissingRandomSeed => {
                write!(f, "`rseed` missing for `nullifier` verification")
            }
            VerifyError::MissingRecipient => write!(f, "`recipient` missing for verification"),
            VerifyError::MissingRho => write!(f, "`rho` missing for `nullifier` verification"),
            VerifyError::MissingSpendAuthRandomizer => {
                write!(f, "`alpha` missing for `rk` verification")
            }
            VerifyError::MissingValue => write!(f, "`value` missing"),
            VerifyError::MissingValueCommitTrapdoor => {
                write!(f, "`rcv` missing for `cv_net` verification")
            }
            VerifyError::WrongFvkForNote => write!(f, "`fvk` does not own the action's spent note"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for VerifyError {}
