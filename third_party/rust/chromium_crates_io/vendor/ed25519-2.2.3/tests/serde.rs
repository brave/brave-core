//! Tests for serde serializers/deserializers

#![cfg(feature = "serde")]

use ed25519::{Signature, SignatureBytes};
use hex_literal::hex;

const EXAMPLE_SIGNATURE: SignatureBytes = hex!(
    "3f3e3d3c3b3a393837363534333231302f2e2d2c2b2a29282726252423222120"
    "1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a09080706050403020100"
);

#[test]
fn test_serialize() {
    let signature = Signature::try_from(&EXAMPLE_SIGNATURE[..]).unwrap();
    dbg!(&signature);
    let encoded_signature: Vec<u8> = bincode::serialize(&signature).unwrap();
    assert_eq!(&EXAMPLE_SIGNATURE[..], &encoded_signature[..]);
}

#[test]
fn test_deserialize() {
    let signature = bincode::deserialize::<Signature>(&EXAMPLE_SIGNATURE).unwrap();
    assert_eq!(EXAMPLE_SIGNATURE, signature.to_bytes());
}

#[cfg(feature = "serde_bytes")]
#[test]
fn test_serialize_bytes() {
    use bincode::Options;

    let signature = Signature::try_from(&EXAMPLE_SIGNATURE[..]).unwrap();

    let mut encoded_signature = Vec::new();
    let options = bincode::DefaultOptions::new()
        .with_fixint_encoding()
        .allow_trailing_bytes();
    let mut serializer = bincode::Serializer::new(&mut encoded_signature, options);
    serde_bytes::serialize(&signature, &mut serializer).unwrap();

    let mut expected = Vec::from(Signature::BYTE_SIZE.to_le_bytes());
    expected.extend(&EXAMPLE_SIGNATURE[..]);
    assert_eq!(&expected[..], &encoded_signature[..]);
}

#[cfg(feature = "serde_bytes")]
#[test]
fn test_deserialize_bytes() {
    use bincode::Options;

    let mut encoded_signature = Vec::from(Signature::BYTE_SIZE.to_le_bytes());
    encoded_signature.extend(&EXAMPLE_SIGNATURE[..]);

    let options = bincode::DefaultOptions::new()
        .with_fixint_encoding()
        .allow_trailing_bytes();
    let mut deserializer = bincode::de::Deserializer::from_slice(&encoded_signature[..], options);

    let signature: Signature = serde_bytes::deserialize(&mut deserializer).unwrap();

    assert_eq!(EXAMPLE_SIGNATURE, signature.to_bytes());
}
