// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

//! Cardano Transaction Decoder
//!
//! This module provides functionality to decode Cardano transactions from CBOR
//! format and apply signatures to create signed transactions. It follows the
//! Cardano transaction structure: [body, witness_set, metadata].
//!
//! The decoder extracts transaction inputs and outputs from the transaction
//! body, while the signature application preserves existing witnesses and adds
//! new ones.

use std::sync::atomic::{AtomicBool, Ordering};

use blake2b_simd::Params as Blake2bParams;
use ciborium::{de::from_reader, Value as CborValue};

// Re-export types for external use
#[allow(unused_imports)]
use crate::ffi::CxxSerializableCoinValue;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableMintToken;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTx;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTxBody;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTxInput;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTxOutput;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTxOutputToken;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTxWitness;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableVkeyWitness;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableWithdrawal;

// Constants definitions come from
// https://github.com/IntersectMBO/cardano-ledger/blob/30d293d7166561d47bccf56b7e1473cdab84714d/eras/conway/impl/cddl/data/conway.cddl
const CARDANO_TX_HASH_SIZE: usize = 32;
const CARDANO_SCRIPT_HASH_SIZE: usize = 28;
const CARDANO_PUBKEY_SIZE: usize = 32;
const CARDANO_SIGNATURE_SIZE: usize = 64;
const TRANSACTION_BODY_INDEX: usize = 0;
const WITNESS_SET_INDEX: usize = 1;
const INPUTS_KEY: u8 = 0;
const OUTPUTS_KEY: u8 = 1;
const FEE_KEY: u8 = 2;
const TTL_KEY: u8 = 3;
const CERTIFICATES_KEY: u8 = 4;
const WITHDRAWALS_KEY: u8 = 5;
const MINT_KEY: u8 = 9;
const VK_WITNESS_KEY: u8 = 0;
const BABBAGE_OUTPUT_ADDRESS_KEY: u8 = 0;
const BABBAGE_OUTPUT_VALUE_KEY: u8 = 1;
const SET_TAG: u64 = 258;

// Global variable to control whether SET_TAG is used (for testing purposes)
static USE_SET_TAG: AtomicBool = AtomicBool::new(true);
pub fn use_set_tag_for_testing(enable: bool) {
    USE_SET_TAG.store(enable, Ordering::Relaxed);
}

#[macro_export]
macro_rules! impl_result {
    ($r:ident) => {
        impl $r {
            fn is_ok(self: &$r) -> bool {
                self.0.is_ok()
            }
        }
    };
}

fn unwrap_or_panic<T>(result: &Result<T, ()>) -> &T {
    match result {
        Ok(v) => v,
        Err(()) => panic!("Can't unwrap result. Must check is_ok() first"),
    }
}

fn decode_u32(value: &CborValue) -> Result<u32, ()> {
    match value {
        CborValue::Integer(i) => (*i).try_into().map_err(|_| ()),
        _ => Err(()),
    }
}

fn decode_u64(value: &CborValue) -> Result<u64, ()> {
    match value {
        CborValue::Integer(i) => (*i).try_into().map_err(|_| ()),
        _ => Err(()),
    }
}

fn decode_i64(value: &CborValue) -> Result<i64, ()> {
    match value {
        CborValue::Integer(i) => (*i).try_into().map_err(|_| ()),
        _ => Err(()),
    }
}

/// nonzero_int64 = negative_int64 / positive_int64 (zero is invalid).
/// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L473
fn decode_nonzero_i64(value: &CborValue) -> Result<i64, ()> {
    let amount = decode_i64(value)?;
    if amount == 0 {
        return Err(());
    }
    Ok(amount)
}

fn decode_bytes_array<const N: usize>(value: &CborValue) -> Result<[u8; N], ()> {
    match value {
        CborValue::Bytes(bytes) => bytes.clone().try_into().map_err(|_| ()),
        _ => Err(()),
    }
}

fn decode_bytes_vector(value: &CborValue) -> Result<Vec<u8>, ()> {
    match value {
        CborValue::Bytes(bytes) => Ok(bytes.clone()),
        _ => Err(()),
    }
}

fn decode_array(value: &CborValue) -> Result<&Vec<CborValue>, ()> {
    match value {
        CborValue::Array(arr) => Ok(arr),
        _ => Err(()),
    }
}

// Cardano CDDL for nonempty_set supports both bare arrays and tagged sets as
// `#6.258([...])` (CBOR tag 258).
// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L544
fn decode_nonempty_set(value: &CborValue) -> Result<&Vec<CborValue>, ()> {
    let arr = match value {
        CborValue::Array(arr) => arr,
        CborValue::Tag(SET_TAG, tagged) => {
            let CborValue::Array(arr) = tagged.as_ref() else {
                return Err(());
            };
            arr
        }
        _ => return Err(()),
    };
    if arr.is_empty() {
        return Err(());
    }
    Ok(arr)
}

