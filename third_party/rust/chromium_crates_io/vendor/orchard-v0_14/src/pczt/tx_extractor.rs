use core::fmt;

use nonempty::NonEmpty;
use rand::{CryptoRng, RngCore};

use super::Action;
use crate::{
    bundle::{Authorization, Authorized, EffectsOnly},
    primitives::redpallas::{self, Binding, SpendAuth},
    Proof,
};

impl super::Bundle {
    /// Extracts the effects of this PCZT bundle as a [regular `Bundle`].
    ///
    /// This is used by the Signer role to produce the transaction sighash.
    ///
    /// [regular `Bundle`]: crate::Bundle
    pub fn extract_effects<V: TryFrom<i64>>(
        &self,
    ) -> Result<Option<crate::Bundle<EffectsOnly, V>>, TxExtractorError> {
        self.to_tx_data(|_| Ok(()), |_| Ok(EffectsOnly))
    }

    /// Extracts a fully authorized [regular `Bundle`] from this PCZT bundle.
    ///
    /// This is used by the Transaction Extractor role to produce the final transaction.
    ///
    /// [regular `Bundle`]: crate::Bundle
    pub fn extract<V: TryFrom<i64>>(
        &self,
    ) -> Result<Option<crate::Bundle<Unbound, V>>, TxExtractorError> {
        let bundle = self.to_tx_data(
            |action| {
                action
                    .spend
                    .spend_auth_sig
                    .clone()
                    .ok_or(TxExtractorError::MissingSpendAuthSig)
            },
            |bundle| {
                Ok(Unbound {
                    proof: bundle
                        .zkproof
                        .clone()
                        .ok_or(TxExtractorError::MissingProof)?,
                    bsk: bundle
                        .bsk
                        .clone()
                        .ok_or(TxExtractorError::MissingBindingSignatureSigningKey)?,
                })
            },
        )?;

        // The proof comes straight from the (untrusted) PCZT, so reject it here if it is not
        // the canonical size. This makes "an `Authorized` bundle always has a canonical proof"
        // hold across the `Unbound` -> `Authorized` transition in `apply_binding_signature`.
        if let Some(bundle) = &bundle {
            crate::bundle::validate_proof_size(
                &bundle.authorization().proof,
                bundle.actions().len(),
            )?;
        }

        Ok(bundle)
    }

    /// Converts this PCZT bundle into a regular bundle with the given authorizations.
    fn to_tx_data<A, V, E, F, G>(
        &self,
        action_auth: F,
        bundle_auth: G,
    ) -> Result<Option<crate::Bundle<A, V>>, E>
    where
        A: Authorization,
        E: From<TxExtractorError>,
        F: Fn(&Action) -> Result<<A as Authorization>::SpendAuth, E>,
        G: FnOnce(&Self) -> Result<A, E>,
        V: TryFrom<i64>,
    {
        let actions = self
            .actions
            .iter()
            .map(|action| {
                let authorization = action_auth(action)?;

                crate::Action::from_parts(
                    action.spend.nullifier,
                    action.spend.rk.clone(),
                    action.output.cmx,
                    action.output.encrypted_note.clone(),
                    action.cv_net.clone(),
                    authorization,
                )
                .map_err(|e| TxExtractorError::from(e).into())
            })
            .collect::<Result<_, E>>()?;

        Ok(if let Some(actions) = NonEmpty::from_vec(actions) {
            let value_balance = i64::try_from(self.value_sum)
                .ok()
                .and_then(|v| v.try_into().ok())
                .ok_or(TxExtractorError::ValueSumOutOfRange)?;

            let authorization = bundle_auth(self)?;

            Some(crate::Bundle::from_parts_unchecked(
                actions,
                self.flags,
                value_balance,
                self.anchor,
                authorization,
            ))
        } else {
            None
        })
    }
}

