// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

extern crate cxx;
extern crate parity_scale_codec;

use parity_scale_codec::{Compact, Decode, Encode};
use std::fmt;

const EXTRINSIC_VERSION: u8 = 4;
const MULTIADDRESS_TYPE: u8 = 0;

// "Balances" pallet lives at index 4:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/kusama-json.json#L921
// https://github.com/paritytech/polkadot-sdk/blob/69f210b33fce91b23570f3bda64f8e3deff04843/polkadot/runtime/westend/src/lib.rs#L1853-L1854
const POLKADOT_TESTNET_BALANCES_PALLET: u8 = 4;
const POLKADOT_TESTNET_TRANSFER_ALLOW_DEATH_CALL_INDEX: u8 = 0;

// "Balances" pallet lives at index 10:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-kusama-json.json#L969
const POLKADOT_ASSET_HUB_TESTNET_BALANCES_PALLET: u8 = 10;
const POLKADOT_ASSET_HUB_TESTNET_TRANSFER_ALLOW_DEATH_CALL_INDEX: u8 = 0;

// "Balances" pallet lives at index 5:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/polkadot-json.json#L1096
const POLKADOT_MAINNET_BALANCES_PALLET: u8 = 5;
const POLKADOT_MAINNET_TRANSFER_ALLOW_DEATH_CALL_INDEX: u8 = 0;

// "Balances" pallet lives at index 10:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-polkadot-json.json#L969
const POLKADOT_ASSET_HUB_MAINNET_BALANCES_PALLET: u8 = 10;
const POLKADOT_ASSET_HUB_MAINNET_TRANSFER_ALLOW_DEATH_CALL_INDEX: u8 = 0;

const UNSIGNED_TRANSFER_ALLOW_DEATH_MIN_LEN: usize = 4 + 32 + 1;

#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    extern "Rust" {
        type CxxPolkadotChainMetadata;
        type CxxPolkadotChainMetadataResult;

        fn is_ok(self: &CxxPolkadotChainMetadataResult) -> bool;
        fn error_message(self: &CxxPolkadotChainMetadataResult) -> String;
        fn unwrap(self: &mut CxxPolkadotChainMetadataResult) -> Box<CxxPolkadotChainMetadata>;

        fn make_chain_metadata(chain_name: &str) -> Box<CxxPolkadotChainMetadataResult>;

        fn encode_unsigned_transfer_allow_death(
            chain_metadata: &CxxPolkadotChainMetadata,
            send_amount_bytes: &[u8; 16],
            pubkey: &[u8; 32],
        ) -> Vec<u8>;

        fn decode_unsigned_transfer_allow_death(
            chain_metadata: &CxxPolkadotChainMetadata,
            mut input: &[u8],
            pubkey: &mut [u8; 32],
            send_amount: &mut [u8; 16],
        ) -> bool;
    }
}

#[macro_export]
macro_rules! impl_result {
    ($t:ident, $r:ident) => {
        impl $r {
            fn error_message(self: &$r) -> String {
                match &self.0 {
                    Err(e) => e.to_string(),
                    Ok(_) => String::new(),
                }
            }

            fn is_ok(self: &$r) -> bool {
                self.0.is_ok()
            }

            fn unwrap(self: &mut $r) -> Box<$t> {
                match std::mem::replace(&mut self.0, Err(Error::AlreadyUnwrapped)) {
                    Ok(v) => Box::new(v),
                    Err(e) => panic!("{}", e.to_string()),
                }
            }
        }
    };
}

/// Errors that can occur when parsing Polkadot runtime metadata or parsing
/// extrinsics.
#[derive(Clone, Debug)]
pub enum Error {
    /// The Result has already been unwrapped.
    AlreadyUnwrapped,
    /// The supplied chain name did not match our hard-coded whitelist.
    ChainNameNotFound,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::ChainNameNotFound => {
                write!(f, "The supplied chain spec name did not match the whitelist.")
            }
            Error::AlreadyUnwrapped => write!(f, "Already unwrapped"),
        }
    }
}

struct CxxPolkadotChainMetadata {
    balances_pallet_index: u8,
    transfer_allow_death_call_index: u8,
}

