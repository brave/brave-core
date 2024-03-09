#![feature(test)]

extern crate test;

use arrayref::array_ref;
use libsecp256k1::{sign, Message, SecretKey};
use secp256k1_test::{rand::thread_rng, Secp256k1};
use test::Bencher;

#[bench]
fn bench_sign_message(b: &mut Bencher) {
    let secp256k1 = Secp256k1::new();
    let message = Message::parse(&[5u8; 32]);
    let (secp_privkey, _) = secp256k1.generate_keypair(&mut thread_rng());
    let seckey = SecretKey::parse(array_ref!(secp_privkey, 0, 32)).unwrap();

    b.iter(|| {
        let _ = sign(&message, &seckey);
    });
}
