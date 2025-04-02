//! AVX2 helpers for implementing Poly1305 using 26-bit limbs.

use core::fmt;
use core::ops::{Add, Mul};

#[cfg(target_arch = "x86")]
use core::arch::x86::*;
#[cfg(target_arch = "x86_64")]
use core::arch::x86_64::*;

use super::ParBlocks;
use crate::{Block, Key};

const fn set02(x3: u8, x2: u8, x1: u8, x0: u8) -> i32 {
    (((x3) << 6) | ((x2) << 4) | ((x1) << 2) | (x0)) as i32
}

/// Helper for Display impls of aligned values.
fn write_130(f: &mut fmt::Formatter<'_>, limbs: [u32; 5]) -> fmt::Result {
    let r0 = limbs[0] as u128;
    let r1 = limbs[1] as u128;
    let r2 = limbs[2] as u128;
    let r3 = limbs[3] as u128;
    let r4 = limbs[4] as u128;

    // Reduce into two u128s
    let l0 = r0 + (r1 << 26) + (r2 << 52) + (r3 << 78);
    let (l0, c) = l0.overflowing_add(r4 << 104);
    let l1 = (r4 >> 24) + if c { 1 } else { 0 };

    write!(f, "0x{:02x}{:032x}", l1, l0)
}

/// Helper for Display impls of unreduced values.
fn write_130_wide(f: &mut fmt::Formatter<'_>, limbs: [u64; 5]) -> fmt::Result {
    let r0 = limbs[0] as u128;
    let r1 = limbs[1] as u128;
    let r2 = limbs[2] as u128;
    let r3 = limbs[3] as u128;
    let r4 = limbs[4] as u128;

    // Reduce into two u128s
    let l0 = r0 + (r1 << 26) + (r2 << 52);
    let (l0, c1) = l0.overflowing_add(r3 << 78);
    let (l0, c2) = l0.overflowing_add(r4 << 104);
    let l1 = (r3 >> 50) + (r4 >> 24) + if c1 { 1 } else { 0 } + if c2 { 1 } else { 0 };

    write!(f, "0x{:02x}{:032x}", l1, l0)
}

/// Derives the Poly1305 addition and polynomial keys.
#[target_feature(enable = "avx2")]
pub(super) unsafe fn prepare_keys(key: &Key) -> (AdditionKey, PrecomputedMultiplier) {
    // [k7, k6, k5, k4, k3, k2, k1, k0]
    let key = _mm256_loadu_si256(key.as_ptr() as *const _);

    // Prepare addition key: [0, k7, 0, k6, 0, k5, 0, k4]
    let k = AdditionKey(_mm256_and_si256(
        _mm256_permutevar8x32_epi32(key, _mm256_set_epi32(3, 7, 2, 6, 1, 5, 0, 4)),
        _mm256_set_epi32(0, -1, 0, -1, 0, -1, 0, -1),
    ));

    // Prepare polynomial key R = k & 0xffffffc0ffffffc0ffffffc0fffffff:
    let r = Aligned130::new(_mm256_and_si256(
        key,
        _mm256_set_epi32(0, 0, 0, 0, 0x0ffffffc, 0x0ffffffc, 0x0ffffffc, 0x0fffffff),
    ));

    (k, r.into())
}

/// A 130-bit integer aligned across five 26-bit limbs.
///
/// The top three 32-bit words of the underlying 256-bit vector are ignored.
#[derive(Clone, Copy, Debug)]
pub(super) struct Aligned130(pub(super) __m256i);

impl fmt::Display for Aligned130 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut v0 = [0u8; 32];
        unsafe {
            _mm256_storeu_si256(v0.as_mut_ptr() as *mut _, self.0);
        }

        write!(f, "Aligned130(")?;
        write_130(
            f,
            [
                u32::from_le_bytes(v0[0..4].try_into().unwrap()),
                u32::from_le_bytes(v0[4..8].try_into().unwrap()),
                u32::from_le_bytes(v0[8..12].try_into().unwrap()),
                u32::from_le_bytes(v0[12..16].try_into().unwrap()),
                u32::from_le_bytes(v0[16..20].try_into().unwrap()),
            ],
        )?;
        write!(f, ")")
    }
}

impl Aligned130 {
    /// Aligns a 16-byte Poly1305 block at 26-bit boundaries within 32-bit words, and sets
    /// the high bit.
    #[target_feature(enable = "avx2")]
    pub(super) unsafe fn from_block(block: &Block) -> Self {
        Aligned130::new(_mm256_or_si256(
            _mm256_and_si256(
                // Load the 128-bit block into a 256-bit vector.
                _mm256_castsi128_si256(_mm_loadu_si128(block.as_ptr() as *const _)),
                // Mask off the upper 128 bits (undefined by _mm256_castsi128_si256).
                _mm256_set_epi64x(0, 0, -1, -1),
            ),
            // Set the high bit.
            _mm256_set_epi64x(0, 1, 0, 0),
        ))
    }

    /// Aligns a partial Poly1305 block at 26-bit boundaries within 32-bit words.
    ///
    /// Assumes that the high bit is already correctly set for the partial block.
    #[target_feature(enable = "avx2")]
    pub(super) unsafe fn from_partial_block(block: &Block) -> Self {
        Aligned130::new(_mm256_and_si256(
            // Load the 128-bit block into a 256-bit vector.
            _mm256_castsi128_si256(_mm_loadu_si128(block.as_ptr() as *const _)),
            // Mask off the upper 128 bits (undefined by _mm256_castsi128_si256).
            _mm256_set_epi64x(0, 0, -1, -1),
        ))
    }

    /// Splits a 130-bit integer into five 26-bit limbs.
    #[target_feature(enable = "avx2")]
    unsafe fn new(x: __m256i) -> Self {
        // Starting from a 130-bit integer split across 32-bit words:
        //     [0, 0, 0, [0; 30] || x4[2..0], x3, x2, x1, x0]

        // - Grab the low bits of each word:
        // x1 = [
        //                            [0; 32],
        //                            [0; 32],
        //                            [0; 32],
        //     [0; 6] || x4[ 2..0] || [0; 24],
        //               x3[14..0] || [0; 18],
        //               x2[20..0] || [0; 12],
        //               x1[26..0] || [0;  6],
        //               x0,
        // ]
        let xl = _mm256_sllv_epi32(x, _mm256_set_epi32(32, 32, 32, 24, 18, 12, 6, 0));

        // Grab the high bits of each word, rotated up by one word:
        // xh = [
        //     [0; 32],
        //     [0; 32],
        //     [0; 32],
        //     [0;  8] || x3[32.. 8]
        //     [0; 14] || x2[32..14]
        //     [0; 20] || x1[32..20]
        //     [0; 26] || x0[32..26],
        //     [0; 32],
        // ]
        let xh = _mm256_permutevar8x32_epi32(
            _mm256_srlv_epi32(x, _mm256_set_epi32(32, 32, 32, 2, 8, 14, 20, 26)),
            _mm256_set_epi32(6, 5, 4, 3, 2, 1, 0, 7),
        );

        // - Combine the low and high bits:
        // [
        //     [0; 32],
        //     [0; 32],
        //     [0; 32],
        //     [0;  6] || x4[ 2..0] || x3[32.. 8]
        //                x3[14..0] || x2[32..14]
        //                x2[20..0] || x1[32..20]
        //                x1[26..0] || x0[32..26],
        //                x0,
        // ]
        // - Mask to 26 bits:
        // [
        //     [0; 32],
        //     [0; 32],
        //     [0; 32],
        //     [0;  6] || x4[ 2..0] || x3[32.. 8]
        //     [0;  6] || x3[ 8..0] || x2[32..14]
        //     [0;  6] || x2[14..0] || x1[32..20]
        //     [0;  6] || x1[20..0] || x0[32..26],
        //     [0;  6] || x0[26..0],
        // ]
        Aligned130(_mm256_and_si256(
            _mm256_or_si256(xl, xh),
            _mm256_set_epi32(
                0, 0, 0, 0x3ffffff, 0x3ffffff, 0x3ffffff, 0x3ffffff, 0x3ffffff,
            ),
        ))
    }
}

impl Add<Aligned130> for Aligned130 {
    type Output = Aligned130;

    fn add(self, other: Aligned130) -> Aligned130 {
        // With 26-bit limbs inside 32-bit words, there is plenty of space for unreduced
        // addition.
        unsafe { Aligned130(_mm256_add_epi32(self.0, other.0)) }
    }
}

/// A pre-computed multiplier.
#[derive(Clone, Copy, Debug)]
pub(super) struct PrecomputedMultiplier {
    pub(super) a: __m256i,
    pub(super) a_5: __m256i,
}

impl From<Aligned130> for PrecomputedMultiplier {
    fn from(r: Aligned130) -> Self {
        unsafe {
            // Precompute 5*R.
            //
            // The 5-limb representation (r_4, r_3, r_2, r_1, r_0) of R and
            // (5·r4, 5·r3, 5·r2, 5·r1) are represented in two 256-bit vectors in the
            // following manner:
            //     r1:   [5·r_4, 5·r_3, 5·r_2,   r_4,   r_3,   r_2,   r_1,   r_0]
            //     r1_5: [5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1]
            let a_5 = _mm256_permutevar8x32_epi32(
                _mm256_add_epi32(r.0, _mm256_slli_epi32(r.0, 2)),
                _mm256_set_epi32(4, 3, 2, 1, 1, 1, 1, 1),
            );
            let a = _mm256_blend_epi32(r.0, a_5, 0b11100000);
            let a_5 = _mm256_permute2x128_si256(a_5, a_5, 0);

            PrecomputedMultiplier { a, a_5 }
        }
    }
}

impl Mul<PrecomputedMultiplier> for PrecomputedMultiplier {
    type Output = Unreduced130;

    fn mul(self, other: PrecomputedMultiplier) -> Unreduced130 {
        // Pass through to `self.a` for multiplication.
        Aligned130(self.a) * other
    }
}

impl Mul<PrecomputedMultiplier> for Aligned130 {
    type Output = Unreduced130;