/// Helper function to find a value in a CBOR map by integer key
fn find_map_value(map: &[(CborValue, CborValue)], key: u8) -> Option<&CborValue> {
    map.iter()
        .find(|(k, _)| {
            matches!(k, CborValue::Integer(i) if *i == ciborium::value::Integer::from(key))
        })
        .map(|(_, v)| v)
}

/// CXX bridge module for FFI with C++
#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    #[derive(Clone)]
    struct CxxSerializableTxInput {
        tx_hash: [u8; 32], // CARDANO_TX_HASH_SIZE
        index: u32,
    }

    #[derive(Clone)]
    struct CxxSerializableTxOutputToken {
        token_id: Vec<u8>,
        amount: u64,
    }

    #[derive(Clone)]
    struct CxxSerializableCoinValue {
        lovelace_amount: u64,
        tokens: Vec<CxxSerializableTxOutputToken>,
    }

    #[derive(Clone)]
    struct CxxSerializableTxOutput {
        addr: Vec<u8>,
        coin_value: CxxSerializableCoinValue,
    }

    #[derive(Clone)]
    struct CxxSerializableWithdrawal {
        reward_account: Vec<u8>,
        coin: u64,
    }

    #[derive(Clone)]
    struct CxxSerializableMintToken {
        token_id: Vec<u8>,
        amount: i64,
    }

    #[derive(Clone)]
    struct CxxSerializableTxBody {
        inputs: Vec<CxxSerializableTxInput>,
        outputs: Vec<CxxSerializableTxOutput>,
        fee: u64,
        has_ttl: bool,
        ttl: u64,
        withdrawals: Vec<CxxSerializableWithdrawal>,
        mint: Vec<CxxSerializableMintToken>,
    }

    #[derive(Clone)]
    struct CxxSerializableVkeyWitness {
        pubkey: [u8; 32],    // CARDANO_PUBKEY_SIZE
        signature: [u8; 64], // CARDANO_SIGNATURE_SIZE
    }

    #[derive(Clone)]
    struct CxxSerializableTxWitness {
        vkey_witness_set: Vec<CxxSerializableVkeyWitness>,
    }

    #[derive(Clone)]
    struct CxxSerializableTx {
        body: CxxSerializableTxBody,
        witness: CxxSerializableTxWitness,
    }

    extern "Rust" {
        type CxxEncodedCardanoTransactionResult;
        type CxxEncodedCardanoTransactionOutputResult;
        type CxxEncodedCardanoUtxoResult;
        type CxxEncodedCardanoCoinValueResult;
        type CxxDecodedCardanoCoinValueResult;
        type CxxDecodedCardanoTransactionResult;
        type CxxEncodedWitnessResult;

        fn use_set_tag_for_testing(enable: bool);

        fn encode_cardano_transaction(
            tx: &CxxSerializableTx,
        ) -> Box<CxxEncodedCardanoTransactionResult>;
        fn encode_cardano_coin_value(
            value: &CxxSerializableCoinValue,
        ) -> Box<CxxEncodedCardanoCoinValueResult>;
        fn decode_cardano_coin_value(value_bytes: &[u8]) -> Box<CxxDecodedCardanoCoinValueResult>;
        fn encode_cardano_transaction_output(
            output: &CxxSerializableTxOutput,
        ) -> Box<CxxEncodedCardanoTransactionOutputResult>;
        fn encode_cardano_utxo(
            input: &CxxSerializableTxInput,
            output: &CxxSerializableTxOutput,
        ) -> Box<CxxEncodedCardanoUtxoResult>;
        fn get_cardano_transaction_hash(tx: &CxxSerializableTx) -> [u8; 32]; // CARDANO_TX_HASH_SIZE
        fn decode_cardano_transaction(bytes: &[u8]) -> Box<CxxDecodedCardanoTransactionResult>;
        fn encode_cardano_witness(
            witnesses: CxxSerializableTxWitness,
        ) -> Box<CxxEncodedWitnessResult>;

        fn is_ok(self: &CxxEncodedCardanoTransactionResult) -> bool;
        fn bytes(self: &CxxEncodedCardanoTransactionResult) -> Vec<u8>;

        fn is_ok(self: &CxxEncodedCardanoCoinValueResult) -> bool;
        fn bytes(self: &CxxEncodedCardanoCoinValueResult) -> Vec<u8>;

        fn is_ok(self: &CxxDecodedCardanoCoinValueResult) -> bool;
        fn value(self: &CxxDecodedCardanoCoinValueResult) -> CxxSerializableCoinValue;

        fn is_ok(self: &CxxEncodedCardanoTransactionOutputResult) -> bool;
        fn bytes(self: &CxxEncodedCardanoTransactionOutputResult) -> Vec<u8>;

        fn is_ok(self: &CxxEncodedCardanoUtxoResult) -> bool;
        fn bytes(self: &CxxEncodedCardanoUtxoResult) -> Vec<u8>;

        fn is_ok(self: &CxxDecodedCardanoTransactionResult) -> bool;
        fn tx(self: &CxxDecodedCardanoTransactionResult) -> CxxSerializableTx;
        fn raw_body(self: &CxxDecodedCardanoTransactionResult) -> Vec<u8>;
        fn raw_tx(self: &CxxDecodedCardanoTransactionResult) -> Vec<u8>;

        fn is_ok(self: &CxxEncodedWitnessResult) -> bool;
        fn bytes(self: &CxxEncodedWitnessResult) -> Vec<u8>;
    }
}

