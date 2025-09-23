// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

//! Cardano Transaction Decoder
//!
//! This module provides functionality to decode Cardano transactions from CBOR format
//! and apply signatures to create signed transactions. It follows the Cardano
//! transaction structure: [body, witness_set, metadata].
//!
//! The decoder extracts transaction inputs and outputs from the transaction body,
//! while the signature application preserves existing witnesses and adds new ones.

use std::fmt;

use ciborium::{de::from_reader, Value as CborValue};

// Re-export types for external use
#[allow(unused_imports)]
use crate::ffi::CxxRestoredCardanoBody;
#[allow(unused_imports)]
use crate::ffi::CxxRestoredCardanoInput;
#[allow(unused_imports)]
use crate::ffi::CxxRestoredCardanoOutput;
#[allow(unused_imports)]
use crate::ffi::CxxWittness;

// Constants for Cardano transaction structure
const TRANSACTION_BODY_INDEX: u8 = 0;
const WITNESS_SET_INDEX: u8 = 1;
const INPUTS_KEY: u8 = 0;
const OUTPUTS_KEY: u8 = 1;
const WITNESS_KEY: u8 = 0;

/// Macro to implement common result type functionality
#[macro_export]
macro_rules! impl_result {
    ($t:ident, $r:ident, $f:ident) => {
        impl $r {
            /// Returns the error message if the result contains an error
            fn error_message(self: &$r) -> String {
                match &self.0 {
                    Err(e) => e.to_string(),
                    Ok(_) => String::new(),
                }
            }

            /// Returns true if the result is successful
            fn is_ok(self: &$r) -> bool {
                self.0.is_ok()
            }

            /// Unwraps the result, panicking if it contains an error
            fn unwrap(self: &mut $r) -> Box<$t> {
                Box::new((self.0.as_mut().unwrap()).take().unwrap())
            }
        }

        impl From<Result<$f, Error>> for $r {
            fn from(result: Result<$f, Error>) -> Self {
                match result {
                    Ok(v) => Self(Ok(Some($t(v)))),
                    Err(e) => Self(Err(e)),
                }
            }
        }
    };
}

/// Macro to implement error conversion
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
    /// Cardano transaction input
    #[derive(Clone)]
    struct CxxRestoredCardanoInput {
        tx_hash: Vec<u8>,
        index: u32,
    }

    /// Cardano transaction output
    #[derive(Clone)]
    struct CxxRestoredCardanoOutput {
        addr: Vec<u8>,
        amount: u64,
    }

    /// Cardano transaction body containing inputs, outputs, and raw bytes
    #[derive(Clone)]
    struct CxxRestoredCardanoBody {
        inputs: Vec<CxxRestoredCardanoInput>,
        outputs: Vec<CxxRestoredCardanoOutput>,
        raw_body: Vec<u8>,
    }

    /// Cardano witness containing public key and signature
    #[allow(dead_code)]
    struct CxxWittness {
        pubkey: Vec<u8>,
        signature: Vec<u8>,
    }

    extern "Rust" {
        type CxxDecodedCardanoTransaction;
        type CxxSignedCardanoTransaction;
        type CxxDecodedCardanoTransactionResult;
        type CxxSignedCardanoTransactionResult;

        /// Decodes a Cardano transaction from CBOR bytes
        fn decode_cardano_transaction(bytes: &[u8]) -> Box<CxxDecodedCardanoTransactionResult>;
        /// Applies signatures to an unsigned transaction
        fn apply_signatures(
            bytes: &[u8],
            witnesses: Vec<CxxWittness>,
        ) -> Box<CxxSignedCardanoTransactionResult>;

        // Result type methods for decoded transactions
        fn is_ok(self: &CxxDecodedCardanoTransactionResult) -> bool;
        fn error_message(self: &CxxDecodedCardanoTransactionResult) -> String;
        fn unwrap(
            self: &mut CxxDecodedCardanoTransactionResult,
        ) -> Box<CxxDecodedCardanoTransaction>;
        /// Returns the transaction body
        fn tx_body(self: &CxxDecodedCardanoTransaction) -> CxxRestoredCardanoBody;

        // Result type methods for signed transactions
        fn is_ok(self: &CxxSignedCardanoTransactionResult) -> bool;
        fn error_message(self: &CxxSignedCardanoTransactionResult) -> String;
        fn unwrap(self: &mut CxxSignedCardanoTransactionResult) -> Box<CxxSignedCardanoTransaction>;

        /// Returns the signed transaction bytes
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
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::CborDecodeError => write!(f, "Failed to decode CBOR data"),
            Error::InvalidTransactionFormat => write!(f, "Invalid transaction format"),
            Error::InvalidInputFormat => write!(f, "Invalid input format"),
            Error::InvalidOutputFormat => write!(f, "Invalid output format"),
            Error::SerializationError => write!(f, "Serialization error"),
        }
    }
}

