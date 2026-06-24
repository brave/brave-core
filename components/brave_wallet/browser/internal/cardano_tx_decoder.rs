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

use std::fmt;
use std::sync::atomic::{AtomicBool, Ordering};

use blake2b_simd::Params as Blake2bParams;
use ciborium::{de::from_reader, Value as CborValue};

// Re-export types for external use
#[allow(unused_imports)]
use crate::ffi::CxxSerializableCoinValue;
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

// Constants definitions come from
// https://github.com/IntersectMBO/cardano-ledger/blob/30d293d7166561d47bccf56b7e1473cdab84714d/eras/conway/impl/cddl/data/conway.cddl
const CARDANO_TX_HASH_SIZE: usize = 32;
const CARDANO_SCRIPT_HASH_SIZE: usize = 28;
const TRANSACTION_BODY_INDEX: usize = 0;
const WITNESS_SET_INDEX: usize = 1;
const INPUTS_KEY: u8 = 0;
const OUTPUTS_KEY: u8 = 1;
const FEE_KEY: u8 = 2;
const TTL_KEY: u8 = 3;
const VK_WITNESS_KEY: u8 = 0;
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

fn unwrap_or_panic<T>(result: &Result<T, Error>) -> &T {
    match result {
        Ok(v) => v,
        Err(e) => panic!("{}", e.to_string()),
    }
}

#[macro_export]
macro_rules! impl_error {
    ($t:ident, $n:ident) => {
        impl From<$t> for Error {
            fn from(err: $t) -> Self {
                Self::$n(err)
            }
        }
    };
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
    struct CxxSerializableTxBody {
        inputs: Vec<CxxSerializableTxInput>,
        outputs: Vec<CxxSerializableTxOutput>,
        fee: u64,
        has_ttl: bool,
        ttl: u64,
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
        type CxxSignedCardanoTransactionResult;

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
        fn apply_signatures(
            bytes: &[u8],
            witnesses: CxxSerializableTxWitness,
        ) -> Box<CxxSignedCardanoTransactionResult>;

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

        fn is_ok(self: &CxxSignedCardanoTransactionResult) -> bool;
        fn bytes(self: &CxxSignedCardanoTransactionResult) -> Vec<u8>;
    }
}

/// Errors that can occur during Cardano transaction processing
#[derive(Clone, Debug)]
pub enum Error {
    /// Failed to decode CBOR data
    CborDecodeError,
    /// Invalid transaction format
    InvalidTransactionFormat,
    /// Invalid input format
    InvalidInputFormat,
    /// Invalid output format
    InvalidOutputFormat,
    /// Serialization error
    SerializationError,
    /// Failed to resolve witness array from transaction
    WitnessArrayResolutionError,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::CborDecodeError => write!(f, "Failed to decode CBOR data"),
            Error::InvalidTransactionFormat => write!(f, "Invalid transaction format"),
            Error::InvalidInputFormat => write!(f, "Invalid input format"),
            Error::InvalidOutputFormat => write!(f, "Invalid output format"),
            Error::SerializationError => write!(f, "Serialization error"),
            Error::WitnessArrayResolutionError => {
                write!(f, "Failed to resolve witness array from transaction")
            }
        }
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

struct CxxSignedCardanoTransaction {
    signed_bytes: Vec<u8>,
}

pub struct CxxEncodedCardanoTransactionResult(Result<CxxEncodedCardanoTransaction, Error>);
pub struct CxxEncodedCardanoCoinValueResult(Result<CxxEncodedCardanoCoinValue, Error>);
pub struct CxxDecodedCardanoCoinValueResult(Result<CxxDecodedCardanoCoinValue, Error>);
pub struct CxxEncodedCardanoTransactionOutputResult(
    Result<CxxEncodedCardanoTransactionOutput, Error>,
);
pub struct CxxEncodedCardanoUtxoResult(Result<CxxEncodedCardanoUtxo, Error>);
pub struct CxxDecodedCardanoTransactionResult(Result<CxxDecodedCardanoTransaction, Error>);
pub struct CxxSignedCardanoTransactionResult(Result<CxxSignedCardanoTransaction, Error>);

