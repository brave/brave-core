// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use crate::ffi::{
    BlockDecodeResult, ErrorData,
};
use cxx::CxxVector;
use libipld_cbor::DagCborCodec;
use libipld_pb::DagPbCodec;
use libipld_json::DagJsonCodec;
use libipld_core::{codec::Codec, ipld::Ipld, raw::RawCodec};
use thiserror::Error;
use libipld_core::cid::{Cid};

#[derive(Error, Debug)]
enum BlockDecoderError {
    #[error("Internal error with code {0}")]
    InternalError(String, u16),
}
impl BlockDecoderError {
    fn to_u16(&self) -> u16 {
        match self {
            BlockDecoderError::InternalError(.., code) => *code,
        }
    }
}

const CODEC_DAGCBOR: u64 = 113;
const CODEC_DAGPB: u64 = 112;
const CODEC_JSON: u64 = 297;
const CODEC_RAW: u64 = 85;

/**
Takes vector of block bytes (without prefix length of block `variant`, version etc) and decode it
*/
pub fn decode(offset: usize, data: &CxxVector<u8>) -> BlockDecodeResult {
    let block_cid = Cid::try_from(data.as_slice()).map_err(|e| {
        let error = BlockDecoderError::InternalError(format!("Could not decode cid. Error: {}", e), 90u16);
        return BlockDecodeResult {
            data_offset: offset,
            cid: String::new(),
            data:  String::new(),
            is_block_data: false,
            error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
        }
    }).unwrap();

    let block_cid_len = block_cid.to_bytes().len();
    println!("\n\n\nblock_cid: {:?} \nblock_cid_len:{:?} \ndata: {:?}", block_cid.to_string(), &block_cid_len, data);

    println!("Codec:{:?}", block_cid.codec());

    let result: Ipld = match block_cid.codec() {
        CODEC_DAGCBOR => {
            DagCborCodec
            .decode(&data.as_slice()[block_cid_len .. data.len()])
            .map_err(|e| {
                let error = BlockDecoderError::InternalError(format!("Could not decode block. Error:{:?}", e), 100u16);
                return BlockDecodeResult {
                    data_offset: offset,
                    cid: String::new(),
                    data:  String::new(),
                    is_block_data: false,
                    error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
                }
            })
            .unwrap()
        },
        CODEC_DAGPB => {
            DagPbCodec
            .decode(&data.as_slice()[block_cid_len .. data.len()])
            .map_err(|e| {
                println!("Tried to decode:{:?}", &data.as_slice()[block_cid_len .. data.len()]);
                let error = BlockDecoderError::InternalError(format!("Could not decode block. Error:{:?}", e), 101u16);
                return BlockDecodeResult {
                    data_offset: offset,
                    cid: String::new(),
                    data:  String::new(),
                    is_block_data: false,
                    error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
                }
            })
            .unwrap()
        },
        CODEC_JSON => {
            DagJsonCodec
            .decode(&data.as_slice()[block_cid_len .. data.len()])
            .map_err(|e| {
                let error = BlockDecoderError::InternalError(format!("Could not decode block. Error:{:?}", e), 102u16);
                return BlockDecodeResult {
                    data_offset: offset,
                    cid: String::new(),
                    data:  String::new(),
                    is_block_data: false,
                    error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
                }
            })
            .unwrap()
        },
        CODEC_RAW => {
            RawCodec
            .decode(&data.as_slice()[block_cid_len .. data.len()])
            .map_err(|e| {
                let error = BlockDecoderError::InternalError(format!("Could not decode block. Error:{:?}", e), 103u16);
                return BlockDecodeResult {
                    data_offset: offset,
                    cid: String::new(),
                    data:  String::new(),
                    is_block_data: false,
                    error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
                }
            })
            .unwrap()
        }
        _ => {
            let error = BlockDecoderError::InternalError("Unknown codec selected.".to_string(), 110u16);
            return BlockDecodeResult {
                data_offset: offset,
                cid: String::new(),
                data:  String::new(),
                is_block_data: false,
                error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
            }
        }
    };

    let mut is_block_data = false;
    let data = match &result {
        // Ipld::List(ipld) => {
        //     println!("List:{:?}", ipld);
        // },
        // Ipld::Link(ipld) => {
        //     println!("Link:{:?}", ipld);
        // },
        Ipld::Map(ipld) => {
            format!("{:?}",ipld)
        },
        Ipld::Bytes(_) => {
            is_block_data = true;
            "".to_string()
        },
        _ => {
            let error = BlockDecoderError::InternalError(format!("Could not format content: {:?}", result), 110u16);
            return BlockDecodeResult {
                data_offset: offset,
                cid: String::new(),
                data:  String::new(),
                is_block_data: false,
                error: ErrorData { error: error.to_string(), error_code: error.to_u16() },
            }
        }
    };

    println!("Decoded: {:?}", data);

    BlockDecodeResult {
        data_offset: offset + data.len(),
        cid: block_cid.to_string(),
        data: data,
        is_block_data: is_block_data,
        error: ErrorData { error: String::new(), error_code: 0 },
    }
}
