// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use std::{
    cell::RefCell, cmp::{Ord, Ordering},
    collections::BTreeSet, convert::TryFrom, error, fmt, io::Cursor, marker::PhantomData,
    ops::{Add, Bound, RangeBounds, Sub}, rc::Rc, vec};

use orchard::{
    builder:: {
        BuildError as OrchardBuildError, InProgress, Unauthorized, Unproven
    }, bundle::{commitments, Bundle},
    keys::{FullViewingKey as OrchardFVK,
        PreparedIncomingViewingKey,
        Scope as OrchardScope, SpendingKey},
    note:: {
        ExtractedNoteCommitment, Nullifier, RandomSeed, Rho
    }, note_encryption:: {
        CompactAction, OrchardDomain
    }, keys::SpendAuthorizingKey,
    tree::{MerkleHashOrchard, MerklePath},
    value::NoteValue,
    zip32::{
        ChildIndex as OrchardChildIndex,
        Error as Zip32Error,
        ExtendedSpendingKey},
    Anchor
};

use zcash_note_encryption::EphemeralKeyBytes;
use zcash_protocol::consensus::BlockHeight;
use zcash_primitives::{
    merkle_tree::{read_commitment_tree, HashSer},
    transaction::components::amount::Amount};

use incrementalmerkletree::{
    frontier::{self, Frontier},
    Address,
    Position,
    Retention};

use rand::{rngs::OsRng, CryptoRng, Error as OtherError, RngCore};

use brave_wallet::impl_error;
use std::sync::Arc;
use zcash_note_encryption::{
    batch, Domain, ShieldedOutput, COMPACT_NOTE_SIZE,
};

use shardtree::{
    error::ShardTreeError,
    store::{Checkpoint, ShardStore, TreeState},
    LocatedPrunableTree, LocatedTree, PrunableTree, RetentionFlags,
    ShardTree,
};

use zcash_client_backend::serialization::shardtree::{read_shard, write_shard};
use cxx::UniquePtr;

use pasta_curves::{group::ff::Field, pallas};

use crate::ffi::{
    OrchardCompactAction,
    OrchardOutput,
    OrchardSpend,
    ShardTreeDelegate,
    ShardStoreStatusCode,
    ShardTreeAddress,
    ShardTreeCap,
    ShardTreeCheckpoint,
    ShardTreeCheckpointBundle,
    ShardTreeCheckpointRetention,
    ShardTreeLeaf,
    ShardTreeLeafs,
    ShardTreeShard,
    ShardTreeState
};

use shardtree::error::QueryError;

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

            fn unwrap(self: &mut $r) -> Box<$t> {
                Box::new((self.0.as_mut().unwrap()).take().unwrap())
            }
        }

        impl From<Result<$f, Error>> for $r {
            fn from(result: Result<$f, Error>) -> Self {
                match result {
                    Ok(v) => Self(Ok(Some($t(v)))),
                    Err(e) => Self(Err(e)),
                }
            }
        }
    };
}

pub(crate) const PRUNING_DEPTH: u8 = 100;
pub(crate) const SHARD_HEIGHT: u8 = 16;
pub(crate) const TREE_HEIGHT: u8 = 32;
pub(crate) const CHUNK_SIZE: usize = 1024;

