//! IPLD Codecs.
#[cfg(feature = "dag-cbor")]
use crate::cbor::DagCborCodec;
use crate::cid::Cid;
use crate::codec::{Codec, Decode, Encode, References};
use crate::error::{Result, UnsupportedCodec};
use crate::ipld::Ipld;
#[cfg(feature = "dag-json")]
use crate::json::DagJsonCodec;
#[cfg(feature = "dag-pb")]
use crate::pb::DagPbCodec;
use crate::raw::RawCodec;
use core::convert::TryFrom;
use std::io::{Read, Seek, Write};

/// Default codecs.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum IpldCodec {
    /// Raw codec.
    Raw,
    /// Cbor codec.
    #[cfg(feature = "dag-cbor")]
    DagCbor,
    /// Json codec.
    #[cfg(feature = "dag-json")]
    DagJson,
    /// Protobuf codec.
    #[cfg(feature = "dag-pb")]
    DagPb,
}

impl TryFrom<u64> for IpldCodec {
    type Error = UnsupportedCodec;

    fn try_from(ccode: u64) -> core::result::Result<Self, Self::Error> {
        Ok(match ccode {
            0x55 => Self::Raw,
            #[cfg(feature = "dag-cbor")]
            0x71 => Self::DagCbor,
            #[cfg(feature = "dag-json")]
            0x0129 => Self::DagJson,
            #[cfg(feature = "dag-pb")]
            0x70 => Self::DagPb,
            _ => return Err(UnsupportedCodec(ccode)),
        })
    }
}

impl From<IpldCodec> for u64 {
    fn from(mc: IpldCodec) -> Self {
        match mc {
            IpldCodec::Raw => 0x55,
            #[cfg(feature = "dag-cbor")]
            IpldCodec::DagCbor => 0x71,
            #[cfg(feature = "dag-json")]
            IpldCodec::DagJson => 0x0129,
            #[cfg(feature = "dag-pb")]
            IpldCodec::DagPb => 0x70,
        }
    }
}

impl From<RawCodec> for IpldCodec {
    fn from(_: RawCodec) -> Self {
        Self::Raw
    }
}

#[cfg(feature = "dag-cbor")]
impl From<DagCborCodec> for IpldCodec {
    fn from(_: DagCborCodec) -> Self {
        Self::DagCbor
    }
}

#[cfg(feature = "dag-cbor")]
impl From<IpldCodec> for DagCborCodec {
    fn from(_: IpldCodec) -> Self {
        Self
    }
}

#[cfg(feature = "dag-json")]
impl From<DagJsonCodec> for IpldCodec {
    fn from(_: DagJsonCodec) -> Self {
        Self::DagJson
    }
}

#[cfg(feature = "dag-json")]
impl From<IpldCodec> for DagJsonCodec {
    fn from(_: IpldCodec) -> Self {
        Self
    }
}

#[cfg(feature = "dag-pb")]
impl From<DagPbCodec> for IpldCodec {
    fn from(_: DagPbCodec) -> Self {
        Self::DagPb
    }
}

#[cfg(feature = "dag-pb")]
impl From<IpldCodec> for DagPbCodec {
    fn from(_: IpldCodec) -> Self {
        Self
    }
}

impl Codec for IpldCodec {}

impl Encode<IpldCodec> for Ipld {
    fn encode<W: Write>(&self, c: IpldCodec, w: &mut W) -> Result<()> {
        match c {
            IpldCodec::Raw => self.encode(RawCodec, w)?,
            #[cfg(feature = "dag-cbor")]
            IpldCodec::DagCbor => self.encode(DagCborCodec, w)?,
            #[cfg(feature = "dag-json")]
            IpldCodec::DagJson => self.encode(DagJsonCodec, w)?,
            #[cfg(feature = "dag-pb")]
            IpldCodec::DagPb => self.encode(DagPbCodec, w)?,
        };
        Ok(())
    }
}

