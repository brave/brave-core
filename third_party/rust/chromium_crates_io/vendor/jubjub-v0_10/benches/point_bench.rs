use criterion::{criterion_group, criterion_main, Criterion};
use jubjub::*;

// Non-Niels

fn bench_point_doubling(c: &mut Criterion) {
    let a = ExtendedPoint::identity();
    c.bench_function("Jubjub point doubling", |bencher| {
        bencher.iter(move || a.double())
    });
}

fn bench_point_addition(c: &mut Criterion) {
    let a = ExtendedPoint::identity();
    let b = -ExtendedPoint::identity();
    c.bench_function("Jubjub point addition", |bencher| {
        bencher.iter(move || a + b)
    });
}

fn bench_point_subtraction(c: &mut Criterion) {
    let a = ExtendedPoint::identity();
    let b = -ExtendedPoint::identity();
    c.bench_function("Jubjub point subtraction", |bencher| {
        bencher.iter(move || a + b)
    });
}

// Niels

fn bench_cached_point_addition(c: &mut Criterion) {
    let a = ExtendedPoint::identity();
    let b = ExtendedPoint::identity().to_niels();
    c.bench_function("Jubjub cached point addition", |bencher| {
        bencher.iter(move || a + b)
    });
}

fn bench_cached_point_subtraction(c: &mut Criterion) {
    let a = ExtendedPoint::identity();
    let b = ExtendedPoint::identity().to_niels();
    c.bench_function("Jubjub cached point subtraction", |bencher| {
        bencher.iter(move || a + b)
    });
}

fn bench_cached_affine_point_addition(c: &mut Criterion) {
    let a = ExtendedPoint::identity();
    let b = AffinePoint::identity().to_niels();
    c.bench_function("Jubjub cached affine point addition", |bencher| {
        bencher.iter(move || a + b)
    });
}

fn bench_cached_affine_point_subtraction(c: &mut Criterion) {
    let a = ExtendedPoint::identity();
    let b = AffinePoint::identity().to_niels();
    c.bench_function("Jubjub cached affine point subtraction", |bencher| {
        bencher.iter(move || a + b)
    });
}

criterion_group!(
    benches,
    bench_point_doubling,
    bench_point_addition,
    bench_point_subtraction,
    bench_cached_point_addition,
    bench_cached_point_subtraction,
    bench_cached_affine_point_addition,
    bench_cached_affine_point_subtraction,
);
criterion_main!(benches);