pub(crate) const TESTING_PRUNING_DEPTH: u8 = 10;
pub(crate) const TESTING_SHARD_HEIGHT: u8 = 4;
pub(crate) const TESTING_TREE_HEIGHT: u8 = 8;
pub(crate) const TESTING_CHUNK_SIZE: usize = 16;

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
        addr: [u8; 43],
        memo: [u8; 512],
        // Whether to use memo field in the transacition
        use_memo: bool
    }

    struct MerkleHash {
        hash: [u8; 32]
    }

    struct MerklePath {
        position: u32,
        auth_path: Vec<MerkleHash>,
        root: MerkleHash
    }

    struct OrchardSpend {
        fvk: [u8; 96],
        sk: [u8; 32],
        // Note value
        value: u32,
        addr: [u8; 43],
        rho: [u8; 32],
        r: [u8; 32],
        // Witness merkle path
        merkle_path: MerklePath
    }

    // Encoded orchard output extracted from the transaction
    struct OrchardCompactAction {
        nullifier: [u8; 32],  // kOrchardNullifierSize
        ephemeral_key: [u8; 32],  // kOrchardEphemeralKeySize
        cmx: [u8; 32],  // kOrchardCmxSize
        enc_cipher_text : [u8; 52],  // kOrchardCipherTextSize
        block_id: u32,
        is_block_last_action: bool
    }

    // Represents information about tree state at the end of the block prior to the scan range
    #[derive(Clone)]
    struct ShardTreeState {
        // Frontier is a compressed representation of merkle tree state at some leaf position
        // It allows to compute merkle path to the next leafs without storing all the tree
        // May be empty if no frontier inserted(In case of append)
        frontier: Vec<u8>,
        // Block height of the previous block prior to the scan range
        // The height of the block
        block_height: u32,
        // Tree size of the tree at the end of the prior block, used to calculate leafs indexes
        tree_size: u32
    }

    #[derive(Clone)]
    struct ShardTreeCheckpointRetention {
        checkpoint: bool,
        marked: bool,
        checkpoint_id: u32
    }

    #[derive(Clone)]
    struct ShardTreeLeaf {
        hash: [u8; 32],
        retention: ShardTreeCheckpointRetention
    }

    #[derive(Clone)]
    struct ShardTreeLeafs {
        commitments: Vec<ShardTreeLeaf>
    }

    #[repr(u32)]
    enum ShardStoreStatusCode {
        Ok = 0,
        None = 1,
        Error = 2
    }

    #[derive(Default)]
    struct ShardTreeAddress {
        level: u8,
        index: u32
    }

    #[derive(Default)]
    struct ShardTreeCap {
        data: Vec<u8>,
    }

    #[derive(Default)]
    struct ShardTreeShard {
        address: ShardTreeAddress,
        // Maybe empty on uncompleted shards
        hash: Vec<u8>,
        data: Vec<u8>
    }

    #[derive(Default)]
    struct ShardTreeCheckpoint {
        empty: bool,
        position: u32,
        mark_removed: Vec<u32>
    }

    #[derive(Default)]
    struct ShardTreeCheckpointBundle {
        checkpoint_id: u32,
        checkpoint: ShardTreeCheckpoint
    }

    extern "Rust" {
        type OrchardExtendedSpendingKey;
        type OrchardUnauthorizedBundle;
        type OrchardAuthorizedBundle;
        type BatchOrchardDecodeBundle;
        type OrchardShardTreeBundle;
        type OrchardTestingShardTreeBundle;
        type OrchardWitnessBundle;

        type OrchardExtendedSpendingKeyResult;
        type OrchardUnauthorizedBundleResult;
        type OrchardAuthorizedBundleResult;
        type OrchardWitnessBundleResult;
        type BatchOrchardDecodeBundleResult;
        type OrchardTestingShardTreeBundleResult;
        type OrchardShardTreeBundleResult;

        // OsRng is used
        fn create_orchard_bundle(
            tree_state: &[u8],
            spends: Vec<OrchardSpend>,
            outputs: Vec<OrchardOutput>
        ) -> Box<OrchardUnauthorizedBundleResult>;

        // Creates orchard bundle with mocked rng using provided rng seed.
        // Must not be used in production, only in tests.
        fn create_testing_orchard_bundle(
            tree_state: &[u8],
            spends: Vec<OrchardSpend>,
            outputs: Vec<OrchardOutput>,
            rng_seed: u64
        ) -> Box<OrchardUnauthorizedBundleResult>;

        fn generate_orchard_extended_spending_key_from_seed(
            bytes: &[u8]
        ) -> Box<OrchardExtendedSpendingKeyResult>;

        fn is_ok(self: &OrchardExtendedSpendingKeyResult) -> bool;
        fn error_message(self: &OrchardExtendedSpendingKeyResult) -> String;
        fn unwrap(self: &mut OrchardExtendedSpendingKeyResult) -> Box<OrchardExtendedSpendingKey>;

        fn batch_decode(
            fvk_bytes: &[u8; 96],  // Array size should match kOrchardFullViewKeySize
            prior_tree_state: ShardTreeState,
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

        fn spending_key(
            self: &OrchardExtendedSpendingKey
        ) -> [u8; 32];  // Array size should match kSpendingKeySize

        fn is_ok(self: &OrchardAuthorizedBundleResult) -> bool;
        fn error_message(self: &OrchardAuthorizedBundleResult) -> String;
        fn unwrap(self: &mut OrchardAuthorizedBundleResult) -> Box<OrchardAuthorizedBundle>;

        fn is_ok(self: &OrchardUnauthorizedBundleResult) -> bool;
        fn error_message(self: &OrchardUnauthorizedBundleResult) -> String;
        fn unwrap(self: &mut OrchardUnauthorizedBundleResult) -> Box<OrchardUnauthorizedBundle>;

        fn is_ok(self: &BatchOrchardDecodeBundleResult) -> bool;
        fn error_message(self: &BatchOrchardDecodeBundleResult) -> String;
        fn unwrap(self: &mut BatchOrchardDecodeBundleResult) -> Box<BatchOrchardDecodeBundle>;

        fn size(self :&BatchOrchardDecodeBundle) -> usize;
        fn note_value(self :&BatchOrchardDecodeBundle, index: usize) -> u32;
        // Result array size should match kOrchardNullifierSize
        // fvk array size should match kOrchardFullViewKeySize
        fn note_nullifier(self :&BatchOrchardDecodeBundle, index: usize) -> [u8; 32];
        fn note_rho(self :&BatchOrchardDecodeBundle, index: usize) -> [u8; 32];
        fn note_rseed(self :&BatchOrchardDecodeBundle, index: usize) -> [u8; 32];
        fn note_addr(self :&BatchOrchardDecodeBundle, index: usize) -> [u8; 43];
        fn note_block_height(self :&BatchOrchardDecodeBundle, index: usize) -> u32;
        fn note_commitment_tree_position(self :&BatchOrchardDecodeBundle, index: usize) -> u32;

        fn is_ok(self: &OrchardShardTreeBundleResult) -> bool;
        fn error_message(self: &OrchardShardTreeBundleResult) -> String;
        fn unwrap(self: &mut OrchardShardTreeBundleResult) -> Box<OrchardShardTreeBundle>;

        fn is_ok(self: &OrchardTestingShardTreeBundleResult) -> bool;
        fn error_message(self: &OrchardTestingShardTreeBundleResult) -> String;
        fn unwrap(self: &mut OrchardTestingShardTreeBundleResult) -> Box<OrchardTestingShardTreeBundle>;

        // Orchard digest is desribed here https://zips.z.cash/zip-0244#t-4-orchard-digest
        // Used in constructing signature digest and tx id
        fn orchard_digest(self: &OrchardUnauthorizedBundle) -> [u8; 32];  // Array size should match kZCashDigestSize
        // Completes unauthorized bundle to authorized state
        // Signature digest should be constructed as desribed in https://zips.z.cash/zip-0244#signature-digest
        fn complete(self: &OrchardUnauthorizedBundle, sighash: [u8; 32]) -> Box<OrchardAuthorizedBundleResult>;  // Array size should match kZCashDigestSize

        // Orchard part of v5 transaction as described in
        // https://zips.z.cash/zip-0225
        fn raw_tx(self: &OrchardAuthorizedBundle) -> Vec<u8>;

        // Witness is used to construct zk-proof for the transaction
        fn is_ok(self: &OrchardWitnessBundleResult) -> bool;
        fn error_message(self: &OrchardWitnessBundleResult) -> String;
        fn unwrap(self: &mut OrchardWitnessBundleResult) -> Box<OrchardWitnessBundle>;
        fn size(self :&OrchardWitnessBundle) -> usize;
        fn item(self: &OrchardWitnessBundle, index: usize) -> [u8; 32];

        // Creates shard tree of default orchard height
        fn create_shard_tree(
            delegate: UniquePtr<ShardTreeDelegate>
        ) -> Box<OrchardShardTreeBundleResult>;
        // Creates shard tree of smaller size for testing purposes
        fn create_testing_shard_tree(
            delegate: UniquePtr<ShardTreeDelegate>
        ) -> Box<OrchardTestingShardTreeBundleResult>;

        fn insert_commitments(
            self: &mut OrchardShardTreeBundle,
            scan_result: &mut BatchOrchardDecodeBundle) -> bool;
        fn calculate_witness(
            self: &mut OrchardShardTreeBundle,
            commitment_tree_position: u32,
            checkpoint: u32) -> Box<OrchardWitnessBundleResult>;
        fn truncate(self: &mut OrchardShardTreeBundle, checkpoint_id: u32) -> bool;

        fn insert_commitments(
            self: &mut OrchardTestingShardTreeBundle,
            scan_result: &mut BatchOrchardDecodeBundle) -> bool;
        fn calculate_witness(
            self: &mut OrchardTestingShardTreeBundle,
            commitment_tree_position: u32,
            checkpoint: u32) -> Box<OrchardWitnessBundleResult>;
        fn truncate(self: &mut OrchardTestingShardTreeBundle, checkpoint_id: u32) -> bool;

        // Size matches kOrchardCmxSize in zcash_utils
        fn create_mock_commitment(position: u32, seed: u32) -> [u8; 32];
        fn create_mock_decode_result(
            prior_tree_state: ShardTreeState,
            commitments: ShardTreeLeafs) -> Box<BatchOrchardDecodeBundleResult>;

    }

    unsafe extern "C++" {
        include!("brave/components/brave_wallet/browser/zcash/rust/shard_tree_delegate.h");

        type ShardTreeDelegate;

        fn LastShard(
            &self, into: &mut ShardTreeShard, shard_level: u8) -> ShardStoreStatusCode;
        fn GetShard(
            &self,
            addr: &ShardTreeAddress,
            tree: &mut ShardTreeShard) -> ShardStoreStatusCode;
        fn PutShard(
            &self,
            tree: &ShardTreeShard) -> ShardStoreStatusCode;
        fn GetShardRoots(
            &self, into: &mut Vec<ShardTreeAddress>, shard_level: u8) -> ShardStoreStatusCode;
        fn Truncate(
            &self,
            address: &ShardTreeAddress) -> ShardStoreStatusCode;
        fn GetCap(
            &self,
            into: &mut ShardTreeCap) -> ShardStoreStatusCode;
        fn PutCap(
            &self,
            tree: &ShardTreeCap) -> ShardStoreStatusCode;
        fn MinCheckpointId(
            &self, into: &mut u32) -> ShardStoreStatusCode;
        fn MaxCheckpointId(
            &self, into: &mut u32) -> ShardStoreStatusCode;
        fn AddCheckpoint(
            &self,
            checkpoint_id: u32,
            checkpoint: &ShardTreeCheckpoint) -> ShardStoreStatusCode;
        fn UpdateCheckpoint(
            &self,
            checkpoint_id: u32,
            checkpoint: &ShardTreeCheckpoint) -> ShardStoreStatusCode;
        fn CheckpointCount(
            &self,
            into: &mut usize) -> ShardStoreStatusCode;
        fn CheckpointAtDepth(
            &self,
            depth: usize,
            into_checkpoint_id: &mut u32,
            into_checkpoint: &mut ShardTreeCheckpoint) -> ShardStoreStatusCode;
        fn GetCheckpoint(
            &self,
            checkpoint_id: u32,
            into: &mut ShardTreeCheckpoint) -> ShardStoreStatusCode;
        fn RemoveCheckpoint(
            &self,
            checkpoint_id: u32) -> ShardStoreStatusCode;
        fn TruncateCheckpoint(
            &self,
            checkpoint_id: u32) -> ShardStoreStatusCode;
        fn GetCheckpoints(
            &self,
            limit: usize,
            into: &mut Vec<ShardTreeCheckpointBundle>) -> ShardStoreStatusCode;
    }

}