/// Errors that can occur while extracting a regular Orchard bundle from a PCZT bundle.
#[derive(Debug)]
#[non_exhaustive]
pub enum TxExtractorError {
    /// The Transaction Extractor role requires `bsk` to be set.
    MissingBindingSignatureSigningKey,
    /// The Transaction Extractor role requires `zkproof` to be set.
    MissingProof,
    /// The Transaction Extractor role requires all `spend_auth_sig` fields to be set.
    MissingSpendAuthSig,
    /// The value sum does not fit into a `valueBalance`.
    ValueSumOutOfRange,
    /// An action has an identity `rk`, which is forbidden by the consensus
    /// rule introduced in zcashd v6.12.1 and Zebra 4.3.1.
    IdentityRk,
    /// An action has an `epk` that does not encode a non-identity Pallas point.
    InvalidEpk,
    /// The `zkproof` does not have the canonical length for the bundle's number of actions.
    NonCanonicalProofSize {
        /// The canonical proof length for the bundle's number of actions.
        expected: usize,
        /// The length of the proof that was provided.
        actual: usize,
    },
}

impl From<crate::ActionFromPartsError> for TxExtractorError {
    fn from(e: crate::ActionFromPartsError) -> Self {
        match e {
            crate::ActionFromPartsError::IdentityRk => TxExtractorError::IdentityRk,
            crate::ActionFromPartsError::InvalidEpk => TxExtractorError::InvalidEpk,
        }
    }
}

impl From<crate::bundle::BundleError> for TxExtractorError {
    fn from(e: crate::bundle::BundleError) -> Self {
        match e {
            crate::bundle::BundleError::NonCanonicalProofSize { expected, actual } => {
                TxExtractorError::NonCanonicalProofSize { expected, actual }
            }
        }
    }
}

impl fmt::Display for TxExtractorError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            TxExtractorError::MissingBindingSignatureSigningKey => {
                write!(f, "`bsk` must be set for the Transaction Extractor role")
            }
            TxExtractorError::MissingProof => write!(
                f,
                "Orchard `zkproof` must be set for the Transaction Extractor role"
            ),
            TxExtractorError::MissingSpendAuthSig => write!(
                f,
                "`spend_auth_sig` fields must all be set for the Transaction Extractor role"
            ),
            TxExtractorError::ValueSumOutOfRange => {
                write!(f, "value sum does not fit into a `valueBalance`")
            }
            TxExtractorError::IdentityRk => {
                write!(f, "an Orchard action with identity `rk` is not valid")
            }
            TxExtractorError::InvalidEpk => write!(
                f,
                "an Orchard action's `epk` is not a valid non-identity Pallas point"
            ),
            TxExtractorError::NonCanonicalProofSize { expected, actual } => write!(
                f,
                "Orchard `zkproof` has non-canonical length {actual}; expected {expected} bytes",
            ),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for TxExtractorError {}

/// Authorizing data for a bundle of actions that is just missing a binding signature.
#[derive(Debug)]
pub struct Unbound {
    proof: Proof,
    bsk: redpallas::SigningKey<Binding>,
}

impl Authorization for Unbound {
    type SpendAuth = redpallas::Signature<SpendAuth>;
}

impl<V> crate::Bundle<Unbound, V> {
    /// Verifies the given sighash with every `spend_auth_sig`, and then binds the bundle.
    ///
    /// Returns `None` if the given sighash does not validate against every `spend_auth_sig`.
    pub fn apply_binding_signature<R: RngCore + CryptoRng>(
        self,
        sighash: [u8; 32],
        rng: R,
    ) -> Option<crate::Bundle<Authorized, V>> {
        if self
            .actions()
            .iter()
            .all(|action| action.rk().verify(&sighash, action.authorization()).is_ok())
        {
            Some(self.map_authorization(
                &mut (),
                |_, _, a| a,
                |_, Unbound { proof, bsk }| Authorized::from_parts(proof, bsk.sign(rng, &sighash)),
            ))
        } else {
            None
        }
    }
}
