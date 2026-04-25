use core::slice;

use crate::{IntoU128 as _, IntoU32 as _};

pub mod large;

pub(crate) use large::dispatch;
pub use large::{Algorithm, Vector};

pub mod secret;

pub use secret::{Secret, SECRET_MINIMUM_LENGTH};

mod streaming;

pub use streaming::{
    Finalize, FixedBuffer, FixedMutBuffer, RawHasherCore, SecretBuffer, SecretTooShortError,
    SecretWithSeedError,
};

#[cfg(feature = "alloc")]
pub use streaming::AllocRawHasher;

pub mod primes {
    pub const PRIME32_1: u64 = 0x9E3779B1;
    pub const PRIME32_2: u64 = 0x85EBCA77;
    pub const PRIME32_3: u64 = 0xC2B2AE3D;
    pub const PRIME64_1: u64 = 0x9E3779B185EBCA87;
    pub const PRIME64_2: u64 = 0xC2B2AE3D27D4EB4F;
    pub const PRIME64_3: u64 = 0x165667B19E3779F9;
    pub const PRIME64_4: u64 = 0x85EBCA77C2B2AE63;
    pub const PRIME64_5: u64 = 0x27D4EB2F165667C5;
    pub const PRIME_MX1: u64 = 0x165667919E3779F9;
    pub const PRIME_MX2: u64 = 0x9FB21C651E98DF25;
}

pub const CUTOFF: usize = 240;

pub const DEFAULT_SEED: u64 = 0;

/// The length of the default secret.
pub const DEFAULT_SECRET_LENGTH: usize = 192;

pub type DefaultSecret = [u8; DEFAULT_SECRET_LENGTH];

pub const DEFAULT_SECRET_RAW: DefaultSecret = [
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
    0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
    0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
    0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
    0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
    0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
    0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
];

// Safety: The default secret is long enough
pub const DEFAULT_SECRET: &Secret = unsafe { Secret::new_unchecked(&DEFAULT_SECRET_RAW) };

/// # Correctness
///
/// This function assumes that the incoming buffer has been populated
/// with the default secret.
#[inline]
pub fn derive_secret(seed: u64, secret: &mut DefaultSecret) {
    if seed == DEFAULT_SEED {
        return;
    }

    let (words, _) = secret.bp_as_chunks_mut();
    let (pairs, _) = words.bp_as_chunks_mut();

    for [a_p, b_p] in pairs {
        let a = u64::from_le_bytes(*a_p);
        let b = u64::from_le_bytes(*b_p);

        let a = a.wrapping_add(seed);
        let b = b.wrapping_sub(seed);

        *a_p = a.to_le_bytes();
        *b_p = b.to_le_bytes();
    }
}

/// The provided secret was not at least [`SECRET_MINIMUM_LENGTH`][]
/// bytes.
#[derive(Debug)]
pub struct OneshotWithSecretError(pub(crate) secret::Error);

impl core::error::Error for OneshotWithSecretError {}

impl core::fmt::Display for OneshotWithSecretError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        self.0.fmt(f)
    }
}

macro_rules! assert_input_range {
    ($min:literal.., $len:expr) => {
        assert!($min <= $len);
    };
    ($min:literal..=$max:literal, $len:expr) => {
        assert!($min <= $len);
        assert!($len <= $max);
    };
}
pub(crate) use assert_input_range;

#[inline(always)]
pub fn impl_1_to_3_bytes_combined(input: &[u8]) -> u32 {
    assert_input_range!(1..=3, input.len());
    let input_length = input.len() as u8; // OK as we checked that the length fits

    input[input.len() - 1].into_u32()
        | input_length.into_u32() << 8
        | input[0].into_u32() << 16
        | input[input.len() >> 1].into_u32() << 24
}

#[inline]
pub fn impl_17_to_128_bytes_iter(
    secret: &Secret,
    input: &[u8],
    mut f: impl FnMut(&[u8; 16], &[u8; 16], &[[u8; 16]; 2]),
) {
    let secret = secret.words_for_17_to_128();
    let (secret, _) = secret.bp_as_chunks::<2>();
    let (fwd, _) = input.bp_as_chunks();
    let (_, bwd) = input.bp_as_rchunks();

    let q = bwd.len();

    if input.len() > 32 {
        if input.len() > 64 {
            if input.len() > 96 {
                f(&fwd[3], &bwd[q - 4], &secret[3]);
            }

            f(&fwd[2], &bwd[q - 3], &secret[2]);
        }

        f(&fwd[1], &bwd[q - 2], &secret[1]);
    }

    f(&fwd[0], &bwd[q - 1], &secret[0]);
}

