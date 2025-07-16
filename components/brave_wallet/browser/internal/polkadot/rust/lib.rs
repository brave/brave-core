// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

extern crate blake2b_simd;
extern crate cxx;
extern crate schnorrkel;

use schnorrkel::Signature;

#[allow(unused)]
#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = brave_wallet::polkadot)]
mod ffi {
    extern "Rust" {
        type CxxSchnorrkelKeyPair;
        type CxxSchnorrkelKeyPairResult;

        fn generate_sr25519_keypair_from_seed(bytes: &[u8]) -> Box<CxxSchnorrkelKeyPairResult>;
        fn derive_hard(self: &CxxSchnorrkelKeyPair, junction: &[u8]) -> Box<CxxSchnorrkelKeyPair>;
        fn get_public_key(self: &CxxSchnorrkelKeyPair) -> [u8; 32];
        fn sign_message(self: &CxxSchnorrkelKeyPair, msg: &[u8]) -> [u8; 64];
        fn verify_message(self: &CxxSchnorrkelKeyPair, sig_bytes: &[u8], msg: &[u8]) -> bool;
        fn is_ok(self: &CxxSchnorrkelKeyPairResult) -> bool;
        fn unwrap(self: &mut CxxSchnorrkelKeyPairResult) -> Box<CxxSchnorrkelKeyPair>;
    }
}

#[derive(Clone, Debug)]
pub enum Error {
    Schnorrkel(schnorrkel::SignatureError),
}

#[derive(Clone)]
struct CxxSchnorrkelKeyPair(schnorrkel::Keypair);

#[derive(Clone)]
struct CxxSchnorrkelKeyPairResult(Result<Option<CxxSchnorrkelKeyPair>, Error>);

const SIGNING_CTX: &'static [u8] = b"substrate";
// equivalent to the chaincode len
const JUNCTION_ID_LEN: usize = 32;

fn generate_sr25519_keypair_from_seed(bytes: &[u8]) -> Box<CxxSchnorrkelKeyPairResult> {
    let kp = schnorrkel::MiniSecretKey::from_bytes(bytes)
        .map(|kp| {
            Some(CxxSchnorrkelKeyPair(kp.expand_to_keypair(schnorrkel::ExpansionMode::Ed25519)))
        })
        .map_err(|e| Error::Schnorrkel(e));

    Box::new(CxxSchnorrkelKeyPairResult(kp))
}

impl CxxSchnorrkelKeyPair {
    fn get_public_key(self: &CxxSchnorrkelKeyPair) -> [u8; 32] {
        self.0.public.to_bytes()
    }

    fn sign_message(self: &CxxSchnorrkelKeyPair, msg: &[u8]) -> [u8; 64] {
        self.0.sign_simple(SIGNING_CTX, msg).to_bytes()
    }

    fn verify_message(self: &CxxSchnorrkelKeyPair, sig_bytes: &[u8], msg: &[u8]) -> bool {
        let Ok(signature) = Signature::from_bytes(sig_bytes) else {
            return false;
        };

        match self.0.verify_simple(SIGNING_CTX, msg, &signature) {
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

        Box::new(CxxSchnorrkelKeyPair(
            self.0
                .hard_derive_mini_secret_key(Some(schnorrkel::derive::ChainCode(cc)), b"")
                .0
                .expand_to_keypair(schnorrkel::ExpansionMode::Ed25519),
        ))
    }
}

impl CxxSchnorrkelKeyPairResult {
    fn is_ok(self: &CxxSchnorrkelKeyPairResult) -> bool {
        match &self.0 {
            Ok(_) => true,
            _ => false,
        }
    }

    fn unwrap(self: &mut CxxSchnorrkelKeyPairResult) -> Box<CxxSchnorrkelKeyPair> {
        Box::new(self.0.as_mut().unwrap().take().unwrap())
    }
}
