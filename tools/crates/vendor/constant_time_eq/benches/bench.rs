use constant_time_eq::{constant_time_eq, constant_time_eq_n};
use criterion::{criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};

fn bench_array(c: &mut Criterion) {
    let mut group = c.benchmark_group("constant_time_eq_n");

    let input = (&[1; 16], &[2; 16]);
    group.throughput(Throughput::Bytes(16));
    group.bench_with_input(BenchmarkId::from_parameter(16), &input, |b, &(x, y)| {
        b.iter(|| constant_time_eq_n(x, y))
    });

    let input = (&[1; 20], &[2; 20]);
    group.throughput(Throughput::Bytes(20));
    group.bench_with_input(BenchmarkId::from_parameter(20), &input, |b, &(x, y)| {
        b.iter(|| constant_time_eq_n(x, y))
    });

    let input = (&[1; 32], &[2; 32]);
    group.throughput(Throughput::Bytes(32));
    group.bench_with_input(BenchmarkId::from_parameter(32), &input, |b, &(x, y)| {
        b.iter(|| constant_time_eq_n(x, y))
    });

    let input = (&[1; 64], &[2; 64]);
    group.throughput(Throughput::Bytes(64));
    group.bench_with_input(BenchmarkId::from_parameter(64), &input, |b, &(x, y)| {
        b.iter(|| constant_time_eq_n(x, y))
    });

    group.finish();
}

fn bench_slice(c: &mut Criterion) {
    let mut group = c.benchmark_group("constant_time_eq");

    let input = (&[1; 65536], &[2; 65536]);
    for &size in &[16, 20, 32, 64, 4 * 1024, 16 * 1024, 64 * 1024] {
        let input = (&input.0[..size], &input.1[..size]);
        group.throughput(Throughput::Bytes(size as u64));
        group.bench_with_input(BenchmarkId::from_parameter(size), &input, |b, &(x, y)| {
            b.iter(|| constant_time_eq(x, y))
        });
    }

    group.finish();
}

criterion_group!(benches, bench_array, bench_slice);
criterion_main!(benches);