struct CxxEncodedCardanoCoinValue {
    value_bytes: Vec<u8>,
}

struct CxxDecodedCardanoCoinValue {
    value: CxxSerializableCoinValue,
}

struct CxxEncodedCardanoTransaction {
    tx_bytes: Vec<u8>,
}

struct CxxEncodedCardanoTransactionOutput {
    output_bytes: Vec<u8>,
}

struct CxxEncodedCardanoUtxo {
    utxo_bytes: Vec<u8>,
}

struct CxxDecodedCardanoTransaction {
    tx: CxxSerializableTx,
    raw_body: Vec<u8>,
    raw_tx: Vec<u8>,
}

struct CxxEncodedWitness {
    witness_bytes: Vec<u8>,
}

pub struct CxxEncodedCardanoTransactionResult(Result<CxxEncodedCardanoTransaction, ()>);
pub struct CxxEncodedCardanoCoinValueResult(Result<CxxEncodedCardanoCoinValue, ()>);
pub struct CxxDecodedCardanoCoinValueResult(Result<CxxDecodedCardanoCoinValue, ()>);
pub struct CxxEncodedCardanoTransactionOutputResult(Result<CxxEncodedCardanoTransactionOutput, ()>);
pub struct CxxEncodedCardanoUtxoResult(Result<CxxEncodedCardanoUtxo, ()>);
pub struct CxxDecodedCardanoTransactionResult(Result<CxxDecodedCardanoTransaction, ()>);
pub struct CxxEncodedWitnessResult(Result<CxxEncodedWitness, ()>);

impl_result!(CxxEncodedCardanoTransactionResult);
impl_result!(CxxEncodedCardanoCoinValueResult);
impl_result!(CxxDecodedCardanoCoinValueResult);
impl_result!(CxxEncodedCardanoTransactionOutputResult);
impl_result!(CxxEncodedCardanoUtxoResult);
impl_result!(CxxDecodedCardanoTransactionResult);
impl_result!(CxxEncodedWitnessResult);

impl CxxEncodedCardanoTransactionResult {
    fn bytes(self: &CxxEncodedCardanoTransactionResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).tx_bytes.clone()
    }
}

fn encode_cardano_transaction_impl(
    tx: &CxxSerializableTx,
) -> Result<CxxEncodedCardanoTransaction, ()> {
    let mut arr: Vec<CborValue> = vec![];

    arr.push(encode_tx_body(&tx.body));
    arr.push(encode_tx_witness(&tx.witness));
    arr.push(CborValue::Bool(true)); // valid flag
    arr.push(CborValue::Null); // auxiliary data

    let mut tx_bytes = Vec::new();
    ciborium::ser::into_writer(&CborValue::Array(arr), &mut tx_bytes).map_err(|_| ())?;

    Ok(CxxEncodedCardanoTransaction { tx_bytes })
}

pub fn encode_cardano_transaction(
    tx: &CxxSerializableTx,
) -> Box<CxxEncodedCardanoTransactionResult> {
    Box::new(CxxEncodedCardanoTransactionResult(encode_cardano_transaction_impl(tx)))
}

impl CxxEncodedCardanoCoinValueResult {
    fn bytes(self: &CxxEncodedCardanoCoinValueResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).value_bytes.clone()
    }
}

