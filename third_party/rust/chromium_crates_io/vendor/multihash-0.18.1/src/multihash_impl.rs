pub use multihash_derive::Multihash;

/// Default (cryptographically secure) Multihash implementation.
///
/// This is a default set of hashing algorithms. Usually applications would use their own subset of
/// algorithms. See the [`Multihash` derive] for more information.
///
/// [`Multihash` derive]: crate::derive
#[cfg_attr(feature = "serde-codec", derive(serde::Deserialize))]
#[cfg_attr(feature = "serde-codec", derive(serde::Serialize))]
#[derive(Copy, Clone, Debug, Eq, Multihash, PartialEq)]
#[mh(alloc_size = 64)]
pub enum Code {
    /// SHA-256 (32-byte hash size)
    #[cfg(feature = "sha2")]
    #[mh(code = 0x12, hasher = crate::Sha2_256)]
    Sha2_256,
    /// SHA-512 (64-byte hash size)
    #[cfg(feature = "sha2")]
    #[mh(code = 0x13, hasher = crate::Sha2_512)]
    Sha2_512,
    /// SHA3-224 (28-byte hash size)
    #[cfg(feature = "sha3")]
    #[mh(code = 0x17, hasher = crate::Sha3_224)]
    Sha3_224,
    /// SHA3-256 (32-byte hash size)
    #[cfg(feature = "sha3")]
    #[mh(code = 0x16, hasher = crate::Sha3_256)]
    Sha3_256,
    /// SHA3-384 (48-byte hash size)
    #[cfg(feature = "sha3")]
    #[mh(code = 0x15, hasher = crate::Sha3_384)]
    Sha3_384,
    /// SHA3-512 (64-byte hash size)
    #[cfg(feature = "sha3")]
    #[mh(code = 0x14, hasher = crate::Sha3_512)]
    Sha3_512,
    /// Keccak-224 (28-byte hash size)
    #[cfg(feature = "sha3")]
    #[mh(code = 0x1a, hasher = crate::Keccak224)]
    Keccak224,
    /// Keccak-256 (32-byte hash size)
    #[cfg(feature = "sha3")]
    #[mh(code = 0x1b, hasher = crate::Keccak256)]
    Keccak256,
    /// Keccak-384 (48-byte hash size)
    #[cfg(feature = "sha3")]
    #[mh(code = 0x1c, hasher = crate::Keccak384)]
    Keccak384,
    /// Keccak-512 (64-byte hash size)
    #[cfg(feature = "sha3")]
    #[mh(code = 0x1d, hasher = crate::Keccak512)]
    Keccak512,
    /// BLAKE2b-256 (32-byte hash size)
    #[cfg(feature = "blake2b")]
    #[mh(code = 0xb220, hasher = crate::Blake2b256)]
    Blake2b256,
    /// BLAKE2b-512 (64-byte hash size)
    #[cfg(feature = "blake2b")]
    #[mh(code = 0xb240, hasher = crate::Blake2b512)]
    Blake2b512,
    /// BLAKE2s-128 (16-byte hash size)
    #[cfg(feature = "blake2s")]
    #[mh(code = 0xb250, hasher = crate::Blake2s128)]
    Blake2s128,
    /// BLAKE2s-256 (32-byte hash size)
    #[cfg(feature = "blake2s")]
    #[mh(code = 0xb260, hasher = crate::Blake2s256)]
    Blake2s256,
    /// BLAKE3-256 (32-byte hash size)
    #[cfg(feature = "blake3")]
    #[mh(code = 0x1e, hasher = crate::Blake3_256)]
    Blake3_256,
    /// RIPEMD-160 (20-byte hash size)
    #[cfg(feature = "ripemd")]
    #[mh(code = 0x1053, hasher = crate::Ripemd160)]
    Ripemd160,
    /// RIPEMD-256 (32-byte hash size)
    #[cfg(feature = "ripemd")]
    #[mh(code = 0x1054, hasher = crate::Ripemd256)]
    Ripemd256,
    /// RIPEMD-320 (40-byte hash size)
    #[cfg(feature = "ripemd")]
    #[mh(code = 0x1055, hasher = crate::Ripemd320)]
    Ripemd320,

    // The following hashes are not cryptographically secure hashes and are not enabled by default
    /// Identity hash (max. 64 bytes)
    #[cfg(feature = "identity")]
    #[mh(code = 0x00, hasher = crate::IdentityHasher::<64>)]
    Identity,
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::hasher::Hasher;
    use crate::hasher_impl::sha3::{Sha3_256, Sha3_512};
    use crate::multihash::MultihashDigest;

    #[test]
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
}
