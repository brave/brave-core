#![allow(
    clippy::clone_on_copy,
    clippy::useless_conversion,
    suspicious_double_ref_op
)]

use criterion::{criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};

type StringCow<'s> = std::borrow::Cow<'s, str>;

#[cfg(not(feature = "unstable_bench_subset"))]
pub static FIXTURES: &[&str] = &[
    // Empty handling
    "",
    // Barely used
    "1",
    // kstring's max small-string size
    "123456789012345",
    // Boundary conditions for most small-string optimizations
    "1234567890123456789012",
    "12345678901234567890123",
    "123456789012345678901234",
    "1234567890123456789012345",
    // Small heap
    "1234567890123456789012345678901234567890123456789012345678901234",
    // Large heap
    "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
];

#[cfg(feature = "unstable_bench_subset")]
pub static FIXTURES: &[&str] = &[
    "0123456789",
    "01234567890123456789012345678901234567890123456789012345678901234567890123456789",
];

fn bench_clone(c: &mut Criterion) {
    let mut group = c.benchmark_group("clone");
    for fixture in FIXTURES {
        let len = fixture.len();
        group.throughput(Throughput::Bytes(len as u64));
        group.bench_with_input(
            BenchmarkId::new("StringCow::Borrowed", len),
            &len,
            |b, _| {
                let uut = StringCow::Borrowed(*fixture);
                let uut = criterion::black_box(uut);
                b.iter(|| uut.clone())
            },
        );
        group.bench_with_input(BenchmarkId::new("StringCow::Owned", len), &len, |b, _| {
            let fixture = String::from(*fixture);
            let uut = StringCow::Owned(fixture);
            let uut = criterion::black_box(uut);
            b.iter(|| uut.clone())
        });
        group.bench_with_input(
            BenchmarkId::new("KString::from_static", len),
            &len,
            |b, _| {
                let uut = kstring::KString::from_static(fixture);
                let uut = criterion::black_box(uut);
                b.iter(|| uut.clone())
            },
        );
        group.bench_with_input(BenchmarkId::new("KString::from_ref", len), &len, |b, _| {
            let fixture = String::from(*fixture);
            let uut = kstring::KString::from_ref(&fixture);
            let uut = criterion::black_box(uut);
            b.iter(|| uut.clone())
        });
        group.bench_with_input(
            BenchmarkId::new("KString::from_string", len),
            &len,
            |b, _| {
                let fixture = String::from(*fixture);
                let uut = kstring::KString::from_string(fixture);
                let uut = criterion::black_box(uut);
                b.iter(|| uut.clone())
            },
        );
        #[cfg(not(feature = "unstable_bench_subset"))]
        group.bench_with_input(
            BenchmarkId::new("KStringCow::from_static", len),
            &len,
            |b, _| {
                let uut = kstring::KStringCow::from_static(fixture);
                let uut = criterion::black_box(uut);
                b.iter(|| uut.clone())
            },
        );
        #[cfg(not(feature = "unstable_bench_subset"))]
        group.bench_with_input(
            BenchmarkId::new("KStringCow::from_ref", len),
            &len,
            |b, _| {
                let fixture = String::from(*fixture);
                let uut = kstring::KStringCow::from_ref(&fixture);
                let uut = criterion::black_box(uut);
                b.iter(|| uut.clone())
            },
        );
        #[cfg(not(feature = "unstable_bench_subset"))]
        group.bench_with_input(
            BenchmarkId::new("KStringCow::from_string", len),
            &len,
            |b, _| {
                let fixture = String::from(*fixture);
                let uut = kstring::KStringCow::from_string(fixture);
                let uut = criterion::black_box(uut);
                b.iter(|| uut.clone())
            },
        );
        #[cfg(not(feature = "unstable_bench_subset"))]
        group.bench_with_input(
            BenchmarkId::new("KStringRef::from_static", len),
            &len,
            |b, _| {
                let uut = kstring::KStringRef::from_static(fixture);
                let uut = criterion::black_box(uut);
                b.iter(|| uut.clone())
            },
        );
        #[cfg(not(feature = "unstable_bench_subset"))]
        group.bench_with_input(
            BenchmarkId::new("KStringRef::from_ref", len),
            &len,
            |b, _| {
                let fixture = String::from(*fixture);
                let uut = kstring::KStringRef::from_ref(&fixture);
                let uut = criterion::black_box(uut);
                b.iter(|| uut.clone())
            },
        );
    }
    group.finish();
}

criterion_group!(benches, bench_clone);
criterion_main!(benches);
