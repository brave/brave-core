//! Abstractions over the proving system and parameters.

use bellman::groth16::{create_random_proof, Proof};
use bls12_381::Bls12;
use rand_core::RngCore;

use crate::{
    bundle::GrothProofBytes,
    circuit::{self, GROTH_PROOF_SIZE},
    value::{NoteValue, ValueCommitTrapdoor},
    MerklePath,
};

use super::{
    circuit::{Output, OutputParameters, Spend, SpendParameters, ValueCommitmentOpening},
    Diversifier, Note, PaymentAddress, ProofGenerationKey, Rseed,
};

/// Interface for creating Sapling Spend proofs.
pub trait SpendProver {
    /// The proof type created by this prover.
    type Proof;

    /// Prepares an instance of the Sapling Spend circuit for the given inputs.
    ///
    /// Returns `None` if `diversifier` is not a valid Sapling diversifier.
    #[allow(clippy::too_many_arguments)]
    fn prepare_circuit(
        proof_generation_key: ProofGenerationKey,
        diversifier: Diversifier,
        rseed: Rseed,
        value: NoteValue,
        alpha: jubjub::Fr,
        rcv: ValueCommitTrapdoor,
        anchor: bls12_381::Scalar,
        merkle_path: MerklePath,
    ) -> Option<circuit::Spend>;

    /// Create the proof for a Sapling [`SpendDescription`].
    ///
    /// [`SpendDescription`]: crate::bundle::SpendDescription
    fn create_proof<R: RngCore>(&self, circuit: circuit::Spend, rng: &mut R) -> Self::Proof;

    /// Encodes the given Sapling [`SpendDescription`] proof, erasing its type.
    ///
    /// [`SpendDescription`]: crate::bundle::SpendDescription
    fn encode_proof(proof: Self::Proof) -> GrothProofBytes;
}

/// Interface for creating Sapling Output proofs.
pub trait OutputProver {
    /// The proof type created by this prover.
    type Proof;

    /// Prepares an instance of the Sapling Output circuit for the given inputs.
    ///
    /// Returns `None` if `diversifier` is not a valid Sapling diversifier.
    fn prepare_circuit(
        esk: jubjub::Fr,
        payment_address: PaymentAddress,
        rcm: jubjub::Fr,
        value: NoteValue,
        rcv: ValueCommitTrapdoor,
    ) -> circuit::Output;

    /// Create the proof for a Sapling [`OutputDescription`].
    ///
    /// [`OutputDescription`]: crate::bundle::OutputDescription
    fn create_proof<R: RngCore>(&self, circuit: circuit::Output, rng: &mut R) -> Self::Proof;

    /// Encodes the given Sapling [`OutputDescription`] proof, erasing its type.
    ///
    /// [`OutputDescription`]: crate::bundle::OutputDescription
    fn encode_proof(proof: Self::Proof) -> GrothProofBytes;
}

impl SpendProver for SpendParameters {
    type Proof = Proof<Bls12>;

    fn prepare_circuit(
        proof_generation_key: ProofGenerationKey,
        diversifier: Diversifier,
        rseed: Rseed,
        value: NoteValue,
        alpha: jubjub::Fr,
        rcv: ValueCommitTrapdoor,
        anchor: bls12_381::Scalar,
        merkle_path: MerklePath,
    ) -> Option<Spend> {
        // Construct the value commitment
        let value_commitment_opening = ValueCommitmentOpening {
            value,
            randomness: rcv.inner(),
        };

        // Construct the viewing key
        let viewing_key = proof_generation_key.to_viewing_key();

        // Construct the payment address with the viewing key / diversifier
        let payment_address = viewing_key.to_payment_address(diversifier)?;

        let note = Note::from_parts(payment_address, value, rseed);

        // We now have the full witness for our circuit
        let pos: u64 = merkle_path.position().into();
        Some(Spend {
            value_commitment_opening: Some(value_commitment_opening),
            proof_generation_key: Some(proof_generation_key),
            payment_address: Some(payment_address),
            commitment_randomness: Some(note.rcm()),
            ar: Some(alpha),
            auth_path: merkle_path
                .path_elems()
                .iter()
                .enumerate()
                .map(|(i, node)| Some(((*node).into(), pos >> i & 0x1 == 1)))
                .collect(),
            anchor: Some(anchor),
        })
    }

