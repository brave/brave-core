use criterion::{criterion_group, criterion_main, Criterion};
use orchard::keys::{FullViewingKey, Scope, SpendingKey};

fn key_derivation(c: &mut Criterion) {
    // Meaningless random spending key.
    let sk = SpendingKey::from_bytes([
        0x2e, 0x0f, 0xd6, 0xc0, 0xed, 0x0b, 0xcf, 0xd8, 0x07, 0xf5, 0xdb, 0xff, 0x47, 0x4e, 0xdc,
        0x78, 0x8c, 0xe0, 0x09, 0x30, 0x66, 0x10, 0x1e, 0x95, 0x82, 0x87, 0xb1, 0x00, 0x50, 0x9b,
        0xf7, 0x9a,
    ])
    .unwrap();
    let fvk = FullViewingKey::from(&sk);

    c.bench_function("derive_fvk", |b| b.iter(|| FullViewingKey::from(&sk)));
    c.bench_function("default_address", |b| {
        b.iter(|| fvk.address_at(0u32, Scope::External))
    });
}

criterion_group!(benches, key_derivation);
criterion_main!(benches);
