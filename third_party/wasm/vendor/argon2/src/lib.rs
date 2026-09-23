#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![doc = include_str!("../README.md")]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/logo.svg",
    html_favicon_url = "https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/logo.svg"
)]
#![warn(
    clippy::cast_lossless,
    clippy::cast_possible_truncation,
    clippy::cast_possible_wrap,
    clippy::cast_precision_loss,
    clippy::cast_sign_loss,
    clippy::checked_conversions,
    clippy::implicit_saturating_sub,
    clippy::panic,
    clippy::panic_in_result_fn,
    clippy::unwrap_used,
    missing_docs,
    rust_2018_idioms,
    unused_lifetimes,
    unused_qualifications
)]

//! ## Usage
//!
//! ### Password Hashing
//!
//! This API hashes a password to a "PHC string" suitable for the purposes of
//! password-based authentication. Do not use this API to derive cryptographic
//! keys: see the "key derivation" usage example below.
//!
#![cfg_attr(feature = "std", doc = "```")]
#![cfg_attr(not(feature = "std"), doc = "```ignore")]
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! use argon2::{
//!     password_hash::{
//!         rand_core::OsRng,
//!         PasswordHash, PasswordHasher, PasswordVerifier, SaltString
//!     },
//!     Argon2
//! };
//!
//! let password = b"hunter42"; // Bad password; don't actually use!
//! let salt = SaltString::generate(&mut OsRng);
//!
//! // Argon2 with default params (Argon2id v19)
//! let argon2 = Argon2::default();
//!
//! // Hash password to PHC string ($argon2id$v=19$...)
//! let password_hash = argon2.hash_password(password, &salt)?.to_string();
//!
//! // Verify password against PHC string.
//! //
//! // NOTE: hash params from `parsed_hash` are used instead of what is configured in the
//! // `Argon2` instance.
//! let parsed_hash = PasswordHash::new(&password_hash)?;
//! assert!(Argon2::default().verify_password(password, &parsed_hash).is_ok());
//! # Ok(())
//! # }
//! ```
//!
//! ### Key Derivation
//!
//! This API is useful for transforming a password into cryptographic keys for
//! e.g. password-based encryption.
//!
#![cfg_attr(feature = "std", doc = "```")]
#![cfg_attr(not(feature = "std"), doc = "```ignore")]
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! use argon2::Argon2;
//!
//! let password = b"hunter42"; // Bad password; don't actually use!
//! let salt = b"example salt"; // Salt should be unique per password
//!
//! let mut output_key_material = [0u8; 32]; // Can be any desired size
//! Argon2::default().hash_password_into(password, salt, &mut output_key_material)?;
//! # Ok(())
//! # }
//! ```

// Call sites which cast `u32` to `usize` and are annotated with
// allow(clippy::cast_possible_truncation) need this check to avoid truncation.
#[cfg(not(any(target_pointer_width = "32", target_pointer_width = "64")))]
compile_error!("this crate builds on 32-bit and 64-bit platforms only");

#[cfg(feature = "alloc")]
#[macro_use]
extern crate alloc;

#[cfg(feature = "std")]
extern crate std;

mod algorithm;
mod blake2b_long;
mod block;
mod error;
mod params;
mod version;

pub use crate::{
    algorithm::Algorithm,
    block::Block,
    error::{Error, Result},
    params::{AssociatedData, KeyId, Params, ParamsBuilder},
    version::Version,
};

#[cfg(feature = "password-hash")]
#[cfg_attr(docsrs, doc(cfg(feature = "password-hash")))]
pub use {
    crate::algorithm::{ARGON2D_IDENT, ARGON2ID_IDENT, ARGON2I_IDENT},
    password_hash::{self, PasswordHash, PasswordHasher, PasswordVerifier},
};

use crate::blake2b_long::blake2b_long;
use blake2::{digest, Blake2b512, Digest};
use core::fmt;

#[cfg(all(feature = "alloc", feature = "password-hash"))]
use password_hash::{Decimal, Ident, ParamsString, Salt};

#[cfg(feature = "zeroize")]
use zeroize::Zeroize;

/// Maximum password length in bytes.
pub const MAX_PWD_LEN: usize = 0xFFFFFFFF;

/// Minimum salt length in bytes.
pub const MIN_SALT_LEN: usize = 8;

/// Maximum salt length in bytes.
pub const MAX_SALT_LEN: usize = 0xFFFFFFFF;

/// Recommended salt length for password hashing in bytes.
pub const RECOMMENDED_SALT_LEN: usize = 16;

/// Maximum secret key length in bytes.
pub const MAX_SECRET_LEN: usize = 0xFFFFFFFF;

/// Number of synchronization points between lanes per pass
pub(crate) const SYNC_POINTS: usize = 4;

/// To generate reference block positions
const ADDRESSES_IN_BLOCK: usize = 128;

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
cpufeatures::new!(avx2_cpuid, "avx2");

/// Argon2 context.
///
/// This is the primary type of this crate's API, and contains the following:
///
/// - Argon2 [`Algorithm`] variant to be used
/// - Argon2 [`Version`] to be used
/// - Default set of [`Params`] to be used
/// - (Optional) Secret key a.k.a. "pepper" to be used
#[derive(Clone)]
pub struct Argon2<'key> {
    /// Algorithm to use
    algorithm: Algorithm,

    /// Version number
    version: Version,

    /// Algorithm parameters
    params: Params,

    /// Key array
    secret: Option<&'key [u8]>,

    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    cpu_feat_avx2: avx2_cpuid::InitToken,
}

