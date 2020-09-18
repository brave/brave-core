#[allow(unused_imports)]
use repsys_crypto::{
    combine_pks, compute_checks, encrypt_input, generate_key_vector, generate_keys,
    partial_decryption_and_proof, randomize_and_prove, verify_partial_decryption_proofs,
};

use curve25519_dalek::constants::RISTRETTO_BASEPOINT_TABLE;
use curve25519_dalek::ristretto::RistrettoPoint;
use curve25519_dalek::scalar::Scalar;
use rand_core::OsRng;
use repsys_crypto::{
    check_tests, combine_pks_vector, partial_decryption_and_proof_vec_key,
    verify_randomization_proofs,
};
use zkp::CompactProof;

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

#[test]
fn test_e2e_simple() {
    let vector_hashes: &[Scalar] = &[Scalar::random(&mut OsRng), Scalar::random(&mut OsRng)];
    let vector_checks: Vec<Scalar> = vec![vector_hashes[0], vector_hashes[1]];
    let length = vector_hashes.len();

    let (sks, pks) = generate_key_vector(length);
    let encrypted_hashes = encrypt_input(&pks, vector_hashes).expect("Error encrypting hashes");
    let encrypted_checks = compute_checks(&pks, &encrypted_hashes, &vector_checks)
        .expect("Error encrypting check vector");

    let decrypted_checks: Vec<RistrettoPoint> = encrypted_checks
        .into_iter()
        .zip(sks.into_iter())
        .map(|(ciphertext, sk)| sk.decrypt(&ciphertext))
        .collect();

    let zero_point = RISTRETTO_BASEPOINT_TABLE.basepoint() * Scalar::zero();
    assert_eq!(zero_point, decrypted_checks[0]);
    assert_eq!(zero_point, decrypted_checks[1]);
}

#[test]
fn test_e2e_not_passing() {
    // generate random scalars as user reply.
    //let mut csprng: OsRng = OsRng::new().unwrap();
    let vector_hashes: &[Scalar] = &[
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
    ];

    // `vector_checks` != from `vector_hashes`, so that the test fails
    let vector_checks: Vec<Scalar> = vec![
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
    ];

    let length = vector_hashes.len();

    let (sk_server, pk_server) = generate_keys();
    let pk_server_proof = sk_server.prove_knowledge();

    let (sks_user, pks_user) = generate_key_vector(length);
    let pk_user_proofs: Vec<CompactProof> = (&sks_user)
        .into_iter()
        .map(|sk| sk.prove_knowledge())
        .collect();

    assert!(pk_server.verify_proof_knowledge(&pk_server_proof));

    let shared_pks_user = combine_pks_vector(&pks_user, pk_server);
    let encrypted_hashes =
        encrypt_input(&shared_pks_user, vector_hashes).expect("Error encrypting input");

    for (pk, proof) in (&pks_user).iter().zip(pk_user_proofs.iter()) {
        assert!(pk.verify_proof_knowledge(&proof))
    }

    let shared_pks_server = combine_pks_vector(&pks_user, pk_server);

    assert_eq!(shared_pks_server, shared_pks_user);

    let encrypted_checks = compute_checks(&shared_pks_server, &encrypted_hashes, &vector_checks)
        .expect("Error encrypting check vector");

    let (randomized_vector, _proofs_correct_rand) = randomize_and_prove(&encrypted_checks);

    let (partial_decryption, proofs_correct_decryption) =
        partial_decryption_and_proof_vec_key(&randomized_vector, &sks_user)
            .expect("Error decrypting the vector of ciphertexts");

    assert!(verify_partial_decryption_proofs(
        &pks_user,
        &randomized_vector,
        &partial_decryption,
        &proofs_correct_decryption
    )
        .unwrap());

    let (final_decryption, _proofs_correct_decryption_server) =
        partial_decryption_and_proof(&partial_decryption, &sk_server);

    let plaintext: Vec<RistrettoPoint> = final_decryption.into_iter().map(|x| x.points.1).collect();

    // tests must not pass since the `vector_hashes` is generated randomly
    assert_eq!(false, check_tests(&plaintext));
}

