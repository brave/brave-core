#![feature(test)]
extern crate test;

use test::{black_box, Bencher};

use bls_signatures::*;
use rand::Rng;

#[bench]
fn bench_serialize_private_key_as_bytes(b: &mut Bencher) {
    let private_key = PrivateKey::generate(&mut rand::thread_rng());

    b.iter(|| black_box(private_key.as_bytes()));
}

#[bench]
fn bench_serialize_private_key_from_bytes(b: &mut Bencher) {
    let private_key = PrivateKey::generate(&mut rand::thread_rng());
    let bytes = private_key.as_bytes();

    b.iter(|| black_box(PrivateKey::from_bytes(&bytes).unwrap()));
}

#[bench]
fn bench_serialize_public_key_as_bytes(b: &mut Bencher) {
    let public_key = PrivateKey::generate(&mut rand::thread_rng()).public_key();

    b.iter(|| black_box(public_key.as_bytes()));
}

#[bench]
fn bench_serialize_public_key_from_bytes(b: &mut Bencher) {
    let public_key = PrivateKey::generate(&mut rand::thread_rng()).public_key();
    let bytes = public_key.as_bytes();

    b.iter(|| black_box(PublicKey::from_bytes(&bytes).unwrap()));
}

#[bench]
fn bench_serialize_signature_as_bytes(b: &mut Bencher) {
    let mut rng = rand::thread_rng();
    let private_key = PrivateKey::generate(&mut rng);
    let msg = (0..64).map(|_| rng.gen()).collect::<Vec<u8>>();
    let signature = private_key.sign(&msg);

    b.iter(|| black_box(signature.as_bytes()));
}

#[bench]
fn bench_serialize_signature_from_bytes(b: &mut Bencher) {
    let mut rng = rand::thread_rng();
    let private_key = PrivateKey::generate(&mut rng);
    let msg = (0..64).map(|_| rng.gen()).collect::<Vec<u8>>();
    let signature = private_key.sign(&msg);
    let bytes = signature.as_bytes();

    b.iter(|| black_box(Signature::from_bytes(&bytes).unwrap()));
}
