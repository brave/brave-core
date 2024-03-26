use crate::Error;
#[cfg(feature = "alloc")]
use alloc::vec::Vec;
use core::convert::TryFrom;

use core::convert::TryInto;
use core::fmt::Debug;
#[cfg(feature = "serde-codec")]
use serde_big_array::BigArray;

use unsigned_varint::encode as varint_encode;

#[cfg(feature = "std")]
use std::io;

#[cfg(not(feature = "std"))]
use core2::io;

/// Trait that implements hashing.
///
/// It is usually implemented by a custom code table enum that derives the [`Multihash` derive].
///
/// [`Multihash` derive]: crate::derive
pub trait MultihashDigest<const S: usize>:
    TryFrom<u64> + Into<u64> + Send + Sync + Unpin + Copy + Eq + Debug + 'static
{
    /// Calculate the hash of some input data.
    ///
    /// # Example
    ///
    /// ```
    /// // `Code` implements `MultihashDigest`
    /// use multihash::{Code, MultihashDigest};
    ///
    /// let hash = Code::Sha3_256.digest(b"Hello world!");
    /// println!("{:02x?}", hash);
    /// ```
    fn digest(&self, input: &[u8]) -> Multihash<S>;

    /// Create a multihash from an existing multihash digest.
    ///
    /// # Example
    ///
    /// ```
    /// use multihash::{Code, Hasher, MultihashDigest, Sha3_256};
    ///
    /// let mut hasher = Sha3_256::default();
    /// hasher.update(b"Hello world!");
    /// let hash = Code::Sha3_256.wrap(&hasher.finalize()).unwrap();
    /// println!("{:02x?}", hash);
    /// ```
    fn wrap(&self, digest: &[u8]) -> Result<Multihash<S>, Error>;
}

/// A Multihash instance that only supports the basic functionality and no hashing.
///
/// With this Multihash implementation you can operate on Multihashes in a generic way, but
/// no hasher implementation is associated with the code.
///
/// # Example
///
/// ```
/// use multihash::Multihash;
///
/// const Sha3_256: u64 = 0x16;
/// let digest_bytes = [
///     0x16, 0x20, 0x64, 0x4b, 0xcc, 0x7e, 0x56, 0x43, 0x73, 0x04, 0x09, 0x99, 0xaa, 0xc8, 0x9e,
///     0x76, 0x22, 0xf3, 0xca, 0x71, 0xfb, 0xa1, 0xd9, 0x72, 0xfd, 0x94, 0xa3, 0x1c, 0x3b, 0xfb,
///     0xf2, 0x4e, 0x39, 0x38,
/// ];
/// let mh = Multihash::from_bytes(&digest_bytes).unwrap();
/// assert_eq!(mh.code(), Sha3_256);
/// assert_eq!(mh.size(), 32);
/// assert_eq!(mh.digest(), &digest_bytes[2..]);
/// ```
#[cfg_attr(feature = "serde-codec", derive(serde::Deserialize))]
#[cfg_attr(feature = "serde-codec", derive(serde::Serialize))]
#[derive(Clone, Copy, Debug, Eq, Ord, PartialOrd)]
pub struct Multihash<const S: usize> {
    /// The code of the Multihash.
    code: u64,
    /// The actual size of the digest in bytes (not the allocated size).
    size: u8,
    /// The digest.
    #[cfg_attr(feature = "serde-codec", serde(with = "BigArray"))]
    digest: [u8; S],
}

impl<const S: usize> Default for Multihash<S> {
    fn default() -> Self {
        Self {
            code: 0,
            size: 0,
            digest: [0; S],
        }
    }
}

impl<const S: usize> Multihash<S> {
    /// Wraps the digest in a multihash.
    pub const fn wrap(code: u64, input_digest: &[u8]) -> Result<Self, Error> {
        if input_digest.len() > S {
            return Err(Error::InvalidSize(input_digest.len() as _));
        }
        let size = input_digest.len();
        let mut digest = [0; S];
        let mut i = 0;
        while i < size {
            digest[i] = input_digest[i];
            i += 1;
        }
        Ok(Self {
            code,
            size: size as u8,
            digest,
        })
    }