#[derive(Debug)]
pub enum Error {
    Zip32(Zip32Error),
    WrongInputError,
    WrongOutputError,
    BuildError,
    FvkError,
    OrchardActionFormatError,
    ShardStoreError,
    OrchardBuilder(OrchardBuildError),
    WitnessError,
    SpendError,
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
            Error::FvkError => write!(f, "Error, fvk format error"),
            Error::ShardStoreError => write!(f, "Shard store error"),
            Error::WrongInputError => write!(f, "Wrong input error"),
            Error::WitnessError => write!(f, "Witness error"),
            Error::SpendError => write!(f, "Spend error"),
        }
    }
}

impl error::Error for Error {
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        Some(self)
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
    rng: OrchardRandomSource,
    asks: Vec<SpendAuthorizingKey>
}

// Authorized bundle is a bundle where inputs are signed with signature digests
// and proof is generated.
#[derive(Clone)]
pub struct OrchardAuthorizedBundleValue {
    raw_tx: Vec<u8>
}

#[derive(Clone)]
pub struct DecryptedOrchardOutput {
    note: <OrchardDomain as Domain>::Note,
    block_height: u32,
    commitment_tree_position: u32
}

#[derive(Clone)]
pub struct BatchOrchardDecodeBundleValue {
    fvk: [u8; 96],
    outputs: Vec<DecryptedOrchardOutput>,
    commitments: Vec<(MerkleHashOrchard, Retention<BlockHeight>)>,
    prior_tree_state: ShardTreeState
}

pub struct OrchardGenericShardTreeBundleValue<const T: u8, const S: u8, const P: u8> {
    tree: ShardTree<CxxShardStoreImpl<orchard::tree::MerkleHashOrchard, S>, T, S>
}

type OrchardShardTreeBundleValue =
    OrchardGenericShardTreeBundleValue<TREE_HEIGHT, SHARD_HEIGHT, PRUNING_DEPTH>;
type OrchardTestingShardTreeBundleValue =
    OrchardGenericShardTreeBundleValue<TESTING_TREE_HEIGHT, TESTING_SHARD_HEIGHT, TESTING_PRUNING_DEPTH>;

#[derive(Clone)]
pub struct MarkleHashVec(Vec<MerkleHashOrchard>);

impl From<incrementalmerkletree::MerklePath<MerkleHashOrchard, 8>> for MarkleHashVec {
    fn from(item: incrementalmerkletree::MerklePath<MerkleHashOrchard, 8>) -> Self {
        let mut result : Vec<MerkleHashOrchard> = vec![];
        for elem in item.path_elems() {
            result.push(*elem);
        }
        MarkleHashVec(result)
    }
}


pub struct OrchardWitnessBundleValue {
    path: MarkleHashVec
}

struct OrchardExtendedSpendingKey(ExtendedSpendingKey);
struct OrchardAuthorizedBundle(OrchardAuthorizedBundleValue);
struct OrchardUnauthorizedBundle(OrchardUnauthorizedBundleValue);
struct BatchOrchardDecodeBundle(BatchOrchardDecodeBundleValue);
struct OrchardShardTreeBundle(OrchardShardTreeBundleValue);
struct OrchardTestingShardTreeBundle(OrchardTestingShardTreeBundleValue);
struct OrchardWitnessBundle(OrchardWitnessBundleValue);

