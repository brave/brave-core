//! The implementation of XXH3_64.

#![deny(
    clippy::missing_safety_doc,
    clippy::undocumented_unsafe_blocks,
    unsafe_op_in_unsafe_fn
)]

use core::hash;

use crate::{
    xxhash3::{primes::*, *},
    IntoU128 as _, IntoU64 as _,
};

pub use crate::xxhash3::{
    FixedBuffer, FixedMutBuffer, OneshotWithSecretError, SecretBuffer, SecretTooShortError,
    SecretWithSeedError, DEFAULT_SECRET_LENGTH, SECRET_MINIMUM_LENGTH,
};

/// Calculates the 64-bit hash.
#[derive(Clone)]
pub struct Hasher {
    #[cfg(feature = "alloc")]
    inner: AllocRawHasher,
    _private: (),
}

impl Hasher {
    /// Hash all data at once. If you can use this function, you may
    /// see noticable speed gains for certain types of input.
    #[must_use]
    #[inline]
    pub fn oneshot(input: &[u8]) -> u64 {
        impl_oneshot(DEFAULT_SECRET, DEFAULT_SEED, input)
    }

    /// Hash all data at once using the provided seed and a secret
    /// derived from the seed. If you can use this function, you may
    /// see noticable speed gains for certain types of input.
    #[must_use]
    #[inline]
    pub fn oneshot_with_seed(seed: u64, input: &[u8]) -> u64 {
        let mut secret = DEFAULT_SECRET_RAW;

        // We know that the secret will only be used if we have more
        // than 240 bytes, so don't waste time computing it otherwise.
        if input.len() > CUTOFF {
            derive_secret(seed, &mut secret);
        }

        let secret = Secret::new(&secret).expect("The default secret length is invalid");

        impl_oneshot(secret, seed, input)
    }

    /// Hash all data at once using the provided secret and the
    /// default seed. If you can use this function, you may see
    /// noticable speed gains for certain types of input.
    #[inline]
    pub fn oneshot_with_secret(secret: &[u8], input: &[u8]) -> Result<u64, OneshotWithSecretError> {
        let secret = Secret::new(secret).map_err(OneshotWithSecretError)?;
        Ok(impl_oneshot(secret, DEFAULT_SEED, input))
    }

    /// Hash all data at once using the provided seed and secret. If
    /// you can use this function, you may see noticable speed gains
    /// for certain types of input.
    #[inline]
    pub fn oneshot_with_seed_and_secret(
        seed: u64,
        secret: &[u8],
        input: &[u8],
    ) -> Result<u64, OneshotWithSecretError> {
        let secret = if input.len() > CUTOFF {
            Secret::new(secret).map_err(OneshotWithSecretError)?
        } else {
            DEFAULT_SECRET
        };

        Ok(impl_oneshot(secret, seed, input))
    }
}

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
mod with_alloc {
    use ::alloc::boxed::Box;

    use super::*;

    impl Hasher {
        /// Constructs the hasher using the default seed and secret values.
        pub fn new() -> Self {
            Self {
                inner: RawHasherCore::allocate_default(),
                _private: (),
            }
        }

        /// Constructs the hasher using the provided seed and a secret
        /// derived from the seed.
        pub fn with_seed(seed: u64) -> Self {
            Self {
                inner: RawHasherCore::allocate_with_seed(seed),
                _private: (),
            }
        }

        /// Constructs the hasher using the provided seed and secret.
        pub fn with_seed_and_secret(
            seed: u64,
            secret: impl Into<Box<[u8]>>,
        ) -> Result<Self, SecretTooShortError<Box<[u8]>>> {
            Ok(Self {
                inner: RawHasherCore::allocate_with_seed_and_secret(seed, secret)?,
                _private: (),
            })
        }

        /// Returns the secret.
        pub fn into_secret(self) -> Box<[u8]> {
            self.inner.into_secret()
        }
    }