    /// Returns the code of the multihash.
    pub const fn code(&self) -> u64 {
        self.code
    }

    /// Returns the size of the digest.
    pub const fn size(&self) -> u8 {
        self.size
    }

    /// Returns the digest.
    pub fn digest(&self) -> &[u8] {
        &self.digest[..self.size as usize]
    }

    /// Reads a multihash from a byte stream.
    pub fn read<R: io::Read>(r: R) -> Result<Self, Error>
    where
        Self: Sized,
    {
        let (code, size, digest) = read_multihash(r)?;
        Ok(Self { code, size, digest })
    }

    /// Parses a multihash from a bytes.
    ///
    /// You need to make sure the passed in bytes have the correct length. The digest length
    /// needs to match the `size` value of the multihash.
    pub fn from_bytes(mut bytes: &[u8]) -> Result<Self, Error>
    where
        Self: Sized,
    {
        let result = Self::read(&mut bytes)?;
        // There were more bytes supplied than read
        if !bytes.is_empty() {
            return Err(Error::InvalidSize(bytes.len().try_into().expect(
                "Currently the maximum size is 255, therefore always fits into usize",
            )));
        }

        Ok(result)
    }

    /// Writes a multihash to a byte stream, returning the written size.
    pub fn write<W: io::Write>(&self, w: W) -> Result<usize, Error> {
        write_multihash(w, self.code(), self.size(), self.digest())
    }

    /// Returns the length in bytes needed to encode this multihash into bytes.
    pub fn encoded_len(&self) -> usize {
        let mut code_buf = varint_encode::u64_buffer();
        let code = varint_encode::u64(self.code, &mut code_buf);

        let mut size_buf = varint_encode::u8_buffer();
        let size = varint_encode::u8(self.size, &mut size_buf);

        code.len() + size.len() + usize::from(self.size)
    }

    #[cfg(feature = "alloc")]
    /// Returns the bytes of a multihash.
    pub fn to_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(self.size().into());
        let written = self
            .write(&mut bytes)
            .expect("writing to a vec should never fail");
        debug_assert_eq!(written, bytes.len());
        bytes
    }

    /// Truncates the multihash to the given size. It's up to the caller to ensure that the new size
    /// is secure (cryptographically) to use.
    ///
    /// If the new size is larger than the current size, this method does nothing.
    ///
    /// ```
    /// use multihash::{Code, MultihashDigest};
    ///
    /// let hash = Code::Sha3_256.digest(b"Hello world!").truncate(20);
    /// ```
    pub fn truncate(&self, size: u8) -> Self {
        let mut mh = *self;
        mh.size = mh.size.min(size);
        mh
    }

    /// Resizes the backing multihash buffer. This function fails if the hash digest is larger than
    /// the target size.
    ///
    /// ```
    /// use multihash::{Code, MultihashDigest, MultihashGeneric};
    ///
    /// let hash = Code::Sha3_256.digest(b"Hello world!");
    /// let large_hash: MultihashGeneric<32> = hash.resize().unwrap();
    /// ```
    pub fn resize<const R: usize>(&self) -> Result<Multihash<R>, Error> {
        let size = self.size as usize;
        if size > R {
            return Err(Error::InvalidSize(self.size as u64));
        }
        let mut mh = Multihash {
            code: self.code,
            size: self.size,
            digest: [0; R],
        };
        mh.digest[..size].copy_from_slice(&self.digest[..size]);
        Ok(mh)
    }

    /// Decomposes struct, useful when needing a `Sized` array or moving all the data into another type
    ///
    /// It is recommended to use `digest()` `code()` and `size()` for most cases
    ///
    /// ```
    /// use multihash::{Code, MultihashDigest};
    /// struct Foo<const S: usize> {
    ///     arr: [u8; S],
    ///     len: usize,
    /// }
    ///
    /// let hash = Code::Sha3_256.digest(b"Hello world!");
    /// let (.., arr, size) = hash.into_inner();
    /// let foo = Foo { arr, len: size as usize };
    /// ```
    pub fn into_inner(self) -> (u64, [u8; S], u8) {
        let Self { code, digest, size } = self;
        (code, digest, size)
    }
}

