use strobe_rs::{SecParam, Strobe};

use criterion::{criterion_group, criterion_main, Criterion};

// Literally all these functions (besides ratchet) should have the same runtime. But a benchmark
// can't hurt, I suppose

fn bench_nonmeta(c: &mut Criterion) {
    let mut g = c.benchmark_group("simple bench");

    let mut s = Strobe::new(b"simplebench", SecParam::B256);
    let mut v = [0u8; 256];
    g.bench_function("send_enc", |b| b.iter(|| s.send_enc(&mut v, false)));
    g.bench_function("recv_enc", |b| b.iter(|| s.recv_enc(&mut v, false)));
    g.bench_function("send_clr", |b| b.iter(|| s.send_clr(&v, false)));
    g.bench_function("recv_clr", |b| b.iter(|| s.recv_clr(&v, false)));
    g.bench_function("ad", |b| b.iter(|| s.ad(&v, false)));
    g.bench_function("key", |b| b.iter(|| s.key(&v, false)));
    g.bench_function("prf", |b| b.iter(|| s.prf(&mut v, false)));
    g.bench_function("send_mac", |b| b.iter(|| s.send_mac(&mut v, false)));
    g.bench_function("rachet 16", |b| b.iter(|| s.ratchet(16, false)));
}

fn bench_meta(c: &mut Criterion) {
    let mut g = c.benchmark_group("meta benches");

    let mut s = Strobe::new(b"simplebench", SecParam::B256);
    let mut v = [0u8; 256];
    g.bench_function("meta_send_enc", |b| {
        b.iter(|| s.meta_send_enc(&mut v, false))
    });
    g.bench_function("meta_recv_enc", |b| {
        b.iter(|| s.meta_recv_enc(&mut v, false))
    });
    g.bench_function("meta_send_clr", |b| b.iter(|| s.meta_send_clr(&v, false)));
    g.bench_function("meta_recv_clr", |b| b.iter(|| s.meta_recv_clr(&v, false)));
    g.bench_function("meta_ad", |b| b.iter(|| s.meta_ad(&v, false)));
    g.bench_function("meta_key", |b| b.iter(|| s.meta_key(&v, false)));
    g.bench_function("meta_prf", |b| b.iter(|| s.meta_prf(&mut v, false)));
    g.bench_function("meta_send_mac", |b| {
        b.iter(|| s.meta_send_mac(&mut v, false))
    });
    g.bench_function("meta_rachet 16", |b| b.iter(|| s.meta_ratchet(16, false)));
}

criterion_group!(benches, bench_nonmeta, bench_meta);
criterion_main!(benches);