struct OrchardExtendedSpendingKeyResult(Result<Option<OrchardExtendedSpendingKey>, Error>);
struct OrchardAuthorizedBundleResult(Result<Option<OrchardAuthorizedBundle>, Error>);
struct OrchardUnauthorizedBundleResult(Result<Option<OrchardUnauthorizedBundle>, Error>);
struct BatchOrchardDecodeBundleResult(Result<Option<BatchOrchardDecodeBundle>, Error>);
struct OrchardShardTreeBundleResult(Result<Option<OrchardShardTreeBundle>, Error>);
struct OrchardWitnessBundleResult(Result<Option<OrchardWitnessBundle>, Error>);
struct OrchardTestingShardTreeBundleResult(Result<Option<OrchardTestingShardTreeBundle>, Error>);

impl_result!(OrchardExtendedSpendingKey, OrchardExtendedSpendingKeyResult, ExtendedSpendingKey);
impl_result!(OrchardAuthorizedBundle, OrchardAuthorizedBundleResult, OrchardAuthorizedBundleValue);
impl_result!(OrchardUnauthorizedBundle, OrchardUnauthorizedBundleResult, OrchardUnauthorizedBundleValue);
impl_result!(BatchOrchardDecodeBundle, BatchOrchardDecodeBundleResult, BatchOrchardDecodeBundleValue);
impl_result!(OrchardShardTreeBundle, OrchardShardTreeBundleResult, OrchardShardTreeBundleValue);
impl_result!(OrchardTestingShardTreeBundle, OrchardTestingShardTreeBundleResult, OrchardTestingShardTreeBundleValue);
impl_result!(OrchardWitnessBundle, OrchardWitnessBundleResult, OrchardWitnessBundleValue);

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

    fn spending_key(
        self: &OrchardExtendedSpendingKey
    ) -> [u8; 32] {
        *self.0.sk().to_bytes()
    }
}

impl OrchardAuthorizedBundle {
    fn raw_tx(self: &OrchardAuthorizedBundle) -> Vec<u8> {
        self.0.raw_tx.clone()
    }
}

fn create_orchard_builder_internal(
    orchard_tree_bytes: &[u8],
    spends: Vec<OrchardSpend>,
    outputs: Vec<OrchardOutput>,
    random_source: OrchardRandomSource
) -> Box<OrchardUnauthorizedBundleResult> {
    // To construct transaction orchard tree state of some block should be provided
    // But in tests we can use empty anchor.
    let anchor = if orchard_tree_bytes.len() > 0 {
        match read_commitment_tree::<MerkleHashOrchard, _, { orchard::NOTE_COMMITMENT_TREE_DEPTH as u8 }>(
                &orchard_tree_bytes[..]) {
            Ok(tree) => Anchor::from(tree.root()),
            Err(_e) => return Box::new(OrchardUnauthorizedBundleResult::from(
                Err(Error::from(OrchardBuildError::AnchorMismatch)))),
        }
    } else {
        orchard::Anchor::empty_tree()
    };

    let mut builder = orchard::builder::Builder::new(
        orchard::builder::BundleType::DEFAULT,
        anchor);

    let mut asks: Vec<SpendAuthorizingKey> = vec![];

    for spend in spends {
        let fvk = OrchardFVK::from_bytes(&spend.fvk);
        if fvk.is_none().into() {
            return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::FvkError)))
        }

        let auth_path = spend.merkle_path.auth_path.iter().map(|v| {
            let hash = MerkleHashOrchard::from_bytes(&v.hash);
            if hash.is_some().into() {
                Ok(hash.unwrap())
            } else {
                Err(Error::WitnessError)
            }
        }).collect::<Result<Vec<MerkleHashOrchard>, _>>();

        if auth_path.is_err() {
            return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::WitnessError)))
        }

        let auth_path_sized : Result<[MerkleHashOrchard; orchard::NOTE_COMMITMENT_TREE_DEPTH], _> = auth_path.unwrap().try_into();
        if auth_path_sized.is_err() {
            return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::WitnessError)))
        }

        let merkle_path = MerklePath::from_parts(
            spend.merkle_path.position,
            auth_path_sized.unwrap(),
        );

        let rho = Rho::from_bytes(&spend.rho);
        if rho.is_none().into() {
            return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::OrchardActionFormatError)))
        }
        let rseed = RandomSeed::from_bytes(spend.r, &rho.unwrap());
        if rseed.is_none().into() {
            return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::OrchardActionFormatError)))
        }
        let addr = orchard::Address::from_raw_address_bytes(&spend.addr);
        if addr.is_none().into() {
            return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::WrongInputError)))
        }

        let note = orchard::Note::from_parts(
            addr.unwrap(),
            NoteValue::from_raw(u64::from(spend.value)),
            rho.unwrap().clone(),
            rseed.unwrap());

        if note.is_none().into() {
            return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::OrchardActionFormatError)))
        }

        let add_spend_result = builder.add_spend(fvk.unwrap(), note.unwrap(), merkle_path);
        if add_spend_result.is_err() {
            return Box::new(OrchardUnauthorizedBundleResult::from(Err(Error::SpendError)))
        }
        asks.push(SpendAuthorizingKey::from(&SpendingKey::from_bytes(spend.sk).unwrap()));
    }

    for out in outputs {
        let _ = match Option::from(orchard::Address::from_raw_address_bytes(&out.addr)) {
            Some(addr) => {
                builder.add_output(None, addr,
                    orchard::value::NoteValue::from_raw(
                        u64::from(out.value)), if out.use_memo { Some(out.memo)} else { Option::None })
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
                        rng: OrchardRandomSource::OsRng(rng),
                        asks: asks }).ok_or(Error::BuildError)
                })
        },
        OrchardRandomSource::MockRng(mut rng) => {
            builder.build(&mut rng)
                .map_err(Error::from)
                .and_then(|builder| {
                    builder.map(|bundle| OrchardUnauthorizedBundleValue {
                        unauthorized_bundle: bundle.0,
                        rng: OrchardRandomSource::MockRng(rng), asks: asks
                    }).ok_or(Error::BuildError)
                })
        }
    }))
}

fn create_orchard_bundle(
    orchard_tree_bytes: &[u8],
    spends: Vec<OrchardSpend>,
    outputs: Vec<OrchardOutput>
) -> Box<OrchardUnauthorizedBundleResult> {
    create_orchard_builder_internal(orchard_tree_bytes, spends, outputs, OrchardRandomSource::OsRng(OsRng))
}

