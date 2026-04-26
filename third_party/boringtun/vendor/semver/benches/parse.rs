use criterion::{criterion_group, criterion_main, Criterion};
use semver::{Prerelease, Version, VersionReq};
use std::hint;

fn bench(c: &mut Criterion) {
    c.bench_function("parse Prerelease", |b| {
        let text = "x.7.z.92";
        b.iter(|| hint::black_box(text).parse::<Prerelease>().unwrap());
    });
    c.bench_function("parse Version", |b| {
        let text = "1.0.2021-beta+exp.sha.5114f85";
        b.iter(|| hint::black_box(text).parse::<Version>().unwrap());
    });
    c.bench_function("parse VersionReq", |b| {
        let text = ">=1.2.3, <2.0.0";
        b.iter(|| hint::black_box(text).parse::<VersionReq>().unwrap());
    });
}

criterion_group!(benches, bench);
criterion_main!(benches);
