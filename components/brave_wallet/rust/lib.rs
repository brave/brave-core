// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use bech32::Error as Bech32Error;
use bech32::FromBase32;
use core::fmt;
use ed25519_dalek_bip32::derivation_path::{
    ChildIndexError, DerivationPath, DerivationPathParseError,
};
use ed25519_dalek_bip32::ed25519_dalek::{
    Signature, SignatureError, Signer, SigningKey, KEYPAIR_LENGTH, PUBLIC_KEY_LENGTH,
    SECRET_KEY_LENGTH, SIGNATURE_LENGTH,
};
use ed25519_dalek_bip32::Error as Ed25519Bip32Error;
use ed25519_dalek_bip32::{ChildIndex, ExtendedSigningKey};
use ffi::Bech32DecodeVariant;

#[macro_export]
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

#[macro_export]
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
    enum Bech32DecodeVariant {
        Bech32,
        Bech32m,
    }

    extern "Rust" {
        type Ed25519DalekExtendedSecretKey;
        type Ed25519DalekSignature;
        type Bech32DecodeValue;

        type Ed25519DalekExtendedSecretKeyResult;
        type Ed25519DalekSignatureResult;
        type Ed25519DalekVerificationResult;
        type Bech32DecodeResult;

        fn generate_ed25519_extended_secret_key_from_seed(
            bytes: &[u8],
        ) -> Box<Ed25519DalekExtendedSecretKeyResult>;

        fn generate_ed25519_extended_secret_key_from_bytes(
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

        fn data(self: &Bech32DecodeValue) -> Vec<u8>;
        fn hrp(self: &Bech32DecodeValue) -> String;
        fn variant(self: &Bech32DecodeValue) -> Bech32DecodeVariant;

        fn decode_bech32(input: &str) -> Box<Bech32DecodeResult>;

        fn is_ok(self: &Ed25519DalekExtendedSecretKeyResult) -> bool;
        fn error_message(self: &Ed25519DalekExtendedSecretKeyResult) -> String;
        fn unwrap(self: &Ed25519DalekExtendedSecretKeyResult) -> &Ed25519DalekExtendedSecretKey;

        fn is_ok(self: &Ed25519DalekSignatureResult) -> bool;
        fn error_message(self: &Ed25519DalekSignatureResult) -> String;
        fn unwrap(self: &Ed25519DalekSignatureResult) -> &Ed25519DalekSignature;

        fn is_ok(self: &Ed25519DalekVerificationResult) -> bool;
        fn error_message(self: &Ed25519DalekVerificationResult) -> String;

        fn is_ok(self: &Bech32DecodeResult) -> bool;
        fn error_message(self: &Bech32DecodeResult) -> String;
        fn unwrap(self: &Bech32DecodeResult) -> &Bech32DecodeValue;
    }
}

#[derive(Debug)]
pub enum Error {
    Ed25519Bip32(Ed25519Bip32Error),
    DerivationPathParse(DerivationPathParseError),
    ChildIndex(ChildIndexError),
    Signature(SignatureError),
    Bech32(Bech32Error),
    KeyLengthMismatch,
}

impl_error!(Ed25519Bip32Error, Ed25519Bip32);
impl_error!(DerivationPathParseError, DerivationPathParse);
impl_error!(ChildIndexError, ChildIndex);
impl_error!(SignatureError, Signature);
impl_error!(Bech32Error, Bech32);

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self {
            Error::Ed25519Bip32(e) => write!(f, "Error: {}", e.to_string()),
            Error::DerivationPathParse(e) => write!(f, "Error: {}", e.to_string()),
            Error::ChildIndex(e) => write!(f, "Error: {}", e.to_string()),
            Error::Signature(e) => write!(f, "Error: {}", e.to_string()),
            Error::Bech32(e) => write!(f, "Error: {}", e.to_string()),
            Error::KeyLengthMismatch => {
                write!(f, "Error: raw key bytes were not the expected length")
            }
        }
    }
}
pub struct Bech32Decoded {
    hrp: String,
    data: Vec<u8>,
    variant: Bech32DecodeVariant,
}

pub struct Bech32DecodeValue(Bech32Decoded);
pub struct Ed25519DalekExtendedSecretKey(ExtendedSigningKey);
pub struct Ed25519DalekSignature(Signature);

struct Ed25519DalekExtendedSecretKeyResult(Result<Ed25519DalekExtendedSecretKey, Error>);
struct Ed25519DalekSignatureResult(Result<Ed25519DalekSignature, Error>);
struct Ed25519DalekVerificationResult(Result<(), Error>);
struct Bech32DecodeResult(Result<Bech32DecodeValue, Error>);

impl_result!(
    Ed25519DalekExtendedSecretKey,
    Ed25519DalekExtendedSecretKeyResult,
    ExtendedSigningKey
);
impl_result!(Ed25519DalekSignature, Ed25519DalekSignatureResult, Signature);
impl_result!(Bech32DecodeValue, Bech32DecodeResult, Bech32Decoded);

