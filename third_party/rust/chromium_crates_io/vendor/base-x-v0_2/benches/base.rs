#[macro_use]
extern crate bencher;
extern crate base_x;
extern crate rand;

use base_x::{decode, encode, Alphabet};
use bencher::Bencher;

fn random_input(size: usize) -> Vec<u8> {
    let mut v = vec![0; size];

    for x in v.iter_mut() {
        *x = rand::random()
    }

    v
}

fn test_decode<A: Alphabet + Copy>(bench: &mut Bencher, alph: A) {
    let input = random_input(100);
    let out = encode(alph, &input);

    bench.iter(|| decode(alph, &out).unwrap());
}

fn test_encode<A: Alphabet + Copy>(bench: &mut Bencher, alph: A) {
    let input = random_input(100);

    bench.iter(|| encode(alph, &input));
}

// Actual benchmarks

// Encode UTF-8
fn encode_base2(bench: &mut Bencher) {
    const ALPH: &str = "01";
    test_encode(bench, ALPH);
}

fn encode_base16(bench: &mut Bencher) {
    const ALPH: &str = "0123456789abcdef";
    test_encode(bench, ALPH);
}

fn encode_base58(bench: &mut Bencher) {
    const ALPH: &str = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    test_encode(bench, ALPH);
}

// Decode UTF-8
fn decode_base2(bench: &mut Bencher) {
    const ALPH: &str = "01";
    test_decode(bench, ALPH);
}

fn decode_base16(bench: &mut Bencher) {
    const ALPH: &str = "0123456789abcdef";
    test_decode(bench, ALPH);
}

fn decode_base58(bench: &mut Bencher) {
    const ALPH: &str = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    test_decode(bench, ALPH);
}

benchmark_group!(
    benches,
    encode_base2,
    decode_base2,
    encode_base16,
    decode_base16,
    encode_base58,
    decode_base58
);
benchmark_main!(benches);
