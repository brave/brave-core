#![cfg_attr(feature = "arb", allow(unreachable_code))] // Otherwise the "Cargo Hack" check fails since "arb" includes no hash algos by default
#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(not(feature = "std"), no_std)]

//! A batteries-included code table of multihashes.
//!
//! Whilst the `multihash` crate itself only defines the data structure, this crate defines a codetable via [`multihash_derive`] for several hashers.
//! Although convenient, depending on this crate will increase the dependency footprint of your project.
//!
//! It is only recommended as a getting-started-quickly solution.
//! For production, you should either define your own codetable or rely only on the `multihash` crate itself.

mod hasher_impl;

pub use multihash_derive::MultihashDigest;

#[cfg(feature = "blake2b")]
#[cfg_attr(docsrs, doc(cfg(feature = "blake2b")))]
pub use crate::hasher_impl::blake2b::{Blake2b256, Blake2b512, Blake2bHasher};
#[cfg(feature = "blake2s")]
#[cfg_attr(docsrs, doc(cfg(feature = "blake2s")))]
pub use crate::hasher_impl::blake2s::{Blake2s128, Blake2s256, Blake2sHasher};
#[cfg(feature = "blake3")]
#[cfg_attr(docsrs, doc(cfg(feature = "blake3")))]
pub use crate::hasher_impl::blake3::{Blake3Hasher, Blake3_256};
#[cfg(feature = "ripemd")]
#[cfg_attr(docsrs, doc(cfg(feature = "ripemd")))]
pub use crate::hasher_impl::ripemd::{Ripemd160, Ripemd256, Ripemd320};
#[cfg(feature = "sha1")]
#[cfg_attr(docsrs, doc(cfg(feature = "sha1")))]
pub use crate::hasher_impl::sha1::Sha1;
#[cfg(feature = "sha2")]
#[cfg_attr(docsrs, doc(cfg(feature = "sha2")))]
pub use crate::hasher_impl::sha2::{Sha2_256, Sha2_512};
#[cfg(feature = "sha3")]
#[cfg_attr(docsrs, doc(cfg(feature = "sha3")))]
pub use crate::hasher_impl::sha3::{
    Keccak224, Keccak256, Keccak384, Keccak512, Sha3_224, Sha3_256, Sha3_384, Sha3_512,
};
#[cfg(feature = "strobe")]
#[cfg_attr(docsrs, doc(cfg(feature = "strobe")))]
pub use crate::hasher_impl::strobe::{Strobe256, Strobe512, StrobeHasher};

/// Default (cryptographically secure) Multihash implementation.
///
/// This is a default set of hashing algorithms. Usually applications would use their own subset of
/// algorithms. See the [`multihash-derive`] crate for more information.
///
/// [`multihash-derive`]: https://docs.rs/multihash-derive
#[cfg_attr(feature = "arb", derive(arbitrary::Arbitrary))]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, Eq, MultihashDigest, PartialEq)]
#[mh(alloc_size = 64)]
pub enum Code {
    /// SHA-256 (32-byte hash size)
    #[cfg(feature = "sha2")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha2")))]
    #[mh(code = 0x12, hasher = crate::Sha2_256)]
    Sha2_256,
    /// SHA-512 (64-byte hash size)
    #[cfg(feature = "sha2")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha2")))]
    #[mh(code = 0x13, hasher = crate::Sha2_512)]
    Sha2_512,
    /// SHA3-224 (28-byte hash size)
    #[cfg(feature = "sha3")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha3")))]
    #[mh(code = 0x17, hasher = crate::Sha3_224)]
    Sha3_224,
    /// SHA3-256 (32-byte hash size)
    #[cfg(feature = "sha3")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha3")))]
    #[mh(code = 0x16, hasher = crate::Sha3_256)]
    Sha3_256,
    /// SHA3-384 (48-byte hash size)
    #[cfg(feature = "sha3")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha3")))]
    #[mh(code = 0x15, hasher = crate::Sha3_384)]
    Sha3_384,
    /// SHA3-512 (64-byte hash size)
    #[cfg(feature = "sha3")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha3")))]
    #[mh(code = 0x14, hasher = crate::Sha3_512)]
    Sha3_512,
    /// Keccak-224 (28-byte hash size)
    #[cfg(feature = "sha3")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha3")))]
    #[mh(code = 0x1a, hasher = crate::Keccak224)]
    Keccak224,
    /// Keccak-256 (32-byte hash size)
    #[cfg(feature = "sha3")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha3")))]
    #[mh(code = 0x1b, hasher = crate::Keccak256)]
    Keccak256,
    /// Keccak-384 (48-byte hash size)
    #[cfg(feature = "sha3")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha3")))]
    #[mh(code = 0x1c, hasher = crate::Keccak384)]
    Keccak384,
    /// Keccak-512 (64-byte hash size)
    #[cfg(feature = "sha3")]
    #[cfg_attr(docsrs, doc(cfg(feature = "sha3")))]
    #[mh(code = 0x1d, hasher = crate::Keccak512)]
    Keccak512,
    /// BLAKE2b-256 (32-byte hash size)
    #[cfg(feature = "blake2b")]
    #[cfg_attr(docsrs, doc(cfg(feature = "blake2b")))]
    #[mh(code = 0xb220, hasher = crate::Blake2b256)]
    Blake2b256,
    /// BLAKE2b-512 (64-byte hash size)
    #[cfg(feature = "blake2b")]
    #[cfg_attr(docsrs, doc(cfg(feature = "blake2b")))]
    #[mh(code = 0xb240, hasher = crate::Blake2b512)]
    Blake2b512,
    /// BLAKE2s-128 (16-byte hash size)
    #[cfg(feature = "blake2s")]
    #[cfg_attr(docsrs, doc(cfg(feature = "blake2s")))]
    #[mh(code = 0xb250, hasher = crate::Blake2s128)]
    Blake2s128,
    /// BLAKE2s-256 (32-byte hash size)
    #[cfg(feature = "blake2s")]
    #[cfg_attr(docsrs, doc(cfg(feature = "blake2s")))]
    #[mh(code = 0xb260, hasher = crate::Blake2s256)]
    Blake2s256,
    /// BLAKE3-256 (32-byte hash size)
    #[cfg(feature = "blake3")]
    #[cfg_attr(docsrs, doc(cfg(feature = "blake3")))]
    #[mh(code = 0x1e, hasher = crate::Blake3_256)]
    Blake3_256,
    /// RIPEMD-160 (20-byte hash size)
    #[cfg(feature = "ripemd")]
    #[cfg_attr(docsrs, doc(cfg(feature = "ripemd")))]
    #[mh(code = 0x1053, hasher = crate::Ripemd160)]
    Ripemd160,
    /// RIPEMD-256 (32-byte hash size)
    #[cfg(feature = "ripemd")]
    #[cfg_attr(docsrs, doc(cfg(feature = "ripemd")))]
    #[mh(code = 0x1054, hasher = crate::Ripemd256)]
    Ripemd256,
    /// RIPEMD-320 (40-byte hash size)
    #[cfg(feature = "ripemd")]
    #[cfg_attr(docsrs, doc(cfg(feature = "ripemd")))]
    #[mh(code = 0x1055, hasher = crate::Ripemd320)]
    Ripemd320,
}

