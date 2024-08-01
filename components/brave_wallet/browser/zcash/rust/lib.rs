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
use ffi::ShardTree;
use ffi::ShardTreeContext;

use rand::rngs::OsRng;
use rand::{RngCore, Error as OtherError};
use rand::CryptoRng;

use brave_wallet::{
  impl_error
};

use zcash_note_encryption::{
    batch, BatchDomain, Domain, ShieldedOutput, COMPACT_NOTE_SIZE, ENC_CIPHERTEXT_SIZE,
};

use crate::ffi::OrchardCompactAction;

use shardtree::{
    error::ShardTreeError,
    store::{Checkpoint, ShardStore, TreeState},
    LocatedPrunableTree, LocatedTree, PrunableTree, RetentionFlags,
    ShardTree
};

use zcash_client_backend::serialization::shardtree::{read_shard, write_shard};
use cxx::UniquePtr;

pub use brave_wallet::orchard;

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

pub(crate) const PRUNING_DEPTH: u32 = 100;
pub(crate) const SHARD_HEIGHT: u8 = 16;

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

#[allow(unused)]
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
        nullifier: [u8; 32],
        ephemeral_key: [u8; 32],
        cmx: [u8; 32],
        enc_cipher_text : [u8; 52]
    }

    struct ShardAddress {
        level: u8,
        index: u32
    }

    struct ShardTree {
        index: ShardAddress,
        hash: [u8; 32],
        data: Vec<u8>,
        contains_marked: bool
    }

    struct ShardCheckpoint {
    }

    struct ShardCheckpointId {
        block_height: u32
    }

    extern "Rust" {
        type OrchardExtendedSpendingKey;
        type OrchardUnauthorizedBundle;
        type OrchardAuthorizedBundle;

        type BatchOrchardDecodeBundle;

        type OrchardShardTree;

        type OrchardExtendedSpendingKeyResult;
        type OrchardUnauthorizedBundleResult;
        type OrchardAuthorizedBundleResult;

        type BatchOrchardDecodeBundleResult;

        type OrchardShardTreeResult;

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
            fvk_bytes: &[u8; 96],
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
        ) -> [u8; 96];

        fn is_ok(self: &OrchardAuthorizedBundleResult) -> bool;
        fn error_message(self: &OrchardAuthorizedBundleResult) -> String;
        fn unwrap(self: &OrchardAuthorizedBundleResult) -> Box<OrchardAuthorizedBundle>;

        fn is_ok(self: &OrchardUnauthorizedBundleResult) -> bool;
        fn error_message(self: &OrchardUnauthorizedBundleResult) -> String;
        fn unwrap(self: &OrchardUnauthorizedBundleResult) -> Box<OrchardUnauthorizedBundle>;

        fn is_ok(self: &BatchOrchardDecodeBundleResult) -> bool;
        fn error_message(self: &BatchOrchardDecodeBundleResult) -> String;
        fn unwrap(self: &BatchOrchardDecodeBundleResult) -> Box<BatchOrchardDecodeBundle>;

        fn is_ok(self: &OrchardShardTreeResult) -> bool;
        fn error_message(self: &OrchardShardTreeResult) -> String;
        fn unwrap(self: &OrchardShardTreeResult) -> Box<OrchardShardTree>;

        fn size(self :&BatchOrchardDecodeBundle) -> u64;
        fn note_value(self :&BatchOrchardDecodeBundle, index: u64) -> u32;
        fn note_nullifier(self :&BatchOrchardDecodeBundle, fvk: &[u8; 96], index: u64) -> [u8; 32];

        // Orchard digest is desribed here https://zips.z.cash/zip-0244#t-4-orchard-digest
        // Used in constructing signature digest and tx id
        fn orchard_digest(self: &OrchardUnauthorizedBundle) -> [u8; 32];  // Array size should match kZCashDigestSize
        // Completes unauthorized bundle to authorized state
        // Signature digest should be constructed as desribed in https://zips.z.cash/zip-0244#signature-digest
        fn complete(self: &OrchardUnauthorizedBundle, sighash: [u8; 32]) -> Box<OrchardAuthorizedBundleResult>;  // Array size should match kZCashDigestSize

        // Orchard part of v5 transaction as described in
        // https://zips.z.cash/zip-0225
        fn raw_tx(self: &OrchardAuthorizedBundle) -> Vec<u8>;

        fn create_shard_tree(ctx: UniquePtr<ffi::ShardStoreContext>) -> Box<OrchardShardTreeResult>;
        fn insert_subtree_roots(self: &OrchardShardTree, roots: Vec<ShardTree>) -> bool;
        fn insert_commitments(self: &OrchardShardTree, commitments: Vec<ShardTree>) -> bool;
    }

    unsafe extern "C++" {
        include!("brave/components/brave_wallet/browser/zcash/rust/cxx/src/shard_store.h");

        type ShardStoreContext;
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
pub struct OrchardShardTree {
    tree: ShardTree
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
        let fvk = OrchardFVK::from(&self.0);
        return fvk.to_bytes();
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
    fn size(self :&BatchOrchardDecodeBundle) -> u64 {
        return self.0.outputs.len() as u64;
      }

      fn note_value(self :&BatchOrchardDecodeBundle, index: u64) -> u32 {
        return u32::try_from(self.0.outputs[index as usize].note.value().inner()).unwrap();
      }

      fn note_nullifier(self :&BatchOrchardDecodeBundle, fvk: &[u8; 96], index: u64) -> [u8; 32] {
       return self.0.outputs[index as usize].note.nullifier(&OrchardFVK::from_bytes(fvk).unwrap()).to_bytes();
      }
}

// pub(crate) fn get_shard(shard_index: u32) {
//   shard: Optional<ffi::ShardTree> = ffi::get_shard(shard_index);

//   let shard_tree = read_shard(&mut Cursor::new(shard.data)).map_err(Error::Serialization)?;
//   let located_tree = LocatedPrunableTree::from_parts(shard_root_addr, shard_tree);
//   if let Some(root_hash_data) = root_hash {
//       let root_hash = H::read(Cursor::new(root_hash_data)).map_err(Error::Serialization)?;
//       Ok(located_tree.reannotate_root(Some(Arc::new(root_hash))))
//   } else {
//       Ok(located_tree)
//   }

// }

// pub(crate) fn last_shard() {
//     shard: Optional<ffi::ShardTree> = ffi::last_shard(shard_index);
//     let shard_root_addr = Address(shard.address.level, shard.address.index);
//     let shard_tree = read_shard(&mut Cursor::new(shard.data)).map_err(Error::Serialization)?;
//     let located_tree = LocatedPrunableTree::from_parts(shard_root_addr, shard_tree);
//     if let Some(root_hash_data) = root_hash {
//         let root_hash = H::read(Cursor::new(root_hash_data)).map_err(Error::Serialization)?;
//         Ok(located_tree.reannotate_root(Some(Arc::new(root_hash))))
//     } else {
//         Ok(located_tree)
//     }
  
// }

// pub(crate) fn get_shard_roots() {
//     shard: Optional<ffi::ShardTree> = ffi::get_shard(shard_index);

// }

// pub(crate) fn put_shard(subtree: LocatedPrunableTree<H>) {
//     let subtree_root_hash = subtree
//         .root()
//         .annotation()
//         .and_then(|ann| {
//             ann.as_ref().map(|rc| {
//                 let mut root_hash = vec![];
//                 rc.write(&mut root_hash)?;
//                 Ok(root_hash)
//             })
//         })
//         .transpose()
//         .map_err(Error::Serialization)?;

//     let mut subtree_data = vec![];
//     write_shard(&mut subtree_data, subtree.root()).map_err(Error::Serialization)?;

//     let shard_index = subtree.root_addr().index();
//     ffi::put_shard(shard_index, subtree_root_hash, subtree_data);
// }

// fn truncate(&mut self, from: Address) -> Result<(), Self::Error> {
// }

// fn get_cap(&self) -> Result<PrunableTree<Self::H>, Self::Error> {
// }

// fn put_cap(&mut self, cap: PrunableTree<Self::H>) -> Result<(), Self::Error> {
// }

// fn min_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
// }

// fn max_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
// }

// fn add_checkpoint(
//     &mut self,
//     checkpoint_id: Self::CheckpointId,
//     checkpoint: Checkpoint,
// ) -> Result<(), Self::Error> {
// }

// fn checkpoint_count(&self) -> Result<usize, Self::Error> {
// }

// fn get_checkpoint_at_depth(
//     &self,
//     checkpoint_depth: usize,
// ) -> Result<Option<(Self::CheckpointId, Checkpoint)>, Self::Error> {
// }

// fn get_checkpoint(
//     &self,
//     checkpoint_id: &Self::CheckpointId,
// ) -> Result<Option<Checkpoint>, Self::Error> {
// }

// fn with_checkpoints<F>(&mut self, limit: usize, callback: F) -> Result<(), Self::Error>
// where
//     F: FnMut(&Self::CheckpointId, &Checkpoint) -> Result<(), Self::Error>,
// {
// }

// fn update_checkpoint_with<F>(
//     &mut self,
//     checkpoint_id: &Self::CheckpointId,
//     update: F,
// ) -> Result<bool, Self::Error>
// where
//     F: Fn(&mut Checkpoint) -> Result<(), Self::Error>,
// {
//     let tx = self.conn.transaction().map_err(Error::Query)?;
//     let result = update_checkpoint_with(&tx, self.table_prefix, *checkpoint_id, update)?;
//     tx.commit().map_err(Error::Query)?;
//     Ok(result)
// }

// fn remove_checkpoint(&mut self, checkpoint_id: &Self::CheckpointId) -> Result<(), Self::Error> {

// }

// fn truncate_checkpoints(
//     &mut self,
//     checkpoint_id: &Self::CheckpointId,
// ) -> Result<(), Self::Error> {
// }


// fn create_shard_tree(context: &ShardTreeContext) -> Box<ShardTreeResult> {
//     let shard_store = CxxShardStoreImpl(native_context: Rc::new(RefCell::new(context)));
//     if let mut shardtree = ShardTree::new(shard_store, PRUNING_DEPTH.try_into().unwrap()) {

//     } else {

//     }

// }

pub struct CxxShardStoreImpl {
    native_context: Rc<RefCell<UniquePtr<ShardStoreContext>>>
}

impl<H: HashSer, const SHARD_HEIGHT: u8> ShardStore
    for CxxShardStoreImpl<rusqlite::Connection, H, SHARD_HEIGHT>
{
    type H = H;
    type CheckpointId = BlockHeight;
    type Error = Error;

    fn get_shard(
        &self,
        shard_root: Address,
    ) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        Ok(Option::None)
    }

    fn last_shard(&self) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        Result::None
    }

    fn put_shard(&mut self, subtree: LocatedPrunableTree<Self::H>) -> Result<(), Self::Error> {
        Ok(())
    }

    fn get_shard_roots(&self) -> Result<Vec<Address>, Self::Error> {
        Result::None
    }

    fn truncate(&mut self, from: Address) -> Result<(), Self::Error> {
        Ok(())
    }

    fn get_cap(&self) -> Result<PrunableTree<Self::H>, Self::Error> {
        get_cap(&self.conn, self.table_prefix)
    }

    fn put_cap(&mut self, cap: PrunableTree<Self::H>) -> Result<(), Self::Error> {
        put_cap(&self.conn, self.table_prefix, cap)
    }

    fn min_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        min_checkpoint_id(&self.conn, self.table_prefix)
    }

    fn max_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        max_checkpoint_id(&self.conn, self.table_prefix)
    }

    fn add_checkpoint(
        &mut self,
        checkpoint_id: Self::CheckpointId,
        checkpoint: Checkpoint,
    ) -> Result<(), Self::Error> {
        Ok(())
    }

    fn checkpoint_count(&self) -> Result<usize, Self::Error> {
        Ok(0)
    }

    fn get_checkpoint_at_depth(
        &self,
        checkpoint_depth: usize,
    ) -> Result<Option<(Self::CheckpointId, Checkpoint)>, Self::Error> {
        Ok(Option::None)
    }

    fn get_checkpoint(
        &self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<Option<Checkpoint>, Self::Error> {
        Ok(Option::None)
    }

    fn with_checkpoints<F>(&mut self, limit: usize, callback: F) -> Result<(), Self::Error>
    where
        F: FnMut(&Self::CheckpointId, &Checkpoint) -> Result<(), Self::Error>,
    {

    }

    fn update_checkpoint_with<F>(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
        update: F,
    ) -> Result<bool, Self::Error>
    where
        F: Fn(&mut Checkpoint) -> Result<(), Self::Error>,
    {
        Ok(false)
    }

    fn remove_checkpoint(&mut self, checkpoint_id: &Self::CheckpointId) -> Result<(), Self::Error> {
        Ok(())
    }

    fn truncate_checkpoints(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        Ok(())
    }
}
