use bellman::groth16::{verify_proof, Proof};
use bls12_381::Bls12;
use redjubjub::{Binding, SpendAuth};

use super::SaplingVerificationContextInner;
use crate::{
    circuit::{PreparedOutputVerifyingKey, PreparedSpendVerifyingKey},
    note::ExtractedNoteCommitment,
    value::ValueCommitment,
};

/// A context object for verifying the Sapling components of a single Zcash transaction.
pub struct SaplingVerificationContext {
    inner: SaplingVerificationContextInner,
}

impl SaplingVerificationContext {
    /// Construct a new context to be used with a single transaction.
    #[allow(clippy::new_without_default)]
    pub fn new() -> Self {
        SaplingVerificationContext {
            inner: SaplingVerificationContextInner::new(),
        }
    }

    /// Perform consensus checks on a Sapling SpendDescription, while
    /// accumulating its value commitment inside the context for later use.
    #[allow(clippy::too_many_arguments)]
    pub fn check_spend(
        &mut self,
        cv: &ValueCommitment,
        anchor: bls12_381::Scalar,
        nullifier: &[u8; 32],
        rk: redjubjub::VerificationKey<SpendAuth>,
        sighash_value: &[u8; 32],
        spend_auth_sig: redjubjub::Signature<SpendAuth>,
        zkproof: Proof<Bls12>,
        verifying_key: &PreparedSpendVerifyingKey,
    ) -> bool {
        self.inner.check_spend(
            cv,
            anchor,
            nullifier,
            &rk,
            zkproof,
            &mut (),
            |_, rk| rk.verify(sighash_value, &spend_auth_sig).is_ok(),
            |_, proof, public_inputs| {
                verify_proof(&verifying_key.0, &proof, &public_inputs[..]).is_ok()
            },
        )
    }

    /// Perform consensus checks on a Sapling OutputDescription, while
    /// accumulating its value commitment inside the context for later use.
    pub fn check_output(
        &mut self,
        cv: &ValueCommitment,
        cmu: ExtractedNoteCommitment,
        epk: jubjub::ExtendedPoint,
        zkproof: Proof<Bls12>,
        verifying_key: &PreparedOutputVerifyingKey,
    ) -> bool {
        self.inner
            .check_output(cv, cmu, epk, zkproof, |proof, public_inputs| {
                verify_proof(&verifying_key.0, &proof, &public_inputs[..]).is_ok()
            })
    }

    /// Perform consensus checks on the valueBalance and bindingSig parts of a
    /// Sapling transaction. All SpendDescriptions and OutputDescriptions must
    /// have been checked before calling this function.
    pub fn final_check<V: Into<i64>>(
        &self,
        value_balance: V,
        sighash_value: &[u8; 32],
        binding_sig: redjubjub::Signature<Binding>,
    ) -> bool {
        self.inner.final_check(value_balance, |bvk| {
            bvk.verify(sighash_value, &binding_sig).is_ok()
        })
    }
}