    /// Multiplies 2 values using lazy reduction.
    ///
    /// Context switches from 32 bit to 64 bit.
    #[inline(always)]
    fn mul(self, other: PrecomputedMultiplier) -> Unreduced130 {
        unsafe {
            // Starting with the following limb layout:
            // x = [    0,     0,     0,   x_4,   x_3,   x_2,   x_1,   x_0]
            // y = [5·r_4, 5·r_3, 5·r_2,   r_4,   r_3,   r_2,   r_1,   r_0]
            // z = [5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1]
            let x = self.0;
            let y = other.a;
            let z = other.a_5;

            //   [ 0,   x_4,  0,   x_3,  0,   x_2,  0,   x_1] (32-bit words)
            // *  5·r_4
            // = [5·r_4·x_4, 5·r_4·x_3, 5·r_4·x_2, 5·r_4·x_1] (64-bit words)
            let v0 = _mm256_mul_epu32(
                _mm256_permutevar8x32_epi32(x, _mm256_set_epi64x(4, 3, 2, 1)),
                _mm256_permutevar8x32_epi32(y, _mm256_set_epi64x(7, 7, 7, 7)),
            );
            //   [   0, x_3,    0, x_2,    0, x_1,    0, x_0] (32-bit words)
            // *    r_0
            // = [  r_0·x_3,   r_0·x_2,   r_0·x_1,   r_0·x_0] (64-bit words)
            // + previous step
            // = [
            //     r_0·x_3 + 5·r_4·x_4,
            //     r_0·x_2 + 5·r_4·x_3,
            //     r_0·x_1 + 5·r_4·x_2,
            //     r_0·x_0 + 5·r_4·x_1,
            // ]
            let v0 = _mm256_add_epi64(
                v0,
                _mm256_mul_epu32(
                    _mm256_permutevar8x32_epi32(x, _mm256_set_epi64x(3, 2, 1, 0)),
                    _mm256_broadcastd_epi32(_mm256_castsi256_si128(y)),
                ),
            );
            //   [ 0, x_1,  0, x_1,  0,   x_3,  0,   x_3]
            // * [ 0, r_2,  0, r_1,  0, 5·r_3,  0, 5·r_2]
            // = [r_2·x_1, r_1·x_1, 5·r_3·x_3, 5·r_2·x_3]
            // + previous step
            // = [
            //     r_0·x_3 +   r_2·x_1 + 5·r_4·x_4,
            //     r_0·x_2 +   r_1·x_1 + 5·r_4·x_3,
            //     r_0·x_1 + 5·r_3·x_3 + 5·r_4·x_2,
            //     r_0·x_0 + 5·r_2·x_3 + 5·r_4·x_1,
            // ]
            let v0 = _mm256_add_epi64(
                v0,
                _mm256_mul_epu32(
                    _mm256_permutevar8x32_epi32(x, _mm256_set_epi64x(1, 1, 3, 3)),
                    _mm256_permutevar8x32_epi32(y, _mm256_set_epi64x(2, 1, 6, 5)),
                ),
            );
            //   [x_3, x_2, x_1, x_0, x_1, x_0,     0,   x_4]
            // * [  0, r_1,   0, r_2,   0, r_1, 5·r_1, 5·r_1]
            // = [ r_1·x_2,  r_2·x_0,  r_1·x_0,    5·r_1·x_4]
            // + previous step
            // = [
            //     r_0·x_3 +   r_1·x_2 +   r_2·x_1 + 5·r_4·x_4,
            //     r_0·x_2 +   r_1·x_1 +   r_2·x_0 + 5·r_4·x_3,
            //     r_0·x_1 +   r_1·x_0 + 5·r_3·x_3 + 5·r_4·x_2,
            //     r_0·x_0 + 5·r_1·x_4 + 5·r_2·x_3 + 5·r_4·x_1,
            // ]
            let v0 = _mm256_add_epi64(
                v0,
                _mm256_mul_epu32(
                    _mm256_permute4x64_epi64(x, set02(1, 0, 0, 2)),
                    _mm256_blend_epi32(
                        _mm256_permutevar8x32_epi32(y, _mm256_set_epi64x(1, 2, 1, 1)),
                        z,
                        0x03,
                    ),
                ),
            );
            //   [x_1, x_0,  0,   x_4,  0,   x_4, x_3,   x_2]
            // * [  0, r_3,  0, 5·r_3,  0, 5·r_2,   0, 5·r_3]
            // = [ r_3·x_0, 5·r_3·x_4, 5·r_2·x_4,  5·r_3·x_2]
            // + previous step
            // v0 = [
            //     r_0·x_3 +   r_1·x_2 +   r_2·x_1 +   r_3·x_0 + 5·r_4·x_4,
            //     r_0·x_2 +   r_1·x_1 +   r_2·x_0 + 5·r_3·x_4 + 5·r_4·x_3,
            //     r_0·x_1 +   r_1·x_0 + 5·r_2·x_4 + 5·r_3·x_3 + 5·r_4·x_2,
            //     r_0·x_0 + 5·r_1·x_4 + 5·r_2·x_3 + 5·r_3·x_2 + 5·r_4·x_1,
            // ]
            let v0 = _mm256_add_epi64(
                v0,
                _mm256_mul_epu32(
                    _mm256_permute4x64_epi64(x, set02(0, 2, 2, 1)),
                    _mm256_permutevar8x32_epi32(y, _mm256_set_epi64x(3, 6, 5, 6)),
                ),
            );

            //   [ 0, x_3,  0, x_2,  0, x_1,  0, x_0]
            // * [ 0, r_1,  0, r_2,  0, r_3,  0, r_4]
            // = [r_1·x_3, r_2·x_2, r_3·x_1, r_4·x_0]
            let v1 = _mm256_mul_epu32(
                _mm256_permutevar8x32_epi32(x, _mm256_set_epi64x(3, 2, 1, 0)),
                _mm256_permutevar8x32_epi32(y, _mm256_set_epi64x(1, 2, 3, 4)),
            );
            //   [r_3·x_1, r_4·x_0, r_1·x_3, r_2·x_2]
            // + previous step
            // = [
            //     r_1·x_3 + r_3·x_1,
            //     r_2·x_2 + r_4·x_0,
            //     r_1·x_3 + r_3·x_1,
            //     r_2·x_2 + r_4·x_0,
            // ]
            let v1 = _mm256_add_epi64(v1, _mm256_permute4x64_epi64(v1, set02(1, 0, 3, 2)));
            // [
            //     r_2·x_2 + r_4·x_0,
            //     r_2·x_2 + r_4·x_0,
            //     r_2·x_2 + r_4·x_0,
            //     r_1·x_3 + r_3·x_1,
            // ]
            // + previous step
            // = [
            //     r_1·x_3 + r_2·x_2 + r_3·x_1 + r_4·x_0,
            //     2·r_2·x_2 + 2·r_4·x_0,
            //     r_1·x_3 + r_2·x_2 + r_3·x_1 + r_4·x_0,
            //     r_1·x_3 + r_2·x_2 + r_3·x_1 + r_4·x_0,
            // ]
            let v1 = _mm256_add_epi64(v1, _mm256_permute4x64_epi64(v1, set02(0, 0, 0, 1)));
            //   [  x_1,   x_0,   x_1, x_0, x_1, x_0,   0, x_4]
            // * [5·r_4, 5·r_3, 5·r_2, r_4, r_3, r_2, r_1, r_0]
            // = [   5·r_3·x_0,    r_4·x_0,  r_2·x_0,  r_0·x_4]
            // + previous step
            // v1 = [
            //     5·r_3·x_0 + r_1·x_3 +   r_2·x_2 + r_3·x_1 +   r_4·x_0,
            //                           2·r_2·x_2           + 3·r_4·x_0,
            //       r_2·x_0 + r_1·x_3 +   r_2·x_2 + r_3·x_1 +   r_4·x_0,
            //       r_0·x_4 + r_1·x_3 +   r_2·x_2 + r_3·x_1 +   r_4·x_0,
            // ]
            let v1 = _mm256_add_epi64(
                v1,
                _mm256_mul_epu32(_mm256_permute4x64_epi64(x, set02(0, 0, 0, 2)), y),
            );

            // The result:
            // v1 = [
            //     5·r_3·x_0 +   r_1·x_3 +   r_2·x_2 +   r_3·x_1 +   r_4·x_0,
            //                             2·r_2·x_2             + 3·r_4·x_0,
            //       r_2·x_0 +   r_1·x_3 +   r_2·x_2 +   r_3·x_1 +   r_4·x_0,
            //       r_0·x_4 +   r_1·x_3 +   r_2·x_2 +   r_3·x_1 +   r_4·x_0,
            // ]
            // v0 = [
            //       r_0·x_3 +   r_1·x_2 +   r_2·x_1 +   r_3·x_0 + 5·r_4·x_4,
            //       r_0·x_2 +   r_1·x_1 +   r_2·x_0 + 5·r_3·x_4 + 5·r_4·x_3,
            //       r_0·x_1 +   r_1·x_0 + 5·r_2·x_4 + 5·r_3·x_3 + 5·r_4·x_2,
            //       r_0·x_0 + 5·r_1·x_4 + 5·r_2·x_3 + 5·r_3·x_2 + 5·r_4·x_1,
            // ]
            // This corresponds to (3) in Goll Gueron 2015:
            // v1 = [  _,   _,   _, t_4]
            // v0 = [t_3, t_2, t_1, t_0]
            Unreduced130 { v0, v1 }
        }
    }
}

/// The unreduced output of an `Aligned130` multiplication.
///
/// Represented internally with 64-bit limbs.
#[derive(Copy, Clone, Debug)]
pub(super) struct Unreduced130 {
    v0: __m256i,
    v1: __m256i,
}

impl fmt::Display for Unreduced130 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut v0 = [0u8; 32];
        let mut v1 = [0u8; 32];
        unsafe {
            _mm256_storeu_si256(v0.as_mut_ptr() as *mut _, self.v0);
            _mm256_storeu_si256(v1.as_mut_ptr() as *mut _, self.v1);
        }

        write!(f, "Unreduced130(")?;
        write_130_wide(
            f,
            [
                u64::from_le_bytes(v0[0..8].try_into().unwrap()),
                u64::from_le_bytes(v0[8..16].try_into().unwrap()),
                u64::from_le_bytes(v0[16..24].try_into().unwrap()),
                u64::from_le_bytes(v0[24..32].try_into().unwrap()),
                u64::from_le_bytes(v1[0..8].try_into().unwrap()),
            ],
        )?;
        write!(f, ")")
    }
}

impl Unreduced130 {
    /// Reduces x modulo 2^130 - 5.
    ///
    /// Context switches from 64 bit to 32 bit.
    #[inline(always)]
    pub(super) fn reduce(self) -> Aligned130 {
        unsafe {
            // Starting with the following limb layout:
            // self.v1 = [  _,   _,   _, t_4]
            // self.v0 = [t_3, t_2, t_1, t_0]
            let (red_1, red_0) = adc(self.v1, self.v0);
            let (red_1, red_0) = red(red_1, red_0);
            let (red_1, red_0) = adc(red_1, red_0);

            // - Switch context from 64-bit limbs to 32-bit limbs:
            Aligned130(_mm256_blend_epi32(
                _mm256_permutevar8x32_epi32(red_0, _mm256_set_epi32(0, 6, 4, 0, 6, 4, 2, 0)),
                _mm256_permutevar8x32_epi32(red_1, _mm256_set_epi32(0, 6, 4, 0, 6, 4, 2, 0)),
                0x90,
            ))
        }
    }
}

/// Carry chain
#[inline(always)]
unsafe fn adc(v1: __m256i, v0: __m256i) -> (__m256i, __m256i) {
    //   [t_3,       t_2 % 2^26, t_1 % 2^26, t_0 % 2^26]
    // + [t_2 >> 26, t_1 >>  26, t_0 >>  26,  0        ]
    // = [
    //     t_3        + t_2 >> 26,
    //     t_2 % 2^26 + t_1 >> 26,
    //     t_1 % 2^26 + t_0 >> 26,
    //     t_0 % 2^26,
    // ]
    let v0 = _mm256_add_epi64(
        _mm256_and_si256(v0, _mm256_set_epi64x(-1, 0x3ffffff, 0x3ffffff, 0x3ffffff)),
        _mm256_permute4x64_epi64(
            _mm256_srlv_epi64(v0, _mm256_set_epi64x(64, 26, 26, 26)),
            set02(2, 1, 0, 3),
        ),
    );
    //   [_, _, _, t_4]
    // + [
    //     (t_2 % 2^26 + t_1 >> 26) >> 26,
    //     (t_1 % 2^26 + t_0 >> 26) >> 26,
    //     (t_0 % 2^26            ) >> 26,
    //     (t_3        + t_2 >> 26) >> 26,
    // ]
    // = [_, _, _, t_4 + (t_3 + t_2 >> 26) >> 26]
    let v1 = _mm256_add_epi64(
        v1,
        _mm256_permute4x64_epi64(_mm256_srli_epi64(v0, 26), set02(2, 1, 0, 3)),
    );
    // [
    //     (t_3 + t_2 >> 26) % 2^26,
    //     t_2 % 2^26 + t_1 >> 26,
    //     t_1 % 2^26 + t_0 >> 26,
    //     t_0 % 2^26,
    // ]
    let chain = _mm256_and_si256(v0, _mm256_set_epi64x(0x3ffffff, -1, -1, -1));

    (v1, chain)
}

/// Reduction modulus 2^130-5
#[inline(always)]
unsafe fn red(v1: __m256i, v0: __m256i) -> (__m256i, __m256i) {
    // t = [0, 0, 0, t_4 >> 26]
    let t = _mm256_srlv_epi64(v1, _mm256_set_epi64x(64, 64, 64, 26));
    // v0 + 5·t = [t_3, t_2, t_1, t_0 + 5·(t_4 >> 26)]
    let red_0 = _mm256_add_epi64(_mm256_add_epi64(v0, t), _mm256_slli_epi64(t, 2));
    // [0, 0, 0, t_4 % 2^26]
    let red_1 = _mm256_and_si256(v1, _mm256_set_epi64x(0, 0, 0, 0x3ffffff));
    (red_1, red_0)
}

/// A pair of `Aligned130`s.
#[derive(Clone, Debug)]
pub(super) struct Aligned2x130 {
    v0: Aligned130,
    v1: Aligned130,
}

impl fmt::Display for Aligned2x130 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        writeln!(f, "Aligned2x130([")?;
        writeln!(f, "    {},", self.v0)?;
        writeln!(f, "    {},", self.v1)?;
        write!(f, "])")
    }
}

impl Aligned2x130 {
    /// Aligns two 16-byte Poly1305 blocks at 26-bit boundaries within 32-bit words, and
    /// sets the high bit for each block.
    ///
    /// # Panics
    ///
    /// Panics if `src.len() < 32`.
    #[target_feature(enable = "avx2")]
    pub(super) unsafe fn from_blocks(src: &[Block; 2]) -> Self {
        Aligned2x130 {
            v0: Aligned130::from_block(&src[0]),
            v1: Aligned130::from_block(&src[1]),
        }
    }

