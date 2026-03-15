/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use chacha20poly1305::{
    aead::{Aead, AeadCore, KeyInit},
    Key as ChaChaKey, XChaCha20Poly1305, XNonce,
};
use curve25519_dalek::{edwards::CompressedEdwardsY, scalar::clamp_integer, EdwardsPoint};
use hkdf::Hkdf;
use rand::{rngs::OsRng, RngCore};
use sha2::{Digest, Sha256, Sha512};
use x25519_dalek::{
    EphemeralSecret as X25519EphemeralSecret, PublicKey as X25519PublicKey,
    StaticSecret as X25519StaticSecret,
};

#[cxx::bridge(namespace = "ai_chat")]
mod ffi {
    struct ReceivingKeyPairResult {
        public_key: Vec<u8>,
        secret_key: Box<ReceivingSecretKey>,
    }

    struct EncryptResult {
        error: String,
        ciphertext: Vec<u8>,
    }

    struct DecryptResult {
        error: String,
        plaintext: String,
    }

    extern "Rust" {
        type ReceivingSecretKey;

        fn generate_receiving_keypair() -> ReceivingKeyPairResult;
        fn encrypt(plaintext: &[u8], model_public_key: &[u8]) -> EncryptResult;
        fn decrypt(ciphertext: &[u8], receiving_secret_key: &ReceivingSecretKey) -> DecryptResult;
    }
}

use ffi::*;

const HKDF_INFO: &[u8] = b"ed25519_encryption";
const KEY_LEN: usize = size_of::<ChaChaKey>();
const NONCE_LEN: usize = size_of::<XNonce>();

/// Opaque wrapper around an x25519 static secret.
pub struct ReceivingSecretKey(X25519StaticSecret);

/// Generate an ed25519 keypair. Expose an ed25519 public key and x25519 secret.
/// The public key by NEAR to encrypt responses.
pub fn generate_receiving_keypair() -> ReceivingKeyPairResult {
    let (_, result) = generate_receiving_keypair_inner();
    result
}

fn generate_receiving_keypair_inner() -> ([u8; KEY_LEN], ReceivingKeyPairResult) {
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

    let result = ReceivingKeyPairResult {
        public_key,
        secret_key: Box::new(ReceivingSecretKey(X25519StaticSecret::from(scalar_bytes))),
    };
    (seed, result)
}

/// Converts an ed25519 public key to its x25519 Montgomery equivalent.
fn convert_model_public_key_to_x25519(model_public_key: &[u8]) -> Result<X25519PublicKey, String> {
    let model_pub_bytes: [u8; KEY_LEN] = model_public_key
        .try_into()
        .map_err(|_| format!("expected 32-byte public key, got {}", model_public_key.len()))?;

    let model_edwards = CompressedEdwardsY(model_pub_bytes)
        .decompress()
        .ok_or_else(|| "invalid public key: decompression failed".to_string())?;

    Ok(X25519PublicKey::from(model_edwards.to_montgomery().to_bytes()))
}

/// Encrypts `plaintext` for the model identified by `model_public_key`.
/// Performs ECDH key exchange with an ephemeral x25519 key pair, then encrypts
/// with XChaCha20-Poly1305. Wire format: [32B ephemeral public | 24B nonce |
/// ciphertext].
pub fn encrypt(plaintext: &[u8], model_public_key: &[u8]) -> EncryptResult {
    match encrypt_inner(plaintext, model_public_key) {
        Ok(ciphertext) => EncryptResult { error: String::new(), ciphertext },
        Err(e) => EncryptResult { error: e, ciphertext: Vec::new() },
    }
}

fn encrypt_inner(plaintext: &[u8], model_public_key: &[u8]) -> Result<Vec<u8>, String> {
    let mut rng = OsRng;
    let model_public_key_x25519 = convert_model_public_key_to_x25519(model_public_key)?;

    let ephemeral_secret = X25519EphemeralSecret::random_from_rng(&mut rng);
    let ephemeral_public = X25519PublicKey::from(&ephemeral_secret);

    let shared_secret = ephemeral_secret.diffie_hellman(&model_public_key_x25519);
    let cipher = derive_key_and_init_cipher(shared_secret.as_bytes())?;

    let nonce = XChaCha20Poly1305::generate_nonce(&mut rng);
    let encrypted =
        cipher.encrypt(&nonce, plaintext).map_err(|e| format!("encryption failed: {e}"))?;

    let mut wire = Vec::with_capacity(KEY_LEN + nonce.len() + encrypted.len());
    wire.extend_from_slice(ephemeral_public.as_bytes());
    wire.extend_from_slice(&nonce);
    wire.extend_from_slice(&encrypted);

    Ok(wire)
}