fn encode_cardano_coin_value_impl(
    value: &CxxSerializableCoinValue,
) -> Result<CxxEncodedCardanoCoinValue, ()> {
    let mut value_bytes = Vec::new();
    ciborium::ser::into_writer(&encode_coin_value(value), &mut value_bytes).map_err(|_| ())?;

    Ok(CxxEncodedCardanoCoinValue { value_bytes })
}

pub fn encode_cardano_coin_value(
    value: &CxxSerializableCoinValue,
) -> Box<CxxEncodedCardanoCoinValueResult> {
    Box::new(CxxEncodedCardanoCoinValueResult(encode_cardano_coin_value_impl(value)))
}

impl CxxDecodedCardanoCoinValueResult {
    fn value(self: &CxxDecodedCardanoCoinValueResult) -> CxxSerializableCoinValue {
        unwrap_or_panic(&self.0).value.clone()
    }
}

fn decode_cardano_coin_value_impl(value_bytes: &[u8]) -> Result<CxxDecodedCardanoCoinValue, ()> {
    let cbor_value: CborValue = from_reader(value_bytes).map_err(|_| ())?;
    let value = decode_coin_value(&cbor_value)?;
    Ok(CxxDecodedCardanoCoinValue { value })
}

pub fn decode_cardano_coin_value(value_bytes: &[u8]) -> Box<CxxDecodedCardanoCoinValueResult> {
    Box::new(CxxDecodedCardanoCoinValueResult(decode_cardano_coin_value_impl(value_bytes)))
}

impl CxxEncodedCardanoTransactionOutputResult {
    fn bytes(self: &CxxEncodedCardanoTransactionOutputResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).output_bytes.clone()
    }
}

fn encode_cardano_transaction_output_impl(
    output: &CxxSerializableTxOutput,
) -> Result<CxxEncodedCardanoTransactionOutput, ()> {
    let mut output_bytes = Vec::new();
    ciborium::ser::into_writer(&encode_tx_output(output), &mut output_bytes).map_err(|_| ())?;

    Ok(CxxEncodedCardanoTransactionOutput { output_bytes })
}

pub fn encode_cardano_transaction_output(
    output: &CxxSerializableTxOutput,
) -> Box<CxxEncodedCardanoTransactionOutputResult> {
    Box::new(CxxEncodedCardanoTransactionOutputResult(encode_cardano_transaction_output_impl(
        output,
    )))
}

impl CxxEncodedCardanoUtxoResult {
    fn bytes(self: &CxxEncodedCardanoUtxoResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).utxo_bytes.clone()
    }
}

fn encode_cardano_utxo_impl(
    input: &CxxSerializableTxInput,
    output: &CxxSerializableTxOutput,
) -> Result<CxxEncodedCardanoUtxo, ()> {
    let utxo = CborValue::Array(vec![encode_tx_input(input), encode_tx_output(output)]);

    let mut utxo_bytes = Vec::new();
    ciborium::ser::into_writer(&utxo, &mut utxo_bytes).map_err(|_| ())?;

    Ok(CxxEncodedCardanoUtxo { utxo_bytes })
}

pub fn encode_cardano_utxo(
    input: &CxxSerializableTxInput,
    output: &CxxSerializableTxOutput,
) -> Box<CxxEncodedCardanoUtxoResult> {
    Box::new(CxxEncodedCardanoUtxoResult(encode_cardano_utxo_impl(input, output)))
}

fn encode_tx_input(input: &CxxSerializableTxInput) -> CborValue {
    let tx_hash = CborValue::Bytes(input.tx_hash.to_vec());
    let index = CborValue::Integer(input.index.into());
    CborValue::Array(vec![tx_hash, index])
}

fn encode_tx_inputs(inputs: &[CxxSerializableTxInput]) -> CborValue {
    // Sort inputs as it is required to produce same binary form for the same
    // transaction.
    let mut sorted_inputs: Vec<_> = inputs.iter().collect();
    sorted_inputs.sort_by_key(|input| (&input.tx_hash, input.index));

    let inputs_cbor: Vec<CborValue> =
        sorted_inputs.iter().map(|input| encode_tx_input(input)).collect();

    if USE_SET_TAG.load(Ordering::Relaxed) {
        CborValue::Tag(SET_TAG, Box::new(CborValue::Array(inputs_cbor)))
    } else {
        CborValue::Array(inputs_cbor)
    }
}

