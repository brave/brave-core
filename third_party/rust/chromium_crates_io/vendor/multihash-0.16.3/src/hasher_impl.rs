use crate::hasher::Hasher;

#[cfg(feature = "std")]
use std::io;

#[cfg(not(feature = "std"))]
use core2::io;

macro_rules! derive_write {
    ($name:ident) => {
        impl<const S: usize> io::Write for $name<S> {
            fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
                self.update(buf);
                Ok(buf.len())
            }

            fn flush(&mut self) -> io::Result<()> {
                Ok(())
            }
        }
    };
}

#[cfg(any(feature = "blake2b", feature = "blake2s"))]
macro_rules! derive_hasher_blake {
    ($module:ident, $name:ident) => {
        /// Multihash hasher.
        #[derive(Debug)]
        pub struct $name<const S: usize> {
            state: $module::State,
            digest: [u8; S],
        }

        impl<const S: usize> Default for $name<S> {
            fn default() -> Self {
                let mut params = $module::Params::new();
                params.hash_length(S);
                Self {
                    state: params.to_state(),
                    digest: [0; S],
                }
            }
        }

        impl<const S: usize> Hasher for $name<S> {
            fn update(&mut self, input: &[u8]) {
                self.state.update(input);
            }

            fn finalize(&mut self) -> &[u8] {
                let digest = self.state.finalize();
                let digest_bytes = digest.as_bytes();
                let digest_out = &mut self.digest[..digest_bytes.len().max(S)];
                digest_out.copy_from_slice(digest_bytes);
                digest_out
            }

            fn reset(&mut self) {
                let Self { state, .. } = Self::default();
                self.state = state;
            }
        }

        derive_write!($name);
    };
}

#[cfg(feature = "blake2b")]
pub mod blake2b {
    use super::*;

    derive_hasher_blake!(blake2b_simd, Blake2bHasher);

    /// 256 bit blake2b hasher.
    pub type Blake2b256 = Blake2bHasher<32>;

    /// 512 bit blake2b hasher.
    pub type Blake2b512 = Blake2bHasher<64>;
}

#[cfg(feature = "blake2s")]
pub mod blake2s {
    use super::*;

    derive_hasher_blake!(blake2s_simd, Blake2sHasher);

    /// 256 bit blake2b hasher.
    pub type Blake2s128 = Blake2sHasher<16>;

    /// 512 bit blake2b hasher.
    pub type Blake2s256 = Blake2sHasher<32>;
}

#[cfg(feature = "blake3")]
pub mod blake3 {
    use super::*;

    /// Multihash hasher.
    #[derive(Debug)]
    pub struct Blake3Hasher<const S: usize> {
        hasher: ::blake3::Hasher,
        digest: [u8; S],
    }

    impl<const S: usize> Blake3Hasher<S> {
        /// using blake3's XOF function, fills the given slice with hash output
        pub fn finalize_xof_fill(&mut self, digest_out: &mut [u8]) {
            let mut digest = self.hasher.finalize_xof();
            digest.fill(digest_out)
        }
    }

    impl<const S: usize> Default for Blake3Hasher<S> {
        fn default() -> Self {
            let hasher = ::blake3::Hasher::new();

            Self {
                hasher,
                digest: [0; S],
            }
        }
    }

    impl<const S: usize> Hasher for Blake3Hasher<S> {
        fn update(&mut self, input: &[u8]) {
            self.hasher.update(input);
        }

        fn finalize(&mut self) -> &[u8] {
            let mut output = self.hasher.finalize_xof();
            output.fill(&mut self.digest);
            &self.digest
        }

        fn reset(&mut self) {
            self.hasher.reset();
        }
    }

    derive_write!(Blake3Hasher);

    /// blake3-256 hasher.
    pub type Blake3_256 = Blake3Hasher<32>;
}

#[cfg(feature = "digest")]
macro_rules! derive_rustcrypto_hasher {
    ($module:ty, $name:ident, $size:expr) => {
        /// Multihash hasher.
        #[derive(Debug)]
        pub struct $name {
            state: $module,
            digest: [u8; $size],
        }

        impl Default for $name {
            fn default() -> Self {
                $name {
                    state: Default::default(),
                    digest: [0; $size],
                }
            }
        }

        impl $crate::hasher::Hasher for $name {
            fn update(&mut self, input: &[u8]) {
                use digest::Digest;
                self.state.update(input)
            }

            fn finalize(&mut self) -> &[u8] {
                use digest::Digest;
                let digest = self.state.clone().finalize();
                let digest_bytes = digest.as_slice();
                let digest_out = &mut self.digest[..digest_bytes.len().max($size)];
                digest_out.copy_from_slice(digest_bytes);
                digest_out
            }

            fn reset(&mut self) {
                use digest::Digest;
                self.state.reset();
            }
        }

        impl io::Write for $name {
            fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
                self.update(buf);
                Ok(buf.len())
            }

            fn flush(&mut self) -> io::Result<()> {
                Ok(())
            }
        }
    };
}