// Don't hash the whole allocated space, but just the actual digest
#[allow(unknown_lints, renamed_and_removed_lints)]
#[allow(clippy::derived_hash_with_manual_eq, clippy::derive_hash_xor_eq)]
impl<const S: usize> core::hash::Hash for Multihash<S> {
    fn hash<T: core::hash::Hasher>(&self, state: &mut T) {
        self.code.hash(state);
        self.digest().hash(state);
    }
}

#[cfg(feature = "alloc")]
impl<const S: usize> From<Multihash<S>> for Vec<u8> {
    fn from(multihash: Multihash<S>) -> Self {
        multihash.to_bytes()
    }
}

impl<const A: usize, const B: usize> PartialEq<Multihash<B>> for Multihash<A> {
    fn eq(&self, other: &Multihash<B>) -> bool {
        // NOTE: there's no need to explicitly check the sizes, that's implicit in the digest.
        self.code == other.code && self.digest() == other.digest()
    }
}

#[cfg(feature = "scale-codec")]
impl<const S: usize> parity_scale_codec::Encode for Multihash<S> {
    fn encode_to<EncOut: parity_scale_codec::Output + ?Sized>(&self, dest: &mut EncOut) {
        self.code.encode_to(dest);
        self.size.encode_to(dest);
        // **NOTE** We write the digest directly to dest, since we have known the size of digest.
        //
        // We do not choose to encode &[u8] directly, because it will add extra bytes (the compact length of digest).
        // For a valid multihash, the length of digest must equal to `size`.
        // Therefore, we can only read raw bytes whose length is equal to `size` when decoding.
        dest.write(self.digest());
    }
}

#[cfg(feature = "scale-codec")]
impl<const S: usize> parity_scale_codec::EncodeLike for Multihash<S> {}

#[cfg(feature = "scale-codec")]
impl<const S: usize> parity_scale_codec::Decode for Multihash<S> {
    fn decode<DecIn: parity_scale_codec::Input>(
        input: &mut DecIn,
    ) -> Result<Self, parity_scale_codec::Error> {
        let mut mh = Multihash {
            code: parity_scale_codec::Decode::decode(input)?,
            size: parity_scale_codec::Decode::decode(input)?,
            digest: [0; S],
        };
        if mh.size as usize > S {
            return Err(parity_scale_codec::Error::from("invalid size"));
        }
        // For a valid multihash, the length of digest must equal to the size.
        input.read(&mut mh.digest[..mh.size as usize])?;
        Ok(mh)
    }
}

/// Writes the multihash to a byte stream.
pub fn write_multihash<W>(mut w: W, code: u64, size: u8, digest: &[u8]) -> Result<usize, Error>
where
    W: io::Write,
{
    let mut code_buf = varint_encode::u64_buffer();
    let code = varint_encode::u64(code, &mut code_buf);

    let mut size_buf = varint_encode::u8_buffer();
    let size = varint_encode::u8(size, &mut size_buf);

    let written = code.len() + size.len() + digest.len();

    w.write_all(code)?;
    w.write_all(size)?;
    w.write_all(digest)?;

    Ok(written)
}

/// Reads a multihash from a byte stream that contains a full multihash (code, size and the digest)
///
/// Returns the code, size and the digest. The size is the actual size and not the
/// maximum/allocated size of the digest.
///
/// Currently the maximum size for a digest is 255 bytes.
pub fn read_multihash<R, const S: usize>(mut r: R) -> Result<(u64, u8, [u8; S]), Error>
where
    R: io::Read,
{
    let code = read_u64(&mut r)?;
    let size = read_u64(&mut r)?;

    if size > S as u64 || size > u8::MAX as u64 {
        return Err(Error::InvalidSize(size));
    }

    let mut digest = [0; S];
    r.read_exact(&mut digest[..size as usize])?;
    Ok((code, size as u8, digest))
}