fn encode_tx_output_tokens(tokens: &Vec<CxxSerializableTxOutputToken>) -> CborValue {
    fn asset_map_to_cbor(assets: &BTreeMap<&[u8], u64>) -> CborValue {
        let assets_cbor_list = assets
            .iter()
            .map(|(name, amount)| {
                (CborValue::Bytes(name.to_vec()), CborValue::Integer((*amount).into()))
            })
            .collect();

        CborValue::Map(assets_cbor_list)
    }

    fn policies_map_to_cbor(multiasset: &BTreeMap<&[u8], BTreeMap<&[u8], u64>>) -> CborValue {
        let cbor_value_list = multiasset
            .iter()
            .map(|(policy_id, assets)| {
                (CborValue::Bytes(policy_id.to_vec()), asset_map_to_cbor(&assets))
            })
            .collect();

        CborValue::Map(cbor_value_list)
    }

    // Build the multiasset map for tokens (policy_id -> (name -> amount)).
    use std::collections::BTreeMap;
    let mut multiasset: BTreeMap<&[u8], BTreeMap<&[u8], u64>> = BTreeMap::new();
    for token in tokens.iter() {
        let token_id = token.token_id.as_slice();
        let (policy_id, name) = token_id.split_at(token_id.len().min(CARDANO_SCRIPT_HASH_SIZE));

        let policy_entry = multiasset.entry(policy_id).or_insert_with(BTreeMap::new);
        policy_entry.insert(name, token.amount);
    }

    // Build the CBOR map for the sorted multiasset.
    policies_map_to_cbor(&multiasset)
}

// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L190
fn encode_coin_value(amount: &CxxSerializableCoinValue) -> CborValue {
    if amount.tokens.is_empty() {
        // No tokens, so return as lovelace_amount
        return CborValue::Integer(amount.lovelace_amount.into());
    }

    // Have tokens, so return as [lovelace_amount, multiasset]
    return CborValue::Array(vec![
        CborValue::Integer(amount.lovelace_amount.into()),
        encode_tx_output_tokens(&amount.tokens),
    ]);
}

// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L190
fn decode_coin_value(value: &CborValue) -> Result<CxxSerializableCoinValue, ()> {
    match value {
        // Simple case: only ADA.
        CborValue::Integer(_) => {
            let lovelace_amount = decode_u64(value)?;
            Ok(CxxSerializableCoinValue { lovelace_amount, tokens: Vec::new() })
        }
        // Multi-asset case: [ada, tokens_map].
        CborValue::Array(arr) => {
            if arr.len() != 2 {
                return Err(());
            }
            let lovelace_amount = decode_u64(&arr[0])?;
            let tokens = extract_tokens(&arr[1])?;
            Ok(CxxSerializableCoinValue { lovelace_amount, tokens })
        }
        _ => Err(()),
    }
}

//https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L146
fn encode_tx_output(output: &CxxSerializableTxOutput) -> CborValue {
    return CborValue::Array(vec![
        CborValue::Bytes(output.addr.clone()),
        encode_coin_value(&output.coin_value),
    ]);
}

fn encode_tx_outputs(outputs: &[CxxSerializableTxOutput]) -> CborValue {
    let outputs_cbor: Vec<CborValue> =
        outputs.iter().map(|output| encode_tx_output(output)).collect();

    CborValue::Array(outputs_cbor)
}

// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L112
fn encode_tx_body(body: &CxxSerializableTxBody) -> CborValue {
    let mut body_map = Vec::new();

    // Inputs.
    body_map.push((CborValue::Integer(INPUTS_KEY.into()), encode_tx_inputs(&body.inputs)));

    // Outputs.
    body_map.push((CborValue::Integer(OUTPUTS_KEY.into()), encode_tx_outputs(&body.outputs)));

    // Fee.
    body_map.push((CborValue::Integer(FEE_KEY.into()), CborValue::Integer(body.fee.into())));

    // TTL (optional).
    if body.has_ttl {
        body_map.push((CborValue::Integer(TTL_KEY.into()), CborValue::Integer(body.ttl.into())));
    }

    CborValue::Map(body_map)
}

fn encode_tx_vkey_witness(witness: &CxxSerializableVkeyWitness) -> CborValue {
    CborValue::Array(vec![
        CborValue::Bytes(witness.pubkey.to_vec()),
        CborValue::Bytes(witness.signature.to_vec()),
    ])
}

// Encode `? 0 : nonempty_set<vkeywitness>`.
// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L688
fn encode_tx_vkey_witness_set(
    vkey_witnesses: &[CxxSerializableVkeyWitness],
) -> Option<(CborValue, CborValue)> {
    // If there is no signatures, return None and don't include VK_WITNESS_KEY in
    // the map.
    if vkey_witnesses.is_empty() {
        return None;
    }

    // Spec does't require any sorting of vkey witnesses, but we sort them to
    // produce same binary form for the same transaction.
    let mut sorted_witnesses: Vec<_> = vkey_witnesses.iter().collect();
    sorted_witnesses.sort_by_key(|w| w.pubkey);

    let witness_cbor: Vec<CborValue> =
        sorted_witnesses.into_iter().map(|witness| encode_tx_vkey_witness(witness)).collect();

    let encoded_set = if USE_SET_TAG.load(Ordering::Relaxed) {
        CborValue::Tag(SET_TAG, Box::new(CborValue::Array(witness_cbor)))
    } else {
        CborValue::Array(witness_cbor)
    };

    Some((CborValue::Integer(VK_WITNESS_KEY.into()), encoded_set))
}

