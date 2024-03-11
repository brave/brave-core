///! Benchmarks for the Fp field.
use criterion::{criterion_group, criterion_main, Bencher, Criterion};

use rand::SeedableRng;
use rand_xorshift::XorShiftRng;

use ff::{Field, PrimeField};
use pasta_curves::Fp;

fn criterion_benchmark(c: &mut Criterion) {
    let mut group = c.benchmark_group("Fp");

    group.bench_function("double", bench_fp_double);
    group.bench_function("add_assign", bench_fp_add_assign);
    group.bench_function("sub_assign", bench_fp_sub_assign);
    group.bench_function("mul_assign", bench_fp_mul_assign);
    group.bench_function("square", bench_fp_square);
    group.bench_function("invert", bench_fp_invert);
    group.bench_function("neg", bench_fp_neg);
    group.bench_function("sqrt", bench_fp_sqrt);
    group.bench_function("to_repr", bench_fp_to_repr);
    group.bench_function("from_repr", bench_fp_from_repr);
}

fn bench_fp_double(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<Fp> = (0..SAMPLES).map(|_| Fp::random(&mut rng)).collect();

    let mut count = 0;
    b.iter(|| {
        let mut tmp = v[count];
        tmp = tmp.double();
        count = (count + 1) % SAMPLES;
        tmp
    });
}

fn bench_fp_add_assign(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<(Fp, Fp)> = (0..SAMPLES)
        .map(|_| (Fp::random(&mut rng), Fp::random(&mut rng)))
        .collect();

    let mut count = 0;
    b.iter(|| {
        let mut tmp = v[count].0;
        tmp += &v[count].1;
        count = (count + 1) % SAMPLES;
        tmp
    });
}

fn bench_fp_sub_assign(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<(Fp, Fp)> = (0..SAMPLES)
        .map(|_| (Fp::random(&mut rng), Fp::random(&mut rng)))
        .collect();

    let mut count = 0;
    b.iter(|| {
        let mut tmp = v[count].0;
        tmp -= &v[count].1;
        count = (count + 1) % SAMPLES;
        tmp
    });
}

fn bench_fp_mul_assign(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<(Fp, Fp)> = (0..SAMPLES)
        .map(|_| (Fp::random(&mut rng), Fp::random(&mut rng)))
        .collect();

    let mut count = 0;
    b.iter(|| {
        let mut tmp = v[count].0;
        tmp *= &v[count].1;
        count = (count + 1) % SAMPLES;
        tmp
    });
}

fn bench_fp_square(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<Fp> = (0..SAMPLES).map(|_| Fp::random(&mut rng)).collect();

    let mut count = 0;
    b.iter(|| {
        let mut tmp = v[count];
        tmp = tmp.square();
        count = (count + 1) % SAMPLES;
        tmp
    });
}

fn bench_fp_invert(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<Fp> = (0..SAMPLES).map(|_| Fp::random(&mut rng)).collect();

    let mut count = 0;
    b.iter(|| {
        count = (count + 1) % SAMPLES;
        v[count].invert()
    });
}

fn bench_fp_neg(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<Fp> = (0..SAMPLES).map(|_| Fp::random(&mut rng)).collect();

    let mut count = 0;
    b.iter(|| {
        let mut tmp = v[count];
        tmp = tmp.neg();
        count = (count + 1) % SAMPLES;
        tmp
    });
}

fn bench_fp_sqrt(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<Fp> = (0..SAMPLES)
        .map(|_| {
            let tmp = Fp::random(&mut rng);
            tmp.square()
        })
        .collect();

    let mut count = 0;
    b.iter(|| {
        count = (count + 1) % SAMPLES;
        v[count].sqrt()
    });
}

fn bench_fp_to_repr(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<Fp> = (0..SAMPLES).map(|_| Fp::random(&mut rng)).collect();

    let mut count = 0;
    b.iter(|| {
        count = (count + 1) % SAMPLES;
        v[count].to_repr()
    });
}

fn bench_fp_from_repr(b: &mut Bencher) {
    const SAMPLES: usize = 1000;

    let mut rng = XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);

    let v: Vec<<Fp as PrimeField>::Repr> = (0..SAMPLES)
        .map(|_| Fp::random(&mut rng).to_repr())
        .collect();

    let mut count = 0;
    b.iter(|| {
        count = (count + 1) % SAMPLES;
        Fp::from_repr(v[count])
    });
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
