// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use bech32::Error as Bech32Error;
use bech32::FromBase32;
use bls_signatures::Serialize;
use ciborium::{de::from_reader, Value as CborValue};
use core::fmt;
use ffi::Bech32DecodeVariant;

#[macro_export]
macro_rules! impl_result {
    ($t:ident, $r:ident, $f:ident) => {
        impl $r {
            fn error_message(self: &$r) -> String {
                match &self.0 {
                    Err(e) => e.to_string(),
                    Ok(_) => "".to_string(),
                }
            }

            fn is_ok(self: &$r) -> bool {
                match &self.0 {
                    Err(_) => false,
                    Ok(_) => true,
                }
            }

            fn unwrap(self: &$r) -> &$t {
                self.0.as_ref().expect("Unhandled error before unwrap call")
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

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    enum Bech32DecodeVariant {
        Bech32,
        Bech32m,
    }

    extern "Rust" {
        type Bech32DecodeValue;
        type Bech32DecodeResult;

        fn bytes_are_curve25519_point(bytes: &[u8]) -> bool;

        fn data(self: &Bech32DecodeValue) -> Vec<u8>;
        fn hrp(self: &Bech32DecodeValue) -> String;
        fn variant(self: &Bech32DecodeValue) -> Bech32DecodeVariant;

        fn decode_bech32(input: &str) -> Box<Bech32DecodeResult>;
        fn is_ok(self: &Bech32DecodeResult) -> bool;
        fn error_message(self: &Bech32DecodeResult) -> String;
        fn unwrap(self: &Bech32DecodeResult) -> &Bech32DecodeValue;

        fn bls_private_key_to_public_key(private_key: &[u8]) -> Vec<u8>;
        fn bls_sign_message(private_key: &[u8], message_cid: &[u8]) -> Vec<u8>;

        fn validate_byron_address(bytes: &[u8]) -> bool;
    }
}

#[derive(Debug)]
pub enum Error {
    Bech32(Bech32Error),
    KeyLengthMismatch,
}

impl_error!(Bech32Error, Bech32);

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self {
            Error::Bech32(e) => write!(f, "Error: {}", e.to_string()),
            Error::KeyLengthMismatch => {
                write!(f, "Error: raw key bytes were not the expected length")
            }
        }
    }
}
pub struct Bech32Decoded {
    hrp: String,
    data: Vec<u8>,
    variant: Bech32DecodeVariant,
}

pub struct Bech32DecodeValue(Bech32Decoded);

struct Bech32DecodeResult(Result<Bech32DecodeValue, Error>);

impl_result!(Bech32DecodeValue, Bech32DecodeResult, Bech32Decoded);

impl From<bech32::Variant> for Bech32DecodeVariant {
    fn from(v: bech32::Variant) -> Bech32DecodeVariant {
        match v {
            bech32::Variant::Bech32m => Bech32DecodeVariant::Bech32m,
            bech32::Variant::Bech32 => Bech32DecodeVariant::Bech32,
        }
    }
}

fn bytes_are_curve25519_point(bytes: &[u8]) -> bool {
    match curve25519_dalek::edwards::CompressedEdwardsY::from_slice(bytes) {
        // If the y coordinate decompresses, it represents a curve point.
        Ok(point) => point.decompress().is_some(),
        // Creating the CompressedEdwardsY failed, so bytes does not represent
        // a curve point, probably the slice wasn't the expected size.
        Err(_) => false,
    }
}

fn decode_bech32(input: &str) -> Box<Bech32DecodeResult> {
    let decoded = bech32::decode(&input);
    match decoded {
        Ok(decoded_value) => {
            let (hrp, data, variant) = decoded_value;
            Box::new(Bech32DecodeResult::from(
                Vec::<u8>::from_base32(&data).map_err(Error::from).and_then(|as_u8| {
                    Ok(Bech32Decoded {
                        hrp: hrp,
                        data: as_u8,
                        variant: Bech32DecodeVariant::from(variant),
                    })
                }),
            ))
        }
        Err(e) => Box::new(Bech32DecodeResult::from(Err(Error::from(e)))),
    }
}

impl Bech32DecodeValue {
    fn hrp(self: &Bech32DecodeValue) -> String {
        self.0.hrp.clone()
    }
    fn data(self: &Bech32DecodeValue) -> Vec<u8> {
        self.0.data.clone()
    }
    fn variant(self: &Bech32DecodeValue) -> Bech32DecodeVariant {
        self.0.variant.clone()
    }
}

fn bls_private_key_to_public_key(private_key: &[u8]) -> Vec<u8> {
    return match bls_signatures::PrivateKey::from_bytes(private_key) {
        Ok(wrapped_private_key) => {
            let mut public_key = vec![];
            wrapped_private_key.public_key().write_bytes(&mut public_key).expect("preallocated");
            return public_key;
        }
        Err(_) => vec![],
    };
}

fn bls_sign_message(private_key: &[u8], message_cid: &[u8]) -> Vec<u8> {
    return match bls_signatures::PrivateKey::from_bytes(private_key) {
        Ok(sk) => sk.sign(&message_cid).as_bytes().to_vec(),
        Err(_) => vec![],
    };
}

const BYRON_ADDRESS_ROOT_SIZE: usize = 28;
const BYRON_CBOR_TAG_EMBEDDED: u64 = 24;

/// Validates raw bytes of a Byron address per the CDDL spec.
///
/// Expects the raw bytes obtained after Base58-decoding a Byron address string.
/// Checks CBOR structure, CRC32 integrity, payload shape (28-byte root,
/// attributes map, address type 0 or 2).
/// https://raw.githubusercontent.com/cardano-foundation/CIPs/master/CIP-0019/CIP-0019-byron-addresses.cddl
/// reference implementation: https://github.com/cardano-foundation/CIPs/blob/master/CIP-0019/CIP-0019-byron-addresses.cddl#L33
pub fn validate_byron_address(bytes: &[u8]) -> bool {
    validate_byron_address_impl(bytes).is_ok()
}

fn validate_byron_address_impl(bytes: &[u8]) -> Result<(), ()> {
    let cbor: CborValue = from_reader(bytes).map_err(|_| ())?;

    let arr = match &cbor {
        CborValue::Array(a) if a.len() == 2 => a,
        _ => Err(())?,
    };

    let payload_encoded = match &arr[0] {
        CborValue::Tag(BYRON_CBOR_TAG_EMBEDDED, inner) => match inner.as_ref() {
            CborValue::Bytes(b) => b,
            _ => Err(())?,
        },
        _ => Err(())?,
    };

    let stored_crc = match &arr[1] {
        CborValue::Integer(i) => {
            let v: u64 = (*i).try_into().map_err(|_| ())?;
            v as u32
        }
        _ => Err(())?,
    };

    // CRC32 of the BYRON_ADDRESS_PAYLOAD must match BYRON_ADDRESS last item.
    let mut h = crc32fast::Hasher::new();
    h.update(payload_encoded);
    if h.finalize() != stored_crc {
        Err(())?
    }

    let payload: CborValue = from_reader(payload_encoded.as_slice()).map_err(|_| ())?;
    // BYRON_ADDRESS_PAYLOAD is an array of 3 elements.
    let payload_arr = match &payload {
        CborValue::Array(a) if a.len() == 3 => a,
        _ => Err(())?,
    };

    // 0-element is 28 bytes hash.
    match &payload_arr[0] {
        CborValue::Bytes(b) if b.len() == BYRON_ADDRESS_ROOT_SIZE => {}
        _ => Err(())?,
    }

    // 1-element is BYRON_ADDRESS_ATTRIBUTES.
    let attrs = match &payload_arr[1] {
        CborValue::Map(m) => m.as_slice(),
        _ => Err(())?,
    };
    validate_byron_address_attributes(attrs)?;

    // 2-element is either 0 or 2 integer.
    let addr_type: u64 = match &payload_arr[2] {
        CborValue::Integer(i) => (*i).try_into().map_err(|_| ())?,
        _ => Err(())?,
    };
    if addr_type != 0 && addr_type != 2 {
        Err(())?
    }

    Ok(())
}

/// Validates BYRON_ADDRESS_ATTRIBUTES map: only keys 1 and 2 allowed.
/// Key 1: <<bytes .cbor BYRON_DERIVATION_PATH_CIPHERTEXT>> (inner CBOR =
/// bytes.size 28)
/// Key 2: <<uint32>> (inner CBOR = uint32)
fn validate_byron_address_attributes(attrs: &[(CborValue, CborValue)]) -> Result<(), ()> {
    for (k, v) in attrs {
        let key_u64: u64 = match k {
            CborValue::Integer(i) => (*i).try_into().map_err(|_| ())?,
            _ => Err(())?,
        };
        let inner: CborValue = match v {
            CborValue::Bytes(b) => from_reader(b.as_slice()).map_err(|_| ())?,
            _ => Err(())?,
        };
        match (key_u64, &inner) {
            (1, CborValue::Bytes(b)) if b.len() == BYRON_ADDRESS_ROOT_SIZE => {}
            (2, CborValue::Integer(i)) => {
                let _: u32 = (*i).try_into().map_err(|_| ())?;
            }
            _ => Err(())?,
        }
    }
    Ok(())
}