    /// Multiplies 2x2 and add both results simultaneously using lazy reduction.
    ///
    /// Context switches from 32 bit to 64 bit.
    #[inline(always)]
    pub(super) fn mul_and_sum(
        self,
        r1: PrecomputedMultiplier,
        r2: PrecomputedMultiplier,
    ) -> Unreduced130 {
        unsafe {
            // Starting with the following limb layout:
            // x.v1 = [     0,      0,      0,   x1_4,   x1_3,   x1_2,   x1_1,   x1_0]
            // x.v0 = [     0,      0,      0,   x0_4,   x0_3,   x0_2,   x0_1,   x0_0]
            // r1   = [5·r1_4, 5·r1_3, 5·r1_2,   r1_4,   r1_3,   r1_2,   r1_1,   r1_0]
            // r15  = [5·r1_1, 5·r1_1, 5·r1_1, 5·r1_1, 5·r1_1, 5·r1_1, 5·r1_1, 5·r1_1]
            // r2   = [5·r2_4, 5·r2_3, 5·r2_2,   r2_4,   r2_3,   r2_2,   r2_1,   r2_0]
            // r25  = [5·r2_1, 5·r2_1, 5·r2_1, 5·r2_1, 5·r2_1, 5·r2_1, 5·r2_1, 5·r2_1]
            let x = self;
            let r15 = r1.a_5;
            let r25 = r2.a_5;
            let r1 = r1.a;
            let r2 = r2.a;

            // v0 = [
            //     5·x0_4·r2_4,
            //     5·x0_3·r2_4,
            //     5·x0_2·r2_4,
            //     5·x0_1·r2_4,
            // ]
            let mut v0 = _mm256_mul_epu32(
                // [_,   x0_4, _,   x0_3, _,   x0_2, _,   x0_1]
                _mm256_permutevar8x32_epi32(x.v0.0, _mm256_set_epi64x(4, 3, 2, 1)),
                // [_, 5·r2_4, _, 5·r2_4, _, 5·r2_4, _, 5·r2_4]
                _mm256_permutevar8x32_epi32(r2, _mm256_set1_epi64x(7)),
            );
            // v1 = [
            //     5·x1_4·r1_4,
            //     5·x1_3·r1_4,
            //     5·x1_2·r1_4,
            //     5·x1_1·r1_4,
            // ]
            let mut v1 = _mm256_mul_epu32(
                // [_,   x1_4, _,   x1_3, _,   x1_2, _,   x1_1]
                _mm256_permutevar8x32_epi32(x.v1.0, _mm256_set_epi64x(4, 3, 2, 1)),
                // [_, 5·r1_4, _, 5·r1_4, _, 5·r1_4, _, 5·r1_4]
                _mm256_permutevar8x32_epi32(r1, _mm256_set1_epi64x(7)),
            );

            // v0 = [
            //                    `x0_0·r2_3`+ 5·x0_4·r2_4,
            //                  `5·x0_4·r2_3`+ 5·x0_3·r2_4,
            //    `5·x0_4·r2_2`              + 5·x0_2·r2_4,
            //                  `5·x0_2·r2_3`+ 5·x0_1·r2_4,
            // ]
            v0 = _mm256_add_epi64(
                v0,
                _mm256_mul_epu32(
                    // [_, x0_0, _,   x0_4, _,   x0_4, _,   x0_2]
                    _mm256_permute4x64_epi64(x.v0.0, set02(0, 2, 2, 1)),
                    // [_, r2_3, _, 5·r2_3, _, 5·r2_2, _, 5·r2_3]
                    _mm256_permutevar8x32_epi32(r2, _mm256_set_epi64x(3, 6, 5, 6)),
                ),
            );
            // v1 = [
            //                  `x1_0·r1_3`+ 5·x1_4·r1_4,
            //                `5·x1_4·r1_3`+ 5·x1_3·r1_4,
            //    `5·x1_4·r1_2`            + 5·x1_2·r1_4,
            //                `5·x1_2·r1_3`+ 5·x1_1·r1_4,
            // ]
            v1 = _mm256_add_epi64(
                v1,
                _mm256_mul_epu32(
                    // [_, x1_0, _,   x1_4, _,   x1_4, _,   x1_2]
                    _mm256_permute4x64_epi64(x.v1.0, set02(0, 2, 2, 1)),
                    // [_, r1_3, _, 5·r1_3, _, 5·r1_2, _, 5·r1_3]
                    _mm256_permutevar8x32_epi32(r1, _mm256_set_epi64x(3, 6, 5, 6)),
                ),
            );
            // v0 = [
            //                  `x0_1·r2_2`+   x0_0·r2_3 + 5·x0_4·r2_4,
            //    `x0_1·r2_1`              + 5·x0_4·r2_3 + 5·x0_3·r2_4,
            //                 5·x0_4·r2_2 +`5·x0_3·r2_3`+ 5·x0_2·r2_4,
            //                `5·x0_3·r2_2`+ 5·x0_2·r2_3 + 5·x0_1·r2_4,
            // ]
            v0 = _mm256_add_epi64(
                v0,
                _mm256_mul_epu32(
                    // [_, x0_1, _, x0_1, _,   x0_3, _,   x0_3]
                    _mm256_permutevar8x32_epi32(x.v0.0, _mm256_set_epi64x(1, 1, 3, 3)),
                    // [_, r2_2, _, r2_1, _, 5·r2_3, _, 5·r2_2]
                    _mm256_permutevar8x32_epi32(r2, _mm256_set_epi64x(2, 1, 6, 5)),
                ),
            );
            // v1 = [
            //                `x1_1·r1_2`+   x1_0·r1_3 + 5·x1_4·r1_4,
            //    `x1_1·r1_1`            + 5·x1_4·r1_3 + 5·x1_3·r1_4,
            //               5·x1_4·r1_2 +`5·x1_3·r1_3`+ 5·x1_2·r1_4,
            //              `5·x1_3·r1_2`+ 5·x1_2·r1_3 + 5·x1_1·r1_4,
            // ]
            v1 = _mm256_add_epi64(
                v1,
                _mm256_mul_epu32(
                    // [_, x1_1, _, x1_1, _,   x1_3, _,   x1_3]
                    _mm256_permutevar8x32_epi32(x.v1.0, _mm256_set_epi64x(1, 1, 3, 3)),
                    // [_, r1_2, _, r1_1, _, 5·r1_3, _, 5·r1_2]
                    _mm256_permutevar8x32_epi32(r1, _mm256_set_epi64x(2, 1, 6, 5)),
                ),
            );
            // v0 = [
            //    `x0_3·r2_0`              +   x0_1·r2_2 +   x0_0·r2_3 + 5·x0_4·r2_4,
            //    `x0_2·r2_0`+   x0_1·r2_1               + 5·x0_4·r2_3 + 5·x0_3·r2_4,
            //    `x0_1·r2_0`              + 5·x0_4·r2_2 + 5·x0_3·r2_3 + 5·x0_2·r2_4,
            //    `x0_0·r2_0`              + 5·x0_3·r2_2 + 5·x0_2·r2_3 + 5·x0_1·r2_4,
            // ]
            v0 = _mm256_add_epi64(
                v0,
                _mm256_mul_epu32(
                    // [_, x0_3, _, x0_2, _, x0_1, _, x0_0]
                    _mm256_permutevar8x32_epi32(x.v0.0, _mm256_set_epi64x(3, 2, 1, 0)),
                    // [_, r2_0, _, r2_0, _, r2_0, _, r2_0]
                    _mm256_broadcastd_epi32(_mm256_castsi256_si128(r2)),
                ),
            );
            // v1 = [
            //    `x1_3·r1_0`              +   x1_1·r1_2 +   x1_0·r1_3 + 5·x1_4·r1_4,
            //    `x1_2·r1_0`+   x1_1·r1_1               + 5·x1_4·r1_3 + 5·x1_3·r1_4,
            //    `x1_1·r1_0`              + 5·x1_4·r1_2 + 5·x1_3·r1_3 + 5·x1_2·r1_4,
            //    `x1_0·r1_0`              + 5·x1_3·r1_2 + 5·x1_2·r1_3 + 5·x1_1·r1_4,
            // ]
            v1 = _mm256_add_epi64(
                v1,
                _mm256_mul_epu32(
                    // [_, x1_3, _, x1_2, _, x1_1, _, x1_0]
                    _mm256_permutevar8x32_epi32(x.v1.0, _mm256_set_epi64x(3, 2, 1, 0)),
                    // [_, r1_0, _, r1_0, _, r1_0, _, r1_0]
                    _mm256_broadcastd_epi32(_mm256_castsi256_si128(r1)),
                ),
            );

            // t0 = [x0_3, x0_2, x0_1, x0_0, x0_1, x0_0, 0, x0_4]
            // t1 = [x1_3, x1_2, x1_1, x1_0, x1_1, x1_0, 0, x1_4]
            let mut t0 = _mm256_permute4x64_epi64(x.v0.0, set02(1, 0, 0, 2));
            let mut t1 = _mm256_permute4x64_epi64(x.v1.0, set02(1, 0, 0, 2));

            // v0 = [
            //     x0_3·r2_0 +  `x0_2·r2_1`+   x0_1·r2_2 +   x0_0·r2_3 + 5·x0_4·r2_4,
            //     x0_2·r2_0 +   x0_1·r2_1 +  `x0_0·r2_2`+ 5·x0_4·r2_3 + 5·x0_3·r2_4,
            //     x0_1·r2_0 +  `x0_0·r2_1`+ 5·x0_4·r2_2 + 5·x0_3·r2_3 + 5·x0_2·r2_4,
            //     x0_0·r2_0 +`5·x0_4·r2_1`+ 5·x0_3·r2_2 + 5·x0_2·r2_3 + 5·x0_1·r2_4,
            // ]
            v0 = _mm256_add_epi64(
                v0,
                _mm256_mul_epu32(
                    // [_, x0_2, _, x0_0, _, x0_0, _,   x0_4]
                    t0,
                    // [_, r2_1, _, r2_2, _, r2_1, _, 5·r2_1]
                    _mm256_blend_epi32(
                        // [r2_0, r2_1, r2_0, r2_2, r2_0, r2_1, r2_0, r2_1]
                        _mm256_permutevar8x32_epi32(r2, _mm256_set_epi64x(1, 2, 1, 1)),
                        r25,
                        0b00000011,
                    ),
                ),
            );
            // v1 = [
            //     x1_3·r1_0 +  `x1_2·r1_1`+   x1_1·r1_2 +   x1_0·r1_3 + 5·x1_4·r1_4,
            //     x1_2·r1_0 +   x1_1·r1_1 +  `x1_0·r1_2`+ 5·x1_4·r1_3 + 5·x1_3·r1_4,
            //     x1_1·r1_0 +  `x1_0·r1_1`+ 5·x1_4·r1_2 + 5·x1_3·r1_3 + 5·x1_2·r1_4,
            //     x1_0·r1_0 +`5·x1_4·r1_1`+ 5·x1_3·r1_2 + 5·x1_2·r1_3 + 5·x1_1·r1_4,
            // ]
            v1 = _mm256_add_epi64(
                v1,
                _mm256_mul_epu32(
                    // [_, x1_2, _, x1_0, _, x1_0, _,   x1_4]
                    t1,
                    // [_, r1_1, _, r1_2, _, r1_1, _, 5·r1_1]
                    _mm256_blend_epi32(
                        // [r1_0, r1_1, r1_0, r1_2, r1_0, r1_1, r1_0, r1_1]
                        _mm256_permutevar8x32_epi32(r1, _mm256_set_epi64x(1, 2, 1, 1)),
                        r15,
                        0b00000011,
                    ),
                ),
            );
            // v0 = [
            //     x0_3·r2_0 +   x0_2·r2_1 +   x0_1·r2_2 +   x0_0·r2_3 + 5·x0_4·r2_4 + x1_3·r1_0 +   x1_2·r1_1 +   x1_1·r1_2 +   x1_0·r1_3 + 5·x1_4·r1_4,
            //     x0_2·r2_0 +   x0_1·r2_1 +   x0_0·r2_2 + 5·x0_4·r2_3 + 5·x0_3·r2_4 + x1_2·r1_0 +   x1_1·r1_1 +   x1_0·r1_2 + 5·x1_4·r1_3 + 5·x1_3·r1_4,
            //     x0_1·r2_0 +   x0_0·r2_1 + 5·x0_4·r2_2 + 5·x0_3·r2_3 + 5·x0_2·r2_4 + x1_1·r1_0 +   x1_0·r1_1 + 5·x1_4·r1_2 + 5·x1_3·r1_3 + 5·x1_2·r1_4,
            //     x0_0·r2_0 + 5·x0_4·r2_1 + 5·x0_3·r2_2 + 5·x0_2·r2_3 + 5·x0_1·r2_4 + x1_0·r1_0 + 5·x1_4·r1_1 + 5·x1_3·r1_2 + 5·x1_2·r1_3 + 5·x1_1·r1_4,
            // ]
            v0 = _mm256_add_epi64(v0, v1);

            // t0 = [
            //     5·x0_2·r2_3,
            //       x0_0·r2_4,
            //       x0_0·r2_2,
            //       x0_4·r2_0,
            // ]
            // t1 = [
            //     5·x1_2·r1_3,
            //       x1_0·r1_4,
            //       x1_0·r1_2,
            //       x1_4·r1_0,
            // ]
            t0 = _mm256_mul_epu32(t0, r2);
            t1 = _mm256_mul_epu32(t1, r1);

            // v1 = [
            //     5·x0_2·r2_3 + 5·x1_2·r1_3,
            //       x0_0·r2_4 +   x1_0·r1_4,
            //       x0_0·r2_2 +   x1_0·r1_2,
            //       x0_4·r2_0 +   x1_4·r1_0,
            // ]
            v1 = _mm256_add_epi64(t0, t1);

            // t0 = [
            //     x0_3·r2_1,
            //     x0_2·r2_2,
            //     x0_1·r2_3,
            //     x0_0·r2_4,
            // ]
            t0 = _mm256_mul_epu32(
                // [_, x0_3, _, x0_2, _, x0_1, _, x0_0]
                _mm256_permutevar8x32_epi32(x.v0.0, _mm256_set_epi64x(3, 2, 1, 0)),
                // [_, r2_1, _, r2_2, _, r2_3, _, r2_4]
                _mm256_permutevar8x32_epi32(r2, _mm256_set_epi64x(1, 2, 3, 4)),
            );
            // t1 = [
            //     x1_3·r1_1,
            //     x1_2·r1_2,
            //     x1_1·r1_3,
            //     x1_0·r1_4,
            // ]
            t1 = _mm256_mul_epu32(
                // [_, x1_3, _, x1_2, _, x1_1, _, x1_0]
                _mm256_permutevar8x32_epi32(x.v1.0, _mm256_set_epi64x(3, 2, 1, 0)),
                // [_, r1_1, _, r1_2, _, r1_3, _, r1_4]
                _mm256_permutevar8x32_epi32(r1, _mm256_set_epi64x(1, 2, 3, 4)),
            );
            // t0 = [
            //     x0_3·r2_1 + x1_3·r1_1,
            //     x0_2·r2_2 + x1_2·r1_2,
            //     x0_1·r2_3 + x1_1·r1_3,
            //     x0_0·r2_4 + x1_0·r1_4,
            // ]
            t0 = _mm256_add_epi64(t0, t1);
            // t0 = [
            //     x0_3·r2_1 + x0_1·r2_3 + x1_3·r1_1 + x1_1·r1_3,
            //     x0_2·r2_2 + x0_0·r2_4 + x1_2·r1_2 + x1_0·r1_4,
            //     x0_3·r2_1 + x0_1·r2_3 + x1_3·r1_1 + x1_1·r1_3,
            //     x0_2·r2_2 + x0_0·r2_4 + x1_2·r1_2 + x1_0·r1_4,
            // ]
            t0 = _mm256_add_epi64(t0, _mm256_permute4x64_epi64(t0, set02(1, 0, 3, 2)));
            // t0 = [
            //     x0_3·r2_1 + x0_2·r2_2 + x0_1·r2_3 + x0_0·r2_4 + x1_3·r1_1 + x1_2·r1_2 + x1_1·r1_3 + x1_0·r1_4,
            //     x0_3·r2_1 + x0_2·r2_2 + x0_1·r2_3 + x0_0·r2_4 + x1_3·r1_1 + x1_2·r1_2 + x1_1·r1_3 + x1_0·r1_4,
            //     x0_3·r2_1 + x0_2·r2_2 + x0_1·r2_3 + x0_0·r2_4 + x1_3·r1_1 + x1_2·r1_2 + x1_1·r1_3 + x1_0·r1_4,
            //     x0_3·r2_1 + x0_2·r2_2 + x0_1·r2_3 + x0_0·r2_4 + x1_3·r1_1 + x1_2·r1_2 + x1_1·r1_3 + x1_0·r1_4,
            // ]
            t0 = _mm256_add_epi64(t0, _mm256_permute4x64_epi64(t0, set02(2, 3, 0, 1)));

            // v1 = [
            //     5·x0_2·r2_3 + x0_3·r2_1 + x0_2·r2_2 + x0_1·r2_3 + x0_0·r2_4 + 5·x1_2·r1_3 + x1_3·r1_1 + x1_2·r1_2 + x1_1·r1_3 + x1_0·r1_4,
            //       x0_0·r2_4 + x0_3·r2_1 + x0_2·r2_2 + x0_1·r2_3 + x0_0·r2_4 +   x1_0·r1_4 + x1_3·r1_1 + x1_2·r1_2 + x1_1·r1_3 + x1_0·r1_4,
            //       x0_0·r2_2 + x0_3·r2_1 + x0_2·r2_2 + x0_1·r2_3 + x0_0·r2_4 +   x1_0·r1_2 + x1_3·r1_1 + x1_2·r1_2 + x1_1·r1_3 + x1_0·r1_4,
            //       x0_4·r2_0 + x0_3·r2_1 + x0_2·r2_2 + x0_1·r2_3 + x0_0·r2_4 +   x1_4·r1_0 + x1_3·r1_1 + x1_2·r1_2 + x1_1·r1_3 + x1_0·r1_4,
            // ]
            v1 = _mm256_add_epi64(v1, t0);

            // The result:
            // v1 = [
            //     _, _, _,
            //     x0_4·r2_0 +   x0_3·r2_1 +   x0_2·r2_2 +   x0_1·r2_3 +   x0_0·r2_4 + x1_4·r1_0 +   x1_3·r1_1 +   x1_2·r1_2 +   x1_1·r1_3 +   x1_0·r1_4,
            // ]
            // v0 = [
            //     x0_3·r2_0 +   x0_2·r2_1 +   x0_1·r2_2 +   x0_0·r2_3 + 5·x0_4·r2_4 + x1_3·r1_0 +   x1_2·r1_1 +   x1_1·r1_2 +   x1_0·r1_3 + 5·x1_4·r1_4,
            //     x0_2·r2_0 +   x0_1·r2_1 +   x0_0·r2_2 + 5·x0_4·r2_3 + 5·x0_3·r2_4 + x1_2·r1_0 +   x1_1·r1_1 +   x1_0·r1_2 + 5·x1_4·r1_3 + 5·x1_3·r1_4,
            //     x0_1·r2_0 +   x0_0·r2_1 + 5·x0_4·r2_2 + 5·x0_3·r2_3 + 5·x0_2·r2_4 + x1_1·r1_0 +   x1_0·r1_1 + 5·x1_4·r1_2 + 5·x1_3·r1_3 + 5·x1_2·r1_4,
            //     x0_0·r2_0 + 5·x0_4·r2_1 + 5·x0_3·r2_2 + 5·x0_2·r2_3 + 5·x0_1·r2_4 + x1_0·r1_0 + 5·x1_4·r1_1 + 5·x1_3·r1_2 + 5·x1_2·r1_3 + 5·x1_1·r1_4,
            // ]
            Unreduced130 { v0, v1 }
        }
    }
}