    impl Default for Hasher {
        fn default() -> Self {
            Self::new()
        }
    }

    impl hash::Hasher for Hasher {
        #[inline]
        fn write(&mut self, input: &[u8]) {
            self.inner.write(input)
        }

        #[inline]
        fn finish(&self) -> u64 {
            self.inner.finish(Finalize64)
        }
    }
}

#[derive(Clone)]
/// A lower-level interface for computing a hash from streaming data.
///
/// The algorithm requires a secret which can be a reasonably large
/// piece of data. [`Hasher`][] makes one concrete implementation
/// decision that uses dynamic memory allocation, but specialized
/// usages may desire more flexibility. This type, combined with
/// [`SecretBuffer`][], offer that flexibility at the cost of a
/// generic type.
pub struct RawHasher<S>(RawHasherCore<S>);

impl<S> RawHasher<S> {
    /// Construct the hasher with the provided seed, secret, and
    /// temporary buffer.
    pub fn new(secret_buffer: SecretBuffer<S>) -> Self {
        Self(RawHasherCore::new(secret_buffer))
    }

    /// Returns the secret.
    pub fn into_secret(self) -> S {
        self.0.into_secret()
    }
}

impl<S> hash::Hasher for RawHasher<S>
where
    S: FixedBuffer,
{
    #[inline]
    fn write(&mut self, input: &[u8]) {
        self.0.write(input);
    }

    #[inline]
    fn finish(&self) -> u64 {
        self.0.finish(Finalize64)
    }
}

struct Finalize64;

impl Finalize for Finalize64 {
    type Output = u64;

    #[inline(always)]
    fn small(&self, secret: &Secret, seed: u64, input: &[u8]) -> Self::Output {
        impl_oneshot(secret, seed, input)
    }

    #[inline(always)]
    fn large(
        &self,
        vector: impl Vector,
        acc: [u64; 8],
        last_block: &[u8],
        last_stripe: &[u8; 64],
        secret: &Secret,
        len: usize,
    ) -> Self::Output {
        Algorithm(vector).finalize_64(acc, last_block, last_stripe, secret, len)
    }
}

#[inline(always)]
fn impl_oneshot(secret: &Secret, seed: u64, input: &[u8]) -> u64 {
    match input.len() {
        241.. => impl_241_plus_bytes(secret, input),

        129..=240 => impl_129_to_240_bytes(secret, seed, input),

        17..=128 => impl_17_to_128_bytes(secret, seed, input),

        9..=16 => impl_9_to_16_bytes(secret, seed, input),

        4..=8 => impl_4_to_8_bytes(secret, seed, input),

        1..=3 => impl_1_to_3_bytes(secret, seed, input),

        0 => impl_0_bytes(secret, seed),
    }
}

#[inline(always)]
fn impl_0_bytes(secret: &Secret, seed: u64) -> u64 {
    let secret_words = secret.for_64().words_for_0();
    avalanche_xxh64(seed ^ secret_words[0] ^ secret_words[1])
}

#[inline(always)]
fn impl_1_to_3_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u64 {
    assert_input_range!(1..=3, input.len());
    let combined = impl_1_to_3_bytes_combined(input);

    let secret_words = secret.for_64().words_for_1_to_3();

    let value = {
        let secret = (secret_words[0] ^ secret_words[1]).into_u64();
        secret.wrapping_add(seed) ^ combined.into_u64()
    };

    // FUTURE: TEST: "Note that the XXH3-64 result is the lower half of XXH3-128 result."
    avalanche_xxh64(value)
}

