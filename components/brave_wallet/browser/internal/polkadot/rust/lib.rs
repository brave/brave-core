// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use schnorrkel::Signature;

extern crate cxx;
extern crate schnorrkel;

#[allow(unused)]
#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = brave_wallet::polkadot)]
mod ffi {
    extern "Rust" {
        type CxxSchnorrkelKeyPair;
        type CxxSchnorrkelKeyPairResult;

        fn generate_sr25519_keypair_from_seed(bytes: &[u8]) -> Box<CxxSchnorrkelKeyPairResult>;
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
struct CxxSchnorrkelKeyPair(SchnorrkelKeyPair);

#[derive(Clone)]
struct CxxSchnorrkelKeyPairResult(Result<Option<CxxSchnorrkelKeyPair>, Error>);

#[derive(Clone)]
struct SchnorrkelKeyPair {
    kp: schnorrkel::Keypair,
}

const SIGNING_CTX: &'static [u8] = b"substrate";

fn generate_sr25519_keypair_from_seed(bytes: &[u8]) -> Box<CxxSchnorrkelKeyPairResult> {
    let kp = schnorrkel::MiniSecretKey::from_bytes(bytes)
        .map(|kp| {
            Some(CxxSchnorrkelKeyPair(SchnorrkelKeyPair {
                kp: kp.expand_to_keypair(schnorrkel::ExpansionMode::Ed25519),
            }))
        })
        .map_err(|e| Error::Schnorrkel(e));

    Box::new(CxxSchnorrkelKeyPairResult(kp))
}

impl CxxSchnorrkelKeyPair {
    fn get_public_key(self: &CxxSchnorrkelKeyPair) -> [u8; 32] {
        self.0.kp.public.to_bytes()
    }

    fn sign_message(self: &CxxSchnorrkelKeyPair, msg: &[u8]) -> [u8; 64] {
        self.0.kp.sign_simple(SIGNING_CTX, msg).to_bytes()
    }

    fn verify_message(self: &CxxSchnorrkelKeyPair, sig_bytes: &[u8], msg: &[u8]) -> bool {
        let Ok(signature) = Signature::from_bytes(sig_bytes) else {
            return false;
        };

        match self.0.kp.verify_simple(SIGNING_CTX, msg, &signature) {
            Ok(_) => true,
            _ => false,
        }
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
