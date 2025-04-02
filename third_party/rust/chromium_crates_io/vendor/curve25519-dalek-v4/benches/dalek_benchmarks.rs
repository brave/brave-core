#![allow(non_snake_case)]

use rand::{rngs::OsRng, thread_rng};

use criterion::{
    criterion_main, measurement::Measurement, BatchSize, BenchmarkGroup, BenchmarkId, Criterion,
};

use curve25519_dalek::constants;
use curve25519_dalek::scalar::Scalar;

static BATCH_SIZES: [usize; 5] = [1, 2, 4, 8, 16];
static MULTISCALAR_SIZES: [usize; 13] = [1, 2, 4, 8, 16, 32, 64, 128, 256, 384, 512, 768, 1024];

mod edwards_benches {
    use super::*;

    use curve25519_dalek::edwards::EdwardsPoint;

    fn compress<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        let B = &constants::ED25519_BASEPOINT_POINT;
        c.bench_function("EdwardsPoint compression", move |b| b.iter(|| B.compress()));
    }

    fn decompress<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        let B_comp = &constants::ED25519_BASEPOINT_COMPRESSED;
        c.bench_function("EdwardsPoint decompression", move |b| {
            b.iter(|| B_comp.decompress().unwrap())
        });
    }

    fn consttime_fixed_base_scalar_mul<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        let s = Scalar::from(897987897u64).invert();
        c.bench_function("Constant-time fixed-base scalar mul", move |b| {
            b.iter(|| EdwardsPoint::mul_base(&s))
        });
    }

    fn consttime_variable_base_scalar_mul<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        let B = &constants::ED25519_BASEPOINT_POINT;
        let s = Scalar::from(897987897u64).invert();
        c.bench_function("Constant-time variable-base scalar mul", move |b| {
            b.iter(|| B * s)
        });
    }

    fn vartime_double_base_scalar_mul<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        c.bench_function("Variable-time aA+bB, A variable, B fixed", |bench| {
            let mut rng = thread_rng();
            let A = EdwardsPoint::mul_base(&Scalar::random(&mut rng));
            bench.iter_batched(
                || (Scalar::random(&mut rng), Scalar::random(&mut rng)),
                |(a, b)| EdwardsPoint::vartime_double_scalar_mul_basepoint(&a, &A, &b),
                BatchSize::SmallInput,
            );
        });
    }

    pub(crate) fn edwards_benches() {
        let mut c = Criterion::default();
        let mut g = c.benchmark_group("edwards benches");

        compress(&mut g);
        decompress(&mut g);
        consttime_fixed_base_scalar_mul(&mut g);
        consttime_variable_base_scalar_mul(&mut g);
        vartime_double_base_scalar_mul(&mut g);
    }
}

mod multiscalar_benches {
    use super::*;

    use curve25519_dalek::edwards::EdwardsPoint;
    use curve25519_dalek::edwards::VartimeEdwardsPrecomputation;
    use curve25519_dalek::traits::MultiscalarMul;
    use curve25519_dalek::traits::VartimeMultiscalarMul;
    use curve25519_dalek::traits::VartimePrecomputedMultiscalarMul;

    fn construct_scalars(n: usize) -> Vec<Scalar> {
        let mut rng = thread_rng();
        (0..n).map(|_| Scalar::random(&mut rng)).collect()
    }

    fn construct_points(n: usize) -> Vec<EdwardsPoint> {
        let mut rng = thread_rng();
        (0..n)
            .map(|_| EdwardsPoint::mul_base(&Scalar::random(&mut rng)))
            .collect()
    }