#[cfg(all(test, any(feature = "sha2", feature = "sha3")))]
mod tests {
    use super::*;
    #[cfg(feature = "sha3")]
    use crate::hasher_impl::sha3::{Sha3_256, Sha3_512};
    #[cfg(feature = "sha3")]
    use multihash_derive::Hasher;
    #[cfg(feature = "sha2")]
    use multihash_derive::{Multihash, MultihashDigest};

    #[test]
    #[cfg(feature = "sha3")]
    fn test_hasher_256() {
        let mut hasher = Sha3_256::default();
        hasher.update(b"hello world");
        let digest = hasher.finalize();
        let hash = Code::Sha3_256.wrap(digest).unwrap();
        let hash2 = Code::Sha3_256.digest(b"hello world");
        assert_eq!(hash.code(), u64::from(Code::Sha3_256));
        assert_eq!(hash.size(), 32);
        assert_eq!(hash.digest(), digest);
        assert_eq!(hash, hash2);
    }

    #[test]
    #[cfg(feature = "sha3")]
    fn test_hasher_512() {
        let mut hasher = Sha3_512::default();
        hasher.update(b"hello world");
        let digest = hasher.finalize();
        let hash = Code::Sha3_512.wrap(digest).unwrap();
        let hash2 = Code::Sha3_512.digest(b"hello world");
        assert_eq!(hash.code(), u64::from(Code::Sha3_512));
        assert_eq!(hash.size(), 64);
        assert_eq!(hash.digest(), digest);
        assert_eq!(hash, hash2);
    }

    #[test]
    #[cfg(feature = "sha2")]
    fn roundtrip() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let mut buf = [0u8; 35];
        let written = hash.write(&mut buf[..]).unwrap();
        let hash2 = Multihash::<32>::read(&buf[..]).unwrap();
        assert_eq!(hash, hash2);
        assert_eq!(hash.encoded_len(), written);
    }

    #[test]
    #[cfg(feature = "sha2")]
    fn test_truncate_down() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let small = hash.truncate(20);
        assert_eq!(small.size(), 20);
    }

    #[test]
    #[cfg(feature = "sha2")]
    fn test_truncate_up() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let small = hash.truncate(100);
        assert_eq!(small.size(), 32);
    }

    #[test]
    #[cfg(feature = "sha2")]
    fn test_resize_fits() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let _: Multihash<32> = hash.resize().unwrap();
    }

    #[test]
    #[cfg(feature = "sha2")]
    fn test_resize_up() {
        let hash = Code::Sha2_256.digest(b"hello world");
        let _: Multihash<100> = hash.resize().unwrap();
    }

    #[test]
    #[cfg(feature = "sha2")]
    fn test_resize_truncate() {
        let hash = Code::Sha2_256.digest(b"hello world");
        hash.resize::<20>().unwrap_err();
    }
}