impl Add<Aligned130> for Aligned2x130 {
    type Output = Aligned2x130;

    /// Adds `other` into the lower integer of `self`.
    fn add(self, other: Aligned130) -> Aligned2x130 {
        Aligned2x130 {
            v0: self.v0 + other,
            v1: self.v1,
        }
    }
}

/// A multiplier that takes 130-bit integers `(x3, x2, x1, x0)` and computes
/// `(x3·R^4, x2·R^3, x1·R^2, x0·R) mod 2^130 - 5`.
#[derive(Copy, Clone, Debug)]
pub(super) struct SpacedMultiplier4x130 {
    v0: __m256i,
    v1: __m256i,
    r1: PrecomputedMultiplier,
}

impl SpacedMultiplier4x130 {
    /// Returns `(multipler, R^4)` given `(R^1, R^2)`.
    #[target_feature(enable = "avx2")]
    pub(super) unsafe fn new(
        r1: PrecomputedMultiplier,
        r2: PrecomputedMultiplier,
    ) -> (Self, PrecomputedMultiplier) {
        let r3 = (r2 * r1).reduce();
        let r4 = (r2 * r2).reduce();

        // v0 = [r2_4, r2_3, r2_1, r3_4, r3_3, r3_2, r3_1, r3_0]
        let v0 = _mm256_blend_epi32(
            r3.0,
            _mm256_permutevar8x32_epi32(r2.a, _mm256_set_epi32(4, 3, 1, 0, 0, 0, 0, 0)),
            0b11100000,
        );

        // v1 = [r2_4, r2_2, r2_0, r4_4, r4_3, r4_2, r4_1, r4_0]
        let v1 = _mm256_blend_epi32(
            r4.0,
            _mm256_permutevar8x32_epi32(r2.a, _mm256_set_epi32(4, 2, 0, 0, 0, 0, 0, 0)),
            0b11100000,
        );

        let m = SpacedMultiplier4x130 { v0, v1, r1 };

        (m, r4.into())
    }
}

/// Four 130-bit integers aligned across five 26-bit limbs each.
///
/// Unlike `Aligned2x130` which wraps two `Aligned130`s, this struct represents the four
/// integers as 20 limbs spread across three 256-bit vectors.
#[derive(Copy, Clone, Debug)]
pub(super) struct Aligned4x130 {
    v0: __m256i,
    v1: __m256i,
    v2: __m256i,
}

impl fmt::Display for Aligned4x130 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut v0 = [0u8; 32];
        let mut v1 = [0u8; 32];
        let mut v2 = [0u8; 32];
        unsafe {
            _mm256_storeu_si256(v0.as_mut_ptr() as *mut _, self.v0);
            _mm256_storeu_si256(v1.as_mut_ptr() as *mut _, self.v1);
            _mm256_storeu_si256(v2.as_mut_ptr() as *mut _, self.v2);
        }

        writeln!(f, "Aligned4x130([")?;
        write!(f, "    ")?;
        write_130(
            f,
            [
                u32::from_le_bytes(v0[0..4].try_into().unwrap()),
                u32::from_le_bytes(v1[0..4].try_into().unwrap()),
                u32::from_le_bytes(v0[4..8].try_into().unwrap()),
                u32::from_le_bytes(v1[4..8].try_into().unwrap()),
                u32::from_le_bytes(v2[0..4].try_into().unwrap()),
            ],
        )?;
        writeln!(f, ",")?;
        write!(f, "    ")?;
        write_130(
            f,
            [
                u32::from_le_bytes(v0[8..12].try_into().unwrap()),
                u32::from_le_bytes(v1[8..12].try_into().unwrap()),
                u32::from_le_bytes(v0[12..16].try_into().unwrap()),
                u32::from_le_bytes(v1[12..16].try_into().unwrap()),
                u32::from_le_bytes(v2[8..12].try_into().unwrap()),
            ],
        )?;
        writeln!(f, ",")?;
        write!(f, "    ")?;
        write_130(
            f,
            [
                u32::from_le_bytes(v0[16..20].try_into().unwrap()),
                u32::from_le_bytes(v1[16..20].try_into().unwrap()),
                u32::from_le_bytes(v0[20..24].try_into().unwrap()),
                u32::from_le_bytes(v1[20..24].try_into().unwrap()),
                u32::from_le_bytes(v2[16..20].try_into().unwrap()),
            ],
        )?;
        writeln!(f, ",")?;
        write!(f, "    ")?;
        write_130(
            f,
            [
                u32::from_le_bytes(v0[24..28].try_into().unwrap()),
                u32::from_le_bytes(v1[24..28].try_into().unwrap()),
                u32::from_le_bytes(v0[28..32].try_into().unwrap()),
                u32::from_le_bytes(v1[28..32].try_into().unwrap()),
                u32::from_le_bytes(v2[24..28].try_into().unwrap()),
            ],
        )?;
        writeln!(f, ",")?;
        write!(f, "])")
    }
}

impl Aligned4x130 {
    /// Aligns four 16-byte Poly1305 blocks at 26-bit boundaries within 32-bit words, and
    /// sets the high bit for each block.
    ///
    /// # Panics
    ///
    /// Panics if `src.len() < 64`.
    #[target_feature(enable = "avx2")]
    pub(super) unsafe fn from_blocks(src: &[Block; 4]) -> Self {
        let (lo, hi) = src.split_at(2);
        let blocks_23 = _mm256_loadu_si256(hi.as_ptr() as *const _);
        let blocks_01 = _mm256_loadu_si256(lo.as_ptr() as *const _);

        Self::from_loaded_blocks(blocks_01, blocks_23)
    }

    /// Aligns four 16-byte Poly1305 blocks at 26-bit boundaries within 32-bit words, and
    /// sets the high bit for each block.
    #[target_feature(enable = "avx2")]
    pub(super) unsafe fn from_par_blocks(src: &ParBlocks) -> Self {
        let (lo, hi) = src.split_at(2);
        let blocks_23 = _mm256_loadu_si256(hi.as_ptr() as *const _);
        let blocks_01 = _mm256_loadu_si256(lo.as_ptr() as *const _);

        Self::from_loaded_blocks(blocks_01, blocks_23)
    }

    /// Aligns four 16-byte Poly1305 blocks at 26-bit boundaries within 32-bit words, and
    /// sets the high bit for each block.
    ///
    /// The four blocks must be in the following 32-bit word layout:
    ///      [b33, b32, b31, b30, b23, b22, b21, b20]
    ///      [b13, b12, b11, b10, b03, b02, b01, b00]
    #[target_feature(enable = "avx2")]
    unsafe fn from_loaded_blocks(blocks_01: __m256i, blocks_23: __m256i) -> Self {
        // 26-bit mask on each 32-bit word.
        let mask_26 = _mm256_set1_epi32(0x3ffffff);
        // Sets bit 24 of each 32-bit word.
        let set_hibit = _mm256_set1_epi32(1 << 24);

        // - Unpack the upper and lower 64 bits:
        //      [b33, b32, b13, b12, b23, b22, b03, b02]
        //      [b31, b30, b11, b10, b21, b20, b01, b00]
        //
        // - Swap the middle two 64-bit words:
        // a0 = [b33, b32, b23, b22, b13, b12, b03, b02]
        // a1 = [b31, b30, b21, b20, b11, b10, b01, b00]
        let a0 = _mm256_permute4x64_epi64(
            _mm256_unpackhi_epi64(blocks_01, blocks_23),
            set02(3, 1, 2, 0),
        );
        let a1 = _mm256_permute4x64_epi64(
            _mm256_unpacklo_epi64(blocks_01, blocks_23),
            set02(3, 1, 2, 0),
        );

        // - Take the upper 24 bits of each 64-bit word in a0, and set the high bits:
        // v2 = [
        //     [0; 7] || 1 || [0; 31] || 1 || b33[32..8],
        //     [0; 7] || 1 || [0; 31] || 1 || b23[32..8],
        //     [0; 7] || 1 || [0; 31] || 1 || b13[32..8],
        //     [0; 7] || 1 || [0; 31] || 1 || b03[32..8],
        // ]
        let v2 = _mm256_or_si256(_mm256_srli_epi64(a0, 40), set_hibit);

        // - Combine the lower 46 bits of each 64-bit word in a0 with the upper 18
        //   bits of each 64-bit word in a1:
        // a2 = [
        //     b33[14..0] || b32 || b31[32..14],
        //     b23[14..0] || b22 || b21[32..14],
        //     b13[14..0] || b12 || b11[32..14],
        //     b03[14..0] || b02 || b01[32..14],
        // ]
        let a2 = _mm256_or_si256(_mm256_srli_epi64(a1, 46), _mm256_slli_epi64(a0, 18));

        // - Take the upper 38 bits of each 64-bit word in a1:
        // [
        //     [0; 26] || b31 || b30[32..26],
        //     [0; 26] || b21 || b20[32..26],
        //     [0; 26] || b11 || b10[32..26],
        //     [0; 26] || b01 || b00[32..26],
        // ]
        // - Blend in a2 on 32-bit words with alternating [a2 a1 ..] control pattern:
        // [
        //     b33[14..0] || b32[32..14] || b31[26..0] || b30[32..26],
        //     b23[14..0] || b22[32..14] || b21[26..0] || b20[32..26],
        //     b13[14..0] || b12[32..14] || b11[26..0] || b10[32..26],
        //     b03[14..0] || b02[32..14] || b01[26..0] || b00[32..26],
        // ]
        // - Apply the 26-bit mask to each 32-bit word:
        // v1 = [
        //     [0; 6] || b33[8..0] || b32[32..14] || [0; 6] || b31[20..0] || b30[32..26],
        //     [0; 6] || b23[8..0] || b22[32..14] || [0; 6] || b21[20..0] || b20[32..26],
        //     [0; 6] || b13[8..0] || b12[32..14] || [0; 6] || b11[20..0] || b10[32..26],
        //     [0; 6] || b03[8..0] || b02[32..14] || [0; 6] || b01[20..0] || b00[32..26],
        // ]
        let v1 = _mm256_and_si256(
            _mm256_blend_epi32(_mm256_srli_epi64(a1, 26), a2, 0xAA),
            mask_26,
        );

        // - Take the lower 38 bits of each 64-bit word in a2:
        // [
        //     b32[20..0] || b31[32..14] || [0; 26],
        //     b22[20..0] || b21[32..14] || [0; 26],
        //     b12[20..0] || b11[32..14] || [0; 26],
        //     b02[20..0] || b01[32..14] || [0; 26],
        // ]
        // - Blend in a1 on 32-bit words with alternating [a2 a1 ..] control pattern:
        // [
        //     b32[20..0] || b31[32..20] || b30,
        //     b22[20..0] || b21[32..20] || b20,
        //     b12[20..0] || b11[32..20] || b10,
        //     b02[20..0] || b01[32..20] || b00,
        // ]
        // - Apply the 26-bit mask to each 32-bit word:
        // v0 = [
        //     [0; 6] || b32[14..0] || b31[32..20] || [0; 6] || b30[26..0],
        //     [0; 6] || b22[14..0] || b21[32..20] || [0; 6] || b20[26..0],
        //     [0; 6] || b12[14..0] || b11[32..20] || [0; 6] || b10[26..0],
        //     [0; 6] || b02[14..0] || b01[32..20] || [0; 6] || b00[26..0],
        // ]
        let v0 = _mm256_and_si256(
            _mm256_blend_epi32(a1, _mm256_slli_epi64(a2, 26), 0xAA),
            mask_26,
        );

        // The result:
        // v2 = [                         v1 = [                                   v0 = [
        //     [0; 7] || 1 ||    [0; 24],     [0; 6] || b33[ 8..0] || b32[32..14],     [0; 6] || b32[14..0] || b31[32..20],
        //     [0; 7] || 1 || b33[32..8],     [0; 6] || b31[20..0] || b30[32..26],     [0; 6] || b30[26..0],
        //     [0; 7] || 1 ||    [0; 24],     [0; 6] || b23[ 8..0] || b22[32..14],     [0; 6] || b22[14..0] || b21[32..20],
        //     [0; 7] || 1 || b23[32..8],     [0; 6] || b21[20..0] || b20[32..26],     [0; 6] || b20[26..0],
        //     [0; 7] || 1 ||    [0; 24],     [0; 6] || b13[ 8..0] || b12[32..14],     [0; 6] || b12[14..0] || b11[32..20],
        //     [0; 7] || 1 || b13[32..8],     [0; 6] || b11[20..0] || b10[32..26],     [0; 6] || b10[26..0],
        //     [0; 7] || 1 ||    [0; 24],     [0; 6] || b03[ 8..0] || b02[32..14],     [0; 6] || b02[14..0] || b01[32..20],
        //     [0; 7] || 1 || b03[32..8],     [0; 6] || b01[20..0] || b00[32..26],     [0; 6] || b00[26..0],
        // ]                              ]                                        ]
        Aligned4x130 { v0, v1, v2 }
    }
}

