// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use crate::message::MessageAPI;
use blake2b_simd::Params;
use bls_signatures::Serialize;
use core::{array::TryFromSliceError, num::ParseIntError};
use fvm_ipld_encoding::DAG_CBOR;
use fvm_ipld_encoding::to_vec;
use fvm_shared::address::Network;
use fvm_shared::address::set_current_network;
use fvm_shared::crypto::signature::Signature;
use fvm_shared::message::Message as UnsignedMessage;
use libsecp256k1::util::{SECRET_KEY_SIZE, SIGNATURE_SIZE};
use multihash_codetable::{Code, MultihashDigest};
use thiserror::Error;

pub struct PrivateKey(pub [u8; SECRET_KEY_SIZE]);

/// Filecoin Signer Error
/// https://github.com/Zondax/filecoin-signing-tools/blob/222362ae38a1ccd8523ea026d5298d4718b43e2b/signer/src/error.rs#L7
#[derive(Error, Debug)]
pub enum SignerError {
    /// Secp256k1 error
    #[error("secp256k1 error")]
    Secp256k1(#[from] libsecp256k1::Error),
    /// InvalidBigInt error
    #[error("InvalidBigInt error")]
    InvalidBigInt(#[from] num_bigint_chainsafe::ParseBigIntError),
    /// Generic error message
    #[error("Error: `{0}`")]
    GenericString(String),
    /// Not able to parse integer
    #[error("Cannot parse integer")]
    ParseIntError(#[from] ParseIntError),
    /// BLS error
    #[error("BLS error | {0}")]
    BLS(#[from] bls_signatures::Error),
    /// BLS error
    #[error("Couldn't convert from slice")]
    TryFromSlice(#[from] TryFromSliceError),
    /// Base64 decode Error
    #[error("Base64 decode error | {0}")]
    DecodeError(#[from] base64::DecodeError),

    #[error("Marshall error | {0}")]
    FvmSharedEncodingError(#[from] fvm_ipld_encoding::Error),

    // CID error
    #[error("Cannot read CID from string | {0}")]
    CidError(#[from] cid::Error),
}

/// https://github.com/Zondax/filecoin-signing-tools/blob/222362ae38a1ccd8523ea026d5298d4718b43e2b/signer/src/utils.rs#L6
pub fn blake2b_256(ingest: &[u8]) -> [u8; 32] {
    let digest = Params::new().hash_length(32).to_state().update(ingest).finalize();

    let mut ret = [0u8; 32];
    ret.clone_from_slice(digest.as_bytes());
    ret
}

/// https://github.com/Zondax/filecoin-signing-tools/blob/222362ae38a1ccd8523ea026d5298d4718b43e2b/signer/src/lib.rs#L308
fn transaction_sign_secp56k1_raw(
    message: &UnsignedMessage,
    private_key: &PrivateKey,
) -> Result<Signature, SignerError> {
    let secret_key = libsecp256k1::SecretKey::parse_slice(&private_key.0)?;
    let message_ser = to_vec(message)?;
    let hash = Code::Blake2b256.digest(&message_ser);
    let message_cid = cid::Cid::new_v1(DAG_CBOR, hash);
    let message_digest = libsecp256k1::Message::parse_slice(&blake2b_256(&message_cid.to_bytes()))?;

    let (signature_rs, recovery_id) = libsecp256k1::sign(&message_digest, &secret_key);

    let mut sig = [0; SIGNATURE_SIZE + 1];
    sig[..SIGNATURE_SIZE].copy_from_slice(&signature_rs.serialize().to_vec());
    sig[SIGNATURE_SIZE] = recovery_id.serialize();

    let signature = Signature::new_secp256k1(sig.to_vec());

    Ok(signature)
}

/// https://github.com/Zondax/filecoin-signing-tools/blob/222362ae38a1ccd8523ea026d5298d4718b43e2b/signer/src/lib.rs#L330
fn transaction_sign_bls_raw(
    message: &UnsignedMessage,
    private_key: &PrivateKey,
) -> Result<Signature, SignerError> {
    let sk = bls_signatures::PrivateKey::from_bytes(&private_key.0)?;
    let message_ser = to_vec(message)?;
    let hash = Code::Blake2b256.digest(&message_ser);
    let message_cid = cid::Cid::new_v1(DAG_CBOR, hash);
    let sig = sk.sign(&message_cid.to_bytes());
    let signature = Signature::new_bls(sig.as_bytes());

    Ok(signature)
}

/// Sign a transaction and return a raw signature (RSV format).
///
/// # Arguments
///
/// * `unsigned_message_api` - an unsigned filecoin message
/// * `private_key` - a `PrivateKey`
///
/// https://github.com/Zondax/filecoin-signing-tools/blob/222362ae38a1ccd8523ea026d5298d4718b43e2b/signer-npm/src/lib.rs#L342
pub fn transaction_sign_raw(
    message: &UnsignedMessage,
    private_key: &PrivateKey,
) -> Result<Signature, SignerError> {
    // the `from` address protocol let us know which signing scheme to use
    let signature = match message.from.protocol() {
        fvm_shared::address::Protocol::Secp256k1 => {
            transaction_sign_secp56k1_raw(message, private_key)?
        }
        fvm_shared::address::Protocol::BLS => transaction_sign_bls_raw(message, private_key)?,
        _ => {
            return Err(SignerError::GenericString("Unknown signing protocol".to_string()));
        }
    };

    Ok(signature)
}

pub fn transaction_sign(is_mainnet: bool, transaction: &str, private_key: &[u8]) -> String {
    if is_mainnet {
        set_current_network(Network::Mainnet);
    } else {
        set_current_network(Network::Testnet);
    }
    let mut de = serde_json::Deserializer::from_str(transaction);
    let mut sk = PrivateKey([0; SECRET_KEY_SIZE]);
    sk.0.copy_from_slice(&private_key[..SECRET_KEY_SIZE]);
    let message_user_api = MessageAPI::deserialize(&mut de);
    if let Ok(message_user_api) = message_user_api {
        let raw_signature = transaction_sign_raw(&message_user_api, &sk);
        if let Ok(raw_signature) = raw_signature {
            return base64::encode(raw_signature.bytes());
        }
    }
    // Empty string is returned as an error cause this code is executed from cxx
    String::new()
}