    fn consttime_multiscalar_mul<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        for multiscalar_size in &MULTISCALAR_SIZES {
            c.bench_with_input(
                BenchmarkId::new(
                    "Constant-time variable-base multiscalar multiplication",
                    *multiscalar_size,
                ),
                &multiscalar_size,
                |b, &&size| {
                    let points = construct_points(size);
                    // This is supposed to be constant-time, but we might as well
                    // rerandomize the scalars for every call just in case.
                    b.iter_batched(
                        || construct_scalars(size),
                        |scalars| EdwardsPoint::multiscalar_mul(&scalars, &points),
                        BatchSize::SmallInput,
                    );
                },
            );
        }
    }

    fn vartime_multiscalar_mul<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        for multiscalar_size in &MULTISCALAR_SIZES {
            c.bench_with_input(
                BenchmarkId::new(
                    "Variable-time variable-base multiscalar multiplication",
                    *multiscalar_size,
                ),
                &multiscalar_size,
                |b, &&size| {
                    let points = construct_points(size);
                    // Rerandomize the scalars for every call to prevent
                    // false timings from better caching (e.g., the CPU
                    // cache lifts exactly the right table entries for the
                    // benchmark into the highest cache levels).
                    b.iter_batched(
                        || construct_scalars(size),
                        |scalars| EdwardsPoint::vartime_multiscalar_mul(&scalars, &points),
                        BatchSize::SmallInput,
                    );
                },
            );
        }
    }

    fn vartime_precomputed_pure_static<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        for multiscalar_size in &MULTISCALAR_SIZES {
            c.bench_with_input(
                BenchmarkId::new(
                    "Variable-time fixed-base multiscalar multiplication",
                    multiscalar_size,
                ),
                &multiscalar_size,
                move |b, &&total_size| {
                    let static_size = total_size;

                    let static_points = construct_points(static_size);
                    let precomp = VartimeEdwardsPrecomputation::new(static_points);
                    // Rerandomize the scalars for every call to prevent
                    // false timings from better caching (e.g., the CPU
                    // cache lifts exactly the right table entries for the
                    // benchmark into the highest cache levels).
                    b.iter_batched(
                        || construct_scalars(static_size),
                        |scalars| precomp.vartime_multiscalar_mul(scalars),
                        BatchSize::SmallInput,
                    );
                },
            );
        }
    }

    fn vartime_precomputed_helper<M: Measurement>(
        c: &mut BenchmarkGroup<M>,
        dynamic_fraction: f64,
    ) {
        for multiscalar_size in &MULTISCALAR_SIZES {
            let bench_id = BenchmarkId::new(
                "Variable-time mixed-base",
                format!(
                    "(size: {:?}), ({:.0}pct dyn)",
                    multiscalar_size,
                    100.0 * dynamic_fraction
                ),
            );

            c.bench_with_input(bench_id, &multiscalar_size, move |b, &&total_size| {
                let dynamic_size = ((total_size as f64) * dynamic_fraction) as usize;
                let static_size = total_size - dynamic_size;

                let static_points = construct_points(static_size);
                let dynamic_points = construct_points(dynamic_size);
                let precomp = VartimeEdwardsPrecomputation::new(static_points);
                // Rerandomize the scalars for every call to prevent
                // false timings from better caching (e.g., the CPU
                // cache lifts exactly the right table entries for the
                // benchmark into the highest cache levels).  Timings
                // should be independent of points so we don't
                // randomize them.
                b.iter_batched(
                    || {
                        (
                            construct_scalars(static_size),
                            construct_scalars(dynamic_size),
                        )
                    },
                    |(static_scalars, dynamic_scalars)| {
                        precomp.vartime_mixed_multiscalar_mul(
                            &static_scalars,
                            &dynamic_scalars,
                            &dynamic_points,
                        )
                    },
                    BatchSize::SmallInput,
                );
            });
        }
    }

    pub(crate) fn multiscalar_benches() {
        let mut c = Criterion::default();
        let mut g = c.benchmark_group("multiscalar benches");

        consttime_multiscalar_mul(&mut g);
        vartime_multiscalar_mul(&mut g);
        vartime_precomputed_pure_static(&mut g);

        let dynamic_fracs = [0.0, 0.2, 0.5];

        for frac in dynamic_fracs.iter() {
            vartime_precomputed_helper(&mut g, *frac);
        }
    }
}

mod ristretto_benches {
    use super::*;
    use curve25519_dalek::ristretto::RistrettoPoint;

