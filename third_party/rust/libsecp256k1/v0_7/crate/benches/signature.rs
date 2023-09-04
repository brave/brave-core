#![feature(test)]

extern crate test;

use libsecp256k1::Signature;
use secp256k1_test::{rand::thread_rng, Message as SecpMessage, Secp256k1};
use test::Bencher;

#[bench]
fn bench_signature_parse(b: &mut Bencher) {
    let secp256k1 = Secp256k1::new();
    let message_arr = [5u8; 32];
    let (privkey, _) = secp256k1.generate_keypair(&mut thread_rng());
    let message = SecpMessage::from_slice(&message_arr).unwrap();
    let signature = secp256k1.sign(&message, &privkey);
    let signature_arr = signature.serialize_compact();
    assert!(signature_arr.len() == 64);
    let mut signature_a = [0u8; 64];
    signature_a.copy_from_slice(&signature_arr[0..64]);

    b.iter(|| {
        let _signature = Signature::parse_standard_slice(&signature_a);
    });
}

#[bench]
fn bench_signature_serialize(b: &mut Bencher) {
    let secp256k1 = Secp256k1::new();
    let message_arr = [5u8; 32];
    let (privkey, _) = secp256k1.generate_keypair(&mut thread_rng());
    let message = SecpMessage::from_slice(&message_arr).unwrap();
    let signature = secp256k1.sign(&message, &privkey);
    let signature_arr = signature.serialize_compact();
    assert!(signature_arr.len() == 64);
    let mut signature_a = [0u8; 64];
    signature_a.copy_from_slice(&signature_arr[0..64]);
    let signature = Signature::parse_standard_slice(&signature_a).expect("parsed signature");

    b.iter(|| {
        let _serialized = signature.serialize();
    });
}
