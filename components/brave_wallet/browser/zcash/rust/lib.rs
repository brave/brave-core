// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

mod librustzcash;

use std::fmt;

use orchard::{
    builder:: {
        BuildError as OrchardBuildError,
        InProgress,
        Unproven,
        Unauthorized
    },
    bundle::Bundle,
    zip32::ChildIndex as OrchardChildIndex,
    keys::Scope as OrchardScope,
    keys::FullViewingKey as OrchardFVK,
    zip32::Error as Zip32Error,
    zip32::ExtendedSpendingKey,
    tree::MerkleHashOrchard
};

use crate::librustzcash::amount::Amount;

use ffi::OrchardOutput;

use rand_core::OsRng;
use rand_core::{RngCore, Error as OtherError, impls};
use rand_core::CryptoRng;

use brave_wallet::{
  impl_error
};

// The rest of the wallet code should be updated to use this version of unwrap
// and then this code can be removed
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

            // Unfortunately cxx doesn't support passing $r by value here so
            // we have to clone the inner value instead of passing ownership
            // This is not really a big deal because eventually we want to
            // replace this with mojo which would serialize this anyway
            fn unwrap(self: &$r) -> Box<$t> {
                Box::new(self.0.as_ref().expect(
                    "Unhandled error before unwrap call").clone())
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
        impls::fill_bytes_via_next(self, dest)
    }

    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), OtherError> {
        Ok(self.fill_bytes(dest))
    }
}


#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = brave_wallet::orchard)]
mod ffi {
    struct OrchardOutput {
        value: u64,
        addr: [u8; 43]  // Array size should match kOrchardRawBytesSize
    }

    extern "Rust" {
        type OrchardExtendedSpendingKey;
        type OrchardBundleValue;
        type OrchardBuilderValue;

        type OrchardExtendedSpendingKeyResult;
        type OrchardBundleResult;
        type OrchardBuilderResult;

        fn create_orchard_builder(
            tree_state: &[u8],
            outputs: Vec<OrchardOutput>
        ) -> Box<OrchardBuilderResult>;

        fn create_testing_orchard_builder(
            tree_state: &[u8],
            outputs: Vec<OrchardOutput>,
            rng_seed: u64
        ) -> Box<OrchardBuilderResult>;

        fn generate_orchard_extended_spending_key_from_seed(
            bytes: &[u8]
        ) -> Box<OrchardExtendedSpendingKeyResult>;

        fn is_ok(self: &OrchardExtendedSpendingKeyResult) -> bool;
        fn error_message(self: &OrchardExtendedSpendingKeyResult) -> String;
        fn unwrap(self: &OrchardExtendedSpendingKeyResult) -> Box<OrchardExtendedSpendingKey>;

        fn derive(
            self: &OrchardExtendedSpendingKey,
            index: u32
        ) -> Box<OrchardExtendedSpendingKeyResult>;
        fn external_address(
            self: &OrchardExtendedSpendingKey,
            diversifier_index: u32
        ) -> [u8; 43];  // Array size should match kOrchardRawBytesSize
        fn internal_address(
            self: &OrchardExtendedSpendingKey,
            diversifier_index: u32
        ) -> [u8; 43];  // Array size should match kOrchardRawBytesSize

        fn is_ok(self: &OrchardBundleResult) -> bool;
        fn error_message(self: &OrchardBundleResult) -> String;
        fn unwrap(self: &OrchardBundleResult) -> &OrchardBundleValue;

        fn is_ok(self: &OrchardBuilderResult) -> bool;
        fn error_message(self: &OrchardBuilderResult) -> String;
        fn unwrap(self: &OrchardBuilderResult) -> &OrchardBuilderValue;

        fn orchard_digest(self: &OrchardBuilderValue) -> [u8; 32];  // Array size should match kZCashDigestSize
        fn complete(self: &OrchardBuilderValue, sighash: [u8; 32]) -> Box<OrchardBundleResult>;  // Array size should match kZCashDigestSize

        fn raw_tx(self: &OrchardBundleValue) -> Vec<u8>;
    }
}

#[derive(Debug)]
pub enum Error {
    Zip32(Zip32Error),
    OrchardBuilder(OrchardBuildError),
    WrongOutputError,
    BuildError,
}

impl_error!(Zip32Error, Zip32);
impl_error!(OrchardBuildError, OrchardBuilder);

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self {
            Error::Zip32(e) => write!(f, "Zip32 Error: {e}"),
            Error::OrchardBuilder(e) => write!(f, "Error: {}", e.to_string()),
            Error::WrongOutputError => write!(f, "Error: Can't parse output"),
            Error::BuildError => write!(f, "Error, build error"),
        }
    }
}

#[derive(Clone)]
enum OrchardRandomSource {
    OsRng(OsRng),
    MockRng(MockRng),
}

pub struct OrchardBuilder {
    unauthorized_bundle: Bundle<InProgress<Unproven, Unauthorized>, Amount>,
    rng: OrchardRandomSource
}

pub struct OrchardBundle {
    raw_tx: Vec<u8>
}

struct OrchardExtendedSpendingKey(ExtendedSpendingKey);
struct OrchardBundleValue(OrchardBundle);
struct OrchardBuilderValue(OrchardBuilder);

