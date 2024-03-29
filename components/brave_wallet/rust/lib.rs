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
use bech32::FromBase32;
use ffi::{Bech32DecodeVariant};
use bech32::Error as Bech32Error;

// Orchard deps
use zip32::ChildIndex as OrchardChildIndex;
use orchard::keys::Scope as OrchardScope;
use orchard::keys::FullViewingKey as OrchardFVK;
use orchard::zip32::Error as Zip32Error;
use orchard::builder::Builder;
use orchard::builder::BuildError as OrchardBuildError;
use orchard::zip32::ExtendedSpendingKey;
use ffi::OrchardOutput;

use zcash::orchard::write_v5_bundle;
use orchard::circuit::ProvingKey;
use orchard::bundle::Authorized;
use orchard::bundle::Bundle;
use zcash::amount::Amount;
use orchard::builder::InProgress;
use orchard::builder::Unauthorized;
use orchard::builder::Unproven;
use rand_core::CryptoRng;
use zcash::merkle_tree::read_commitment_tree;
use orchard::tree::MerkleHashOrchard;
use orchard::Anchor;
use rand_core::OsRng;

// #![allow(dead_code)]
use rand_core::{RngCore, Error as OtherError, impls};

#[derive(Clone)]
struct MockRng;

impl CryptoRng for MockRng {}

impl RngCore for MockRng {
    fn next_u32(&mut self) -> u32 {
        0
    }

    fn next_u64(&mut self) -> u64 {
        0
    }

    fn fill_bytes(&mut self, dest: &mut [u8]) {
        impls::fill_bytes_via_next(self, dest)
    }

    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), OtherError> {
        Ok(self.fill_bytes(dest))
    }
}


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
    enum Bech32DecodeVariant {
        Bech32,
        Bech32m
    }
    struct OrchardOutput {
        value: u64,
        addr: [u8; 43]
    }

    extern "Rust" {
        type Ed25519DalekExtendedSecretKey;
        type Ed25519DalekSignature;
        type Bech32DecodeValue;
        type OrchardExtendedSpendingKey;
        type OrchardBundleValue;
        type OrchardBuilderValue;


        type Ed25519DalekExtendedSecretKeyResult;
        type Ed25519DalekSignatureResult;
        type Ed25519DalekVerificationResult;
        type Bech32DecodeResult;
        type OrchardExtendedSpendingKeyResult;
        type OrchardBundleResult;
        type OrchardBuilderResult;


        fn create_orchard_builder(
            tree_state: &[u8],
            outputs: Vec<OrchardOutput>
        ) -> Box<OrchardBuilderResult>;

        fn create_testing_orchard_builder(
            tree_state: &[u8],
            outputs: Vec<OrchardOutput>
        ) -> Box<OrchardBuilderResult>;

        fn generate_orchard_extended_spending_key_from_seed(
            bytes: &[u8]
        ) -> Box<OrchardExtendedSpendingKeyResult>;

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

        fn is_ok(self: &OrchardExtendedSpendingKeyResult) -> bool;
        fn error_message(self: &OrchardExtendedSpendingKeyResult) -> String;
        fn unwrap(self: &OrchardExtendedSpendingKeyResult) -> &OrchardExtendedSpendingKey;

        fn is_ok(self: &OrchardBundleResult) -> bool;
        fn error_message(self: &OrchardBundleResult) -> String;
        fn unwrap(self: &OrchardBundleResult) -> &OrchardBundleValue;

        fn is_ok(self: &OrchardBuilderResult) -> bool;
        fn error_message(self: &OrchardBuilderResult) -> String;
        fn unwrap(self: &OrchardBuilderResult) -> &OrchardBuilderValue;

        fn derive(
            self: &OrchardExtendedSpendingKey,
            index: u32
        ) -> Box<OrchardExtendedSpendingKeyResult>;
        fn external_address(
            self: &OrchardExtendedSpendingKey,
            diversifier_index: u32
        ) -> [u8; 43];
        fn internal_address(
            self: &OrchardExtendedSpendingKey,
            diversifier_index: u32
        ) -> [u8; 43];

        fn orchard_digest(self: &OrchardBuilderValue) -> [u8; 32];
        fn complete(self: &mut OrchardBuilderValue, sighash: [u8; 32]) -> Box<OrchardBundleResult>;

        fn raw_tx(self: &OrchardBundleValue) -> Vec<u8>;
    }
}

#[derive(Debug)]
pub enum Error {
    Ed25519Bip32(Ed25519Bip32Error),
    DerivationPathParse(DerivationPathParseError),
    ChildIndex(ChildIndexError),
    Signature(SignatureError),
    Bech32(Bech32Error),
    Zip32(Zip32Error),
    OrchardBuilder(OrchardBuildError),
}

