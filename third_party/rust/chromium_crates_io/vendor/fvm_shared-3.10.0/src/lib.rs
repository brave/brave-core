// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

#[macro_use]
extern crate lazy_static;

use address::Address;
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
pub mod version;

use econ::TokenAmount;
use fvm_ipld_encoding::ipld_block::IpldBlock;

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
