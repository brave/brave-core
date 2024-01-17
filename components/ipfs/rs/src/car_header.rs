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

const CARV1_VERSION: u64 = 1; 
const CARV2_VERSION: u64 = 2; 

#[derive(Error, Debug)]
enum DecodeCarHeaderError {
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
impl DecodeCarHeaderError {
    fn to_u16(&self) -> u16 {
        match self {
            DecodeCarHeaderError::HeaderCborDecodingError { .. } => 1u16,
            DecodeCarHeaderError::HeaderCborMapExpectedError { .. } => 10u16,
            DecodeCarHeaderError::HeaderRootsExpectedError => 20u16,
            DecodeCarHeaderError::HeaderVersionNotSupportedError { .. } => 30u16,
            DecodeCarHeaderError::InternalError(.., code) => *code,
        }
    }
}

/**
Takes vector of DAG-CBOR block bytes (without prefix length of block `variant`) and parse them as CAR_V1 header
|--------------- Header ---------------|
[ varint block length | DAG-CBOR block ]

*/
pub fn decode_carv1_header(data: &CxxVector<u8>) -> CarV1HeaderResult {
    decode_carv1_header_impl(data.as_slice())
}

fn decode_carv1_header_impl(data: &[u8]) -> CarV1HeaderResult {
    println!("decode_carv1_header: {:?}", data);
    let header: Ipld = DagCborCodec
        .decode(data)
        .map_err(|e| {
            let error = DecodeCarHeaderError::HeaderCborDecodingError { reason: format!("{e:?}") };
            CarV1HeaderResult {
                data: CarV1Header { version: 0, roots: Vec::new() },
                error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
            }
        })
        .unwrap();
    println!("header: {:?}", header);
    let header = if let Ipld::Map(map) = header {
        map
    } else {
        let error =
            DecodeCarHeaderError::HeaderCborMapExpectedError { reason: format!("{:#?}", header) };
        return CarV1HeaderResult {
            data: CarV1Header { version: 0, roots: Vec::new() },
            error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
        };
    };
    println!("header:{:?}", header);
    let roots = match header.get("roots") {
        Some(Ipld::List(roots_ipld)) => {
            let mut roots = Vec::with_capacity(roots_ipld.len());
            for root in roots_ipld {
                if let Ipld::Link(cid) = root {
                    roots.push(*cid);
                } else {
                    let error = DecodeCarHeaderError::InternalError(
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
            let error = DecodeCarHeaderError::InternalError(
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
            let error = DecodeCarHeaderError::InternalError(
                format!("Car header, version key expected cbor Integer but got {:#?}", ipld),
                300u16,
            );
            return CarV1HeaderResult {
                data: CarV1Header { version: 0, roots: Vec::new() },
                error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
            };
        }
        None => {
            let error = DecodeCarHeaderError::InternalError(
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

    if version != CARV1_VERSION {
        let error =
            DecodeCarHeaderError::HeaderVersionNotSupportedError { version: version.to_string() };
        return CarV1HeaderResult {
            data: CarV1Header { version: version, roots: Vec::new() },
            error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
        };
    }

    if roots.is_none() {
        let error = DecodeCarHeaderError::HeaderRootsExpectedError {};
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
|--------------- Header -------------------|
[ pragma 11 bytes | CAR_V2 header 40 bytes ]
*/
pub fn decode_carv2_header(data: &CxxVector<u8>) -> CarV2HeaderResult {
    let pragma_result = decode_carv1_header_impl(&data.as_slice()[1..11]);
    print!("P1: {:?}", pragma_result);
    let error =
            DecodeCarHeaderError::HeaderVersionNotSupportedError { version: pragma_result.data.version.to_string() };
    if pragma_result.data.version != CARV2_VERSION && pragma_result.error.error_code != error.to_u16()  {
        return CarV2HeaderResult {
            data: CarV2Header { characteristics: Characteristics { data: [0, 0] }, data_offset: 0, data_size: 0, index_offset: 0 },
            error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
        };
    }

    let characteristics0 = u64::from_be_bytes(data.as_slice()[11..19].try_into().unwrap());
    let characteristics1 = u64::from_be_bytes(data.as_slice()[19..27].try_into().unwrap());

    let data_offset = u64::from_le_bytes(data.as_slice()[27..35].try_into().unwrap());
    let data_size = u64::from_le_bytes(data.as_slice()[35..43].try_into().unwrap());
    let index_offset = u64::from_le_bytes(data.as_slice()[43..51].try_into().unwrap());
    println!("decode_carv2_header: {:?}", data);
    println!("characteristics0: {} characteristics1: {}, data_offset: {}, data_size: {}, index_offset: {}", characteristics0, characteristics1, data_offset, data_size, index_offset);

    CarV2HeaderResult {
        data: CarV2Header {
            characteristics: Characteristics { data: [characteristics0, characteristics1] },
            data_offset: data_offset,
            data_size: data_size,
            index_offset: index_offset,
        },
        error: ErrorData { error: String::new(), error_code: 0 },
    }
}
