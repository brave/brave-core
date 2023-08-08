use criterion::{black_box, criterion_group, criterion_main, Criterion};
use rand::Rng;

use multihash::{
    Blake2b256, Blake2b512, Blake2s128, Blake2s256, Blake3_256, Hasher, Keccak224, Keccak256,
    Keccak384, Keccak512, Sha1, Sha2_256, Sha2_512, Sha3_224, Sha3_256, Sha3_384, Sha3_512,
    Strobe256, Strobe512,
};

macro_rules! group_digest {
    ($criterion:ident, $( $id:expr => $hash:ident, $input:expr)* ) => {{
        let mut group = $criterion.benchmark_group("digest");
        $(
            group.bench_function($id, |b| {
                b.iter(|| {
                    let mut hasher = $hash::default();
                    hasher.update(black_box($input));
                    let _ = black_box(hasher.finalize());
                })
            });
        )*
        group.finish();
    }};
}

macro_rules! group_stream {
    ($criterion:ident, $( $id:expr => $hash:ident, $input:expr)* ) => {{
        let mut group = $criterion.benchmark_group("stream");
        $(
            group.bench_function($id, |b| {
                b.iter(|| {
                    let input = black_box($input);
                    let mut hasher = <$hash>::default();
                    for i in 0..3 {
                        let start = i * 256;
                        hasher.update(&input[start..(start + 256)]);
                    }
                    let _ = black_box(hasher.finalize());
                })
            });
        )*
        group.finish();
    }};
}

fn bench_digest(c: &mut Criterion) {
    let mut rng = rand::thread_rng();
    let data: Vec<u8> = (0..1024).map(|_| rng.gen()).collect();
    group_digest!(c,
        "sha1" => Sha1, &data
        "sha2_256" => Sha2_256, &data
        "sha2_512" => Sha2_512, &data
        "sha3_224" => Sha3_224, &data
        "sha3_256" => Sha3_256, &data
        "sha3_384" => Sha3_384, &data
        "sha3_512" => Sha3_512, &data
        "keccak_224" => Keccak224, &data
        "keccak_256" => Keccak256, &data
        "keccak_384" => Keccak384, &data
        "keccak_512" => Keccak512, &data
        "blake2b_256" => Blake2b256, &data
        "blake2b_512" => Blake2b512, &data
        "blake2s_128" => Blake2s128, &data
        "blake2s_256" => Blake2s256, &data
        "blake3_256" => Blake3_256, &data
        "strobe_256" => Strobe256, &data
        "strobe_512" => Strobe512, &data
    );
}

/// Chunks the data into 256-byte slices.
fn bench_stream(c: &mut Criterion) {
    let mut rng = rand::thread_rng();
    let data: Vec<u8> = (0..1024).map(|_| rng.gen()).collect();
    group_stream!(c,
        "sha1" => Sha1, &data
        "sha2_256" => Sha2_256, &data
        "sha2_512" => Sha2_512, &data
        "sha3_224" => Sha3_224, &data
        "sha3_256" => Sha3_256, &data
        "sha3_384" => Sha3_384, &data
        "sha3_512" => Sha3_512, &data
        "keccak_224" => Keccak224, &data
        "keccak_256" => Keccak256, &data
        "keccak_384" => Keccak384, &data
        "keccak_512" => Keccak512, &data
        "blake2b_256" => Blake2b256, &data
        "blake2b_512" => Blake2b512, &data
        "blake2s_128" => Blake2s128, &data
        "blake2s_256" => Blake2s256, &data
        "blake3_256" => Blake3_256, &data
        "strobe_256" => Strobe256, &data
        "strobe_512" => Strobe512, &data
    );
}

criterion_group!(benches, bench_digest, bench_stream);
criterion_main!(benches);
