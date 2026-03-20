/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use chacha20poly1305::{
    aead::{Aead, AeadCore, KeyInit},
    Key as ChaChaKey, XChaCha20Poly1305, XNonce,
};
use curve25519_dalek::{edwards::CompressedEdwardsY, scalar::clamp_integer, EdwardsPoint};
use cxx::{CxxString, CxxVector};
use hkdf::Hkdf;
use rand::{rngs::OsRng, RngCore};
use sha2::{Digest, Sha256, Sha512};
use x25519_dalek::{
    EphemeralSecret as X25519EphemeralSecret, PublicKey as X25519PublicKey,
    StaticSecret as X25519StaticSecret,
};

#[cxx::bridge(namespace = "ai_chat")]
mod ffi {
    extern "Rust" {
        type ClientSecretKey;

        fn generate_client_keypair() -> ClientKeyPair;
        fn encrypt(plaintext: &CxxString, model_public_key: &CxxVector<u8>) -> Vec<u8>;
        fn decrypt(ciphertext: &CxxVector<u8>, client_secret_key: &ClientSecretKey) -> String;
    }

    struct ClientKeyPair {
        public_key: Vec<u8>,
        secret_key: Box<ClientSecretKey>,
    }
}

use ffi::*;

const HKDF_INFO: &[u8] = b"ed25519_encryption";
const KEY_LEN: usize = size_of::<ChaChaKey>();
const NONCE_LEN: usize = size_of::<XNonce>();

/// Opaque wrapper around an x25519 static secret.
pub struct ClientSecretKey(X25519StaticSecret);

/// Generate an ed25519 keypair. Expose an ed25519 public key and x25519 secret.
/// The public key by NEAR to encrypt responses.
pub fn generate_client_keypair() -> ClientKeyPair {
    let (_, result) = generate_client_keypair_inner();
    result
}

fn generate_client_keypair_inner() -> ([u8; KEY_LEN], ClientKeyPair) {
    // NEAR uses the Edwards/ed25519 curve points for transmitting public keys.
    // Since we can't convert Montgomery/x25519 points to Edwards/ed25519 without
    // providing a sign (+/-), we need to generate a ed25519 key pair, and convert.
    //
    // The relationship between the two curves/algorithms is as follows:
    //
    //  ed25519 secret ──────► x25519 secret
    //       │                      │
    //       ▼                      ▼
    //  ed25519 public ──────► x25519 public
    //
    // Following is equivalent to the first part of
    // ed25519_dalek::SigningKey::generate
    let mut seed = [0u8; KEY_LEN];
    OsRng.fill_bytes(&mut seed);
    // Equivalent to ed25519_dalek::ExpandedSecretKey::from
    let hash = Sha512::digest(&seed);
    // Equivalent to ed25519_dalek::ExpandedSecretKey::from_bytes
    // This is the expanded secret key/scalar.
    let scalar_bytes = clamp_integer(hash[..KEY_LEN].try_into().unwrap());

    // Multiply by Edwards curve base point to create the uncompressed public
    // key/point.
    let point = EdwardsPoint::mul_base_clamped(scalar_bytes);
    // Compress the point so it can be used/transmitted externally.
    // This is the final public key.
    let public_key = point.compress().to_bytes().to_vec();

    let result = ClientKeyPair {
        public_key,
        secret_key: Box::new(ClientSecretKey(X25519StaticSecret::from(scalar_bytes))),
    };
    (seed, result)
}

/// Converts an ed25519 public key to its x25519 Montgomery equivalent.
fn convert_model_public_key_to_x25519(model_public_key: &[u8]) -> Result<X25519PublicKey, ()> {
    let model_pub_bytes: [u8; KEY_LEN] = model_public_key.try_into().map_err(|_| ())?;
    let model_edwards = CompressedEdwardsY(model_pub_bytes).decompress().ok_or(())?;
    Ok(X25519PublicKey::from(model_edwards.to_montgomery().to_bytes()))
}

/// Encrypts `plaintext` for the model identified by `model_public_key`.
/// Performs ECDH key exchange with an ephemeral x25519 key pair, then encrypts
/// with XChaCha20-Poly1305. Wire format: [32B ephemeral public | 24B nonce |
/// ciphertext]. Returns empty Vec on error.
pub fn encrypt(plaintext: &CxxString, model_public_key: &CxxVector<u8>) -> Vec<u8> {
    let model_pub_slice = model_public_key.as_slice();
    encrypt_inner(plaintext.as_bytes(), model_pub_slice).unwrap_or_default()
}

