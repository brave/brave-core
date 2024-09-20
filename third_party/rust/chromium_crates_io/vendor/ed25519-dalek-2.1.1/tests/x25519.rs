//! Tests for converting Ed25519 keys into X25519 (Montgomery form) keys.

use curve25519_dalek::scalar::{clamp_integer, Scalar};
use ed25519_dalek::SigningKey;
use hex_literal::hex;
use sha2::{Digest, Sha512};
use x25519_dalek::{PublicKey as XPublicKey, StaticSecret as XStaticSecret};

/// Tests that X25519 Diffie-Hellman works when using keys converted from Ed25519.
// TODO: generate test vectors using another implementation of Ed25519->X25519
#[test]
fn ed25519_to_x25519_dh() {
    // Keys from RFC8032 test vectors (from section 7.1)
    let ed_secret_key_a = hex!("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60");
    let ed_secret_key_b = hex!("4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb");

    let ed_signing_key_a = SigningKey::from_bytes(&ed_secret_key_a);
    let ed_signing_key_b = SigningKey::from_bytes(&ed_secret_key_b);

    // Create an x25519 static secret from the ed25519 signing key
    let scalar_bytes_a = ed_signing_key_a.to_scalar_bytes();
    let scalar_bytes_b = ed_signing_key_b.to_scalar_bytes();
    let x_static_secret_a = XStaticSecret::from(scalar_bytes_a);
    let x_static_secret_b = XStaticSecret::from(scalar_bytes_b);

    // Compute the secret scalars too
    let scalar_a = ed_signing_key_a.to_scalar();
    let scalar_b = ed_signing_key_b.to_scalar();

    // Compare the scalar bytes to the first 32 bytes of SHA-512(secret_key). We have to clamp and
    // reduce the SHA-512 output because that's what the spec does before using the scalars for
    // anything.
    assert_eq!(scalar_bytes_a, &Sha512::digest(ed_secret_key_a)[..32]);
    assert_eq!(scalar_bytes_b, &Sha512::digest(ed_secret_key_b)[..32]);

    // Compare the scalar with the clamped and reduced scalar bytes
    assert_eq!(
        scalar_a,
        Scalar::from_bytes_mod_order(clamp_integer(scalar_bytes_a))
    );
    assert_eq!(
        scalar_b,
        Scalar::from_bytes_mod_order(clamp_integer(scalar_bytes_b))
    );

    let x_public_key_a = XPublicKey::from(&x_static_secret_a);
    let x_public_key_b = XPublicKey::from(&x_static_secret_b);
    assert_eq!(
        x_public_key_a.to_bytes(),
        hex!("d85e07ec22b0ad881537c2f44d662d1a143cf830c57aca4305d85c7a90f6b62e")
    );
    assert_eq!(
        x_public_key_b.to_bytes(),
        hex!("25c704c594b88afc00a76b69d1ed2b984d7e22550f3ed0802d04fbcd07d38d47")
    );

    // Test the claim made in the comments of SigningKey::to_scalar_bytes, i.e., that the resulting
    // scalar is a valid private key for the x25519 pubkey represented by
    // `sk.verifying_key().to_montgomery()`
    assert_eq!(
        ed_signing_key_a.verifying_key().to_montgomery().as_bytes(),
        x_public_key_a.as_bytes()
    );
    assert_eq!(
        ed_signing_key_b.verifying_key().to_montgomery().as_bytes(),
        x_public_key_b.as_bytes()
    );

    // Check that Diffie-Hellman works
    let expected_shared_secret =
        hex!("5166f24a6918368e2af831a4affadd97af0ac326bdf143596c045967cc00230e");
    assert_eq!(
        x_static_secret_a.diffie_hellman(&x_public_key_b).to_bytes(),
        expected_shared_secret,
    );
    assert_eq!(
        x_static_secret_b.diffie_hellman(&x_public_key_a).to_bytes(),
        expected_shared_secret,
    );
}
