// Copyright 2018 Developers of the Rand project.
// Copyright 2017 Paul Dicker.
// Copyright 2014-2017 Melissa O'Neill and PCG Project contributors
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! PCG random number generators

// This is the default multiplier used by PCG for 128-bit state.
const MULTIPLIER: u128 = 0x2360_ED05_1FC6_5DA4_4385_DF64_9FCC_F645;

use core::fmt;
use rand_core::{Infallible, SeedableRng, TryRng, utils};
#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// A PCG random number generator (XSL RR 128/64 (LCG) variant).
///
/// Permuted Congruential Generator with 128-bit state, internal Linear
/// Congruential Generator, and 64-bit output via "xorshift low (bits),
/// random rotation" output function.
///
/// This is a 128-bit LCG with explicitly chosen stream with the PCG-XSL-RR
/// output function. This combination is the standard `pcg64`.
///
/// Despite the name, this implementation uses 32 bytes (256 bit) space
/// comprising 128 bits of state and 128 bits stream selector. These are both
/// set by `SeedableRng`, using a 256-bit seed.
///
/// Note that two generators with different stream parameters may be closely
/// correlated.
#[derive(Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Lcg128Xsl64 {
    state: u128,
    increment: u128,
}

/// [`Lcg128Xsl64`] is also officially known as `pcg64`.
pub type Pcg64 = Lcg128Xsl64;

impl Lcg128Xsl64 {
    /// Multi-step advance functions (jump-ahead, jump-back)
    ///
    /// The method used here is based on Brown, "Random Number Generation
    /// with Arbitrary Stride,", Transactions of the American Nuclear
    /// Society (Nov. 1994).  The algorithm is very similar to fast
    /// exponentiation.
    ///
    /// Even though delta is an unsigned integer, we can pass a
    /// signed integer to go backwards, it just goes "the long way round".
    ///
    /// Using this function is equivalent to calling `next_64()` `delta`
    /// number of times.
    ///
    /// # Warning
    ///
    /// Do not advance by a large power of two, such as `1 << 127` (half of the
    /// period), to split a single stream into several. The state is an LCG
    /// modulo `2^128`, whose lowest `e` bits repeat every `2^e` steps, so
    /// advancing by `2^e` leaves them unchanged. After `advance(1 << 127)`, the
    /// two states differ only in their top bit at every step, and each output
    /// is the other with its 32-bit halves swapped and one bit flipped.
    #[inline]
    pub fn advance(&mut self, delta: u128) {
        let mut acc_mult: u128 = 1;
        let mut acc_plus: u128 = 0;
        let mut cur_mult = MULTIPLIER;
        let mut cur_plus = self.increment;
        let mut mdelta = delta;

        while mdelta > 0 {
            if (mdelta & 1) != 0 {
                acc_mult = acc_mult.wrapping_mul(cur_mult);
                acc_plus = acc_plus.wrapping_mul(cur_mult).wrapping_add(cur_plus);
            }
            cur_plus = cur_mult.wrapping_add(1).wrapping_mul(cur_plus);
            cur_mult = cur_mult.wrapping_mul(cur_mult);
            mdelta /= 2;
        }
        self.state = acc_mult.wrapping_mul(self.state).wrapping_add(acc_plus);
    }

    /// Construct an instance compatible with PCG seed and stream.
    ///
    /// Note that the highest bit of the `stream` parameter is discarded
    /// to simplify upholding internal invariants.
    ///
    /// Note that two generators with different stream parameters may be closely
    /// correlated.
    ///
    /// PCG specifies the following default values for both parameters:
    ///
    /// - `state = 0xcafef00dd15ea5e5`
    /// - `stream = 0xa02bdbf7bb3c0a7ac28fa16a64abf96`
    pub fn new(state: u128, stream: u128) -> Self {
        // The increment must be odd, hence we discard one bit:
        let increment = (stream << 1) | 1;
        Lcg128Xsl64::from_state_incr(state, increment)
    }

    #[inline]
    fn from_state_incr(state: u128, increment: u128) -> Self {
        let mut pcg = Lcg128Xsl64 { state, increment };
        // Move away from initial value:
        pcg.state = pcg.state.wrapping_add(pcg.increment);
        pcg.step();
        pcg
    }

    #[inline]
    fn step(&mut self) {
        // prepare the LCG for the next round
        self.state = self
            .state
            .wrapping_mul(MULTIPLIER)
            .wrapping_add(self.increment);
    }
}

crate::macros::impl_state_stream!(Lcg128Xsl64, u128);

// Custom Debug implementation that does not expose the internal state
impl fmt::Debug for Lcg128Xsl64 {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Lcg128Xsl64 {{}}")
    }
}

impl SeedableRng for Lcg128Xsl64 {
    type Seed = [u8; 32];

    /// We use a single 255-bit seed to initialise the state and select a stream.
    /// One `seed` bit (lowest bit of `seed[8]`) is ignored.
    fn from_seed(seed: Self::Seed) -> Self {
        let seed_u64: [u64; 4] = utils::read_words(&seed);
        let state = u128::from(seed_u64[0]) | (u128::from(seed_u64[1]) << 64);
        let incr = u128::from(seed_u64[2]) | (u128::from(seed_u64[3]) << 64);

        // The increment must be odd, hence we discard one bit:
        Lcg128Xsl64::from_state_incr(state, incr | 1)
    }
}

impl TryRng for Lcg128Xsl64 {
    type Error = Infallible;

