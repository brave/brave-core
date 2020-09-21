use curve25519_dalek::scalar::Scalar;
use elgamal_ristretto::ciphertext::Ciphertext;
use elgamal_ristretto::private::SecretKey;
use elgamal_ristretto::public::PublicKey;
use repsys_crypto::{
    combine_pks_vector, encrypt_input, generate_key_vector, partial_decryption_and_proof_vec_key,
    randomize_and_prove,
};

use sha2::Sha512;
use std::str::FromStr;

pub struct FirstRoundOutput {
    pub pkeys: Vec<u8>,
    pub skeys: Vec<u8>,
    pub shared_pks: Vec<u8>,
    pub enc_hashes: Vec<u8>,
}

pub struct SecondRoundOutput {
    pub rand_vec: Vec<u8>,
    pub proofs_rand: Vec<u8>,
    pub partial_dec: Vec<u8>,
    pub proofs_dec: Vec<u8>,
}

pub fn start_challenge(
    input: Vec<String>,
    server_pk_encoded: String,
) -> Result<FirstRoundOutput, ()> {
    let pk_server = match parse_pk(server_pk_encoded) {
        Ok(pk) => pk,
        Err(_) => return Err(()),
    };

    let vector_hashes: Vec<Scalar> = input
        .iter()
        .map(|input_n| Scalar::hash_from_bytes::<Sha512>(input_n.as_bytes()))
        .collect();
    let length = vector_hashes.len();

    // generate new key pair at every round
    let (sks_client, pks_client) = generate_key_vector(length);

    let shared_pks = combine_pks_vector(&pks_client, pk_server);

    let encrypted_hashes = encrypt_input(&shared_pks, &vector_hashes).map_err(|_| ())?;

    let encoded_pks = bincode::serialize(&pks_client).map_err(|_| ())?;
    let encoded_sks = bincode::serialize(&sks_client).map_err(|_| ())?;
    let encoded_shared_pks = bincode::serialize(&shared_pks).map_err(|_| ())?;
    let encoded_enc_hashes = bincode::serialize(&encrypted_hashes).map_err(|_| ())?;

    Ok(FirstRoundOutput {
        pkeys: encoded_pks,
        skeys: encoded_sks,
        shared_pks: encoded_shared_pks,
        enc_hashes: encoded_enc_hashes,
    })
}

pub fn second_round(input: &[u8], encoded_sks: String) -> Result<SecondRoundOutput, ()> {
    let sks = match parse_sk_vector(encoded_sks) {
        Ok(sk) => sk,
        Err(_) => return Err(()),
    };

    let enc_checks: Vec<Ciphertext> = bincode::deserialize(input).map_err(|_| ())?;

    let (rand_vec, proofs_randomization) = randomize_and_prove(&enc_checks);

    let (partial_decryption, proofs_correct_decryption) =
        partial_decryption_and_proof_vec_key(&rand_vec, &sks).map_err(|_| ())?;

    let encoded_rand_vec = bincode::serialize(&rand_vec).map_err(|_| ())?;
    let proofs_rand = bincode::serialize(&proofs_randomization).map_err(|_| ())?;

    let encoded_partial_decryption = bincode::serialize(&partial_decryption).map_err(|_| ())?;
    let encoded_proofs = bincode::serialize(&proofs_correct_decryption).map_err(|_| ())?;

    Ok(SecondRoundOutput {
        rand_vec: encoded_rand_vec,
        proofs_rand: proofs_rand,
        partial_dec: encoded_partial_decryption,
        proofs_dec: encoded_proofs,
    })
}

pub fn parse_pk(s: String) -> Result<PublicKey, ()> {
    let enc_pk = s[1..s.len() - 1]
        .split(", ")
        .map(|x| u8::from_str(x))
        .filter_map(Result::ok)
        .collect::<Vec<u8>>();

    match bincode::deserialize(&enc_pk) {
        Ok(pk) => Ok(pk),
        Err(_) => Err(()),
    }
}

pub fn parse_sk_vector(s: String) -> Result<Vec<SecretKey>, ()> {
    let sk_vec = s[1..s.len() - 1]
        .split(", ")
        .map(|x| u8::from_str(x))
        .filter_map(Result::ok)
        .collect::<Vec<u8>>();

    match bincode::deserialize(&sk_vec) {
        Ok(pk) => Ok(pk),
        Err(_) => Err(()),
    }
}