impl From<bech32::Variant> for Bech32DecodeVariant {
    fn from(v: bech32::Variant) -> Bech32DecodeVariant {
        match v {
            bech32::Variant::Bech32m => Bech32DecodeVariant::Bech32m,
            bech32::Variant::Bech32 => Bech32DecodeVariant::Bech32,
        }
    }
}

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

fn generate_ed25519_extended_secret_key_from_seed(
    bytes: &[u8],
) -> Box<Ed25519DalekExtendedSecretKeyResult> {
    Box::new(Ed25519DalekExtendedSecretKeyResult::from(
        ExtendedSigningKey::from_seed(bytes).map_err(Error::from),
    ))
}

fn generate_ed25519_extended_secret_key_from_bytes(
    bytes: &[u8],
) -> Box<Ed25519DalekExtendedSecretKeyResult> {
    let key_result = match bytes.try_into() {
        Err(_) => Err(Error::KeyLengthMismatch),
        Ok(array) => {
            let signing_key = SigningKey::from_bytes(array);
            Ok(ExtendedSigningKey {
                depth: 0,
                child_index: ChildIndex::Normal(0),
                signing_key,
                chain_code: [0; 32],
            })
        }
    };
    Box::new(Ed25519DalekExtendedSecretKeyResult::from(key_result))
}

fn bytes_are_curve25519_point(bytes: &[u8]) -> bool {
    match curve25519_dalek::edwards::CompressedEdwardsY::from_slice(bytes) {
        // If the y coordinate decompresses, it represents a curve point.
        Ok(point) => point.decompress().is_some(),
        // Creating the CompressedEdwardsY failed, so bytes does not represent
        // a curve point, probably the slice wasn't the expected size.
        Err(_) => false,
    }
}

fn decode_bech32(input: &str) -> Box<Bech32DecodeResult> {
    let decoded = bech32::decode(&input);
    match decoded {
        Ok(decoded_value) => {
            let (hrp, data, variant) = decoded_value;
            Box::new(Bech32DecodeResult::from(
                Vec::<u8>::from_base32(&data).map_err(Error::from).and_then(|as_u8| {
                    Ok(Bech32Decoded {
                        hrp: hrp,
                        data: as_u8,
                        variant: Bech32DecodeVariant::from(variant),
                    })
                }),
            ))
        }
        Err(e) => Box::new(Bech32DecodeResult::from(Err(Error::from(e)))),
    }
}

impl Ed25519DalekExtendedSecretKey {
    fn derive(&self, path: String) -> Box<Ed25519DalekExtendedSecretKeyResult> {
        Box::new(Ed25519DalekExtendedSecretKeyResult::from(
            path.parse::<DerivationPath>()
                .map_err(|err| Error::from(err))
                .and_then(|d_path| Ok(self.0.derive(&d_path)?)),
        ))
    }
    fn derive_hardened_child(&self, index: u32) -> Box<Ed25519DalekExtendedSecretKeyResult> {
        Box::new(Ed25519DalekExtendedSecretKeyResult::from(
            ChildIndex::hardened(index)
                .map_err(|err| Error::from(err))
                .and_then(|child_index| Ok(self.0.derive_child(child_index)?)),
        ))
    }
    fn keypair_raw(&self) -> [u8; KEYPAIR_LENGTH] {
        self.0.signing_key.to_keypair_bytes()
    }
    fn secret_key_raw(&self) -> [u8; SECRET_KEY_LENGTH] {
        self.0.signing_key.to_bytes()
    }
    fn public_key_raw(&self) -> [u8; PUBLIC_KEY_LENGTH] {
        self.0.verifying_key().to_bytes()
    }

    fn sign(self: &Ed25519DalekExtendedSecretKey, msg: &[u8]) -> Box<Ed25519DalekSignatureResult> {
        Box::new(Ed25519DalekSignatureResult::from(
            self.0.signing_key.try_sign(msg).map_err(Error::from),
        ))
    }

    fn verify(
        self: &Ed25519DalekExtendedSecretKey,
        msg: &[u8],
        sig: &[u8],
    ) -> Box<Ed25519DalekVerificationResult> {
        let sig_result = match Signature::from_slice(sig) {
            Ok(signature) => self.0.signing_key.verify(msg, &signature).map_err(Error::from),
            Err(e) => Err(Error::from(e)),
        };
        Box::new(Ed25519DalekVerificationResult::from(sig_result))
    }
}

impl Ed25519DalekSignature {
    fn to_bytes(self: &Ed25519DalekSignature) -> [u8; SIGNATURE_LENGTH] {
        self.0.to_bytes()
    }
}

impl Bech32DecodeValue {
    fn hrp(self: &Bech32DecodeValue) -> String {
        self.0.hrp.clone()
    }
    fn data(self: &Bech32DecodeValue) -> Vec<u8> {
        self.0.data.clone()
    }
    fn variant(self: &Bech32DecodeValue) -> Bech32DecodeVariant {
        self.0.variant.clone()
    }
}