#[cfg(feature = "sha1")]
pub mod sha1 {
    use super::*;

    derive_rustcrypto_hasher!(::sha1::Sha1, Sha1, 20);
}

#[cfg(feature = "sha2")]
pub mod sha2 {
    use super::*;

    derive_rustcrypto_hasher!(sha_2::Sha256, Sha2_256, 32);
    derive_rustcrypto_hasher!(sha_2::Sha512, Sha2_512, 64);
}

#[cfg(feature = "sha3")]
pub mod sha3 {
    use super::*;

    derive_rustcrypto_hasher!(sha_3::Sha3_224, Sha3_224, 28);
    derive_rustcrypto_hasher!(sha_3::Sha3_256, Sha3_256, 32);
    derive_rustcrypto_hasher!(sha_3::Sha3_384, Sha3_384, 48);
    derive_rustcrypto_hasher!(sha_3::Sha3_512, Sha3_512, 64);

    derive_rustcrypto_hasher!(sha_3::Keccak224, Keccak224, 28);
    derive_rustcrypto_hasher!(sha_3::Keccak256, Keccak256, 32);
    derive_rustcrypto_hasher!(sha_3::Keccak384, Keccak384, 48);
    derive_rustcrypto_hasher!(sha_3::Keccak512, Keccak512, 64);
}

#[cfg(feature = "ripemd")]
pub mod ripemd {

    use super::*;

    derive_rustcrypto_hasher!(ripemd_rs::Ripemd160, Ripemd160, 20);
    derive_rustcrypto_hasher!(ripemd_rs::Ripemd256, Ripemd256, 32);
    derive_rustcrypto_hasher!(ripemd_rs::Ripemd320, Ripemd320, 40);
}

pub mod identity {
    use super::*;

    /// Identity hasher with a maximum size.
    ///
    /// # Panics
    ///
    /// Panics if the input is bigger than the maximum size.
    #[derive(Debug)]
    pub struct IdentityHasher<const S: usize> {
        i: usize,
        bytes: [u8; S],
    }

    impl<const S: usize> Default for IdentityHasher<S> {
        fn default() -> Self {
            Self {
                i: 0,
                bytes: [0u8; S],
            }
        }
    }

    impl<const S: usize> Hasher for IdentityHasher<S> {
        fn update(&mut self, input: &[u8]) {
            let start = self.i.min(self.bytes.len());
            let end = (self.i + input.len()).min(self.bytes.len());
            self.bytes[start..end].copy_from_slice(input);
            self.i = end;
        }

        fn finalize(&mut self) -> &[u8] {
            &self.bytes[..self.i]
        }

        fn reset(&mut self) {
            self.i = 0
        }
    }

    derive_write!(IdentityHasher);

    /// 32 byte Identity hasher (constrained to 32 bytes).
    ///
    /// # Panics
    ///
    /// Panics if the input is bigger than 32 bytes.
    pub type Identity256 = IdentityHasher<32>;
}

#[cfg(feature = "strobe")]
pub mod strobe {
    use super::*;
    use strobe_rs::{SecParam, Strobe};

    /// Strobe hasher.
    pub struct StrobeHasher<const S: usize> {
        strobe: Strobe,
        initialized: bool,
        digest: [u8; S],
    }

    impl<const S: usize> Default for StrobeHasher<S> {
        fn default() -> Self {
            Self {
                strobe: Strobe::new(b"StrobeHash", SecParam::B128),
                initialized: false,
                digest: [0; S],
            }
        }
    }

    impl<const S: usize> Hasher for StrobeHasher<S> {
        fn update(&mut self, input: &[u8]) {
            self.strobe.ad(input, self.initialized);
            self.initialized = true;
        }

        fn finalize(&mut self) -> &[u8] {
            self.strobe.clone().prf(&mut self.digest, false);
            &self.digest
        }

        fn reset(&mut self) {
            let Self { strobe, .. } = Self::default();
            self.strobe = strobe;
            self.initialized = false;
        }
    }

    derive_write!(StrobeHasher);

    /// 256 bit strobe hasher.
    pub type Strobe256 = StrobeHasher<32>;

    /// 512 bit strobe hasher.
    pub type Strobe512 = StrobeHasher<64>;
}
