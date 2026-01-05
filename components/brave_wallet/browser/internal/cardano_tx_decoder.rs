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
use crate::ffi::CxxSerializableTx;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTxBody;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTxInput;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTxOutput;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableTxWitness;
#[allow(unused_imports)]
use crate::ffi::CxxSerializableVkeyWitness;

const CARDANO_TX_HASH_SIZE: usize = 32;
const TRANSACTION_BODY_INDEX: usize = 0;
const WITNESS_SET_INDEX: usize = 1;
const INPUTS_KEY: u8 = 0;
const OUTPUTS_KEY: u8 = 1;
const FEE_KEY: u8 = 2;
const TTL_KEY: u8 = 3;
const VK_WITNESS_KEY: u8 = 0;
// https://github.com/IntersectMBO/cardano-ledger/blob/master/eras/conway/impl/cddl-files/conway.cddl#L154
const SET_TAG: u64 = 258;

// Global variable to control whether SET_TAG is used (for testing purposes)
static USE_SET_TAG: AtomicBool = AtomicBool::new(true);
pub fn use_set_tag_for_testing(enable: bool) {
    USE_SET_TAG.store(enable, Ordering::Relaxed);
}

#[macro_export]
macro_rules! impl_result {
    ($t:ident, $r:ident, $f:ident) => {
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

        impl From<Result<$f, Error>> for $r {
            fn from(result: Result<$f, Error>) -> Self {
                match result {
                    Ok(v) => Self(Ok($t(v))),
                    Err(e) => Self(Err(e)),
                }
            }
        }
    };
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
        tx_hash: [u8; 32],
        index: u32,
    }

    #[derive(Clone)]
    struct CxxSerializableTxOutput {
        addr: Vec<u8>,
        amount: u64,
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
        pubkey: [u8; 32],
        signature: [u8; 64],
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
        type CxxEncodedCardanoTransaction;
        type CxxEncodedCardanoTransactionOutput;
        type CxxDecodedCardanoTransaction;
        type CxxSignedCardanoTransaction;
        type CxxEncodedCardanoTransactionResult;
        type CxxEncodedCardanoTransactionOutputResult;
        type CxxDecodedCardanoTransactionResult;
        type CxxSignedCardanoTransactionResult;

        fn use_set_tag_for_testing(enable: bool);

        fn encode_cardano_transaction(
            tx: &CxxSerializableTx,
        ) -> Box<CxxEncodedCardanoTransactionResult>;
        fn encode_cardano_transaction_output(
            output: &CxxSerializableTxOutput,
        ) -> Box<CxxEncodedCardanoTransactionOutputResult>;
        fn get_cardano_transaction_hash(tx: &CxxSerializableTx) -> [u8; 32];
        fn decode_cardano_transaction(bytes: &[u8]) -> Box<CxxDecodedCardanoTransactionResult>;
        fn apply_signatures(
            bytes: &[u8],
            witnesses: CxxSerializableTxWitness,
        ) -> Box<CxxSignedCardanoTransactionResult>;

        fn is_ok(self: &CxxEncodedCardanoTransactionResult) -> bool;
        fn error_message(self: &CxxEncodedCardanoTransactionResult) -> String;
        fn unwrap(
            self: &mut CxxEncodedCardanoTransactionResult,
        ) -> Box<CxxEncodedCardanoTransaction>;
        fn bytes(self: &CxxEncodedCardanoTransaction) -> Vec<u8>;

        fn is_ok(self: &CxxEncodedCardanoTransactionOutputResult) -> bool;
        fn error_message(self: &CxxEncodedCardanoTransactionOutputResult) -> String;
        fn unwrap(
            self: &mut CxxEncodedCardanoTransactionOutputResult,
        ) -> Box<CxxEncodedCardanoTransactionOutput>;
        fn bytes(self: &CxxEncodedCardanoTransactionOutput) -> Vec<u8>;

        fn is_ok(self: &CxxDecodedCardanoTransactionResult) -> bool;
        fn error_message(self: &CxxDecodedCardanoTransactionResult) -> String;
        fn unwrap(
            self: &mut CxxDecodedCardanoTransactionResult,
        ) -> Box<CxxDecodedCardanoTransaction>;
        fn tx(self: &CxxDecodedCardanoTransaction) -> CxxSerializableTx;
        fn raw_body(self: &CxxDecodedCardanoTransaction) -> Vec<u8>;
        fn raw_tx(self: &CxxDecodedCardanoTransaction) -> Vec<u8>;

        fn is_ok(self: &CxxSignedCardanoTransactionResult) -> bool;
        fn error_message(self: &CxxSignedCardanoTransactionResult) -> String;
        fn unwrap(self: &mut CxxSignedCardanoTransactionResult)
            -> Box<CxxSignedCardanoTransaction>;
        fn bytes(self: &CxxSignedCardanoTransaction) -> Vec<u8>;
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
    /// Already unwrapped
    AlreadyUnwrapped,
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
            Error::AlreadyUnwrapped => write!(f, "Already unwrapped"),
        }
    }
}

