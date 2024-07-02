// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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

use zcash_primitives::transaction::components::amount::Amount;

use ffi::OrchardOutput;

use rand::rngs::OsRng;
use rand::{RngCore, Error as OtherError};
use rand::CryptoRng;

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
        // https://github.com/rust-random/rand/blob/master/rand_core/src/impls.rs#L38
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

    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), OtherError> {
        Ok(self.fill_bytes(dest))
    }
}


#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = brave_wallet::orchard)]
mod ffi {
    struct OrchardOutput {
        // Amount of zashi being spend
        value: u64,
        // Recipient raw Orchard address.
        // Array size should match kOrchardRawBytesSize
        addr: [u8; 43]
    }

    extern "Rust" {
        type OrchardExtendedSpendingKey;
        type OrchardUnauthorizedBundle;
        type OrchardAuthorizedBundle;

        type OrchardExtendedSpendingKeyResult;
        type OrchardUnauthorizedBundleResult;
        type OrchardAuthorizedBundleResult;

        // OsRng is used
        fn create_orchard_bundle(
            tree_state: &[u8],
            outputs: Vec<OrchardOutput>
        ) -> Box<OrchardUnauthorizedBundleResult>;

        // Testing rng is used with the provided rng_seed
        fn create_testing_orchard_bundle(
            tree_state: &[u8],
            outputs: Vec<OrchardOutput>,
            rng_seed: u64
        ) -> Box<OrchardUnauthorizedBundleResult>;

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
        // External addresses can be used for receiving funds from external
        // senders.
        fn external_address(
            self: &OrchardExtendedSpendingKey,
            diversifier_index: u32
        ) -> [u8; 43];  // Array size should match kOrchardRawBytesSize
        // Internal addresses are used for change or internal shielding and
        // shouldn't be exposed to public.
        fn internal_address(
            self: &OrchardExtendedSpendingKey,
            diversifier_index: u32
        ) -> [u8; 43];  // Array size should match kOrchardRawBytesSize

        fn is_ok(self: &OrchardAuthorizedBundleResult) -> bool;
        fn error_message(self: &OrchardAuthorizedBundleResult) -> String;
        fn unwrap(self: &OrchardAuthorizedBundleResult) -> Box<OrchardAuthorizedBundle>;

        fn is_ok(self: &OrchardUnauthorizedBundleResult) -> bool;
        fn error_message(self: &OrchardUnauthorizedBundleResult) -> String;
        fn unwrap(self: &OrchardUnauthorizedBundleResult) -> Box<OrchardUnauthorizedBundle>;

        // Orchard digest is desribed here https://zips.z.cash/zip-0244#t-4-orchard-digest
        // Used in constructing signature digest and tx id
        fn orchard_digest(self: &OrchardUnauthorizedBundle) -> [u8; 32];  // Array size should match kZCashDigestSize
        // Completes unauthorized bundle to authorized state
        // Signature digest should be constructed as desribed in https://zips.z.cash/zip-0244#signature-digest
        fn complete(self: &OrchardUnauthorizedBundle, sighash: [u8; 32]) -> Box<OrchardAuthorizedBundleResult>;  // Array size should match kZCashDigestSize

        // Orchard part of v5 transaction as described in
        // https://zips.z.cash/zip-0225
        fn raw_tx(self: &OrchardAuthorizedBundle) -> Vec<u8>;
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

// Different random sources are used for testing and for release
// Since Orchard uses randomness we need to mock it to get
// deterministic resuluts in tests.
#[derive(Clone)]
enum OrchardRandomSource {
    OsRng(OsRng),
    MockRng(MockRng),
}

// Unauthorized bundle is a bundle without generated proof, but it
// contains Orchard digest needed to calculate tx signature digests.
#[derive(Clone)]
pub struct OrchardUnauthorizedBundleValue {
    unauthorized_bundle: Bundle<InProgress<Unproven, Unauthorized>, Amount>,
    rng: OrchardRandomSource
}

// Authorized bundle is a bundle where inputs are signed with signature digests
// and proof is generated.
#[derive(Clone)]
pub struct OrchardAuthorizedBundleValue {
    raw_tx: Vec<u8>
}

#[derive(Clone)]
struct OrchardExtendedSpendingKey(ExtendedSpendingKey);
#[derive(Clone)]
struct OrchardAuthorizedBundle(OrchardAuthorizedBundleValue);
#[derive(Clone)]
struct OrchardUnauthorizedBundle(OrchardUnauthorizedBundleValue);

struct OrchardExtendedSpendingKeyResult(Result<OrchardExtendedSpendingKey, Error>);
struct OrchardAuthorizedBundleResult(Result<OrchardAuthorizedBundle, Error>);
struct OrchardUnauthorizedBundleResult(Result<OrchardUnauthorizedBundle, Error>);

impl_result!(OrchardExtendedSpendingKey, OrchardExtendedSpendingKeyResult, ExtendedSpendingKey);
impl_result!(OrchardAuthorizedBundle, OrchardAuthorizedBundleResult, OrchardAuthorizedBundleValue);
impl_result!(OrchardUnauthorizedBundle, OrchardUnauthorizedBundleResult, OrchardUnauthorizedBundleValue);

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

impl OrchardAuthorizedBundle {
    fn raw_tx(self: &OrchardAuthorizedBundle) -> Vec<u8> {
        self.0.raw_tx.clone()
    }
}

fn create_orchard_builder_internal(
    orchard_tree_bytes: &[u8],
    outputs: Vec<OrchardOutput>,
    random_source: OrchardRandomSource
) -> Box<OrchardUnauthorizedBundleResult> {
    use orchard::Anchor;
    use zcash_primitives::merkle_tree::read_commitment_tree;

    // To construct transaction orchard tree state of some block should be provided
    // But in tests we can use empty anchor.
    let anchor = if orchard_tree_bytes.len() > 0 {
        match read_commitment_tree::<MerkleHashOrchard, _, { orchard::NOTE_COMMITMENT_TREE_DEPTH as u8 }>(
                &orchard_tree_bytes[..]) {
            Ok(tree) => Anchor::from(tree.root()),
            Err(_e) => return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::from(OrchardBuildError::AnchorMismatch)))),
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
            None => return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::WrongOutputError)))
        };
    }

    Box::new(OrchardUnauthorizedBundleResult::from(match random_source {
        OrchardRandomSource::OsRng(mut rng) => {
            builder.build(&mut rng)
                .map_err(Error::from)
                .and_then(|builder| {
                    builder.map(|bundle| OrchardUnauthorizedBundleValue {
                        unauthorized_bundle: bundle.0,
                        rng: OrchardRandomSource::OsRng(rng) }).ok_or(Error::BuildError)
                })
        },
        OrchardRandomSource::MockRng(mut rng) => {
            builder.build(&mut rng)
                .map_err(Error::from)
                .and_then(|builder| {
                    builder.map(|bundle| OrchardUnauthorizedBundleValue {
                        unauthorized_bundle: bundle.0,
                        rng: OrchardRandomSource::MockRng(rng) }).ok_or(Error::BuildError)
                })
        }
    }))
}

