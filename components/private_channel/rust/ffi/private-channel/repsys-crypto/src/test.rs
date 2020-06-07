#[allow(warnings)]
use super::*;

use curve25519_dalek::scalar::Scalar;
use rand_core::OsRng;

#[test]
fn test_e2e_simple() {
    let vector_hashes: &[Scalar] = &[Scalar::random(&mut OsRng), Scalar::random(&mut OsRng)];
    let vector_checks: Vec<Scalar> = vec![vector_hashes[0], vector_hashes[1]];

    let (sk, pk) = generate_keys();
    let encrypted_hashes = encrypt_input(pk, vector_hashes);
    let encrypted_checks = compute_checks(&pk, encrypted_hashes, vector_checks)
        .expect("Error encrypting check vector");

    let decrypted_checks: Vec<RistrettoPoint> = encrypted_checks
        .into_iter()
        .map(|ciphertext| sk.decrypt(&ciphertext))
        .collect();

    let zero_point = RISTRETTO_BASEPOINT_TABLE.basepoint() * Scalar::zero();
    assert_eq!(zero_point, decrypted_checks[0]);
    assert_eq!(zero_point, decrypted_checks[1]);
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

    /* SERVER */
    let (sk_server, pk_server) = generate_keys();
    let pk_server_proof = sk_server.prove_knowledge();

    /* USER */
    let (sk_user, pk_user) = generate_keys();
    let pk_user_proof = sk_user.prove_knowledge();

    // User verifies if the server does know the private key associated with
    // `pk_server`
    assert!(pk_server.verify_proof_knowledge(&pk_server_proof));

    // The user encrypts the vector of values to send the server.
    // This vector will contain `n` hashes, out of which only `l` will be used
    // to compute the final score.
    let shared_pk_user = combine_pks(pk_user, pk_server);
    let encrypted_hashes = encrypt_input(shared_pk_user, vector_hashes);

    /* SERVER */

    // 2.- User sends the encrypted hashes to the server, together with its
    // share of the key The server generates the shared key, verifies the proof and produces
    // the checks.
    assert!(pk_user.verify_proof_knowledge(&pk_user_proof));
    let shared_pk_server = combine_pks(pk_server, pk_user);

    // Generated shared keys must be the same
    assert_eq!(shared_pk_server, shared_pk_user);

    // Server now computes the encrypted checks, i.e. performs ElGamal
    // homomorphic addition over each element of the arrays (between the
    // `encrypted_hashes` sent by user and the `vector_checks` in the server)
    let encrypted_checks = compute_checks(&shared_pk_server, encrypted_hashes, vector_checks)
        .expect("Error encrypting check vector");

    /* USER */
    // 3.- Server sends the aggregate (`encrypted_checks`) back to the user.
    // The user first randomises each of the values, and then decrypts the
    // entries in the vector.
    // The randomisation is necessary so that, if the server cheats it will not
    // be able to learn anything about it the user input. Thus, the server
    // will only be able to determine whether some of these encrypted values
    // were zero.
    let (randomized_vector, _proof_correct_rand) = randomize_and_prove(&encrypted_checks);
    let (partial_decryption, proofs_correct_decryption) =
        partial_decryption_and_proof(&randomized_vector, &sk_user);

    /* SERVER */
    // 4.- User sends the partially decrypted aggregated count to the server,
    // together with the proof of correct decryption.
    // First, the server verifies the proof of correct decryption.
    assert!(verify_partial_decryption_proofs(
        &pk_user,
        &randomized_vector,
        &partial_decryption,
        &proofs_correct_decryption
    ).unwrap());

    // Finally, the server fully decrypts the ciphertext and checks if the
    // decryption equals zero.
    let (final_decryption, _proof_correct_decryption_server) =
        partial_decryption_and_proof(&partial_decryption, &sk_server);

    let plaintext: Vec<curve25519_dalek::ristretto::RistrettoPoint> =
        final_decryption.into_iter().map(|x| x.points.1).collect();

    // tests must pass since the `vector_hashes` is a copy of `vector_checks`
    assert!(check_tests(plaintext));
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

    let (sk_server, pk_server) = generate_keys();
    let pk_server_proof = sk_server.prove_knowledge();

    let (sk_user, pk_user) = generate_keys();
    let pk_user_proof = sk_user.prove_knowledge();

    assert!(pk_server.verify_proof_knowledge(&pk_server_proof));

    let shared_pk_user = combine_pks(pk_user, pk_server);
    let encrypted_hashes = encrypt_input(shared_pk_user, vector_hashes);
    assert!(pk_user.verify_proof_knowledge(&pk_user_proof));

    let shared_pk_server = combine_pks(pk_server, pk_user);
    assert_eq!(shared_pk_server, shared_pk_user);

    let encrypted_checks = compute_checks(&shared_pk_server, encrypted_hashes, vector_checks)
        .expect("Error encrypting check vector");

    let (randomized_vector, _proofs_correct_rand) = randomize_and_prove(&encrypted_checks);

    let (partial_decryption, proofs_correct_decryption) =
        partial_decryption_and_proof(&randomized_vector, &sk_user);

    assert!(verify_partial_decryption_proofs(
        &pk_user,
        &randomized_vector,
        &partial_decryption,
        &proofs_correct_decryption
    ).unwrap());

    let (final_decryption, _proofs_correct_decryption_server) =
        partial_decryption_and_proof(&partial_decryption, &sk_server);

    let plaintext = final_decryption.into_iter().map(|x| x.points.1).collect();

    // tests must not pass since the `vector_hashes` is generated randomly
    assert_eq!(false, check_tests(plaintext));
}
