use std::fmt::{Debug, Formatter};

// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use serde::de::value;
use {serde, serde_ipld_dagcbor};

use crate::{CodecProtocol, Error, RawBytes, CBOR, DAG_CBOR, IPLD_RAW};

#[derive(PartialEq, Eq, Clone, Default)]
pub struct IpldBlock {
    pub codec: u64,
    pub data: Vec<u8>,
}

impl Debug for IpldBlock {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        struct HexFmtHelper<'a>(&'a [u8]);
        impl Debug for HexFmtHelper<'_> {
            fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
                write!(f, "[")?;
                for byte in self.0 {
                    write!(f, "{:02x}", byte)?;
                }
                write!(f, "]")
            }
        }

        f.debug_struct("IpldBlock")
            .field("codec", &format_args!("{:x}", self.codec))
            .field("data", &HexFmtHelper(&self.data))
            .finish()
    }
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
            DAG_CBOR | CBOR => Ok(serde_ipld_dagcbor::from_slice(self.data.as_slice())?),
            _ => Err(Error {
                description: "unsupported protocol".to_string(),
                protocol: CodecProtocol::Unsupported,
            }),
        }
    }
    pub fn serialize<T: serde::Serialize + ?Sized>(codec: u64, value: &T) -> Result<Self, Error> {
        let data = match codec {
            IPLD_RAW => crate::raw::to_vec(value)?,
            DAG_CBOR | CBOR => crate::to_vec(value)?,
            _ => {
                return Err(Error {
                    description: "unsupported protocol".to_string(),
                    protocol: CodecProtocol::Unsupported,
                });
            }
        };
        Ok(IpldBlock { codec, data })
    }

    /// Serialize the object as CBOR. Links (cids) are NOT considered reachable by the FVM.
    pub fn serialize_cbor<T: serde::Serialize + ?Sized>(value: &T) -> Result<Option<Self>, Error> {
        Ok(Some(IpldBlock::serialize(CBOR, value)?))
    }

    /// Serialize the object as DagCBOR. Links (cids) _are_ considered reachable by the FVM.
    pub fn serialize_dag_cbor<T: serde::Serialize + ?Sized>(
        value: &T,
    ) -> Result<Option<Self>, Error> {
        Ok(Some(IpldBlock::serialize(DAG_CBOR, value)?))
    }
}

impl From<RawBytes> for Option<IpldBlock> {
    fn from(other: RawBytes) -> Self {
        (!other.is_empty()).then(|| IpldBlock {
            codec: CBOR,
            data: other.into(),
        })
    }
}

#[cfg(test)]
mod test {
    use super::IpldBlock;

    #[test]
    fn debug_hex() {
        assert_eq!(
            "IpldBlock { codec: 0, data: [] }",
            format!(
                "{:?}",
                IpldBlock {
                    codec: 0,
                    data: vec![]
                }
            )
        );
        assert_eq!(
            "IpldBlock { codec: 1, data: [00] }",
            format!(
                "{:?}",
                IpldBlock {
                    codec: 1,
                    data: vec![0]
                }
            )
        );
        assert_eq!(
            "IpldBlock { codec: ab, data: [0f] }",
            format!(
                "{:?}",
                IpldBlock {
                    codec: 0xab,
                    data: vec![15]
                }
            )
        );
        assert_eq!(
            "IpldBlock { codec: aaaa, data: [00010a10ff] }",
            format!(
                "{:?}",
                IpldBlock {
                    codec: 0xaaaa,
                    data: vec![0, 1, 10, 16, 255]
                }
            )
        );
    }
}
