// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use crate::CxxPolkadotChainMetadata;
use crate::Error;
use crate::polkadot_scale_utils::{
    decode_option, decode_scale, decode_type_id, decode_vec, decode_vec_len,
};
use parity_scale_codec::Decode;
use std::collections::HashMap;

#[derive(Clone, Copy)]
struct CxxPolkadotChainMetadataFields {
    system_pallet_index: u8,
    balances_pallet_index: u8,
    transaction_payment_pallet_index: u8,
    transfer_allow_death_call_index: u8,
    transfer_keep_alive_call_index: u8,
    transfer_all_call_index: u8,
    assets_pallet_index: u8,
    assets_asset_storage_index: u8,
    assets_metadata_storage_index: u8,
    assets_account_storage_index: u8,
    assets_transfer_keep_alive_call_index: u8,
    has_assets_pallet: bool,
    ss58_prefix: u16,
    spec_version: u32,
    asset_tx_payment: bool,
}

crate::impl_result!(CxxPolkadotChainMetadataFields, CxxPolkadotChainMetadataFieldsResult);

#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    extern "Rust" {
        type CxxPolkadotChainMetadataFieldsResult;

        fn parse_chain_metadata_from_scale(
            metadata_bytes: &[u8],
        ) -> Box<CxxPolkadotChainMetadataFieldsResult>;
        fn is_ok(self: &CxxPolkadotChainMetadataFieldsResult) -> bool;
        fn error_message(self: &CxxPolkadotChainMetadataFieldsResult) -> String;
        fn unwrap(
            self: &mut CxxPolkadotChainMetadataFieldsResult,
        ) -> Box<CxxPolkadotChainMetadataFields>;

        type CxxPolkadotChainMetadataFields;
        fn system_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn balances_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn transaction_payment_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn transfer_allow_death_call_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn transfer_keep_alive_call_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn transfer_all_call_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn assets_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn assets_asset_storage_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn assets_metadata_storage_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn assets_account_storage_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn assets_transfer_keep_alive_call_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn has_assets_pallet(self: &CxxPolkadotChainMetadataFields) -> bool;
        fn ss58_prefix(self: &CxxPolkadotChainMetadataFields) -> u16;
        fn spec_version(self: &CxxPolkadotChainMetadataFields) -> u32;
        fn asset_tx_payment(self: &CxxPolkadotChainMetadataFields) -> bool;
    }
}

impl CxxPolkadotChainMetadataFields {
    fn system_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.system_pallet_index
    }

    fn balances_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.balances_pallet_index
    }

    fn transaction_payment_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.transaction_payment_pallet_index
    }

    fn transfer_allow_death_call_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.transfer_allow_death_call_index
    }

    fn transfer_keep_alive_call_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.transfer_keep_alive_call_index
    }

    fn transfer_all_call_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.transfer_all_call_index
    }

    fn assets_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.assets_pallet_index
    }

    fn assets_asset_storage_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.assets_asset_storage_index
    }

    fn assets_metadata_storage_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.assets_metadata_storage_index
    }

    fn assets_account_storage_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.assets_account_storage_index
    }

    fn assets_transfer_keep_alive_call_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.assets_transfer_keep_alive_call_index
    }

    fn has_assets_pallet(self: &CxxPolkadotChainMetadataFields) -> bool {
        self.has_assets_pallet
    }

    fn ss58_prefix(self: &CxxPolkadotChainMetadataFields) -> u16 {
        self.ss58_prefix
    }

    fn spec_version(self: &CxxPolkadotChainMetadataFields) -> u32 {
        self.spec_version
    }

    fn asset_tx_payment(self: &CxxPolkadotChainMetadataFields) -> bool {
        self.asset_tx_payment
    }
}

fn parse_field(input: &mut &[u8]) -> Result<(), Error> {
    // Option<String>
    let _: Option<String> = decode_scale(input)?;
    // type id
    let _: u32 = decode_type_id(input)?;
    // Option<String>
    let _: Option<String> = decode_scale(input)?;
    // docs: Vec<String>
    let _: Vec<String> = decode_vec(input, decode_scale::<String>)?;
    Ok(())
}

#[derive(Clone)]
struct VariantInfo {
    name: String,
    index: u8,
}

fn parse_variants(input: &mut &[u8]) -> Result<Vec<VariantInfo>, Error> {
    decode_vec(input, |input| {
        let name: String = decode_scale(input)?;
        // fields
        let _ = decode_vec(input, parse_field)?;
        let index: u8 = decode_scale(input)?;
        // docs
        let _: Vec<String> = decode_vec(input, decode_scale::<String>)?;
        Ok(VariantInfo { name, index })
    })
}

