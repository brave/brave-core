#![cfg(feature = "std")]

use libsecp256k1::*;

const DEBUG_SECRET_KEY: [u8; 32] = [1u8; 32];
// Public key for debug secret key
const SERIALIZED_DEBUG_PUBLIC_KEY: &str =
    "\"BBuExVZ7EmRAmV0+1aq6BWXXHhg0YEgZ/5wX9enV3QePcL6vj1iLVBUH/tamQsWrQt/fgSCn9jneUSLUemmo6NE=\"";

fn debug_public_key() -> PublicKey {
    let skey = SecretKey::parse(&DEBUG_SECRET_KEY).unwrap();
    PublicKey::from_secret_key(&skey)
}

#[test]
fn test_serialize_public_key() {
    let pkey = debug_public_key();
    let serialized_pkey = serde_json::to_string(&pkey).unwrap();
    assert_eq!(serialized_pkey, SERIALIZED_DEBUG_PUBLIC_KEY);
}

#[test]
fn test_deserialize_public_key() {
    let pkey: PublicKey = serde_json::from_str(&SERIALIZED_DEBUG_PUBLIC_KEY).unwrap();
    assert_eq!(pkey, debug_public_key());
}

#[test]
fn test_public_key_bincode_serde() {
    let pkey = debug_public_key();
    let serialized_pkey: Vec<u8> = bincode::serialize(&pkey).unwrap();
    let pkey2 = bincode::deserialize(&serialized_pkey).unwrap();
    assert_eq!(pkey, pkey2);
}