#[inline]
pub fn mix_step(data: &[u8; 16], secret: &[u8; 16], seed: u64) -> u64 {
    let data_words = to_u64s(data);
    let secret_words = to_u64s(secret);

    let mul_result = {
        let a = (data_words[0] ^ secret_words[0].wrapping_add(seed)).into_u128();
        let b = (data_words[1] ^ secret_words[1].wrapping_sub(seed)).into_u128();

        a.wrapping_mul(b)
    };

    mul_result.lower_half() ^ mul_result.upper_half()
}

#[inline]
pub fn to_u64s(bytes: &[u8; 16]) -> [u64; 2] {
    let (pair, _) = bytes.bp_as_chunks::<8>();
    [pair[0], pair[1]].map(u64::from_le_bytes)
}

#[inline]
#[cfg(feature = "xxhash3_128")]
pub fn pairs_of_u64_bytes(bytes: &[u8]) -> &[[[u8; 16]; 2]] {
    let (u64_bytes, _) = bytes.bp_as_chunks::<16>();
    let (pairs, _) = u64_bytes.bp_as_chunks::<2>();
    pairs
}

#[inline]
pub fn avalanche(mut x: u64) -> u64 {
    x ^= x >> 37;
    x = x.wrapping_mul(primes::PRIME_MX1);
    x ^= x >> 32;
    x
}

#[inline]
pub fn avalanche_xxh64(mut x: u64) -> u64 {
    x ^= x >> 33;
    x = x.wrapping_mul(primes::PRIME64_2);
    x ^= x >> 29;
    x = x.wrapping_mul(primes::PRIME64_3);
    x ^= x >> 32;
    x
}

#[inline]
pub fn stripes_with_tail(block: &[u8]) -> (&[[u8; 64]], &[u8]) {
    match block.bp_as_chunks() {
        ([stripes @ .., last], []) => (stripes, last),
        (stripes, last) => (stripes, last),
    }
}

/// THis exists just to easily map the XXH3 algorithm to Rust as the
/// algorithm describes 128-bit results as a pair of high and low u64
/// values.
#[derive(Copy, Clone)]
pub(crate) struct X128 {
    pub low: u64,
    pub high: u64,
}

impl From<X128> for u128 {
    fn from(value: X128) -> Self {
        value.high.into_u128() << 64 | value.low.into_u128()
    }
}

impl crate::IntoU128 for X128 {
    fn into_u128(self) -> u128 {
        self.into()
    }
}

pub trait Halves {
    type Output;

    fn upper_half(self) -> Self::Output;
    fn lower_half(self) -> Self::Output;
}

impl Halves for u64 {
    type Output = u32;

    #[inline]
    fn upper_half(self) -> Self::Output {
        (self >> 32) as _
    }

    #[inline]
    fn lower_half(self) -> Self::Output {
        self as _
    }
}

impl Halves for u128 {
    type Output = u64;

    #[inline]
    fn upper_half(self) -> Self::Output {
        (self >> 64) as _
    }

    #[inline]
    fn lower_half(self) -> Self::Output {
        self as _
    }
}

pub trait U8SliceExt {
    fn first_u32(&self) -> Option<u32>;

    fn last_u32(&self) -> Option<u32>;

    fn first_u64(&self) -> Option<u64>;

    fn last_u64(&self) -> Option<u64>;
}

impl U8SliceExt for [u8] {
    #[inline]
    fn first_u32(&self) -> Option<u32> {
        self.first_chunk().copied().map(u32::from_le_bytes)
    }

    #[inline]
    fn last_u32(&self) -> Option<u32> {
        self.last_chunk().copied().map(u32::from_le_bytes)
    }

    #[inline]
    fn first_u64(&self) -> Option<u64> {
        self.first_chunk().copied().map(u64::from_le_bytes)
    }

    #[inline]
    fn last_u64(&self) -> Option<u64> {
        self.last_chunk().copied().map(u64::from_le_bytes)
    }
}

pub trait SliceBackport<T> {
    fn bp_as_chunks<const N: usize>(&self) -> (&[[T; N]], &[T]);

    fn bp_as_chunks_mut<const N: usize>(&mut self) -> (&mut [[T; N]], &mut [T]);

    fn bp_as_rchunks<const N: usize>(&self) -> (&[T], &[[T; N]]);
}

impl<T> SliceBackport<T> for [T] {
    fn bp_as_chunks<const N: usize>(&self) -> (&[[T; N]], &[T]) {
        assert_ne!(N, 0);
        let len = self.len() / N;
        // Safety: `(len / N) * N` has to be less-than-or-equal to `len`
        let (head, tail) = unsafe { self.split_at_unchecked(len * N) };
        // Safety: (1) `head` points to valid data, (2) the alignment
        // of an array and the individual type are the same, (3) the
        // valid elements are less-than-or-equal to the original
        // slice.
        let head = unsafe { slice::from_raw_parts(head.as_ptr().cast(), len) };
        (head, tail)
    }

