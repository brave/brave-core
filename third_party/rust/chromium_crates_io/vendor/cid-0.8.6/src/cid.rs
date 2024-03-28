//! This module contains the main CID type.
//!
//! If you are an application developer you likely won't use the `Cid` which is generic over the
//! digest size. Intead you would use the concrete top-level `Cid` type.
//!
//! As a library author that works with CIDs that should support hashes of anysize, you would
//! import the `Cid` type from this module.
use core::convert::TryFrom;

#[cfg(feature = "alloc")]
use multibase::{encode as base_encode, Base};

use multihash::MultihashGeneric as Multihash;
use unsigned_varint::encode as varint_encode;

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(feature = "alloc")]
use alloc::{
    borrow,
    string::{String, ToString},
    vec::Vec,
};

#[cfg(feature = "std")]
pub(crate) use unsigned_varint::io::read_u64 as varint_read_u64;

/// Reads 64 bits from a byte array into a u64
/// Adapted from unsigned-varint's generated read_u64 function at
/// https://github.com/paritytech/unsigned-varint/blob/master/src/io.rs
#[cfg(not(feature = "std"))]
pub(crate) fn varint_read_u64<R: io::Read>(mut r: R) -> Result<u64> {
    use unsigned_varint::decode;
    let mut b = varint_encode::u64_buffer();
    for i in 0..b.len() {
        let n = r.read(&mut (b[i..i + 1]))?;
        if n == 0 {
            return Err(Error::VarIntDecodeError);
        } else if decode::is_last(b[i]) {
            return Ok(decode::u64(&b[..=i]).unwrap().0);
        }
    }
    Err(Error::VarIntDecodeError)
}

#[cfg(feature = "std")]
use std::io;

#[cfg(not(feature = "std"))]
use core2::io;

use crate::error::{Error, Result};
use crate::version::Version;

/// DAG-PB multicodec code
const DAG_PB: u64 = 0x70;
/// The SHA_256 multicodec code
const SHA2_256: u64 = 0x12;

/// Representation of a CID.
///
/// The generic is about the allocated size of the multihash.
#[derive(Copy, PartialEq, Eq, Clone, PartialOrd, Ord, Hash)]
#[cfg_attr(feature = "scale-codec", derive(parity_scale_codec::Decode))]
#[cfg_attr(feature = "scale-codec", derive(parity_scale_codec::Encode))]
pub struct Cid<const S: usize> {
    /// The version of CID.
    version: Version,
    /// The codec of CID.
    codec: u64,
    /// The multihash of CID.
    hash: Multihash<S>,
}

impl<const S: usize> Cid<S> {
    /// Create a new CIDv0.
    pub const fn new_v0(hash: Multihash<S>) -> Result<Self> {
        if hash.code() != SHA2_256 {
            return Err(Error::InvalidCidV0Multihash);
        }
        Ok(Self {
            version: Version::V0,
            codec: DAG_PB,
            hash,
        })
    }

    /// Create a new CIDv1.
    pub const fn new_v1(codec: u64, hash: Multihash<S>) -> Self {
        Self {
            version: Version::V1,
            codec,
            hash,
        }
    }

    /// Create a new CID.
    pub const fn new(version: Version, codec: u64, hash: Multihash<S>) -> Result<Self> {
        match version {
            Version::V0 => {
                if codec != DAG_PB {
                    return Err(Error::InvalidCidV0Codec);
                }
                Self::new_v0(hash)
            }
            Version::V1 => Ok(Self::new_v1(codec, hash)),
        }
    }

    /// Convert a CIDv0 to a CIDv1. Returns unchanged if already a CIDv1.
    pub fn into_v1(self) -> Result<Self> {
        match self.version {
            Version::V0 => {
                if self.codec != DAG_PB {
                    return Err(Error::InvalidCidV0Codec);
                }
                Ok(Self::new_v1(self.codec, self.hash))
            }
            Version::V1 => Ok(self),
        }
    }

    /// Returns the cid version.
    pub const fn version(&self) -> Version {
        self.version
    }

    /// Returns the cid codec.
    pub const fn codec(&self) -> u64 {
        self.codec
    }

    /// Returns the cid multihash.
    pub const fn hash(&self) -> &Multihash<S> {
        &self.hash
    }

