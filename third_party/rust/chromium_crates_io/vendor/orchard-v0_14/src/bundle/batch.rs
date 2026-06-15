use alloc::vec::Vec;

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

/// Batch validation context for Orchard.
///
/// This batch-validates proofs and RedPallas signatures.
#[derive(Debug, Default)]
pub struct BatchValidator {
    proofs: plonk::BatchVerifier<vesta::Affine>,
    signatures: Vec<BundleSignature>,
}

impl BatchValidator {
    /// Constructs a new batch validation context.
    pub fn new() -> Self {
        BatchValidator {
            proofs: plonk::BatchVerifier::new(),
            signatures: vec![],
        }
    }

    /// Adds the proof and RedPallas signatures from the given bundle to the validator.
    pub fn add_bundle<V: Copy + Into<i64>>(
        &mut self,
        bundle: &Bundle<Authorized, V>,
        sighash: [u8; 32],
    ) {
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
    }

    /// Batch-validates the accumulated bundles.
    ///
    /// Returns `true` if every proof and signature in every bundle added to the batch
    /// validator is valid, or `false` if one or more are invalid. No attempt is made to
    /// figure out which of the accumulated bundles might be invalid; if that information
    /// is desired, construct separate [`BatchValidator`]s for sub-batches of the bundles.
    pub fn validate<R: RngCore + CryptoRng>(self, vk: &VerifyingKey, rng: R) -> bool {
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
            Ok(()) => self.proofs.finalize(&vk.params, &vk.vk),
            Err(e) => {
                debug!("RedPallas batch validation failed: {}", e);
                false
            }
        }
    }
}