struct CxxEncodedCardanoTransactionValue {
    tx_bytes: Vec<u8>,
}

struct CxxEncodedCardanoTransactionOutputValue {
    output_bytes: Vec<u8>,
}

struct CxxDecodedCardanoTransactionValue {
    tx: CxxSerializableTx,
    raw_body: Vec<u8>,
    raw_tx: Vec<u8>,
}

struct CxxSignedCardanoTransactionValue {
    signed_bytes: Vec<u8>,
}

pub struct CxxEncodedCardanoTransaction(CxxEncodedCardanoTransactionValue);
pub struct CxxEncodedCardanoTransactionOutput(CxxEncodedCardanoTransactionOutputValue);
pub struct CxxDecodedCardanoTransaction(CxxDecodedCardanoTransactionValue);
pub struct CxxSignedCardanoTransaction(CxxSignedCardanoTransactionValue);

pub struct CxxEncodedCardanoTransactionResult(Result<CxxEncodedCardanoTransaction, Error>);
pub struct CxxEncodedCardanoTransactionOutputResult(
    Result<CxxEncodedCardanoTransactionOutput, Error>,
);
pub struct CxxDecodedCardanoTransactionResult(Result<CxxDecodedCardanoTransaction, Error>);
pub struct CxxSignedCardanoTransactionResult(Result<CxxSignedCardanoTransaction, Error>);

impl_result!(
    CxxEncodedCardanoTransaction,
    CxxEncodedCardanoTransactionResult,
    CxxEncodedCardanoTransactionValue
);

impl_result!(
    CxxEncodedCardanoTransactionOutput,
    CxxEncodedCardanoTransactionOutputResult,
    CxxEncodedCardanoTransactionOutputValue
);

impl_result!(
    CxxDecodedCardanoTransaction,
    CxxDecodedCardanoTransactionResult,
    CxxDecodedCardanoTransactionValue
);

impl_result!(
    CxxSignedCardanoTransaction,
    CxxSignedCardanoTransactionResult,
    CxxSignedCardanoTransactionValue
);

impl CxxEncodedCardanoTransaction {
    fn bytes(self: &CxxEncodedCardanoTransaction) -> Vec<u8> {
        self.0.tx_bytes.clone()
    }
}

fn encode_cardano_transaction_impl(
    tx: &CxxSerializableTx,
) -> Result<CxxEncodedCardanoTransactionValue, Error> {
    let mut arr: Vec<CborValue> = vec![];

    arr.push(encode_tx_body(&tx.body));
    arr.push(encode_tx_witness(&tx.witness));
    arr.push(CborValue::Bool(true)); // valid flag
    arr.push(CborValue::Null); // auxiliary data

    let mut tx_bytes = Vec::new();
    ciborium::ser::into_writer(&CborValue::Array(arr), &mut tx_bytes)
        .map_err(|_| Error::SerializationError)?;

    Ok(CxxEncodedCardanoTransactionValue { tx_bytes })
}