    /// Reads the bytes from a byte stream.
    pub fn read_bytes<R: io::Read>(mut r: R) -> Result<Self> {
        let version = varint_read_u64(&mut r)?;
        let codec = varint_read_u64(&mut r)?;

        // CIDv0 has the fixed `0x12 0x20` prefix
        if [version, codec] == [0x12, 0x20] {
            let mut digest = [0u8; 32];
            r.read_exact(&mut digest)?;
            let mh = Multihash::wrap(version, &digest).expect("Digest is always 32 bytes.");
            return Self::new_v0(mh);
        }

        let version = Version::try_from(version)?;
        match version {
            Version::V0 => Err(Error::InvalidExplicitCidV0),
            Version::V1 => {
                let mh = Multihash::read(r)?;
                Self::new(version, codec, mh)
            }
        }
    }

    fn write_bytes_v1<W: io::Write>(&self, mut w: W) -> Result<()> {
        let mut version_buf = varint_encode::u64_buffer();
        let version = varint_encode::u64(self.version.into(), &mut version_buf);

        let mut codec_buf = varint_encode::u64_buffer();
        let codec = varint_encode::u64(self.codec, &mut codec_buf);

        w.write_all(version)?;
        w.write_all(codec)?;
        self.hash.write(&mut w)?;
        Ok(())
    }

    /// Writes the bytes to a byte stream.
    pub fn write_bytes<W: io::Write>(&self, w: W) -> Result<()> {
        match self.version {
            Version::V0 => self.hash.write(w)?,
            Version::V1 => self.write_bytes_v1(w)?,
        }
        Ok(())
    }

    /// Returns the encoded bytes of the `Cid`.
    #[cfg(feature = "alloc")]
    pub fn to_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::new();
        self.write_bytes(&mut bytes).unwrap();
        bytes
    }

    #[cfg(feature = "alloc")]
    #[allow(clippy::wrong_self_convention)]
    fn to_string_v0(&self) -> String {
        Base::Base58Btc.encode(self.hash.to_bytes())
    }

    #[cfg(feature = "alloc")]
    #[allow(clippy::wrong_self_convention)]
    fn to_string_v1(&self) -> String {
        multibase::encode(Base::Base32Lower, self.to_bytes().as_slice())
    }

    /// Convert CID into a multibase encoded string
    ///
    /// # Example
    ///
    /// ```
    /// use cid::Cid;
    /// use multibase::Base;
    /// use multihash::{Code, MultihashDigest};
    ///
    /// const RAW: u64 = 0x55;
    ///
    /// let cid = Cid::new_v1(RAW, Code::Sha2_256.digest(b"foo"));
    /// let encoded = cid.to_string_of_base(Base::Base64).unwrap();
    /// assert_eq!(encoded, "mAVUSICwmtGto/8aP+ZtFPB0wQTQTQi1wZIO/oPmKXohiZueu");
    /// ```
    #[cfg(feature = "alloc")]
    pub fn to_string_of_base(&self, base: Base) -> Result<String> {
        match self.version {
            Version::V0 => {
                if base == Base::Base58Btc {
                    Ok(self.to_string_v0())
                } else {
                    Err(Error::InvalidCidV0Base)
                }
            }
            Version::V1 => Ok(base_encode(base, self.to_bytes())),
        }
    }
}

impl<const S: usize> Default for Cid<S> {
    fn default() -> Self {
        Self {
            version: Version::V1,
            codec: 0,
            hash: Multihash::<S>::default(),
        }
    }
}

// TODO: remove the dependency on alloc by fixing
// https://github.com/multiformats/rust-multibase/issues/33
#[cfg(feature = "alloc")]
impl<const S: usize> core::fmt::Display for Cid<S> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        let output = match self.version {
            Version::V0 => self.to_string_v0(),
            Version::V1 => self.to_string_v1(),
        };
        write!(f, "{}", output)
    }
}

#[cfg(feature = "alloc")]
impl<const S: usize> core::fmt::Debug for Cid<S> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        if f.alternate() {
            f.debug_struct("Cid")
                .field("version", &self.version())
                .field("codec", &self.codec())
                .field("hash", self.hash())
                .finish()
        } else {
            let output = match self.version {
                Version::V0 => self.to_string_v0(),
                Version::V1 => self.to_string_v1(),
            };
            write!(f, "Cid({})", output)
        }
    }
}