// Encode transaction_witness_set.
// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L687
fn encode_tx_witness(witness: &CxxSerializableTxWitness) -> CborValue {
    let mut witness_map = Vec::new();
    if let Some(entry) = encode_tx_vkey_witness_set(&witness.vkey_witness_set) {
        witness_map.push(entry);
    }

    CborValue::Map(witness_map)
}

pub fn get_cardano_transaction_hash(tx: &CxxSerializableTx) -> [u8; CARDANO_TX_HASH_SIZE] {
    let body_value = encode_tx_body(&tx.body);

    let mut raw_body = Vec::new();
    ciborium::ser::into_writer(&body_value, &mut raw_body).unwrap();

    let mut hash = [0_u8; CARDANO_TX_HASH_SIZE];
    hash.copy_from_slice(
        Blake2bParams::new().hash_length(CARDANO_TX_HASH_SIZE).hash(&raw_body).as_bytes(),
    );
    hash
}

/// Extracts the transaction body from a Cardano transaction CBOR
/// https://github.com/IntersectMBO/cardano-ledger/blob/master/eras/conway/impl/cddl-files/conway.cddl#L17-L18
fn extract_cardano_body(cbor_value: &CborValue) -> Result<(CxxSerializableTxBody, Vec<u8>), ()> {
    let transaction_array = decode_array(cbor_value)?;

    if transaction_array.len() <= TRANSACTION_BODY_INDEX {
        return Err(());
    };

    let body_value = &transaction_array[TRANSACTION_BODY_INDEX];
    let body_map = match body_value {
        CborValue::Map(map) => map,
        _ => return Err(()),
    };

    // Extract inputs and outputs
    let inputs = extract_inputs(body_map)?;
    let outputs = decode_outputs(body_map)?;

    // Extract fee
    let fee: u64 = match find_map_value(body_map, FEE_KEY) {
        Some(val) => decode_u64(val)?,
        _ => return Err(()),
    };

    // Extract ttl (optional)
    let (has_ttl, ttl) = match find_map_value(body_map, TTL_KEY) {
        Some(val) => (true, decode_u64(val)?),
        None => (false, 0),
    };

    // Certificates are not supported. Reject any transaction that carries them
    // so we never sign a body whose staking/delegation effects we don't decode.
    if find_map_value(body_map, CERTIFICATES_KEY).is_some() {
        return Err(());
    }

    // Extract withdrawals (optional).
    let withdrawals = decode_withdrawals(body_map)?;

    // Extract mint (optional).
    let mint = decode_mint(body_map)?;

    // Serialize the body for raw bytes
    let mut raw_body = Vec::new();
    ciborium::ser::into_writer(body_value, &mut raw_body).map_err(|_| ())?;

    Ok((CxxSerializableTxBody { inputs, outputs, fee, has_ttl, ttl, withdrawals, mint }, raw_body))
}

// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L699
fn decode_tx_vkey_witness(witness_value: &CborValue) -> Result<CxxSerializableVkeyWitness, ()> {
    let witness_array = decode_array(witness_value)?;

    if witness_array.len() != 2 {
        return Err(());
    }

    let pubkey: [u8; CARDANO_PUBKEY_SIZE] = decode_bytes_array(&witness_array[0])?;
    let signature: [u8; CARDANO_SIGNATURE_SIZE] = decode_bytes_array(&witness_array[1])?;

    Ok(CxxSerializableVkeyWitness { pubkey, signature })
}

// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L687
fn decode_witness(cbor_value: &CborValue) -> Result<CxxSerializableTxWitness, ()> {
    let transaction_array = decode_array(cbor_value)?;

    if transaction_array.len() <= WITNESS_SET_INDEX {
        return Err(());
    }

    let CborValue::Map(witness_map) = &transaction_array[WITNESS_SET_INDEX] else {
        return Err(());
    };

    let vk_witness_value = match find_map_value(witness_map, VK_WITNESS_KEY) {
        Some(val) => val,
        None => return Ok(CxxSerializableTxWitness { vkey_witness_set: Vec::new() }),
    };
    let vk_witness_array = decode_nonempty_set(vk_witness_value)?;
    let vkey_witness_set = vk_witness_array
        .iter()
        .map(|witness| decode_tx_vkey_witness(witness))
        .collect::<Result<Vec<_>, _>>()?;

    Ok(CxxSerializableTxWitness { vkey_witness_set })
}

