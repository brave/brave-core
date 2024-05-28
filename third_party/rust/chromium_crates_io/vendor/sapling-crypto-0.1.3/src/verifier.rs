use bellman::{gadgets::multipack, groth16::Proof};
use bls12_381::Bls12;
use group::{ff::PrimeField, Curve};
use redjubjub::{Binding, SpendAuth};

use crate::{
    note::ExtractedNoteCommitment,
    value::{CommitmentSum, ValueCommitment},
};

mod single;
pub use single::SaplingVerificationContext;

mod batch;
pub use batch::BatchValidator;

/// A context object for verifying the Sapling components of a Zcash transaction.
struct SaplingVerificationContextInner {
    // (sum of the Spend value commitments) - (sum of the Output value commitments)
    cv_sum: CommitmentSum,
}

impl SaplingVerificationContextInner {
    /// Construct a new context to be used with a single transaction.
    fn new() -> Self {
        SaplingVerificationContextInner {
            cv_sum: CommitmentSum::zero(),
        }
    }

    /// Perform consensus checks on a Sapling SpendDescription, while
    /// accumulating its value commitment inside the context for later use.
    #[allow(clippy::too_many_arguments)]
    fn check_spend<C>(
        &mut self,
        cv: &ValueCommitment,
        anchor: bls12_381::Scalar,
        nullifier: &[u8; 32],
        rk: &redjubjub::VerificationKey<SpendAuth>,
        zkproof: Proof<Bls12>,
        verifier_ctx: &mut C,
        spend_auth_sig_verifier: impl FnOnce(&mut C, &redjubjub::VerificationKey<SpendAuth>) -> bool,
        proof_verifier: impl FnOnce(&mut C, Proof<Bls12>, [bls12_381::Scalar; 7]) -> bool,
    ) -> bool {
        // The "cv is not small order" happens when a SpendDescription is deserialized.
        // This happens when transactions or blocks are received over the network, or when
        // mined blocks are introduced via the `submitblock` RPC method on full nodes.
        let rk_affine = jubjub::AffinePoint::from_bytes((*rk).into()).unwrap();
        if rk_affine.is_small_order().into() {
            return false;
        }

        // Accumulate the value commitment in the context
        self.cv_sum += cv;

        // Grab the nullifier as a sequence of bytes
        let nullifier = &nullifier[..];

        // Verify the spend_auth_sig
        if !spend_auth_sig_verifier(verifier_ctx, rk) {
            return false;
        }

        // Construct public input for circuit
        let mut public_input = [bls12_381::Scalar::zero(); 7];
        {
            let affine = rk_affine;
            let (u, v) = (affine.get_u(), affine.get_v());
            public_input[0] = u;
            public_input[1] = v;
        }
        {
            let affine = cv.as_inner().to_affine();
            let (u, v) = (affine.get_u(), affine.get_v());
            public_input[2] = u;
            public_input[3] = v;
        }
        public_input[4] = anchor;

        // Add the nullifier through multiscalar packing
        {
            let nullifier = multipack::bytes_to_bits_le(nullifier);
            let nullifier = multipack::compute_multipacking(&nullifier);

            assert_eq!(nullifier.len(), 2);

            public_input[5] = nullifier[0];
            public_input[6] = nullifier[1];
        }

        // Verify the proof
        proof_verifier(verifier_ctx, zkproof, public_input)
    }

    /// Perform consensus checks on a Sapling OutputDescription, while
    /// accumulating its value commitment inside the context for later use.
    fn check_output(
        &mut self,
        cv: &ValueCommitment,
        cmu: ExtractedNoteCommitment,
        epk: jubjub::ExtendedPoint,
        zkproof: Proof<Bls12>,
        proof_verifier: impl FnOnce(Proof<Bls12>, [bls12_381::Scalar; 5]) -> bool,
    ) -> bool {
        // The "cv is not small order" happens when an OutputDescription is deserialized.
        // This happens when transactions or blocks are received over the network, or when
        // mined blocks are introduced via the `submitblock` RPC method on full nodes.
        if epk.is_small_order().into() {
            return false;
        }

        // Accumulate the value commitment in the context
        self.cv_sum -= cv;

        // Construct public input for circuit
        let mut public_input = [bls12_381::Scalar::zero(); 5];
        {
            let affine = cv.as_inner().to_affine();
            let (u, v) = (affine.get_u(), affine.get_v());
            public_input[0] = u;
            public_input[1] = v;
        }
        {
            let affine = epk.to_affine();
            let (u, v) = (affine.get_u(), affine.get_v());
            public_input[2] = u;
            public_input[3] = v;
        }
        public_input[4] = bls12_381::Scalar::from_repr(cmu.to_bytes()).unwrap();

        // Verify the proof
        proof_verifier(zkproof, public_input)
    }

    /// Perform consensus checks on the valueBalance and bindingSig parts of a
    /// Sapling transaction. All SpendDescriptions and OutputDescriptions must
    /// have been checked before calling this function.
    fn final_check<V: Into<i64>>(
        &self,
        value_balance: V,
        binding_sig_verifier: impl FnOnce(redjubjub::VerificationKey<Binding>) -> bool,
    ) -> bool {
        // Compute the final bvk.
        let bvk = self.cv_sum.into_bvk(value_balance);

        // Verify the binding_sig
        binding_sig_verifier(bvk)
    }
}
