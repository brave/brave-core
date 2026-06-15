use criterion::{criterion_group, criterion_main, BenchmarkId, Criterion};
use ff::Field;
use halo2_gadgets::{
    poseidon::primitives::{self as poseidon, ConstantLength, P128Pow5T3},
    sinsemilla::primitives as sinsemilla,
};

use pasta_curves::pallas;
#[cfg(unix)]
use pprof::criterion::{Output, PProfProfiler};
use rand::{rngs::OsRng, Rng};

fn bench_primitives(c: &mut Criterion) {
    let mut rng = OsRng;

    {
        let mut group = c.benchmark_group("Poseidon");

        let message = [pallas::Base::random(rng), pallas::Base::random(rng)];

        group.bench_function("2-to-1", |b| {
            b.iter(|| {
                poseidon::Hash::<_, P128Pow5T3, ConstantLength<2>, 3, 2>::init().hash(message)
            })
        });
    }

    {
        let mut group = c.benchmark_group("Sinsemilla");

        let hasher = sinsemilla::HashDomain::new("hasher");
        let committer = sinsemilla::CommitDomain::new("committer");
        let bits: Vec<bool> = (0..1086).map(|_| rng.gen()).collect();
        let r = pallas::Scalar::random(rng);

        // Benchmark the input sizes we use in Orchard:
        // - 510 bits for Commit^ivk
        // - 520 bits for MerkleCRH
        // - 1086 bits for NoteCommit
        for size in [510, 520, 1086] {
            group.bench_function(BenchmarkId::new("hash-to-point", size), |b| {
                b.iter(|| hasher.hash_to_point(bits[..size].iter().cloned()))
            });

            group.bench_function(BenchmarkId::new("hash", size), |b| {
                b.iter(|| hasher.hash(bits[..size].iter().cloned()))
            });

            group.bench_function(BenchmarkId::new("commit", size), |b| {
                b.iter(|| committer.commit(bits[..size].iter().cloned(), &r))
            });

            group.bench_function(BenchmarkId::new("short-commit", size), |b| {
                b.iter(|| committer.commit(bits[..size].iter().cloned(), &r))
            });
        }
    }
}

#[cfg(unix)]
criterion_group! {
    name = benches;
    config = Criterion::default().with_profiler(PProfProfiler::new(100, Output::Flamegraph(None)));
    targets = bench_primitives
}
#[cfg(not(unix))]
criterion_group!(benches, bench_primitives);
criterion_main!(benches);