impl Default for Argon2<'_> {
    fn default() -> Self {
        Self::new(Algorithm::default(), Version::default(), Params::default())
    }
}

impl fmt::Debug for Argon2<'_> {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt.debug_struct("Argon2")
            .field("algorithm", &self.algorithm)
            .field("version", &self.version)
            .field("params", &self.params)
            .finish_non_exhaustive()
    }
}

impl<'key> Argon2<'key> {
    /// Create a new Argon2 context.
    pub fn new(algorithm: Algorithm, version: Version, params: Params) -> Self {
        Self {
            algorithm,
            version,
            params,
            secret: None,
            #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
            cpu_feat_avx2: avx2_cpuid::init(),
        }
    }

    /// Create a new Argon2 context.
    pub fn new_with_secret(
        secret: &'key [u8],
        algorithm: Algorithm,
        version: Version,
        params: Params,
    ) -> Result<Self> {
        if MAX_SECRET_LEN < secret.len() {
            return Err(Error::SecretTooLong);
        }

        Ok(Self {
            algorithm,
            version,
            params,
            secret: Some(secret),
            #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
            cpu_feat_avx2: avx2_cpuid::init(),
        })
    }

    /// Hash a password and associated parameters into the provided output buffer.
    #[cfg(feature = "alloc")]
    #[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
    pub fn hash_password_into(&self, pwd: &[u8], salt: &[u8], out: &mut [u8]) -> Result<()> {
        let mut blocks = vec![Block::default(); self.params.block_count()];
        self.hash_password_into_with_memory(pwd, salt, out, &mut blocks)
    }

    /// Hash a password and associated parameters into the provided output buffer.
    ///
    /// This method takes an explicit `memory_blocks` parameter which allows
    /// the caller to provide the backing storage for the algorithm's state:
    ///
    /// - Users with the `alloc` feature enabled can use [`Argon2::hash_password_into`]
    ///   to have it allocated for them.
    /// - `no_std` users on "heapless" targets can use an array of the [`Block`] type
    ///   to stack allocate this buffer.
    pub fn hash_password_into_with_memory(
        &self,
        pwd: &[u8],
        salt: &[u8],
        out: &mut [u8],
        mut memory_blocks: impl AsMut<[Block]>,
    ) -> Result<()> {
        // Validate output length
        if out.len() < self.params.output_len().unwrap_or(Params::MIN_OUTPUT_LEN) {
            return Err(Error::OutputTooShort);
        }

        if out.len() > self.params.output_len().unwrap_or(Params::MAX_OUTPUT_LEN) {
            return Err(Error::OutputTooLong);
        }

        Self::verify_inputs(pwd, salt)?;

        // Hashing all inputs
        let initial_hash = self.initial_hash(pwd, salt, out);

        self.fill_blocks(memory_blocks.as_mut(), initial_hash)?;
        self.finalize(memory_blocks.as_mut(), out)
    }