pub fn encode_cardano_transaction(
    tx: &CxxSerializableTx,
) -> Box<CxxEncodedCardanoTransactionResult> {
    Box::new(CxxEncodedCardanoTransactionResult::from(encode_cardano_transaction_impl(tx)))
}

impl CxxEncodedCardanoTransactionOutput {
    fn bytes(self: &CxxEncodedCardanoTransactionOutput) -> Vec<u8> {
        self.0.output_bytes.clone()
    }
}

fn encode_cardano_transaction_output_impl(
    output: &CxxSerializableTxOutput,
) -> Result<CxxEncodedCardanoTransactionOutputValue, Error> {
    let mut output_bytes = Vec::new();
    ciborium::ser::into_writer(&encode_tx_output(output), &mut output_bytes)
        .map_err(|_| Error::SerializationError)?;

    Ok(CxxEncodedCardanoTransactionOutputValue { output_bytes })
}

pub fn encode_cardano_transaction_output(
    output: &CxxSerializableTxOutput,
) -> Box<CxxEncodedCardanoTransactionOutputResult> {
    Box::new(CxxEncodedCardanoTransactionOutputResult::from(
        encode_cardano_transaction_output_impl(output),
    ))
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

fn encode_tx_output(output: &CxxSerializableTxOutput) -> CborValue {
    let addr = CborValue::Bytes(output.addr.clone());
    let amount = CborValue::Integer(output.amount.into());
    CborValue::Array(vec![addr, amount])
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
        let tx_hash: [u8; 32] = match &input_array[0] {
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

        let amount = match &output_array[1] {
            CborValue::Integer(i) => (*i).try_into().map_err(|_| Error::InvalidOutputFormat)?,
            _ => return Err(Error::InvalidOutputFormat),
        };

        outputs.push(CxxSerializableTxOutput { addr: addr_bytes, amount });
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
    Box::new(CxxDecodedCardanoTransactionResult::from(decode_cardano_transaction_impl(bytes)))
}

fn decode_cardano_transaction_impl(
    bytes: &[u8],
) -> Result<CxxDecodedCardanoTransactionValue, Error> {
    let cbor_value: CborValue = from_reader(bytes).map_err(|_| Error::CborDecodeError)?;

    let (body, raw_body) = extract_cardano_body(&cbor_value)?;

    let witness = extract_witness(&cbor_value)?;

    Ok({
        CxxDecodedCardanoTransactionValue {
            tx: CxxSerializableTx { body, witness },
            raw_body,
            raw_tx: bytes.to_vec(),
        }
    })
}

impl CxxDecodedCardanoTransaction {
    fn tx(self: &CxxDecodedCardanoTransaction) -> CxxSerializableTx {
        self.0.tx.clone()
    }

    fn raw_body(self: &CxxDecodedCardanoTransaction) -> Vec<u8> {
        self.0.raw_body.clone()
    }

    fn raw_tx(self: &CxxDecodedCardanoTransaction) -> Vec<u8> {
        self.0.raw_tx.clone()
    }
}

pub fn apply_signatures(
    bytes: &[u8],
    witnesses: CxxSerializableTxWitness,
) -> Box<CxxSignedCardanoTransactionResult> {
    Box::new(CxxSignedCardanoTransactionResult::from(apply_signatures_impl(bytes, witnesses)))
}

/// Applies signatures to an unsigned Cardano transaction
///
/// This function preserves existing witnesses and adds new signatures.
/// The vk_witness set follows the Cardano format: {0: [[pubkey1, signature1],
/// ...]}
fn apply_signatures_impl(
    bytes: &[u8],
    witnesses: CxxSerializableTxWitness,
) -> Result<CxxSignedCardanoTransactionValue, Error> {
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

    Ok(CxxSignedCardanoTransactionValue { signed_bytes })
}

impl CxxSignedCardanoTransaction {
    fn bytes(self: &CxxSignedCardanoTransaction) -> Vec<u8> {
        self.0.signed_bytes.clone()
    }
}