fn create_testing_orchard_bundle(
    orchard_tree_bytes: &[u8],
    spends: Vec<OrchardSpend>,
    outputs: Vec<OrchardOutput>,
    rng_seed: u64
) -> Box<OrchardUnauthorizedBundleResult> {
    create_orchard_builder_internal(orchard_tree_bytes, spends, outputs, OrchardRandomSource::MockRng(MockRng(rng_seed)))
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
                                &self.0.asks,
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
                                &self.0.asks,
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
    prior_tree_state: ShardTreeState,
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

            let compact_action =
                CompactAction::from_parts(nullifier, cmx, ephemeral_key, enc_cipher_text);
            let orchard_domain = OrchardDomain::for_compact_action(&compact_action);

            Ok((orchard_domain, v))
        })
        .collect();

    let input_actions = match input_actions {
        Ok(actions) => actions,
        Err(e) => return Box::new(BatchOrchardDecodeBundleResult::from(Err(e.into())))
    };

    let mut decrypted_len = 0;
    let (decrypted_opts, _decrypted_len) = (
        batch::try_compact_note_decryption(&ivks, &input_actions)
            .into_iter()
            .map(|v| {
                v.map(|((note, _), ivk_idx)| {
                    decrypted_len += 1;
                    (ivks[ivk_idx].clone(), note)
                })
            })
            .collect::<Vec<_>>(),
        decrypted_len,
    );

    let mut found_notes: Vec<DecryptedOrchardOutput> = vec![];
    let mut note_commitments: Vec<(MerkleHashOrchard, Retention<BlockHeight>)> = vec![];

    for (output_idx, ((_, output), decrypted_note)) in
         input_actions.iter().zip(decrypted_opts).enumerate() {
        // If the commitment is the last in the block, ensure that is is retained as a checkpoint
        let is_checkpoint = &output.is_block_last_action;
        let block_id = &output.block_id;
        let retention : Retention<BlockHeight> = match (decrypted_note.is_some(), is_checkpoint) {
            (is_marked, true) => Retention::Checkpoint {
                id: BlockHeight::from_u32(*block_id),
                is_marked,
            },
            (true, false) => Retention::Marked,
            (false, false) => Retention::Ephemeral,
        };
        let commitment = MerkleHashOrchard::from_bytes(&output.cmx);
        if commitment.is_none().into() {
            return Box::new(BatchOrchardDecodeBundleResult::from(Err(Error::OrchardActionFormatError)))
        }
        note_commitments.push((commitment.unwrap(), retention));

        if let Some((_key_id, note)) = decrypted_note {
            found_notes.push(DecryptedOrchardOutput{
                note: note,
                block_height: output.block_id,
                commitment_tree_position: (output_idx as u32) + prior_tree_state.tree_size
            });
        }
    }

    Box::new(BatchOrchardDecodeBundleResult::from(Ok(BatchOrchardDecodeBundleValue {
        fvk: *fvk_bytes,
        outputs: found_notes,
        commitments: note_commitments,
        prior_tree_state: prior_tree_state
     })))
}

impl BatchOrchardDecodeBundle {
    fn size(self :&BatchOrchardDecodeBundle) -> usize {
      self.0.outputs.len()
    }

    fn note_value(self :&BatchOrchardDecodeBundle, index: usize) -> u32 {
      u32::try_from(self.0.outputs[index].note.value().inner()).expect(
          "Outputs are always created from a u32, so conversion back will always succeed")
    }

    fn note_nullifier(self :&BatchOrchardDecodeBundle, index: usize) -> [u8; 32] {
      self.0.outputs[index].note.nullifier(&OrchardFVK::from_bytes(&self.0.fvk).unwrap()).to_bytes()
    }

    fn note_block_height(self :&BatchOrchardDecodeBundle, index: usize) -> u32 {
        self.0.outputs[index].block_height
    }

    fn note_rho(self :&BatchOrchardDecodeBundle, index: usize) -> [u8; 32] {
        self.0.outputs[index].note.rho().to_bytes()

    }

    fn note_rseed(self :&BatchOrchardDecodeBundle, index: usize) -> [u8; 32] {
        *self.0.outputs[index].note.rseed().as_bytes()
    }

    fn note_addr(self :&BatchOrchardDecodeBundle, index: usize) -> [u8; 43] {
        self.0.outputs[index].note.recipient().to_raw_address_bytes()
    }

    fn note_commitment_tree_position(self :&BatchOrchardDecodeBundle, index: usize) -> u32 {
        self.0.outputs[index].commitment_tree_position
    }
}

fn insert_frontier<const COMMITMENT_TREE_DEPTH: u8, const SHARD_HEIGHT: u8>(
    tree: &mut ShardTree<CxxShardStoreImpl<orchard::tree::MerkleHashOrchard, SHARD_HEIGHT>, COMMITMENT_TREE_DEPTH, SHARD_HEIGHT>,
    frontier: &Vec<u8>
) -> bool {
    let frontier_commitment_tree = read_commitment_tree::<MerkleHashOrchard, _, COMMITMENT_TREE_DEPTH>(
        &frontier[..]);

    if frontier_commitment_tree.is_err() {
        return false;
    }

    let frontier_result = tree.insert_frontier(
        frontier_commitment_tree.unwrap().to_frontier(),
        Retention::Marked,
    );

    frontier_result.is_ok()
}

fn insert_commitments<const COMMITMENT_TREE_DEPTH: u8, const CHUNK_SIZE: usize, const SHARD_HEIGHT: u8>(
      shard_tree: &mut ShardTree<CxxShardStoreImpl<orchard::tree::MerkleHashOrchard, SHARD_HEIGHT>, COMMITMENT_TREE_DEPTH, SHARD_HEIGHT>,
      scan_result: &mut BatchOrchardDecodeBundle) -> bool {
    let start_position : u64 = scan_result.0.prior_tree_state.tree_size.into();

    if !scan_result.0.prior_tree_state.frontier.is_empty() {
        let frontier_result = insert_frontier::<COMMITMENT_TREE_DEPTH, SHARD_HEIGHT>(
            shard_tree, &scan_result.0.prior_tree_state.frontier);
        if !frontier_result {
            return false;
        }
    }

    let batch_insert_result = shard_tree.batch_insert(
        Position::from(start_position),
        scan_result.0.commitments.clone().into_iter());

    if batch_insert_result.is_err() {
        return false;
    }

    true
}

impl From<&[MerkleHashOrchard]> for MarkleHashVec {
    fn from(item: &[MerkleHashOrchard]) -> Self {
        let mut result : Vec<MerkleHashOrchard> = vec![];
        for elem in item {
            result.push(*elem);
        }
        MarkleHashVec(result)
    }
}