    fn bp_as_chunks_mut<const N: usize>(&mut self) -> (&mut [[T; N]], &mut [T]) {
        assert_ne!(N, 0);
        let len = self.len() / N;
        // Safety: `(len / N) * N` has to be less than or equal to `len`
        let (head, tail) = unsafe { self.split_at_mut_unchecked(len * N) };
        // Safety: (1) `head` points to valid data, (2) the alignment
        // of an array and the individual type are the same, (3) the
        // valid elements are less-than-or-equal to the original
        // slice.
        let head = unsafe { slice::from_raw_parts_mut(head.as_mut_ptr().cast(), len) };
        (head, tail)
    }

    fn bp_as_rchunks<const N: usize>(&self) -> (&[T], &[[T; N]]) {
        assert_ne!(N, 0);
        let len = self.len() / N;
        // Safety: `(len / N) * N` has to be less than or equal to `len`
        let (head, tail) = unsafe { self.split_at_unchecked(self.len() - len * N) };
        // Safety: (1) `tail` points to valid data, (2) the alignment
        // of an array and the individual type are the same, (3) the
        // valid elements are less-than-or-equal to the original
        // slice.
        let tail = unsafe { slice::from_raw_parts(tail.as_ptr().cast(), len) };
        (head, tail)
    }
}

#[cfg(test)]
pub mod test {
    use std::array;

    use super::*;

    macro_rules! bytes {
        ($($n: literal),* $(,)?) => {
            &[$(&crate::xxhash3::test::gen_bytes::<$n>() as &[u8],)*] as &[&[u8]]
        };
    }
    pub(crate) use bytes;

    pub fn gen_bytes<const N: usize>() -> [u8; N] {
        // Picking 251 as it's a prime number, which will hopefully
        // help avoid incidental power-of-two alignment.
        array::from_fn(|i| (i % 251) as u8)
    }

    #[test]
    fn default_secret_is_valid() {
        assert!(DEFAULT_SECRET.is_valid())
    }

    #[test]
    fn backported_as_chunks() {
        let x = [1, 2, 3, 4, 5];

        let (a, b) = x.bp_as_chunks::<1>();
        assert_eq!(a, &[[1], [2], [3], [4], [5]]);
        assert_eq!(b, &[] as &[i32]);

        let (a, b) = x.bp_as_chunks::<2>();
        assert_eq!(a, &[[1, 2], [3, 4]]);
        assert_eq!(b, &[5]);

        let (a, b) = x.bp_as_chunks::<3>();
        assert_eq!(a, &[[1, 2, 3]]);
        assert_eq!(b, &[4, 5]);

        let (a, b) = x.bp_as_chunks::<4>();
        assert_eq!(a, &[[1, 2, 3, 4]]);
        assert_eq!(b, &[5]);

        let (a, b) = x.bp_as_chunks::<5>();
        assert_eq!(a, &[[1, 2, 3, 4, 5]]);
        assert_eq!(b, &[] as &[i32]);

        let (a, b) = x.bp_as_chunks::<6>();
        assert_eq!(a, &[] as &[[i32; 6]]);
        assert_eq!(b, &[1, 2, 3, 4, 5]);
    }

    #[test]
    fn backported_as_rchunks() {
        let x = [1, 2, 3, 4, 5];

        let (a, b) = x.bp_as_rchunks::<1>();
        assert_eq!(a, &[] as &[i32]);
        assert_eq!(b, &[[1], [2], [3], [4], [5]]);

        let (a, b) = x.bp_as_rchunks::<2>();
        assert_eq!(a, &[1]);
        assert_eq!(b, &[[2, 3], [4, 5]]);

        let (a, b) = x.bp_as_rchunks::<3>();
        assert_eq!(a, &[1, 2]);
        assert_eq!(b, &[[3, 4, 5]]);

        let (a, b) = x.bp_as_rchunks::<4>();
        assert_eq!(a, &[1]);
        assert_eq!(b, &[[2, 3, 4, 5]]);

        let (a, b) = x.bp_as_rchunks::<5>();
        assert_eq!(a, &[] as &[i32]);
        assert_eq!(b, &[[1, 2, 3, 4, 5]]);

        let (a, b) = x.bp_as_rchunks::<6>();
        assert_eq!(a, &[1, 2, 3, 4, 5]);
        assert_eq!(b, &[] as &[[i32; 6]]);
    }
}