    /// Use a password and associated parameters only to fill the given memory blocks.
    ///
    /// This method omits the calculation of a hash and can be used when only the
    /// filled memory is required. It is not necessary to call this method
    /// before calling any of the hashing functions.
    pub fn fill_memory(
        &self,
        pwd: &[u8],
        salt: &[u8],
        mut memory_blocks: impl AsMut<[Block]>,
    ) -> Result<()> {
        Self::verify_inputs(pwd, salt)?;

        let initial_hash = self.initial_hash(pwd, salt, &[]);

        self.fill_blocks(memory_blocks.as_mut(), initial_hash)
    }

    #[allow(clippy::cast_possible_truncation, unused_mut)]
    fn fill_blocks(
        &self,
        memory_blocks: &mut [Block],
        mut initial_hash: digest::Output<Blake2b512>,
    ) -> Result<()> {
        let block_count = self.params.block_count();
        let memory_blocks = memory_blocks
            .get_mut(..block_count)
            .ok_or(Error::MemoryTooLittle)?;

        let segment_length = self.params.segment_length();
        let iterations = self.params.t_cost() as usize;
        let lane_length = self.params.lane_length();
        let lanes = self.params.lanes();

        // Initialize the first two blocks in each lane
        for (l, lane) in memory_blocks.chunks_exact_mut(lane_length).enumerate() {
            for (i, block) in lane[..2].iter_mut().enumerate() {
                let i = i as u32;
                let l = l as u32;

                // Make the first and second block in each lane as G(H0||0||i) or
                // G(H0||1||i)
                let inputs = &[
                    initial_hash.as_ref(),
                    &i.to_le_bytes()[..],
                    &l.to_le_bytes()[..],
                ];

                let mut hash = [0u8; Block::SIZE];
                blake2b_long(inputs, &mut hash)?;
                block.load(&hash);
            }
        }

        #[cfg(feature = "zeroize")]
        initial_hash.zeroize();

        // Run passes on blocks
        for pass in 0..iterations {
            for slice in 0..SYNC_POINTS {
                let data_independent_addressing = self.algorithm == Algorithm::Argon2i
                    || (self.algorithm == Algorithm::Argon2id
                        && pass == 0
                        && slice < SYNC_POINTS / 2);

                for lane in 0..lanes {
                    let mut address_block = Block::default();
                    let mut input_block = Block::default();
                    let zero_block = Block::default();

                    if data_independent_addressing {
                        input_block.as_mut()[..6].copy_from_slice(&[
                            pass as u64,
                            lane as u64,
                            slice as u64,
                            memory_blocks.len() as u64,
                            iterations as u64,
                            self.algorithm as u64,
                        ]);
                    }

                    let first_block = if pass == 0 && slice == 0 {
                        if data_independent_addressing {
                            // Generate first set of addresses
                            self.update_address_block(
                                &mut address_block,
                                &mut input_block,
                                &zero_block,
                            );
                        }

                        // The first two blocks of each lane are already initialized
                        2
                    } else {
                        0
                    };

                    let mut cur_index = lane * lane_length + slice * segment_length + first_block;
                    let mut prev_index = if slice == 0 && first_block == 0 {
                        // Last block in current lane
                        cur_index + lane_length - 1
                    } else {
                        // Previous block
                        cur_index - 1
                    };

                    // Fill blocks in the segment
                    for block in first_block..segment_length {
                        // Extract entropy
                        let rand = if data_independent_addressing {
                            let addres_index = block % ADDRESSES_IN_BLOCK;

                            if addres_index == 0 {
                                self.update_address_block(
                                    &mut address_block,
                                    &mut input_block,
                                    &zero_block,
                                );
                            }

                            address_block.as_ref()[addres_index]
                        } else {
                            memory_blocks[prev_index].as_ref()[0]
                        };

                        // Calculate source block index for compress function
                        let ref_lane = if pass == 0 && slice == 0 {
                            // Cannot reference other lanes yet
                            lane
                        } else {
                            (rand >> 32) as usize % lanes
                        };

                        let reference_area_size = if pass == 0 {
                            // First pass
                            if slice == 0 {
                                // First slice
                                block - 1 // all but the previous
                            } else if ref_lane == lane {
                                // The same lane => add current segment
                                slice * segment_length + block - 1
                            } else {
                                slice * segment_length - if block == 0 { 1 } else { 0 }
                            }
                        } else {
                            // Second pass
                            if ref_lane == lane {
                                lane_length - segment_length + block - 1
                            } else {
                                lane_length - segment_length - if block == 0 { 1 } else { 0 }
                            }
                        };

                        // 1.2.4. Mapping rand to 0..<reference_area_size-1> and produce
                        // relative position
                        let mut map = rand & 0xFFFFFFFF;
                        map = (map * map) >> 32;
                        let relative_position = reference_area_size
                            - 1
                            - ((reference_area_size as u64 * map) >> 32) as usize;

                        // 1.2.5 Computing starting position
                        let start_position = if pass != 0 && slice != SYNC_POINTS - 1 {
                            (slice + 1) * segment_length
                        } else {
                            0
                        };

                        let lane_index = (start_position + relative_position) % lane_length;
                        let ref_index = ref_lane * lane_length + lane_index;

                        // Calculate new block
                        let result =
                            self.compress(&memory_blocks[prev_index], &memory_blocks[ref_index]);

                        if self.version == Version::V0x10 || pass == 0 {
                            memory_blocks[cur_index] = result;
                        } else {
                            memory_blocks[cur_index] ^= &result;
                        };

                        prev_index = cur_index;
                        cur_index += 1;
                    }
                }
            }
        }

        Ok(())
    }

