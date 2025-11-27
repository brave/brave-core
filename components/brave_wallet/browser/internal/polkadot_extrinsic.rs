// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use ffi::CxxPolkadotDecodeUnsignedTransfer;
use parity_scale_codec::{Compact, Decode, Encode};
use std::fmt;

const EXTRINSIC_VERSION: u8 = 4;
const MULTIADDRESS_TYPE: u8 = 0;

// "Balances" pallet lives at index 4:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/kusama-json.json#L921
// https://github.com/paritytech/polkadot-sdk/blob/69f210b33fce91b23570f3bda64f8e3deff04843/polkadot/runtime/westend/src/lib.rs#L1853-L1854
const POLKADOT_TESTNET: CxxPolkadotChainMetadata =
    CxxPolkadotChainMetadata { balances_pallet_index: 4, transfer_allow_death_call_index: 0 };

// "Balances" pallet lives at index 10:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-kusama-json.json#L969
const POLKADOT_ASSET_HUB_TESTNET: CxxPolkadotChainMetadata =
    CxxPolkadotChainMetadata { balances_pallet_index: 10, transfer_allow_death_call_index: 0 };

// "Balances" pallet lives at index 5:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/polkadot-json.json#L1096
const POLKADOT_MAINNET: CxxPolkadotChainMetadata =
    CxxPolkadotChainMetadata { balances_pallet_index: 5, transfer_allow_death_call_index: 0 };

// "Balances" pallet lives at index 10:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-polkadot-json.json#L969
const POLKADOT_ASSET_HUB_MAINNET: CxxPolkadotChainMetadata =
    CxxPolkadotChainMetadata { balances_pallet_index: 10, transfer_allow_death_call_index: 0 };

const UNSIGNED_TRANSFER_ALLOW_DEATH_MIN_LEN: usize = 1  /* extrinsic version */
                                                   + 1  /* pallet index */
                                                   + 1  /* call index */
                                                   + 1  /* MultiAddress type */
                                                   + 32 /* recipient pubkey */
                                                   + 1  /* SCALE-encoded send amount */
                                                   ;

#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    #[derive(Clone, Copy)]
    pub struct CxxPolkadotDecodeUnsignedTransfer {
        pub recipient: [u8; 32],
        pub send_amount_bytes: [u8; 16],
    }

    extern "Rust" {
        type CxxPolkadotChainMetadata;
        type CxxPolkadotChainMetadataResult;

        fn is_ok(self: &CxxPolkadotChainMetadataResult) -> bool;
        fn error_message(self: &CxxPolkadotChainMetadataResult) -> String;
        fn unwrap(self: &mut CxxPolkadotChainMetadataResult) -> Box<CxxPolkadotChainMetadata>;

        type CxxPolkadotDecodeUnsignedTransferResult;

        fn is_ok(self: &CxxPolkadotDecodeUnsignedTransferResult) -> bool;
        fn error_message(self: &CxxPolkadotDecodeUnsignedTransferResult) -> String;
        fn unwrap(
            self: &mut CxxPolkadotDecodeUnsignedTransferResult,
        ) -> Box<CxxPolkadotDecodeUnsignedTransfer>;

        fn make_chain_metadata(chain_name: &str) -> Box<CxxPolkadotChainMetadataResult>;

        fn encode_unsigned_transfer_allow_death(
            chain_metadata: &CxxPolkadotChainMetadata,
            send_amount_bytes: &[u8; 16],
            pubkey: &[u8; 32],
        ) -> Vec<u8>;

        fn decode_unsigned_transfer_allow_death(
            chain_metadata: &CxxPolkadotChainMetadata,
            mut input: &[u8],
        ) -> Box<CxxPolkadotDecodeUnsignedTransferResult>;
    }
}

#[macro_export]
macro_rules! impl_result {
    ($t:ident, $r:ident) => {
        struct $r(Result<$t, Error>);

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
    /// Invalid SCALE value found.
    InvalidScale,
    /// Invalid metadata such as the wrong pallet index or call index.
    InvalidMetadata,
    /// Invalid length.
    InvalidLength,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::ChainNameNotFound => {
                write!(f, "The supplied chain spec name did not match the whitelist.")
            }
            Error::AlreadyUnwrapped => write!(f, "Already unwrapped."),
            Error::InvalidScale => write!(f, "Invalid SCALE-encoded bytes were found."),
            Error::InvalidMetadata => write!(f, "Invalid chain metadata was encountered."),
            Error::InvalidLength => write!(f, "Invalid length."),
        }
    }
}

struct CxxPolkadotChainMetadata {
    balances_pallet_index: u8,
    transfer_allow_death_call_index: u8,
}

impl_result!(CxxPolkadotChainMetadata, CxxPolkadotChainMetadataResult);
impl_result!(CxxPolkadotDecodeUnsignedTransfer, CxxPolkadotDecodeUnsignedTransferResult);

fn make_chain_metadata(chain_name: &str) -> Box<CxxPolkadotChainMetadataResult> {
    let metadata = match chain_name {
        "Westend" => Ok(POLKADOT_TESTNET),
        "Westend Asset Hub" => Ok(POLKADOT_ASSET_HUB_TESTNET),
        "Polkadot Asset Hub" => Ok(POLKADOT_ASSET_HUB_MAINNET),
        "Polkadot" => Ok(POLKADOT_MAINNET),
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

fn next_n_bytes<'a>(input: &mut &'a [u8], n: usize) -> Result<&'a [u8], Error> {
    let Some((first, last)) = input.split_at_checked(n) else {
        return Err(Error::InvalidLength);
    };

    *input = last;
    Ok(first)
}

fn decode_unsigned_transfer_allow_death_impl(
    chain_metadata: &CxxPolkadotChainMetadata,
    mut input: &[u8],
) -> Result<CxxPolkadotDecodeUnsignedTransfer, Error> {
    let Ok(len) = Compact::<u64>::decode(&mut input) else {
        return Err(Error::InvalidScale);
    };

    let Ok(len) = usize::try_from(len.0) else {
        return Err(Error::InvalidLength);
    };

    if input.len() != len || len < UNSIGNED_TRANSFER_ALLOW_DEATH_MIN_LEN {
        return Err(Error::InvalidLength);
    }

    let CxxPolkadotChainMetadata { balances_pallet_index, transfer_allow_death_call_index } =
        chain_metadata;

    if next_n_bytes(&mut input, 4)?
        != &[
            EXTRINSIC_VERSION,
            *balances_pallet_index,
            *transfer_allow_death_call_index,
            MULTIADDRESS_TYPE,
        ]
    {
        return Err(Error::InvalidMetadata);
    }

    let mut recipient = [0_u8; 32];
    recipient.copy_from_slice(next_n_bytes(&mut input, 32)?);

    debug_assert!(!input.is_empty());

    let Ok(send_amount) = Compact::<u128>::decode(&mut input) else {
        return Err(Error::InvalidScale);
    };

    let send_amount = send_amount.0;
    let mut send_amount_bytes = [0_u8; 16];
    send_amount_bytes.copy_from_slice(&send_amount.to_le_bytes());

    Ok(CxxPolkadotDecodeUnsignedTransfer { recipient, send_amount_bytes })
}

fn decode_unsigned_transfer_allow_death(
    chain_metadata: &CxxPolkadotChainMetadata,
    input: &[u8],
) -> Box<CxxPolkadotDecodeUnsignedTransferResult> {
    let r = decode_unsigned_transfer_allow_death_impl(chain_metadata, input);
    Box::new(CxxPolkadotDecodeUnsignedTransferResult(r))
}