fn encrypt_inner(plaintext: &[u8], model_public_key: &[u8]) -> Result<Vec<u8>, ()> {
    let mut rng = OsRng;
    let model_public_key_x25519 = convert_model_public_key_to_x25519(model_public_key)?;

    let ephemeral_secret = X25519EphemeralSecret::random_from_rng(&mut rng);
    let ephemeral_public = X25519PublicKey::from(&ephemeral_secret);

    let shared_secret = ephemeral_secret.diffie_hellman(&model_public_key_x25519);
    let cipher = derive_key_and_init_cipher(shared_secret.as_bytes())?;

    let nonce = XChaCha20Poly1305::generate_nonce(&mut rng);
    let encrypted = cipher.encrypt(&nonce, plaintext).map_err(|_| ())?;

    let mut wire = Vec::with_capacity(KEY_LEN + nonce.len() + encrypted.len());
    wire.extend_from_slice(ephemeral_public.as_bytes());
    wire.extend_from_slice(&nonce);
    wire.extend_from_slice(&encrypted);

    Ok(wire)
}

/// Decrypts `ciphertext` using the client's x25519 static secret.
/// Parses the wire format, performs ECDH with the ephemeral public key, and
/// decrypts. Returns empty String on error.
pub fn decrypt(ciphertext: &CxxVector<u8>, client_secret_key: &ClientSecretKey) -> String {
    decrypt_inner(ciphertext.as_slice(), client_secret_key).unwrap_or_default()
}

fn decrypt_inner(ciphertext: &[u8], client_secret_key: &ClientSecretKey) -> Result<String, ()> {
    if ciphertext.len() < KEY_LEN + NONCE_LEN {
        return Err(());
    }

    let ephemeral_pub_bytes: [u8; KEY_LEN] = ciphertext[..KEY_LEN].try_into().map_err(|_| ())?;
    let nonce = XNonce::from_slice(&ciphertext[KEY_LEN..KEY_LEN + NONCE_LEN]);
    let encrypted = &ciphertext[KEY_LEN + NONCE_LEN..];

    let ephemeral_public = X25519PublicKey::from(ephemeral_pub_bytes);
    let shared_secret = client_secret_key.0.diffie_hellman(&ephemeral_public);
    let cipher = derive_key_and_init_cipher(shared_secret.as_bytes())?;
    let plaintext = cipher.decrypt(nonce, encrypted).map_err(|_| ())?;

    String::from_utf8(plaintext).map_err(|_| ())
}

fn derive_key_and_init_cipher(shared_secret: &[u8]) -> Result<XChaCha20Poly1305, ()> {
    let hkdf = Hkdf::<Sha256>::new(None, shared_secret);
    let mut key_bytes = [0u8; KEY_LEN];
    hkdf.expand(HKDF_INFO, &mut key_bytes).map_err(|_| ())?;
    Ok(XChaCha20Poly1305::new(&ChaChaKey::from(key_bytes)))
}

#[cfg(test)]
mod tests {
    use super::*;
    use ed25519_dalek::SigningKey;

    #[test]
    fn basic() {
        let keypair = generate_client_keypair();
        assert_eq!(keypair.public_key.len(), KEY_LEN);

        let enc = encrypt_inner(b"hello", &keypair.public_key).expect("encrypt failed");
        assert!(!enc.is_empty());

        let dec = decrypt_inner(&enc, &keypair.secret_key).expect("decrypt failed");
        assert_eq!(dec, "hello");
    }

    #[test]
    fn decrypt_with_wrong_key_fails() {
        let keypair = generate_client_keypair();
        let wrong_keypair = generate_client_keypair();

        let enc = encrypt_inner(b"hello", &keypair.public_key).expect("encrypt failed");

        let dec = decrypt_inner(&enc, &wrong_keypair.secret_key);
        assert!(dec.is_err());
    }

    #[test]
    fn client_key_pair_matches_ed25519_dalek() {
        let (seed, result) = generate_client_keypair_inner();
        let secret_key = result.secret_key;

        let signing_key = SigningKey::from_bytes(&seed);

        // ed25519-dalek secret scalar == calculated secret scalar (same scalar after
        // SHA-512+clamp)
        assert_eq!(clamp_integer(signing_key.to_scalar_bytes()), secret_key.0.to_bytes());
        // ed25519-dalek public key == calculated ed25519 public key
        assert_eq!(signing_key.verifying_key().to_bytes(), result.public_key.as_slice());

        // x25519-dalek public key == ed25519-dalek public key converted to Montgomery
        let x25519_public = X25519PublicKey::from(&secret_key.0);
        let montgomery = signing_key.verifying_key().to_montgomery();
        assert_eq!(x25519_public.as_bytes(), montgomery.as_bytes());
    }

    #[test]
    fn convert_model_public_key_to_x25519_matches_ed25519_dalek() {
        let signing_key = SigningKey::generate(&mut OsRng);
        let ed25519_pub_bytes = signing_key.verifying_key().to_bytes();

        let x25519_pub =
            convert_model_public_key_to_x25519(&ed25519_pub_bytes).expect("conversion failed");

        // ed25519-dalek public converted to x25519 == ed25519 public converted to
        // Montgomery
        let montgomery = signing_key.verifying_key().to_montgomery();
        assert_eq!(x25519_pub.as_bytes(), montgomery.as_bytes());
    }
}
