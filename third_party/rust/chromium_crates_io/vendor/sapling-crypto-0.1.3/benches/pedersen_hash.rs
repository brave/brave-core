use criterion::{criterion_group, criterion_main, Criterion};
use rand_core::{OsRng, RngCore};
use sapling_crypto::pedersen_hash::{pedersen_hash, Personalization};

#[cfg(unix)]
use pprof::criterion::{Output, PProfProfiler};

fn bench_pedersen_hash(c: &mut Criterion) {
    let rng = &mut OsRng;
    let bits = (0..510)
        .map(|_| (rng.next_u32() % 2) != 0)
        .collect::<Vec<_>>();
    let personalization = Personalization::MerkleTree(31);

    c.bench_function("pedersen-hash", |b| {
        b.iter(|| pedersen_hash(personalization, bits.clone()))
    });
}

#[cfg(unix)]
criterion_group! {
    name = benches;
    config = Criterion::default().with_profiler(PProfProfiler::new(100, Output::Flamegraph(None)));
    targets = bench_pedersen_hash
}
#[cfg(not(unix))]
criterion_group!(benches, bench_pedersen_hash);
criterion_main!(benches);
