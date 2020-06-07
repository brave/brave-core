#[macro_use]
extern crate zkp;
extern crate elgamal_ristretto;

#[cfg(test)]
mod test;

use curve25519_dalek::constants::RISTRETTO_BASEPOINT_TABLE;
use curve25519_dalek::ristretto::RistrettoPoint;
use curve25519_dalek::scalar::Scalar;
use rand_core::OsRng;

use elgamal_ristretto::public::PublicKey;
use elgamal_ristretto::private::SecretKey;
use elgamal_ristretto::ciphertext::Ciphertext;
use zkp::{CompactProof, Transcript};

/// Generates asymmetric key pair and proof of knowledge of the secret (zero knowledge)
pub fn generate_keys() -> (SecretKey, PublicKey) {
    let sk = SecretKey::new(&mut OsRng);
    let pk = PublicKey::from(&sk);
    (sk, pk)
}

/// Combines two public keys to generate one where the two private keys are required to decrypt
/// a ciphertext.
pub fn combine_pks(pk1: PublicKey, pk2: PublicKey) -> PublicKey {
    PublicKey::from(pk1.get_point() + pk2.get_point())
}

/// Computes an encrypted vector in which each element contains an encrypted
/// element resulting from subtracting the encrypted hash value and the
/// original check value
pub fn compute_checks(
    shared_pk: &PublicKey,
    encrypted_hashes: Vec<Ciphertext>,
    vector_checks: Vec<Scalar>,
) -> Result<Vec<Ciphertext>, &'static str> {
    if encrypted_hashes.len() != vector_checks.len() {
        println!("Error: Size of encrypted hashes slice must be the same as vector checks (encHash: {} Vs vecCheck: {})", encrypted_hashes.len(), vector_checks.len());
        return Err("Size of encrypted hashes slice must be the same as vector checks");
    }
    let encrypted_checks = vector_checks
        .into_iter()
        .zip(encrypted_hashes.into_iter())
        .map(|(checks, hashes)| shared_pk.encrypt(&(&checks * &RISTRETTO_BASEPOINT_TABLE)) - hashes)
        .collect();
    Ok(encrypted_checks)
}

// Generate macro for Discrete Logarithm Equality proof (from zkp crate)
define_proof! {dleq, "DLEQ Proof", (x), (A, B, H), (G) : A = (x * B), H = (x * G)}

/// Randomizes and proofs correctness of operation
pub fn randomize_and_prove(
    encrypted_checks: &Vec<Ciphertext>,
) -> (Vec<Ciphertext>, Vec<CompactProof>) {
    let mut randomized_vector: Vec<Ciphertext> = Vec::new();
    let mut proofs_correct_randomization: Vec<CompactProof> = Vec::new();

    for i in 0..encrypted_checks.len() {
        let randomizer = Scalar::random(&mut OsRng);

        randomized_vector.push(
            encrypted_checks[i] * randomizer
        );
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
    encrypted_vector: &Vec<Ciphertext>,
    randomized_vector: &Vec<Ciphertext>,
    proofs: &Vec<CompactProof>,
) -> Result<bool, &'static str> {
    if encrypted_vector.len() != randomized_vector.len() {
        println!("Error: Size of encrypted vector must equal the size of the randomized vector. Got {} (enc vec) and {} (part decrypted)", encrypted_vector.len(), randomized_vector.len());
        return Err("Size of encrypted vector must be the same as randomized vector");
    }

    if encrypted_vector.len() != proofs.len() {
        println!("Error: Size of encrypted vector must equal the number of proofs. Got {} (enc vec) and {} (proofs)", encrypted_vector.len(), proofs.len());
        return Err("Size of encrypted hashes slice must be the same as vector checks");
    }

    let mut result = true;
    for i in 0..proofs.len() {
        let mut transcript = Transcript::new(b"CorrectRandomization");
        result = result & dleq::verify_compact(
            &proofs[i],
            &mut transcript,
            dleq::VerifyAssignments {
                A: &randomized_vector[i].points.0.compress(),
                B: &encrypted_vector[i].points.0.compress(),
                H: &randomized_vector[i].points.1.compress(),
                G: &encrypted_vector[i].points.1.compress(),
            },
        ).is_ok()
    }
    Ok(result)
}

/// Computes partial decryption of ciphertext and returns a set of proofs of
/// correct decryption.
/// The partial decryption is due to the shared key.
pub fn partial_decryption_and_proof(
    randomized_vector: &Vec<Ciphertext>,
    sk: &SecretKey,
) -> (Vec<Ciphertext>, Vec<CompactProof>) {
    let partial_decryption: Vec<Ciphertext> = randomized_vector
        .into_iter()
        .map(|ciphertext| Ciphertext {
            pk: ciphertext.pk,
            points: (ciphertext.points.0, sk.decrypt(ciphertext)),
        })
        .collect();

    let proofs_correct_decryption: Vec<CompactProof> = partial_decryption
        .clone()
        .into_iter()
        .zip(randomized_vector.into_iter())
        .map(|(x, y)| sk.prove_correct_decryption(y, &x.points.1))
        .collect();

    (partial_decryption, proofs_correct_decryption)
}

