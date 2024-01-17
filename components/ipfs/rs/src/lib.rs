// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

pub mod car_header;
pub mod block_decoder;

use crate::car_header::{decode_carv1_header, decode_carv2_header};
use crate::block_decoder::decode;

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = ipfs)]
mod ffi {

    extern "Rust" {
        fn decode_carv1_header(data: &CxxVector<u8>) -> CarV1HeaderResult;
        fn decode_carv2_header(data: &CxxVector<u8>) -> CarV2HeaderResult;
        fn decode(offset: usize, data: &CxxVector<u8>) -> BlockDecodeResult;
    }
    #[derive(Debug)]
    pub struct CarV1Header {
        pub version: u64,
        pub roots: Vec<String>,
    }

    #[derive(Debug)]
    pub struct Characteristics {
        data: [u64; 2],
    }

    #[derive(Debug)]
    pub struct CarV2Header {
        characteristics: Characteristics,
        data_offset: u64,
        data_size: u64,
        index_offset: u64,
//        data: CarV1Header,
    }

    #[derive(Debug)]
    pub struct ErrorData {
        error: String,
        error_code: u16,
    }

    #[derive(Debug)]
    pub struct CarV1HeaderResult {
        data: CarV1Header,
        error: ErrorData
    }

    #[derive(Debug)]
    pub struct CarV2HeaderResult {
        data: CarV2Header,
        error: ErrorData
    }

    #[derive(Debug)]
    pub struct BlockDecodeResult {
        data_offset: usize,
        cid: String,
        data: String,
        is_block_data: bool,
        error: ErrorData
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