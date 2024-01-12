//! `Ipld` codecs.
use alloc::{format, string::String, vec::Vec};
use core::{convert::TryFrom, ops::Deref};

use crate::cid::Cid;
use crate::error::{Result, UnsupportedCodec};
use crate::io::{Cursor, Read, Seek, Write};
use crate::ipld::Ipld;

/// Codec trait.
pub trait Codec:
    Copy + Unpin + Send + Sync + 'static + Sized + TryFrom<u64, Error = UnsupportedCodec> + Into<u64>
{
    /// Encodes an encodable type.
    fn encode<T: Encode<Self> + ?Sized>(&self, obj: &T) -> Result<Vec<u8>> {
        let mut buf = Vec::with_capacity(u16::MAX as usize);
        obj.encode(*self, &mut buf)?;
        Ok(buf)
    }

    /// Decodes a decodable type.
    fn decode<T: Decode<Self>>(&self, bytes: &[u8]) -> Result<T> {
        T::decode(*self, &mut Cursor::new(bytes))
    }

    /// Scrapes the references.
    fn references<T: References<Self>, E: Extend<Cid>>(
        &self,
        bytes: &[u8],
        set: &mut E,
    ) -> Result<()> {
        T::references(*self, &mut Cursor::new(bytes), set)
    }
}

/// Encode trait.
///
/// This trait is generic over a codec, so that different codecs can be implemented for the same
/// type.
pub trait Encode<C: Codec> {
    /// Encodes into a `impl Write`.
    ///
    /// It takes a specific codec as parameter, so that the [`Encode`] can be generic over an enum
    /// that contains multiple codecs.
    fn encode<W: Write>(&self, c: C, w: &mut W) -> Result<()>;
}

impl<C: Codec, T: Encode<C>> Encode<C> for &T {
    fn encode<W: Write>(&self, c: C, w: &mut W) -> Result<()> {
        self.deref().encode(c, w)
    }
}

/// Decode trait.
///
/// This trait is generic over a codec, so that different codecs can be implemented for the same
/// type.
pub trait Decode<C: Codec>: Sized {
    /// Decode from an `impl Read`.
    ///
    /// It takes a specific codec as parameter, so that the [`Decode`] can be generic over an enum
    /// that contains multiple codecs.
    fn decode<R: Read + Seek>(c: C, r: &mut R) -> Result<Self>;
}

/// References trait.
///
/// This trait is generic over a codec, so that different codecs can be implemented for the same
/// type.
pub trait References<C: Codec>: Sized {
    /// Scrape the references from an `impl Read`.
    ///
    /// It takes a specific codec as parameter, so that the [`References`] can be generic over an
    /// enum that contains multiple codecs.
    fn references<R: Read + Seek, E: Extend<Cid>>(c: C, r: &mut R, set: &mut E) -> Result<()>;
}

/// Utility for testing codecs.
///
/// Encodes the `data` using the codec `c` and checks that it matches the `ipld`.
pub fn assert_roundtrip<C, T>(c: C, data: &T, ipld: &Ipld)
where
    C: Codec,
    T: Decode<C> + Encode<C> + core::fmt::Debug + PartialEq,
    Ipld: Decode<C> + Encode<C>,
{
    fn hex(bytes: &[u8]) -> String {
        bytes.iter().map(|byte| format!("{:02x}", byte)).collect()
    }
    let mut bytes = Vec::new();
    data.encode(c, &mut bytes).unwrap();
    let mut bytes2 = Vec::new();
    ipld.encode(c, &mut bytes2).unwrap();
    if bytes != bytes2 {
        panic!(
            r#"assertion failed: `(left == right)`
        left: `{}`,
       right: `{}`"#,
            hex(&bytes),
            hex(&bytes2)
        );
    }
    let ipld2: Ipld = Decode::decode(c, &mut Cursor::new(bytes.as_slice())).unwrap();
    assert_eq!(&ipld2, ipld);
    let data2: T = Decode::decode(c, &mut Cursor::new(bytes.as_slice())).unwrap();
    assert_eq!(&data2, data);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ipld::Ipld;
    use anyhow::anyhow;

    #[derive(Clone, Copy, Debug)]
    struct CodecImpl;

    impl Codec for CodecImpl {}

    impl From<CodecImpl> for u64 {
        fn from(_: CodecImpl) -> Self {
            0
        }
    }

    impl TryFrom<u64> for CodecImpl {
        type Error = UnsupportedCodec;

        fn try_from(_: u64) -> core::result::Result<Self, Self::Error> {
            Ok(Self)
        }
    }

    impl Encode<CodecImpl> for Ipld {
        fn encode<W: Write>(&self, _: CodecImpl, w: &mut W) -> Result<()> {
            match self {
                Self::Null => Ok(w.write_all(&[0]).map_err(anyhow::Error::msg)?),
                _ => Err(anyhow!("not null")),
            }
        }
    }

    impl Decode<CodecImpl> for Ipld {
        fn decode<R: Read>(_: CodecImpl, r: &mut R) -> Result<Self> {
            let mut buf = [0; 1];
            r.read_exact(&mut buf).map_err(anyhow::Error::msg)?;
            if buf[0] == 0 {
                Ok(Ipld::Null)
            } else {
                Err(anyhow!("not null"))
            }
        }
    }

    #[test]
    fn test_codec() {
        let bytes = CodecImpl.encode(&Ipld::Null).unwrap();
        let ipld: Ipld = CodecImpl.decode(&bytes).unwrap();
        assert_eq!(ipld, Ipld::Null);
    }
}
