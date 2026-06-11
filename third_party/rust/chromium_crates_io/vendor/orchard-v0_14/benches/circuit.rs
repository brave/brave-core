#[macro_use]
extern crate criterion;

use criterion::{BenchmarkId, Criterion};

#[cfg(unix)]
use pprof::criterion::{Output, PProfProfiler};

use orchard::{
    builder::{Builder, BundleType},
    circuit::{ProvingKey, VerifyingKey},
    keys::{FullViewingKey, Scope, SpendingKey},
    value::NoteValue,
    Anchor, Bundle,
};
use rand::rngs::OsRng;

fn criterion_benchmark(c: &mut Criterion) {
    let rng = OsRng;

    let sk = SpendingKey::from_bytes([7; 32]).unwrap();
    let recipient = FullViewingKey::from(&sk).address_at(0u32, Scope::External);

    let vk = VerifyingKey::build();
    let pk = ProvingKey::build();

    let create_bundle = |num_recipients| {
        let mut builder = Builder::new(BundleType::DEFAULT, Anchor::from_bytes([0; 32]).unwrap());
        for _ in 0..num_recipients {
            builder
                .add_output(None, recipient, NoteValue::from_raw(10), [0; 512])
                .unwrap();
        }
        let bundle: Bundle<_, i64> = builder.build(rng).unwrap().unwrap().0;

        let instances: Vec<_> = bundle
            .actions()
            .iter()
            .map(|a| a.to_instance(*bundle.flags(), *bundle.anchor()))
            .collect();

        (bundle, instances)
    };

    let recipients_range = 1..=4;

    {
        let mut group = c.benchmark_group("proving");
        group.sample_size(10);
        for num_recipients in recipients_range.clone() {
            let (bundle, instances) = create_bundle(num_recipients);
            group.bench_function(BenchmarkId::new("bundle", num_recipients), |b| {
                b.iter(|| {
                    bundle
                        .authorization()
                        .create_proof(&pk, &instances, rng)
                        .unwrap()
                });
            });
        }
    }

    {
        let mut group = c.benchmark_group("verifying");
        for num_recipients in recipients_range {
            let (bundle, instances) = create_bundle(num_recipients);
            let bundle = bundle
                .create_proof(&pk, rng)
                .unwrap()
                .apply_signatures(rng, [0; 32], &[])
                .unwrap();
            assert!(bundle.verify_proof(&vk).is_ok());
            group.bench_function(BenchmarkId::new("bundle", num_recipients), |b| {
                b.iter(|| bundle.authorization().proof().verify(&vk, &instances));
            });
        }
    }
}

#[cfg(unix)]
criterion_group! {
    name = benches;
    config = Criterion::default().with_profiler(PProfProfiler::new(100, Output::Flamegraph(None)));
    targets = criterion_benchmark
}
#[cfg(windows)]
criterion_group! {
    name = benches;
    config = Criterion::default();
    targets = criterion_benchmark
}
criterion_main!(benches);
