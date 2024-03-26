//! Implements the raw codec.
use alloc::{boxed::Box, vec, vec::Vec};
use core::{convert::TryFrom, iter::Extend};

use crate::cid::Cid;
use crate::codec::{Codec, Decode, Encode, References};
use crate::error::{Result, UnsupportedCodec};
use crate::io::{Read, Seek, Write};
use crate::ipld::Ipld;

/// Raw codec.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord)]
pub struct RawCodec;

impl Codec for RawCodec {}

impl From<RawCodec> for u64 {
    fn from(_: RawCodec) -> Self {
        0x55
    }
}

impl TryFrom<u64> for RawCodec {
    type Error = UnsupportedCodec;

    fn try_from(_: u64) -> core::result::Result<Self, Self::Error> {
        Ok(Self)
    }
}

impl Encode<RawCodec> for [u8] {
    fn encode<W: Write>(&self, _: RawCodec, w: &mut W) -> Result<()> {
        w.write_all(self).map_err(anyhow::Error::msg)
    }
}

impl Encode<RawCodec> for Box<[u8]> {
    fn encode<W: Write>(&self, _: RawCodec, w: &mut W) -> Result<()> {
        w.write_all(&self[..]).map_err(anyhow::Error::msg)
    }
}

impl Encode<RawCodec> for Vec<u8> {
    fn encode<W: Write>(&self, _: RawCodec, w: &mut W) -> Result<()> {
        w.write_all(&self[..]).map_err(anyhow::Error::msg)
    }
}

impl Encode<RawCodec> for Ipld {
    fn encode<W: Write>(&self, c: RawCodec, w: &mut W) -> Result<()> {
        if let Ipld::Bytes(bytes) = self {
            bytes.encode(c, w)
        } else {
            Err(anyhow::Error::msg(crate::error::TypeError::new(
                crate::error::TypeErrorType::Bytes,
                self,
            )))
        }
    }
}

impl Decode<RawCodec> for Box<[u8]> {
    fn decode<R: Read + Seek>(c: RawCodec, r: &mut R) -> Result<Self> {
        let buf: Vec<u8> = Decode::decode(c, r)?;
        Ok(buf.into_boxed_slice())
    }
}

impl Decode<RawCodec> for Vec<u8> {
    fn decode<R: Read + Seek>(_: RawCodec, r: &mut R) -> Result<Self> {
        let mut buf = vec![];
        r.read_to_end(&mut buf).map_err(anyhow::Error::msg)?;
        Ok(buf)
    }
}

impl Decode<RawCodec> for Ipld {
    fn decode<R: Read + Seek>(c: RawCodec, r: &mut R) -> Result<Self> {
        let bytes: Vec<u8> = Decode::decode(c, r)?;
        Ok(Ipld::Bytes(bytes))
    }
}

impl<T> References<RawCodec> for T {
    fn references<R: Read, E: Extend<Cid>>(_c: RawCodec, _r: &mut R, _set: &mut E) -> Result<()> {
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_raw_codec() {
        let data: &[u8] = &[0, 1, 2, 3];
        let bytes = RawCodec.encode(data).unwrap();
        assert_eq!(data, &*bytes);
        let data2: Vec<u8> = RawCodec.decode(&bytes).unwrap();
        assert_eq!(data, &*data2);

        let ipld = Ipld::Bytes(data2);
        let bytes = RawCodec.encode(&ipld).unwrap();
        assert_eq!(data, &*bytes);
        let ipld2: Ipld = RawCodec.decode(&bytes).unwrap();
        assert_eq!(ipld, ipld2);
    }
}