    fn compress<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        c.bench_function("RistrettoPoint compression", |b| {
            let B = &constants::RISTRETTO_BASEPOINT_POINT;
            b.iter(|| B.compress())
        });
    }

    fn decompress<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        c.bench_function("RistrettoPoint decompression", |b| {
            let B_comp = &constants::RISTRETTO_BASEPOINT_COMPRESSED;
            b.iter(|| B_comp.decompress().unwrap())
        });
    }

    fn double_and_compress_batch<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        for batch_size in &BATCH_SIZES {
            c.bench_with_input(
                BenchmarkId::new("Batch Ristretto double-and-encode", *batch_size),
                &batch_size,
                |b, &&size| {
                    let mut rng = OsRng;
                    let points: Vec<RistrettoPoint> = (0..size)
                        .map(|_| RistrettoPoint::random(&mut rng))
                        .collect();
                    b.iter(|| RistrettoPoint::double_and_compress_batch(&points));
                },
            );
        }
    }

    pub(crate) fn ristretto_benches() {
        let mut c = Criterion::default();
        let mut g = c.benchmark_group("ristretto benches");

        compress(&mut g);
        decompress(&mut g);
        double_and_compress_batch(&mut g);
    }
}

mod montgomery_benches {
    use super::*;
    use curve25519_dalek::montgomery::MontgomeryPoint;

    fn montgomery_ladder<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        c.bench_function("Montgomery pseudomultiplication", |b| {
            let B = constants::X25519_BASEPOINT;
            let s = Scalar::from(897987897u64).invert();
            b.iter(|| B * s);
        });
    }

    fn consttime_fixed_base_scalar_mul<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        let s = Scalar::from(897987897u64).invert();
        c.bench_function("Constant-time fixed-base scalar mul", move |b| {
            b.iter(|| MontgomeryPoint::mul_base(&s))
        });
    }

    pub(crate) fn montgomery_benches() {
        let mut c = Criterion::default();
        let mut g = c.benchmark_group("montgomery benches");

        montgomery_ladder(&mut g);
        consttime_fixed_base_scalar_mul(&mut g);
    }
}

mod scalar_benches {
    use super::*;

    fn scalar_arith<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        let mut rng = thread_rng();

        c.bench_function("Scalar inversion", |b| {
            let s = Scalar::from(897987897u64).invert();
            b.iter(|| s.invert());
        });
        c.bench_function("Scalar addition", |b| {
            b.iter_batched(
                || (Scalar::random(&mut rng), Scalar::random(&mut rng)),
                |(a, b)| a + b,
                BatchSize::SmallInput,
            );
        });
        c.bench_function("Scalar subtraction", |b| {
            b.iter_batched(
                || (Scalar::random(&mut rng), Scalar::random(&mut rng)),
                |(a, b)| a - b,
                BatchSize::SmallInput,
            );
        });
        c.bench_function("Scalar multiplication", |b| {
            b.iter_batched(
                || (Scalar::random(&mut rng), Scalar::random(&mut rng)),
                |(a, b)| a * b,
                BatchSize::SmallInput,
            );
        });
    }

    fn batch_scalar_inversion<M: Measurement>(c: &mut BenchmarkGroup<M>) {
        for batch_size in &BATCH_SIZES {
            c.bench_with_input(
                BenchmarkId::new("Batch scalar inversion", *batch_size),
                &batch_size,
                |b, &&size| {
                    let mut rng = OsRng;
                    let scalars: Vec<Scalar> =
                        (0..size).map(|_| Scalar::random(&mut rng)).collect();
                    b.iter(|| {
                        let mut s = scalars.clone();
                        Scalar::batch_invert(&mut s);
                    });
                },
            );
        }
    }

    pub(crate) fn scalar_benches() {
        let mut c = Criterion::default();
        let mut g = c.benchmark_group("scalar benches");

        scalar_arith(&mut g);
        batch_scalar_inversion(&mut g);
    }
}

criterion_main!(
    scalar_benches::scalar_benches,
    montgomery_benches::montgomery_benches,
    ristretto_benches::ristretto_benches,
    edwards_benches::edwards_benches,
    multiscalar_benches::multiscalar_benches,
);
