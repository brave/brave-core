// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use crate::CxxPolkadotChainMetadata;
use crate::Error;
use parity_scale_codec::{Compact, Decode};
use std::collections::HashMap;

#[derive(Clone, Copy)]
struct CxxPolkadotChainMetadataFields {
    balances_pallet_index: u8,
    transfer_allow_death_call_index: u8,
    ss58_prefix: u16,
    spec_version: u32,
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
        fn balances_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn transfer_allow_death_call_index(self: &CxxPolkadotChainMetadataFields) -> u8;
        fn ss58_prefix(self: &CxxPolkadotChainMetadataFields) -> u16;
        fn spec_version(self: &CxxPolkadotChainMetadataFields) -> u32;
    }
}

impl CxxPolkadotChainMetadataFields {
    fn balances_pallet_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.balances_pallet_index
    }

    fn transfer_allow_death_call_index(self: &CxxPolkadotChainMetadataFields) -> u8 {
        self.transfer_allow_death_call_index
    }

    fn ss58_prefix(self: &CxxPolkadotChainMetadataFields) -> u16 {
        self.ss58_prefix
    }

    fn spec_version(self: &CxxPolkadotChainMetadataFields) -> u32 {
        self.spec_version
    }
}

fn decode_scale<T: Decode>(input: &mut &[u8]) -> Result<T, Error> {
    T::decode(input).map_err(|_| Error::InvalidScale)
}

fn decode_vec_len(input: &mut &[u8]) -> Result<usize, Error> {
    let len: Compact<u32> = decode_scale(input)?;
    usize::try_from(len.0).map_err(|_| Error::InvalidLength)
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
    let _ = decode_vec(input, decode_scale::<String>)?;
    // type params
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
    let _storage = decode_option(input, |input| {
        let _: String = decode_scale(input)?; // prefix
        let _ = decode_vec(input, parse_storage_entry)?;
        Ok(())
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
/// - Pallet/constants/type references needed to extract:
///   - `Balances` pallet index
///   - `transfer_allow_death` call index
///   - `System.SS58Prefix`
///   - `System.Version.spec_version`
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

    let balances_pallet = pallets
        .iter()
        .find(|p| normalize_ident(&p.name) == "balances")
        .ok_or(Error::InvalidMetadata)?;
    let balances_pallet_index = balances_pallet.index;
    let calls_type_id = balances_pallet.calls_type_id.ok_or(Error::InvalidMetadata)?;

    let transfer_allow_death_call_index = portable_registry
        .get(&calls_type_id)
        .and_then(|variants| {
            variants.iter().find(|v| normalize_ident(&v.name) == "transferallowdeath")
        })
        .map(|v| v.index)
        .ok_or(Error::InvalidMetadata)?;

    let system_pallet = pallets
        .iter()
        .find(|p| normalize_ident(&p.name) == "system")
        .ok_or(Error::InvalidMetadata)?;

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
    Ok(CxxPolkadotChainMetadata {
        balances_pallet_index,
        transfer_allow_death_call_index,
        ss58_prefix,
        spec_version,
    })
}

fn parse_chain_metadata_from_scale(
    metadata_bytes: &[u8],
) -> Box<CxxPolkadotChainMetadataFieldsResult> {
    let fields = parse_chain_metadata_fields(metadata_bytes).map(|metadata| {
        CxxPolkadotChainMetadataFields {
            balances_pallet_index: metadata.balances_pallet_index,
            transfer_allow_death_call_index: metadata.transfer_allow_death_call_index,
            ss58_prefix: metadata.ss58_prefix,
            spec_version: metadata.spec_version,
        }
    });
    Box::new(CxxPolkadotChainMetadataFieldsResult(fields))
}