impl Add<Aligned4x130> for Aligned4x130 {
    type Output = Aligned4x130;

    #[inline(always)]
    fn add(self, other: Aligned4x130) -> Aligned4x130 {
        // With 26-bit limbs inside 32-bit words, there is plenty of space for unreduced
        // addition.
        unsafe {
            Aligned4x130 {
                v0: _mm256_add_epi32(self.v0, other.v0),
                v1: _mm256_add_epi32(self.v1, other.v1),
                v2: _mm256_add_epi32(self.v2, other.v2),
            }
        }
    }
}

impl Mul<PrecomputedMultiplier> for &Aligned4x130 {
    type Output = Unreduced4x130;

    #[inline(always)]
    fn mul(self, other: PrecomputedMultiplier) -> Unreduced4x130 {
        unsafe {
            // Starting with the following limb layout:
            // x.v2 = [    _,   x34,     _,   x24,     _,   x14,     _,   x04]
            // x.v1 = [  x33,   x31,   x23,   x21,   x13,   x11,   x03,   x01]
            // x.v0 = [  x32,   x30,   x22,   x20,   x12,   x10,   x02,   x00]
            // y =    [5·r_4, 5·r_3, 5·r_2,   r_4,   r_3,   r_2,   r_1,   r_0]
            // z =    [5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1, 5·r_1]
            let mut x = *self;
            let y = other.a;
            let z = other.a_5;

            // Prepare a permutation that swaps the two limbs within a 64-bit window.
            let ord = _mm256_set_epi32(6, 7, 4, 5, 2, 3, 0, 1);

            // t0 = [r_1, r_0, r_1, r_0, r_1, r_0, r_1, r_0] -> ·r_0
            // t1 = [r_3, r_2, r_3, r_2, r_3, r_2, r_3, r_2] -> ·r_2
            let mut t0 = _mm256_permute4x64_epi64(y, set02(0, 0, 0, 0));
            let mut t1 = _mm256_permute4x64_epi64(y, set02(1, 1, 1, 1));

            // v0 = [x30·r_0, x20·r_0, x10·r_0, x00·r_0]
            // v1 = [x31·r_0, x21·r_0, x11·r_0, x01·r_0]
            // v4 = [x34·r_0, x24·r_0, x14·r_0, x04·r_0]
            // v2 = [x30·r_2, x20·r_2, x10·r_2, x00·r_2]
            // v3 = [x31·r_2, x21·r_2, x11·r_2, x01·r_2]
            let mut v0 = _mm256_mul_epu32(x.v0, t0); // xN0·r_0
            let mut v1 = _mm256_mul_epu32(x.v1, t0); // xN1·r_0
            let mut v4 = _mm256_mul_epu32(x.v2, t0); // xN4·r_0
            let mut v2 = _mm256_mul_epu32(x.v0, t1); // xN0·r_2
            let mut v3 = _mm256_mul_epu32(x.v1, t1); // xN1·r_2

            // t0 = [r_0, r_1, r_0, r_1, r_0, r_1, r_0, r_1] -> ·r_1
            // t1 = [r_2, r_3, r_2, r_3, r_2, r_3, r_2, r_3] -> ·r_3
            t0 = _mm256_permutevar8x32_epi32(t0, ord);
            t1 = _mm256_permutevar8x32_epi32(t1, ord);

            // v1 = [x31·r_0 + x30·r_1, x21·r_0 + x20·r_1, x11·r_0 + x10·r_1, x01·r_0 + x00·r_1]
            // v2 = [x31·r_1 + x30·r_2, x21·r_1 + x20·r_2, x11·r_1 + x10·r_2, x01·r_1 + x00·r_2]
            // v3 = [x31·r_2 + x30·r_3, x21·r_2 + x20·r_3, x11·r_2 + x10·r_3, x01·r_2 + x00·r_3]
            // v4 = [x34·r_0 + x31·r_3, x24·r_0 + x21·r_3, x14·r_0 + x11·r_3, x04·r_0 + x01·r_3]
            v1 = _mm256_add_epi64(v1, _mm256_mul_epu32(x.v0, t0)); // + xN0·r_1
            v2 = _mm256_add_epi64(v2, _mm256_mul_epu32(x.v1, t0)); // + xN1·r_1
            v3 = _mm256_add_epi64(v3, _mm256_mul_epu32(x.v0, t1)); // + xN0·r_3
            v4 = _mm256_add_epi64(v4, _mm256_mul_epu32(x.v1, t1)); // + xN1·r_3

            // t2 = [5·r_2, r_4, 5·r_2, r_4, 5·r_2, r_4, 5·r_2, r_4] -> ·r_4
            let mut t2 = _mm256_permute4x64_epi64(y, set02(2, 2, 2, 2));

            // v4 = [
            //     x34·r_0 + x31·r_3 + x30·r_4,
            //     x24·r_0 + x21·r_3 + x20·r_4,
            //     x14·r_0 + x11·r_3 + x10·r_4,
            //     x04·r_0 + x01·r_3 + x00·r_4,
            // ]
            v4 = _mm256_add_epi64(v4, _mm256_mul_epu32(x.v0, t2)); // + xN0·r_4

            // x.v0 = [x30,   x32, x20,   x22, x10,   x12, x00,   x02]
            // x.v1 = [x31,   x33, x21,   x23, x11,   x13, x01,   x03]
            // t2   = [r_4, 5·r_2, r_4, 5·r_2, r_4, 5·r_2, r_4, 5·r_2] -> ·5·r_2
            x.v0 = _mm256_permutevar8x32_epi32(x.v0, ord);
            x.v1 = _mm256_permutevar8x32_epi32(x.v1, ord);
            t2 = _mm256_permutevar8x32_epi32(t2, ord);

            // v0 = [
            //     x30·r_0           + 5·x33·r_2,
            //     x20·r_0           + 5·x23·r_2,
            //     x10·r_0           + 5·x13·r_2,
            //     x00·r_0           + 5·x03·r_2,
            // ]
            // v1 = [
            //     x31·r_0 + x30·r_1 + 5·x34·r_2,
            //     x21·r_0 + x20·r_1 + 5·x24·r_2,
            //     x11·r_0 + x10·r_1 + 5·x14·r_2,
            //     x01·r_0 + x00·r_1 + 5·x04·r_2,
            // ]
            // v3 = [
            //               x32·r_1 +   x31·r_2 + x30·r_3,
            //               x22·r_1 +   x21·r_2 + x20·r_3,
            //               x12·r_1 +   x11·r_2 + x10·r_3,
            //               x02·r_1 +   x01·r_2 + x00·r_3,
            // ]
            // v4 = [
            //     x34·r_0 + x33·r_1             + x31·r_3 + x30·r_4,
            //     x24·r_0 + x23·r_1             + x21·r_3 + x20·r_4,
            //     x14·r_0 + x13·r_1             + x11·r_3 + x10·r_4,
            //     x04·r_0 + x03·r_1             + x01·r_3 + x00·r_4,
            // ]
            v0 = _mm256_add_epi64(v0, _mm256_mul_epu32(x.v1, t2)); // + 5·xN3·r_2
            v1 = _mm256_add_epi64(v1, _mm256_mul_epu32(x.v2, t2)); // + 5·xN4·r_2
            v3 = _mm256_add_epi64(v3, _mm256_mul_epu32(x.v0, t0)); // +   xN2·r_1
            v4 = _mm256_add_epi64(v4, _mm256_mul_epu32(x.v1, t0)); // +   xN3·r_1

            // t0 = [r_1, r_0, r_1, r_0, r_1, r_0, r_1, r_0] -> ·r_0
            // t1 = [r_3, r_2, r_3, r_2, r_3, r_2, r_3, r_2] -> ·r_2
            t0 = _mm256_permutevar8x32_epi32(t0, ord);
            t1 = _mm256_permutevar8x32_epi32(t1, ord);

            // v2 = [
            //     x32·r_0 + x31·r_1 + x30·r_2,
            //     x22·r_0 + x21·r_1 + x20·r_2,
            //     x12·r_0 + x11·r_1 + x10·r_2,
            //     x02·r_0 + x01·r_1 + x00·r_2,
            // ]
            // v3 = [
            //     x33·r_0 + x32·r_1 + x31·r_2 + x30·r_3,
            //     x23·r_0 + x22·r_1 + x21·r_2 + x20·r_3,
            //     x13·r_0 + x12·r_1 + x11·r_2 + x10·r_3,
            //     x03·r_0 + x02·r_1 + x01·r_2 + x00·r_3,
            // ]
            // v4 = [
            //     x34·r_0 + x33·r_1 + x32·r_2 + x31·r_3 + x30·r_4,
            //     x24·r_0 + x23·r_1 + x22·r_2 + x21·r_3 + x20·r_4,
            //     x14·r_0 + x13·r_1 + x12·r_2 + x11·r_3 + x10·r_4,
            //     x04·r_0 + x03·r_1 + x02·r_2 + x01·r_3 + x00·r_4,
            // ]
            v2 = _mm256_add_epi64(v2, _mm256_mul_epu32(x.v0, t0)); // + xN2·r_0
            v3 = _mm256_add_epi64(v3, _mm256_mul_epu32(x.v1, t0)); // + xN3·r_0
            v4 = _mm256_add_epi64(v4, _mm256_mul_epu32(x.v0, t1)); // + xN2·r_2

            // t0 = [5·r_4, 5·r_3, 5·r_4, 5·r_3, 5·r_4, 5·r_3, 5·r_4, 5·r_3] -> ·5·r_3
            t0 = _mm256_permute4x64_epi64(y, set02(3, 3, 3, 3));

            // v0 = [
            //     x30·r_0           + 5·x33·r_2 + 5·x32·r_3,
            //     x20·r_0           + 5·x23·r_2 + 5·x22·r_3,
            //     x10·r_0           + 5·x13·r_2 + 5·x12·r_3,
            //     x00·r_0           + 5·x03·r_2 + 5·x02·r_3,
            // ]
            // v1 = [
            //     x31·r_0 + x30·r_1 + 5·x34·r_2 + 5·x33·r_3,
            //     x21·r_0 + x20·r_1 + 5·x24·r_2 + 5·x23·r_3,
            //     x11·r_0 + x10·r_1 + 5·x14·r_2 + 5·x13·r_3,
            //     x01·r_0 + x00·r_1 + 5·x04·r_2 + 5·x03·r_3,
            // ]
            // v2 = [
            //     x32·r_0 + x31·r_1 +   x30·r_2 + 5·x34·r_3,
            //     x22·r_0 + x21·r_1 +   x20·r_2 + 5·x24·r_3,
            //     x12·r_0 + x11·r_1 +   x10·r_2 + 5·x14·r_3,
            //     x02·r_0 + x01·r_1 +   x00·r_2 + 5·x04·r_3,
            // ]
            v0 = _mm256_add_epi64(v0, _mm256_mul_epu32(x.v0, t0)); // + 5·xN2·r_3
            v1 = _mm256_add_epi64(v1, _mm256_mul_epu32(x.v1, t0)); // + 5·xN3·r_3
            v2 = _mm256_add_epi64(v2, _mm256_mul_epu32(x.v2, t0)); // + 5·xN4·r_3

            // t0 = [5·r_3, 5·r_4, 5·r_3, 5·r_4, 5·r_3, 5·r_4, 5·r_3, 5·r_4] -> ·5·r_4
            t0 = _mm256_permutevar8x32_epi32(t0, ord);

            // v1 = [
            //     x31·r_0 + x30·r_1 + 5·x34·r_2 + 5·x33·r_3 + 5·x32·r_4,
            //     x21·r_0 + x20·r_1 + 5·x24·r_2 + 5·x23·r_3 + 5·x22·r_4,
            //     x11·r_0 + x10·r_1 + 5·x14·r_2 + 5·x13·r_3 + 5·x12·r_4,
            //     x01·r_0 + x00·r_1 + 5·x04·r_2 + 5·x03·r_3 + 5·x02·r_4,
            // ]
            // v2 = [
            //     x32·r_0 + x31·r_1 +   x30·r_2 + 5·x34·r_3 + 5·x33·r_4,
            //     x22·r_0 + x21·r_1 +   x20·r_2 + 5·x24·r_3 + 5·x23·r_4,
            //     x12·r_0 + x11·r_1 +   x10·r_2 + 5·x14·r_3 + 5·x13·r_4,
            //     x02·r_0 + x01·r_1 +   x00·r_2 + 5·x04·r_3 + 5·x03·r_4,
            // ]
            // v3 = [
            //     x33·r_0 + x32·r_1 +   x31·r_2 +   x30·r_3 + 5·x34·r_4,
            //     x23·r_0 + x22·r_1 +   x21·r_2 +   x20·r_3 + 5·x24·r_4,
            //     x13·r_0 + x12·r_1 +   x11·r_2 +   x10·r_3 + 5·x14·r_4,
            //     x03·r_0 + x02·r_1 +   x01·r_2 +   x00·r_3 + 5·x04·r_4,
            // ]
            v1 = _mm256_add_epi64(v1, _mm256_mul_epu32(x.v0, t0)); // + 5·xN2·r_4
            v2 = _mm256_add_epi64(v2, _mm256_mul_epu32(x.v1, t0)); // + 5·xN3·r_4
            v3 = _mm256_add_epi64(v3, _mm256_mul_epu32(x.v2, t0)); // + 5·xN4·r_4

            // x.v1 = [x33, x31, x23, x21, x13, x11, x03, x01]
            x.v1 = _mm256_permutevar8x32_epi32(x.v1, ord);

            // v0 = [
            //     x30·r_0 + 5·x34·r_1 + 5·x33·r_2 + 5·x32·r_3 + 5·x31·r_4,
            //     x20·r_0 + 5·x24·r_1 + 5·x23·r_2 + 5·x22·r_3 + 5·x21·r_4,
            //     x10·r_0 + 5·x14·r_1 + 5·x13·r_2 + 5·x12·r_3 + 5·x11·r_4,
            //     x00·r_0 + 5·x04·r_1 + 5·x03·r_2 + 5·x02·r_3 + 5·x01·r_4,
            // ]
            v0 = _mm256_add_epi64(v0, _mm256_mul_epu32(x.v1, t0)); // + 5·xN1·r_4
            v0 = _mm256_add_epi64(v0, _mm256_mul_epu32(x.v2, z)); // + 5·xN4·r_1

            // The result:
            // v4 = [
            //     x34·r_0 +   x33·r_1 +   x32·r_2 +   x31·r_3 +   x30·r_4,
            //     x24·r_0 +   x23·r_1 +   x22·r_2 +   x21·r_3 +   x20·r_4,
            //     x14·r_0 +   x13·r_1 +   x12·r_2 +   x11·r_3 +   x10·r_4,
            //     x04·r_0 +   x03·r_1 +   x02·r_2 +   x01·r_3 +   x00·r_4,
            // ]
            // v3 = [
            //     x33·r_0 +   x32·r_1 +   x31·r_2 +   x30·r_3 + 5·x34·r_4,
            //     x23·r_0 +   x22·r_1 +   x21·r_2 +   x20·r_3 + 5·x24·r_4,
            //     x13·r_0 +   x12·r_1 +   x11·r_2 +   x10·r_3 + 5·x14·r_4,
            //     x03·r_0 +   x02·r_1 +   x01·r_2 +   x00·r_3 + 5·x04·r_4,
            // ]
            // v2 = [
            //     x32·r_0 +   x31·r_1 +   x30·r_2 + 5·x34·r_3 + 5·x33·r_4,
            //     x22·r_0 +   x21·r_1 +   x20·r_2 + 5·x24·r_3 + 5·x23·r_4,
            //     x12·r_0 +   x11·r_1 +   x10·r_2 + 5·x14·r_3 + 5·x13·r_4,
            //     x02·r_0 +   x01·r_1 +   x00·r_2 + 5·x04·r_3 + 5·x03·r_4,
            // ]
            // v1 = [
            //     x31·r_0 +   x30·r_1 + 5·x34·r_2 + 5·x33·r_3 + 5·x32·r_4,
            //     x21·r_0 +   x20·r_1 + 5·x24·r_2 + 5·x23·r_3 + 5·x22·r_4,
            //     x11·r_0 +   x10·r_1 + 5·x14·r_2 + 5·x13·r_3 + 5·x12·r_4,
            //     x01·r_0 +   x00·r_1 + 5·x04·r_2 + 5·x03·r_3 + 5·x02·r_4,
            // ]
            // v0 = [
            //     x30·r_0 + 5·x34·r_1 + 5·x33·r_2 + 5·x32·r_3 + 5·x31·r_4,
            //     x20·r_0 + 5·x24·r_1 + 5·x23·r_2 + 5·x22·r_3 + 5·x21·r_4,
            //     x10·r_0 + 5·x14·r_1 + 5·x13·r_2 + 5·x12·r_3 + 5·x11·r_4,
            //     x00·r_0 + 5·x04·r_1 + 5·x03·r_2 + 5·x02·r_3 + 5·x01·r_4,
            // ]
            Unreduced4x130 { v0, v1, v2, v3, v4 }
        }
    }
}