// https://github.com/paritytech/scale-info/blob/7629a7fa4023674f015aa47730a65367cf56d2b7/src/ty/mod.rs#L275-L300
fn parse_type_def(input: &mut &[u8]) -> Result<Option<Vec<VariantInfo>>, Error> {
    let type_def_tag = decode_scale::<u8>(input)?;
    match type_def_tag {
        // Composite(Vec<Field>)
        0 => {
            let _ = decode_vec(input, parse_field)?;
            Ok(None)
        }
        // Variant(Vec<Variant>)
        1 => parse_variants(input).map(Some),
        // Sequence(type_id)
        2 => {
            let _: u32 = decode_type_id(input)?;
            Ok(None)
        }
        // Array(len, type_id)
        3 => {
            let _: u32 = decode_scale(input)?;
            let _: u32 = decode_type_id(input)?;
            Ok(None)
        }
        // Tuple(Vec<type_id>)
        4 => {
            let _ = decode_vec(input, decode_type_id)?;
            Ok(None)
        }
        // Primitive(primitive discriminant)
        5 => {
            let _: u8 = decode_scale(input)?;
            Ok(None)
        }
        // Compact(type_id)
        6 => {
            let _: u32 = decode_type_id(input)?;
            Ok(None)
        }
        // BitSequence(store_type, order_type)
        7 => {
            let _: u32 = decode_type_id(input)?;
            let _: u32 = decode_type_id(input)?;
            Ok(None)
        }
        _ => Err(Error::InvalidMetadata),
    }
}

fn parse_type(input: &mut &[u8]) -> Result<Option<Vec<VariantInfo>>, Error> {
    // path: Vec<String>
    let _: Vec<String> = decode_vec(input, decode_scale::<String>)?;
    // type params: Vec<(name: String, type_id: Option<Compact<u32>>)>
    let _ = decode_vec(input, |input| {
        let _: String = decode_scale(input)?;
        let _: Option<u32> = decode_option(input, decode_type_id)?;
        Ok(())
    })?;
    let variants = parse_type_def(input)?;
    // docs
    let _: Vec<String> = decode_vec(input, decode_scale::<String>)?;
    Ok(variants)
}

// PortableRegistry is the runtime metadata type table (scale-info), keyed by
// type id. Pallets reference call/event/error types by id; we keep only
// variant-bearing entries so we can resolve `Balances.calls` and locate the
// `transfer_allow_death` call index.
fn parse_portable_registry(input: &mut &[u8]) -> Result<HashMap<u32, Vec<VariantInfo>>, Error> {
    let mut by_id = HashMap::new();
    let types_len = decode_vec_len(input)?;
    for _ in 0..types_len {
        let id = decode_type_id(input)?;
        if let Some(variants) = parse_type(input)? {
            by_id.insert(id, variants);
        }
    }

    Ok(by_id)
}

#[derive(Clone)]
struct PalletInfo {
    name: String,
    index: u8,
    calls_type_id: Option<u32>,
    storage_entries: Vec<StorageEntryInfo>,
    constants: Vec<(String, Vec<u8>)>,
}

#[derive(Clone)]
struct StorageEntryInfo {
    name: String,
    index: u8,
}

// https://github.com/paritytech/frame-metadata/blob/285e5bdc5e9a97b1fcd3dcc94c3666d8a61226a8/frame-metadata/src/v14.rs#L281-L293
fn parse_storage_entry_type(input: &mut &[u8]) -> Result<(), Error> {
    let tag = decode_scale::<u8>(input)?;
    match tag {
        // Plain(type_id)
        0 => {
            let _: u32 = decode_type_id(input)?;
            Ok(())
        }
        // Map { hashers, key, value }
        1 => {
            let _ = decode_vec(input, |input| {
                let _: u8 = decode_scale(input)?;
                Ok(())
            })?;
            let _: u32 = decode_type_id(input)?;
            let _: u32 = decode_type_id(input)?;
            Ok(())
        }
        _ => Err(Error::InvalidMetadata),
    }
}

fn parse_storage_entry(input: &mut &[u8]) -> Result<String, Error> {
    let name: String = decode_scale(input)?;
    let _: u8 = decode_scale(input)?; // modifier
    parse_storage_entry_type(input)?;
    let _: Vec<u8> = decode_scale(input)?; // default
    let _: Vec<String> = decode_vec(input, decode_scale::<String>)?; // docs
    Ok(name)
}

