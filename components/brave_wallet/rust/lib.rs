// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use core::fmt;
use curve25519_dalek;
use ed25519_dalek_bip32::derivation_path::{
    ChildIndexError, DerivationPath, DerivationPathParseError,
};
use ed25519_dalek_bip32::ed25519_dalek::{
    Keypair, SecretKey, Signature, SignatureError, Signer, KEYPAIR_LENGTH, PUBLIC_KEY_LENGTH,
    SECRET_KEY_LENGTH, SIGNATURE_LENGTH,
};
use ed25519_dalek_bip32::Error as Ed25519Bip32Error;
use ed25519_dalek_bip32::{ChildIndex, ExtendedSecretKey};

macro_rules! impl_result {
    ($t:ident, $r:ident, $f:ident) => {
        impl $r {
            fn error_message(self: &$r) -> String {
                match &self.0 {
                    Err(e) => e.to_string(),
                    Ok(_) => "".to_string(),
                }
            }

            fn is_ok(self: &$r) -> bool {
                match &self.0 {
                    Err(_) => false,
                    Ok(_) => true,
                }
            }

            fn unwrap(self: &$r) -> &$t {
                self.0.as_ref().expect("Unhandled error before unwrap call")
            }
        }

        impl From<Result<$f, Error>> for $r {
            fn from(result: Result<$f, Error>) -> Self {
                match result {
                    Ok(v) => Self(Ok($t(v))),
                    Err(e) => Self(Err(e)),
                }
            }
        }
    };
}

macro_rules! impl_error {
    ($t:ident, $n:ident) => {
        impl From<$t> for Error {
            fn from(err: $t) -> Self {
                Self::$n(err)
            }
        }
    };
}

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace =  brave_wallet)]
mod ffi {
    extern "Rust" {
        type Ed25519DalekExtendedSecretKey;
        type Ed25519DalekSignature;

        type Ed25519DalekExtendedSecretKeyResult;
        type Ed25519DalekSignatureResult;
        type Ed25519DalekVerificationResult;

        fn generate_ed25519_extended_secrect_key_from_seed(
            bytes: &[u8],
        ) -> Box<Ed25519DalekExtendedSecretKeyResult>;

        fn generate_ed25519_extended_secrect_key_from_bytes(
            bytes: &[u8],
        ) -> Box<Ed25519DalekExtendedSecretKeyResult>;

        fn bytes_are_curve25519_point(bytes: &[u8]) -> bool;

        fn derive(
            self: &Ed25519DalekExtendedSecretKey,
            path: String,
        ) -> Box<Ed25519DalekExtendedSecretKeyResult>;
        fn derive_hardened_child(
            self: &Ed25519DalekExtendedSecretKey,
            index: u32,
        ) -> Box<Ed25519DalekExtendedSecretKeyResult>;
        fn keypair_raw(self: &Ed25519DalekExtendedSecretKey) -> [u8; 64];
        fn secret_key_raw(self: &Ed25519DalekExtendedSecretKey) -> [u8; 32];
        fn public_key_raw(self: &Ed25519DalekExtendedSecretKey) -> [u8; 32];
        fn sign(
            self: &Ed25519DalekExtendedSecretKey,
            msg: &[u8],
        ) -> Box<Ed25519DalekSignatureResult>;
        fn verify(
            self: &Ed25519DalekExtendedSecretKey,
            msg: &[u8],
            sig: &[u8],
        ) -> Box<Ed25519DalekVerificationResult>;

        fn to_bytes(self: &Ed25519DalekSignature) -> [u8; 64];

        fn is_ok(self: &Ed25519DalekExtendedSecretKeyResult) -> bool;
        fn error_message(self: &Ed25519DalekExtendedSecretKeyResult) -> String;
        fn unwrap(self: &Ed25519DalekExtendedSecretKeyResult) -> &Ed25519DalekExtendedSecretKey;

        fn is_ok(self: &Ed25519DalekSignatureResult) -> bool;
        fn error_message(self: &Ed25519DalekSignatureResult) -> String;
        fn unwrap(self: &Ed25519DalekSignatureResult) -> &Ed25519DalekSignature;

        fn is_ok(self: &Ed25519DalekVerificationResult) -> bool;
        fn error_message(self: &Ed25519DalekVerificationResult) -> String;
    }
}

#[derive(Debug)]
pub enum Error {
    Ed25519Bip32(Ed25519Bip32Error),
    DerivationPathParse(DerivationPathParseError),
    ChildIndex(ChildIndexError),
    Signature(SignatureError),
}

impl_error!(Ed25519Bip32Error, Ed25519Bip32);
impl_error!(DerivationPathParseError, DerivationPathParse);
impl_error!(ChildIndexError, ChildIndex);
impl_error!(SignatureError, Signature);

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self {
            Error::Ed25519Bip32(e) => write!(f, "Error: {}", e.to_string()),
            Error::DerivationPathParse(e) => write!(f, "Error: {}", e.to_string()),
            Error::ChildIndex(e) => write!(f, "Error: {}", e.to_string()),
            Error::Signature(e) => write!(f, "Error: {}", e.to_string()),
        }
    }
}

