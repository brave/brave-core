// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

extern crate cxx;
extern crate parity_scale_codec;

use std::collections::HashMap;

use parity_scale_codec::{Compact, Decode, Encode};

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
        type ChainMetadata;

        fn make_chain_metadata() -> Box<ChainMetadata>;
        fn add_chain_metadata(self: &mut ChainMetadata, chain_id: &str, chain_name: &str) -> bool;
        fn has_chain_metadata(self: &ChainMetadata, chain_id: &str) -> bool;

        fn encode_unsigned_transfer_allow_death(
            chain_data: &ChainMetadata,
            chain_id: &str,
            send_amount_bytes: &[u8; 16],
            pubkey: &[u8; 32],
        ) -> Vec<u8>;

        fn decode_unsigned_transfer_allow_death(
            chain_data: &ChainMetadata,
            chain_id: &str,
            mut input: &[u8],
            pubkey: &mut [u8; 32],
            send_amount: &mut [u8; 16],
        ) -> bool;
    }
}

struct ChainMetadata {
    metadata: HashMap<String, MetaData>,
}

struct MetaData {
    balances_pallet_index: u8,
    transfer_allow_death_call_index: u8,
}

fn make_chain_metadata() -> Box<ChainMetadata> {
    Box::new(ChainMetadata { metadata: HashMap::default() })
}

impl ChainMetadata {
    fn add_chain_metadata(self: &mut ChainMetadata, chain_id: &str, chain_name: &str) -> bool {
        if self.metadata.contains_key(chain_id) {
            return true;
        }

        let meta_data = match chain_name {
            "Westend" => MetaData {
                balances_pallet_index: POLKADOT_TESTNET_BALANCES_PALLET,
                transfer_allow_death_call_index: POLKADOT_TESTNET_TRANSFER_ALLOW_DEATH_CALL_INDEX,
            },
            "Westend Asset Hub" => MetaData {
                balances_pallet_index: POLKADOT_ASSET_HUB_TESTNET_BALANCES_PALLET,
                transfer_allow_death_call_index:
                    POLKADOT_ASSET_HUB_TESTNET_TRANSFER_ALLOW_DEATH_CALL_INDEX,
            },
            "Polkadot Asset Hub" => MetaData {
                balances_pallet_index: POLKADOT_ASSET_HUB_MAINNET_BALANCES_PALLET,
                transfer_allow_death_call_index:
                    POLKADOT_ASSET_HUB_MAINNET_TRANSFER_ALLOW_DEATH_CALL_INDEX,
            },
            "Polkadot" => MetaData {
                balances_pallet_index: POLKADOT_MAINNET_BALANCES_PALLET,
                transfer_allow_death_call_index: POLKADOT_MAINNET_TRANSFER_ALLOW_DEATH_CALL_INDEX,
            },
            _ => return false,
        };

        self.metadata.insert(chain_id.to_string(), meta_data);
        true
    }

    fn has_chain_metadata(self: &ChainMetadata, chain_id: &str) -> bool {
        self.metadata.contains_key(chain_id)
    }
}

fn encode_unsigned_transfer_allow_death(
    chain_data: &ChainMetadata,
    chain_id: &str,
    send_amount_bytes: &[u8; 16],
    pubkey: &[u8; 32],
) -> Vec<u8> {
    let Some(MetaData { balances_pallet_index, transfer_allow_death_call_index }) =
        chain_data.metadata.get(chain_id)
    else {
        return Vec::new();
    };

    let mut buf = vec![
        EXTRINSIC_VERSION,
        *balances_pallet_index,
        *transfer_allow_death_call_index,
        MULTIADDRESS_TYPE,
    ];

    buf.extend_from_slice(pubkey);

    Compact(u128::from_le_bytes(*send_amount_bytes)).encode_to(&mut buf);

    Compact(buf.len() as u64).using_encoded(|encoded_len| {
        buf.splice(0..0, encoded_len.iter().copied());
    });

    buf
}

fn decode_unsigned_transfer_allow_death(
    chain_data: &ChainMetadata,
    chain_id: &str,
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

    let Some(MetaData { balances_pallet_index, transfer_allow_death_call_index }) =
        chain_data.metadata.get(chain_id)
    else {
        return false;
    };

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
