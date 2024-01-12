// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use thiserror::Error;
use libipld_core::{
//    cid::Cid,
    codec::Codec,
};
//use std::io::Cursor;
use libipld_cbor::DagCborCodec;
use libipld_core::{
    //  codec::{
    //     Decode, 
    //     //Encode
    // },
       ipld::Ipld,
//      raw_value::RawValue,
};
 use cxx::{
//    let_cxx_string, 
//    CxxString, 
    CxxVector
};

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = ipfs)]
mod ffi {

    extern "Rust" {
        fn decode_carv1_header(data: &CxxVector<u8>) -> CarV1HeaderResult;
//        fn dagpb_decode(data: &CxxVector<u8>) -> Vec<u8>;
    }
    #[derive(Debug)]
    pub struct CarV1Header {
        pub version: u64,
        pub roots: Vec<String>,
    }



    #[derive(Debug)]
    pub struct CarV1HeaderResult {
        data: CarV1Header,
        error: String,
        error_code: u16,
    }
}

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
    fn to_u16(&self) -> u16{
        match self {
            DecodeCarV1ErrorCode::HeaderCborDecodingError { .. } => 1u16,
            DecodeCarV1ErrorCode::HeaderCborMapExpectedError { .. } => 10u16,
            DecodeCarV1ErrorCode::HeaderRootsExpectedError => 20u16,
            DecodeCarV1ErrorCode::HeaderVersionNotSupportedError { .. } => 30u16,
            DecodeCarV1ErrorCode::InternalError(.., code) => *code,
        }
    }
}
// pub fn dagpb_decode(data: &CxxVector<u8>) -> Vec<u8> {
//     let ipld: Ipld = IpldCodec::DagPb.decode(&data.as_slice()).unwrap();

// }

// pub fn is_car_valid(path_str: &str) -> bool {

//     let input = cbor_seq();
//     let mut r = Cursor::new(&input);
//     let raw: RawValue<DagCborCodec> = Decode::decode(DagCborCodec, &mut r).unwrap();
//     assert_eq!(r.position(), 1);
//     assert_eq!(raw.as_ref(), &input[0..1]);

//     // let input =
//     //     "a163666f6fd82a582300122031c3d57080d8463a3c63b2923df5a1d40ad7a73eae5a14af584213e5f504ac33";
//     // //let input = hex::decode(input).unwrap();

//     // let ipld: Ipld = DagCborCodec.decode(&input).unwrap();
//     // let bytes = DagCborCodec.encode(&ipld).unwrap().to_vec();

//     return path_str.is_empty();
// }

use crate::ffi::{ CarV1Header, CarV1HeaderResult };

fn decode_carv1_header(data: &CxxVector<u8>) -> CarV1HeaderResult {
//    println!("{:?}", data);
    let header: Ipld = DagCborCodec.decode(data.as_slice()).map_err(|e| {
        let error = DecodeCarV1ErrorCode::HeaderCborDecodingError{ reason: format!("{e:?}") };
        CarV1HeaderResult {
          data: CarV1Header{ version: 0, roots: Vec::new()},
          error: error.to_string(),
          error_code: error.to_u16(),
        }
    }).unwrap();

    let header = if let Ipld::Map(map) = header {
        map
    } else {
        let error = DecodeCarV1ErrorCode::HeaderCborMapExpectedError{ reason: format!("{:#?}", header) };
        return CarV1HeaderResult {
            data: CarV1Header{ version: 0, roots: Vec::new()},
            error: error.to_string(),
            error_code: error.to_u16(),
          };
    };

    let roots = match header.get("roots") {
        Some(Ipld::List(roots_ipld)) => {
            let mut roots = Vec::with_capacity(roots_ipld.len());
            for root in roots_ipld {
                if let Ipld::Link(cid) = root {
                    roots.push(*cid);
                } else {
                    let error = DecodeCarV1ErrorCode::InternalError(format!("Car header, roots key elements expected cbor Link but got {:#?}", root), 100u16);
                    return CarV1HeaderResult {
                        data: CarV1Header{ version: 0, roots: Vec::new()},
                        error: error.to_string(),
                        error_code: error.to_u16(),
                    };
                }
            }
            Some(roots)
        }
        Some(ipld) => {
            let error = DecodeCarV1ErrorCode::InternalError(format!("Car header, roots key expected cbor List but got {:#?}", ipld), 200u16);
            return CarV1HeaderResult {
                data: CarV1Header{ version: 0, roots: Vec::new()},
                error: error.to_string(),
                error_code: error.to_u16(),
            };
        }
        // CARv2 does not have 'roots' key, so allow to not be specified
        None => None,
    };

    let version = match header.get("version") {
        Some(Ipld::Integer(int)) => *int as u64,
        Some(ipld) => {
            let error = DecodeCarV1ErrorCode::InternalError(format!("Car header, version key expected cbor Integer but got {:#?}", ipld), 300u16);
            return CarV1HeaderResult {
                data: CarV1Header{ version: 0, roots: Vec::new()},
                error: error.to_string(),
                error_code: error.to_u16(),
            };
        }
        None => {
            let error = DecodeCarV1ErrorCode::InternalError(format!("Car header, expected header key version, keys: {:?}", header.keys().collect::<Vec<&String>>()), 400u16);
            return CarV1HeaderResult {
                data: CarV1Header{ version: 0, roots: Vec::new()},
                error: error.to_string(),
                error_code: error.to_u16(),
            };
        }
    };

    if version != 1 {
        let error = DecodeCarV1ErrorCode::HeaderVersionNotSupportedError{ version: version.to_string() };
        return CarV1HeaderResult {
            data: CarV1Header{ version: version, roots: Vec::new()},
            error: error.to_string(),
            error_code: error.to_u16(),
        };
    }

    if roots.is_none() {
        let error = DecodeCarV1ErrorCode::HeaderRootsExpectedError{};
        return CarV1HeaderResult {
            data: CarV1Header{ version: version, roots: Vec::new()},
            error: error.to_string(),
            error_code: error.to_u16(),
        };
    }

    let mut roots_str: Vec<String> = Vec::new();
    for el in &roots.unwrap() {
        roots_str.push(el.to_string());
    }

    CarV1HeaderResult {
        data: CarV1Header{ version: version, roots: roots_str},
        error: String::new(),
        error_code: 0
    }
}

// fn cbor_seq() -> Vec<u8> {
//     let mut buf = Vec::new();
//     1u8.encode(DagCborCodec, &mut buf).unwrap();
//     (u16::MAX as u64 + 1)
//         .encode(DagCborCodec, &mut buf)
//         .unwrap();
//     vec![String::from("foo")]
//         .encode(DagCborCodec, &mut buf)
//         .unwrap();
//     buf.extend_from_slice(&[0xff, 0xff, 0xff, 0xff]);
//     buf
// }