impl_result!(CxxEncodedCardanoTransactionResult);
impl_result!(CxxEncodedCardanoCoinValueResult);
impl_result!(CxxDecodedCardanoCoinValueResult);
impl_result!(CxxEncodedCardanoTransactionOutputResult);
impl_result!(CxxEncodedCardanoUtxoResult);
impl_result!(CxxDecodedCardanoTransactionResult);
impl_result!(CxxSignedCardanoTransactionResult);

impl CxxEncodedCardanoTransactionResult {
    fn bytes(self: &CxxEncodedCardanoTransactionResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).tx_bytes.clone()
    }
}

fn encode_cardano_transaction_impl(
    tx: &CxxSerializableTx,
) -> Result<CxxEncodedCardanoTransaction, Error> {
    let mut arr: Vec<CborValue> = vec![];

    arr.push(encode_tx_body(&tx.body));
    arr.push(encode_tx_witness(&tx.witness));
    arr.push(CborValue::Bool(true)); // valid flag
    arr.push(CborValue::Null); // auxiliary data

    let mut tx_bytes = Vec::new();
    ciborium::ser::into_writer(&CborValue::Array(arr), &mut tx_bytes)
        .map_err(|_| Error::SerializationError)?;

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
) -> Result<CxxEncodedCardanoCoinValue, Error> {
    let mut value_bytes = Vec::new();
    ciborium::ser::into_writer(&encode_coin_value(value), &mut value_bytes)
        .map_err(|_| Error::SerializationError)?;

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

fn decode_cardano_coin_value_impl(
    value_bytes: &[u8],
) -> Result<CxxDecodedCardanoCoinValue, Error> {
    let cbor_value: CborValue = from_reader(value_bytes).map_err(|_| Error::CborDecodeError)?;
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
) -> Result<CxxEncodedCardanoTransactionOutput, Error> {
    let mut output_bytes = Vec::new();
    ciborium::ser::into_writer(&encode_tx_output(output), &mut output_bytes)
        .map_err(|_| Error::SerializationError)?;

    Ok(CxxEncodedCardanoTransactionOutput { output_bytes })
}

pub fn encode_cardano_transaction_output(
    output: &CxxSerializableTxOutput,
) -> Box<CxxEncodedCardanoTransactionOutputResult> {
    Box::new(CxxEncodedCardanoTransactionOutputResult(
        encode_cardano_transaction_output_impl(output),
    ))
}

impl CxxEncodedCardanoUtxoResult {
    fn bytes(self: &CxxEncodedCardanoUtxoResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).utxo_bytes.clone()
    }
}