fn parse_pallet(input: &mut &[u8], has_pallet_docs: bool) -> Result<PalletInfo, Error> {
    let name: String = decode_scale(input)?;

    // storage: Option<PalletStorageMetadata>
    let storage_entries = decode_option(input, |input| {
        let _: String = decode_scale(input)?; // prefix
        decode_vec(input, parse_storage_entry)
    })?
    .unwrap_or_default();
    let storage_entries = storage_entries
        .into_iter()
        .enumerate()
        .map(|(index, name)| {
            let index = u8::try_from(index).map_err(|_| Error::InvalidLength)?;
            Ok(StorageEntryInfo { name, index })
        })
        .collect::<Result<Vec<_>, Error>>()?;

    // calls: Option<PalletCallMetadata { ty: u32 }>
    let calls_type_id = decode_option(input, decode_type_id)?;

    // event: Option<PalletEventMetadata { ty: u32 }>
    let _event_ty = decode_option(input, decode_type_id)?;

    // constants: Vec<PalletConstantMetadata>
    let constants = decode_vec(input, |input| {
        let constant_name: String = decode_scale(input)?;
        let _: u32 = decode_type_id(input)?; // type id
        let value: Vec<u8> = decode_scale(input)?;
        let _: Vec<String> = decode_vec(input, decode_scale::<String>)?;
        Ok((constant_name, value))
    })?;

    // error: Option<PalletErrorMetadata { ty: u32 }>
    let _error_ty = decode_option(input, decode_type_id)?;

    let index: u8 = decode_scale(input)?;
    // RuntimeMetadataV15 includes pallet-level docs while V14 does not.
    if has_pallet_docs {
        let _: Vec<String> = decode_vec(input, decode_scale::<String>)?;
    }

    Ok(PalletInfo { name, index, calls_type_id, storage_entries, constants })
}

fn parse_pallets(input: &mut &[u8], has_pallet_docs: bool) -> Result<Vec<PalletInfo>, Error> {
    let len = decode_vec_len(input)?;
    let mut pallets = Vec::with_capacity(len);
    for _ in 0..len {
        let pallet = parse_pallet(input, has_pallet_docs)?;
        pallets.push(pallet);
    }
    Ok(pallets)
}

// SS58Prefix constant is SCALE-encoded but the concrete integer width varies
// across runtimes (u8, u16, or u32). Try each width and accept the first that
// consumes the entire buffer.
fn decode_ss58_prefix(raw: &[u8]) -> Option<u16> {
    let mut input = raw;
    if let Ok(v) = u16::decode(&mut input) {
        if input.is_empty() {
            return Some(v);
        }
    }
    let mut input = raw;
    if let Ok(v) = u32::decode(&mut input) {
        if input.is_empty() {
            return u16::try_from(v).ok();
        }
    }
    let mut input = raw;
    if let Ok(v) = u8::decode(&mut input) {
        if input.is_empty() {
            return Some(v as u16);
        }
    }
    None
}

fn normalize_ident(name: &str) -> String {
    name.chars().filter(|c| *c != '_').flat_map(|c| c.to_lowercase()).collect()
}

fn decode_runtime_spec_version(raw: &[u8]) -> Option<u32> {
    // RuntimeVersion starts with: spec_name, impl_name, authoring_version,
    // spec_version.
    let mut input = raw;
    let _: String = Decode::decode(&mut input).ok()?;
    let _: String = Decode::decode(&mut input).ok()?;
    let _: u32 = Decode::decode(&mut input).ok()?;
    Decode::decode(&mut input).ok()
}

const RUNTIME_METADATA_PREFIX_MAGIC: u32 = 0x6174_656d;

fn get_call_index(
    portable_registry: &HashMap<u32, Vec<VariantInfo>>,
    pallet: &PalletInfo,
    call_name: &str,
) -> Result<u8, Error> {
    portable_registry
        .get(&pallet.calls_type_id.ok_or(Error::InvalidMetadata)?)
        .and_then(|variants| variants.iter().find(|v| normalize_ident(&v.name) == call_name))
        .map(|v| v.index)
        .ok_or(Error::InvalidMetadata)
}

fn get_storage_entry_index(pallet: &PalletInfo, storage_entry_name: &str) -> Result<u8, Error> {
    pallet
        .storage_entries
        .iter()
        .find(|entry| normalize_ident(&entry.name) == storage_entry_name)
        .map(|entry| entry.index)
        .ok_or(Error::InvalidMetadata)
}