/// Internal value wrapper for decoded transactions
struct CxxDecodedCardanoTransactionValue {
    body: CxxRestoredCardanoBody,
}

/// Internal value wrapper for signed transactions
struct CxxSignedCardanoTransactionValue {
    signed_bytes: Vec<u8>,
}

/// Public wrapper for decoded Cardano transactions
pub struct CxxDecodedCardanoTransaction(CxxDecodedCardanoTransactionValue);

/// Public wrapper for signed Cardano transactions
pub struct CxxSignedCardanoTransaction(CxxSignedCardanoTransactionValue);

/// Result type for decoded transaction operations
pub struct CxxDecodedCardanoTransactionResult(Result<Option<CxxDecodedCardanoTransaction>, Error>);

/// Result type for signed transaction operations
pub struct CxxSignedCardanoTransactionResult(Result<Option<CxxSignedCardanoTransaction>, Error>);

// Implement result functionality for both result types
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

/// Extracts the transaction body from a Cardano transaction CBOR
///
/// Cardano transactions follow the structure: [body, witness_set, metadata]
/// This function extracts and parses the body component.
fn extract_cardano_body(cbor_value: &CborValue) -> Result<CxxRestoredCardanoBody, Error> {
    let transaction_array = match cbor_value {
        CborValue::Array(arr) => arr,
        _ => return Err(Error::InvalidTransactionFormat),
    };

    if transaction_array.is_empty() {
        return Err(Error::InvalidTransactionFormat);
    }

    // Extract transaction body (first element)
    let body_value = &transaction_array[TRANSACTION_BODY_INDEX as usize];
    let body_map = match body_value {
        CborValue::Map(map) => map,
        _ => return Err(Error::InvalidTransactionFormat),
    };

    // Extract inputs and outputs
    let inputs = extract_inputs(body_map)?;
    let outputs = extract_outputs(body_map)?;

    // Serialize the body for raw bytes
    let mut raw_body = Vec::new();
    ciborium::ser::into_writer(body_value, &mut raw_body)
        .map_err(|_| Error::SerializationError)?;

    Ok(CxxRestoredCardanoBody {
        inputs,
        outputs,
        raw_body,
    })
}

/// Extracts transaction inputs from the transaction body
fn extract_inputs(
    body_map: &[(CborValue, CborValue)],
) -> Result<Vec<CxxRestoredCardanoInput>, Error> {
    let inputs_value = find_map_value(body_map, INPUTS_KEY)?;
    let inputs_array = match inputs_value {
        Some(CborValue::Array(arr)) => arr,
        Some(_) => return Err(Error::InvalidInputFormat),
        None => return Ok(Vec::new()), // No inputs
    };

    let mut inputs = Vec::with_capacity(inputs_array.len());
    for input_value in inputs_array {
        let input_array = match input_value {
            CborValue::Array(arr) if arr.len() >= 2 => arr,
            _ => return Err(Error::InvalidInputFormat),
        };

        // Extract transaction hash and index
        let tx_hash_bytes = match &input_array[0] {
            CborValue::Bytes(bytes) => bytes.clone(),
            _ => return Err(Error::InvalidInputFormat),
        };

        let index = match &input_array[1] {
            CborValue::Integer(i) => (*i).try_into().unwrap_or(0),
            _ => return Err(Error::InvalidInputFormat),
        };

        inputs.push(CxxRestoredCardanoInput {
            tx_hash: tx_hash_bytes,
            index,
        });
    }

    Ok(inputs)
}

/// Extracts transaction outputs from the transaction body
fn extract_outputs(
    body_map: &[(CborValue, CborValue)],
) -> Result<Vec<CxxRestoredCardanoOutput>, Error> {
    let outputs_value = find_map_value(body_map, OUTPUTS_KEY)?;
    let outputs_array = match outputs_value {
        Some(CborValue::Array(arr)) => arr,
        Some(_) => return Err(Error::InvalidOutputFormat),
        None => return Ok(Vec::new()), // No outputs
    };

    let mut outputs = Vec::with_capacity(outputs_array.len());
    for output_value in outputs_array {
        let output_array = match output_value {
            CborValue::Array(arr) if arr.len() >= 2 => arr,
            _ => return Err(Error::InvalidOutputFormat),
        };

        // Extract address and amount
        let addr_bytes = match &output_array[0] {
            CborValue::Bytes(bytes) => bytes.clone(),
            _ => return Err(Error::InvalidOutputFormat),
        };

        let amount = match &output_array[1] {
            CborValue::Integer(i) => (*i).try_into().unwrap_or(0),
            _ => return Err(Error::InvalidOutputFormat),
        };

        outputs.push(CxxRestoredCardanoOutput {
            addr: addr_bytes,
            amount,
        });
    }

    Ok(outputs)
}