/// Decrypts `ciphertext` using the receiver's x25519 static secret.
/// Parses the wire format, performs ECDH with the ephemeral public key, and
/// decrypts.
pub fn decrypt(ciphertext: &[u8], receiving_secret_key: &ReceivingSecretKey) -> DecryptResult {
    match decrypt_inner(ciphertext, receiving_secret_key) {
        Ok(plaintext) => DecryptResult { error: String::new(), plaintext },
        Err(e) => DecryptResult { error: e, plaintext: String::new() },
    }
}

fn decrypt_inner(
    ciphertext: &[u8],
    receiving_secret_key: &ReceivingSecretKey,
) -> Result<String, String> {
    if ciphertext.len() < KEY_LEN + NONCE_LEN {
        return Err(format!(
            "ciphertext too short: got {} bytes, need at least {}",
            ciphertext.len(),
            KEY_LEN + NONCE_LEN
        ));
    }

    let ephemeral_pub_bytes: [u8; KEY_LEN] = ciphertext[..KEY_LEN].try_into().unwrap();
    let nonce = XNonce::from_slice(&ciphertext[KEY_LEN..KEY_LEN + NONCE_LEN]);
    let encrypted = &ciphertext[KEY_LEN + NONCE_LEN..];

    let ephemeral_public = X25519PublicKey::from(ephemeral_pub_bytes);
    let shared_secret = receiving_secret_key.0.diffie_hellman(&ephemeral_public);
    let cipher = derive_key_and_init_cipher(shared_secret.as_bytes())?;
    let plaintext =
        cipher.decrypt(nonce, encrypted).map_err(|e| format!("decryption failed: {e}"))?;

    String::from_utf8(plaintext).map_err(|e| format!("invalid utf-8 in plaintext: {e}"))
}

fn derive_key_and_init_cipher(shared_secret: &[u8]) -> Result<XChaCha20Poly1305, String> {
    let hkdf = Hkdf::<Sha256>::new(None, shared_secret);
    let mut key_bytes = [0u8; KEY_LEN];
    hkdf.expand(HKDF_INFO, &mut key_bytes).map_err(|e| format!("HKDF expand failed: {e}"))?;
    Ok(XChaCha20Poly1305::new(&ChaChaKey::from(key_bytes)))
}

#[cfg(test)]
mod tests {
    use super::*;
    use ed25519_dalek::SigningKey;

    #[test]
    fn basic() {
        let server_keypair = generate_receiving_keypair();
        assert_eq!(server_keypair.public_key.len(), KEY_LEN);

        let enc = encrypt(b"hello", &server_keypair.public_key);
        assert!(enc.error.is_empty(), "encrypt error: {}", enc.error);
        assert!(!enc.ciphertext.is_empty());

        let dec = decrypt(&enc.ciphertext, &server_keypair.secret_key);
        assert!(dec.error.is_empty(), "decrypt error: {}", dec.error);
        assert_eq!(dec.plaintext, "hello");
    }

    #[test]
    fn decrypt_with_wrong_key_fails() {
        let server_keypair = generate_receiving_keypair();
        let wrong_keypair = generate_receiving_keypair();

        let enc = encrypt(b"hello", &server_keypair.public_key);
        assert!(enc.error.is_empty(), "encrypt error: {}", enc.error);

        let dec = decrypt(&enc.ciphertext, &wrong_keypair.secret_key);
        assert!(!dec.error.is_empty());
        assert!(dec.plaintext.is_empty());
    }

    #[test]
    fn receiving_key_pair_matches_ed25519_dalek() {
        let (seed, result) = generate_receiving_keypair_inner();
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
