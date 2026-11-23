//! `zeroize` benchmarks.

#![allow(missing_docs, reason = "benchmark")]

use core::hint::black_box;
use criterion::{BenchmarkId, Criterion, Throughput, criterion_group, criterion_main};
use zeroize::{Zeroize, zeroize_bytes};

fn bench_slices(c: &mut Criterion) {
    let mut group = c.benchmark_group("slice/u8");
    for len in [15, 16, 17, 32, 63, 64, 65, 128, 4096, 65536] {
        let mut bytes = vec![0xa5u8; len];
        group.throughput(Throughput::Bytes(len as u64));
        group.bench_function(BenchmarkId::from_parameter(len), |b| {
            b.iter(|| black_box(bytes.as_mut_slice()).zeroize());
        });
        group.bench_function(BenchmarkId::new("zeroize_bytes", len), |b| {
            b.iter(|| zeroize_bytes(black_box(bytes.as_mut_slice())));
        });
    }
    let mut bytes = [0xa5u8; 16];
    group.throughput(Throughput::Bytes(16));
    group.bench_function("fixed16", |b| {
        b.iter(|| black_box(&mut bytes).as_mut_slice().zeroize());
    });
    group.bench_function("zeroize_bytes/fixed16", |b| {
        b.iter(|| zeroize_bytes(black_box(&mut bytes).as_mut_slice()));
    });
    group.finish();
}

criterion_group!(benches, bench_slices);
criterion_main!(benches);
