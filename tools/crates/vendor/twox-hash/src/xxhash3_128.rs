//! The implementation of XXH3_128.

#![deny(
    clippy::missing_safety_doc,
    clippy::undocumented_unsafe_blocks,
    unsafe_op_in_unsafe_fn
)]

use crate::{
    xxhash3::{primes::*, *},
    IntoU128 as _, IntoU64 as _,
};

pub use crate::xxhash3::{
    FixedBuffer, FixedMutBuffer, OneshotWithSecretError, SecretBuffer, SecretTooShortError,
    SecretWithSeedError, DEFAULT_SECRET_LENGTH, SECRET_MINIMUM_LENGTH,
};

/// Calculates the 128-bit hash.
///
/// This type does not implement [`std::hash::Hasher`] as that trait
/// requires a 64-bit result while this computes a 128-bit result.
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
    pub fn oneshot(input: &[u8]) -> u128 {
        impl_oneshot(DEFAULT_SECRET, DEFAULT_SEED, input)
    }

    /// Hash all data at once using the provided seed and a secret
    /// derived from the seed. If you can use this function, you may
    /// see noticable speed gains for certain types of input.
    #[must_use]
    #[inline]
    pub fn oneshot_with_seed(seed: u64, input: &[u8]) -> u128 {
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
    pub fn oneshot_with_secret(
        secret: &[u8],
        input: &[u8],
    ) -> Result<u128, OneshotWithSecretError> {
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
    ) -> Result<u128, OneshotWithSecretError> {
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

        /// Writes some data into this `Hasher`.
        #[inline]
        pub fn write(&mut self, input: &[u8]) {
            self.inner.write(input);
        }

        /// Returns the hash value for the values written so
        /// far. Unlike [`std::hash::Hasher::finish`][], this method
        /// returns the complete 128-bit value calculated, not a
        /// 64-bit value.
        #[inline]
        pub fn finish_128(&self) -> u128 {
            self.inner.finish(Finalize128)
        }
    }

    impl Default for Hasher {
        fn default() -> Self {
            Self::new()
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

impl<S> RawHasher<S>
where
    S: FixedBuffer,
{
    /// Writes some data into this `Hasher`.
    #[inline]
    pub fn write(&mut self, input: &[u8]) {
        self.0.write(input);
    }

    /// Returns the hash value for the values written so
    /// far. Unlike [`std::hash::Hasher::finish`][], this method
    /// returns the complete 128-bit value calculated, not a
    /// 64-bit value.
    #[inline]
    pub fn finish_128(&self) -> u128 {
        self.0.finish(Finalize128)
    }
}

struct Finalize128;

impl Finalize for Finalize128 {
    type Output = u128;

    #[inline]
    fn small(&self, secret: &Secret, seed: u64, input: &[u8]) -> Self::Output {
        impl_oneshot(secret, seed, input)
    }

    #[inline]
    fn large(
        &self,
        vector: impl Vector,
        acc: [u64; 8],
        last_block: &[u8],
        last_stripe: &[u8; 64],
        secret: &Secret,
        len: usize,
    ) -> Self::Output {
        Algorithm(vector).finalize_128(acc, last_block, last_stripe, secret, len)
    }
}

#[inline(always)]
fn impl_oneshot(secret: &Secret, seed: u64, input: &[u8]) -> u128 {
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
fn impl_0_bytes(secret: &Secret, seed: u64) -> u128 {
    let secret_words = secret.for_128().words_for_0();

    let low = avalanche_xxh64(seed ^ secret_words[0] ^ secret_words[1]);
    let high = avalanche_xxh64(seed ^ secret_words[2] ^ secret_words[3]);

    X128 { low, high }.into()
}

#[inline(always)]
fn impl_1_to_3_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u128 {
    assert_input_range!(1..=3, input.len());

    let combined = impl_1_to_3_bytes_combined(input);
    let secret_words = secret.for_128().words_for_1_to_3();

    let low = {
        let secret = (secret_words[0] ^ secret_words[1]).into_u64();
        secret.wrapping_add(seed) ^ combined.into_u64()
    };
    let high = {
        let secret = (secret_words[2] ^ secret_words[3]).into_u64();
        secret.wrapping_sub(seed) ^ combined.swap_bytes().rotate_left(13).into_u64()
    };

    let low = avalanche_xxh64(low);
    let high = avalanche_xxh64(high);

    X128 { low, high }.into()
}

#[inline(always)]
fn impl_4_to_8_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u128 {
    assert_input_range!(4..=8, input.len());
    let input_first = input.first_u32().unwrap();
    let input_last = input.last_u32().unwrap();

    let modified_seed = seed ^ (seed.lower_half().swap_bytes().into_u64() << 32);
    let secret_words = secret.for_128().words_for_4_to_8();

    let combined = input_first.into_u64() | (input_last.into_u64() << 32);
    let lhs = {
        let a = secret_words[0] ^ secret_words[1];
        let b = a.wrapping_add(modified_seed);
        b ^ combined
    };
    let rhs = PRIME64_1.wrapping_add(input.len().into_u64() << 2);
    let mul_result = lhs.into_u128().wrapping_mul(rhs.into_u128());

    let mut high = mul_result.upper_half();
    let mut low = mul_result.lower_half();

    high = high.wrapping_add(low << 1);

    low ^= high >> 3;
    low ^= low >> 35;
    low = low.wrapping_mul(PRIME_MX2);
    low ^= low >> 28;

    high = avalanche(high);

    X128 { low, high }.into()
}

#[inline(always)]
fn impl_9_to_16_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u128 {
    assert_input_range!(9..=16, input.len());
    let input_first = input.first_u64().unwrap();
    let input_last = input.last_u64().unwrap();

    let secret_words = secret.for_128().words_for_9_to_16();
    let val1 = ((secret_words[0] ^ secret_words[1]).wrapping_sub(seed)) ^ input_first ^ input_last;
    let val2 = ((secret_words[2] ^ secret_words[3]).wrapping_add(seed)) ^ input_last;
    let mul_result = val1.into_u128().wrapping_mul(PRIME64_1.into_u128());
    let low = mul_result
        .lower_half()
        .wrapping_add((input.len() - 1).into_u64() << 54);

    // Algorithm describes this in two ways
    let high = mul_result
        .upper_half()
        .wrapping_add(val2.upper_half().into_u64() << 32)
        .wrapping_add(val2.lower_half().into_u64().wrapping_mul(PRIME32_2));

    let low = low ^ high.swap_bytes();

    // Algorithm describes this multiplication in two ways.
    let q = X128 { low, high }
        .into_u128()
        .wrapping_mul(PRIME64_2.into_u128());

    let low = avalanche(q.lower_half());
    let high = avalanche(q.upper_half());

    X128 { low, high }.into()
}

#[inline]
fn impl_17_to_128_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u128 {
    assert_input_range!(17..=128, input.len());
    let input_len = input.len().into_u64();
    let mut acc = [input_len.wrapping_mul(PRIME64_1), 0];

    impl_17_to_128_bytes_iter(secret, input, |fwd, bwd, secret| {
        mix_two_chunks(&mut acc, fwd, bwd, secret, seed);
    });

    finalize_medium(acc, input_len, seed)
}

#[inline]
fn impl_129_to_240_bytes(secret: &Secret, seed: u64, input: &[u8]) -> u128 {
    assert_input_range!(129..=240, input.len());
    let input_len = input.len().into_u64();
    let mut acc = [input_len.wrapping_mul(PRIME64_1), 0];

    let head = pairs_of_u64_bytes(input);
    let mut head = head.iter();

    let ss = secret.for_128().words_for_127_to_240_part1();
    for (input, secret) in head.by_ref().zip(ss).take(4) {
        mix_two_chunks(&mut acc, &input[0], &input[1], secret, seed);
    }

    let mut acc = acc.map(avalanche);

    let ss = secret.for_128().words_for_127_to_240_part2();
    for (input, secret) in head.zip(ss) {
        mix_two_chunks(&mut acc, &input[0], &input[1], secret, seed);
    }

    let (_, tail) = input.bp_as_rchunks::<16>();
    let (_, tail) = tail.bp_as_rchunks::<2>();
    let tail = tail.last().unwrap();
    let ss = secret.for_128().words_for_127_to_240_part3();

    // note that the half-chunk order and the seed is different here
    mix_two_chunks(&mut acc, &tail[1], &tail[0], ss, seed.wrapping_neg());

    finalize_medium(acc, input_len, seed)
}

#[inline]
fn mix_two_chunks(
    acc: &mut [u64; 2],
    data1: &[u8; 16],
    data2: &[u8; 16],
    secret: &[[u8; 16]; 2],
    seed: u64,
) {
    let data_words1 = to_u64s(data1);
    let data_words2 = to_u64s(data2);

    acc[0] = acc[0].wrapping_add(mix_step(data1, &secret[0], seed));
    acc[1] = acc[1].wrapping_add(mix_step(data2, &secret[1], seed));
    acc[0] ^= data_words2[0].wrapping_add(data_words2[1]);
    acc[1] ^= data_words1[0].wrapping_add(data_words1[1]);
}

#[inline]
fn finalize_medium(acc: [u64; 2], input_len: u64, seed: u64) -> u128 {
    let low = acc[0].wrapping_add(acc[1]);
    let high = acc[0]
        .wrapping_mul(PRIME64_1)
        .wrapping_add(acc[1].wrapping_mul(PRIME64_4))
        .wrapping_add((input_len.wrapping_sub(seed)).wrapping_mul(PRIME64_2));

    let low = avalanche(low);
    let high = avalanche(high).wrapping_neg();

    X128 { low, high }.into()
}

#[inline]
fn impl_241_plus_bytes(secret: &Secret, input: &[u8]) -> u128 {
    assert_input_range!(241.., input.len());
    dispatch! {
        fn oneshot_impl<>(secret: &Secret, input: &[u8]) -> u128
        []
    }
}

#[inline]
fn oneshot_impl(vector: impl Vector, secret: &Secret, input: &[u8]) -> u128 {
    Algorithm(vector).oneshot(secret, input, Finalize128)
}

#[cfg(test)]
mod test {
    use crate::xxhash3::test::bytes;

    use super::*;

    const _: () = {
        const fn is_clone<T: Clone>() {}
        is_clone::<Hasher>();
    };

    const EMPTY_BYTES: [u8; 0] = [];

    fn hash_byte_by_byte(input: &[u8]) -> u128 {
        let mut hasher = Hasher::new();
        for byte in input.chunks(1) {
            hasher.write(byte)
        }
        hasher.finish_128()
    }

    #[test]
    fn oneshot_empty() {
        let hash = Hasher::oneshot(&EMPTY_BYTES);
        assert_eq!(hash, 0x99aa_06d3_0147_98d8_6001_c324_468d_497f);
    }

    #[test]
    fn streaming_empty() {
        let hash = hash_byte_by_byte(&EMPTY_BYTES);
        assert_eq!(hash, 0x99aa_06d3_0147_98d8_6001_c324_468d_497f);
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
    fn test_1_to_3_bytes(mut f: impl FnMut(&[u8]) -> u128) {
        let inputs = bytes![1, 2, 3];

        let expected = [
            0xa6cd_5e93_9200_0f6a_c44b_dff4_074e_ecdb,
            0x6a4a_5274_c1b0_d3ad_d664_5fc3_051a_9457,
            0xe3b5_5f57_945a_17cf_5f42_99fc_161c_9cbb,
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
    fn test_4_to_8_bytes(mut f: impl FnMut(&[u8]) -> u128) {
        let inputs = bytes![4, 5, 6, 7, 8];

        let expected = [
            0xeb70_bf5f_c779_e9e6_a611_1d53_e80a_3db5,
            0x9434_5321_06a7_c141_c920_d234_7a85_929b,
            0x545f_093d_32b1_68fe_a6b5_2f4d_ea38_96a3,
            0x61ce_291b_c3a4_357d_dbb2_0782_1e6d_5efe,
            0xe1e4_432a_6221_7fe4_cfd5_0c61_c8bb_98c1,
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
    fn test_9_to_16_bytes(mut f: impl FnMut(&[u8]) -> u128) {
        let inputs = bytes![9, 10, 11, 12, 13, 14, 15, 16];

        let expected = [
            0x16c7_69d8_3e4a_ebce_9079_3197_9dca_3746,
            0xbd93_0669_a87b_4b37_e67b_f1ad_8dcf_73a8,
            0xacad_8071_8f47_d494_7d67_cfc1_730f_22a3,
            0x38f9_2247_a7f7_3cc5_7780_eb31_198f_13ca,
            0xae92_e123_e947_2408_bd79_5526_1902_66c0,
            0x5f91_e6bf_7418_cfaa_55d6_5715_e2a5_7c31,
            0x301a_9f75_4e8f_569a_0017_ea4b_e19b_c787,
            0x7295_0631_8276_07e2_8428_12cc_870d_cae2,
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
    fn test_17_to_128_bytes(mut f: impl FnMut(&[u8]) -> u128) {
        let lower_boundary = bytes![17, 18, 19];
        let chunk_boundary = bytes![31, 32, 33];
        let upper_boundary = bytes![126, 127, 128];

        let inputs = lower_boundary
            .iter()
            .chain(chunk_boundary)
            .chain(upper_boundary);

        let expected = [
            // lower_boundary
            0x685b_c458_b37d_057f_c06e_233d_f772_9217,
            0x87ce_996b_b557_6d8d_e3a3_c96b_b0af_2c23,
            0x7619_bcef_2e31_1cd8_c47d_dc58_8737_93df,
            // chunk_boundary
            0x4ed3_946d_393b_687b_b54d_e399_3874_ed20,
            0x25e7_c9b3_424c_eed2_457d_9566_b6fc_d697,
            0x0217_5c3a_abb0_0637_e08d_8495_1339_de86,
            // upper_boundary
            0x0abc_2062_87ce_2afe_5181_0be2_9323_2106,
            0xd5ad_d870_c9c9_e00f_060c_2e3d_df0f_2fb9,
            0x1479_2fc3_af88_dc6c_0532_1a0b_64d6_7b41,
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
    fn test_129_to_240_bytes(mut f: impl FnMut(&[u8]) -> u128) {
        let lower_boundary = bytes![129, 130, 131];
        let upper_boundary = bytes![238, 239, 240];

        let inputs = lower_boundary.iter().chain(upper_boundary);

        let expected = [
            // lower_boundary
            0xdd5e_74ac_6b45_f54e_bc30_b633_82b0_9a3b,
            0x6cd2_e56a_10f1_e707_3ec5_f135_d0a7_d28f,
            0x6da7_92f1_702d_4494_5609_cfc7_9dba_18fd,
            // upper_boundary
            0x73a9_e8f7_bd32_83c8_2a9b_ddd0_e5c4_014c,
            0x9843_ab31_a06b_e0df_fe21_3746_28fc_c539,
            0x65b5_be86_da55_40e7_c92b_68e1_6f83_bbb6,
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
    fn test_241_plus_bytes(mut f: impl FnMut(&[u8]) -> u128) {
        let inputs = bytes![241, 242, 243, 244, 1024, 10240];

        let expected = [
            0x1da1_cb61_bcb8_a2a1_02e8_cd95_421c_6d02,
            0x1623_84cb_44d1_d806_ddcb_33c4_9405_1832,
            0xbd2e_9fcf_378c_35e9_8835_f952_9193_e3dc,
            0x3ff4_93d7_a813_7ab6_bc17_c91e_c3cf_8d7f,
            0xd0ac_1f7b_93bf_57b9_e5d7_8baf_a45b_2aa5,
            0x4f63_75cc_a7ec_e1e1_bcd6_3266_df6e_2244,
        ];

        for (input, expected) in inputs.iter().zip(expected) {
            let hash = f(input);
            eprintln!("{hash:032x}\n{expected:032x}");
            assert_eq!(hash, expected, "input was {} bytes", input.len());
        }
    }
}