impl_error!(Ed25519Bip32Error, Ed25519Bip32);
impl_error!(DerivationPathParseError, DerivationPathParse);
impl_error!(ChildIndexError, ChildIndex);
impl_error!(SignatureError, Signature);
impl_error!(Bech32Error, Bech32);
impl_error!(Zip32Error, Zip32);
impl_error!(OrchardBuildError, OrchardBuilder);

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self {
            Error::Ed25519Bip32(e) => write!(f, "Error: {}", e.to_string()),
            Error::DerivationPathParse(e) => write!(f, "Error: {}", e.to_string()),
            Error::ChildIndex(e) => write!(f, "Error: {}", e.to_string()),
            Error::Signature(e) => write!(f, "Error: {}", e.to_string()),
            Error::Bech32(e) => write!(f, "Error: {}", e.to_string()),
            Error::Zip32(e) => write!(f, "Error: {}", e.to_string()),
            Error::OrchardBuilder(e) => write!(f, "Error: {}", e.to_string()),
        }
    }
}

enum OrchardRandomSource {
    OsRng(rand::rngs::OsRng),
    MockRng(MockRng),
}

pub struct OrchardBuilder {
    unauthorized_bundle: Option<Bundle<InProgress<Unproven, Unauthorized>, Amount>>,
    rng: OrchardRandomSource
}

pub struct OrchardBundle {
    raw_tx: Vec<u8>
}

pub struct Bech32Decoded {
    hrp: String,
    data: Vec<u8>,
    variant: Bech32DecodeVariant,
}

pub struct Bech32DecodeValue(Bech32Decoded);
pub struct Ed25519DalekExtendedSecretKey(ExtendedSecretKey);
pub struct Ed25519DalekSignature(Signature);
pub struct OrchardExtendedSpendingKey(ExtendedSpendingKey);
pub struct OrchardBundleValue(OrchardBundle);
pub struct OrchardBuilderValue(OrchardBuilder);

struct Ed25519DalekExtendedSecretKeyResult(Result<Ed25519DalekExtendedSecretKey, Error>);
struct Ed25519DalekSignatureResult(Result<Ed25519DalekSignature, Error>);
struct Ed25519DalekVerificationResult(Result<(), Error>);
struct Bech32DecodeResult(Result<Bech32DecodeValue, Error>);
struct OrchardExtendedSpendingKeyResult(Result<OrchardExtendedSpendingKey, Error>);
struct OrchardBundleResult(Result<OrchardBundleValue, Error>);
struct OrchardBuilderResult(Result<OrchardBuilderValue, Error>);

impl_result!(Ed25519DalekExtendedSecretKey, Ed25519DalekExtendedSecretKeyResult, ExtendedSecretKey);
impl_result!(Ed25519DalekSignature, Ed25519DalekSignatureResult, Signature);
impl_result!(Bech32DecodeValue, Bech32DecodeResult, Bech32Decoded);
impl_result!(OrchardExtendedSpendingKey, OrchardExtendedSpendingKeyResult, ExtendedSpendingKey);
impl_result!(OrchardBundleValue, OrchardBundleResult, OrchardBundle);
impl_result!(OrchardBuilderValue, OrchardBuilderResult, OrchardBuilder);

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

