// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use crate::{CxxPolkadotChainMetadata, CxxPolkadotChainMetadataResult, Error};
use parity_scale_codec::{Compact, Decode};
use std::collections::HashMap;

fn decode_scale<T: Decode>(input: &mut &[u8]) -> Result<T, Error> {
    T::decode(input).map_err(|_| Error::InvalidScale)
}

const MAX_VEC_LEN: usize = 10_000;

fn decode_vec_len(input: &mut &[u8]) -> Result<usize, Error> {
    let len: Compact<u32> = decode_scale(input)?;
    let len = usize::try_from(len.0).map_err(|_| Error::InvalidLength)?;
    if len > MAX_VEC_LEN {
        return Err(Error::InvalidLength);
    }
    Ok(len)
}

fn decode_type_id(input: &mut &[u8]) -> Result<u32, Error> {
    decode_scale::<Compact<u32>>(input).map(|v| v.0)
}

fn decode_vec<T>(
    input: &mut &[u8],
    mut parse_elem: impl FnMut(&mut &[u8]) -> Result<T, Error>,
) -> Result<Vec<T>, Error> {
    let len = decode_vec_len(input)?;
    let mut out = Vec::with_capacity(len);
    for _ in 0..len {
        out.push(parse_elem(input)?);
    }
    Ok(out)
}

fn decode_option<T>(
    input: &mut &[u8],
    parse_some: impl FnOnce(&mut &[u8]) -> Result<T, Error>,
) -> Result<Option<T>, Error> {
    match decode_scale::<u8>(input)? {
        0 => Ok(None),
        1 => Ok(Some(parse_some(input)?)),
        _ => Err(Error::InvalidMetadata),
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
        let id: u32 = decode_scale::<Compact<u32>>(input)?.0;
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
    constants: Vec<(String, Vec<u8>)>,
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
            let saved = *input;
            // Most runtimes (including v14 Polkadot) use Vec<StorageHasher>.
            if decode_vec(input, decode_scale::<u8>).is_ok() {
                let _: u32 = decode_type_id(input)?;
                let _: u32 = decode_type_id(input)?;
                return Ok(());
            }
            // Fallback: v14 spec defines a single StorageHasher byte.
            *input = saved;
            let _: u8 = decode_scale(input)?;
            let _: u32 = decode_type_id(input)?;
            let _: u32 = decode_type_id(input)?;
            Ok(())
        }
        _ => Err(Error::InvalidMetadata),
    }
}

fn parse_storage_entry(input: &mut &[u8]) -> Result<(), Error> {
    let _: String = decode_scale(input)?;
    let _: u8 = decode_scale(input)?; // modifier
    parse_storage_entry_type(input)?;
    let _: Vec<u8> = decode_scale(input)?; // default
    let _: Vec<String> = decode_vec(input, decode_scale::<String>)?; // docs
    Ok(())
}

fn parse_pallet(input: &mut &[u8], has_pallet_docs: bool) -> Result<PalletInfo, Error> {
    let name: String = decode_scale(input)?;

    // storage: Option<PalletStorageMetadata>
    decode_option(input, |input| {
        let _: String = decode_scale(input)?; // prefix
        decode_vec(input, parse_storage_entry)
    })?;

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

    Ok(PalletInfo { name, index, calls_type_id, constants })
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
// across runtimes (u8, u16, u32, or Compact<u32>). Try fixed-width first,
// then Compact<u32>, accepting whichever consumes the entire buffer.
// Compact<u32> must come after u16/u32 to avoid misinterpreting multi-byte
// fixed-width encodings as compact (e.g. u16(42)=[0x2a,0x00] has mode bits
// 10 which compact decodes as value 10, not 42).
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
    if let Ok(Compact(v)) = Compact::<u32>::decode(&mut input) {
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
///   - `Assets` pallet index and transfer call indexes when the pallet is
///     present
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

    let mut has_assets_pallet = false;
    let mut assets_pallet_index = 0;
    let mut assets_transfer_all_call_index = 0;
    let mut assets_transfer_keep_alive_call_index = 0;
    if let Some(assets_pallet) = pallets.iter().find(|p| normalize_ident(&p.name) == "assets") {
        has_assets_pallet = true;
        assets_pallet_index = assets_pallet.index;
        assets_transfer_all_call_index =
            get_call_index(&portable_registry, assets_pallet, "transferall")?;
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
        assets_transfer_all_call_index,
        assets_transfer_keep_alive_call_index,
        has_assets_pallet,
        ss58_prefix,
        spec_version,
        asset_tx_payment,
    })
}

pub(super) fn parse_chain_metadata_from_scale(
    metadata_bytes: &[u8],
) -> Box<CxxPolkadotChainMetadataResult> {
    Box::new(CxxPolkadotChainMetadataResult(parse_chain_metadata_fields(metadata_bytes)))
}
