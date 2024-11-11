#[cfg(any(
    feature = "strobe",
    feature = "blake2b",
    feature = "blake2s",
    feature = "blake3"
))]
macro_rules! derive_write {
    ($name:ident) => {
        impl<const S: usize> core2::io::Write for $name<S> {
            fn write(&mut self, buf: &[u8]) -> core2::io::Result<usize> {
                use multihash_derive::Hasher as _;

                self.update(buf);
                Ok(buf.len())
            }

            fn flush(&mut self) -> core2::io::Result<()> {
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

        impl<const S: usize> multihash_derive::Hasher for $name<S> {
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
    derive_hasher_blake!(blake2b_simd, Blake2bHasher);

    /// 256 bit blake2b hasher.
    pub type Blake2b256 = Blake2bHasher<32>;

    /// 512 bit blake2b hasher.
    pub type Blake2b512 = Blake2bHasher<64>;
}

#[cfg(feature = "blake2s")]
pub mod blake2s {
    derive_hasher_blake!(blake2s_simd, Blake2sHasher);

    /// 256 bit blake2s hasher.
    pub type Blake2s128 = Blake2sHasher<16>;

    /// 512 bit blake2s hasher.
    pub type Blake2s256 = Blake2sHasher<32>;
}

#[cfg(feature = "blake3")]
pub mod blake3 {
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

    impl<const S: usize> multihash_derive::Hasher for Blake3Hasher<S> {
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

#[cfg(any(
    feature = "sha1",
    feature = "sha2",
    feature = "sha3",
    feature = "ripemd"
))]
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

        impl ::multihash_derive::Hasher for $name {
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

        impl core2::io::Write for $name {
            fn write(&mut self, buf: &[u8]) -> core2::io::Result<usize> {
                use multihash_derive::Hasher as _;

                self.update(buf);
                Ok(buf.len())
            }

            fn flush(&mut self) -> core2::io::Result<()> {
                Ok(())
            }
        }
    };
}

#[cfg(feature = "sha1")]
pub mod sha1 {
    derive_rustcrypto_hasher!(::sha1::Sha1, Sha1, 20);
}

#[cfg(feature = "sha2")]
pub mod sha2 {
    derive_rustcrypto_hasher!(::sha2::Sha256, Sha2_256, 32);
    derive_rustcrypto_hasher!(::sha2::Sha512, Sha2_512, 64);
}

#[cfg(feature = "sha3")]
pub mod sha3 {
    derive_rustcrypto_hasher!(::sha3::Sha3_224, Sha3_224, 28);
    derive_rustcrypto_hasher!(::sha3::Sha3_256, Sha3_256, 32);
    derive_rustcrypto_hasher!(::sha3::Sha3_384, Sha3_384, 48);
    derive_rustcrypto_hasher!(::sha3::Sha3_512, Sha3_512, 64);

    derive_rustcrypto_hasher!(::sha3::Keccak224, Keccak224, 28);
    derive_rustcrypto_hasher!(::sha3::Keccak256, Keccak256, 32);
    derive_rustcrypto_hasher!(::sha3::Keccak384, Keccak384, 48);
    derive_rustcrypto_hasher!(::sha3::Keccak512, Keccak512, 64);
}

#[cfg(feature = "ripemd")]
pub mod ripemd {
    derive_rustcrypto_hasher!(::ripemd::Ripemd160, Ripemd160, 20);
    derive_rustcrypto_hasher!(::ripemd::Ripemd256, Ripemd256, 32);
    derive_rustcrypto_hasher!(::ripemd::Ripemd320, Ripemd320, 40);
}

#[cfg(feature = "strobe")]
pub mod strobe {
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

    impl<const S: usize> multihash_derive::Hasher for StrobeHasher<S> {
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