/// Helper function to find a value in a CBOR map by integer key
fn find_map_value(
    map: &[(CborValue, CborValue)],
    key: u8,
) -> Result<Option<&CborValue>, Error> {
    Ok(map.iter()
        .find(|(k, _)| {
            matches!(k, CborValue::Integer(i) if *i == ciborium::value::Integer::from(key))
        })
        .map(|(_, v)| v))
}

/// Decodes a Cardano transaction from CBOR bytes
pub fn decode_cardano_transaction(bytes: &[u8]) -> Box<CxxDecodedCardanoTransactionResult> {
    let cbor_value: CborValue = match from_reader(bytes) {
        Ok(value) => value,
        Err(_) => {
            return Box::new(CxxDecodedCardanoTransactionResult::from(Err(Error::CborDecodeError)));
        }
    };

    let body = match extract_cardano_body(&cbor_value) {
        Ok(body) => body,
        Err(e) => {
            return Box::new(CxxDecodedCardanoTransactionResult::from(Err(e)));
        }
    };

    let decoded_tx = CxxDecodedCardanoTransactionValue { body };
    Box::new(CxxDecodedCardanoTransactionResult::from(Ok(decoded_tx)))
}

impl CxxDecodedCardanoTransaction {
    /// Returns the transaction body
    fn tx_body(self: &CxxDecodedCardanoTransaction) -> CxxRestoredCardanoBody {
        self.0.body.clone()
    }
}

/// Applies signatures to an unsigned Cardano transaction
///
/// This function preserves existing witnesses and adds new signatures.
/// The witness set follows the Cardano format: {0: [[pubkey1, signature1], ...]}
pub fn apply_signatures(
    bytes: &[u8],
    witnesses: Vec<CxxWittness>,
) -> Box<CxxSignedCardanoTransactionResult> {
    let mut cbor_value: CborValue = match from_reader(bytes) {
        Ok(value) => value,
        Err(_) => {
            return Box::new(CxxSignedCardanoTransactionResult::from(Err(
                Error::CborDecodeError,
            )));
        }
    };

    let transaction_array = match &mut cbor_value {
        CborValue::Array(arr) => arr,
        _ => {
            return Box::new(CxxSignedCardanoTransactionResult::from(Err(
                Error::InvalidTransactionFormat,
            )));
        }
    };

    if transaction_array.len() < 2 {
        return Box::new(CxxSignedCardanoTransactionResult::from(Err(
            Error::InvalidTransactionFormat,
        )));
    }

    // Extract existing witnesses
    let mut existing_witnesses = extract_existing_witnesses(
        &transaction_array[WITNESS_SET_INDEX as usize],
    );

    // Add new witnesses
    for witness in witnesses {
        existing_witnesses.push(CborValue::Array(vec![
            CborValue::Bytes(witness.pubkey),
            CborValue::Bytes(witness.signature),
        ]));
    }

    // Create new witness set map
    let witness_set_map = vec![(
        CborValue::Integer(ciborium::value::Integer::from(WITNESS_KEY)),
        CborValue::Array(existing_witnesses),
    )];

    // Replace the witness set
    transaction_array[WITNESS_SET_INDEX as usize] =
        CborValue::Map(witness_set_map);

    // Serialize the signed transaction
    let mut signed_bytes = Vec::new();
    if let Err(_) = ciborium::ser::into_writer(&cbor_value, &mut signed_bytes) {
        return Box::new(CxxSignedCardanoTransactionResult::from(Err(
            Error::SerializationError,
        )));
    }

    let signed_tx = CxxSignedCardanoTransactionValue { signed_bytes };
    Box::new(CxxSignedCardanoTransactionResult::from(Ok(signed_tx)))
}

/// Extracts existing witnesses from the witness set
fn extract_existing_witnesses(witness_set: &CborValue) -> Vec<CborValue> {
    if let CborValue::Map(witness_map) = witness_set {
        for (key, value) in witness_map {
            if let CborValue::Integer(i) = key {
                if *i == ciborium::value::Integer::from(WITNESS_KEY) {
                    if let CborValue::Array(witness_array) = value {
                        return witness_array.clone();
                    }
                    break;
                }
            }
        }
    }
    Vec::new()
}

impl CxxSignedCardanoTransaction {
    /// Returns the signed transaction bytes
    fn bytes(self: &CxxSignedCardanoTransaction) -> Vec<u8> {
        self.0.signed_bytes.clone()
    }
}