// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use serde::de::value;
use {serde, serde_ipld_dagcbor};

use crate::{CodecProtocol, Error, RawBytes, DAG_CBOR, IPLD_RAW};

#[derive(Debug, PartialEq, Eq, Clone, Default)]
pub struct IpldBlock {
    pub codec: u64,
    pub data: Vec<u8>,
}

impl IpldBlock {
    pub fn deserialize<'de, T>(&'de self) -> Result<T, Error>
    where
        T: serde::Deserialize<'de>,
    {
        match self.codec {
            IPLD_RAW => T::deserialize(value::BytesDeserializer::<value::Error>::new(
                self.data.as_slice(),
            ))
            .map_err(|e| Error {
                description: e.to_string(),
                protocol: CodecProtocol::Raw,
            }),
            DAG_CBOR => Ok(serde_ipld_dagcbor::from_slice(self.data.as_slice())?),
            _ => Err(Error {
                description: "unsupported protocol".to_string(),
                protocol: CodecProtocol::Unsupported,
            }),
        }
    }
    pub fn serialize<T: serde::Serialize + ?Sized>(codec: u64, value: &T) -> Result<Self, Error> {
        let data = match codec {
            IPLD_RAW => crate::raw::to_vec(value)?,
            DAG_CBOR => crate::to_vec(value)?,
            _ => {
                return Err(Error {
                    description: "unsupported protocol".to_string(),
                    protocol: CodecProtocol::Unsupported,
                });
            }
        };
        Ok(IpldBlock { codec, data })
    }
    pub fn serialize_cbor<T: serde::Serialize + ?Sized>(value: &T) -> Result<Option<Self>, Error> {
        Ok(Some(IpldBlock::serialize(DAG_CBOR, value)?))
    }
}

impl From<RawBytes> for Option<IpldBlock> {
    fn from(other: RawBytes) -> Self {
        (!other.is_empty()).then(|| IpldBlock {
            codec: DAG_CBOR,
            data: other.into(),
        })
    }
}
