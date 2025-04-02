//! Benchmarks for hashing to the Pasta curves.

use criterion::{criterion_group, criterion_main, Criterion};

use halo2_proofs::arithmetic::CurveExt;
use halo2_proofs::pasta::{pallas, vesta};

fn criterion_benchmark(c: &mut Criterion) {
    bench_hash_to_curve(c);
}

fn bench_hash_to_curve(c: &mut Criterion) {
    let mut group = c.benchmark_group("hash-to-curve");

    let hash_pallas = pallas::Point::hash_to_curve("z.cash:test");
    group.bench_function("Pallas", |b| b.iter(|| hash_pallas(b"benchmark")));

    let hash_vesta = vesta::Point::hash_to_curve("z.cash:test");
    group.bench_function("Vesta", |b| b.iter(|| hash_vesta(b"benchmark")));
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