fn extract_inputs(body_map: &[(CborValue, CborValue)]) -> Result<Vec<CxxSerializableTxInput>, ()> {
    let inputs_value = find_map_value(body_map, INPUTS_KEY);

    let inputs_array = match inputs_value {
        Some(value) => decode_nonempty_set(value)?,
        None => return Ok(Vec::new()), // No inputs
    };

    let mut inputs = Vec::with_capacity(inputs_array.len());
    for input_value in inputs_array {
        let input_array = decode_array(input_value)?;

        if input_array.len() != 2 {
            return Err(());
        }

        // Extract transaction hash and index
        let tx_hash = decode_bytes_array(&input_array[0])?;
        let index = decode_u32(&input_array[1])?;

        inputs.push(CxxSerializableTxInput { tx_hash, index });
    }

    Ok(inputs)
}

fn extract_tokens(multiasset: &CborValue) -> Result<Vec<CxxSerializableTxOutputToken>, ()> {
    // Parse (name -> amount) map.
    fn process_asset_map(asset_map_val: &CborValue) -> Result<Vec<(Vec<u8>, u64)>, ()> {
        let CborValue::Map(asset_map) = asset_map_val else {
            return Err(());
        };

        return asset_map
            .iter()
            .map(|(asset_name_val, amount_val)| {
                let asset_name = decode_bytes_vector(asset_name_val)?;
                let token_amount = decode_u64(amount_val)?;
                Ok((asset_name, token_amount))
            })
            .collect();
    }

    // Parse multiasset map (policy_id -> (name -> amount)).
    let CborValue::Map(token_map) = &multiasset else {
        return Err(());
    };

    let mut tokens: Vec<CxxSerializableTxOutputToken> = Vec::new();

    for (policy_id_val, asset_map_val) in token_map {
        let policy_id: [u8; CARDANO_SCRIPT_HASH_SIZE] = decode_bytes_array(policy_id_val)?;
        let pairs = process_asset_map(asset_map_val)?;
        for (asset_name, token_amount) in pairs {
            tokens.push(CxxSerializableTxOutputToken {
                token_id: [policy_id.as_slice(), asset_name.as_slice()].concat(),
                amount: token_amount,
            });
        }
    }

    Ok(tokens)
}

// alonzo_transaction_output = [address, amount : value, ? datum_hash : hash32]
// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L146
fn parse_alonzo_transaction_output(
    output_array: &[CborValue],
) -> Result<CxxSerializableTxOutput, ()> {
    if output_array.len() < 2 {
        return Err(());
    }

    let addr_bytes = decode_bytes_vector(&output_array[0])?;
    let coin_value = decode_coin_value(&output_array[1])?;

    Ok(CxxSerializableTxOutput { addr: addr_bytes, coin_value })
}

// babbage_transaction_output = { 0 : address, 1 : value, ? 2 : datum_option, ?
// 3 : script_ref }
// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L213
fn parse_babbage_transaction_output(
    output_map: &[(CborValue, CborValue)],
) -> Result<CxxSerializableTxOutput, ()> {
    let address = find_map_value(output_map, BABBAGE_OUTPUT_ADDRESS_KEY).ok_or(())?;
    let value = find_map_value(output_map, BABBAGE_OUTPUT_VALUE_KEY).ok_or(())?;

    let addr_bytes = decode_bytes_vector(address)?;
    let coin_value = decode_coin_value(value)?;

    Ok(CxxSerializableTxOutput { addr: addr_bytes, coin_value })
}

// Decode array of transaction outputs from the transaction body.
// 1  : [* transaction_output]
// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L114
fn decode_outputs(body_map: &[(CborValue, CborValue)]) -> Result<Vec<CxxSerializableTxOutput>, ()> {
    let outputs_value = find_map_value(body_map, OUTPUTS_KEY);
    let Some(outputs_value) = outputs_value else {
        return Ok(Vec::new()); // No outputs
    };

    let outputs_array = decode_array(outputs_value)?;

    let mut outputs = Vec::with_capacity(outputs_array.len());
    for output_value in outputs_array {
        // Each item might be an alonzo or babbage transaction output.
        // https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L144
        match output_value {
            CborValue::Array(arr) => {
                outputs.push(parse_alonzo_transaction_output(&arr)?);
            }
            CborValue::Map(output_map) => {
                outputs.push(parse_babbage_transaction_output(output_map)?);
            }
            _ => return Err(()),
        }
    }

    Ok(outputs)
}