    fn compress(&self, rhs: &Block, lhs: &Block) -> Block {
        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        {
            /// Enable AVX2 optimizations.
            #[target_feature(enable = "avx2")]
            unsafe fn compress_avx2(rhs: &Block, lhs: &Block) -> Block {
                Block::compress(rhs, lhs)
            }

            if self.cpu_feat_avx2.get() {
                return unsafe { compress_avx2(rhs, lhs) };
            }
        }

        Block::compress(rhs, lhs)
    }

    /// Get default configured [`Params`].
    pub const fn params(&self) -> &Params {
        &self.params
    }

    fn finalize(&self, memory_blocks: &[Block], out: &mut [u8]) -> Result<()> {
        let lane_length = self.params.lane_length();

        let mut blockhash = memory_blocks[lane_length - 1];

        // XOR the last blocks
        for l in 1..self.params.lanes() {
            let last_block_in_lane = l * lane_length + (lane_length - 1);
            blockhash ^= &memory_blocks[last_block_in_lane];
        }

        // Hash the result
        let mut blockhash_bytes = [0u8; Block::SIZE];

        for (chunk, v) in blockhash_bytes.chunks_mut(8).zip(blockhash.iter()) {
            chunk.copy_from_slice(&v.to_le_bytes())
        }

        blake2b_long(&[&blockhash_bytes], out)?;

        #[cfg(feature = "zeroize")]
        {
            blockhash.zeroize();
            blockhash_bytes.zeroize();
        }

        Ok(())
    }

    fn update_address_block(
        &self,
        address_block: &mut Block,
        input_block: &mut Block,
        zero_block: &Block,
    ) {
        input_block.as_mut()[6] += 1;
        *address_block = self.compress(zero_block, input_block);
        *address_block = self.compress(zero_block, address_block);
    }

    /// Hashes all the inputs into `blockhash[PREHASH_DIGEST_LEN]`.
    #[allow(clippy::cast_possible_truncation)]
    fn initial_hash(&self, pwd: &[u8], salt: &[u8], out: &[u8]) -> digest::Output<Blake2b512> {
        let mut digest = Blake2b512::new();
        digest.update(self.params.p_cost().to_le_bytes());
        digest.update((out.len() as u32).to_le_bytes());
        digest.update(self.params.m_cost().to_le_bytes());
        digest.update(self.params.t_cost().to_le_bytes());
        digest.update(self.version.to_le_bytes());
        digest.update(self.algorithm.to_le_bytes());
        digest.update((pwd.len() as u32).to_le_bytes());
        digest.update(pwd);
        digest.update((salt.len() as u32).to_le_bytes());
        digest.update(salt);

        if let Some(secret) = &self.secret {
            digest.update((secret.len() as u32).to_le_bytes());
            digest.update(secret);
        } else {
            digest.update(0u32.to_le_bytes());
        }

        digest.update((self.params.data().len() as u32).to_le_bytes());
        digest.update(self.params.data());
        digest.finalize()
    }