impl OrchardShardTreeBundle {
    fn insert_commitments(self: &mut OrchardShardTreeBundle,
                          scan_result: &mut BatchOrchardDecodeBundle) -> bool {
        insert_commitments::<TREE_HEIGHT, CHUNK_SIZE, SHARD_HEIGHT>(&mut self.0.tree, scan_result)
    }

    fn calculate_witness(self: &mut OrchardShardTreeBundle,
                         commitment_tree_position: u32,
                         checkpoint: u32) -> Box<OrchardWitnessBundleResult> {
        match self.0.tree.witness_at_checkpoint_id_caching((
                commitment_tree_position as u64).into(), &checkpoint.into()) {
            Ok(witness) => Box::new(OrchardWitnessBundleResult::from(
                Ok(OrchardWitnessBundleValue { path: witness.path_elems().into() }))),
            Err(_e) => Box::new(OrchardWitnessBundleResult::from(Err(Error::WitnessError)))
        }
    }

    fn truncate(self: &mut OrchardShardTreeBundle, checkpoint: u32) -> bool {
        self.0.tree.truncate_removing_checkpoint(&BlockHeight::from_u32(checkpoint)).is_ok()
    }
}

impl OrchardTestingShardTreeBundle {
    fn insert_commitments(self: &mut OrchardTestingShardTreeBundle, scan_result: &mut BatchOrchardDecodeBundle) -> bool {
        insert_commitments::<TESTING_TREE_HEIGHT, TESTING_CHUNK_SIZE, TESTING_SHARD_HEIGHT>(&mut self.0.tree, scan_result)
    }

    fn calculate_witness(self: &mut OrchardTestingShardTreeBundle,
            commitment_tree_position: u32, checkpoint: u32) -> Box<OrchardWitnessBundleResult> {
        match self.0.tree.witness_at_checkpoint_id_caching((commitment_tree_position as u64).into(), &checkpoint.into()) {
            Ok(witness) => Box::new(OrchardWitnessBundleResult::from(Ok(OrchardWitnessBundleValue { path: witness.into() }))),
            Err(_e) => Box::new(OrchardWitnessBundleResult::from(Err(Error::WitnessError)))
        }
    }

    fn truncate(self: &mut OrchardTestingShardTreeBundle, checkpoint: u32) -> bool {
        let result =  self.0.tree.truncate_removing_checkpoint(&BlockHeight::from_u32(checkpoint));
        return result.is_ok() && result.unwrap();
    }
}

impl OrchardWitnessBundle {
    fn size(self: &OrchardWitnessBundle) -> usize {
        self.0.path.0.len()
    }

    fn item(self: &OrchardWitnessBundle, index: usize) -> [u8; 32] {
        self.0.path.0[index].to_bytes()
    }
}

pub struct CxxShardStoreImpl<H, const SHARD_HEIGHT: u8>  {
    delegate: UniquePtr<ShardTreeDelegate>,
    _hash_type: PhantomData<H>,
}

impl From<&ShardTreeCheckpoint> for Checkpoint {
    fn from(item: &ShardTreeCheckpoint) -> Self {
        let tree_state : TreeState =
            if item.empty { TreeState::Empty } else { TreeState::AtPosition((item.position as u64).into()) };
        let marks_removed : BTreeSet<Position> =
            item.mark_removed.iter().map(|x| Position::from(*x as u64)).collect();
        Checkpoint::from_parts(tree_state, marks_removed)
    }
}

impl TryFrom<&Checkpoint> for ShardTreeCheckpoint {
    type Error = Error;
    fn try_from(item: &Checkpoint) -> Result<Self, Self::Error> {
        let position: u32 = match item.tree_state() {
            TreeState::Empty => 0,
            TreeState::AtPosition(pos) => (u64::from(pos)).try_into().map_err(|_| Error::ShardStoreError)?
        };
        let marks_removed : Result<Vec<u32>, Error> = item.marks_removed().into_iter().map(
            |x| u32::try_from(u64::from(*x)).map_err(|_| Error::ShardStoreError)).collect();
        Ok(ShardTreeCheckpoint {
            empty: item.is_tree_empty(),
            position: position,
            mark_removed: marks_removed?
        })
    }
}

impl TryFrom<&Address> for ShardTreeAddress {
    type Error = Error;

    fn try_from(item: &Address) -> Result<Self, Self::Error> {
        let index : u32 =  item.index().try_into().map_err(|_| Error::ShardStoreError)?;
        Ok(ShardTreeAddress{
            level: item.level().into(),
            index: index })
    }
}

impl From<&ShardTreeAddress> for Address {
    fn from(item: &ShardTreeAddress) -> Self {
        Address::from_parts(item.level.into(), item.index.into())
    }
}

impl<H: HashSer> TryFrom<&ShardTreeShard> for LocatedPrunableTree<H> {
    type Error = Error;

    fn try_from(item: &ShardTreeShard) -> Result<Self, Self::Error> {
        let shard_tree =
            read_shard(&mut Cursor::new(&item.data)).map_err(|_| Error::ShardStoreError)?;
        let located_tree: LocatedTree<_, (_, RetentionFlags)> =
            LocatedPrunableTree::from_parts(Address::from(&item.address), shard_tree);
        if !item.hash.is_empty() {
            let root_hash = H::read(Cursor::new(item.hash.clone())).map_err(|_| Error::ShardStoreError)?;
            Ok(located_tree.reannotate_root(Some(Arc::new(root_hash))))
        } else {
            Ok(located_tree)
        }
    }
}

impl <H: HashSer> TryFrom<&ShardTreeCap> for PrunableTree<H> {
    type Error = Error;

    fn try_from(item: &ShardTreeCap) -> Result<Self, Self::Error> {
        read_shard(&mut Cursor::new(&item.data)).map_err(|_| Error::ShardStoreError)
    }
}

impl <H: HashSer> TryFrom<&PrunableTree<H>> for ShardTreeCap {
    type Error = Error;

    fn try_from(item: &PrunableTree<H>) -> Result<Self, Self::Error> {
        let mut data = vec![];
        write_shard(&mut data, item).map_err(|_| Error::ShardStoreError)?;
        Ok(ShardTreeCap {
            data: data
        })
    }
}

impl<H: HashSer> TryFrom<&LocatedPrunableTree<H>> for ShardTreeShard {
    type Error = Error;