struct CxxPolkadotChainMetadataResult(Result<CxxPolkadotChainMetadata, Error>);

impl_result!(CxxPolkadotChainMetadata, CxxPolkadotChainMetadataResult);

fn make_chain_metadata(chain_name: &str) -> Box<CxxPolkadotChainMetadataResult> {
    let metadata = match chain_name {
        "Westend" => Ok(CxxPolkadotChainMetadata {
            balances_pallet_index: POLKADOT_TESTNET_BALANCES_PALLET,
            transfer_allow_death_call_index: POLKADOT_TESTNET_TRANSFER_ALLOW_DEATH_CALL_INDEX,
        }),
        "Westend Asset Hub" => Ok(CxxPolkadotChainMetadata {
            balances_pallet_index: POLKADOT_ASSET_HUB_TESTNET_BALANCES_PALLET,
            transfer_allow_death_call_index:
                POLKADOT_ASSET_HUB_TESTNET_TRANSFER_ALLOW_DEATH_CALL_INDEX,
        }),
        "Polkadot Asset Hub" => Ok(CxxPolkadotChainMetadata {
            balances_pallet_index: POLKADOT_ASSET_HUB_MAINNET_BALANCES_PALLET,
            transfer_allow_death_call_index:
                POLKADOT_ASSET_HUB_MAINNET_TRANSFER_ALLOW_DEATH_CALL_INDEX,
        }),
        "Polkadot" => Ok(CxxPolkadotChainMetadata {
            balances_pallet_index: POLKADOT_MAINNET_BALANCES_PALLET,
            transfer_allow_death_call_index: POLKADOT_MAINNET_TRANSFER_ALLOW_DEATH_CALL_INDEX,
        }),
        _ => Err(Error::ChainNameNotFound),
    };

    Box::new(CxxPolkadotChainMetadataResult(metadata))
}

fn encode_unsigned_transfer_allow_death(
    chain_metadata: &CxxPolkadotChainMetadata,
    send_amount_bytes: &[u8; 16],
    pubkey: &[u8; 32],
) -> Vec<u8> {
    let CxxPolkadotChainMetadata { balances_pallet_index, transfer_allow_death_call_index } =
        chain_metadata;

    let mut buf = Vec::<u8>::with_capacity(256);

    buf.extend_from_slice(&[
        EXTRINSIC_VERSION,
        *balances_pallet_index,
        *transfer_allow_death_call_index,
        MULTIADDRESS_TYPE,
    ]);

    buf.extend_from_slice(pubkey);

    Compact(u128::from_le_bytes(*send_amount_bytes)).encode_to(&mut buf);

    Compact(buf.len() as u64).using_encoded(|encoded_len| {
        buf.splice(0..0, encoded_len.iter().copied());
    });

    buf
}

fn decode_unsigned_transfer_allow_death(
    chain_metadata: &CxxPolkadotChainMetadata,
    mut input: &[u8],
    pubkey: &mut [u8; 32],
    send_amount_bytes: &mut [u8; 16],
) -> bool {
    let Ok(len) = Compact::<u64>::decode(&mut input) else {
        return false;
    };

    let len = len.0 as usize;
    if input.len() != len {
        return false;
    }

    if len < UNSIGNED_TRANSFER_ALLOW_DEATH_MIN_LEN {
        return false;
    }

    if input[0] != EXTRINSIC_VERSION {
        return false;
    }
    input = &input[1..];

    let CxxPolkadotChainMetadata { balances_pallet_index, transfer_allow_death_call_index } =
        chain_metadata;

    if input[0] != *balances_pallet_index {
        return false;
    }
    input = &input[1..];

    if input[0] != *transfer_allow_death_call_index {
        return false;
    }
    input = &input[1..];

    if input[0] != MULTIADDRESS_TYPE {
        return false;
    }

    pubkey.copy_from_slice(&input[1..1 + 32]);
    input = &input[1 + 32..];

    let Ok(send_amount) = Compact::<u128>::decode(&mut input) else {
        return false;
    };

    let send_amount = send_amount.0;

    send_amount_bytes.copy_from_slice(&send_amount.to_le_bytes());

    true
}
