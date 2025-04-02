use aes::Aes256;
use criterion::{criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
use criterion_cycles_per_byte::CyclesPerByte;

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
type Criterion_ = Criterion<CyclesPerByte>;

#[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
type Criterion_ = Criterion;

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
fn setup_criterion() -> Criterion_ {
    Criterion::default().with_measurement(CyclesPerByte)
}

#[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
fn setup_criterion() -> Criterion_ {
    Criterion::default()
}

#[cfg(unix)]
use pprof::criterion::{Output, PProfProfiler};

fn ff1_binary_benchmark(c: &mut Criterion<CyclesPerByte>) {
    let bytes = vec![7; 1000];

    let fpe_ff = fpe::ff1::FF1::<Aes256>::new(&[0; 32], 2).unwrap();
    let mut fpe_group = c.benchmark_group("fpe");
    for size in [10, 100, 1000].iter() {
        fpe_group.throughput(Throughput::Bytes(*size as u64));
        fpe_group.bench_with_input(BenchmarkId::from_parameter(size), size, |b, &size| {
            b.iter(|| {
                fpe_ff.encrypt(
                    &[],
                    &fpe::ff1::BinaryNumeralString::from_bytes_le(&bytes[..size]),
                )
            });
        });
    }
    fpe_group.finish();

    /*
    let mut binary_ff1_group = c.benchmark_group("binary-ff1");
    for size in [10, 100, 1000].iter() {
        use aes_old::{block_cipher_trait::BlockCipher, Aes256};

        let mut buf = bytes[..*size].to_vec();

        let cipher = Aes256::new([0; 32][..].into());
        let mut scratch = vec![0; size + 1];
        let mut binary_ff1_ff =
            binary_ff1::BinaryFF1::new(&cipher, *size, &[], &mut scratch).unwrap();

        binary_ff1_group.throughput(Throughput::Bytes(*size as u64));
        binary_ff1_group.bench_with_input(BenchmarkId::from_parameter(size), size, |b, _| {
            b.iter(|| binary_ff1_ff.encrypt(&mut buf));
        });
    }
    binary_ff1_group.finish();
    */
}

#[cfg(unix)]
criterion_group!(
    name = benches;
    config = setup_criterion()
        .with_profiler(PProfProfiler::new(100, Output::Flamegraph(None)));
    targets = ff1_binary_benchmark
);
#[cfg(not(unix))]
criterion_group!(
    name = benches;
    config = setup_criterion();
    targets = ff1_binary_benchmark
);
criterion_main!(benches);