fn encode_cardano_utxo_impl(
    input: &CxxSerializableTxInput,
    output: &CxxSerializableTxOutput,
) -> Result<CxxEncodedCardanoUtxo, Error> {
    let utxo = CborValue::Array(vec![encode_tx_input(input), encode_tx_output(output)]);

    let mut utxo_bytes = Vec::new();
    ciborium::ser::into_writer(&utxo, &mut utxo_bytes).map_err(|_| Error::SerializationError)?;

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
fn decode_coin_value(value: &CborValue) -> Result<CxxSerializableCoinValue, Error> {
    match value {
        // Simple case: only ADA.
        CborValue::Integer(i) => {
            let lovelace_amount: u64 = (*i).try_into().map_err(|_| Error::InvalidOutputFormat)?;
            Ok(CxxSerializableCoinValue { lovelace_amount, tokens: Vec::new() })
        }
        // Multi-asset case: [ada, tokens_map].
        CborValue::Array(arr) => {
            if arr.len() != 2 {
                return Err(Error::InvalidOutputFormat);
            }
            let lovelace_amount: u64 = match &arr[0] {
                CborValue::Integer(i) => (*i).try_into().map_err(|_| Error::InvalidOutputFormat)?,
                _ => return Err(Error::InvalidOutputFormat),
            };
            let tokens = extract_tokens(&arr[1])?;
            Ok(CxxSerializableCoinValue { lovelace_amount, tokens })
        }
        _ => Err(Error::InvalidOutputFormat),
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

fn encode_tx_vkey_witness_set(vkey_witnesses: &[CxxSerializableVkeyWitness]) -> CborValue {
    // Spec does't require any sorting of vkey witnesses, but we sort them to
    // produce same binary form for the same transaction.
    let mut sorted_witnesses: Vec<_> = vkey_witnesses.iter().collect();
    sorted_witnesses.sort_by_key(|w| w.pubkey);

    let witness_cbor: Vec<CborValue> =
        sorted_witnesses.into_iter().map(|witness| encode_tx_vkey_witness(witness)).collect();

    if USE_SET_TAG.load(Ordering::Relaxed) {
        CborValue::Tag(SET_TAG, Box::new(CborValue::Array(witness_cbor)))
    } else {
        CborValue::Array(witness_cbor)
    }
}

fn encode_tx_witness(witness: &CxxSerializableTxWitness) -> CborValue {
    let mut witness_map = Vec::new();
    witness_map.push((
        CborValue::Integer(VK_WITNESS_KEY.into()),
        encode_tx_vkey_witness_set(&witness.vkey_witness_set),
    ));

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
fn extract_cardano_body(cbor_value: &CborValue) -> Result<(CxxSerializableTxBody, Vec<u8>), Error> {
    let transaction_array = match cbor_value {
        CborValue::Array(arr) => arr,
        _ => return Err(Error::InvalidTransactionFormat),
    };

    if TRANSACTION_BODY_INDEX >= transaction_array.len() {
        return Err(Error::InvalidTransactionFormat);
    };

    let body_value = &transaction_array[TRANSACTION_BODY_INDEX];
    let body_map = match body_value {
        CborValue::Map(map) => map,
        _ => return Err(Error::InvalidTransactionFormat),
    };

    // Extract inputs and outputs
    let inputs = extract_inputs(body_map)?;
    let outputs = extract_outputs(body_map)?;

    // Extract fee
    let fee: u64 = match find_map_value(body_map, FEE_KEY)? {
        Some(CborValue::Integer(f)) => {
            (*f).try_into().map_err(|_| Error::InvalidTransactionFormat)?
        }
        Some(_) => return Err(Error::InvalidTransactionFormat),
        None => return Err(Error::InvalidTransactionFormat),
    };

    // Extract ttl (optional)
    let (has_ttl, ttl) = match find_map_value(body_map, TTL_KEY)? {
        Some(CborValue::Integer(t)) => {
            let ttl_val: u64 = (*t).try_into().map_err(|_| Error::InvalidTransactionFormat)?;
            (true, ttl_val)
        }
        Some(_) => return Err(Error::InvalidTransactionFormat),
        None => (false, 0),
    };

    // Serialize the body for raw bytes
    let mut raw_body = Vec::new();
    ciborium::ser::into_writer(body_value, &mut raw_body).map_err(|_| Error::SerializationError)?;

    Ok((CxxSerializableTxBody { inputs, outputs, fee, has_ttl, ttl }, raw_body))
}

fn extract_witness(_cbor_value: &CborValue) -> Result<CxxSerializableTxWitness, Error> {
    // Don't support extracting witness from the transaction yet.
    Ok(CxxSerializableTxWitness { vkey_witness_set: Vec::new() })
}

fn extract_inputs(
    body_map: &[(CborValue, CborValue)],
) -> Result<Vec<CxxSerializableTxInput>, Error> {
    let inputs_value = find_map_value(body_map, INPUTS_KEY)?;
    // Inputs are stored directly in an array value or in a tag value wrapping an
    // array.
    let inputs_array: &Vec<_> = match inputs_value {
        Some(CborValue::Array(arr)) => arr,
        Some(CborValue::Tag(SET_TAG, tagged_array)) => match &**tagged_array {
            CborValue::Array(arr) => arr,
            _ => return Err(Error::InvalidInputFormat),
        },
        Some(_) => return Err(Error::InvalidInputFormat),
        None => return Ok(Vec::new()), // No inputs
    };

    let mut inputs = Vec::with_capacity(inputs_array.len());
    for input_value in inputs_array {
        let input_array = match input_value {
            CborValue::Array(arr) => {
                if arr.len() != 2 {
                    return Err(Error::InvalidInputFormat);
                }
                arr
            }
            _ => return Err(Error::InvalidInputFormat),
        };

        // Extract transaction hash and index
        let tx_hash: [u8; CARDANO_TX_HASH_SIZE] = match &input_array[0] {
            CborValue::Bytes(bytes) => {
                bytes.clone().try_into().map_err(|_| Error::InvalidInputFormat)?
            }
            _ => return Err(Error::InvalidInputFormat),
        };

        let index: u32 = match &input_array[1] {
            CborValue::Integer(i) => (*i).try_into().map_err(|_| Error::InvalidInputFormat)?,
            _ => return Err(Error::InvalidInputFormat),
        };

        inputs.push(CxxSerializableTxInput { tx_hash, index });
    }

    Ok(inputs)
}

fn extract_tokens(multiasset: &CborValue) -> Result<Vec<CxxSerializableTxOutputToken>, Error> {
    // Parse (name -> amount) map.
    fn process_asset_map(asset_map_val: &CborValue) -> Result<Vec<(Vec<u8>, u64)>, Error> {
        let CborValue::Map(asset_map) = asset_map_val else {
            return Err(Error::InvalidOutputFormat);
        };

        return asset_map
            .iter()
            .map(|(asset_name_val, amount_val)| {
                let asset_name = match asset_name_val {
                    CborValue::Bytes(bytes) => bytes.clone(),
                    _ => return Err(Error::InvalidOutputFormat),
                };
                let token_amount: u64 = match amount_val {
                    CborValue::Integer(n) => {
                        (*n).try_into().map_err(|_| Error::InvalidOutputFormat)?
                    }
                    _ => return Err(Error::InvalidOutputFormat),
                };
                Ok((asset_name, token_amount))
            })
            .collect();
    }

    // Parse multiasset map (policy_id -> (name -> amount)).
    let CborValue::Map(token_map) = &multiasset else {
        return Err(Error::InvalidOutputFormat);
    };

    let mut tokens: Vec<CxxSerializableTxOutputToken> = Vec::new();

    for (policy_id_val, asset_map_val) in token_map {
        let policy_id: Vec<u8> = match policy_id_val {
            CborValue::Bytes(bytes) if bytes.len() == CARDANO_SCRIPT_HASH_SIZE => bytes.clone(),
            _ => return Err(Error::InvalidOutputFormat),
        };
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

fn extract_outputs(
    body_map: &[(CborValue, CborValue)],
) -> Result<Vec<CxxSerializableTxOutput>, Error> {
    let outputs_value = find_map_value(body_map, OUTPUTS_KEY)?;
    let outputs_array = match outputs_value {
        Some(CborValue::Array(arr)) => arr,
        Some(_) => return Err(Error::InvalidOutputFormat),
        None => return Ok(Vec::new()), // No outputs
    };

    let mut outputs = Vec::with_capacity(outputs_array.len());
    for output_value in outputs_array {
        let output_array = match output_value {
            CborValue::Array(arr) => {
                if arr.len() < 2 {
                    return Err(Error::InvalidOutputFormat);
                }
                arr
            }
            _ => return Err(Error::InvalidOutputFormat),
        };

        let addr_bytes = match &output_array[0] {
            CborValue::Bytes(bytes) => bytes.clone(),
            _ => return Err(Error::InvalidOutputFormat),
        };

        let coin_value = decode_coin_value(&output_array[1])?;

        outputs.push(CxxSerializableTxOutput { addr: addr_bytes, coin_value });
    }

    Ok(outputs)
}

/// Helper function to find a value in a CBOR map by integer key
fn find_map_value(map: &[(CborValue, CborValue)], key: u8) -> Result<Option<&CborValue>, Error> {
    Ok(map.iter()
        .find(|(k, _)| {
            matches!(k, CborValue::Integer(i) if *i == ciborium::value::Integer::from(key))
        })
        .map(|(_, v)| v))
}

pub fn decode_cardano_transaction(bytes: &[u8]) -> Box<CxxDecodedCardanoTransactionResult> {
    Box::new(CxxDecodedCardanoTransactionResult(decode_cardano_transaction_impl(bytes)))
}

fn decode_cardano_transaction_impl(
    bytes: &[u8],
) -> Result<CxxDecodedCardanoTransaction, Error> {
    let cbor_value: CborValue = from_reader(bytes).map_err(|_| Error::CborDecodeError)?;

    let (body, raw_body) = extract_cardano_body(&cbor_value)?;

    let witness = extract_witness(&cbor_value)?;

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

pub fn apply_signatures(
    bytes: &[u8],
    witnesses: CxxSerializableTxWitness,
) -> Box<CxxSignedCardanoTransactionResult> {
    Box::new(CxxSignedCardanoTransactionResult(apply_signatures_impl(bytes, witnesses)))
}

/// Applies signatures to an unsigned Cardano transaction
///
/// This function preserves existing witnesses and adds new signatures.
/// The vk_witness set follows the Cardano format: {0: [[pubkey1, signature1],
/// ...]}
fn apply_signatures_impl(
    bytes: &[u8],
    witnesses: CxxSerializableTxWitness,
) -> Result<CxxSignedCardanoTransaction, Error> {
    let mut cbor_value: CborValue = from_reader(bytes).map_err(|_| Error::CborDecodeError)?;

    let transaction_array = match &mut cbor_value {
        CborValue::Array(arr) => arr,
        _ => return Err(Error::InvalidTransactionFormat),
    };

    if WITNESS_SET_INDEX >= transaction_array.len() {
        return Err(Error::InvalidTransactionFormat);
    };

    let CborValue::Map(witness_map) = &mut transaction_array[WITNESS_SET_INDEX] else {
        return Err(Error::WitnessArrayResolutionError);
    };

    let vk_witness_entry = {
        // If not found, insert a SET_TAG entry with an empty array so it will be found
        // below
        if !witness_map.iter().any(|(k, _)| {
                matches!(k, CborValue::Integer(i) if *i == ciborium::value::Integer::from(VK_WITNESS_KEY))
            }) {
                witness_map.push((
                    CborValue::Integer(ciborium::value::Integer::from(VK_WITNESS_KEY)),
                    CborValue::Tag(SET_TAG, Box::new(CborValue::Array(Vec::new()))),
                ));
            }
        witness_map.iter_mut()
                .find(|(k, _)| {
                    matches!(k, CborValue::Integer(i) if *i == ciborium::value::Integer::from(VK_WITNESS_KEY))
                })
    };

    // Get vk_witness array(possible tagged) and append witness items.
    let Some((_, vk_witness_value)) = vk_witness_entry else {
        return Err(Error::WitnessArrayResolutionError);
    };
    let vk_witness_array = match vk_witness_value {
        CborValue::Array(arr) => arr,
        CborValue::Tag(SET_TAG, tagged_array) => match &mut **tagged_array {
            CborValue::Array(arr) => arr,
            _ => {
                return Err(Error::InvalidInputFormat);
            }
        },
        _ => {
            return Err(Error::InvalidInputFormat);
        }
    };

    for witness in &witnesses.vkey_witness_set {
        vk_witness_array.push(CborValue::Array(vec![
            CborValue::Bytes(witness.pubkey.to_vec()),
            CborValue::Bytes(witness.signature.to_vec()),
        ]));
    }

    // Serialize the signed transaction
    let mut signed_bytes = Vec::new();
    ciborium::ser::into_writer(&cbor_value, &mut signed_bytes)
        .map_err(|_| Error::SerializationError)?;

    Ok(CxxSignedCardanoTransaction { signed_bytes })
}

impl CxxSignedCardanoTransactionResult {
    fn bytes(self: &CxxSignedCardanoTransactionResult) -> Vec<u8> {
        unwrap_or_panic(&self.0).signed_bytes.clone()
    }
}
