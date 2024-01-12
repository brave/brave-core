// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use crate::ffi::{
    CarV1Header, CarV1HeaderResult, CarV2Header, CarV2HeaderResult, Characteristics, ErrorData,
};
use cxx::CxxVector;
use libipld_cbor::DagCborCodec;
use libipld_core::{codec::Codec, ipld::Ipld};
use thiserror::Error;

// const CARV2_HEADER_SIZE: usize = 40;
// const CARV2_PRAGMA_SIZE: usize = 11;
// const CARV2_PRAGMA: [u8; CARV2_PRAGMA_SIZE] = [
//     0x0a, // unit(10)
//     0xa1, // map(1)
//     0x67, // string(7)
//     0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, // "version"
//     0x02, // uint(2)
// ];

#[derive(Error, Debug)]
enum DecodeCarV1ErrorCode {
    #[error("Car header cbor codec error: `{reason}`")]
    HeaderCborDecodingError { reason: String },
    #[error("Car header expected cbor Map but got: `{reason}`")]
    HeaderCborMapExpectedError { reason: String },
    #[error("Car header, no roots found.")]
    HeaderRootsExpectedError,
    #[error("Car header version is unsupported: `{version}`")]
    HeaderVersionNotSupportedError { version: String },
    #[error("Internal error with code {0}")]
    InternalError(String, u16),
}
impl DecodeCarV1ErrorCode {
    fn to_u16(&self) -> u16 {
        match self {
            DecodeCarV1ErrorCode::HeaderCborDecodingError { .. } => 1u16,
            DecodeCarV1ErrorCode::HeaderCborMapExpectedError { .. } => 10u16,
            DecodeCarV1ErrorCode::HeaderRootsExpectedError => 20u16,
            DecodeCarV1ErrorCode::HeaderVersionNotSupportedError { .. } => 30u16,
            DecodeCarV1ErrorCode::InternalError(.., code) => *code,
        }
    }
}

/**
Takes vector of bytes (header DAG-CBOR block  with length `variant`) and parse them as CAR_V1 header
|--------------- Header ---------------|
[ varint block length | DAG-CBOR block ]

*/
pub fn decode_carv1_header(data: &CxxVector<u8>) -> CarV1HeaderResult {
    println!("decode_carv1_header: {:?}", data);
    let header: Ipld = DagCborCodec
        .decode(data.as_slice())
        .map_err(|e| {
            let error = DecodeCarV1ErrorCode::HeaderCborDecodingError { reason: format!("{e:?}") };
            CarV1HeaderResult {
                data: CarV1Header { version: 0, roots: Vec::new() },
                error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
            }
        })
        .unwrap();

    let header = if let Ipld::Map(map) = header {
        map
    } else {
        let error =
            DecodeCarV1ErrorCode::HeaderCborMapExpectedError { reason: format!("{:#?}", header) };
        return CarV1HeaderResult {
            data: CarV1Header { version: 0, roots: Vec::new() },
            error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
        };
    };

    let roots = match header.get("roots") {
        Some(Ipld::List(roots_ipld)) => {
            let mut roots = Vec::with_capacity(roots_ipld.len());
            for root in roots_ipld {
                if let Ipld::Link(cid) = root {
                    roots.push(*cid);
                } else {
                    let error = DecodeCarV1ErrorCode::InternalError(
                        format!(
                            "Car header, roots key elements expected cbor Link but got {:#?}",
                            root
                        ),
                        100u16,
                    );
                    return CarV1HeaderResult {
                        data: CarV1Header { version: 0, roots: Vec::new() },
                        error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
                    };
                }
            }
            Some(roots)
        }
        Some(ipld) => {
            let error = DecodeCarV1ErrorCode::InternalError(
                format!("Car header, roots key expected cbor List but got {:#?}", ipld),
                200u16,
            );
            return CarV1HeaderResult {
                data: CarV1Header { version: 0, roots: Vec::new() },
                error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
            };
        }
        // CARv2 does not have 'roots' key, so allow to not be specified
        None => None,
    };

    let version = match header.get("version") {
        Some(Ipld::Integer(int)) => *int as u64,
        Some(ipld) => {
            let error = DecodeCarV1ErrorCode::InternalError(
                format!("Car header, version key expected cbor Integer but got {:#?}", ipld),
                300u16,
            );
            return CarV1HeaderResult {
                data: CarV1Header { version: 0, roots: Vec::new() },
                error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
            };
        }
        None => {
            let error = DecodeCarV1ErrorCode::InternalError(
                format!(
                    "Car header, expected header key version, keys: {:?}",
                    header.keys().collect::<Vec<&String>>()
                ),
                400u16,
            );
            return CarV1HeaderResult {
                data: CarV1Header { version: 0, roots: Vec::new() },
                error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
            };
        }
    };

    if version != 1 {
        let error =
            DecodeCarV1ErrorCode::HeaderVersionNotSupportedError { version: version.to_string() };
        return CarV1HeaderResult {
            data: CarV1Header { version: version, roots: Vec::new() },
            error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
        };
    }

    if roots.is_none() {
        let error = DecodeCarV1ErrorCode::HeaderRootsExpectedError {};
        return CarV1HeaderResult {
            data: CarV1Header { version: version, roots: Vec::new() },
            error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
        };
    }

    let mut roots_str: Vec<String> = Vec::new();
    for el in &roots.unwrap() {
        roots_str.push(el.to_string());
    }

    CarV1HeaderResult {
        data: CarV1Header { version: version, roots: roots_str },
        error: ErrorData { error: String::new(), error_code: 0 },
    }
}

/**
Takes vector of bytes (header DAG-CBOR block  with length `variant`) and parse them as CAR_V1 header
|--------------- Header ---------------|
[ varint block length | DAG-CBOR block ]
*/
pub fn decode_carv2_header(data: &CxxVector<u8>) -> CarV2HeaderResult {
    println!("decode_carv2_header: {:?}", data);
    let mut roots_str: Vec<String> = Vec::new();
    roots_str.push("bla-bla".to_string());

    CarV2HeaderResult {
        data: CarV2Header {
            characteristics: Characteristics { data: [0, 1] },
            data_offset: 0,
            data_size: 0,
            index_offset: 0,
            data: CarV1Header { version: 1, roots: roots_str },
        },
        error: ErrorData { error: String::new(), error_code: 0 },
    }
}
