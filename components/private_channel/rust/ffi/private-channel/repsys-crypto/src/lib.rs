#[macro_use]
extern crate zkp;

use curve25519_dalek::constants::RISTRETTO_BASEPOINT_TABLE;
use curve25519_dalek::scalar::Scalar;
use rand_core::OsRng;

use elgamal_ristretto::ciphertext::Ciphertext;
use elgamal_ristretto::private::SecretKey;
use elgamal_ristretto::public::PublicKey;
use zkp::{CompactProof, Transcript};

/// Generates asymmetric key pair
pub fn generate_keys() -> (SecretKey, PublicKey) {
    let sk = SecretKey::new(&mut OsRng);
    let pk = PublicKey::from(&sk);
    (sk, pk)
}

/// Generates a vector of asymmetric key pairs
pub fn generate_key_vector(length: usize) -> (Vec<SecretKey>, Vec<PublicKey>) {
    let sks: Vec<SecretKey> = (0..length).map(|_| SecretKey::new(&mut OsRng)).collect();
    let pks: Vec<PublicKey> = (&sks).iter().map(|x| PublicKey::from(x)).collect();

    (sks, pks)
}

/// Combines two public keys to generate one where the two private keys are required to decrypt
/// a ciphertext.
pub fn combine_pks(pk1: PublicKey, pk2: PublicKey) -> PublicKey {
    PublicKey::from(pk1.get_point() + pk2.get_point())
}

/// Combines a vector of public keys with another public key, to generate a vector of shared public
/// keys, where the two private keys are required to decrypt a ciphertext.
pub fn combine_pks_vector(pk1: &[PublicKey], pk2: PublicKey) -> Vec<PublicKey> {
    pk1.iter().map(|&pk| combine_pks(pk, pk2)).collect()
}

// Generate macro for Discrete Logarithm Equality proof (from zkp crate)
define_proof! {dleq, "DLEQ Proof", (x), (A, B, H), (G) : A = (x * B), H = (x * G)}

/// Randomizes and proves correctness of operation
pub fn randomize_and_prove(
    encrypted_checks: &[Ciphertext],
) -> (Vec<Ciphertext>, Vec<CompactProof>) {
    let mut randomized_vector: Vec<Ciphertext> = Vec::with_capacity(encrypted_checks.len());
    let mut proofs_correct_randomization: Vec<CompactProof> =
        Vec::with_capacity(encrypted_checks.len());

    for i in 0..encrypted_checks.len() {
        let randomizer = Scalar::random(&mut OsRng);

        randomized_vector.push(encrypted_checks[i] * randomizer);
        let mut transcript = Transcript::new(b"CorrectRandomization");
        let (proof, _) = dleq::prove_compact(
            &mut transcript,
            dleq::ProveAssignments {
                x: &randomizer,
                A: &randomized_vector[i].points.0,
                B: &encrypted_checks[i].points.0,
                H: &randomized_vector[i].points.1,
                G: &encrypted_checks[i].points.1,
            },
        );
        proofs_correct_randomization.push(proof);
    }
    (randomized_vector, proofs_correct_randomization)
}

/// Verify Randomization proofs
pub fn verify_randomization_proofs(
    encrypted_vector: &[Ciphertext],
    randomized_vector: &[Ciphertext],
    proofs: &[CompactProof],
) -> Result<bool, &'static str> {
    if encrypted_vector.len() != randomized_vector.len() {
        return Err("Size of encrypted vector must be the same as randomized vector");
    }

    if encrypted_vector.len() != proofs.len() {
        return Err("Size of encrypted hashes slice must be the same as vector checks");
    }

    for i in 0..proofs.len() {
        let mut transcript = Transcript::new(b"CorrectRandomization");
        if dleq::verify_compact(
            &proofs[i],
            &mut transcript,
            dleq::VerifyAssignments {
                A: &randomized_vector[i].points.0.compress(),
                B: &encrypted_vector[i].points.0.compress(),
                H: &randomized_vector[i].points.1.compress(),
                G: &encrypted_vector[i].points.1.compress(),
            },
        )
            .is_err()
        {
            return Ok(false);
        }
    }
    Ok(true)
}