#[cfg(feature = "std")]
pub(crate) use unsigned_varint::io::read_u64;

/// Reads 64 bits from a byte array into a u64
/// Adapted from unsigned-varint's generated read_u64 function at
/// https://github.com/paritytech/unsigned-varint/blob/master/src/io.rs
#[cfg(not(feature = "std"))]
pub(crate) fn read_u64<R: io::Read>(mut r: R) -> Result<u64, Error> {
    use unsigned_varint::decode;
    let mut b = varint_encode::u64_buffer();
    for i in 0..b.len() {
        let n = r.read(&mut (b[i..i + 1]))?;
        if n == 0 {
            return Err(Error::Varint(decode::Error::Insufficient));
        } else if decode::is_last(b[i]) {
            return decode::u64(&b[..=i])
                .map(|decoded| decoded.0)
                .map_err(Error::Varint);
        }
    }
    Err(Error::Varint(decode::Error::Overflow))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::multihash_impl::Code;

    #[test]
    fn roundtrip() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let mut buf = [0u8; 35];
        let written = hash.write(&mut buf[..]).unwrap();
        let hash2 = Multihash::<32>::read(&buf[..]).unwrap();
        assert_eq!(hash, hash2);
        assert_eq!(hash.encoded_len(), written);
    }

    #[test]
    fn test_truncate_down() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let small = hash.truncate(20);
        assert_eq!(small.size(), 20);
    }

    #[test]
    fn test_truncate_up() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let small = hash.truncate(100);
        assert_eq!(small.size(), 32);
    }

    #[test]
    fn test_resize_fits() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let _: Multihash<32> = hash.resize().unwrap();
    }

    #[test]
    fn test_resize_up() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let _: Multihash<100> = hash.resize().unwrap();
    }

    #[test]
    fn test_resize_truncate() {
        let hash = Code::Sha2_256.digest(b"hello world");
        hash.resize::<20>().unwrap_err();
    }

    #[test]
    #[cfg(feature = "scale-codec")]
    fn test_scale() {
        use parity_scale_codec::{Decode, Encode};

        let mh1 = Code::Sha2_256.digest(b"hello world");
        // println!("mh1: code = {}, size = {}, digest = {:?}", mh1.code(), mh1.size(), mh1.digest());
        let mh1_bytes = mh1.encode();
        // println!("Multihash<32>: {}", hex::encode(&mh1_bytes));
        let mh2: Multihash<32> = Decode::decode(&mut &mh1_bytes[..]).unwrap();
        assert_eq!(mh1, mh2);

        let mh3: Multihash<64> = Code::Sha2_256.digest(b"hello world");
        // println!("mh3: code = {}, size = {}, digest = {:?}", mh3.code(), mh3.size(), mh3.digest());
        let mh3_bytes = mh3.encode();
        // println!("Multihash<64>: {}", hex::encode(&mh3_bytes));
        let mh4: Multihash<64> = Decode::decode(&mut &mh3_bytes[..]).unwrap();
        assert_eq!(mh3, mh4);

        assert_eq!(mh1_bytes, mh3_bytes);
    }

    #[test]
    #[cfg(feature = "serde-codec")]
    fn test_serde() {
        let mh = Multihash::<32>::default();
        let bytes = serde_json::to_string(&mh).unwrap();
        let mh2: Multihash<32> = serde_json::from_str(&bytes).unwrap();
        assert_eq!(mh, mh2);
    }

    #[test]
    fn test_eq_sizes() {
        let mh1 = Multihash::<32>::default();
        let mh2 = Multihash::<64>::default();
        assert_eq!(mh1, mh2);
    }

    #[test]
    fn decode_non_minimal_error() {
        // This is a non-minimal varint.
        let data = [241, 0, 0, 0, 0, 0, 128, 132, 132, 132, 58];
        let result = read_u64(&data[..]);
        assert!(result.is_err());
    }
}
