//! CBOR codec.
#![deny(missing_docs)]
#![deny(warnings)]

use core::convert::TryFrom;
use libipld_core::codec::{Codec, Decode, Encode};
pub use libipld_core::error::{Result, UnsupportedCodec};

pub mod cbor;
pub mod decode;
pub mod encode;
pub mod error;

/// CBOR codec.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord)]
pub struct DagCborCodec;

impl Codec for DagCborCodec {}

impl From<DagCborCodec> for u64 {
    fn from(_: DagCborCodec) -> Self {
        0x71
    }
}

impl TryFrom<u64> for DagCborCodec {
    type Error = UnsupportedCodec;

    fn try_from(_: u64) -> core::result::Result<Self, Self::Error> {
        Ok(Self)
    }
}

/// Marker trait for types supporting the `DagCborCodec`.
pub trait DagCbor: Encode<DagCborCodec> + Decode<DagCborCodec> {}

impl<T: Encode<DagCborCodec> + Decode<DagCborCodec>> DagCbor for T {}

#[cfg(test)]
mod tests {
    use super::*;
    use libipld_core::cid::Cid;
    use libipld_core::codec::assert_roundtrip;
    use libipld_core::ipld::Ipld;
    use libipld_core::multihash::{Code, MultihashDigest};
    use libipld_macro::ipld;
    use std::collections::HashSet;

    #[test]
    fn test_encode_decode_cbor() {
        let cid = Cid::new_v1(0, Code::Blake3_256.digest(&b"cid"[..]));
        let ipld = ipld!({
          "number": 1,
          "list": [true, null, false],
          "bytes": vec![0, 1, 2, 3],
          "map": { "float": 0.0, "string": "hello" },
          "link": cid,
        });
        let bytes = DagCborCodec.encode(&ipld).unwrap();
        let ipld2 = DagCborCodec.decode(&bytes).unwrap();
        assert_eq!(ipld, ipld2);
    }

    #[test]
    fn test_references() {
        let cid = Cid::new_v1(0, Code::Blake3_256.digest(&b"0"[..]));
        let ipld = ipld!({
            "list": [true, cid],
        });
        let bytes = DagCborCodec.encode(&ipld).unwrap();
        let mut set = HashSet::new();
        DagCborCodec
            .references::<Ipld, _>(&bytes, &mut set)
            .unwrap();
        assert!(set.contains(&cid));
    }

    #[test]
    fn test_encode_max() {
        assert_roundtrip(DagCborCodec, &i8::MAX, &Ipld::Integer(i8::MAX as i128));
        assert_roundtrip(DagCborCodec, &i16::MAX, &Ipld::Integer(i16::MAX as i128));
        assert_roundtrip(DagCborCodec, &i32::MAX, &Ipld::Integer(i32::MAX as i128));
        assert_roundtrip(DagCborCodec, &i64::MAX, &Ipld::Integer(i64::MAX as i128));
        assert_roundtrip(DagCborCodec, &u8::MAX, &Ipld::Integer(u8::MAX as i128));
        assert_roundtrip(DagCborCodec, &u16::MAX, &Ipld::Integer(u16::MAX as i128));
        assert_roundtrip(DagCborCodec, &u32::MAX, &Ipld::Integer(u32::MAX as i128));
        assert_roundtrip(DagCborCodec, &u64::MAX, &Ipld::Integer(u64::MAX as i128));
    }

    #[test]
    fn test_encode_min() {
        assert_roundtrip(DagCborCodec, &i8::MIN, &Ipld::Integer(i8::MIN as i128));
        assert_roundtrip(DagCborCodec, &i16::MIN, &Ipld::Integer(i16::MIN as i128));
        assert_roundtrip(DagCborCodec, &i32::MIN, &Ipld::Integer(i32::MIN as i128));
        assert_roundtrip(DagCborCodec, &i64::MIN, &Ipld::Integer(i64::MIN as i128));
        assert_roundtrip(DagCborCodec, &u8::MIN, &Ipld::Integer(u8::MIN as i128));
        assert_roundtrip(DagCborCodec, &u16::MIN, &Ipld::Integer(u16::MIN as i128));
        assert_roundtrip(DagCborCodec, &u32::MIN, &Ipld::Integer(u32::MIN as i128));
        assert_roundtrip(DagCborCodec, &u64::MIN, &Ipld::Integer(u64::MIN as i128));
    }
}
