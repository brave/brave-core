extern crate curve25519_dalek;
extern crate sha2;

use curve25519_dalek::scalar::Scalar;
use elgamal_ristretto::ciphertext::Ciphertext;
use elgamal_ristretto::private::SecretKey;
use repsys_crypto::{
    combine_pks, encrypt_input, generate_keys, partial_decryption_and_proof, randomize_and_prove,
};

use sha2::Sha512;
use std::vec::Vec;

use bincode;

pub fn start_challenge(
    input: Vec<String>,
    server_pk_encoded: [u8; 32],
) -> Result<(Vec<u8>, Vec<u8>, Vec<u8>, Vec<u8>), ()> {
    let pk_server = match bincode::deserialize(&server_pk_encoded) {
			Err(_) => return Err(()),
			Ok(pk) => pk,
		};

    let mut vector_hashes: Vec<Scalar> = Vec::new();
    for n in 0..input.len() {
        vector_hashes.push(Scalar::hash_from_bytes::<Sha512>(input[n].as_bytes()));
    }

    // generate new key pair at every round
    let (sk_client, pk_client) = generate_keys();
    let shared_pk = combine_pks(pk_client, pk_server);

    let encrypted_hashes = encrypt_input(shared_pk, &vector_hashes);

    let encoded_pk = match bincode::serialize(&pk_client) {
      Err(_) => return Err(()),
      Ok(pk) => pk,
    };

    let encoded_sk = match bincode::serialize(&sk_client) {
			Err(_) => return Err(()),
			Ok(sk) => sk,
		};
    let encoded_shared_pk = match bincode::serialize(&shared_pk) {
			Err(_) => return Err(()),
			Ok(pk) => pk,
		};
    let encoded_enc_hashes = match bincode::serialize(&encrypted_hashes) {
			Err(_) => return Err(()),
			Ok(h) => h,
		};
    Ok((encoded_pk, encoded_sk, encoded_shared_pk, encoded_enc_hashes))
}

pub fn second_round(input: &[u8], encoded_sk: &[u8]) -> Result<(Vec<u8>,Vec<u8>,
Vec<u8>), ()> {
    let sk: SecretKey = match bincode::deserialize(encoded_sk) {
			Err(_) => return Err(()),
			Ok(h) => h,
		};
    let enc_checks: Vec<Ciphertext> = match bincode::deserialize(input) {
			Err(_) => return Err(()),
			Ok(h) => h,
		};

    let (rand_vec, _) = randomize_and_prove(&enc_checks);
    let (partial_decryption, proofs_correct_decryption) =
        partial_decryption_and_proof(&rand_vec, &sk);

    let encoded_partial_decryption = match bincode::serialize(&partial_decryption) {
			Err(_) => return Err(()),
			Ok(h) => h,
		};
    let encoded_proofs = match bincode::serialize(&proofs_correct_decryption) {
			Err(_) => return Err(()),
			Ok(h) => h,
		};
    let encoded_rand_vec = match bincode::serialize(&rand_vec) {
			Err(_) => return Err(()),
			Ok(h) => h,
		};
    Ok((encoded_partial_decryption, encoded_proofs, encoded_rand_vec))
}