    const fn verify_inputs(pwd: &[u8], salt: &[u8]) -> Result<()> {
        if pwd.len() > MAX_PWD_LEN {
            return Err(Error::PwdTooLong);
        }

        // Validate salt (required param)
        if salt.len() < MIN_SALT_LEN {
            return Err(Error::SaltTooShort);
        }

        if salt.len() > MAX_SALT_LEN {
            return Err(Error::SaltTooLong);
        }

        Ok(())
    }
}

#[cfg(all(feature = "alloc", feature = "password-hash"))]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
#[cfg_attr(docsrs, doc(cfg(feature = "password-hash")))]
impl PasswordHasher for Argon2<'_> {
    type Params = Params;

    fn hash_password<'a>(
        &self,
        password: &[u8],
        salt: impl Into<Salt<'a>>,
    ) -> password_hash::Result<PasswordHash<'a>> {
        let salt = salt.into();
        let mut salt_arr = [0u8; 64];
        let salt_bytes = salt.decode_b64(&mut salt_arr)?;

        let output_len = self
            .params
            .output_len()
            .unwrap_or(Params::DEFAULT_OUTPUT_LEN);

        let output = password_hash::Output::init_with(output_len, |out| {
            Ok(self.hash_password_into(password, salt_bytes, out)?)
        })?;

        Ok(PasswordHash {
            algorithm: self.algorithm.ident(),
            version: Some(self.version.into()),
            params: ParamsString::try_from(&self.params)?,
            salt: Some(salt),
            hash: Some(output),
        })
    }

    fn hash_password_customized<'a>(
        &self,
        password: &[u8],
        alg_id: Option<Ident<'a>>,
        version: Option<Decimal>,
        params: Params,
        salt: impl Into<Salt<'a>>,
    ) -> password_hash::Result<PasswordHash<'a>> {
        let algorithm = alg_id
            .map(Algorithm::try_from)
            .transpose()?
            .unwrap_or_default();

        let version = version
            .map(Version::try_from)
            .transpose()?
            .unwrap_or_default();

        let salt = salt.into();

        Self {
            secret: self.secret,
            algorithm,
            version,
            params,
            #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
            cpu_feat_avx2: self.cpu_feat_avx2,
        }
        .hash_password(password, salt)
    }
}

impl<'key> From<Params> for Argon2<'key> {
    fn from(params: Params) -> Self {
        Self::new(Algorithm::default(), Version::default(), params)
    }
}

impl<'key> From<&Params> for Argon2<'key> {
    fn from(params: &Params) -> Self {
        Self::from(params.clone())
    }
}

#[cfg(all(test, feature = "alloc", feature = "password-hash"))]
#[allow(clippy::unwrap_used)]
mod tests {
    use crate::{Algorithm, Argon2, Params, PasswordHasher, Salt, Version};

    /// Example password only: don't use this as a real password!!!
    const EXAMPLE_PASSWORD: &[u8] = b"hunter42";

    /// Example salt value. Don't use a static salt value!!!
    const EXAMPLE_SALT: &str = "examplesaltvalue";

    #[test]
    fn decoded_salt_too_short() {
        let argon2 = Argon2::default();

        // Too short after decoding
        let salt = Salt::from_b64("somesalt").unwrap();

        let res =
            argon2.hash_password_customized(EXAMPLE_PASSWORD, None, None, Params::default(), salt);
        assert_eq!(
            res,
            Err(password_hash::Error::SaltInvalid(
                password_hash::errors::InvalidValue::TooShort
            ))
        );
    }

    #[test]
    fn hash_simple_retains_configured_params() {
        // Non-default but valid parameters
        let t_cost = 4;
        let m_cost = 2048;
        let p_cost = 2;
        let version = Version::V0x10;

        let params = Params::new(m_cost, t_cost, p_cost, None).unwrap();
        let hasher = Argon2::new(Algorithm::default(), version, params);
        let salt = Salt::from_b64(EXAMPLE_SALT).unwrap();
        let hash = hasher.hash_password(EXAMPLE_PASSWORD, salt).unwrap();

        assert_eq!(hash.version.unwrap(), version.into());

        for &(param, value) in &[("t", t_cost), ("m", m_cost), ("p", p_cost)] {
            assert_eq!(
                hash.params
                    .get(param)
                    .and_then(|p| p.decimal().ok())
                    .unwrap(),
                value,
            );
        }
    }
}