// withdrawals = {+ reward_account => coin}
// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L467
fn decode_withdrawals(
    body_map: &[(CborValue, CborValue)],
) -> Result<Vec<CxxSerializableWithdrawal>, ()> {
    let Some(withdrawals_value) = find_map_value(body_map, WITHDRAWALS_KEY) else {
        return Ok(Vec::new());
    };

    let CborValue::Map(withdrawals_map) = withdrawals_value else {
        return Err(());
    };

    // CDDL `{+ ...}` requires a non-empty map when the field is present.
    if withdrawals_map.is_empty() {
        return Err(());
    }

    withdrawals_map
        .iter()
        .map(|(reward_account_val, coin_val)| {
            let reward_account = decode_bytes_vector(reward_account_val)?;
            let coin = decode_u64(coin_val)?;
            Ok(CxxSerializableWithdrawal { reward_account, coin })
        })
        .collect()
}

// mint = {+ policy_id => {+ asset_name => nonzero_int64}}
// https://github.com/IntersectMBO/cardano-ledger/blob/8d5d83d9929f7facbcd972edfcda8da3bfdeec10/eras/conway/impl/cddl/data/conway.cddl#L471
fn decode_mint(body_map: &[(CborValue, CborValue)]) -> Result<Vec<CxxSerializableMintToken>, ()> {
    let Some(mint_value) = find_map_value(body_map, MINT_KEY) else {
        return Ok(Vec::new());
    };

    let CborValue::Map(mint_map) = mint_value else {
        return Err(());
    };

    // CDDL `{+ ...}` requires a non-empty map when the field is present.
    if mint_map.is_empty() {
        return Err(());
    }

    let mut mint_tokens: Vec<CxxSerializableMintToken> = Vec::new();

    for (policy_id_val, asset_map_val) in mint_map {
        let policy_id: [u8; CARDANO_SCRIPT_HASH_SIZE] = decode_bytes_array(policy_id_val)?;
        let CborValue::Map(asset_map) = asset_map_val else {
            return Err(());
        };
        if asset_map.is_empty() {
            return Err(());
        }
        for (asset_name_val, amount_val) in asset_map {
            let asset_name = decode_bytes_vector(asset_name_val)?;
            let amount = decode_nonzero_i64(amount_val)?;
            mint_tokens.push(CxxSerializableMintToken {
                token_id: [policy_id.as_slice(), asset_name.as_slice()].concat(),
                amount,
            });
        }
    }

    Ok(mint_tokens)
}

pub fn decode_cardano_transaction(bytes: &[u8]) -> Box<CxxDecodedCardanoTransactionResult> {
    Box::new(CxxDecodedCardanoTransactionResult(decode_cardano_transaction_impl(bytes)))
}

fn decode_cardano_transaction_impl(bytes: &[u8]) -> Result<CxxDecodedCardanoTransaction, ()> {
    let cbor_value: CborValue = from_reader(bytes).map_err(|_| ())?;

    let (body, raw_body) = extract_cardano_body(&cbor_value)?;

    let witness = decode_witness(&cbor_value)?;

    Ok(CxxDecodedCardanoTransaction {
        tx: CxxSerializableTx { body, witness },
        raw_body,
        raw_tx: bytes.to_vec(),
    })
}

impl CxxDecodedCardanoTransactionResult {
    fn tx(self: &CxxDecodedCardanoTransactionResult) -> CxxSerializableTx {
        unwrap_or_panic(&self.0).tx.clone()
    }

    fn raw_body(self: &CxxDecodedCardanoTransactionResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).raw_body.clone()
    }

    fn raw_tx(self: &CxxDecodedCardanoTransactionResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).raw_tx.clone()
    }
}

fn encode_cardano_witness_impl(
    witnesses: &CxxSerializableTxWitness,
) -> Result<CxxEncodedWitness, ()> {
    let witness_value = encode_tx_witness(witnesses);

    let mut witness_bytes = Vec::new();
    ciborium::ser::into_writer(&witness_value, &mut witness_bytes).map_err(|_| ())?;

    Ok(CxxEncodedWitness { witness_bytes })
}

pub fn encode_cardano_witness(witnesses: CxxSerializableTxWitness) -> Box<CxxEncodedWitnessResult> {
    Box::new(CxxEncodedWitnessResult(encode_cardano_witness_impl(&witnesses)))
}

impl CxxEncodedWitnessResult {
    fn bytes(self: &CxxEncodedWitnessResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).witness_bytes.clone()
    }
}
