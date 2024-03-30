use criterion::{black_box, criterion_group, criterion_main, Criterion};
use rand::Rng;

use multibase::{decode, encode, Base};

fn bench_encode(c: &mut Criterion) {
    let mut rng = rand::thread_rng();
    let data: Vec<u8> = (0..1024).map(|_| rng.gen()).collect();

    let mut group = c.benchmark_group("encode");
    group.bench_function("base32", |b| {
        b.iter(|| {
            let _ = black_box(encode(Base::Base32Upper, &data));
        })
    });
    group.bench_function("base58btc", |b| {
        b.iter(|| {
            let _ = black_box(encode(Base::Base58Btc, &data));
        })
    });
    group.bench_function("base64", |b| {
        b.iter(|| {
            let _ = black_box(encode(Base::Base64, &data));
        })
    });
    group.finish();
}

fn bench_decode(c: &mut Criterion) {
    let mut rng = rand::thread_rng();
    let data: Vec<usize> = (0..1024).map(|_| rng.gen()).collect();

    let base32 = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    let base58 = b"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    let base64 = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    let mut base32_data = data
        .iter()
        .map(|i| base32[i % 31] as char)
        .collect::<String>();
    base32_data.insert(0, Base::Base32Upper.code());
    let mut base58_data = data
        .iter()
        .map(|i| base58[i % 57] as char)
        .collect::<String>();
    base58_data.insert(0, Base::Base58Btc.code());
    let mut base64_data = data
        .iter()
        .map(|i| base64[i % 64] as char)
        .collect::<String>();
    base64_data.insert(0, Base::Base64.code());

    let mut group = c.benchmark_group("decode");
    group.bench_function("base32", |b| {
        b.iter(|| {
            let _ = black_box(decode(&base32_data).unwrap());
        })
    });
    group.bench_function("base58btc", |b| {
        b.iter(|| {
            let _ = black_box(decode(&base58_data).unwrap());
        })
    });
    group.bench_function("base64", |b| {
        b.iter(|| {
            let _ = black_box(decode(&base64_data).unwrap());
        })
    });
    group.finish();
}

criterion_group!(benches, bench_encode, bench_decode);
criterion_main!(benches);