#[inline(always)]
fn impl_4_to_8_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u64 {
    assert_input_range!(4..=8, input.len());
    let input_first = input.first_u32().unwrap();
    let input_last = input.last_u32().unwrap();

    let modified_seed = seed ^ (seed.lower_half().swap_bytes().into_u64() << 32);
    let secret_words = secret.for_64().words_for_4_to_8();

    let combined = input_last.into_u64() | (input_first.into_u64() << 32);

    let mut value = {
        let a = secret_words[0] ^ secret_words[1];
        let b = a.wrapping_sub(modified_seed);
        b ^ combined
    };
    value ^= value.rotate_left(49) ^ value.rotate_left(24);
    value = value.wrapping_mul(PRIME_MX2);
    value ^= (value >> 35).wrapping_add(input.len().into_u64());
    value = value.wrapping_mul(PRIME_MX2);
    value ^= value >> 28;
    value
}

#[inline(always)]
fn impl_9_to_16_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u64 {
    assert_input_range!(9..=16, input.len());
    let input_first = input.first_u64().unwrap();
    let input_last = input.last_u64().unwrap();

    let secret_words = secret.for_64().words_for_9_to_16();
    let low = ((secret_words[0] ^ secret_words[1]).wrapping_add(seed)) ^ input_first;
    let high = ((secret_words[2] ^ secret_words[3]).wrapping_sub(seed)) ^ input_last;
    let mul_result = low.into_u128().wrapping_mul(high.into_u128());
    let value = input
        .len()
        .into_u64()
        .wrapping_add(low.swap_bytes())
        .wrapping_add(high)
        .wrapping_add(mul_result.lower_half() ^ mul_result.upper_half());

    avalanche(value)
}

#[inline]
fn impl_17_to_128_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u64 {
    assert_input_range!(17..=128, input.len());
    let mut acc = input.len().into_u64().wrapping_mul(PRIME64_1);

    impl_17_to_128_bytes_iter(secret, input, |fwd, bwd, secret| {
        acc = acc.wrapping_add(mix_step(fwd, &secret[0], seed));
        acc = acc.wrapping_add(mix_step(bwd, &secret[1], seed));
    });

    avalanche(acc)
}

#[inline]
fn impl_129_to_240_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u64 {
    assert_input_range!(129..=240, input.len());
    let mut acc = input.len().into_u64().wrapping_mul(PRIME64_1);

    let (head, _) = input.bp_as_chunks();
    let mut head = head.iter();

    let ss = secret.for_64().words_for_127_to_240_part1();
    for (chunk, secret) in head.by_ref().zip(ss).take(8) {
        acc = acc.wrapping_add(mix_step(chunk, secret, seed));
    }

    acc = avalanche(acc);

    let ss = secret.for_64().words_for_127_to_240_part2();
    for (chunk, secret) in head.zip(ss) {
        acc = acc.wrapping_add(mix_step(chunk, secret, seed));
    }

    let last_chunk = input.last_chunk().unwrap();
    let ss = secret.for_64().words_for_127_to_240_part3();
    acc = acc.wrapping_add(mix_step(last_chunk, ss, seed));

    avalanche(acc)
}

#[inline]
fn impl_241_plus_bytes(secret: &Secret, input: &[u8]) -> u64 {
    assert_input_range!(241.., input.len());
    dispatch! {
        fn oneshot_impl<>(secret: &Secret, input: &[u8]) -> u64
        []
    }
}

#[inline]
fn oneshot_impl(vector: impl Vector, secret: &Secret, input: &[u8]) -> u64 {
    Algorithm(vector).oneshot(secret, input, Finalize64)
}

#[cfg(test)]
mod test {
    use std::hash::Hasher as _;

    use crate::xxhash3::test::bytes;

    use super::*;

    const _: () = {
        const fn is_clone<T: Clone>() {}
        is_clone::<Hasher>();
    };

    const EMPTY_BYTES: [u8; 0] = [];

    fn hash_byte_by_byte(input: &[u8]) -> u64 {
        let mut hasher = Hasher::new();
        for byte in input.chunks(1) {
            hasher.write(byte)
        }
        hasher.finish()
    }