// Returns true if "ChargeAssetTxPayment" is present in the signed extensions,
// indicating the signature payload must include an asset_id field.
// https://docs.rs/frame-metadata/latest/frame_metadata/v15/struct.SignedExtensionMetadata.html
// https://github.com/paritytech/polkadot-sdk/blob/c3ecf63034924511d6b92e3533c23c15765f16b9/substrate/frame/transaction-payment/asset-conversion-tx-payment/src/lib.rs#L165-L182
fn parse_signed_extensions(input: &mut &[u8]) -> Result<bool, Error> {
    let mut found = false;
    decode_vec(input, |input| {
        let identifier: String = decode_scale(input)?;
        if normalize_ident(&identifier) == "chargeassettxpayment" {
            found = true;
        }
        let _: u32 = decode_type_id(input)?; // ty
        let _: u32 = decode_type_id(input)?; // additional_signed
        Ok(())
    })?;

    Ok(found)
}

fn parse_extrinsic_metadata(input: &mut &[u8], version: u8) -> Result<bool, Error> {
    match version {
        // V14: https://docs.rs/frame-metadata/latest/frame_metadata/v14/struct.ExtrinsicMetadata.html
        //   { ty: Compact<u32>, version: u8, signed_extensions: Vec<...> }
        14 => {
            let _: u32 = decode_type_id(input)?; // ty
            let _: u8 = decode_scale(input)?; // version
        }
        // V15: https://docs.rs/frame-metadata/latest/frame_metadata/v15/struct.ExtrinsicMetadata.html
        //   { version: u8, address_ty, call_ty, signature_ty, extra_ty,
        //     signed_extensions: Vec<...> }
        15 => {
            let _: u8 = decode_scale(input)?; // version
            let _: u32 = decode_type_id(input)?; // address_ty
            let _: u32 = decode_type_id(input)?; // call_ty
            let _: u32 = decode_type_id(input)?; // signature_ty
            let _: u32 = decode_type_id(input)?; // extra_ty
        }
        _ => return Err(Error::InvalidMetadata),
    }

    parse_signed_extensions(input)
}

