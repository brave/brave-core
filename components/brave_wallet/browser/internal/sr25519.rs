// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use rand_chacha::rand_core::SeedableRng;
use schnorrkel::Signature;
use std::fmt;

#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    extern "Rust" {
        type CxxSchnorrkelKeyPair;
        type CxxSchnorrkelKeyPairResult;

        fn is_ok(self: &CxxSchnorrkelKeyPairResult) -> bool;
        fn error_message(self: &CxxSchnorrkelKeyPairResult) -> String;
        fn unwrap(self: &mut CxxSchnorrkelKeyPairResult) -> Box<CxxSchnorrkelKeyPair>;

        fn generate_sr25519_keypair_from_seed(bytes: &[u8]) -> Box<CxxSchnorrkelKeyPair>;
        fn create_sr25519_keypair_from_pkcs8(pkcs8: &[u8; 117]) -> Box<CxxSchnorrkelKeyPairResult>;
        fn derive_hard(self: &CxxSchnorrkelKeyPair, junction: &[u8]) -> Box<CxxSchnorrkelKeyPair>;
        fn get_public_key(self: &CxxSchnorrkelKeyPair) -> [u8; 32];

        // Used for deterministic schnorr signatures in testing.
        // Absolutely do not use in production.
        fn use_mock_rng_for_testing(self: &mut CxxSchnorrkelKeyPair);
        fn get_secret_key(self: &CxxSchnorrkelKeyPair) -> [u8; 64];
        fn get_export_key_pkcs8(self: &CxxSchnorrkelKeyPair) -> [u8; 117];
        fn sign_message(self: &CxxSchnorrkelKeyPair, msg: &[u8]) -> [u8; 64];
        fn verify_message(self: &CxxSchnorrkelKeyPair, sig_bytes: &[u8], msg: &[u8]) -> bool;
    }
}

#[derive(Clone, Debug)]
pub enum Error {
    /// The Result has already been unwrapped.
    AlreadyUnwrapped,
    Schnorrkel(schnorrkel::SignatureError),
    /// Invalid PKCS8 header found.
    InvalidPkcs8Header,
    /// Invalid PKCS8 divider found.
    InvalidPkcs8Divider,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::AlreadyUnwrapped => write!(f, "Already unwrapped."),
            Error::Schnorrkel(e) => write!(f, "Schnorrkel error: {}", e),
            Error::InvalidPkcs8Header => write!(f, "Invalid PKCS8 header."),
            Error::InvalidPkcs8Divider => write!(f, "Invalid PKCS8 divider."),
        }
    }
}

#[macro_export]
macro_rules! impl_result {
    ($t:ident, $r:ident) => {
        struct $r(Result<$t, Error>);

        impl $r {
            fn error_message(self: &$r) -> String {
                match &self.0 {
                    Err(e) => e.to_string(),
                    Ok(_) => String::new(),
                }
            }

            fn is_ok(self: &$r) -> bool {
                self.0.is_ok()
            }

            fn unwrap(self: &mut $r) -> Box<$t> {
                match std::mem::replace(&mut self.0, Err(Error::AlreadyUnwrapped)) {
                    Ok(v) => Box::new(v),
                    Err(e) => panic!("{}", e.to_string()),
                }
            }
        }
    };
}

#[derive(Clone)]
struct CxxSchnorrkelKeyPair {
    keypair: schnorrkel::Keypair,
    use_mock_rng: bool,
}

const SIGNING_CTX: &'static [u8] = b"substrate";
// equivalent to the chaincode len
const JUNCTION_ID_LEN: usize = 32;

// https://github.com/polkadot-js/common/blob/bf63a0ebf655312f54aa37350d244df3d05e4e32/packages/keyring/src/pair/defaults.ts#L5
// public/secret section divider.
const PAIR_DIV: &[u8] = &[161, 35, 3, 33, 0];
// public/secret start block.
const PAIR_HDR: &[u8] = &[48, 83, 2, 1, 1, 48, 5, 6, 3, 43, 101, 112, 4, 34, 4, 32];

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

impl_result!(CxxSchnorrkelKeyPair, CxxSchnorrkelKeyPairResult);

fn create_sr25519_keypair_from_pkcs8(pkcs8: &[u8; 117]) -> Box<CxxSchnorrkelKeyPairResult> {
    // Export in PKCS8 format: PAIR_HDR + secretKey + PAIR_DIV + publicKey.
    // https://github.com/polkadot-js/common/blob/bf63a0ebf655312f54aa37350d244df3d05e4e32/packages/keyring/src/pair/encode.ts#L19

    // Validate PAIR_HDR at the beginning
    if &pkcs8[..PAIR_HDR.len()] != PAIR_HDR {
        return Box::new(CxxSchnorrkelKeyPairResult(Err(Error::InvalidPkcs8Header)));
    }

    // Extract secret key (64 bytes after PAIR_HDR)
    let secret_key_start = PAIR_HDR.len();
    let secret_key_end = secret_key_start + 64;
    let secret_key =
        match schnorrkel::SecretKey::from_bytes(&pkcs8[secret_key_start..secret_key_end]) {
            Ok(key) => key,
            Err(e) => return Box::new(CxxSchnorrkelKeyPairResult(Err(Error::Schnorrkel(e)))),
        };

    // Validate PAIR_DIV after secret key
    let div_start = secret_key_end;
    let div_end = div_start + PAIR_DIV.len();
    if &pkcs8[div_start..div_end] != PAIR_DIV {
        return Box::new(CxxSchnorrkelKeyPairResult(Err(Error::InvalidPkcs8Divider)));
    }

    let keypair = schnorrkel::Keypair::from(secret_key);
    Box::new(CxxSchnorrkelKeyPairResult(Ok(CxxSchnorrkelKeyPair { keypair, use_mock_rng: false })))
}

impl CxxSchnorrkelKeyPair {
    fn get_public_key(self: &CxxSchnorrkelKeyPair) -> [u8; 32] {
        self.keypair.public.to_bytes()
    }

    fn use_mock_rng_for_testing(self: &mut CxxSchnorrkelKeyPair) {
        self.use_mock_rng = true;
    }

    fn get_secret_key(self: &CxxSchnorrkelKeyPair) -> [u8; 64] {
        self.keypair.secret.to_bytes()
    }

    fn get_export_key_pkcs8(self: &CxxSchnorrkelKeyPair) -> [u8; 117] {
        // Export in PKCS8 format: PAIR_HDR + secretKey + PAIR_DIV + publicKey.
        // https://github.com/polkadot-js/common/blob/bf63a0ebf655312f54aa37350d244df3d05e4e32/packages/keyring/src/pair/encode.ts#L19
        let secret_key = self.keypair.secret.to_bytes();
        let public_key = self.keypair.public.to_bytes();

        let mut result = Vec::with_capacity(
            PAIR_HDR.len() + secret_key.len() + PAIR_DIV.len() + public_key.len(),
        );
        result.extend_from_slice(PAIR_HDR);
        result.extend_from_slice(&secret_key);
        result.extend_from_slice(PAIR_DIV);
        result.extend_from_slice(&public_key);
        result.try_into().unwrap()
    }

    fn sign_message(self: &CxxSchnorrkelKeyPair, msg: &[u8]) -> [u8; 64] {
        let t = schnorrkel::signing_context(SIGNING_CTX).bytes(msg);
        if self.use_mock_rng {
            let t = schnorrkel::context::attach_rng(t, rand_chacha::ChaCha20Rng::seed_from_u64(0));
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
