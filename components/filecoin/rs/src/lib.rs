use bls_signatures::Serialize;
pub mod message;
pub mod signature;
use crate::signature::transaction_sign;

#[cxx::bridge(namespace = filecoin)]
mod ffi {
    extern "Rust" {
        fn bls_private_key_to_public_key(private_key: &[u8; 32]) -> [u8; 48];
        fn transaction_sign(transaction: &str, private_key_base64: &str) -> String;
    }
}

/// Generates a public key from the private key
/// Original implementation in Filecoin xFFI project:
/// <https://github.com/filecoin-project/filecoin-ffi/blob/master/rust/src/bls/api.rs#L324>
fn bls_private_key_to_public_key(private_key: &[u8; 32]) -> [u8; 48] {
    let mut public_key: [u8; 48] = [0; 48];
    let wrapped_private_key = bls_signatures::PrivateKey::from_bytes(private_key);
    if let Ok(wrapped_private_key) = wrapped_private_key {
        wrapped_private_key
            .public_key()
            .write_bytes(&mut public_key.as_mut())
            .expect("preallocated");
    }
    return public_key;
}
