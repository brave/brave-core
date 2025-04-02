use criterion::{criterion_group, criterion_main, Criterion};
use ff::Field;
use jubjub::*;

fn bench_add_assign(c: &mut Criterion) {
    let mut n = Fq::one();
    let neg_one = -Fq::one();
    c.bench_function("Fq add_assign", |b| {
        b.iter(move || {
            n += &neg_one;
        })
    });
}

fn bench_sub_assign(c: &mut Criterion) {
    let mut n = Fq::one();
    let neg_one = -Fq::one();
    c.bench_function("Fq sub_assign", |b| {
        b.iter(move || {
            n -= &neg_one;
        })
    });
}

fn bench_mul_assign(c: &mut Criterion) {
    let mut n = Fq::one();
    let neg_one = -Fq::one();
    c.bench_function("Fq mul_assign", |b| {
        b.iter(move || {
            n *= &neg_one;
        })
    });
}

fn bench_square(c: &mut Criterion) {
    let n = Fq::one();
    c.bench_function("Fq square", |b| b.iter(move || n.square()));
}

fn bench_invert(c: &mut Criterion) {
    let n = Fq::one();
    c.bench_function("Fq invert", |b| b.iter(move || n.invert()));
}

fn bench_sqrt(c: &mut Criterion) {
    let n = Fq::one().double().double();
    c.bench_function("Fq sqrt", |b| b.iter(move || n.sqrt()));
}

criterion_group!(
    benches,
    bench_add_assign,
    bench_sub_assign,
    bench_mul_assign,
    bench_square,
    bench_invert,
    bench_sqrt,
);
criterion_main!(benches);