    fn try_from(item: &LocatedPrunableTree<H>) -> Result<Self, Self::Error> {
        let subtree_root_hash : Option<Vec<u8>> = item
        .root()
        .annotation()
        .and_then(|ann| {
            ann.as_ref().map(|rc| {
                let mut root_hash = vec![];
                rc.write(&mut root_hash)?;
                Ok(root_hash)
            })
        })
        .transpose()
        .map_err(|_err : std::io::Error| Error::ShardStoreError)?;


        let mut result = ShardTreeShard {
            address: ShardTreeAddress::try_from(&item.root_addr()).map_err(|_| Error::ShardStoreError)?,
            hash: subtree_root_hash.unwrap_or_else(|| vec![]).try_into().map_err(|_| Error::ShardStoreError)?,
            data: vec![]
        };

        write_shard(&mut result.data, &item.root()).map_err(|_| Error::ShardStoreError)?;
        Ok(result)
    }
}

type OrchardCxxShardStoreImpl = CxxShardStoreImpl<orchard::tree::MerkleHashOrchard, SHARD_HEIGHT>;
type TestingCxxShardStoreImpl = CxxShardStoreImpl<orchard::tree::MerkleHashOrchard, TESTING_SHARD_HEIGHT>;

impl<H: HashSer, const SHARD_HEIGHT: u8> ShardStore
    for CxxShardStoreImpl<H, SHARD_HEIGHT>
{
    type H = H;
    type CheckpointId = BlockHeight;
    type Error = Error;

    fn get_shard(
        &self,
        addr: Address,
    ) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        let mut into = ShardTreeShard::default();
        let result = self.delegate.GetShard(
            &ShardTreeAddress::try_from(&addr).map_err(|_| Error::ShardStoreError)?,
            &mut into);
        if result == ShardStoreStatusCode::Ok {
            let tree = LocatedPrunableTree::<H>::try_from(&into)?;
            return Ok(Some(tree));
        } else if result == ShardStoreStatusCode::None {
            return Ok(Option::None);
        } else {
            return Err(Error::ShardStoreError);
        }
    }

    fn last_shard(&self) -> Result<Option<LocatedPrunableTree<Self::H>>, Self::Error> {
        let mut into = ShardTreeShard::default();
        let result =
            self.delegate.LastShard(&mut into, SHARD_HEIGHT);
        if result == ShardStoreStatusCode::Ok {
            let tree = LocatedPrunableTree::<H>::try_from(&into)?;
            return Ok(Some(tree));
        } else if result == ShardStoreStatusCode::None {
            return Ok(Option::None);
        } else {
            return Err(Error::ShardStoreError);
        }
    }

    fn put_shard(&mut self, subtree: LocatedPrunableTree<Self::H>) -> Result<(), Self::Error> {
        let shard = ShardTreeShard::try_from(&subtree).map_err(|_| Error::ShardStoreError)?;
        let result =
            self.delegate.PutShard(&shard);
        if result == ShardStoreStatusCode::Ok {
          return Ok(());
        }
        return Err(Error::ShardStoreError);
    }

    fn get_shard_roots(&self) -> Result<Vec<Address>, Self::Error> {
        let mut input : Vec<ShardTreeAddress> = vec![];
        let result = self.delegate.GetShardRoots(&mut input, SHARD_HEIGHT);
        if result == ShardStoreStatusCode::Ok {
          return Ok(input.into_iter().map(|res| {
            Address::from_parts(res.level.into(), res.index.into())
          }).collect())
        } else if result == ShardStoreStatusCode::None {
          return Ok(vec![])
        } else {
          return Err(Error::ShardStoreError)
        }
    }

    fn truncate(&mut self, from: Address) -> Result<(), Self::Error> {
        let result =
            self.delegate.Truncate(
          &ShardTreeAddress::try_from(&from).map_err(|_| Error::ShardStoreError)?);
        if result == ShardStoreStatusCode::Ok || result == ShardStoreStatusCode::None {
          return Ok(());
        } else {
          return Err(Error::ShardStoreError)
        }
    }

    fn get_cap(&self) -> Result<PrunableTree<Self::H>, Self::Error> {
        let mut input = ShardTreeCap::default();
        let result =
            self.delegate.GetCap(&mut input);

        if result == ShardStoreStatusCode::Ok {
            let tree = PrunableTree::<H>::try_from(&input)?;
            return Ok(tree)
        } else
        if result == ShardStoreStatusCode::None {
            return Ok(PrunableTree::empty());
        } else {
            return Err(Error::ShardStoreError);
        }
    }

    fn put_cap(&mut self, cap: PrunableTree<Self::H>) -> Result<(), Self::Error> {
        let mut result_cap = ShardTreeCap::default();
        write_shard(&mut result_cap.data, &cap).map_err(|_| Error::ShardStoreError)?;

        let result =
        self.delegate.PutCap(&result_cap);
        if result == ShardStoreStatusCode::Ok {
            return Ok(());
        }
        return Err(Error::ShardStoreError);
    }

    fn min_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        let mut input : u32 = 0;
        let result =
            self.delegate.MinCheckpointId(&mut input);
        if result == ShardStoreStatusCode::Ok {
            return Ok(Some(input.into()));
        } else if result == ShardStoreStatusCode::None {
            return Ok(Option::None);
        }
        return Err(Error::ShardStoreError);
    }

    fn max_checkpoint_id(&self) -> Result<Option<Self::CheckpointId>, Self::Error> {
        let mut input : u32 = 0;
        let result = self.delegate.MaxCheckpointId(&mut input);
        if result == ShardStoreStatusCode::Ok {
            return Ok(Some(input.into()));
        } else if result == ShardStoreStatusCode::None {
            return Ok(Option::None);
        }
        return Err(Error::ShardStoreError);
    }

    fn add_checkpoint(
        &mut self,
        checkpoint_id: Self::CheckpointId,
        checkpoint: Checkpoint,
    ) -> Result<(), Self::Error> {
        let ffi_checkpoint_id : u32 = checkpoint_id.try_into().map_err(|_| Error::ShardStoreError)?;
        let result = self.delegate.AddCheckpoint(
            ffi_checkpoint_id,
            &ShardTreeCheckpoint::try_from(&checkpoint)?);
        if result == ShardStoreStatusCode::Ok {
          return Ok(());
        }
        return Err(Error::ShardStoreError);
    }

    fn checkpoint_count(&self) -> Result<usize, Self::Error> {
        let mut input : usize = 0;
        let result = self.delegate.CheckpointCount(&mut input);
        if result == ShardStoreStatusCode::Ok {
            return Ok(input.into());
        } else if result == ShardStoreStatusCode::None {
            return Ok(0);
        }
        return Err(Error::ShardStoreError);
    }

    fn get_checkpoint_at_depth(
        &self,
        checkpoint_depth: usize,
    ) -> Result<Option<(Self::CheckpointId, Checkpoint)>, Self::Error> {
        let mut input_checkpoint_id : u32 = 0;
        let mut input_checkpoint : ShardTreeCheckpoint = ShardTreeCheckpoint::default();

        let result = self.delegate.CheckpointAtDepth(
            checkpoint_depth,
            &mut input_checkpoint_id,
            &mut input_checkpoint);

        if result == ShardStoreStatusCode::Ok {
            return Ok(Some((BlockHeight::from(input_checkpoint_id), Checkpoint::from(&input_checkpoint))));
        } else if result == ShardStoreStatusCode::None {
            return Ok(Option::None);
        }
        return Ok(Option::None);
    }

    fn get_checkpoint(
        &self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<Option<Checkpoint>, Self::Error> {
        let mut input_checkpoint : ShardTreeCheckpoint = ShardTreeCheckpoint::default();

        let result = self.delegate.GetCheckpoint(
            (*checkpoint_id).into(),
            &mut input_checkpoint);

        if result == ShardStoreStatusCode::Ok {
            return Ok(Some(Checkpoint::from(&input_checkpoint)));
        } else if result == ShardStoreStatusCode::None {
            return Ok(Option::None);
        }
        return Ok(Option::None);
    }

    fn with_checkpoints<F>(&mut self, limit: usize, mut callback: F) -> Result<(), Self::Error>
    where
        F: FnMut(&Self::CheckpointId, &Checkpoint) -> Result<(), Self::Error>,
    {
        let mut into : Vec<ShardTreeCheckpointBundle> = vec![];
        let result = self.delegate.GetCheckpoints(limit, &mut into);
        if result == ShardStoreStatusCode::Ok {
            for item in into {
                let checkpoint = Checkpoint::from(&item.checkpoint);
                callback(&BlockHeight::from(item.checkpoint_id), &checkpoint).map_err(|_| Error::ShardStoreError)?;
            }
            return Ok(())
        } else if result == ShardStoreStatusCode::None {
            return Ok(())
        }
        Err(Error::ShardStoreError)
    }

    fn update_checkpoint_with<F>(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
        update: F,
    ) -> Result<bool, Self::Error>
    where
        F: Fn(&mut Checkpoint) -> Result<(), Self::Error>,
    {
        let mut input_checkpoint = ShardTreeCheckpoint::default();
        let result_get_checkpoint =
            self.delegate.GetCheckpoint((*checkpoint_id).into(), &mut input_checkpoint);
        if result_get_checkpoint == ShardStoreStatusCode::Ok {
            return Ok(true);
        } else if result_get_checkpoint == ShardStoreStatusCode::None {
            return Ok(false);
        }

        let mut checkpoint = Checkpoint::from(&input_checkpoint);

        update(&mut checkpoint).map_err(|_| Error::ShardStoreError)?;
        let result_update_checkpoint =
        self.delegate.UpdateCheckpoint(
                (*checkpoint_id).into(), &ShardTreeCheckpoint::try_from(&checkpoint)?);
        if result_update_checkpoint == ShardStoreStatusCode::Ok {
            return Ok(true);
        } else if result_update_checkpoint == ShardStoreStatusCode::None {
            return Ok(false);
        }
        return Err(Error::ShardStoreError);
    }

    fn remove_checkpoint(&mut self, checkpoint_id: &Self::CheckpointId) -> Result<(), Self::Error> {
        let result =
        self.delegate.RemoveCheckpoint((*checkpoint_id).into());
        if result == ShardStoreStatusCode::Ok {
            return Ok(());
        } else if result == ShardStoreStatusCode::None {
            return Ok(());
        }
        return Err(Error::ShardStoreError);
    }

    fn truncate_checkpoints(
        &mut self,
        checkpoint_id: &Self::CheckpointId,
    ) -> Result<(), Self::Error> {
        let result =
        self.delegate.TruncateCheckpoint ((*checkpoint_id).into());
        if result == ShardStoreStatusCode::Ok {
            return Ok(());
        } else if result == ShardStoreStatusCode::None {
            return Ok(());
        }
        return Err(Error::ShardStoreError);
    }
}