#[cfg(feature = "alloc")]
impl<const S: usize> core::str::FromStr for Cid<S> {
    type Err = Error;

    fn from_str(cid_str: &str) -> Result<Self> {
        Self::try_from(cid_str)
    }
}

#[cfg(feature = "alloc")]
impl<const S: usize> TryFrom<String> for Cid<S> {
    type Error = Error;

    fn try_from(cid_str: String) -> Result<Self> {
        Self::try_from(cid_str.as_str())
    }
}

#[cfg(feature = "alloc")]
impl<const S: usize> TryFrom<&str> for Cid<S> {
    type Error = Error;

    fn try_from(cid_str: &str) -> Result<Self> {
        static IPFS_DELIMETER: &str = "/ipfs/";

        let hash = match cid_str.find(IPFS_DELIMETER) {
            Some(index) => &cid_str[index + IPFS_DELIMETER.len()..],
            _ => cid_str,
        };

        if hash.len() < 2 {
            return Err(Error::InputTooShort);
        }

        let decoded = if Version::is_v0_str(hash) {
            Base::Base58Btc.decode(hash)?
        } else {
            let (_, decoded) = multibase::decode(hash)?;
            decoded
        };

        Self::try_from(decoded)
    }
}

#[cfg(feature = "alloc")]
impl<const S: usize> TryFrom<Vec<u8>> for Cid<S> {
    type Error = Error;

    fn try_from(bytes: Vec<u8>) -> Result<Self> {
        Self::try_from(bytes.as_slice())
    }
}

impl<const S: usize> TryFrom<&[u8]> for Cid<S> {
    type Error = Error;

    fn try_from(mut bytes: &[u8]) -> Result<Self> {
        Self::read_bytes(&mut bytes)
    }
}

impl<const S: usize> From<&Cid<S>> for Cid<S> {
    fn from(cid: &Cid<S>) -> Self {
        *cid
    }
}

#[cfg(feature = "alloc")]
impl<const S: usize> From<Cid<S>> for Vec<u8> {
    fn from(cid: Cid<S>) -> Self {
        cid.to_bytes()
    }
}

#[cfg(feature = "alloc")]
impl<const S: usize> From<Cid<S>> for String {
    fn from(cid: Cid<S>) -> Self {
        cid.to_string()
    }
}

#[cfg(feature = "alloc")]
impl<'a, const S: usize> From<Cid<S>> for borrow::Cow<'a, Cid<S>> {
    fn from(from: Cid<S>) -> Self {
        borrow::Cow::Owned(from)
    }
}

#[cfg(feature = "alloc")]
impl<'a, const S: usize> From<&'a Cid<S>> for borrow::Cow<'a, Cid<S>> {
    fn from(from: &'a Cid<S>) -> Self {
        borrow::Cow::Borrowed(from)
    }
}

#[cfg(test)]
mod tests {
    #[test]
    #[cfg(feature = "scale-codec")]
    fn test_cid_scale_codec() {
        use super::Cid;
        use parity_scale_codec::{Decode, Encode};

        let cid = Cid::<64>::default();
        let bytes = cid.encode();
        let cid2 = Cid::decode(&mut &bytes[..]).unwrap();
        assert_eq!(cid, cid2);
    }

    #[test]
    #[cfg(feature = "std")]
    fn test_debug_instance() {
        use super::Cid;
        use std::str::FromStr;
        let cid =
            Cid::<64>::from_str("bafyreibjo4xmgaevkgud7mbifn3dzp4v4lyaui4yvqp3f2bqwtxcjrdqg4")
                .unwrap();
        // short debug
        assert_eq!(
            &format!("{:?}", cid),
            "Cid(bafyreibjo4xmgaevkgud7mbifn3dzp4v4lyaui4yvqp3f2bqwtxcjrdqg4)"
        );
        // verbose debug
        let mut txt = format!("{:#?}", cid);
        txt.retain(|c| !c.is_whitespace());
        assert_eq!(&txt, "Cid{version:V1,codec:113,hash:Multihash{code:18,size:32,digest:[41,119,46,195,0,149,81,168,63,176,40,43,118,60,191,149,226,240,10,35,152,172,31,178,232,48,180,238,36,196,112,55,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,],},}");
    }
}
