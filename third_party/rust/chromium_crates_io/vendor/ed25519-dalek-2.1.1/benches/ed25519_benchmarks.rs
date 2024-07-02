// -*- mode: rust; -*-
//
// This file is part of ed25519-dalek.
// Copyright (c) 2018-2019 isis lovecruft
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>

use criterion::{criterion_group, Criterion};

mod ed25519_benches {
    use super::*;
    use ed25519_dalek::Signature;
    use ed25519_dalek::Signer;
    use ed25519_dalek::SigningKey;
    use rand::prelude::ThreadRng;
    use rand::thread_rng;

    fn sign(c: &mut Criterion) {
        let mut csprng: ThreadRng = thread_rng();
        let keypair: SigningKey = SigningKey::generate(&mut csprng);
        let msg: &[u8] = b"";

        c.bench_function("Ed25519 signing", move |b| b.iter(|| keypair.sign(msg)));
    }

    fn verify(c: &mut Criterion) {
        let mut csprng: ThreadRng = thread_rng();
        let keypair: SigningKey = SigningKey::generate(&mut csprng);
        let msg: &[u8] = b"";
        let sig: Signature = keypair.sign(msg);

        c.bench_function("Ed25519 signature verification", move |b| {
            b.iter(|| keypair.verify(msg, &sig))
        });
    }

    fn verify_strict(c: &mut Criterion) {
        let mut csprng: ThreadRng = thread_rng();
        let keypair: SigningKey = SigningKey::generate(&mut csprng);
        let msg: &[u8] = b"";
        let sig: Signature = keypair.sign(msg);

        c.bench_function("Ed25519 strict signature verification", move |b| {
            b.iter(|| keypair.verify_strict(msg, &sig))
        });
    }

    #[cfg(feature = "batch")]
    fn verify_batch_signatures(c: &mut Criterion) {
        use ed25519_dalek::verify_batch;

        static BATCH_SIZES: [usize; 8] = [4, 8, 16, 32, 64, 96, 128, 256];

        // Benchmark batch verification for all the above batch sizes
        let mut group = c.benchmark_group("Ed25519 batch signature verification");
        for size in BATCH_SIZES {
            let name = format!("size={size}");
            group.bench_function(name, |b| {
                let mut csprng: ThreadRng = thread_rng();
                let keypairs: Vec<SigningKey> = (0..size)
                    .map(|_| SigningKey::generate(&mut csprng))
                    .collect();
                let msg: &[u8] = b"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                let messages: Vec<&[u8]> = (0..size).map(|_| msg).collect();
                let signatures: Vec<Signature> = keypairs.iter().map(|key| key.sign(msg)).collect();
                let verifying_keys: Vec<_> =
                    keypairs.iter().map(|key| key.verifying_key()).collect();

                b.iter(|| verify_batch(&messages[..], &signatures[..], &verifying_keys[..]));
            });
        }
    }

    // If the above function isn't defined, make a placeholder function
    #[cfg(not(feature = "batch"))]
    fn verify_batch_signatures(_: &mut Criterion) {}

    fn key_generation(c: &mut Criterion) {
        let mut csprng: ThreadRng = thread_rng();

        c.bench_function("Ed25519 keypair generation", move |b| {
            b.iter(|| SigningKey::generate(&mut csprng))
        });
    }

    criterion_group! {
        name = ed25519_benches;
        config = Criterion::default();
        targets =
            sign,
            verify,
            verify_strict,
            verify_batch_signatures,
            key_generation,
    }
}

criterion::criterion_main!(ed25519_benches::ed25519_benches);
