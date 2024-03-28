#[macro_use]
extern crate bencher;

use bencher::Bencher;
use hkdf::Hkdf;
use sha2::Sha256;

fn sha256_10(b: &mut Bencher) {
    let mut okm = vec![0u8; 10];
    b.iter(|| Hkdf::<Sha256>::new(Some(&[]), &[]).expand(&[], &mut okm));
    b.bytes = 10u64;
}

fn sha256_1k(b: &mut Bencher) {
    let mut okm = vec![0u8; 1024];
    b.iter(|| Hkdf::<Sha256>::new(Some(&[]), &[]).expand(&[], &mut okm));
    b.bytes = 1024u64;
}

fn sha256_8k(b: &mut Bencher) {
    let mut okm = vec![0u8; 8000];
    b.iter(|| Hkdf::<Sha256>::new(Some(&[]), &[]).expand(&[], &mut okm));
    b.bytes = 8000u64;
}

// note: SHA-256 output limit is 255*32=8160 bytes

benchmark_group!(benches, sha256_10, sha256_1k, sha256_8k);
benchmark_main!(benches);