impl Mul<SpacedMultiplier4x130> for Aligned4x130 {
    type Output = Unreduced4x130;

    #[inline(always)]
    fn mul(self, m: SpacedMultiplier4x130) -> Unreduced4x130 {
        unsafe {
            // Starting with the following limb layout:
            // x.v2 = [     _,    x34,      _,  x24,    _,  x14,    _,  x04]
            // x.v1 = [   x33,    x31,    x23,  x21,  x13,  x11,  x03,  x01]
            // x.v0 = [   x32,    x30,    x22,  x20,  x12,  x10,  x02,  x00]
            // m.v1 = [  r2_4,   r2_2,   r2_0, r4_4, r4_3, r4_2, r4_1, r4_0]
            // m.v0 = [  r2_4,   r2_3,   r2_1, r3_4, r3_3, r3_2, r3_1, r3_0]
            // r1   = [5·r1_4, 5·r1_3, 5·r1_2, r1_4, r1_3, r1_2, r1_1, r1_0]
            let mut x = self;
            let r1 = m.r1.a;

            // v0 = [r2_0, r2_1, r4_4, r3_4, r4_1, r3_1, r4_0, r3_0]
            // v1 = [r2_4, r2_4, r2_2, r2_3, r4_3, r3_3, r4_2, r3_2]
            let v0 = _mm256_unpacklo_epi32(m.v0, m.v1);
            let v1 = _mm256_unpackhi_epi32(m.v0, m.v1);

            // m_r_0 = [r1_1, r1_0, r2_1, r2_0, r3_1, r3_0, r4_1, r4_0] -> ·rN_0
            // m_r_2 = [r1_3, r1_2, r2_3, r2_2, r3_3, r3_2, r4_3, r4_2] -> ·rN_2
            // m_r_4 = [r1_1, r1_4, r2_1, r2_4, r3_1, r3_4, r4_1, r4_4] -> ·rN_4
            let ord = _mm256_set_epi32(1, 0, 6, 7, 2, 0, 3, 1);
            let m_r_0 = _mm256_blend_epi32(
                _mm256_permutevar8x32_epi32(r1, ord),
                _mm256_permutevar8x32_epi32(v0, ord),
                0b00111111,
            );
            let ord = _mm256_set_epi32(3, 2, 4, 5, 2, 0, 3, 1);
            let m_r_2 = _mm256_blend_epi32(
                _mm256_permutevar8x32_epi32(r1, ord),
                _mm256_permutevar8x32_epi32(v1, ord),
                0b00111111,
            );
            let ord = _mm256_set_epi32(1, 4, 6, 6, 2, 4, 3, 5);
            let m_r_4 = _mm256_blend_epi32(
                _mm256_blend_epi32(
                    _mm256_permutevar8x32_epi32(r1, ord),
                    _mm256_permutevar8x32_epi32(v1, ord),
                    0b00010000,
                ),
                _mm256_permutevar8x32_epi32(v0, ord),
                0b00101111,
            );

            // v0 = [x30·r1_0, x20·r2_0, x10·r3_0, x00·r4_0]
            // v1 = [x31·r1_0, x21·r2_0, x11·r3_0, x01·r4_0]
            // v2 = [x30·r1_2, x20·r2_2, x10·r3_2, x00·r4_2]
            // v3 = [x31·r1_2, x21·r2_2, x11·r3_2, x01·r4_2]
            // v4 = [x30·r1_4, x20·r2_4, x10·r3_4, x00·r4_4]
            let mut v0 = _mm256_mul_epu32(x.v0, m_r_0); // xM0·rN_0
            let mut v1 = _mm256_mul_epu32(x.v1, m_r_0); // xM1·rN_0
            let mut v2 = _mm256_mul_epu32(x.v0, m_r_2); // xM0·rN_2
            let mut v3 = _mm256_mul_epu32(x.v1, m_r_2); // xM1·rN_2
            let mut v4 = _mm256_mul_epu32(x.v0, m_r_4); // xM0·rN_4

            // m_r_1 = [r1_0, r1_1, r2_0, r2_1, r3_0, r3_1, r4_0, r4_1] -> ·rN_1
            // m_r_3 = [r1_2, r1_3, r2_2, r2_3, r3_2, r3_3, r4_2, r4_3] -> ·rN_3
            let ord = _mm256_set_epi32(6, 7, 4, 5, 2, 3, 0, 1);
            let m_r_1 = _mm256_permutevar8x32_epi32(m_r_0, ord);
            let m_r_3 = _mm256_permutevar8x32_epi32(m_r_2, ord);

            // v1 = [
            //     x31·r1_0 + x30·r1_1,
            //     x21·r2_0 + x20·r2_1,
            //     x11·r3_0 + x10·r3_1,
            //     x01·r4_0 + x00·r4_1,
            // ]
            // v2 = [
            //                x31·r1_1 + x30·r1_2,
            //                x21·r2_1 + x20·r2_2,
            //                x11·r3_1 + x10·r3_2,
            //                x01·r4_1 + x00·r4_2,
            // ]
            // v3 = [
            //                           x31·r1_2 + x30·r1_3,
            //                           x21·r2_2 + x20·r2_3,
            //                           x11·r3_2 + x10·r3_3,
            //                           x01·r4_2 + x00·r4_3,
            // ]
            // v4 = [
            //     x34·r1_0 +                       x31·r1_3 + x30·r1_4,
            //     x24·r2_0 +                       x21·r2_3 + x20·r2_4,
            //     x14·r3_0 +                       x11·r3_3 + x10·r3_4,
            //     x04·r4_0 +                       x01·r4_3 + x00·r4_4,
            // ]
            v1 = _mm256_add_epi64(v1, _mm256_mul_epu32(x.v0, m_r_1)); // + xM0·rN_1
            v2 = _mm256_add_epi64(v2, _mm256_mul_epu32(x.v1, m_r_1)); // + xM1·rN_1
            v3 = _mm256_add_epi64(v3, _mm256_mul_epu32(x.v0, m_r_3)); // + xM0·rN_3
            v4 = _mm256_add_epi64(v4, _mm256_mul_epu32(x.v1, m_r_3)); // + xM1·rN_3
            v4 = _mm256_add_epi64(v4, _mm256_mul_epu32(x.v2, m_r_0)); // + xM4·rN_0

            // x.v0 = [x30, x32, x20, x22, x10, x12, x00, x02]
            x.v0 = _mm256_permutevar8x32_epi32(x.v0, ord);

            // v2 = [
            //     x32·r1_0 + x31·r1_1 + x30·r1_2,
            //     x22·r2_0 + x21·r2_1 + x20·r2_2,
            //     x12·r3_0 + x11·r3_1 + x10·r3_2,
            //     x02·r4_0 + x01·r4_1 + x00·r4_2,
            // ]
            // v3 = [
            //                x32·r1_1 + x31·r1_2 + x30·r1_3,
            //                x22·r2_1 + x21·r2_2 + x20·r2_3,
            //                x12·r3_1 + x11·r3_2 + x10·r3_3,
            //                x02·r4_1 + x01·r4_2 + x00·r4_3,
            // ]
            // v4 = [
            //     x34·r1_0 +            x32·r1_2 + x31·r1_3 + x30·r1_4,
            //     x24·r2_0 +            x22·r2_2 + x21·r2_3 + x20·r2_4,
            //     x14·r3_0 +            x12·r3_2 + x11·r3_3 + x10·r3_4,
            //     x04·r4_0 +            x02·r4_2 + x01·r4_3 + x00·r4_4,
            // ]
            v2 = _mm256_add_epi64(v2, _mm256_mul_epu32(x.v0, m_r_0)); // + xM2·rN_0
            v3 = _mm256_add_epi64(v3, _mm256_mul_epu32(x.v0, m_r_1)); // + xM2·rN_1
            v4 = _mm256_add_epi64(v4, _mm256_mul_epu32(x.v0, m_r_2)); // + xM2·rN_2

            // m_5r_3 = [5·r1_2, 5·r1_3, 5·r2_2, 5·r2_3, 5·r3_2, 5·r3_3, 5·r4_2, 5·r4_3] -> ·5·rN_3
            // m_5r_4 = [5·r1_1, 5·r1_4, 5·r2_1, 5·r2_4, 5·r3_1, 5·r3_4, 5·r4_1, 5·r4_4] -> ·5·rN_4
            let m_5r_3 = _mm256_add_epi32(m_r_3, _mm256_slli_epi32(m_r_3, 2));
            let m_5r_4 = _mm256_add_epi32(m_r_4, _mm256_slli_epi32(m_r_4, 2));

            // v0 = [
            //     x30·r1_0                       + 5·x32·r1_3 + 5·x31·r1_4,
            //     x20·r2_0                       + 5·x22·r2_3 + 5·x21·r2_4,
            //     x10·r3_0                       + 5·x12·r3_3 + 5·x11·r3_4,
            //     x00·r4_0                       + 5·x02·r4_3 + 5·x01·r4_4,
            // ]
            // v1 = [
            //     x31·r1_0 + x30·r1_1                         + 5·x32·r1_4,
            //     x21·r2_0 + x20·r2_1                         + 5·x22·r2_4,
            //     x11·r3_0 + x10·r3_1                         + 5·x12·r3_4,
            //     x01·r4_0 + x00·r4_1                         + 5·x02·r4_4,
            // ]
            // v2 = [
            //     x32·r1_0 + x31·r1_1 + x30·r1_2 + 5·x34·r1_3,
            //     x22·r2_0 + x21·r2_1 + x20·r2_2 + 5·x24·r2_3,
            //     x12·r3_0 + x11·r3_1 + x10·r3_2 + 5·x14·r3_3,
            //     x02·r4_0 + x01·r4_1 + x00·r4_2 + 5·x04·r4_3,
            // ]
            // v3 = [
            //                x32·r1_1 + x31·r1_2 +   x30·r1_3 + 5·x34·r1_4,
            //                x22·r2_1 + x21·r2_2 +   x20·r2_3 + 5·x24·r2_4,
            //                x12·r3_1 + x11·r3_2 +   x10·r3_3 + 5·x14·r3_4,
            //                x02·r4_1 + x01·r4_2 +   x00·r4_3 + 5·x04·r4_4,
            // ]
            v0 = _mm256_add_epi64(v0, _mm256_mul_epu32(x.v0, m_5r_3)); // + 5·xM2·rN_3
            v0 = _mm256_add_epi64(v0, _mm256_mul_epu32(x.v1, m_5r_4)); // + 5·xM1·rN_4
            v1 = _mm256_add_epi64(v1, _mm256_mul_epu32(x.v0, m_5r_4)); // + 5·xM2·rN_4
            v2 = _mm256_add_epi64(v2, _mm256_mul_epu32(x.v2, m_5r_3)); // + 5·xM4·rN_3
            v3 = _mm256_add_epi64(v3, _mm256_mul_epu32(x.v2, m_5r_4)); // + 5·xM4·rN_4

            // x.v1 = [x31, x33, x21, x23, x11, x13, x01, x03]
            x.v1 = _mm256_permutevar8x32_epi32(x.v1, ord);

            // v1 = [
            //     x31·r1_0 + x30·r1_1            + 5·x33·r1_3 + 5·x32·r1_4,
            //     x21·r2_0 + x20·r2_1            + 5·x23·r2_3 + 5·x22·r2_4,
            //     x11·r3_0 + x10·r3_1            + 5·x13·r3_3 + 5·x12·r3_4,
            //     x01·r4_0 + x00·r4_1            + 5·x03·r4_3 + 5·x02·r4_4,
            // ]
            // v2 = [
            //     x32·r1_0 + x31·r1_1 + x30·r1_2 + 5·x34·r1_3 + 5·x33·r1_4,
            //     x22·r2_0 + x21·r2_1 + x20·r2_2 + 5·x24·r2_3 + 5·x23·r2_4,
            //     x12·r3_0 + x11·r3_1 + x10·r3_2 + 5·x14·r3_3 + 5·x13·r3_4,
            //     x02·r4_0 + x01·r4_1 + x00·r4_2 + 5·x04·r4_3 + 5·x03·r4_4,
            // ]
            // v3 = [
            //     x33·r1_0 + x32·r1_1 + x31·r1_2 +   x30·r1_3 + 5·x34·r1_4,
            //     x23·r2_0 + x22·r2_1 + x21·r2_2 +   x20·r2_3 + 5·x24·r2_4,
            //     x13·r3_0 + x12·r3_1 + x11·r3_2 +   x10·r3_3 + 5·x14·r3_4,
            //     x03·r4_0 + x02·r4_1 + x01·r4_2 +   x00·r4_3 + 5·x04·r4_4,
            // ]
            // v4 = [
            //     x34·r1_0 + x33·r1_1 + x32·r1_2 +   x31·r1_3 +   x30·r1_4,
            //     x24·r2_0 + x23·r2_1 + x22·r2_2 +   x21·r2_3 +   x20·r2_4,
            //     x14·r3_0 + x13·r3_1 + x12·r3_2 +   x11·r3_3 +   x10·r3_4,
            //     x04·r4_0 + x03·r4_1 + x02·r4_2 +   x01·r4_3 +   x00·r4_4,
            // ]
            v1 = _mm256_add_epi64(v1, _mm256_mul_epu32(x.v1, m_5r_3)); // + 5·xM3·rN_3
            v2 = _mm256_add_epi64(v2, _mm256_mul_epu32(x.v1, m_5r_4)); // + 5·xM3·rN_4
            v3 = _mm256_add_epi64(v3, _mm256_mul_epu32(x.v1, m_r_0)); //  +   xM3·rN_0
            v4 = _mm256_add_epi64(v4, _mm256_mul_epu32(x.v1, m_r_1)); //  +   xM3·rN_1

            // m_5r_1 = [5·r1_4, 5·r1_1, 5·r2_4, 5·r2_1, 5·r3_4, 5·r3_1, 5·r4_4, 5·r4_1] -> ·5·rN_1
            // m_5r_2 = [5·r1_3, 5·r1_2, 5·r2_3, 5·r2_2, 5·r3_3, 5·r3_2, 5·r4_3, 5·r4_2] -> ·5·rN_2
            let m_5r_1 = _mm256_permutevar8x32_epi32(m_5r_4, ord);
            let m_5r_2 = _mm256_permutevar8x32_epi32(m_5r_3, ord);

            // v0 = [
            //     x30·r1_0 + 5·x34·r1_1 + 5·x33·r1_2 + 5·x32·r1_3 + 5·x31·r1_4,
            //     x20·r2_0 + 5·x24·r2_1 + 5·x23·r2_2 + 5·x22·r2_3 + 5·x21·r2_4,
            //     x10·r3_0 + 5·x14·r3_1 + 5·x13·r3_2 + 5·x12·r3_3 + 5·x11·r3_4,
            //     x00·r4_0 + 5·x04·r4_1 + 5·x03·r4_2 + 5·x02·r4_3 + 5·x01·r4_4,
            // ]
            // v1 = [
            //     x31·r1_0 +   x30·r1_1 + 5·x34·r1_2 + 5·x33·r1_3 + 5·x32·r1_4,
            //     x21·r2_0 +   x20·r2_1 + 5·x24·r2_2 + 5·x23·r2_3 + 5·x22·r2_4,
            //     x11·r3_0 +   x10·r3_1 + 5·x14·r3_2 + 5·x13·r3_3 + 5·x12·r3_4,
            //     x01·r4_0 +   x00·r4_1 + 5·x04·r4_2 + 5·x03·r4_3 + 5·x02·r4_4,
            // ]
            v0 = _mm256_add_epi64(v0, _mm256_mul_epu32(x.v1, m_5r_2)); // + 5·xM3·rN_2
            v0 = _mm256_add_epi64(v0, _mm256_mul_epu32(x.v2, m_5r_1)); // + 5·xM4·rN_1
            v1 = _mm256_add_epi64(v1, _mm256_mul_epu32(x.v2, m_5r_2)); // + 5·xM4·rN_2

            // The result:
            // v4 = [
            //     x34·r1_0 +   x33·r1_1 +   x32·r1_2 +   x31·r1_3 +   x30·r1_4,
            //     x24·r2_0 +   x23·r2_1 +   x22·r2_2 +   x21·r2_3 +   x20·r2_4,
            //     x14·r3_0 +   x13·r3_1 +   x12·r3_2 +   x11·r3_3 +   x10·r3_4,
            //     x04·r4_0 +   x03·r4_1 +   x02·r4_2 +   x01·r4_3 +   x00·r4_4,
            // ]
            // v3 = [
            //     x33·r1_0 +   x32·r1_1 +   x31·r1_2 +   x30·r1_3 + 5·x34·r1_4,
            //     x23·r2_0 +   x22·r2_1 +   x21·r2_2 +   x20·r2_3 + 5·x24·r2_4,
            //     x13·r3_0 +   x12·r3_1 +   x11·r3_2 +   x10·r3_3 + 5·x14·r3_4,
            //     x03·r4_0 +   x02·r4_1 +   x01·r4_2 +   x00·r4_3 + 5·x04·r4_4,
            // ]
            // v2 = [
            //     x32·r1_0 +   x31·r1_1 +   x30·r1_2 + 5·x34·r1_3 + 5·x33·r1_4,
            //     x22·r2_0 +   x21·r2_1 +   x20·r2_2 + 5·x24·r2_3 + 5·x23·r2_4,
            //     x12·r3_0 +   x11·r3_1 +   x10·r3_2 + 5·x14·r3_3 + 5·x13·r3_4,
            //     x02·r4_0 +   x01·r4_1 +   x00·r4_2 + 5·x04·r4_3 + 5·x03·r4_4,
            // ]
            // v1 = [
            //     x31·r1_0 +   x30·r1_1 + 5·x34·r1_2 + 5·x33·r1_3 + 5·x32·r1_4,
            //     x21·r2_0 +   x20·r2_1 + 5·x24·r2_2 + 5·x23·r2_3 + 5·x22·r2_4,
            //     x11·r3_0 +   x10·r3_1 + 5·x14·r3_2 + 5·x13·r3_3 + 5·x12·r3_4,
            //     x01·r4_0 +   x00·r4_1 + 5·x04·r4_2 + 5·x03·r4_3 + 5·x02·r4_4,
            // ]
            // v0 = [
            //     x30·r1_0 + 5·x34·r1_1 + 5·x33·r1_2 + 5·x32·r1_3 + 5·x31·r1_4,
            //     x20·r2_0 + 5·x24·r2_1 + 5·x23·r2_2 + 5·x22·r2_3 + 5·x21·r2_4,
            //     x10·r3_0 + 5·x14·r3_1 + 5·x13·r3_2 + 5·x12·r3_3 + 5·x11·r3_4,
            //     x00·r4_0 + 5·x04·r4_1 + 5·x03·r4_2 + 5·x02·r4_3 + 5·x01·r4_4,
            // ]
            Unreduced4x130 { v0, v1, v2, v3, v4 }
        }
    }
}

