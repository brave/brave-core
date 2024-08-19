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
    keys::PreparedIncomingViewingKey,
    zip32::Error as Zip32Error,
    zip32::ExtendedSpendingKey,
    tree::MerkleHashOrchard,
    note_encryption:: {
        OrchardDomain,
        CompactAction
    },
    note:: {
        Nullifier,
        ExtractedNoteCommitment
    }
};

use zcash_note_encryption::EphemeralKeyBytes;
use zcash_primitives::transaction::components::amount::Amount;

use ffi::OrchardOutput;

use rand::rngs::OsRng;
use rand::{RngCore, Error as OtherError};
use rand::CryptoRng;

use brave_wallet::{
  impl_error
};

use zcash_note_encryption::{
    batch, Domain, ShieldedOutput, COMPACT_NOTE_SIZE,
};

use crate::ffi::OrchardCompactAction;

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

// Used for determenistic zk-proof generation in the
// brave_wallet unittests.
// Must not be used in production see create_testing_orchard_bundle
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
        value: u32,
        // Recipient raw Orchard address.
        // Array size should match kOrchardRawBytesSize
        addr: [u8; 43]
    }

    // Encoded orchard output extracted from the transaction
    struct OrchardCompactAction {
        nullifier: [u8; 32],  // kOrchardNullifierSize
        ephemeral_key: [u8; 32],  // kOrchardEphemeralKeySize
        cmx: [u8; 32],  // kOrchardCmxSize
        enc_cipher_text : [u8; 52]  // kOrchardCipherTextSize
    }

    extern "Rust" {
        type OrchardExtendedSpendingKey;
        type OrchardUnauthorizedBundle;
        type OrchardAuthorizedBundle;

        type BatchOrchardDecodeBundle;

        type OrchardExtendedSpendingKeyResult;
        type OrchardUnauthorizedBundleResult;
        type OrchardAuthorizedBundleResult;

        type BatchOrchardDecodeBundleResult;

        // OsRng is used
        fn create_orchard_bundle(
            tree_state: &[u8],
            outputs: Vec<OrchardOutput>
        ) -> Box<OrchardUnauthorizedBundleResult>;

        // Creates orchard bundle with mocked rng using provided rng seed.
        // Must not be used in production, only in tests.
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

        fn batch_decode(
            fvk_bytes: &[u8; 96],  // Array size should match kOrchardFullViewKeySize
            actions: Vec<OrchardCompactAction>
        ) -> Box<BatchOrchardDecodeBundleResult>;

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
        fn full_view_key(
            self: &OrchardExtendedSpendingKey
        ) -> [u8; 96];  // Array size sohuld match kOrchardFullViewKeySize

        fn is_ok(self: &OrchardAuthorizedBundleResult) -> bool;
        fn error_message(self: &OrchardAuthorizedBundleResult) -> String;
        fn unwrap(self: &OrchardAuthorizedBundleResult) -> Box<OrchardAuthorizedBundle>;

        fn is_ok(self: &OrchardUnauthorizedBundleResult) -> bool;
        fn error_message(self: &OrchardUnauthorizedBundleResult) -> String;
        fn unwrap(self: &OrchardUnauthorizedBundleResult) -> Box<OrchardUnauthorizedBundle>;

        fn is_ok(self: &BatchOrchardDecodeBundleResult) -> bool;
        fn error_message(self: &BatchOrchardDecodeBundleResult) -> String;
        fn unwrap(self: &BatchOrchardDecodeBundleResult) -> Box<BatchOrchardDecodeBundle>;

        fn size(self :&BatchOrchardDecodeBundle) -> usize;
        fn note_value(self :&BatchOrchardDecodeBundle, index: usize) -> u32;
        // Result array size should match kOrchardNullifierSize
        // fvk array size should match kOrchardFullViewKeySize
        fn note_nullifier(self :&BatchOrchardDecodeBundle, fvk: &[u8; 96], index: usize) -> [u8; 32];

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
    FvkError,
    OrchardActionFormatError,
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
            Error::OrchardActionFormatError => write!(f, "Error, orchard action format error"),
            Error::FvkError => write!(f, "Error, fvk format error")
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
pub struct DecryptedOrchardOutput {
    note: <OrchardDomain as Domain>::Note
}
#[derive(Clone)]
pub struct BatchOrchardDecodeBundleValue {
    outputs: Vec<DecryptedOrchardOutput>
}

#[derive(Clone)]
struct OrchardExtendedSpendingKey(ExtendedSpendingKey);
#[derive(Clone)]
struct OrchardAuthorizedBundle(OrchardAuthorizedBundleValue);
#[derive(Clone)]
struct OrchardUnauthorizedBundle(OrchardUnauthorizedBundleValue);
#[derive(Clone)]
struct BatchOrchardDecodeBundle(BatchOrchardDecodeBundleValue);

struct OrchardExtendedSpendingKeyResult(Result<OrchardExtendedSpendingKey, Error>);
struct OrchardAuthorizedBundleResult(Result<OrchardAuthorizedBundle, Error>);
struct OrchardUnauthorizedBundleResult(Result<OrchardUnauthorizedBundle, Error>);
struct BatchOrchardDecodeBundleResult(Result<BatchOrchardDecodeBundle, Error>);

impl_result!(OrchardExtendedSpendingKey, OrchardExtendedSpendingKeyResult, ExtendedSpendingKey);
impl_result!(OrchardAuthorizedBundle, OrchardAuthorizedBundleResult, OrchardAuthorizedBundleValue);
impl_result!(OrchardUnauthorizedBundle, OrchardUnauthorizedBundleResult, OrchardUnauthorizedBundleValue);

impl_result!(BatchOrchardDecodeBundle, BatchOrchardDecodeBundleResult, BatchOrchardDecodeBundleValue);

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

    fn full_view_key(
        self: &OrchardExtendedSpendingKey
    ) -> [u8; 96] {
        OrchardFVK::from(&self.0).to_bytes()
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
                    orchard::value::NoteValue::from_raw(u64::from(out.value)), None)
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

impl ShieldedOutput<OrchardDomain, COMPACT_NOTE_SIZE> for OrchardCompactAction {
    fn ephemeral_key(&self) -> EphemeralKeyBytes {
        EphemeralKeyBytes(self.ephemeral_key)
    }

    fn cmstar_bytes(&self) -> [u8; 32] {
        self.cmx
    }

    fn enc_ciphertext(&self) -> &[u8; COMPACT_NOTE_SIZE] {
        &self.enc_cipher_text
    }
}

fn batch_decode(
    fvk_bytes: &[u8; 96],
    actions: Vec<OrchardCompactAction>
) -> Box<BatchOrchardDecodeBundleResult> {
    let fvk = match OrchardFVK::from_bytes(fvk_bytes) {
        Some(fvk) => fvk,
        None => return Box::new(BatchOrchardDecodeBundleResult::from(Err(Error::FvkError)))
    };

    let ivks = [
        PreparedIncomingViewingKey::new(&fvk.to_ivk(OrchardScope::External)),
        PreparedIncomingViewingKey::new(&fvk.to_ivk(OrchardScope::Internal))
    ];

    let input_actions: Result<Vec<(OrchardDomain, OrchardCompactAction)>, Error> = actions
        .into_iter()
        .map(|v| {
            let nullifier_ctopt = Nullifier::from_bytes(&v.nullifier);
            let nullifier = if nullifier_ctopt.is_none().into() {
                Err(Error::OrchardActionFormatError)
            } else {
                Ok(nullifier_ctopt.unwrap())
            }?;

            let cmx_ctopt = ExtractedNoteCommitment::from_bytes(&v.cmx);
            let cmx = if cmx_ctopt.is_none().into() {
                Err(Error::OrchardActionFormatError)
            } else {
                Ok(cmx_ctopt.unwrap())
            }?;

            let ephemeral_key = EphemeralKeyBytes(v.ephemeral_key);
            let enc_cipher_text = v.enc_cipher_text;

            let compact_action = CompactAction::from_parts(nullifier, cmx, ephemeral_key, enc_cipher_text);
            let orchard_domain = OrchardDomain::for_compact_action(&compact_action);

            Ok((orchard_domain, v))
        })
        .collect();

    let input_actions = match input_actions {
        Ok(actions) => actions,
        Err(e) => return Box::new(BatchOrchardDecodeBundleResult::from(Err(e.into())))
    };

    let decrypted_outputs = batch::try_compact_note_decryption(&ivks, &input_actions.as_slice())
    .into_iter()
    .map(|res| {
        res.map(|((note, _recipient), _ivk_idx)| DecryptedOrchardOutput {
            note: note
        })
    })
    .filter_map(|x| x)
    .collect::<Vec<_>>();

    Box::new(BatchOrchardDecodeBundleResult::from(Ok(BatchOrchardDecodeBundleValue { outputs: decrypted_outputs })))
}

impl BatchOrchardDecodeBundle {
    fn size(self :&BatchOrchardDecodeBundle) -> usize {
      self.0.outputs.len()
    }

    fn note_value(self :&BatchOrchardDecodeBundle, index: usize) -> u32 {
      u32::try_from(self.0.outputs[index].note.value().inner()).expect(
          "Outputs are always created from a u32, so conversion back will always succeed")
    }

    fn note_nullifier(self :&BatchOrchardDecodeBundle, fvk: &[u8; 96], index: usize) -> [u8; 32] {
      self.0.outputs[index].note.nullifier(&OrchardFVK::from_bytes(fvk).unwrap()).to_bytes()
    }
}

