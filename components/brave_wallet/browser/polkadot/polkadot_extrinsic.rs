// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

extern crate cxx;
extern crate parity_scale_codec;

use parity_scale_codec::Compact;
use parity_scale_codec::Decode;
use parity_scale_codec::Encode;

const EXTRINSIC_VERSION: u8 = 4;
const MULTIADDRESS_TYPE: u8 = 0;

// "Balances" pallet lives at index 4:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/kusama-json.json#L921
// https://github.com/paritytech/polkadot-sdk/blob/69f210b33fce91b23570f3bda64f8e3deff04843/polkadot/runtime/westend/src/lib.rs#L1853-L1854
const POLKADOT_TESTNET_BALANCES_PALLET: u8 = 4;
const POLKADOT_TESTNET_TRANSFER_ALLOW_DEATH_CALL_INDEX: u8 = 0;

// "Balances" pallet lives at index 5:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/polkadot-json.json#L1096
const POLKADOT_MAINNET_BALANCES_PALLET: u8 = 5;
const POLKADOT_MAINNET_TRANSFER_ALLOW_DEATH_CALL_INDEX: u8 = 0;

const UNSIGNED_TRANSFER_ALLOW_DEATH_MIN_LEN: usize = 4 + 32 + 1;

#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    extern "Rust" {
        fn encode_unsigned_transfer_allow_death(
            send_amount_high: u64,
            send_amount_low: u64,
            pubkey: &[u8; 32],
            balances_pallet_idx: u8,
            transfer_allow_death_call_idx: u8,
        ) -> Vec<u8>;

        fn decode_unsigned_transfer_allow_death(
            mut input: &[u8],
            pubkey: &mut [u8],
            send_amount_high: &mut u64,
            send_amount_low: &mut u64,
        ) -> bool;
    }
}

fn encode_unsigned_transfer_allow_death(
    send_amount_high: u64,
    send_amount_low: u64,
    pubkey: &[u8; 32],
    balances_pallet_idx: u8,
    transfer_allow_death_call_idx: u8,
) -> Vec<u8> {
    let mut buf = vec![
        EXTRINSIC_VERSION,
        balances_pallet_idx,
        transfer_allow_death_call_idx,
        MULTIADDRESS_TYPE,
    ];

    buf.extend_from_slice(pubkey);

    let send_amount: u128 = ((send_amount_high as u128) << 64) | (send_amount_low as u128);
    Compact(send_amount).encode_to(&mut buf);

    Compact(buf.len() as u64).using_encoded(|encoded_len| {
        buf.splice(0..0, encoded_len.iter().copied());
    });

    buf
}

fn decode_unsigned_transfer_allow_death(
    mut input: &[u8],
    pubkey: &mut [u8],
    send_amount_high: &mut u64,
    send_amount_low: &mut u64,
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

    if input[0] != POLKADOT_MAINNET_BALANCES_PALLET && input[0] != POLKADOT_TESTNET_BALANCES_PALLET
    {
        return false;
    }
    input = &input[1..];

    if input[0] != POLKADOT_MAINNET_TRANSFER_ALLOW_DEATH_CALL_INDEX
        || input[0] != POLKADOT_TESTNET_TRANSFER_ALLOW_DEATH_CALL_INDEX
    {
        return false;
    }
    input = &input[1..];

    if input[0] != MULTIADDRESS_TYPE {
        return false;
    }

    println!("{}", input.iter().map(|b| format!("{b:02x}")).collect::<String>());
    pubkey.copy_from_slice(&input[1..1 + 32]);
    input = &input[1 + 32..];

    let Ok(send_amount) = Compact::<u128>::decode(&mut input) else {
        return false;
    };

    let send_amount = send_amount.0;

    *send_amount_high = (send_amount >> 64) as u64;
    *send_amount_low = (send_amount & 0xffffffffffffffff) as u64;

    true
}
