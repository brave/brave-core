// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use bech32::Error as Bech32Error;
use bech32::FromBase32;
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
#[cxx::bridge(namespace =  brave_wallet)]
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