fn create_orchard_bundle(
    orchard_tree_bytes: &[u8],
    outputs: Vec<OrchardOutput>
) -> Box<OrchardUnauthorizedBundleResult> {
    create_orchard_builder_internal(orchard_tree_bytes, outputs, OrchardRandomSource::OsRng(OsRng))
}

fn create_testing_orchard_bundle(
    orchard_tree_bytes: &[u8],
    outputs: Vec<OrchardOutput>,
    rng_seed: u64
) -> Box<OrchardUnauthorizedBundleResult> {
    create_orchard_builder_internal(orchard_tree_bytes, outputs, OrchardRandomSource::MockRng(MockRng(rng_seed)))
}

impl OrchardUnauthorizedBundle {
    fn orchard_digest(self: &OrchardUnauthorizedBundle) -> [u8; 32] {
        self.0.unauthorized_bundle.commitment().into()
    }

    fn complete(self: &OrchardUnauthorizedBundle, sighash: [u8; 32]) -> Box<OrchardAuthorizedBundleResult> {
        use zcash_primitives::transaction::components::orchard::write_v5_bundle;
        Box::new(OrchardAuthorizedBundleResult::from(match self.0.rng.clone() {
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
            let mut result = OrchardAuthorizedBundleValue {raw_tx : vec![]};
            let _ = write_v5_bundle(Some(&authorized_bundle), &mut result.raw_tx);
            Ok(result)
        })))
    }
}

