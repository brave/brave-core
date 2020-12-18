#[allow(unused_imports)]
use repsys_crypto::*;

use curve25519_dalek::scalar::Scalar;
use rand_core::OsRng;

#[test]
fn randomization_proofs() {
    let vector_scalar: &[Scalar] = &[
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
    ];

    let length = vector_scalar.len();

    let (_sks, pks) = generate_key_vector(length);
    let encrypted_values = encrypt_input(&pks, vector_scalar).expect("Error encrypting hashes");
    let (randomized_vector, proof_correct_rand) = randomize_and_prove(&encrypted_values);

    let verification =
        verify_randomization_proofs(&encrypted_values, &randomized_vector, &proof_correct_rand);

    assert!(verification.unwrap());

    let fake_reencryption = encrypt_input(&pks, vector_scalar).expect("Error encrypting hashes");
    let wrong_verification =
        verify_randomization_proofs(&encrypted_values, &fake_reencryption, &proof_correct_rand);

    assert!(!wrong_verification.unwrap());

    let wrong_size: &[Scalar] = &[Scalar::random(&mut OsRng), Scalar::random(&mut OsRng)];
    let wrong_size_encrypted_values = encrypt_input(&pks, wrong_size);

    assert!(wrong_size_encrypted_values.is_err());

    let wrong_nr_proofs_verification = verify_randomization_proofs(
        &encrypted_values,
        &randomized_vector,
        &(proof_correct_rand[1..3].to_vec()),
    );

    assert!(wrong_nr_proofs_verification.is_err());
}

#[test]
fn partial_decryption_proofs() {
    let vector_scalar: &[Scalar] = &[
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
    ];

    let length = vector_scalar.len();

    let (sks, pks) = generate_key_vector(length);
    let (_sk_2, pk_2) = generate_keys();

    let combined_keys = combine_pks_vector(&pks, pk_2);

    let encrypted_values =
        encrypt_input(&combined_keys, vector_scalar).expect("Error encrypting hashes");

    let (partial_decryption, proofs_correct_decryption) =
        partial_decryption_and_proof_vec_key(&encrypted_values, &sks)
            .expect("Partial decryption failed");

    let verification = verify_partial_decryption_proofs(
        &pks,
        &encrypted_values,
        &partial_decryption,
        &proofs_correct_decryption,
    );

    assert!(verification.unwrap());

    let fake_partial_decryption =
        encrypt_input(&combined_keys, vector_scalar).expect("Error encrypting hashes");
    let wrong_verification = verify_partial_decryption_proofs(
        &pks,
        &encrypted_values,
        &fake_partial_decryption,
        &proofs_correct_decryption,
    );

    assert!(!wrong_verification.unwrap());

    let wrong_size: &[Scalar] = &[Scalar::random(&mut OsRng), Scalar::random(&mut OsRng)];
    let wrong_size_encrypted_values = encrypt_input(&combined_keys, wrong_size);

    assert!(wrong_size_encrypted_values.is_err());

    let wrong_nr_proofs_verification = verify_partial_decryption_proofs(
        &pks,
        &encrypted_values,
        &partial_decryption,
        &(proofs_correct_decryption[1..3].to_vec()),
    );

    assert!(wrong_nr_proofs_verification.is_err());
}
