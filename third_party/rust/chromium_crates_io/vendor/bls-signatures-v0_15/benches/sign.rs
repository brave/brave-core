#![feature(test)]
extern crate test;

use test::{black_box, Bencher};

use bls_signatures::*;
use rand::Rng;

#[bench]
fn sign_64b(b: &mut Bencher) {
    let rng = &mut rand::thread_rng();

    let private_key = PrivateKey::generate(rng);
    let msg: Vec<u8> = (0..64).map(|_| rng.gen()).collect();

    b.iter(|| black_box(private_key.sign(&msg)))
}
