// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use crate::polkadot_scale_utils::decode_scale;
use crate::Error;

const ASSETS_STORAGE_MAP_PREFIX_SIZE: usize = 32;
const BLAKE_TWO_128_CONCAT_U32_STORAGE_KEY_SIZE: usize = 20;
const ASSETS_STORAGE_KEY_SIZE: usize =
    ASSETS_STORAGE_MAP_PREFIX_SIZE + BLAKE_TWO_128_CONCAT_U32_STORAGE_KEY_SIZE;
const ASSETS_ASSET_STORAGE_PREFIX: [u8; ASSETS_STORAGE_MAP_PREFIX_SIZE] = [
    0x68, 0x2a, 0x59, 0xd5, 0x1a, 0xb9, 0xe4, 0x8a, 0x8c, 0x8c, 0xc4, 0x18,
    0xff, 0x97, 0x08, 0xd2, 0xd3, 0x43, 0x71, 0xa1, 0x93, 0xa7, 0x51, 0xee,
    0xa5, 0x88, 0x3e, 0x95, 0x53, 0x45, 0x7b, 0x2e,
];

#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    #[derive(Clone)]
    struct CxxAssetsList {
        identifiers: Vec<u32>,
    }

    #[derive(Clone)]
    struct CxxAssetMetadata {
        deposit: [u8; 16],
        name: Vec<u8>,
        symbol: Vec<u8>,
        decimals: u8,
        is_frozen: bool,
    }

    extern "Rust" {
        type CxxAssetsListResult;

        fn parse_assets_list_from_storage_keys(
            storage_keys: &str,
        ) -> Box<CxxAssetsListResult>;
        fn is_ok(self: &CxxAssetsListResult) -> bool;
        fn error_message(self: &CxxAssetsListResult) -> String;
        fn unwrap(self: &mut CxxAssetsListResult) -> Box<CxxAssetsList>;

        type CxxAssetMetadataResult;

        fn parse_asset_metadata_from_scale(
            metadata_bytes: &[u8],
        ) -> Box<CxxAssetMetadataResult>;
        fn is_ok(self: &CxxAssetMetadataResult) -> bool;
        fn error_message(self: &CxxAssetMetadataResult) -> String;
        fn unwrap(self: &mut CxxAssetMetadataResult) -> Box<CxxAssetMetadata>;

    }
}

use self::ffi::{CxxAssetMetadata, CxxAssetsList};

crate::impl_result!(CxxAssetsList, CxxAssetsListResult);
crate::impl_result!(CxxAssetMetadata, CxxAssetMetadataResult);

fn hex_value(byte: u8) -> Option<u8> {
    match byte {
        b'0'..=b'9' => Some(byte - b'0'),
        b'a'..=b'f' => Some(byte - b'a' + 10),
        b'A'..=b'F' => Some(byte - b'A' + 10),
        _ => None,
    }
}

fn decode_prefixed_hex(input: &str) -> Result<Vec<u8>, Error> {
    let hex = input.strip_prefix("0x").ok_or(Error::InvalidMetadata)?;
    if hex.len() % 2 != 0 {
        return Err(Error::InvalidLength);
    }

    hex.as_bytes()
        .chunks_exact(2)
        .map(|chunk| {
            let high = hex_value(chunk[0]).ok_or(Error::InvalidMetadata)?;
            let low = hex_value(chunk[1]).ok_or(Error::InvalidMetadata)?;
            Ok((high << 4) | low)
        })
        .collect()
}

fn parse_asset_id_from_storage_key(storage_key: &str) -> Result<u32, Error> {
    let bytes = decode_prefixed_hex(storage_key)?;
    if bytes.len() != ASSETS_STORAGE_KEY_SIZE {
        return Err(Error::InvalidLength);
    }
    if bytes[..ASSETS_STORAGE_MAP_PREFIX_SIZE] != ASSETS_ASSET_STORAGE_PREFIX {
        return Err(Error::InvalidMetadata);
    }

    let id_bytes: [u8; 4] = bytes[ASSETS_STORAGE_KEY_SIZE - 4..]
        .try_into()
        .map_err(|_| Error::InvalidLength)?;
    Ok(u32::from_le_bytes(id_bytes))
}

fn parse_assets_list(storage_keys: &str) -> Result<CxxAssetsList, Error> {
    let mut identifiers = Vec::new();
    for storage_key in storage_keys.lines().filter(|key| !key.is_empty()) {
        identifiers.push(parse_asset_id_from_storage_key(storage_key)?);
    }
    Ok(CxxAssetsList { identifiers })
}

fn parse_assets_list_from_storage_keys(storage_keys: &str) -> Box<CxxAssetsListResult> {
    Box::new(CxxAssetsListResult(parse_assets_list(storage_keys)))
}

fn parse_asset_metadata(metadata_bytes: &[u8]) -> Result<CxxAssetMetadata, Error> {
    let mut input = metadata_bytes;
    let deposit: u128 = decode_scale(&mut input)?;
    let name: Vec<u8> = decode_scale(&mut input)?;
    let symbol: Vec<u8> = decode_scale(&mut input)?;
    let decimals: u8 = decode_scale(&mut input)?;
    let is_frozen: bool = decode_scale(&mut input)?;
    if !input.is_empty() {
        return Err(Error::InvalidLength);
    }

    Ok(CxxAssetMetadata {
        deposit: deposit.to_le_bytes(),
        name,
        symbol,
        decimals,
        is_frozen,
    })
}

fn parse_asset_metadata_from_scale(metadata_bytes: &[u8]) -> Box<CxxAssetMetadataResult> {
    Box::new(CxxAssetMetadataResult(parse_asset_metadata(metadata_bytes)))
}
