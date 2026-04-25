#[macro_use]
extern crate criterion;

extern crate bls12_381;
use bls12_381::hash_to_curve::*;
use bls12_381::*;

use criterion::{black_box, Criterion};

fn criterion_benchmark(c: &mut Criterion) {
    // G1Projective
    {
        let name = "G1Projective";

        let message: &[u8] = b"test message";
        let dst: &[u8] = b"test DST";

        c.bench_function(
            &format!("{} encode_to_curve SSWU SHA-256", name),
            move |b| {
                b.iter(|| {
                    <G1Projective as HashToCurve<ExpandMsgXmd<sha2::Sha256>>>::encode_to_curve(
                        black_box(message),
                        black_box(dst),
                    )
                })
            },
        );
        c.bench_function(&format!("{} hash_to_curve SSWU SHA-256", name), move |b| {
            b.iter(|| {
                <G1Projective as HashToCurve<ExpandMsgXmd<sha2::Sha256>>>::hash_to_curve(
                    black_box(message),
                    black_box(dst),
                )
            })
        });
    }
    // G2Projective
    {
        let name = "G2Projective";

        let message: &[u8] = b"test message";
        let dst: &[u8] = b"test DST";

        c.bench_function(
            &format!("{} encode_to_curve SSWU SHA-256", name),
            move |b| {
                b.iter(|| {
                    <G2Projective as HashToCurve<ExpandMsgXmd<sha2::Sha256>>>::encode_to_curve(
                        black_box(message),
                        black_box(dst),
                    )
                })
            },
        );
        c.bench_function(&format!("{} hash_to_curve SSWU SHA-256", name), move |b| {
            b.iter(|| {
                <G2Projective as HashToCurve<ExpandMsgXmd<sha2::Sha256>>>::hash_to_curve(
                    black_box(message),
                    black_box(dst),
                )
            })
        });
    }
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
