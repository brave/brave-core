// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use std::{io, num};

use data_encoding::DecodeError;
use thiserror::Error;
use unsigned_varint::decode::Error as VarintError;

use super::{BLS_PUB_LEN, SECP_PUB_LEN};

/// Address error
#[derive(Debug, PartialEq, Eq, Error)]
pub enum Error {
    #[error("Unknown address network")]
    UnknownNetwork,
    #[error("Unknown address protocol")]
    UnknownProtocol,
    #[error("Invalid address payload")]
    InvalidPayload,
    #[error("Invalid address length")]
    InvalidLength,
    #[error("Invalid payload length: {0}")]
    InvalidPayloadLength(usize),
    #[error("Invalid BLS pub key length, wanted: {} got: {0}", BLS_PUB_LEN)]
    InvalidBLSLength(usize),
    #[error("Invalid SECP pub key length, wanted: {} got: {0}", SECP_PUB_LEN)]
    InvalidSECPLength(usize),
    #[error("Invalid address checksum")]
    InvalidChecksum,
    #[error("Decoding for address failed: {0}")]
    Base32Decoding(#[from] DecodeError),
    #[error("Cannot get id from non id address")]
    NonIDAddress,
    #[error("Cannot get delegated address from non delegate address")]
    NonDelegatedAddress,
}

impl From<num::ParseIntError> for Error {
    fn from(_: num::ParseIntError) -> Error {
        Error::InvalidPayload
    }
}

impl From<io::Error> for Error {
    fn from(_: io::Error) -> Error {
        Error::InvalidPayload
    }
}
impl From<VarintError> for Error {
    fn from(_: VarintError) -> Error {
        Error::InvalidPayload
    }
}
