// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use rand_core::{CryptoRng, RngCore};
use schnorrkel::Signature;

#[derive(Clone)]
pub(crate) struct MockRng(u64);

impl CryptoRng for MockRng {}

impl RngCore for MockRng {
    fn next_u32(&mut self) -> u32 {
        self.next_u64() as u32
    }

    fn next_u64(&mut self) -> u64 {
        self.0 += 1;
        self.0
    }

    fn fill_bytes(&mut self, dest: &mut [u8]) {
        // https://github.com/rust-random/rand/blob/0.6.4/rand_core/src/impls.rs#L36-L64
        let mut left = dest;
        while left.len() >= 8 {
            let (l, r) = { left }.split_at_mut(8);
            left = r;
            let chunk: [u8; 8] = self.next_u64().to_le_bytes();
            l.copy_from_slice(&chunk);
        }
        let n = left.len();
        if n > 4 {
            let chunk: [u8; 8] = self.next_u64().to_le_bytes();
            left.copy_from_slice(&chunk[..n]);
        } else if n > 0 {
            let chunk: [u8; 4] = self.next_u32().to_le_bytes();
            left.copy_from_slice(&chunk[..n]);
        }
    }

    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), rand_core::Error> {
        Ok(self.fill_bytes(dest))
    }
}

#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    extern "Rust" {
        type CxxSchnorrkelKeyPair;

        fn generate_sr25519_keypair_from_seed(bytes: &[u8]) -> Box<CxxSchnorrkelKeyPair>;
        fn derive_hard(self: &CxxSchnorrkelKeyPair, junction: &[u8]) -> Box<CxxSchnorrkelKeyPair>;
        fn get_public_key(self: &CxxSchnorrkelKeyPair) -> [u8; 32];

        // Used for deterministic schnorr signatures in testing.
        // Absolutely do not use in production.
        fn use_mock_rng_for_testing(self: &mut CxxSchnorrkelKeyPair);

        fn sign_message(self: &CxxSchnorrkelKeyPair, msg: &[u8]) -> [u8; 64];
        fn verify_message(self: &CxxSchnorrkelKeyPair, sig_bytes: &[u8], msg: &[u8]) -> bool;
    }
}

#[derive(Clone, Debug)]
pub enum Error {
    Schnorrkel(schnorrkel::SignatureError),
}

#[derive(Clone)]
struct CxxSchnorrkelKeyPair {
    keypair: schnorrkel::Keypair,
    use_mock_rng: bool,
}

const SIGNING_CTX: &'static [u8] = b"substrate";
// equivalent to the chaincode len
const JUNCTION_ID_LEN: usize = 32;

fn generate_sr25519_keypair_from_seed(bytes: &[u8]) -> Box<CxxSchnorrkelKeyPair> {
    // from_bytes() only panics on length mismatch which we've now made
    // (theoretically) impossible here on the C++ side by putting a fixed-sized span
    // in the interface, so we can just unwrap() directly in the Rust
    // code and pass back a keypair unconditionally to the C++.
    // We don't use a fixed-size array reference here because cxxbridge treats
    // `&[T; N]` as `const std::array<T, N>&` which has undesirable properties on
    // the C++ side.
    // This function remains memory safe and if other C++ incorrectly invokes this
    // function, it will just simply panic which will mean it's easy to find.
    let mini_key = schnorrkel::MiniSecretKey::from_bytes(bytes).unwrap();
    Box::new(CxxSchnorrkelKeyPair {
        keypair: mini_key.expand_to_keypair(schnorrkel::ExpansionMode::Ed25519),
        use_mock_rng: false,
    })
}

impl CxxSchnorrkelKeyPair {
    fn get_public_key(self: &CxxSchnorrkelKeyPair) -> [u8; 32] {
        self.keypair.public.to_bytes()
    }

    fn use_mock_rng_for_testing(self: &mut CxxSchnorrkelKeyPair) {
        self.use_mock_rng = true;
    }

    fn sign_message(self: &CxxSchnorrkelKeyPair, msg: &[u8]) -> [u8; 64] {
        let t = schnorrkel::signing_context(SIGNING_CTX).bytes(msg);
        if self.use_mock_rng {
            let t = schnorrkel::context::attach_rng(t, MockRng(0));
            return self.keypair.sign(t).to_bytes();
        }

        self.keypair.sign(t).to_bytes()
    }

    fn verify_message(self: &CxxSchnorrkelKeyPair, sig_bytes: &[u8], msg: &[u8]) -> bool {
        let Ok(signature) = Signature::from_bytes(sig_bytes) else {
            return false;
        };

        match self.keypair.verify_simple(SIGNING_CTX, msg, &signature) {
            Ok(_) => true,
            _ => false,
        }
    }

    // `junction` must already be SCALE-encoded
    fn derive_hard(self: &CxxSchnorrkelKeyPair, junction: &[u8]) -> Box<CxxSchnorrkelKeyPair> {
        let mut cc = [0_u8; JUNCTION_ID_LEN];
        if junction.len() > JUNCTION_ID_LEN {
            // copy what the polkadot-sdk crate does
            // https://paritytech.github.io/polkadot-sdk/master/src/sp_core/crypto.rs.html#138-151
            // https://github.com/paritytech/polkadot-sdk/blob/607a1b24b7902a657426ce2412e316a57b61894b/substrate/primitives/core/src/crypto.rs#L138-L151
            cc.copy_from_slice(
                blake2b_simd::Params::new().hash_length(JUNCTION_ID_LEN).hash(junction).as_bytes(),
            );
        } else {
            // use rote binary representation
            cc[0..junction.len()].copy_from_slice(junction);
        }

        Box::new(CxxSchnorrkelKeyPair {
            keypair: self
                .keypair
                .hard_derive_mini_secret_key(Some(schnorrkel::derive::ChainCode(cc)), b"")
                .0
                .expand_to_keypair(schnorrkel::ExpansionMode::Ed25519),
            use_mock_rng: false,
        })
    }
}
