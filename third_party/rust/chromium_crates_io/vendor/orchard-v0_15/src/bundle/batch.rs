use alloc::vec::Vec;
use core::fmt;

use halo2_proofs::plonk;
use pasta_curves::vesta;
use rand::{CryptoRng, RngCore};
use tracing::debug;

use super::{Authorized, Bundle};
use crate::{
    circuit::VerifyingKey,
    primitives::redpallas::{self, Binding, SpendAuth},
};

/// A signature within an authorized Orchard bundle.
#[derive(Debug)]
struct BundleSignature {
    /// The signature item for validation.
    signature: redpallas::batch::Item<SpendAuth, Binding>,
}

/// Error returned by [`BatchValidator::add_bundle`] when a bundle's flags require a circuit
/// capability the validator's verifying key does not provide.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum BatchError {
    /// The bundle disables cross-address transfers, but the validator's verifying key's
    /// circuit version does not constrain the cross-address restriction (see
    /// [`OrchardCircuitVersion::supports_cross_address_restriction`]).
    ///
    /// [`OrchardCircuitVersion::supports_cross_address_restriction`]: crate::circuit::OrchardCircuitVersion::supports_cross_address_restriction
    RestrictionUnsupportedByKey,
}

impl fmt::Display for BatchError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            BatchError::RestrictionUnsupportedByKey => write!(
                f,
                "bundle disables cross-address transfers, but the verifying key's circuit \
                 version does not constrain the cross-address restriction",
            ),
        }
    }
}

impl core::error::Error for BatchError {}

/// Batch validation context for Orchard.
///
/// This batch-validates proofs and RedPallas signatures. The verifying key is bound at
/// construction: every bundle added to the batch is validated against it, and
/// [`Self::add_bundle`] rejects up front any bundle whose flags the key's circuit version
/// cannot enforce.
#[derive(Debug)]
pub struct BatchValidator<'a> {
    proofs: plonk::BatchVerifier<vesta::Affine>,
    signatures: Vec<BundleSignature>,
    /// The verifying key every queued bundle is validated against, and which
    /// [`Self::validate`] finalizes the proof batch with.
    vk: &'a VerifyingKey,
}

impl<'a> BatchValidator<'a> {
    /// Constructs a new batch validation context that validates against `vk`.
    pub fn new(vk: &'a VerifyingKey) -> Self {
        BatchValidator {
            proofs: plonk::BatchVerifier::new(),
            signatures: vec![],
            vk,
        }
    }

    /// Adds the proof and RedPallas signatures from the given bundle to the validator.
    ///
    /// Returns [`BatchError::RestrictionUnsupportedByKey`] if the bundle disables cross-address
    /// transfers but the validator's verifying key's circuit version does not support the
    /// cross-address restriction; in that case the bundle is not added to the batch.
    pub fn add_bundle<V: Copy + Into<i64>>(
        &mut self,
        bundle: &Bundle<Authorized, V>,
        sighash: [u8; 32],
    ) -> Result<(), BatchError> {
        if !bundle.flags().cross_address_enabled() && !self.vk.supports_cross_address_restriction()
        {
            return Err(BatchError::RestrictionUnsupportedByKey);
        }

        for action in bundle.actions().iter() {
            self.signatures.push(BundleSignature {
                signature: action
                    .rk()
                    .create_batch_item(action.authorization().clone(), &sighash),
            });
        }

        self.signatures.push(BundleSignature {
            signature: bundle
                .binding_validating_key()
                .create_batch_item(bundle.authorization().binding_signature().clone(), &sighash),
        });

        bundle
            .authorization()
            .proof()
            .add_to_batch(&mut self.proofs, bundle.to_instances());

        Ok(())
    }

    /// Batch-validates the accumulated bundles.
    ///
    /// Returns `true` if every proof and signature in every bundle added to the batch
    /// validator is valid. Returns `false` if one or more proofs or signatures are
    /// invalid. No attempt is made to figure out which of the accumulated bundles might
    /// be invalid; if that information is desired, construct separate [`BatchValidator`]s
    /// for sub-batches of the bundles.
    ///
    /// The cross-address-restriction capability is enforced when bundles are added (see
    /// [`Self::add_bundle`]), so it is already guaranteed here.
    pub fn validate<R: RngCore + CryptoRng>(self, rng: R) -> bool {
        // https://p.z.cash/TCR:bad-txns-orchard-binding-signature-invalid?partial

        if self.signatures.is_empty() {
            // An empty batch is always valid, but is not free to run; skip it.
            // Note that a transaction has at least a binding signature, so if
            // there are no signatures, there are also no proofs.
            return true;
        }

        let mut validator = redpallas::batch::Verifier::new();
        for sig in self.signatures.iter() {
            validator.queue(sig.signature.clone());
        }

        match validator.verify(rng) {
            // If signatures are valid, check the proofs.
            Ok(()) => self.proofs.finalize(&self.vk.params, &self.vk.vk),
            Err(e) => {
                debug!("RedPallas batch validation failed: {}", e);
                false
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use rand::rngs::OsRng;

    use super::{BatchError, BatchValidator};
    use crate::{
        bundle::tests::{sample_authorized_bundle, with_cross_address_disabled},
        circuit::{OrchardCircuitVersion, VerifyingKey},
    };

    #[test]
    fn add_bundle_rejects_unsupported_cross_address_restriction() {
        let bundle = with_cross_address_disabled(sample_authorized_bundle(1))
            .try_map_value_balance(i64::try_from)
            .expect("generated bundle value balance fits in i64");

        // Keys whose circuit version cannot constrain the cross-address restriction reject
        // the bundle at insertion, so it never enters the batch.
        for circuit_version in [
            OrchardCircuitVersion::InsecurePreNu6_2,
            OrchardCircuitVersion::FixedPostNu6_2,
        ] {
            let vk = VerifyingKey::build(circuit_version);
            let mut validator = BatchValidator::new(&vk);
            assert_eq!(
                validator.add_bundle(&bundle, [0; 32]),
                Err(BatchError::RestrictionUnsupportedByKey)
            );
        }

        // The post-NU 6.3 key supports the restriction, so the bundle is accepted.
        let vk = VerifyingKey::build(OrchardCircuitVersion::PostNu6_3);
        let mut validator = BatchValidator::new(&vk);
        assert_eq!(validator.add_bundle(&bundle, [0; 32]), Ok(()));
    }

    #[test]
    fn empty_batch_validates() {
        for circuit_version in [
            OrchardCircuitVersion::InsecurePreNu6_2,
            OrchardCircuitVersion::FixedPostNu6_2,
            OrchardCircuitVersion::PostNu6_3,
        ] {
            let vk = VerifyingKey::build(circuit_version);
            assert!(BatchValidator::new(&vk).validate(OsRng));
        }
    }
}