/// The unreduced output of an Aligned4x130 multiplication.
#[derive(Clone, Debug)]
pub(super) struct Unreduced4x130 {
    v0: __m256i,
    v1: __m256i,
    v2: __m256i,
    v3: __m256i,
    v4: __m256i,
}

impl fmt::Display for Unreduced4x130 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut v0 = [0u8; 32];
        let mut v1 = [0u8; 32];
        let mut v2 = [0u8; 32];
        let mut v3 = [0u8; 32];
        let mut v4 = [0u8; 32];
        unsafe {
            _mm256_storeu_si256(v0.as_mut_ptr() as *mut _, self.v0);
            _mm256_storeu_si256(v1.as_mut_ptr() as *mut _, self.v1);
            _mm256_storeu_si256(v2.as_mut_ptr() as *mut _, self.v2);
            _mm256_storeu_si256(v3.as_mut_ptr() as *mut _, self.v3);
            _mm256_storeu_si256(v4.as_mut_ptr() as *mut _, self.v4);
        }

        writeln!(f, "Unreduced4x130([")?;
        write!(f, "    ")?;
        write_130_wide(
            f,
            [
                u64::from_le_bytes(v0[0..8].try_into().unwrap()),
                u64::from_le_bytes(v1[0..8].try_into().unwrap()),
                u64::from_le_bytes(v2[0..8].try_into().unwrap()),
                u64::from_le_bytes(v3[0..8].try_into().unwrap()),
                u64::from_le_bytes(v4[0..8].try_into().unwrap()),
            ],
        )?;
        writeln!(f, ",")?;
        write!(f, "    ")?;
        write_130_wide(
            f,
            [
                u64::from_le_bytes(v0[8..16].try_into().unwrap()),
                u64::from_le_bytes(v1[8..16].try_into().unwrap()),
                u64::from_le_bytes(v2[8..16].try_into().unwrap()),
                u64::from_le_bytes(v3[8..16].try_into().unwrap()),
                u64::from_le_bytes(v4[8..16].try_into().unwrap()),
            ],
        )?;
        writeln!(f, ",")?;
        write!(f, "    ")?;
        write_130_wide(
            f,
            [
                u64::from_le_bytes(v0[16..24].try_into().unwrap()),
                u64::from_le_bytes(v1[16..24].try_into().unwrap()),
                u64::from_le_bytes(v2[16..24].try_into().unwrap()),
                u64::from_le_bytes(v3[16..24].try_into().unwrap()),
                u64::from_le_bytes(v4[16..24].try_into().unwrap()),
            ],
        )?;
        writeln!(f, ",")?;
        write!(f, "    ")?;
        write_130_wide(
            f,
            [
                u64::from_le_bytes(v0[24..32].try_into().unwrap()),
                u64::from_le_bytes(v1[24..32].try_into().unwrap()),
                u64::from_le_bytes(v2[24..32].try_into().unwrap()),
                u64::from_le_bytes(v3[24..32].try_into().unwrap()),
                u64::from_le_bytes(v4[24..32].try_into().unwrap()),
            ],
        )?;
        writeln!(f, ",")?;
        write!(f, "])")
    }
}

