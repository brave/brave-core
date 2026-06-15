use criterion::{criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use orchard::{
    builder::{Builder, BundleType},
    circuit::ProvingKey,
    keys::{FullViewingKey, PreparedIncomingViewingKey, Scope, SpendingKey},
    note_encryption::{CompactAction, OrchardDomain},
    value::NoteValue,
    Anchor, Bundle,
};
use rand::rngs::OsRng;
use zcash_note_encryption::{batch, try_compact_note_decryption, try_note_decryption};

#[cfg(unix)]
use pprof::criterion::{Output, PProfProfiler};

fn bench_note_decryption(c: &mut Criterion) {
    let rng = OsRng;
    let pk = ProvingKey::build();

    let fvk = FullViewingKey::from(&SpendingKey::from_bytes([7; 32]).unwrap());
    let valid_ivk = fvk.to_ivk(Scope::External);
    let recipient = valid_ivk.address_at(0u32);
    let valid_ivk = PreparedIncomingViewingKey::new(&valid_ivk);

    // Compact actions don't have the full AEAD ciphertext, so ZIP 307 trial-decryption
    // relies on an invalid ivk resulting in random noise for which the note commitment
    // is invalid. However, in practice we still get early rejection:
    // - The version byte will be invalid in 255/256 instances.
    // - If the version byte is valid, one of either the note commitment check or the esk
    //   check will be invalid, saving us at least one scalar mul.
    //
    // Our fixed (action, invalid ivk) tuple will always fall into a specific rejection
    // case. In order to reflect the real behaviour in the benchmarks, we trial-decrypt
    // with 10240 invalid ivks (each of which will result in a different uniformly-random
    // plaintext); this is equivalent to trial-decrypting 10240 different actions with the
    // same ivk, but is faster to set up.
    let invalid_ivks: Vec<_> = (0u32..10240)
        .map(|i| {
            let mut sk = [0; 32];
            sk[..4].copy_from_slice(&i.to_le_bytes());
            let fvk = FullViewingKey::from(&SpendingKey::from_bytes(sk).unwrap());
            PreparedIncomingViewingKey::new(&fvk.to_ivk(Scope::External))
        })
        .collect();

    let bundle = {
        let mut builder = Builder::new(BundleType::DEFAULT, Anchor::from_bytes([0; 32]).unwrap());
        // The builder pads to two actions, and shuffles their order. Add two recipients
        // so the first action is always decryptable.
        builder
            .add_output(None, recipient, NoteValue::from_raw(10), [0; 512])
            .unwrap();
        builder
            .add_output(None, recipient, NoteValue::from_raw(10), [0; 512])
            .unwrap();
        let bundle: Bundle<_, i64> = builder.build(rng).unwrap().unwrap().0;
        bundle
            .create_proof(&pk, rng)
            .unwrap()
            .apply_signatures(rng, [0; 32], &[])
            .unwrap()
    };
    let action = bundle.actions().first();

    let domain = OrchardDomain::for_action(action);

    let compact = {
        let mut group = c.benchmark_group("note-decryption");
        group.throughput(Throughput::Elements(1));

        group.bench_function("valid", |b| {
            b.iter(|| try_note_decryption(&domain, &valid_ivk, action).unwrap())
        });

        // Non-compact actions will always early-reject at the same point: AEAD decryption.
        group.bench_function("invalid", |b| {
            b.iter(|| try_note_decryption(&domain, &invalid_ivks[0], action))
        });

        let compact = CompactAction::from(action);

        group.bench_function("compact-valid", |b| {
            b.iter(|| try_compact_note_decryption(&domain, &valid_ivk, &compact).unwrap())
        });

        compact
    };

    {
        let mut group = c.benchmark_group("compact-note-decryption");
        group.throughput(Throughput::Elements(invalid_ivks.len() as u64));
        group.bench_function("invalid", |b| {
            b.iter(|| {
                for ivk in &invalid_ivks {
                    try_compact_note_decryption(&domain, ivk, &compact);
                }
            })
        });
    }

    {
        // Benchmark with 2 IVKs to emulate a wallet with two pools of funds.
        let ivks = 2;
        let valid_ivks = vec![valid_ivk; ivks];
        let actions: Vec<_> = (0..100)
            .map(|_| (OrchardDomain::for_action(action), action.clone()))
            .collect();
        let compact: Vec<_> = (0..100)
            .map(|_| {
                (
                    OrchardDomain::for_action(action),
                    CompactAction::from(action),
                )
            })
            .collect();

        let mut group = c.benchmark_group("batch-note-decryption");

        for size in [10, 50, 100] {
            group.throughput(Throughput::Elements((ivks * size) as u64));

            group.bench_function(BenchmarkId::new("valid", size), |b| {
                b.iter(|| batch::try_note_decryption(&valid_ivks, &actions[..size]))
            });

            group.bench_function(BenchmarkId::new("invalid", size), |b| {
                b.iter(|| batch::try_note_decryption(&invalid_ivks[..ivks], &actions[..size]))
            });

            group.bench_function(BenchmarkId::new("compact-valid", size), |b| {
                b.iter(|| batch::try_compact_note_decryption(&valid_ivks, &compact[..size]))
            });

            group.bench_function(BenchmarkId::new("compact-invalid", size), |b| {
                b.iter(|| {
                    batch::try_compact_note_decryption(&invalid_ivks[..ivks], &compact[..size])
                })
            });
        }
    }
}

#[cfg(unix)]
criterion_group! {
    name = benches;
    config = Criterion::default().with_profiler(PProfProfiler::new(100, Output::Flamegraph(None)));
    targets = bench_note_decryption
}
#[cfg(not(unix))]
criterion_group!(benches, bench_note_decryption);
criterion_main!(benches);