    #[inline]
    fn try_next_u32(&mut self) -> Result<u32, Infallible> {
        self.try_next_u64().map(|result| result as u32)
    }

    #[inline]
    fn try_next_u64(&mut self) -> Result<u64, Infallible> {
        self.step();
        Ok(output_xsl_rr(self.state))
    }

    #[inline]
    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), Infallible> {
        utils::fill_bytes_via_next_word(dest, || self.try_next_u64())
    }
}

/// A PCG random number generator (XSL 128/64 (MCG) variant).
///
/// Permuted Congruential Generator with 128-bit state, internal Multiplicative
/// Congruential Generator, and 64-bit output via "xorshift low (bits),
/// random rotation" output function.
///
/// This is a 128-bit MCG with the PCG-XSL-RR output function, also known as
/// `pcg64_fast`.
/// Note that compared to the standard `pcg64` (128-bit LCG with PCG-XSL-RR
/// output function), this RNG is faster, also has a long cycle, and still has
/// good performance on statistical tests.
#[derive(Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Mcg128Xsl64 {
    state: u128,
}

/// A friendly name for [`Mcg128Xsl64`] (also known as `pcg64_fast`).
pub type Pcg64Mcg = Mcg128Xsl64;

impl Mcg128Xsl64 {
    /// Multi-step advance functions (jump-ahead, jump-back)
    ///
    /// The method used here is based on Brown, "Random Number Generation
    /// with Arbitrary Stride,", Transactions of the American Nuclear
    /// Society (Nov. 1994).  The algorithm is very similar to fast
    /// exponentiation.
    ///
    /// Even though delta is an unsigned integer, we can pass a
    /// signed integer to go backwards, it just goes "the long way round".
    ///
    /// Using this function is equivalent to calling `next_64()` `delta`
    /// number of times.
    ///
    /// # Warning
    ///
    /// Do not advance by a large power of two, such as `1 << 125` (half of the
    /// period), to split a single stream into several. The state is an MCG
    /// modulo `2^128`, whose lowest `e` bits repeat every `2^e` steps, so
    /// advancing by `2^e` leaves them unchanged. After `advance(1 << 125)`, the
    /// two states differ only in their top bit at every step, and each output
    /// is the other with its 32-bit halves swapped and one bit flipped.
    #[inline]
    pub fn advance(&mut self, delta: u128) {
        let mut acc_mult: u128 = 1;
        let mut acc_plus: u128 = 0;
        let mut cur_mult = MULTIPLIER;
        let mut cur_plus: u128 = 0;
        let mut mdelta = delta;

        while mdelta > 0 {
            if (mdelta & 1) != 0 {
                acc_mult = acc_mult.wrapping_mul(cur_mult);
                acc_plus = acc_plus.wrapping_mul(cur_mult).wrapping_add(cur_plus);
            }
            cur_plus = cur_mult.wrapping_add(1).wrapping_mul(cur_plus);
            cur_mult = cur_mult.wrapping_mul(cur_mult);
            mdelta /= 2;
        }
        self.state = acc_mult.wrapping_mul(self.state).wrapping_add(acc_plus);
    }

    /// Construct an instance compatible with PCG seed.
    ///
    /// Note that PCG specifies a default value for the parameter:
    ///
    /// - `state = 0xcafef00dd15ea5e5`
    pub fn new(state: u128) -> Self {
        // Force low bit to 1, as in C version (C++ uses `state | 3` instead).
        Mcg128Xsl64 { state: state | 1 }
    }

    /// Current state of the generator
    ///
    /// Will always be odd.
    pub fn state(&self) -> u128 {
        self.state
    }
}

// Custom Debug implementation that does not expose the internal state
impl fmt::Debug for Mcg128Xsl64 {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Mcg128Xsl64 {{}}")
    }
}

/// We use a single 126-bit seed to initialise the state and select a stream.
/// Two `seed` bits (lowest order of last byte) are ignored.
impl SeedableRng for Mcg128Xsl64 {
    type Seed = [u8; 16];

    fn from_seed(seed: Self::Seed) -> Self {
        // Read as if a little-endian u128 value:
        let seed_u64: [u64; 2] = utils::read_words(&seed);
        let state = u128::from(seed_u64[0]) | (u128::from(seed_u64[1]) << 64);
        Mcg128Xsl64::new(state)
    }
}

impl TryRng for Mcg128Xsl64 {
    type Error = Infallible;

    #[inline]
    fn try_next_u32(&mut self) -> Result<u32, Infallible> {
        self.try_next_u64().map(|result| result as u32)
    }

    #[inline]
    fn try_next_u64(&mut self) -> Result<u64, Infallible> {
        self.state = self.state.wrapping_mul(MULTIPLIER);
        Ok(output_xsl_rr(self.state))
    }

    #[inline]
    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), Infallible> {
        utils::fill_bytes_via_next_word(dest, || self.try_next_u64())
    }
}

#[inline(always)]
fn output_xsl_rr(state: u128) -> u64 {
    // Output function XSL RR ("xorshift low (bits), random rotation")
    // Constants are for 128-bit state, 64-bit output
    const XSHIFT: u32 = 64; // (128 - 64 + 64) / 2
    const ROTATE: u32 = 122; // 128 - 6

    let rot = (state >> ROTATE) as u32;
    let xsl = ((state >> XSHIFT) as u64) ^ (state as u64);
    xsl.rotate_right(rot)
}
