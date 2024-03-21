// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use crate::ffi::{BlockDecodeResult, ErrorData};
// use crate::unixfs_data::Data as UnixFs;
// use crate::unixfs_data::mod_Data::DataType as UnixFsType;
use cxx::CxxVector;
use libipld_cbor::DagCborCodec;
use libipld_core::cid::Cid;
use libipld_core::{codec::Codec, ipld::Ipld, raw::RawCodec};
use libipld_core::multihash::{Code, MultihashDigest};
use libipld_json::DagJsonCodec;
use libipld_pb::DagPbCodec;
use thiserror::Error;

//TODO move errors to separate module
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

const CODE_IDENTITY: u64 = 0x00;
const CODE_SHA2_256: u64 = 0x12;
const CODE_SHA2_512: u64 = 0x13;
const CODE_SHA3_256: u64 = 0x16;
const CODE_SHA3_512: u64 = 0x14;
const CODE_BLAKE2B_256: u64 = 0xb220;

// const DATA_TYPE_FILE: u8 = 0x02;
// const DATA_TYPE_OFFSET_POS: usize = 0x01;

// const DECODED_MAP_DATA_KEY: &str = "Data";
// const DECODED_MAP_LINKS_KEY: &str = "Links";

/**
Takes vector of block bytes (without prefix length of block `variant`, version etc) and decode it
*/
// pub fn decode_block_info(offset: usize, data: &CxxVector<u8>) -> BlockDecodeResult {
//     let mut block_decode_result: BlockDecodeResult = Default::default();
//     let (block_cid, decoded_raw_data, is_verified) = match decode(&data) {
//         Ok(res) => res,
//         Err(err) => {
//             block_decode_result.data_offset = offset;
//             block_decode_result.error = err;
//             return block_decode_result;
//         }
//     };

//     println!("decode_block_info #20 is_verified:{:?}", is_verified);
//     let mut is_content = false;
//     let decoded_str = match decoded_raw_data.as_ref().unwrap() {
//         Ipld::Map(_) => {
//             println!("decode_block_info #22: {:?}", decoded_raw_data.as_ref().unwrap());

//              if is_not_raw_file_data(decoded_raw_data.as_ref().unwrap(), &block_cid) {
//                 is_content = true;
//                  "".to_string()
//              } else {
//                 std::str::from_utf8(&DagJsonCodec.encode(decoded_raw_data.as_ref().unwrap()).unwrap())
//                 .unwrap()
//                 .to_string()
//              }
//         },
//         Ipld::Bytes(_) => {
//             is_content = true;
//             "".to_string()
//         },
//         _ => {
//             let error = BlockDecoderError::InternalError(
//                 format!("Could not format content: {:?}", decoded_raw_data.as_ref()),
//                 110u16,
//             );
//             block_decode_result.data_offset = offset;
//             block_decode_result.verified = is_verified.unwrap();
//             block_decode_result.error = ErrorData { error: error.to_string(), error_code: error.to_u16() };
//             return block_decode_result;
//         }
//     };

//     println!("decode_block_info #30 decoded_str:{:?}", decoded_str);
//     block_decode_result.data_offset = offset + data.len();
//     block_decode_result.cid = block_cid.unwrap().to_string();
//     block_decode_result.meta_data = decoded_str;
//     block_decode_result.verified = is_verified.unwrap();
//     block_decode_result.is_content = is_content;
//     block_decode_result.error = ErrorData { error: String::new(), error_code: 0 };
//     block_decode_result
// }

pub fn decode_block_content(offset: usize, data: &CxxVector<u8>) -> BlockDecodeResult {
    let mut block_decode_result: BlockDecodeResult = Default::default();
//    println!("decode_block_content #10 data:{:?}", data);
    let (block_cid, decoded_raw_data, is_verified) = match decode(&data) {
        Ok(res) => res,
        Err(err) => {            
            block_decode_result.data_offset = offset;
            block_decode_result.error = err;
            return block_decode_result;
        }
    };

//    println!("decode_block_content #20 is_verified:{:?} decoded_raw_data:{:?}", is_verified, decoded_raw_data.as_ref().unwrap());
    match decoded_raw_data.as_ref().unwrap() {
        Ipld::Map(_) => {
 //           println!("decode_block_content #22: {:?}", decoded_raw_data.as_ref().unwrap());

            //  if is_not_raw_file_data(decoded_raw_data.as_ref().unwrap(), &block_cid) {
            // } else {
                block_decode_result.meta_data = std::str::from_utf8(&DagJsonCodec.encode(decoded_raw_data.as_ref().unwrap()).unwrap())
                .unwrap()
                .to_string();
            //  }
        },
        Ipld::Bytes(ipld) => {
 //           println!("decode_block_content #23: {:?}", ipld);
            block_decode_result.is_content = true;
            block_decode_result.content_data = ipld.to_vec();
        },
        _ => {
//            println!("decode_block_content #24");
            let error =
                BlockDecoderError::InternalError("Could not format content.".to_string(), 110u16);
            block_decode_result.data_offset = offset;
            block_decode_result.verified = is_verified.unwrap();
            block_decode_result.error = ErrorData { error: error.to_string(), error_code: error.to_u16() };
            return block_decode_result;
        }
    };

    block_decode_result.data_offset = offset + data.len();
    block_decode_result.cid = block_cid.unwrap().to_string();    
    block_decode_result.verified = is_verified.unwrap();
    block_decode_result.error = ErrorData { error: String::new(), error_code: 0 };

//    println!("decode_block_content #40 is_verified:{:?} block_decode_result:{:?}", is_verified, &block_decode_result);
    block_decode_result
}