fn generate_orchard_extended_spending_key_from_seed(
    bytes: &[u8]
) -> Box<OrchardExtendedSpendingKeyResult> {
  Box::new(OrchardExtendedSpendingKeyResult::from(
    ExtendedSpendingKey::master(&bytes).map_err(|err| Error::from(err)))
  )
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

fn decode_bech32(input: &str) -> Box<Bech32DecodeResult> {
    let decoded = bech32::decode(&input);
    match decoded {
        Ok(decoded_value) => {
            let (hrp, data, variant) = decoded_value;
            Box::new(Bech32DecodeResult::from(
                Vec::<u8>::from_base32(&data)
                    .map_err(Error::from)
                    .and_then(|as_u8| Ok(
                        Bech32Decoded {
                            hrp: hrp,
                            data: as_u8,
                            variant : Bech32DecodeVariant::from(variant)
                        })
                    )
                )
            )
        },
        Err(e) => {
            Box::new(Bech32DecodeResult::from(Err(Error::from(e))))
        }
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

impl OrchardExtendedSpendingKey {
    fn derive(
        self: &OrchardExtendedSpendingKey,
        index: u32
    ) -> Box<OrchardExtendedSpendingKeyResult> {
        Box::new(OrchardExtendedSpendingKeyResult::from(
            self.0.derive_child(
                OrchardChildIndex::hardened(index))
                .map_err(|err| Error::from(err))))
    }

    fn external_address(
        self: &OrchardExtendedSpendingKey,
        diversifier_index: u32
    ) -> [u8; 43] {
        let address = OrchardFVK::from(&self.0).address_at(
            diversifier_index, OrchardScope::External);
        address.to_raw_address_bytes()
    }

    fn internal_address(
        self: &OrchardExtendedSpendingKey,
        diversifier_index: u32
    ) -> [u8; 43] {
        let address = OrchardFVK::from(&self.0).address_at(
            diversifier_index, OrchardScope::Internal);
        address.to_raw_address_bytes()
    }
}


impl OrchardBundleValue {
    fn raw_tx(self: &OrchardBundleValue) -> Vec<u8> {
        self.0.raw_tx.clone()
    }
}

fn create_orchard_builder_internal(
    orchard_tree_bytes: &[u8],
    outputs: Vec<OrchardOutput>,
    random_source: OrchardRandomSource
) -> Box<OrchardBuilderResult> {
    let tree = read_commitment_tree::<MerkleHashOrchard, _, { orchard::NOTE_COMMITMENT_TREE_DEPTH as u8 }>(
        &orchard_tree_bytes[..],
    );

    let anchor = match tree {
        Ok(tree) => Anchor::from(tree.root()),
        Err(_e) => return Box::new(OrchardBuilderResult::from(Err(Error::from(OrchardBuildError::AnchorMismatch)))),
    };

    let mut builder = Builder::new(
        orchard::builder::BundleType::DEFAULT,        
        anchor);

    for out in outputs {
        let _ = match Option::from(orchard::Address::from_raw_address_bytes(&out.addr)) {
            Some(addr) => {
                builder.add_output(None, addr,
                    orchard::value::NoteValue::from_raw(out.value), None)
            },
            None => return Box::new(OrchardBuilderResult::from(Err(Error::from(OrchardBuildError::AnchorMismatch))))
        };
    }

    match random_source {
        OrchardRandomSource::OsRng(mut rng) => {
            if let Ok(builder) = builder.build(&mut rng) {
                if let Some(bundle) = builder {
                    return Box::new(OrchardBuilderResult::from(Ok(OrchardBuilder{unauthorized_bundle: Some(bundle.0), rng: OrchardRandomSource::OsRng(rng)})))
                }
            }
            return Box::new(OrchardBuilderResult::from(Err(Error::from(OrchardBuildError::AnchorMismatch))))
        },
        OrchardRandomSource::MockRng(mut rng) => {
            if let Ok(builder) = builder.build(&mut rng) {
                if let Some(bundle) = builder {
                    return Box::new(OrchardBuilderResult::from(Ok(OrchardBuilder{unauthorized_bundle: Some(bundle.0), rng: OrchardRandomSource::MockRng(rng)})))
                }
            }
            return Box::new(OrchardBuilderResult::from(Err(Error::from(OrchardBuildError::AnchorMismatch))))
        }
    }
}

fn create_orchard_builder(
    orchard_tree_bytes: &[u8],
    outputs: Vec<OrchardOutput>
) -> Box<OrchardBuilderResult> {
    create_orchard_builder_internal(orchard_tree_bytes, outputs, OrchardRandomSource::OsRng(OsRng))
}

fn create_testing_orchard_builder(
    orchard_tree_bytes: &[u8],
    outputs: Vec<OrchardOutput>
) -> Box<OrchardBuilderResult> {
    create_orchard_builder_internal(orchard_tree_bytes, outputs, OrchardRandomSource::MockRng(MockRng))
}


impl OrchardBuilderValue {
    fn orchard_digest(self: &OrchardBuilderValue) -> [u8; 32] {
        self.0.unauthorized_bundle.as_ref().unwrap().commitment().into()
    }

    fn complete(self: &mut OrchardBuilderValue, sighash: [u8; 32]) -> Box<OrchardBundleResult> {

        let pk = ProvingKey::build();

        match self.0.unauthorized_bundle.take() {
            Some(unauthorized_bundle) => {
                let bundle: Bundle<Authorized, Amount> = match self.0.rng {
                    OrchardRandomSource::OsRng(ref mut rng) => {
                        unauthorized_bundle
                            .create_proof(&pk, rng.clone())
                            .unwrap()
                            .prepare(rng.clone(), sighash)
                            .finalize()
                            .unwrap()
                    },
                    OrchardRandomSource::MockRng(ref mut rng) => {
                        unauthorized_bundle
                            .create_proof(&pk, rng.clone())
                            .unwrap()
                            .prepare(rng.clone(), sighash)
                            .finalize()
                            .unwrap()
                    }
                };

                let mut raw_tx = vec![];
                match write_v5_bundle(Some(&bundle), &mut raw_tx) {
                    Ok(_) => {
                        let result = OrchardBundle {
                            raw_tx: raw_tx,
                        };        
                        Box::new(OrchardBundleResult::from(Ok(result)))
                    },
                    Err(_) => {
                        Box::new(OrchardBundleResult::from(Err(Error::from(OrchardBuildError::AnchorMismatch))))
                    }

                }
            },
            None => Box::new(OrchardBundleResult::from(Err(Error::from(OrchardBuildError::AnchorMismatch)))),
        }
    }
}