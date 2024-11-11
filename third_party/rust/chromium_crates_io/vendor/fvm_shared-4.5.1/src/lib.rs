// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

#![cfg_attr(coverage_nightly, feature(coverage_attribute))]

#[macro_use]
extern crate lazy_static;

use address::Address;
use cid::Cid;
use clock::ChainEpoch;

pub mod address;
pub mod bigint;
pub mod chainid;
pub mod clock;
pub mod commcid;
pub mod consensus;
pub mod crypto;
pub mod deal;
pub mod econ;
pub mod error;
pub mod event;
pub mod math;
pub mod message;
pub mod piece;
pub mod randomness;
pub mod receipt;
pub mod reward;
pub mod sector;
pub mod smooth;
pub mod state;
pub mod sys;
pub mod upgrade;
pub mod version;

use cid::multihash::Multihash;
use crypto::hash::SupportedHashes;
use econ::TokenAmount;
use fvm_ipld_encoding::ipld_block::IpldBlock;
use fvm_ipld_encoding::DAG_CBOR;

use crate::error::ExitCode;

lazy_static! {
    /// Total Filecoin available to the network.
    pub static ref TOTAL_FILECOIN: TokenAmount = TokenAmount::from_whole(TOTAL_FILECOIN_BASE);

    /// Zero address used to avoid allowing it to be used for verification.
    /// This is intentionally disallowed because it is an edge case with Filecoin's BLS
    /// signature verification.
    pub static ref ZERO_ADDRESS: Address = address::Network::Mainnet.parse_address("f3yaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaby2smx7a").unwrap();
}

/// Codec for raw data.
pub const IPLD_RAW: u64 = 0x55;

/// Multihash code for the identity hash function.
pub const IDENTITY_HASH: u64 = 0x0;

/// The maximum supported CID size.
pub const MAX_CID_LEN: usize = 100;

/// Identifier for Actors, includes builtin and initialized actors
pub type ActorID = u64;

/// Default bit width for the hamt in the filecoin protocol.
pub const HAMT_BIT_WIDTH: u32 = 5;
/// Total gas limit allowed per block. This is shared across networks.
pub const BLOCK_GAS_LIMIT: u64 = 10_000_000_000;
/// Total Filecoin supply.
pub const TOTAL_FILECOIN_BASE: i64 = 2_000_000_000;

// Epochs
/// Lookback height for retrieving ticket randomness.
pub const TICKET_RANDOMNESS_LOOKBACK: ChainEpoch = 1;
/// Epochs to look back for verifying PoSt proofs.
pub const WINNING_POST_SECTOR_SET_LOOKBACK: ChainEpoch = 10;

/// The expected number of block producers in each epoch.
pub const BLOCKS_PER_EPOCH: u64 = 5;

/// Allowable clock drift in validations.
pub const ALLOWABLE_CLOCK_DRIFT: u64 = 1;

/// Config trait which handles different network configurations.
pub trait NetworkParams {
    /// Total filecoin available to network.
    const TOTAL_FILECOIN: i64;

    /// Available rewards for mining.
    const MINING_REWARD_TOTAL: i64;

    /// Initial reward actor balance. This function is only called in genesis setting up state.
    fn initial_reward_balance() -> TokenAmount {
        TokenAmount::from_whole(Self::MINING_REWARD_TOTAL)
    }
}

/// Params for the network. This is now continued on into mainnet and is static across networks.
// * This can be removed in the future if the new testnet is configred at build time
// * but the reason to keep as is, is for an easier transition to runtime configuration.
pub struct DefaultNetworkParams;

impl NetworkParams for DefaultNetworkParams {
    const TOTAL_FILECOIN: i64 = TOTAL_FILECOIN_BASE;
    const MINING_REWARD_TOTAL: i64 = 1_400_000_000;
}

/// Method number indicator for calling actor methods.
pub type MethodNum = u64;

/// Base actor send method.
pub const METHOD_SEND: MethodNum = 0;
/// Base actor constructor method.
pub const METHOD_CONSTRUCTOR: MethodNum = 1;

/// The outcome of a `Send`, covering its ExitCode and optional return data
#[derive(Debug, PartialEq, Eq, Clone)]
pub struct Response {
    pub exit_code: ExitCode,
    pub return_data: Option<IpldBlock>,
}

// This is a somewhat nasty hack that lets us unwrap in a const function.
const fn const_unwrap<T: Copy, E>(r: Result<T, E>) -> T {
    let v = match r {
        Ok(v) => v,
        Err(_) => panic!(), // aborts at compile time
    };
    // given the match above, this will _only_ drop `Ok(T)` where `T` is copy, so it won't actually
    // do anything. However, we need it to convince the compiler that we never drop `Err(E)` because
    // `E` likely isn't `Copy` (and therefore can't be "dropped" at compile time.
    std::mem::forget(r);
    v
}

// 45b0cfc220ceec5b7c1c62c4d4193d38e4eba48e8815729ce75f9c0ab0e4c1c0
const EMPTY_ARR_HASH_DIGEST: &[u8] = &[
    0x45, 0xb0, 0xcf, 0xc2, 0x20, 0xce, 0xec, 0x5b, 0x7c, 0x1c, 0x62, 0xc4, 0xd4, 0x19, 0x3d, 0x38,
    0xe4, 0xeb, 0xa4, 0x8e, 0x88, 0x15, 0x72, 0x9c, 0xe7, 0x5f, 0x9c, 0x0a, 0xb0, 0xe4, 0xc1, 0xc0,
];

// bafy2bzacebc3bt6cedhoyw34drrmjvazhu4oj25er2ebk4u445pzycvq4ta4a
pub const EMPTY_ARR_CID: Cid = Cid::new_v1(
    DAG_CBOR,
    const_unwrap(Multihash::wrap(
        SupportedHashes::Blake2b256 as u64,
        EMPTY_ARR_HASH_DIGEST,
    )),
);

#[test]
fn test_empty_arr_cid() {
    use fvm_ipld_encoding::to_vec;
    use multihash_codetable::{Code, MultihashDigest};

    let empty = to_vec::<[(); 0]>(&[]).unwrap();
    let expected = Cid::new_v1(DAG_CBOR, Code::Blake2b256.digest(&empty));
    assert_eq!(EMPTY_ARR_CID, expected);
}