/// Parses SCALE-encoded `state_getMetadata` bytes (RuntimeMetadataPrefixed).
///
/// Structure expected by this parser:
/// - u32 magic: little-endian "meta" (`0x6174_656d`)
/// - u8 runtime metadata version (v14 or v15)
/// - PortableRegistry (type table)
/// - Vec<PalletMetadata>
///   - `name: String`
///   - `storage: Option<PalletStorageMetadata>`
///   - `calls: Option<PalletCallMetadata { ty: Compact<u32> }>`
///   - `event: Option<PalletEventMetadata { ty: Compact<u32> }>`
///   - `constants: Vec<PalletConstantMetadata { name, ty, value, docs }>`
///   - `error: Option<PalletErrorMetadata { ty: Compact<u32> }>`
///   - `index: u8`
///   - `docs: Vec<String>` (v15 only)
/// - ExtrinsicMetadata (signed extensions checked for `ChargeAssetTxPayment`)
/// - Pallet/constants/type references needed to extract:
///   - `Balances` pallet index
///   - `transfer_allow_death` call index
///   - `System.SS58Prefix`
///   - `System.Version.spec_version`
///   - `Assets` pallet index, storage entry indexes, and `transfer_keep_alive`
///     call index when the pallet is present
///   - whether `ChargeAssetTxPayment` is a signed extension
///
/// References:
/// - JSON-RPC method: https://github.com/w3f/PSPs/blob/b6d570173146e7a012cf43d270177e02ed886e2e/PSPs/drafts/psp-6.md#1119-state_getmetadata
/// - Runtime metadata format (Rust): https://docs.rs/frame-metadata/latest/frame_metadata/
fn parse_chain_metadata_fields(bytes: &[u8]) -> Result<CxxPolkadotChainMetadata, Error> {
    let mut input = bytes;
    // RuntimeMetadataPrefixed starts with little-endian "meta" magic.
    let magic: u32 = decode_scale(&mut input)?;
    if magic != RUNTIME_METADATA_PREFIX_MAGIC {
        return Err(Error::InvalidMetadata);
    }

    // RuntimeMetadata enum variant discriminant.
    let version: u8 = decode_scale(&mut input)?;
    if version != 14 && version != 15 {
        return Err(Error::InvalidMetadata);
    }

    let portable_registry = parse_portable_registry(&mut input)?;

    let pallets = parse_pallets(&mut input, /* has_pallet_docs= */ version >= 15)?;

    let asset_tx_payment = parse_extrinsic_metadata(&mut input, version)?;

    let balances_pallet = pallets
        .iter()
        .find(|p| normalize_ident(&p.name) == "balances")
        .ok_or(Error::InvalidMetadata)?;
    let balances_pallet_index = balances_pallet.index;

    let transfer_allow_death_call_index =
        get_call_index(&portable_registry, balances_pallet, "transferallowdeath")?;

    let transfer_keep_alive_call_index =
        get_call_index(&portable_registry, balances_pallet, "transferkeepalive")?;

    let transfer_all_call_index =
        get_call_index(&portable_registry, balances_pallet, "transferall")?;

    let mut assets_pallet_index = 0;
    let mut assets_asset_storage_index = 0;
    let mut assets_metadata_storage_index = 0;
    let mut assets_account_storage_index = 0;
    let mut assets_transfer_keep_alive_call_index = 0;
    let mut has_assets_pallet = false;
    if let Some(assets_pallet) = pallets.iter().find(|p| normalize_ident(&p.name) == "assets") {
        has_assets_pallet = true;
        assets_pallet_index = assets_pallet.index;
        assets_asset_storage_index = get_storage_entry_index(assets_pallet, "asset")?;
        assets_metadata_storage_index = get_storage_entry_index(assets_pallet, "metadata")?;
        assets_account_storage_index = get_storage_entry_index(assets_pallet, "account")?;
        assets_transfer_keep_alive_call_index =
            get_call_index(&portable_registry, assets_pallet, "transferkeepalive")?;
    }

    let system_pallet = pallets
        .iter()
        .find(|p| normalize_ident(&p.name) == "system")
        .ok_or(Error::InvalidMetadata)?;

    let system_pallet_index = system_pallet.index;

    let ss58_prefix = system_pallet
        .constants
        .iter()
        .find(|(name, _)| normalize_ident(name) == "ss58prefix")
        .and_then(|(_, value)| decode_ss58_prefix(value))
        .ok_or(Error::InvalidMetadata)?;

    let spec_version = system_pallet
        .constants
        .iter()
        .find(|(name, _)| normalize_ident(name) == "version")
        .and_then(|(_, value)| decode_runtime_spec_version(value))
        .ok_or(Error::InvalidMetadata)?;

    let transaction_payment_pallet_index = pallets
        .iter()
        .find(|p| normalize_ident(&p.name) == "transactionpayment")
        .map(|p| p.index)
        .ok_or(Error::InvalidMetadata)?;

    Ok(CxxPolkadotChainMetadata {
        system_pallet_index,
        balances_pallet_index,
        transaction_payment_pallet_index,
        transfer_allow_death_call_index,
        transfer_keep_alive_call_index,
        transfer_all_call_index,
        assets_pallet_index,
        assets_asset_storage_index,
        assets_metadata_storage_index,
        assets_account_storage_index,
        assets_transfer_keep_alive_call_index,
        has_assets_pallet,
        ss58_prefix,
        spec_version,
        asset_tx_payment,
    })
}

fn parse_chain_metadata_from_scale(
    metadata_bytes: &[u8],
) -> Box<CxxPolkadotChainMetadataFieldsResult> {
    let fields = parse_chain_metadata_fields(metadata_bytes).map(|metadata| {
        CxxPolkadotChainMetadataFields {
            system_pallet_index: metadata.system_pallet_index,
            balances_pallet_index: metadata.balances_pallet_index,
            transaction_payment_pallet_index: metadata.transaction_payment_pallet_index,
            transfer_allow_death_call_index: metadata.transfer_allow_death_call_index,
            transfer_keep_alive_call_index: metadata.transfer_keep_alive_call_index,
            transfer_all_call_index: metadata.transfer_all_call_index,
            assets_pallet_index: metadata.assets_pallet_index,
            assets_asset_storage_index: metadata.assets_asset_storage_index,
            assets_metadata_storage_index: metadata.assets_metadata_storage_index,
            assets_account_storage_index: metadata.assets_account_storage_index,
            assets_transfer_keep_alive_call_index: metadata.assets_transfer_keep_alive_call_index,
            has_assets_pallet: metadata.has_assets_pallet,
            ss58_prefix: metadata.ss58_prefix,
            spec_version: metadata.spec_version,
            asset_tx_payment: metadata.asset_tx_payment,
        }
    });
    Box::new(CxxPolkadotChainMetadataFieldsResult(fields))
}