// fn is_not_raw_file_data(decoded_data: &Ipld, block_cid: &Option<Cid>) -> bool {
//     use quick_protobuf::{BytesReader, MessageRead};
//     println!("is_not_raw_file_data #10");
//     match decoded_data {
//         Ipld::Map(map) => {
//             println!("is_not_raw_file_data #50");
//             let data = match map.get(DECODED_MAP_DATA_KEY) {
//                 Some(item) => item,
//                 None => &Ipld::Null
//             };
//             let test_links_count = match map.get(DECODED_MAP_LINKS_KEY) {
//                 Some(item) => match item {
//                     Ipld::List(val) => val.len(),
//                     _ => 0,
//                 },
//                 None => 0
//             };

//             println!("is_not_raw_file_data #55 data:{:?} test_links_count:{:?}", data, test_links_count);
//             match data {
//                 Ipld::Bytes(data_list) => {
//                     println!("is_not_raw_file_data #67");
//                     if 0 != test_links_count {
//                         println!("is_not_raw_file_data #65_");
//                         return false;
//                     }
//                     let mut reader = BytesReader::from_bytes(data_list);
//                     let data_data = UnixFs::from_reader(&mut reader, data_list);
//                     println!("data_data.Type: {:?}",data_data.as_ref().unwrap().Type == UnixFsType::File);
//                     println!("data_data.Data: {:?}",data_data.as_ref().unwrap().Data.as_ref().unwrap());
                    
//                     println!("is_not_raw_file_data #66_ verified:{:?}", verify_data(block_cid, data_data.as_ref().unwrap().Data.as_ref().unwrap().to_vec().as_ref()));
//                     return data_list.len() > (DATA_TYPE_OFFSET_POS + 1) && data_list[DATA_TYPE_OFFSET_POS] == DATA_TYPE_FILE;
//                 },
//                 _ => {
//                     println!("is_not_raw_file_data #68");
//                     return false;
//                 },
//             }
//         },
//         _ => {
//             println!("is_not_raw_file_data #70");
//             return false;
//         }
//     };
// }

fn decode(data: &CxxVector<u8>) -> Result<(Option<Cid>, Option<Ipld>, Option<bool>), ErrorData> {
    let block_cid = Cid::try_from(data.as_slice()).map_err(|e| {
        let error =
            BlockDecoderError::InternalError(format!("Could not decode cid. Error: {}", e), 90u16);
        ErrorData { error: error.to_string(), error_code: error.to_u16() }
    })?;

    let block_cid_len = block_cid.to_bytes().len();
    let is_verified = verify_data(&Some(block_cid), &data.as_slice()[block_cid_len..data.len()].to_vec()).map_err(|e| {
        let error =
            BlockDecoderError::InternalError(format!("Could not verify block. Error: {}", e), 95u16);
        ErrorData { error: error.to_string(), error_code: error.to_u16() }
    })?;

//    println!("decode #10 is_verified:{:?}", is_verified);
    let decoded_raw_data: Ipld = match block_cid.codec() {
        CODEC_DAGCBOR => {
            DagCborCodec.decode(&data.as_slice()[block_cid_len..data.len()]).map_err(|e| {
                let error = BlockDecoderError::InternalError(
                    format!("Could not decode block. Error:{:?}", e),
                    100u16,
                );
                ErrorData { error: error.to_string(), error_code: error.to_u16() }
            })?
        }
        CODEC_DAGPB => {
            DagPbCodec.decode(&data.as_slice()[block_cid_len..data.len()]).map_err(|e| {
                let error = BlockDecoderError::InternalError(
                    format!("Could not decode block. Error:{:?}", e),
                    101u16,
                );
                ErrorData { error: error.to_string(), error_code: error.to_u16() }
            })?
        }
        CODEC_JSON => {
            DagJsonCodec.decode(&data.as_slice()[block_cid_len..data.len()]).map_err(|e| {
                let error = BlockDecoderError::InternalError(
                    format!("Could not decode block. Error:{:?}", e),
                    102u16,
                );
                ErrorData { error: error.to_string(), error_code: error.to_u16() }
            })?
        }
        CODEC_RAW => RawCodec.decode(&data.as_slice()[block_cid_len..data.len()]).map_err(|e| {
            let error = BlockDecoderError::InternalError(
                format!("Could not decode block. Error:{:?}", e),
                103u16,
            );
            ErrorData { error: error.to_string(), error_code: error.to_u16() }
        })?,
        _ => {
            let error =
                BlockDecoderError::InternalError("Unknown codec selected.".to_string(), 110u16);
            return Err(ErrorData { error: error.to_string(), error_code: error.to_u16() });
        }
    };

    Ok((Some(block_cid), Some(decoded_raw_data), Some(is_verified)))
}


fn verify_data(block_cid: &Option<Cid>, decoded: &Vec<u8>) -> Result<bool, ErrorData> {
    let block_digest = match block_cid.unwrap().hash().code() {
        CODE_IDENTITY => decoded.to_vec(),
        CODE_SHA2_256 => Code::Sha2_256.digest(&decoded).digest().to_vec(),
        CODE_SHA2_512 => Code::Sha2_512.digest(&decoded).digest().to_vec(),
        CODE_SHA3_256 => Code::Sha3_256.digest(&decoded).digest().to_vec(),
        CODE_SHA3_512 => Code::Sha3_512.digest(&decoded).digest().to_vec(),
        CODE_BLAKE2B_256 => Code::Blake2b256.digest(&decoded).digest().to_vec(),
        code => {
            let error =
                BlockDecoderError::InternalError(format!("Usupported hash code:{} cid:{}.", code, block_cid.unwrap().to_string()).to_string(), 110u16);
            return Err(ErrorData { error: error.to_string(), error_code: error.to_u16() })
        }
    };

    return Ok(block_digest == block_cid.unwrap().hash().digest());
}