impl Decode<IpldCodec> for Ipld {
    fn decode<R: Read + Seek>(c: IpldCodec, r: &mut R) -> Result<Self> {
        Ok(match c {
            IpldCodec::Raw => Self::decode(RawCodec, r)?,
            #[cfg(feature = "dag-cbor")]
            IpldCodec::DagCbor => Self::decode(DagCborCodec, r)?,
            #[cfg(feature = "dag-json")]
            IpldCodec::DagJson => Self::decode(DagJsonCodec, r)?,
            #[cfg(feature = "dag-pb")]
            IpldCodec::DagPb => Self::decode(DagPbCodec, r)?,
        })
    }
}

impl References<IpldCodec> for Ipld {
    fn references<R: Read + Seek, E: Extend<Cid>>(
        c: IpldCodec,
        r: &mut R,
        set: &mut E,
    ) -> Result<()> {
        match c {
            IpldCodec::Raw => <Self as References<RawCodec>>::references(RawCodec, r, set)?,
            #[cfg(feature = "dag-cbor")]
            IpldCodec::DagCbor => {
                <Self as References<DagCborCodec>>::references(DagCborCodec, r, set)?
            }
            #[cfg(feature = "dag-json")]
            IpldCodec::DagJson => {
                <Self as References<DagJsonCodec>>::references(DagJsonCodec, r, set)?
            }
            #[cfg(feature = "dag-pb")]
            IpldCodec::DagPb => <Self as References<DagPbCodec>>::references(DagPbCodec, r, set)?,
        };
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn raw_encode() {
        let data = Ipld::Bytes([0x22, 0x33, 0x44].to_vec());
        let result = IpldCodec::Raw.encode(&data).unwrap();
        assert_eq!(result, vec![0x22, 0x33, 0x44]);
    }

    #[test]
    fn raw_decode() {
        let data = [0x22, 0x33, 0x44];
        let result: Ipld = IpldCodec::Raw.decode(&data).unwrap();
        assert_eq!(result, Ipld::Bytes(data.to_vec()));
    }

    #[cfg(feature = "dag-cbor")]
    #[test]
    fn dag_cbor_encode() {
        let data = Ipld::Bytes([0x22, 0x33, 0x44].to_vec());
        let result = IpldCodec::DagCbor.encode(&data).unwrap();
        assert_eq!(result, vec![0x43, 0x22, 0x33, 0x44]);
    }

    #[cfg(feature = "dag-cbor")]
    #[test]
    fn dag_cbor_decode() {
        let data = [0x43, 0x22, 0x33, 0x44];
        let result: Ipld = IpldCodec::DagCbor.decode(&data).unwrap();
        assert_eq!(result, Ipld::Bytes(vec![0x22, 0x33, 0x44]));
    }

    #[cfg(feature = "dag-json")]
    #[test]
    fn dag_json_encode() {
        let data = Ipld::Bool(true);
        let result = String::from_utf8(IpldCodec::DagJson.encode(&data).unwrap().to_vec()).unwrap();
        assert_eq!(result, "true");
    }

    #[cfg(feature = "dag-json")]
    #[test]
    fn dag_json_decode() {
        let data = b"true";
        let result: Ipld = IpldCodec::DagJson.decode(data).unwrap();
        assert_eq!(result, Ipld::Bool(true));
    }

    #[cfg(feature = "dag-pb")]
    #[test]
    fn dag_pb_encode() {
        let mut data_map = std::collections::BTreeMap::<String, Ipld>::new();
        data_map.insert("Data".to_string(), Ipld::Bytes(b"data".to_vec()));
        data_map.insert("Links".to_string(), Ipld::List(vec![]));

        let data = Ipld::Map(data_map);
        let result = IpldCodec::DagPb.encode(&data).unwrap();
        assert_eq!(result, vec![0x0a, 0x04, 0x64, 0x61, 0x74, 0x61]);
    }

    #[cfg(feature = "dag-pb")]
    #[test]
    fn dag_pb_decode() {
        let mut data_map = std::collections::BTreeMap::<String, Ipld>::new();
        data_map.insert("Data".to_string(), Ipld::Bytes(b"data".to_vec()));
        data_map.insert("Links".to_string(), Ipld::List(vec![]));
        let expected = Ipld::Map(data_map);

        let data = [0x0a, 0x04, 0x64, 0x61, 0x74, 0x61];
        let result: Ipld = IpldCodec::DagPb.decode(&data).unwrap();
        assert_eq!(result, expected);
    }
}