impl Unreduced4x130 {
    #[inline(always)]
    pub(super) fn reduce(self) -> Aligned4x130 {
        unsafe {
            // Starting with the following limb layout across 64-bit words:
            // x.v4 = [u34, u24, u14, u04]
            // x.v3 = [u33, u23, u13, u03]
            // x.v2 = [u32, u22, u12, u02]
            // x.v1 = [u31, u21, u11, u01]
            // x.v0 = [u30, u20, u10, u00]
            let x = self;

            // 26-bit mask on each 64-bit word.
            let mask_26 = _mm256_set1_epi64x(0x3ffffff);

            // Carry from x0 up into x1, returning their new values.
            let adc = |x1: __m256i, x0: __m256i| -> (__m256i, __m256i) {
                let y1 = _mm256_add_epi64(x1, _mm256_srli_epi64(x0, 26));
                let y0 = _mm256_and_si256(x0, mask_26);
                (y1, y0)
            };

            // Reduce modulo 2^130 - 5 from x4 down into x0, returning their new values.
            let red = |x4: __m256i, x0: __m256i| -> (__m256i, __m256i) {
                let y0 = _mm256_add_epi64(
                    x0,
                    _mm256_mul_epu32(_mm256_srli_epi64(x4, 26), _mm256_set1_epi64x(5)),
                );
                let y4 = _mm256_and_si256(x4, mask_26);
                (y4, y0)
            };

            // Reduce the four integers in parallel to below 2^130.
            let (red_1, red_0) = adc(x.v1, x.v0);
            let (red_4, red_3) = adc(x.v4, x.v3);
            let (red_2, red_1) = adc(x.v2, red_1);
            let (red_4, red_0) = red(red_4, red_0);
            let (red_3, red_2) = adc(red_3, red_2);
            let (red_1, red_0) = adc(red_1, red_0);
            let (red_4, red_3) = adc(red_4, red_3);

            // At this point, all limbs are contained within the lower 32 bits of each
            // 64-bit word. The upper limb of each integer (in red_4) is positioned
            // correctly for Aligned4x130, but the other limbs need to be blended
            // together:
            // - v0 contains limbs 0 and 2.
            // - v1 contains limbs 1 and 3.
            Aligned4x130 {
                v0: _mm256_blend_epi32(red_0, _mm256_slli_epi64(red_2, 32), 0b10101010),
                v1: _mm256_blend_epi32(red_1, _mm256_slli_epi64(red_3, 32), 0b10101010),
                v2: red_4,
            }
        }
    }

    /// Returns the unreduced sum of the four 130-bit integers.
    #[inline(always)]
    pub(super) fn sum(self) -> Unreduced130 {
        unsafe {
            // Starting with the following limb layout across 64-bit words:
            // x.v4 = [u34, u24, u14, u04]
            // x.v3 = [u33, u23, u13, u03]
            // x.v2 = [u32, u22, u12, u02]
            // x.v1 = [u31, u21, u11, u01]
            // x.v0 = [u30, u20, u10, u00]
            let x = self;

            // v0 = [
            //     u31 + u21,
            //     u30 + u20,
            //     u11 + u01,
            //     u10 + u00,
            // ]
            let v0 = _mm256_add_epi64(
                _mm256_unpackhi_epi64(x.v0, x.v1),
                _mm256_unpacklo_epi64(x.v0, x.v1),
            );

            // v1 = [
            //     u33 + u23,
            //     u32 + u22,
            //     u13 + u03,
            //     u12 + u02,
            // ]
            let v1 = _mm256_add_epi64(
                _mm256_unpackhi_epi64(x.v2, x.v3),
                _mm256_unpacklo_epi64(x.v2, x.v3),
            );

            // v0 = [
            //     u33 + u23 + u13 + u03,
            //     u32 + u22 + u12 + u02,
            //     u31 + u21 + u11 + u01,
            //     u30 + u20 + u10 + u00,
            // ]
            let v0 = _mm256_add_epi64(
                _mm256_inserti128_si256(v0, _mm256_castsi256_si128(v1), 1),
                _mm256_inserti128_si256(v1, _mm256_extractf128_si256(v0, 1), 0),
            );

            // v1 = [
            //     u34 + u14,
            //     u24 + u04,
            //     u14 + u34,
            //     u04 + u24,
            // ]
            let v1 = _mm256_add_epi64(x.v4, _mm256_permute4x64_epi64(x.v4, set02(1, 0, 3, 2)));

            // v1 = [
            //     u34 + u24 + u14 + u04,
            //     u24 + u24 + u04 + u04,
            //     u34 + u24 + u14 + u04,
            //     u34 + u24 + u14 + u04,
            // ]
            let v1 = _mm256_add_epi64(v1, _mm256_permute4x64_epi64(v1, set02(0, 0, 0, 1)));

            // The result:
            // v1 = [
            //     u34 + u24 + u14 + u04,
            //     u24 + u24 + u04 + u04,
            //     u34 + u24 + u14 + u04,
            //     u34 + u24 + u14 + u04,
            // ]
            // v0 = [
            //     u33 + u23 + u13 + u03,
            //     u32 + u22 + u12 + u02,
            //     u31 + u21 + u11 + u01,
            //     u30 + u20 + u10 + u00,
            // ]
            // This corresponds to:
            // v1 = [  _,   _,   _, t_4]
            // v0 = [t_3, t_2, t_1, t_0]
            Unreduced130 { v0, v1 }
        }
    }
}

#[derive(Clone, Copy, Debug)]
pub(super) struct AdditionKey(__m256i);

impl Add<Aligned130> for AdditionKey {
    type Output = IntegerTag;

    /// Computes x + k mod 2^128
    #[inline(always)]
    fn add(self, x: Aligned130) -> IntegerTag {
        unsafe {
            // Starting with the following limb layout:
            // x = [0,  _, _, x4, x3, x2, x1, x0]
            // k = [0, k7, 0, k6,  0, k5,  0, k4]
            let mut x = _mm256_and_si256(x.0, _mm256_set_epi32(0, 0, 0, -1, -1, -1, -1, -1));
            let k = self.0;

            /// Reduce to an integer below 2^130.
            unsafe fn propagate_carry(x: __m256i) -> __m256i {
                // t = [
                //     0,
                //     0,
                //     0,
                //     x3 >> 26,
                //     x2 >> 26,
                //     x1 >> 26,
                //     x0 >> 26,
                //     x4 >> 26,
                // ];
                let t = _mm256_permutevar8x32_epi32(
                    _mm256_srli_epi32(x, 26),
                    _mm256_set_epi32(7, 7, 7, 3, 2, 1, 0, 4),
                );

                // [
                //     0,
                //     0,
                //     0,
                //     x4 % 2^26,
                //     x3 % 2^26,
                //     x2 % 2^26,
                //     x1 % 2^26,
                //     x0 % 2^26,
                // ]
                // + t + [0, 0, 0, 0, 0, 0, 0, 4·(x4 >> 26)]
                // = [
                //     0,
                //     0,
                //     0,
                //     x4 % 2^26 +    x3 >> 26,
                //     x3 % 2^26 +    x2 >> 26,
                //     x2 % 2^26 +    x1 >> 26,
                //     x1 % 2^26 +    x0 >> 26,
                //     x0 % 2^26 + 5·(x4 >> 26),
                // ] => [0, 0, 0, x4, x3, x2, x1, x0]
                _mm256_add_epi32(
                    _mm256_add_epi32(
                        _mm256_and_si256(
                            x,
                            _mm256_set_epi32(
                                0, 0, 0, 0x3ffffff, 0x3ffffff, 0x3ffffff, 0x3ffffff, 0x3ffffff,
                            ),
                        ),
                        t,
                    ),
                    _mm256_permutevar8x32_epi32(
                        _mm256_slli_epi32(t, 2),
                        _mm256_set_epi32(7, 7, 7, 7, 7, 7, 7, 0),
                    ),
                )
            }

            // Reduce modulus 2^130-5:
            // - Reduce to an integer below 2^130:
            // TODO: Is it more efficient to unpack the limbs for this?
            for _ in 0..5 {
                x = propagate_carry(x);
            }

            // - Compute x + -p by adding 5 and carrying up to the top limb:
            // g = [0, 0, 0, g4, g3, g2, g1, g0]
            let mut g = _mm256_add_epi32(x, _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 5));
            // TODO: Is it more efficient to unpack the limbs for this?
            for _ in 0..4 {
                g = propagate_carry(g);
            }
            let g = _mm256_sub_epi32(g, _mm256_set_epi32(0, 0, 0, 1 << 26, 0, 0, 0, 0));

            // - Check whether g4 overflowed:
            let mask = _mm256_permutevar8x32_epi32(
                _mm256_sub_epi32(_mm256_srli_epi32(g, 32 - 1), _mm256_set1_epi32(1)),
                _mm256_set1_epi32(4),
            );

            // - Select x if g4 overflowed, else g:
            let x = _mm256_or_si256(
                _mm256_and_si256(x, _mm256_xor_si256(mask, _mm256_set1_epi32(-1))),
                _mm256_and_si256(g, mask),
            );

            // Align back to 32 bits per digit. We drop the top two bits of the top limb,
            // because we only care about the lower 128 bits from here onward, and don't
            // need to track overflow or reduce.
            // [
            //     0,
            //     0,
            //     0,
            //     0,
            //     x4[24..0] || x3[26..18],
            //     x3[18..0] || x2[26..12],
            //     x2[12..0] || x1[26.. 6],
            //     x1[ 6..0] || x0[26.. 0],
            // ]
            let x = _mm256_or_si256(
                _mm256_srlv_epi32(x, _mm256_set_epi32(32, 32, 32, 32, 18, 12, 6, 0)),
                _mm256_permutevar8x32_epi32(
                    _mm256_sllv_epi32(x, _mm256_set_epi32(32, 32, 32, 8, 14, 20, 26, 32)),
                    _mm256_set_epi32(7, 7, 7, 7, 4, 3, 2, 1),
                ),
            );

            // Add key
            // [
            //     (x4[24..0] || x3[26..18]) + k7,
            //     (x3[18..0] || x2[26..12]) + k6,
            //     (x2[12..0] || x1[26.. 6]) + k5,
            //     (x1[ 6..0] || x0[26.. 0]) + k4,
            // ]
            let mut x = _mm256_add_epi64(
                _mm256_permutevar8x32_epi32(x, _mm256_set_epi32(7, 3, 7, 2, 7, 1, 7, 0)),
                k,
            );

            // Ensure that all carries are handled
            unsafe fn propagate_carry_32(x: __m256i) -> __m256i {
                // [
                //     (l4 % 2^32) + (l3 >> 32),
                //     (l3 % 2^32) + (l2 >> 32),
                //     (l2 % 2^32) + (l1 >> 32),
                //     (l1 % 2^32),
                // ]
                _mm256_add_epi64(
                    _mm256_and_si256(x, _mm256_set_epi32(0, -1, 0, -1, 0, -1, 0, -1)),
                    _mm256_permute4x64_epi64(
                        _mm256_and_si256(
                            _mm256_srli_epi64(x, 32),
                            _mm256_set_epi64x(0, -1, -1, -1),
                        ),
                        set02(2, 1, 0, 3),
                    ),
                )
            }
            for _ in 0..3 {
                x = propagate_carry_32(x);
            }

            // Now that all limbs are at most 32 bits, realign from 64- to 32-bit limbs.
            // [
            //     0,
            //     0,
            //     0,
            //     0,
            //     ((x4[24..0] || x3[26..18]) + k7) % 2^32 + ((x3[18..0] || x2[26..12]) + k6) >> 32,
            //     ((x3[18..0] || x2[26..12]) + k6) % 2^32 + ((x2[12..0] || x1[26.. 6]) + k5) >> 32,
            //     ((x2[12..0] || x1[26.. 6]) + k5) % 2^32 + ((x1[ 6..0] || x0[26.. 0]) + k4) >> 32,
            //     ((x1[ 6..0] || x0[26.. 0]) + k4) % 2^32,
            // ]
            let x = _mm256_permutevar8x32_epi32(x, _mm256_set_epi32(7, 7, 7, 7, 6, 4, 2, 0));

            // Reduce modulus 2^128
            IntegerTag(_mm256_castsi256_si128(x))
        }
    }
}

pub(super) struct IntegerTag(__m128i);

impl From<AdditionKey> for IntegerTag {
    fn from(k: AdditionKey) -> Self {
        unsafe {
            // There was no polynomial to add.
            IntegerTag(_mm256_castsi256_si128(_mm256_permutevar8x32_epi32(
                k.0,
                _mm256_set_epi32(0, 0, 0, 0, 6, 4, 2, 0),
            )))
        }
    }
}

impl IntegerTag {
    pub(super) fn write(self, tag: &mut [u8]) {
        unsafe {
            _mm_storeu_si128(tag.as_mut_ptr() as *mut _, self.0);
        }
    }
}