    fn create_proof<R: RngCore>(&self, circuit: Spend, rng: &mut R) -> Self::Proof {
        create_random_proof(circuit, &self.0, rng).expect("proving should not fail")
    }

    fn encode_proof(proof: Self::Proof) -> GrothProofBytes {
        let mut zkproof = [0u8; GROTH_PROOF_SIZE];
        proof
            .write(&mut zkproof[..])
            .expect("should be able to serialize a proof");
        zkproof
    }
}

impl OutputProver for OutputParameters {
    type Proof = Proof<Bls12>;

    fn prepare_circuit(
        esk: jubjub::Fr,
        payment_address: PaymentAddress,
        rcm: jubjub::Fr,
        value: NoteValue,
        rcv: ValueCommitTrapdoor,
    ) -> Output {
        // Construct the value commitment for the proof instance
        let value_commitment_opening = ValueCommitmentOpening {
            value,
            randomness: rcv.inner(),
        };

        // We now have a full witness for the output proof.
        Output {
            value_commitment_opening: Some(value_commitment_opening),
            payment_address: Some(payment_address),
            commitment_randomness: Some(rcm),
            esk: Some(esk),
        }
    }

    fn create_proof<R: RngCore>(&self, circuit: Output, rng: &mut R) -> Self::Proof {
        create_random_proof(circuit, &self.0, rng).expect("proving should not fail")
    }

    fn encode_proof(proof: Self::Proof) -> GrothProofBytes {
        let mut zkproof = [0u8; GROTH_PROOF_SIZE];
        proof
            .write(&mut zkproof[..])
            .expect("should be able to serialize a proof");
        zkproof
    }
}

#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub mod mock {
    use ff::Field;

    use super::{OutputProver, SpendProver};
    use crate::{
        bundle::GrothProofBytes,
        circuit::{self, ValueCommitmentOpening, GROTH_PROOF_SIZE},
        value::{NoteValue, ValueCommitTrapdoor},
        Diversifier, MerklePath, PaymentAddress, ProofGenerationKey, Rseed,
    };

    pub struct MockSpendProver;

    impl SpendProver for MockSpendProver {
        type Proof = GrothProofBytes;

        fn prepare_circuit(
            proof_generation_key: ProofGenerationKey,
            diversifier: Diversifier,
            _rseed: Rseed,
            value: NoteValue,
            alpha: jubjub::Fr,
            rcv: ValueCommitTrapdoor,
            anchor: bls12_381::Scalar,
            _merkle_path: MerklePath,
        ) -> Option<circuit::Spend> {
            let payment_address = proof_generation_key
                .to_viewing_key()
                .ivk()
                .to_payment_address(diversifier);
            Some(circuit::Spend {
                value_commitment_opening: Some(ValueCommitmentOpening {
                    value,
                    randomness: rcv.inner(),
                }),
                proof_generation_key: Some(proof_generation_key),
                payment_address,
                commitment_randomness: Some(jubjub::Scalar::ZERO),
                ar: Some(alpha),
                auth_path: vec![],
                anchor: Some(anchor),
            })
        }

        fn create_proof<R: rand_core::RngCore>(
            &self,
            _circuit: circuit::Spend,
            _rng: &mut R,
        ) -> Self::Proof {
            [0u8; GROTH_PROOF_SIZE]
        }

        fn encode_proof(proof: Self::Proof) -> GrothProofBytes {
            proof
        }
    }

    pub struct MockOutputProver;

    impl OutputProver for MockOutputProver {
        type Proof = GrothProofBytes;

        fn prepare_circuit(
            esk: jubjub::Fr,
            payment_address: PaymentAddress,
            rcm: jubjub::Fr,
            value: NoteValue,
            rcv: ValueCommitTrapdoor,
        ) -> circuit::Output {
            circuit::Output {
                value_commitment_opening: Some(ValueCommitmentOpening {
                    value,
                    randomness: rcv.inner(),
                }),
                payment_address: Some(payment_address),
                commitment_randomness: Some(rcm),
                esk: Some(esk),
            }
        }

        fn create_proof<R: rand_core::RngCore>(
            &self,
            _circuit: circuit::Output,
            _rng: &mut R,
        ) -> Self::Proof {
            [0u8; GROTH_PROOF_SIZE]
        }

        fn encode_proof(proof: Self::Proof) -> GrothProofBytes {
            proof
        }
    }
}
