#[macro_use]
extern crate criterion;

use crate::arithmetic::best_multiexp;
use crate::pasta::{EqAffine, Fp};
use crate::poly::commitment::Params;
use criterion::{BenchmarkId, Criterion};
use group::ff::Field;
use halo2_proofs::*;
use rand_core::OsRng;

fn criterion_benchmark(c: &mut Criterion) {
    let mut group = c.benchmark_group("msm");
    for k in 8..16 {
        group
            .bench_function(BenchmarkId::new("k", k), |b| {
                let coeffs = (0..(1 << k)).map(|_| Fp::random(OsRng)).collect::<Vec<_>>();
                let bases = Params::<EqAffine>::new(k).get_g();

                b.iter(|| best_multiexp(&coeffs, &bases))
            })
            .sample_size(30);
    }
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