#[test]
fn test_e2e_passing() {
    // generate random scalars as user reply.
    let vector_hashes: &[Scalar] = &[
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
        Scalar::random(&mut OsRng),
    ];
    // `vector_checks` === `vector_hashes`, so that the test passes!
    let vector_checks: Vec<Scalar> = vec![
        vector_hashes[0],
        vector_hashes[1],
        vector_hashes[2],
        vector_hashes[3],
    ];

    let length = vector_checks.len();

    /* SERVER */
    let (sk_server, pk_server) = generate_keys();
    let pk_server_proof = sk_server.prove_knowledge();

    /* USER */
    let (sks_user, pks_user) = generate_key_vector(length);
    let pk_user_proofs: Vec<CompactProof> = (&sks_user)
        .into_iter()
        .map(|sk| sk.prove_knowledge())
        .collect();

    // 1.- User fetches the server's public key, and verifies
    // it does know the private key associated with `pk_server`.
    assert!(pk_server.verify_proof_knowledge(&pk_server_proof));

    // Then, the user encrypts the vector of values to send the server.
    // This vector will contain `n` hashes, out of which only `l` will be used
    // to compute the final score.
    let shared_pks_user = combine_pks_vector(&pks_user, pk_server);
    let encrypted_hashes =
        encrypt_input(&shared_pks_user, vector_hashes).expect("Error encrypting input");

    /* SERVER */

    // 2.- User sends the encrypted hashes to the server, together with its
    // share of the key. The server generates the shared key, verifies the proof and produces
    // the checks.
    for (pk, proof) in pks_user.iter().zip(pk_user_proofs.iter()) {
        assert!(pk.verify_proof_knowledge(&proof))
    }

    let shared_pks_server = combine_pks_vector(&pks_user, pk_server);

    // Generated shared keys must be the same
    assert_eq!(shared_pks_server, shared_pks_user);

    // Server now computes the encrypted checks, i.e. performs ElGamal
    // homomorphic addition over each element of the arrays (between the
    // `encrypted_hashes` sent by user and the `vector_checks` in the server)
    let encrypted_checks = compute_checks(&shared_pks_server, &encrypted_hashes, &vector_checks)
        .expect("Error encrypting check vector");

    /* USER */
    // 3.- Server sends the aggregate (`encrypted_checks`) back to the user.
    // The user first randomises each of the values, and then decrypts the
    // entries in the vector.
    // The randomisation is necessary so that, if the server cheats it will not
    // be able to learn anything about the user input. Thus, the server
    // will only be able to determine whether some of these encrypted values
    // were zero.
    let (randomized_vector, _proof_correct_rand) = randomize_and_prove(&encrypted_checks);
    let (partial_decryption, proofs_correct_decryption) =
        partial_decryption_and_proof_vec_key(&randomized_vector, &sks_user)
            .expect("Error performing partial decryption");

    /* SERVER */
    // 4.- User sends the partially decrypted aggregated count to the server,
    // together with the proof of correct decryption.
    // First, the server verifies the proof of correct decryption.
    assert!(verify_partial_decryption_proofs(
        &pks_user,
        &randomized_vector,
        &partial_decryption,
        &proofs_correct_decryption
    )
        .unwrap());

    // Finally, the server fully decrypts the ciphertext and checks if the
    // decryption equals zero.
    let (final_decryption, _proof_correct_decryption_server) =
        partial_decryption_and_proof(&partial_decryption, &sk_server);

    let plaintext: Vec<RistrettoPoint> = final_decryption.into_iter().map(|x| x.points.1).collect();

    // tests must pass since the `vector_hashes` is a copy of `vector_checks`
    assert!(check_tests(&plaintext));
}