/// Computes partial decryption of ciphertext and returns a set of proofs of
/// correct decryption.
/// The partial decryption is due to the shared key.
pub fn partial_decryption_and_proof(
    randomized_vector: &[Ciphertext],
    sk: &SecretKey,
) -> (Vec<Ciphertext>, Vec<CompactProof>) {
    let partial_decryption: Vec<Ciphertext> = randomized_vector
        .iter()
        .map(|ciphertext| Ciphertext {
            pk: ciphertext.pk,
            points: (ciphertext.points.0, sk.decrypt(ciphertext)),
        })
        .collect();

    let proofs_correct_decryption: Vec<CompactProof> = partial_decryption
        .clone()
        .into_iter()
        .zip(randomized_vector.iter())
        .map(|(x, y)| sk.prove_correct_decryption(y, &x.points.1))
        .collect();

    (partial_decryption, proofs_correct_decryption)
}

/// Computes partial decryption of ciphertext using a vector of private keys, and returns a set of
/// proofs of correct decryption.
/// The partial decryption is due to the shared key.
pub fn partial_decryption_and_proof_vec_key(
    randomized_vector: &[Ciphertext],
    sks: &[SecretKey],
) -> Result<(Vec<Ciphertext>, Vec<CompactProof>), &'static str> {
    if randomized_vector.len() != sks.len() {
        return Err("Size of the vector must equal the number of secret keys");
    }

    let partial_decryption: Vec<Ciphertext> = randomized_vector
        .iter()
        .zip(sks.iter())
        .map(|(ciphertext, key)| Ciphertext {
            pk: ciphertext.pk,
            points: (ciphertext.points.0, key.decrypt(ciphertext)),
        })
        .collect();

    let mut proofs_correct_decryption: Vec<CompactProof> = Vec::new();

    for (index, value) in partial_decryption.iter().enumerate() {
        proofs_correct_decryption
            .push(sks[index].prove_correct_decryption(&randomized_vector[index], &value.points.1));
    }

    Ok((partial_decryption, proofs_correct_decryption))
}

/// Verify vector of proofs
pub fn verify_partial_decryption_proofs(
    public_keys: &[PublicKey],
    ctxt: &[Ciphertext],
    partial_dec_ctxt: &[Ciphertext],
    proofs: &[zkp::CompactProof],
) -> Result<bool, &'static str> {
    if ctxt.len() != partial_dec_ctxt.len() {
        return Err("Size of encrypted vector must be the same as partially decrypted vector");
    }

    if ctxt.len() != proofs.len() {
        return Err("Size of encrypted hashes slice must be the same as vector checks");
    }

    if ctxt.len() != public_keys.len() {
        return Err("Size of encrypted vector must equal the number of public keys.");
    }

    for i in 0..proofs.len() {
        if !public_keys[i].verify_correct_decryption(
            &proofs[i].clone(),
            &ctxt[i],
            &partial_dec_ctxt[i].points.1,
        ) {
            return Ok(false);
        }
    }
    Ok(true)
}

/// Encrypts a vector of inputs. The input is encrypted using a threshold
/// generated among two peers (in our case, the client and the server)
pub fn encrypt_input(
    shared_pks: &[PublicKey],
    vector_hashes: &[Scalar],
) -> Result<Vec<Ciphertext>, &'static str> {
    if shared_pks.len() != vector_hashes.len() {
        return Err("Size of public keys must equal the size of the array.");
    }

    let encrypted_hashes: Vec<Ciphertext> = vector_hashes
        .iter()
        .zip(shared_pks.iter())
        .map(|(hash, pk)| pk.encrypt(&(hash * &RISTRETTO_BASEPOINT_TABLE)))
        .collect();

    Ok(encrypted_hashes)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_combine_pks() {
        let (_sk_1, pk_1) = generate_keys();
        let (_sk_2, pk_2) = generate_keys();

        assert_eq!(combine_pks(pk_1, pk_2), combine_pks(pk_2, pk_1))
    }

    #[test]
    fn test_combine_pks_vector() {
        let size = 8;
        let (_sks, pks) = generate_key_vector(size);
        let (_sk_2, pk_2) = generate_keys();

        let combined_vector_1 = combine_pks_vector(&pks, pk_2);
        let combined_vector_2: Vec<PublicKey> =
            pks.into_iter().map(|pk| combine_pks(pk_2, pk)).collect();

        for (pk1, pk2) in combined_vector_1.iter().zip(combined_vector_2.iter()) {
            assert_eq!(pk1, pk2)
        }
    }
}