fn create_shard_tree(delegate: UniquePtr<ShardTreeDelegate>) -> Box<OrchardShardTreeBundleResult> {
    let shard_store = OrchardCxxShardStoreImpl {
        delegate: delegate,
        _hash_type: Default::default()
    };
    let shardtree = ShardTree::new(shard_store, PRUNING_DEPTH.try_into().unwrap());
    Box::new(OrchardShardTreeBundleResult::from(Ok(OrchardShardTreeBundleValue{tree: shardtree})))
}

fn convert_ffi_commitments(shard_tree_leafs: &ShardTreeLeafs) -> Vec<(MerkleHashOrchard, Retention<BlockHeight>)> {
    shard_tree_leafs.commitments.iter().map(|c| {
        let retention:Retention<BlockHeight> = {
            if c.retention.checkpoint {
                Retention::Checkpoint { id: c.retention.checkpoint_id.into(), is_marked: c.retention.marked }
            } else if c.retention.marked {
                Retention::Marked
            } else {
                Retention::Ephemeral
            }
        };
        let mh = MerkleHashOrchard::from_bytes(&c.hash);
        (mh.unwrap(), retention)
    }).collect()
}

fn create_mock_decode_result(prior_tree_state: ShardTreeState, commitments: ShardTreeLeafs) -> Box<BatchOrchardDecodeBundleResult> {
    Box::new(BatchOrchardDecodeBundleResult::from(Ok(BatchOrchardDecodeBundleValue {
        fvk: [0; 96],
        outputs: vec![],
        commitments: convert_ffi_commitments(&commitments),
        prior_tree_state: prior_tree_state
    })))
}

fn create_testing_shard_tree(delegate: UniquePtr<ShardTreeDelegate>) -> Box<OrchardTestingShardTreeBundleResult> {
    let shard_store: CxxShardStoreImpl<MerkleHashOrchard, 4> = TestingCxxShardStoreImpl {
        delegate: delegate,
        _hash_type: Default::default()
    };
    let shardtree = ShardTree::new(shard_store, TESTING_PRUNING_DEPTH.try_into().unwrap());
    Box::new(OrchardTestingShardTreeBundleResult::from(Ok(OrchardTestingShardTreeBundleValue{tree: shardtree})))
}

fn create_mock_commitment(position: u32, seed: u32) -> [u8; 32] {
    MerkleHashOrchard::from_bytes(
        &(pallas::Base::random(MockRng((position * seed).into())).into())).unwrap().to_bytes()
}