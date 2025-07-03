// -*- mode: rust; -*-
//
// This file is part of schnorrkel
// Copyright (c) 2018 isis lovecruft
// Copyright (c) 2018-2020 Web 3 Foundation
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>
// - Jeff Burdges <jeff@web3.foundation>

use criterion::{BenchmarkId, Criterion, criterion_main, criterion_group};

mod schnorr_benches {
    use super::*;
    use schnorrkel::{signing_context, verify_batch, Keypair, PublicKey, Signature}; // SecretKey

    // TODO: fn sign_mini(c: &mut Criterion)

    fn sign(c: &mut Criterion) {
        let keypair: Keypair = Keypair::generate();
        let msg: &[u8] = b"";

        let ctx = signing_context(b"this signature does this thing");
        c.bench_function("Schnorr signing", move |b| {
            b.iter(|| keypair.sign(ctx.bytes(msg)))
        });
    }

    fn verify(c: &mut Criterion) {
        let keypair: Keypair = Keypair::generate();
        let msg: &[u8] = b"";
        let ctx = signing_context(b"this signature does this thing");
        let sig: Signature = keypair.sign(ctx.bytes(msg));

        c.bench_function("Schnorr signature verification", move |b| {
            b.iter(|| keypair.verify(ctx.bytes(msg), &sig))
        });
    }

    fn verify_batch_signatures(c: &mut Criterion) {
        const BATCH_SIZES: [usize; 8] = [4, 8, 16, 32, 64, 96, 128, 256];

        let mut group = c.benchmark_group("Schnorr batch signature verification");
        for size in &BATCH_SIZES {
            group.bench_with_input(BenchmarkId::from_parameter(size), size, |b, &size| {
                let keypairs: Vec<Keypair> = (0..size).map(|_| Keypair::generate()).collect();
                let msg: &[u8] = b"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                let ctx = signing_context(b"this signature does this thing");
                let signatures: Vec<Signature> = keypairs
                    .iter()
                    .map(|key| key.sign(ctx.bytes(msg)))
                    .collect();
                let public_keys: Vec<PublicKey> = keypairs.iter().map(|key| key.public).collect();
                b.iter(|| {
                    let transcripts = ::std::iter::once(ctx.bytes(msg)).cycle().take(size);
                    let _ = verify_batch(transcripts, &signatures[..], &public_keys[..], true);
                });
            });
        }
    }

    fn key_generation(c: &mut Criterion) {
        c.bench_function("Schnorr keypair generation", move |b| {
            b.iter(|| Keypair::generate())
        });
    }

    criterion_group! {
        name = schnorr_benches;
        config = Criterion::default();
        targets =
            sign,
            verify,
            verify_batch_signatures,
            key_generation,
    }
}

criterion_main!(
    schnorr_benches::schnorr_benches,
);