    fn hash_byte_by_byte_with_seed(seed: u64, input: &[u8]) -> u64 {
        let mut hasher = Hasher::with_seed(seed);
        for byte in input.chunks(1) {
            hasher.write(byte)
        }
        hasher.finish()
    }

    #[test]
    fn oneshot_empty() {
        let hash = Hasher::oneshot(&EMPTY_BYTES);
        assert_eq!(hash, 0x2d06_8005_38d3_94c2);
    }

    #[test]
    fn streaming_empty() {
        let hash = hash_byte_by_byte(&EMPTY_BYTES);
        assert_eq!(hash, 0x2d06_8005_38d3_94c2);
    }

    #[test]
    fn oneshot_1_to_3_bytes() {
        test_1_to_3_bytes(Hasher::oneshot)
    }

    #[test]
    fn streaming_1_to_3_bytes() {
        test_1_to_3_bytes(hash_byte_by_byte)
    }

    #[track_caller]
    fn test_1_to_3_bytes(mut f: impl FnMut(&[u8]) -> u64) {
        let inputs = bytes![1, 2, 3];

        let expected = [
            0xc44b_dff4_074e_ecdb,
            0xd664_5fc3_051a_9457,
            0x5f42_99fc_161c_9cbb,
        ];

        for (input, expected) in inputs.iter().zip(expected) {
            let hash = f(input);
            assert_eq!(hash, expected, "input was {} bytes", input.len());
        }
    }

    #[test]
    fn oneshot_4_to_8_bytes() {
        test_4_to_8_bytes(Hasher::oneshot)
    }

    #[test]
    fn streaming_4_to_8_bytes() {
        test_4_to_8_bytes(hash_byte_by_byte)
    }

    #[track_caller]
    fn test_4_to_8_bytes(mut f: impl FnMut(&[u8]) -> u64) {
        let inputs = bytes![4, 5, 6, 7, 8];

        let expected = [
            0x60da_b036_a582_11f2,
            0xb075_753a_84ca_0fbe,
            0xa658_4d1d_9a6a_e704,
            0x0cd2_084a_6240_6b69,
            0x3a1c_2d7c_85af_88f8,
        ];

        for (input, expected) in inputs.iter().zip(expected) {
            let hash = f(input);
            assert_eq!(hash, expected, "input was {} bytes", input.len());
        }
    }

    #[test]
    fn oneshot_9_to_16_bytes() {
        test_9_to_16_bytes(Hasher::oneshot)
    }

    #[test]
    fn streaming_9_to_16_bytes() {
        test_9_to_16_bytes(hash_byte_by_byte)
    }

    #[track_caller]
    fn test_9_to_16_bytes(mut f: impl FnMut(&[u8]) -> u64) {
        let inputs = bytes![9, 10, 11, 12, 13, 14, 15, 16];

        let expected = [
            0xe961_2598_145b_b9dc,
            0xab69_a08e_f83d_8f77,
            0x1cf3_96aa_4de6_198d,
            0x5ace_6a51_1c10_894b,
            0xb7a5_d8a8_309a_2cb9,
            0x4cf4_5c94_4a9a_2237,
            0x55ec_edc2_b87b_b042,
            0x8355_e3a6_f617_70db,
        ];

        for (input, expected) in inputs.iter().zip(expected) {
            let hash = f(input);
            assert_eq!(hash, expected, "input was {} bytes", input.len());
        }
    }

    #[test]
    fn oneshot_17_to_128_bytes() {
        test_17_to_128_bytes(Hasher::oneshot)
    }

    #[test]
    fn streaming_17_to_128_bytes() {
        test_17_to_128_bytes(hash_byte_by_byte)
    }