/// Verify vector of proofs
pub fn verify_partial_decryption_proofs(
    public_key: &PublicKey,
    ctxt: &Vec<Ciphertext>,
    partial_dec_ctxt: &Vec<Ciphertext>,
    proofs: &Vec<zkp::CompactProof>,
) -> Result<bool, &'static str> {
    if ctxt.len() != partial_dec_ctxt.len() {
        println!("Error: Size of encrypted vector must equal the size of the partially decrypted vector. Got {} (enc vec) and {} (part decrypted)", ctxt.len(), partial_dec_ctxt.len());
        return Err("Size of encrypted vector must be the same as partially decrypted vector");
    }

    if ctxt.len() != proofs.len() {
        println!("Error: Size of encrypted vector must equal the number of proofs. Got {} (enc vec) and {} (proofs)", ctxt.len(), proofs.len());
        return Err("Size of encrypted hashes slice must be the same as vector checks");
    }
    let mut result = true;
    for i in 0..proofs.len() {
        result = result & public_key.verify_correct_decryption(
            &proofs[i].clone(),
            &ctxt[i],
            &partial_dec_ctxt[i].points.1,
        );
    }
    Ok(result)

}

/// Checks if a vector of "tests" has passed. A given test passes if its
/// correspondent index is zero in the Ristretto group. It returns the result
/// of the check.
pub fn check_tests(final_decryption: Vec<RistrettoPoint>) -> bool {
    let mut passed = true;
    let zero_point = RISTRETTO_BASEPOINT_TABLE.basepoint() * Scalar::zero();

    for (index, value) in final_decryption.into_iter().enumerate() {
        if !(value == zero_point) {
            print!("\nCheck {} failed. User is using an emulator.\n", index);
            passed = false;
            break;
        }
    }
    passed
}

/// Encrypts a vector of inputs. The input is encrytped using a threshold
/// generated among two peers (in our case, the client and the server)
pub fn encrypt_input(shared_pk: PublicKey, vector_hashes: &[Scalar]) -> Vec<Ciphertext> {
    let encrypted_hashes: Vec<Ciphertext> = vector_hashes
        .into_iter()
        .map(|x| shared_pk.encrypt(&(x * &RISTRETTO_BASEPOINT_TABLE)))
        .collect();
    encrypted_hashes
}

#[cfg(test)]
mod tests {
    // Note this useful idiom: importing names from outer (for mod tests) scope.
    use super::*;

    #[test]
    fn combine_keys() {
        let (_sk_1, pk_1) = generate_keys();
        let (_sk_2, pk_2) = generate_keys();

        assert_eq!(combine_pks(pk_1, pk_2), combine_pks(pk_2, pk_1))
    }

    #[test]
    fn randomization_proofs() {
        let vector_scalar: &[Scalar] = &[
            Scalar::random(&mut OsRng),
            Scalar::random(&mut OsRng),
            Scalar::random(&mut OsRng),
            Scalar::random(&mut OsRng),
        ];

        let (_sk, pk) = generate_keys();
        let encrypted_values = encrypt_input(pk, vector_scalar);
        let (randomized_vector, proof_correct_rand) = randomize_and_prove(&encrypted_values);

        let verification = verify_randomization_proofs(&encrypted_values, &randomized_vector, &proof_correct_rand);

        assert!(verification.unwrap());

        let fake_reencryption = encrypt_input(pk, vector_scalar);
        let wrong_verification = verify_randomization_proofs(&encrypted_values, &fake_reencryption, &proof_correct_rand);

        assert!(!wrong_verification.unwrap());

        let wrong_size: &[Scalar] = &[
            Scalar::random(&mut OsRng),
            Scalar::random(&mut OsRng),
        ];
        let wrong_size_encrypted_values = encrypt_input(pk, wrong_size);
        let wrong_verification = verify_randomization_proofs(&encrypted_values, &wrong_size_encrypted_values, &proof_correct_rand);

        assert!(!wrong_verification.is_ok());

        let wrong_nr_proofs_verification = verify_randomization_proofs(&encrypted_values, &randomized_vector, &(proof_correct_rand[1..3].to_vec()));

        assert!(!wrong_nr_proofs_verification.is_ok());
    }

    #[test]
    fn partial_decryption_proofs() {
        let vector_scalar: &[Scalar] = &[
            Scalar::random(&mut OsRng),
            Scalar::random(&mut OsRng),
            Scalar::random(&mut OsRng),
            Scalar::random(&mut OsRng),
        ];

        let (sk_1, pk_1) = generate_keys();
        let (_sk_2, pk_2) = generate_keys();

        let combined_key = combine_pks(pk_1, pk_2);

        let encrypted_values = encrypt_input(combined_key, vector_scalar);

        let (partial_decryption, proofs_correct_decryption) =
            partial_decryption_and_proof(&encrypted_values, &sk_1);
        let verification = verify_partial_decryption_proofs(
            &pk_1,
            &encrypted_values,
            &partial_decryption,
            &proofs_correct_decryption
        );

        assert!(verification.unwrap());

        let fake_partial_decryption = encrypt_input(combined_key, vector_scalar);
        let wrong_verification = verify_partial_decryption_proofs(
            &pk_1,
            &encrypted_values,
            &fake_partial_decryption,
            &proofs_correct_decryption
        );

        assert!(!wrong_verification.unwrap());

        let wrong_size: &[Scalar] = &[
            Scalar::random(&mut OsRng),
            Scalar::random(&mut OsRng),
        ];
        let wrong_size_encrypted_values = encrypt_input(combined_key, wrong_size);
        let wrong_verification = verify_partial_decryption_proofs(
            &pk_1,
            &encrypted_values,
            &wrong_size_encrypted_values,
            &proofs_correct_decryption
        );

        assert!(!wrong_verification.is_ok());

        let wrong_nr_proofs_verification = verify_partial_decryption_proofs(
            &pk_1,
            &encrypted_values,
            &partial_decryption,
            &(proofs_correct_decryption[1..3].to_vec()));

        assert!(!wrong_nr_proofs_verification.is_ok());
    }
}