pub struct Ed25519DalekExtendedSecretKey(ExtendedSecretKey);
pub struct Ed25519DalekSignature(Signature);

struct Ed25519DalekExtendedSecretKeyResult(Result<Ed25519DalekExtendedSecretKey, Error>);
struct Ed25519DalekSignatureResult(Result<Ed25519DalekSignature, Error>);
struct Ed25519DalekVerificationResult(Result<(), Error>);

impl_result!(Ed25519DalekExtendedSecretKey, Ed25519DalekExtendedSecretKeyResult, ExtendedSecretKey);
impl_result!(Ed25519DalekSignature, Ed25519DalekSignatureResult, Signature);
impl Ed25519DalekVerificationResult {
    fn error_message(&self) -> String {
        match &self.0 {
            Err(e) => e.to_string(),
            Ok(_) => "".to_string(),
        }
    }

    fn is_ok(&self) -> bool {
        match &self.0 {
            Err(_) => false,
            Ok(_) => true,
        }
    }
}

impl From<Result<(), Error>> for Ed25519DalekVerificationResult {
    fn from(result: Result<(), Error>) -> Self {
        match result {
            Ok(v) => Self(Ok(v)),
            Err(e) => Self(Err(e)),
        }
    }
}

fn generate_ed25519_extended_secrect_key_from_seed(
    bytes: &[u8],
) -> Box<Ed25519DalekExtendedSecretKeyResult> {
    Box::new(Ed25519DalekExtendedSecretKeyResult::from(
        ExtendedSecretKey::from_seed(bytes).map_err(|err| Error::from(err)),
    ))
}
fn generate_ed25519_extended_secrect_key_from_bytes(
    bytes: &[u8],
) -> Box<Ed25519DalekExtendedSecretKeyResult> {
    Box::new(Ed25519DalekExtendedSecretKeyResult::from(
        SecretKey::from_bytes(bytes).map_err(|err| Error::from(err)).and_then(|secret_key| {
            Ok(ExtendedSecretKey {
                depth: 0,
                child_index: ChildIndex::Normal(0),
                secret_key,
                chain_code: [0; 32],
            })
        }),
    ))
}
fn bytes_are_curve25519_point(bytes: &[u8]) -> bool {
    curve25519_dalek::edwards::CompressedEdwardsY::from_slice(bytes).decompress().is_some()
}

impl Ed25519DalekExtendedSecretKey {
    fn derive(&self, path: String) -> Box<Ed25519DalekExtendedSecretKeyResult> {
        Box::new(Ed25519DalekExtendedSecretKeyResult::from(
            path.parse::<DerivationPath>()
                .map_err(|err| Error::from(err))
                .and_then(|d_path| Ok(self.0.derive(&d_path)?)),
        ))
    }
    fn derive_hardened_child(
        &self, index: u32
    ) -> Box<Ed25519DalekExtendedSecretKeyResult> {
        Box::new(Ed25519DalekExtendedSecretKeyResult::from(
            ChildIndex::hardened(index)
                .map_err(|err| Error::from(err))
                .and_then(|child_index| Ok(self.0.derive_child(child_index)?)),
        ))
    }
    fn keypair_raw(&self) -> [u8; KEYPAIR_LENGTH] {
        let mut bytes: [u8; KEYPAIR_LENGTH] = [0u8; KEYPAIR_LENGTH];
        bytes[..SECRET_KEY_LENGTH].copy_from_slice(&self.0.secret_key.to_bytes());
        bytes[SECRET_KEY_LENGTH..].copy_from_slice(&self.0.public_key().to_bytes());
        bytes
    }
    fn secret_key_raw(&self) -> [u8; SECRET_KEY_LENGTH] {
        self.0.secret_key.to_bytes()
    }
    fn public_key_raw(&self) -> [u8; PUBLIC_KEY_LENGTH] {
        self.0.public_key().to_bytes()
    }
    fn sign(self: &Ed25519DalekExtendedSecretKey, msg: &[u8]) -> Box<Ed25519DalekSignatureResult> {
        Box::new(Ed25519DalekSignatureResult::from(
            Keypair::from_bytes(&self.keypair_raw())
                .map_err(|err| Error::from(err))
                .and_then(|keypair| Ok(keypair.try_sign(msg)?)),
        ))
    }
    fn verify(
        self: &Ed25519DalekExtendedSecretKey,
        msg: &[u8],
        sig: &[u8],
    ) -> Box<Ed25519DalekVerificationResult> {
        Box::new(Ed25519DalekVerificationResult::from(
            Keypair::from_bytes(&self.keypair_raw())
                .map_err(|err| Error::from(err))
                .and_then(|keypair| Ok(keypair.verify(msg, &Signature::from_bytes(sig)?)?)),
        ))
    }
}

impl Ed25519DalekSignature {
    fn to_bytes(self: &Ed25519DalekSignature) -> [u8; SIGNATURE_LENGTH] {
        self.0.to_bytes()
    }
}