    #[track_caller]
    fn test_17_to_128_bytes(mut f: impl FnMut(&[u8]) -> u64) {
        let lower_boundary = bytes![17, 18, 19];
        let chunk_boundary = bytes![31, 32, 33];
        let upper_boundary = bytes![126, 127, 128];

        let inputs = lower_boundary
            .iter()
            .chain(chunk_boundary)
            .chain(upper_boundary);

        let expected = [
            // lower_boundary
            0x9ef3_41a9_9de3_7328,
            0xf691_2490_d4c0_eed5,
            0x60e7_2614_3cf5_0312,
            // chunk_boundary
            0x4f36_db8e_4df3_78fd,
            0x3523_581f_e96e_4c05,
            0xe68c_56ba_8899_1e58,
            // upper_boundary
            0x6c2a_9eb7_459c_dc61,
            0x120b_9787_f842_5f2f,
            0x85c6_174c_7ff4_c46b,
        ];

        for (input, expected) in inputs.zip(expected) {
            let hash = f(input);
            assert_eq!(hash, expected, "input was {} bytes", input.len());
        }
    }

    #[test]
    fn oneshot_129_to_240_bytes() {
        test_129_to_240_bytes(Hasher::oneshot)
    }

    #[test]
    fn streaming_129_to_240_bytes() {
        test_129_to_240_bytes(hash_byte_by_byte)
    }

    #[track_caller]
    fn test_129_to_240_bytes(mut f: impl FnMut(&[u8]) -> u64) {
        let lower_boundary = bytes![129, 130, 131];
        let upper_boundary = bytes![238, 239, 240];

        let inputs = lower_boundary.iter().chain(upper_boundary);

        let expected = [
            // lower_boundary
            0xec76_42b4_31ba_3e5a,
            0x4d32_24b1_0090_8a87,
            0xe57f_7ea6_741f_e3a0,
            // upper_boundary
            0x3044_9a0b_4899_dee9,
            0x972b_14e3_c46f_214b,
            0x375a_384d_957f_e865,
        ];

        for (input, expected) in inputs.zip(expected) {
            let hash = f(input);
            assert_eq!(hash, expected, "input was {} bytes", input.len());
        }
    }

    #[test]
    fn oneshot_241_plus_bytes() {
        test_241_plus_bytes(Hasher::oneshot)
    }

    #[test]
    fn streaming_241_plus_bytes() {
        test_241_plus_bytes(hash_byte_by_byte)
    }

    #[track_caller]
    fn test_241_plus_bytes(mut f: impl FnMut(&[u8]) -> u64) {
        let inputs = bytes![241, 242, 243, 244, 1024, 10240];

        let expected = [
            0x02e8_cd95_421c_6d02,
            0xddcb_33c4_9405_1832,
            0x8835_f952_9193_e3dc,
            0xbc17_c91e_c3cf_8d7f,
            0xe5d7_8baf_a45b_2aa5,
            0xbcd6_3266_df6e_2244,
        ];

        for (input, expected) in inputs.iter().zip(expected) {
            let hash = f(input);
            assert_eq!(hash, expected, "input was {} bytes", input.len());
        }
    }

    #[test]
    fn oneshot_with_seed() {
        test_with_seed(Hasher::oneshot_with_seed)
    }

    #[test]
    fn streaming_with_seed() {
        test_with_seed(hash_byte_by_byte_with_seed)
    }

    #[track_caller]
    fn test_with_seed(mut f: impl FnMut(u64, &[u8]) -> u64) {
        let inputs = bytes![0, 1, 4, 9, 17, 129, 241, 1024];

        let expected = [
            0x4aed_e683_89c0_e311,
            0x78fc_079a_75aa_f3c0,
            0x1b73_06b8_9f25_4507,
            0x7df7_627f_d1f9_39b6,
            0x49ca_0fff_0950_1622,
            0x2bfd_caec_30ff_3000,
            0xf984_56bc_25be_0901,
            0x2483_9f0f_cdf4_d078,
        ];

        for (input, expected) in inputs.iter().zip(expected) {
            let hash = f(0xdead_cafe, input);
            assert_eq!(hash, expected, "input was {} bytes", input.len());
        }
    }
}