struct OrchardExtendedSpendingKeyResult(Result<OrchardExtendedSpendingKey, Error>);
struct OrchardBundleResult(Result<OrchardBundleValue, Error>);
struct OrchardBuilderResult(Result<OrchardBuilderValue, Error>);

impl_result!(OrchardExtendedSpendingKey, OrchardExtendedSpendingKeyResult, ExtendedSpendingKey);
impl_result!(OrchardBundleValue, OrchardBundleResult, OrchardBundle);
impl_result!(OrchardBuilderValue, OrchardBuilderResult, OrchardBuilder);

fn generate_orchard_extended_spending_key_from_seed(
    bytes: &[u8]
) -> Box<OrchardExtendedSpendingKeyResult> {
  Box::new(OrchardExtendedSpendingKeyResult::from(
    ExtendedSpendingKey::master(bytes).map_err(Error::from))
  )
}

impl OrchardExtendedSpendingKey {
    fn derive(
        self: &OrchardExtendedSpendingKey,
        index: u32
    ) -> Box<OrchardExtendedSpendingKeyResult> {
        Box::new(OrchardExtendedSpendingKeyResult::from(
            self.0.derive_child(
                OrchardChildIndex::hardened(index))
                .map_err(Error::from)))
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
    use orchard::Anchor;
    use crate::librustzcash::merkle_tree::read_commitment_tree;

    let anchor = if orchard_tree_bytes.len() > 0 {
        match read_commitment_tree::<MerkleHashOrchard, _, { orchard::NOTE_COMMITMENT_TREE_DEPTH as u8 }>(
                &orchard_tree_bytes[..]) {
            Ok(tree) => Anchor::from(tree.root()),
            Err(_e) => return Box::new(OrchardBuilderResult::from(Err(Error::from(OrchardBuildError::AnchorMismatch)))),
        }
    } else {
        orchard::Anchor::empty_tree()
    };

    let mut builder = orchard::builder::Builder::new(
        orchard::builder::BundleType::DEFAULT,
        anchor);

    for out in outputs {
        let _ = match Option::from(orchard::Address::from_raw_address_bytes(&out.addr)) {
            Some(addr) => {
                builder.add_output(None, addr,
                    orchard::value::NoteValue::from_raw(out.value), None)
            },
            None => return Box::new(OrchardBuilderResult::from(Err(Error::WrongOutputError)))
        };
    }

    Box::new(OrchardBuilderResult::from(match random_source {
        OrchardRandomSource::OsRng(mut rng) => {
            builder.build(&mut rng)
                .map_err(Error::from)
                .and_then(|builder| {
                    builder.map(|bundle| OrchardBuilder { unauthorized_bundle: bundle.0, rng: OrchardRandomSource::OsRng(rng) })
                        .ok_or(Error::BuildError)
                })
        },
        OrchardRandomSource::MockRng(mut rng) => {
            builder.build(&mut rng)
                .map_err(Error::from)
                .and_then(|builder| {
                    builder.map(|bundle| OrchardBuilder { unauthorized_bundle: bundle.0, rng: OrchardRandomSource::MockRng(rng) })
                        .ok_or(Error::BuildError)
                })
        }
    }))
}

fn create_orchard_builder(
    orchard_tree_bytes: &[u8],
    outputs: Vec<OrchardOutput>
) -> Box<OrchardBuilderResult> {
    create_orchard_builder_internal(orchard_tree_bytes, outputs, OrchardRandomSource::OsRng(OsRng))
}

fn create_testing_orchard_builder(
    orchard_tree_bytes: &[u8],
    outputs: Vec<OrchardOutput>,
    rng_seed: u64
) -> Box<OrchardBuilderResult> {
    create_orchard_builder_internal(orchard_tree_bytes, outputs, OrchardRandomSource::MockRng(MockRng(rng_seed)))
}

impl OrchardBuilderValue {
    fn orchard_digest(self: &OrchardBuilderValue) -> [u8; 32] {
        self.0.unauthorized_bundle.commitment().into()
    }

    fn complete(self: &OrchardBuilderValue, sighash: [u8; 32]) -> Box<OrchardBundleResult> {
        use crate::librustzcash::orchard::write_v5_bundle;
        Box::new(OrchardBundleResult::from(match self.0.rng.clone() {
            OrchardRandomSource::OsRng(mut rng) => {
                self.0.unauthorized_bundle.clone()
                .create_proof(&orchard::circuit::ProvingKey::build(), &mut rng)
                .and_then(|b| {
                            b.apply_signatures(
                                &mut rng,
                                sighash,
                                &[],
                            )
                        })
            },
            OrchardRandomSource::MockRng(mut rng) => {
                self.0.unauthorized_bundle.clone()
                .create_proof(&orchard::circuit::ProvingKey::build(), &mut rng)
                .and_then(|b| {
                            b.apply_signatures(
                                &mut rng,
                                sighash,
                                &[],
                            )
                        })
            }
        }.map_err(Error::from).and_then(|authorized_bundle| {
            let mut result = OrchardBundle {raw_tx : vec![]};
            let _ = write_v5_bundle(Some(&authorized_bundle), &mut result.raw_tx);
            Ok(result)
        })))
    }
}

