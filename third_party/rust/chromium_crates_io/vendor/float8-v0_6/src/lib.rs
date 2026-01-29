//! Eight bit floating point types in Rust.
//!
//! This crate provides 2 types:
//! - [`F8E4M3`]: Sign + 4-bit exponent + 3-bit mantissa. More precise but less dynamic range.
//! - [`F8E5M2`]: Sign + 5-bit exponent + 2-bit mantissa. Less precise but more dynamic range (same exponent as [`struct@f16`]).
//!
//! Generally, this crate is modelled after the [`half`] crate, so it can be
//! used alongside and with minimal code changes.
//!
//! # Serialization
//!
//! When the `serde` feature is enabled, [`F8E4M3`] and [`F8E5M2`] will be serialized as a newtype of
//! [`u16`] by default. In binary formats this is ideal, as it will generally use just two bytes for
//! storage. For string formats like JSON, however, this isn't as useful, and due to design
//! limitations of serde, it's not possible for the default `Serialize` implementation to support
//! different serialization for different formats.
//!
//! It is up to the container type of the floats to control how it is serialized. This can
//! easily be controlled when using the derive macros using `#[serde(serialize_with="")]`
//! attributes. For both [`F8E4M3`] and [`F8E5M2`], a `serialize_as_f32` and `serialize_as_string` are
//! provided for use with this attribute.
//!
//! Deserialization of both float types supports deserializing from the default serialization,
//! strings, and `f32`/`f64` values, so no additional work is required.
//!
//! # Cargo Features
//!
//! This crate supports a number of optional cargo features. None of these features are enabled by
//! default, even `std`.
//!
//! - **`std`** — Enable features that depend on the Rust [`std`] library.
//!
//! - **`serde`** — Adds support for the [`serde`] crate by implementing [`Serialize`] and
//!   [`Deserialize`] traits for both [`F8E4M3`] and [`F8E5M2`].
//!
//! - **`num-traits`** — Adds support for the [`num-traits`] crate by implementing [`ToPrimitive`],
//!   [`FromPrimitive`], [`AsPrimitive`], [`Num`], [`Float`], [`FloatCore`], and [`Bounded`] traits
//!   for both [`F8E4M3`] and [`F8E5M2`].
//!
//! - **`bytemuck`** — Adds support for the [`bytemuck`] crate by implementing [`Zeroable`] and
//!   [`Pod`] traits for both [`F8E4M3`] and [`F8E5M2`].
//!
//! - **`zerocopy`** — Adds support for the [`zerocopy`] crate by implementing [`AsBytes`] and
//!   [`FromBytes`] traits for both [`F8E4M3`] and [`F8E5M2`].
//!
//! - **`rand_distr`** — Adds support for the [`rand_distr`] crate by implementing [`Distribution`]
//!   and other traits for both [`F8E4M3`] and [`F8E5M2`].
//!
//! - **`rkyv`** -- Enable zero-copy deserialization with [`rkyv`] crate.
//!
//! [`alloc`]: https://doc.rust-lang.org/alloc/
//! [`std`]: https://doc.rust-lang.org/std/
//! [`binary16`]: https://en.wikipedia.org/wiki/Half-precision_floating-point_format
//! [`bfloat16`]: https://en.wikipedia.org/wiki/Bfloat16_floating-point_format
//! [`serde`]: https://crates.io/crates/serde
//! [`bytemuck`]: https://crates.io/crates/bytemuck
//! [`num-traits`]: https://crates.io/crates/num-traits
//! [`zerocopy`]: https://crates.io/crates/zerocopy
//! [`rand_distr`]: https://crates.io/crates/rand_distr
//! [`rkyv`]: https://crates.io/crates/rkyv
//! [`FromBytes`]: https://docs.rs/zerocopy/latest/zerocopy/trait.FromBytes.html
//! [`Distribution`]: https://docs.rs/rand/latest/rand/distributions/trait.Distribution.html
//! [`AsBytes`]: https://docs.rs/zerocopy/0.6.6/zerocopy/trait.AsBytes.html
//! [`Pod`]: https://docs.rs/bytemuck/latest/bytemuck/trait.Pod.html
//! [`Zeroable`]: https://docs.rs/bytemuck/latest/bytemuck/trait.Zeroable.html
//! [`Bounded`]: https://docs.rs/num-traits/latest/num_traits/bounds/trait.Bounded.html
//! [`FloatCore`]: https://docs.rs/num-traits/latest/num_traits/float/trait.FloatCore.html
//! [`Float`]: https://docs.rs/num-traits/latest/num_traits/float/trait.Float.html
//! [`Num`]: https://docs.rs/num-traits/latest/num_traits/trait.Num.html
//! [`AsPrimitive`]: https://docs.rs/num-traits/latest/num_traits/cast/trait.AsPrimitive.html
//! [`ToPrimitive`]: https://docs.rs/num-traits/latest/num_traits/cast/trait.ToPrimitive.html
//! [`FromPrimitive`]: https://docs.rs/num-traits/latest/num_traits/cast/trait.FromPrimitive.html
//! [`Deserialize`]: https://docs.rs/serde/latest/serde/trait.Deserialize.html
//! [`Serialize`]: https://docs.rs/serde/latest/serde/trait.Serialize.html

#![no_std]

#[cfg(feature = "num-traits")]
mod num_traits;
#[cfg(feature = "rand_distr")]
mod rand_distr;

use core::{
    cmp::Ordering,
    f64,
    fmt::{self, Debug, Display, LowerExp, LowerHex, UpperExp, UpperHex},
    mem,
    num::{FpCategory, ParseFloatError},
    ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Rem, RemAssign, Sub, SubAssign},
    str::FromStr,
};
use half::f16;

#[cfg(feature = "bytemuck")]
use bytemuck::{Pod, Zeroable};
#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};
#[cfg(feature = "zerocopy")]
use zerocopy::{AsBytes, FromBytes};

#[derive(Clone, Copy, PartialEq)]
enum Kind {
    E4M3,
    E5M2,
}

#[allow(dead_code)]
#[derive(Clone, Copy, PartialEq, Default)]
/// Saturation type. If `NoSat`, allow NaN and inf.
enum SaturationType {
    NoSat,
    #[default]
    SatFinite,
}

// https://gitlab.com/nvidia/headers/cuda-individual/cudart/-/blob/main/cuda_fp8.hpp?ref_type=heads#L97
const fn convert_to_fp8(x: f64, saturate: SaturationType, fp8_interpretation: Kind) -> u8 {
    // TODO: use x.to_bits() with MSRV 1.83
    #[allow(unknown_lints, unnecessary_transmutes)]
    let xbits: u64 = unsafe { mem::transmute::<f64, u64>(x) };

    let (
        fp8_maxnorm,
        fp8_mantissa_mask,
        fp8_exp_bias,
        fp8_significand_bits,
        fp8_mindenorm_o2,
        fp8_overflow_threshold,
        fp8_minnorm,
    ) = match fp8_interpretation {
        Kind::E4M3 => (
            0x7E_u8,
            0x7_u8,
            7_u16,
            4_u64,
            0x3F50000000000000_u64,
            0x407D000000000000_u64,
            0x3F90000000000000_u64,
        ),
        Kind::E5M2 => (
            0x7B_u8,
            0x3_u8,
            15_u16,
            3_u64,
            0x3EE0000000000000_u64,
            0x40EE000000000000_u64 - 1,
            0x3F10000000000000_u64,
        ),
    };

    const DP_INF_BITS: u64 = 0x7FF0000000000000;
    let fp8_dp_half_ulp: u64 = 1 << (53 - fp8_significand_bits - 1);
    let sign: u8 = ((xbits >> 63) << 7) as u8;
    let exp: u8 = ((((xbits >> 52) as u16) & 0x7FF)
        .wrapping_sub(1023)
        .wrapping_add(fp8_exp_bias)) as u8;
    let mantissa: u8 = ((xbits >> (53 - fp8_significand_bits)) & (fp8_mantissa_mask as u64)) as u8;
    let absx: u64 = xbits & 0x7FFFFFFFFFFFFFFF;

    let res = if absx <= fp8_mindenorm_o2 {
        // Zero or underflow
        0
    } else if absx > DP_INF_BITS {
        // Preserve NaNs
        match fp8_interpretation {
            Kind::E4M3 => 0x7F,
            Kind::E5M2 => 0x7E | mantissa,
        }
    } else if absx > fp8_overflow_threshold {
        // Saturate
        match saturate {
            SaturationType::SatFinite => fp8_maxnorm,
            SaturationType::NoSat => match fp8_interpretation {
                Kind::E4M3 => 0x7F, // NaN
                Kind::E5M2 => 0x7C, // Inf in E5M2
            },
        }
    } else if absx >= fp8_minnorm {
        // Round, normal range
        let mut res = (exp << (fp8_significand_bits - 1)) | mantissa;

        // Round off bits and round-to-nearest-even adjustment
        let round = xbits & ((fp8_dp_half_ulp << 1) - 1);
        if (round > fp8_dp_half_ulp) || ((round == fp8_dp_half_ulp) && (mantissa & 1 != 0)) {
            res = res.wrapping_add(1);
        }
        res
    } else {
        // Denormal numbers
        let shift = 1_u8.wrapping_sub(exp);
        let mantissa = mantissa | (1 << (fp8_significand_bits - 1));
        let mut res = mantissa >> shift;

        // Round off bits and round-to-nearest-even adjustment
        let round = (xbits | (1 << (53 - 1))) & ((fp8_dp_half_ulp << (shift as u64 + 1)) - 1);
        if (round > (fp8_dp_half_ulp << shift as u64))
            || ((round == (fp8_dp_half_ulp << shift as u64)) && (res & 1 != 0))
        {
            res = res.wrapping_add(1);
        }
        res
    };

    res | sign
}

// https://gitlab.com/nvidia/headers/cuda-individual/cudart/-/blob/main/cuda_fp8.hpp?ref_type=heads#L463
const fn convert_fp8_to_fp16(x: u8, fp8_interpretation: Kind) -> u16 {
    let mut ur = (x as u16) << 8;

    match fp8_interpretation {
        Kind::E5M2 => {
            if (ur & 0x7FFF) > 0x7C00 {
                // If NaN, return canonical NaN
                ur = 0x7FFF;
            }
        }
        Kind::E4M3 => {
            let sign = ur & 0x8000;
            let mut exponent = ((ur & 0x7800) >> 1).wrapping_add(0x2000);
            let mut mantissa = (ur & 0x0700) >> 1;
            let absx = 0x7F & x;

            if absx == 0x7F {
                // FP16 canonical NaN, discard sign
                ur = 0x7FFF;
            } else if exponent == 0x2000 {
                // Zero or denormal
                if mantissa != 0 {
                    // Normalize
                    mantissa <<= 1;
                    while (mantissa & 0x0400) == 0 {
                        mantissa <<= 1;
                        exponent = exponent.wrapping_sub(0x0400);
                    }
                    // Discard implicit leading bit
                    mantissa &= 0x03FF;
                } else {
                    // Zero
                    exponent = 0;
                }
                ur = sign | exponent | mantissa;
            } else {
                ur = sign | exponent | mantissa;
            }
        }
    };

    ur
}

#[derive(Clone, Copy, Default)]
#[cfg_attr(feature = "serde", derive(Serialize))]
#[cfg_attr(
    feature = "rkyv",
    derive(rkyv::Archive, rkyv::Serialize, rkyv::Deserialize)
)]
#[cfg_attr(feature = "rkyv", archive(resolver = "F8E4M3Resolver"))]
#[cfg_attr(feature = "bytemuck", derive(Zeroable, Pod))]
#[cfg_attr(feature = "zerocopy", derive(AsBytes, FromBytes))]
#[repr(transparent)]
/// Eight bit floating point type with 4-bit exponent and 3-bit mantissa.
pub struct F8E4M3(u8);

impl F8E4M3 {
    const INTERPRETATION: Kind = Kind::E4M3;

    /// Construct an 8-bit floating point value from the raw bits.
    pub const fn from_bits(bits: u8) -> Self {
        Self(bits)
    }

    /// Return the raw bits.
    pub const fn to_bits(&self) -> u8 {
        self.0
    }

    /// Convert a [`prim@f64`] type into [`F8E4M3`].
    ///
    /// This operation is lossy.
    ///
    /// - If the 64-bit value is to large to fit in 8-bits, ±∞ will result.
    /// - NaN values are preserved.
    /// - 64-bit subnormal values are too tiny to be represented in 8-bits and result in ±0.
    /// - Exponents that underflow the minimum 8-bit exponent will result in 8-bit subnormals or ±0.
    /// - All other values are truncated and rounded to the nearest representable  8-bit value.
    pub const fn from_f64(x: f64) -> Self {
        Self(convert_to_fp8(
            x,
            SaturationType::SatFinite,
            Self::INTERPRETATION,
        ))
    }

    /// Convert a [`f32`] type into [`F8E4M3`].
    ///
    /// This operation is lossy.
    ///
    /// - If the 32-bit value is to large to fit in 8-bits, ±∞ will result.
    /// - NaN values are preserved.
    /// - 32-bit subnormal values are too tiny to be represented in 8-bits and result in ±0.
    /// - Exponents that underflow the minimum 8-bit exponent will result in 8-bit subnormals or ±0.
    /// - All other values are truncated and rounded to the nearest representable  8-bit value.
    pub const fn from_f32(x: f32) -> Self {
        Self::from_f64(x as f64)
    }

    /// Convert this [`F8E4M3`] type into a [`struct@f16`] type.
    ///
    /// This operation may be lossy.
    ///
    /// - NaN and zero values are preserved.
    /// - Subnormal values are normalized.
    /// - Otherwise, the values are mapped to the appropriate 16-bit value.
    pub const fn to_f16(&self) -> f16 {
        f16::from_bits(convert_fp8_to_fp16(self.0, Self::INTERPRETATION))
    }

    /// Convert this [`F8E4M3`] type into a [`f32`] type.
    ///
    /// This operation may be lossy.
    ///
    /// - NaN and zero values are preserved.
    /// - Subnormal values are normalized.
    /// - Otherwise, the values are mapped to the appropriate 16-bit value.
    pub const fn to_f32(&self) -> f32 {
        self.to_f16().to_f32_const()
    }

    /// Convert this [`F8E4M3`] type into a [`prim@f64`] type.
    ///
    /// This operation may be lossy.
    ///
    /// - NaN and zero values are preserved.
    /// - Subnormal values are normalized.
    /// - Otherwise, the values are mapped to the appropriate 16-bit value.
    pub const fn to_f64(&self) -> f64 {
        self.to_f16().to_f64_const()
    }

    /// Returns the ordering between `self` and `other`.
    ///
    /// - negative quiet NaN
    /// - negative signaling NaN
    /// - negative infinity
    /// - negative numbers
    /// - negative subnormal numbers
    /// - negative zero
    /// - positive zero
    /// - positive subnormal numbers
    /// - positive numbers
    /// - positive infinity
    /// - positive signaling NaN
    /// - positive quiet NaN.
    ///
    /// The ordering established by this function does not always agree with the
    /// [`PartialOrd`] and [`PartialEq`] implementations. For example,
    /// they consider negative and positive zero equal, while `total_cmp`
    /// doesn't.
    ///
    /// # Example
    /// ```
    /// # use float8::F8E4M3;
    ///
    /// let mut v: Vec<F8E4M3> = vec![];
    /// v.push(F8E4M3::ONE);
    /// v.push(F8E4M3::INFINITY);
    /// v.push(F8E4M3::NEG_INFINITY);
    /// v.push(F8E4M3::NAN);
    /// v.push(F8E4M3::MAX_SUBNORMAL);
    /// v.push(-F8E4M3::MAX_SUBNORMAL);
    /// v.push(F8E4M3::ZERO);
    /// v.push(F8E4M3::NEG_ZERO);
    /// v.push(F8E4M3::NEG_ONE);
    /// v.push(F8E4M3::MIN_POSITIVE);
    ///
    /// v.sort_by(|a, b| a.total_cmp(&b));
    ///
    /// assert!(v
    ///     .into_iter()
    ///     .zip(
    ///         [
    ///             F8E4M3::NEG_INFINITY,
    ///             F8E4M3::NEG_ONE,
    ///             -F8E4M3::MAX_SUBNORMAL,
    ///             F8E4M3::NEG_ZERO,
    ///             F8E4M3::ZERO,
    ///             F8E4M3::MAX_SUBNORMAL,
    ///             F8E4M3::MIN_POSITIVE,
    ///             F8E4M3::ONE,
    ///             F8E4M3::INFINITY,
    ///             F8E4M3::NAN
    ///         ]
    ///         .iter()
    ///     )
    ///     .all(|(a, b)| a.to_bits() == b.to_bits()));
    /// ```
    pub fn total_cmp(&self, other: &Self) -> Ordering {
        let mut left = self.to_bits() as i8;
        let mut right = other.to_bits() as i8;
        left ^= (((left >> 7) as u8) >> 1) as i8;
        right ^= (((right >> 7) as u8) >> 1) as i8;
        left.cmp(&right)
    }

    /// Returns `true` if and only if `self` has a positive sign, including +0.0, NaNs with a
    /// positive sign bit and +∞.
    pub const fn is_sign_positive(&self) -> bool {
        self.0 & 0x80u8 == 0
    }

    /// Returns `true` if and only if `self` has a negative sign, including −0.0, NaNs with a
    /// negative sign bit and −∞.
    pub const fn is_sign_negative(&self) -> bool {
        self.0 & 0x80u8 != 0
    }

    /// Returns `true` if this value is NaN and `false` otherwise.
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let nan = F8E4M3::NAN;
    /// let f = F8E4M3::from_f32(7.0_f32);
    ///
    /// assert!(nan.is_nan());
    /// assert!(!f.is_nan());
    /// ```
    pub const fn is_nan(&self) -> bool {
        self.0 == 0x7Fu8 || self.0 == 0xFFu8
    }

    /// Returns `true` if this value is ±∞ and `false` otherwise.
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let f = F8E4M3::from_f32(7.0f32);
    /// let inf = F8E4M3::INFINITY;
    /// let neg_inf = F8E4M3::NEG_INFINITY;
    /// let nan = F8E4M3::NAN;
    ///
    /// assert!(!f.is_infinite());
    /// assert!(!nan.is_infinite());
    ///
    /// assert!(inf.is_infinite());
    /// assert!(neg_inf.is_infinite());
    /// ```
    pub const fn is_infinite(&self) -> bool {
        self.0 & 0x7Fu8 == 0x7Eu8
    }

    /// Returns true if this number is neither infinite nor NaN.
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let f = F8E4M3::from_f32(7.0f32);
    /// let inf = F8E4M3::INFINITY;
    /// let neg_inf = F8E4M3::NEG_INFINITY;
    /// let nan = F8E4M3::NAN;
    ///
    /// assert!(f.is_finite());
    ///
    /// assert!(!nan.is_finite());
    /// assert!(!inf.is_finite());
    /// assert!(!neg_inf.is_finite());
    /// ```
    pub const fn is_finite(&self) -> bool {
        !(self.is_infinite() || self.is_nan())
    }

    /// Returns `true` if the number is neither zero, infinite, subnormal, or `NaN` and `false` otherwise.
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let min = F8E4M3::MIN_POSITIVE;
    /// let max = F8E4M3::MAX;
    /// let lower_than_min = F8E4M3::from_f32(1.0e-10_f32);
    /// let zero = F8E4M3::from_f32(0.0_f32);
    ///
    /// assert!(min.is_normal());
    /// assert!(max.is_normal());
    ///
    /// assert!(!zero.is_normal());
    /// assert!(!F8E4M3::NAN.is_normal());
    /// assert!(!F8E4M3::INFINITY.is_normal());
    /// // Values between `0` and `min` are Subnormal.
    /// assert!(!lower_than_min.is_normal());
    /// ```
    pub const fn is_normal(&self) -> bool {
        #[allow(clippy::unusual_byte_groupings)]
        let exp = self.0 & 0b0_1111_000;
        exp != 0 && self.is_finite()
    }

    /// Returns the minimum of the two numbers.
    ///
    /// If one of the arguments is NaN, then the other argument is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// # use float8::*;
    /// let x = F8E4M3::from_f32(1.0);
    /// let y = F8E4M3::from_f32(2.0);
    ///
    /// assert_eq!(x.min(y), x);
    /// ```
    pub fn min(self, other: Self) -> Self {
        if other < self && !other.is_nan() {
            other
        } else {
            self
        }
    }

    /// Returns the minimum of the two numbers.
    ///
    /// If one of the arguments is NaN, then the other argument is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// # use float8::*;
    /// let x = F8E4M3::from_f32(1.0);
    /// let y = F8E4M3::from_f32(2.0);
    ///
    /// assert_eq!(x.min(y), x);
    /// ```
    pub fn max(self, other: Self) -> Self {
        if other > self && !other.is_nan() {
            other
        } else {
            self
        }
    }

    /// Restrict a value to a certain interval unless it is NaN.
    ///
    /// Returns `max` if `self` is greater than `max`, and `min` if `self` is less than `min`.
    /// Otherwise this returns `self`.
    ///
    /// Note that this function returns NaN if the initial value was NaN as well.
    ///
    /// # Panics
    /// Panics if `min > max`, `min` is NaN, or `max` is NaN.
    ///
    /// # Examples
    ///
    /// ```
    /// # use float8::*;
    /// assert!(F8E4M3::from_f32(-3.0).clamp(F8E4M3::from_f32(-2.0), F8E4M3::from_f32(1.0)) == F8E4M3::from_f32(-2.0));
    /// assert!(F8E4M3::from_f32(0.0).clamp(F8E4M3::from_f32(-2.0), F8E4M3::from_f32(1.0)) == F8E4M3::from_f32(0.0));
    /// assert!(F8E4M3::from_f32(2.0).clamp(F8E4M3::from_f32(-2.0), F8E4M3::from_f32(1.0)) == F8E4M3::from_f32(1.0));
    /// assert!(F8E4M3::NAN.clamp(F8E4M3::from_f32(-2.0), F8E4M3::from_f32(1.0)).is_nan());
    /// ```
    pub fn clamp(self, min: Self, max: Self) -> Self {
        assert!(min <= max);
        let mut x = self;
        if x < min {
            x = min;
        }
        if x > max {
            x = max;
        }
        x
    }

    /// Returns a number composed of the magnitude of `self` and the sign of `sign`.
    ///
    /// Equal to `self` if the sign of `self` and `sign` are the same, otherwise equal to `-self`.
    /// If `self` is NaN, then NaN with the sign of `sign` is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// # use float8::*;
    /// let f = F8E4M3::from_f32(3.5);
    ///
    /// assert_eq!(f.copysign(F8E4M3::from_f32(0.42)), F8E4M3::from_f32(3.5));
    /// assert_eq!(f.copysign(F8E4M3::from_f32(-0.42)), F8E4M3::from_f32(-3.5));
    /// assert_eq!((-f).copysign(F8E4M3::from_f32(0.42)), F8E4M3::from_f32(3.5));
    /// assert_eq!((-f).copysign(F8E4M3::from_f32(-0.42)), F8E4M3::from_f32(-3.5));
    ///
    /// assert!(F8E4M3::NAN.copysign(F8E4M3::from_f32(1.0)).is_nan());
    /// ```
    pub const fn copysign(self, sign: Self) -> Self {
        Self((sign.0 & 0x80u8) | (self.0 & 0x7Fu8))
    }

    /// Returns a number that represents the sign of `self`.
    ///
    /// * `1.0` if the number is positive, `+0.0` or [`INFINITY`][Self::INFINITY]
    /// * `-1.0` if the number is negative, `-0.0` or [`NEG_INFINITY`][Self::NEG_INFINITY]
    /// * [`NAN`][Self::NAN] if the number is `NaN`
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let f = F8E4M3::from_f32(3.5_f32);
    ///
    /// assert_eq!(f.signum(), F8E4M3::from_f32(1.0));
    /// assert_eq!(F8E4M3::NEG_INFINITY.signum(), F8E4M3::from_f32(-1.0));
    ///
    /// assert!(F8E4M3::NAN.signum().is_nan());
    /// ```
    pub const fn signum(self) -> Self {
        if self.is_nan() {
            self
        } else if self.0 & 0x80u8 != 0 {
            Self::NEG_ONE
        } else {
            Self::ONE
        }
    }

    /// Returns the floating point category of the number.
    ///
    /// If only one property is going to be tested, it is generally faster to use the specific
    /// predicate instead.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use std::num::FpCategory;
    /// # use float8::*;
    ///
    /// let num = F8E4M3::from_f32(12.4_f32);
    /// let inf = F8E4M3::INFINITY;
    ///
    /// assert_eq!(num.classify(), FpCategory::Normal);
    /// assert_eq!(inf.classify(), FpCategory::Infinite);
    /// ```
    pub const fn classify(&self) -> FpCategory {
        if self.is_infinite() {
            FpCategory::Infinite
        } else if !self.is_normal() {
            FpCategory::Subnormal
        } else if self.is_nan() {
            FpCategory::Nan
        } else if self.0 & 0x7Fu8 == 0 {
            FpCategory::Zero
        } else {
            FpCategory::Normal
        }
    }
}

#[cfg(feature = "serde")]
struct VisitorF8E4M3;

#[cfg(feature = "serde")]
impl<'de> Deserialize<'de> for F8E4M3 {
    fn deserialize<D>(deserializer: D) -> Result<F8E4M3, D::Error>
    where
        D: serde::de::Deserializer<'de>,
    {
        deserializer.deserialize_newtype_struct("f8e4m3", VisitorF8E4M3)
    }
}

#[cfg(feature = "serde")]
impl<'de> serde::de::Visitor<'de> for VisitorF8E4M3 {
    type Value = F8E4M3;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "tuple struct f8e4m3")
    }

    fn visit_newtype_struct<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        Ok(F8E4M3(<u8 as Deserialize>::deserialize(deserializer)?))
    }

    fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        v.parse().map_err(|_| {
            serde::de::Error::invalid_value(serde::de::Unexpected::Str(v), &"a float string")
        })
    }

    fn visit_f32<E>(self, v: f32) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        Ok(F8E4M3::from_f32(v))
    }

    fn visit_f64<E>(self, v: f64) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        Ok(F8E4M3::from_f64(v))
    }
}

#[derive(Clone, Copy, Default)]
#[cfg_attr(feature = "serde", derive(Serialize))]
#[cfg_attr(
    feature = "rkyv",
    derive(rkyv::Archive, rkyv::Serialize, rkyv::Deserialize)
)]
#[cfg_attr(feature = "rkyv", archive(resolver = "F8E5M2Resolver"))]
#[cfg_attr(feature = "bytemuck", derive(Zeroable, Pod))]
#[cfg_attr(feature = "zerocopy", derive(AsBytes, FromBytes))]
#[repr(transparent)]
/// Eight bit floating point type with 5-bit exponent and 2-bit mantissa.
pub struct F8E5M2(u8);

impl F8E5M2 {
    const INTERPRETATION: Kind = Kind::E5M2;

    /// Construct an 8-bit floating point value from the raw bits.
    pub const fn from_bits(bits: u8) -> Self {
        Self(bits)
    }

    /// Return the raw bits.
    pub const fn to_bits(&self) -> u8 {
        self.0
    }

    /// Convert a [`prim@f64`] type into [`F8E5M2`].
    ///
    /// This operation is lossy.
    ///
    /// - If the 64-bit value is to large to fit in 8-bits, ±∞ will result.
    /// - NaN values are preserved.
    /// - 64-bit subnormal values are too tiny to be represented in 8-bits and result in ±0.
    /// - Exponents that underflow the minimum 8-bit exponent will result in 8-bit subnormals or ±0.
    /// - All other values are truncated and rounded to the nearest representable  8-bit value.
    pub const fn from_f64(x: f64) -> Self {
        Self(convert_to_fp8(
            x,
            SaturationType::SatFinite,
            Self::INTERPRETATION,
        ))
    }

    /// Convert a [`f32`] type into [`F8E5M2`].
    ///
    /// This operation is lossy.
    ///
    /// - If the 32-bit value is to large to fit in 8-bits, ±∞ will result.
    /// - NaN values are preserved.
    /// - 32-bit subnormal values are too tiny to be represented in 8-bits and result in ±0.
    /// - Exponents that underflow the minimum 8-bit exponent will result in 8-bit subnormals or ±0.
    /// - All other values are truncated and rounded to the nearest representable  8-bit value.
    pub const fn from_f32(x: f32) -> Self {
        Self::from_f64(x as f64)
    }

    /// Convert this [`F8E5M2`] type into a [`struct@f16`] type.
    ///
    /// This operation may be lossy.
    ///
    /// - NaN and zero values are preserved.
    /// - Subnormal values are normalized.
    /// - Otherwise, the values are mapped to the appropriate 16-bit value.
    pub const fn to_f16(&self) -> f16 {
        f16::from_bits(convert_fp8_to_fp16(self.0, Self::INTERPRETATION))
    }

    /// Convert this [`F8E5M2`] type into a [`prim@f32`] type.
    ///
    /// This operation may be lossy.
    ///
    /// - NaN and zero values are preserved.
    /// - Subnormal values are normalized.
    /// - Otherwise, the values are mapped to the appropriate 16-bit value.
    pub const fn to_f32(&self) -> f32 {
        self.to_f16().to_f32_const()
    }

    /// Convert this [`F8E5M2`] type into a [`prim@f64`] type.
    ///
    /// This operation may be lossy.
    ///
    /// - NaN and zero values are preserved.
    /// - Subnormal values are normalized.
    /// - Otherwise, the values are mapped to the appropriate 16-bit value.
    pub const fn to_f64(&self) -> f64 {
        self.to_f16().to_f64_const()
    }

    /// Returns the ordering between `self` and `other`.
    ///
    /// - negative quiet NaN
    /// - negative signaling NaN
    /// - negative infinity
    /// - negative numbers
    /// - negative subnormal numbers
    /// - negative zero
    /// - positive zero
    /// - positive subnormal numbers
    /// - positive numbers
    /// - positive infinity
    /// - positive signaling NaN
    /// - positive quiet NaN.
    ///
    /// The ordering established by this function does not always agree with the
    /// [`PartialOrd`] and [`PartialEq`] implementations. For example,
    /// they consider negative and positive zero equal, while `total_cmp`
    /// doesn't.
    ///
    /// # Example
    /// ```
    /// # use float8::F8E5M2;
    ///
    /// let mut v: Vec<F8E5M2> = vec![];
    /// v.push(F8E5M2::ONE);
    /// v.push(F8E5M2::INFINITY);
    /// v.push(F8E5M2::NEG_INFINITY);
    /// v.push(F8E5M2::NAN);
    /// v.push(F8E5M2::MAX_SUBNORMAL);
    /// v.push(-F8E5M2::MAX_SUBNORMAL);
    /// v.push(F8E5M2::ZERO);
    /// v.push(F8E5M2::NEG_ZERO);
    /// v.push(F8E5M2::NEG_ONE);
    /// v.push(F8E5M2::MIN_POSITIVE);
    ///
    /// v.sort_by(|a, b| a.total_cmp(&b));
    ///
    /// assert!(v
    ///     .into_iter()
    ///     .zip(
    ///         [
    ///             F8E5M2::NEG_INFINITY,
    ///             F8E5M2::NEG_ONE,
    ///             -F8E5M2::MAX_SUBNORMAL,
    ///             F8E5M2::NEG_ZERO,
    ///             F8E5M2::ZERO,
    ///             F8E5M2::MAX_SUBNORMAL,
    ///             F8E5M2::MIN_POSITIVE,
    ///             F8E5M2::ONE,
    ///             F8E5M2::INFINITY,
    ///             F8E5M2::NAN
    ///         ]
    ///         .iter()
    ///     )
    ///     .all(|(a, b)| a.to_bits() == b.to_bits()));
    /// ```
    pub fn total_cmp(&self, other: &Self) -> Ordering {
        let mut left = self.to_bits() as i8;
        let mut right = other.to_bits() as i8;
        left ^= (((left >> 7) as u8) >> 1) as i8;
        right ^= (((right >> 7) as u8) >> 1) as i8;
        left.cmp(&right)
    }

    /// Returns `true` if and only if `self` has a positive sign, including +0.0, NaNs with a
    /// positive sign bit and +∞.
    pub const fn is_sign_positive(&self) -> bool {
        self.0 & 0x80u8 == 0
    }

    /// Returns `true` if and only if `self` has a negative sign, including −0.0, NaNs with a
    /// negative sign bit and −∞.
    pub const fn is_sign_negative(&self) -> bool {
        self.0 & 0x80u8 != 0
    }

    /// Returns `true` if this value is NaN and `false` otherwise.
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let nan = F8E5M2::NAN;
    /// let f = F8E5M2::from_f32(7.0_f32);
    ///
    /// assert!(nan.is_nan());
    /// assert!(!f.is_nan());
    /// ```
    pub const fn is_nan(&self) -> bool {
        self.0 == 0x7Eu8 || self.0 == 0xFEu8
    }

    /// Returns `true` if this value is ±∞ and `false` otherwise.
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let f = F8E5M2::from_f32(7.0f32);
    /// let inf = F8E5M2::INFINITY;
    /// let neg_inf = F8E5M2::NEG_INFINITY;
    /// let nan = F8E5M2::NAN;
    ///
    /// assert!(!f.is_infinite());
    /// assert!(!nan.is_infinite());
    ///
    /// assert!(inf.is_infinite());
    /// assert!(neg_inf.is_infinite());
    /// ```
    pub const fn is_infinite(&self) -> bool {
        self.0 & 0x7Fu8 == 0x7Bu8
    }

    /// Returns true if this number is neither infinite nor NaN.
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let f = F8E5M2::from_f32(7.0f32);
    /// let inf = F8E5M2::INFINITY;
    /// let neg_inf = F8E5M2::NEG_INFINITY;
    /// let nan = F8E5M2::NAN;
    ///
    /// assert!(f.is_finite());
    ///
    /// assert!(!nan.is_finite());
    /// assert!(!inf.is_finite());
    /// assert!(!neg_inf.is_finite());
    /// ```
    pub const fn is_finite(&self) -> bool {
        !(self.is_infinite() || self.is_nan())
    }

    /// Returns `true` if the number is neither zero, infinite, subnormal, or `NaN` and `false` otherwise.
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let min = F8E5M2::MIN_POSITIVE;
    /// let max = F8E5M2::MAX;
    /// let lower_than_min = F8E5M2::from_f32(1.0e-10_f32);
    /// let zero = F8E5M2::from_f32(0.0_f32);
    ///
    /// assert!(min.is_normal());
    /// assert!(max.is_normal());
    ///
    /// assert!(!zero.is_normal());
    /// assert!(!F8E5M2::NAN.is_normal());
    /// assert!(!F8E5M2::INFINITY.is_normal());
    /// // Values between `0` and `min` are Subnormal.
    /// assert!(!lower_than_min.is_normal());
    /// ```
    pub const fn is_normal(&self) -> bool {
        #[allow(clippy::unusual_byte_groupings)]
        let exp = self.0 & 0b0_11111_00;
        exp != 0 && self.is_finite()
    }

    /// Returns the minimum of the two numbers.
    ///
    /// If one of the arguments is NaN, then the other argument is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// # use float8::*;
    /// let x = F8E5M2::from_f32(1.0);
    /// let y = F8E5M2::from_f32(2.0);
    ///
    /// assert_eq!(x.min(y), x);
    /// ```
    pub fn min(self, other: Self) -> Self {
        if other < self && !other.is_nan() {
            other
        } else {
            self
        }
    }

    /// Returns the minimum of the two numbers.
    ///
    /// If one of the arguments is NaN, then the other argument is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// # use float8::*;
    /// let x = F8E5M2::from_f32(1.0);
    /// let y = F8E5M2::from_f32(2.0);
    ///
    /// assert_eq!(x.min(y), x);
    /// ```
    pub fn max(self, other: Self) -> Self {
        if other > self && !other.is_nan() {
            other
        } else {
            self
        }
    }

    /// Restrict a value to a certain interval unless it is NaN.
    ///
    /// Returns `max` if `self` is greater than `max`, and `min` if `self` is less than `min`.
    /// Otherwise this returns `self`.
    ///
    /// Note that this function returns NaN if the initial value was NaN as well.
    ///
    /// # Panics
    /// Panics if `min > max`, `min` is NaN, or `max` is NaN.
    ///
    /// # Examples
    ///
    /// ```
    /// # use float8::*;
    /// assert!(F8E5M2::from_f32(-3.0).clamp(F8E5M2::from_f32(-2.0), F8E5M2::from_f32(1.0)) == F8E5M2::from_f32(-2.0));
    /// assert!(F8E5M2::from_f32(0.0).clamp(F8E5M2::from_f32(-2.0), F8E5M2::from_f32(1.0)) == F8E5M2::from_f32(0.0));
    /// assert!(F8E5M2::from_f32(2.0).clamp(F8E5M2::from_f32(-2.0), F8E5M2::from_f32(1.0)) == F8E5M2::from_f32(1.0));
    /// assert!(F8E5M2::NAN.clamp(F8E5M2::from_f32(-2.0), F8E5M2::from_f32(1.0)).is_nan());
    /// ```
    pub fn clamp(self, min: Self, max: Self) -> Self {
        assert!(min <= max);
        let mut x = self;
        if x < min {
            x = min;
        }
        if x > max {
            x = max;
        }
        x
    }

    /// Returns a number composed of the magnitude of `self` and the sign of `sign`.
    ///
    /// Equal to `self` if the sign of `self` and `sign` are the same, otherwise equal to `-self`.
    /// If `self` is NaN, then NaN with the sign of `sign` is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// # use float8::*;
    /// let f = F8E5M2::from_f32(3.5);
    ///
    /// assert_eq!(f.copysign(F8E5M2::from_f32(0.42)), F8E5M2::from_f32(3.5));
    /// assert_eq!(f.copysign(F8E5M2::from_f32(-0.42)), F8E5M2::from_f32(-3.5));
    /// assert_eq!((-f).copysign(F8E5M2::from_f32(0.42)), F8E5M2::from_f32(3.5));
    /// assert_eq!((-f).copysign(F8E5M2::from_f32(-0.42)), F8E5M2::from_f32(-3.5));
    ///
    /// assert!(F8E5M2::NAN.copysign(F8E5M2::from_f32(1.0)).is_nan());
    /// ```
    pub const fn copysign(self, sign: Self) -> Self {
        Self((sign.0 & 0x80u8) | (self.0 & 0x7Fu8))
    }

    /// Returns a number that represents the sign of `self`.
    ///
    /// * `1.0` if the number is positive, `+0.0` or [`INFINITY`][Self::INFINITY]
    /// * `-1.0` if the number is negative, `-0.0` or [`NEG_INFINITY`][Self::NEG_INFINITY]
    /// * [`NAN`][Self::NAN] if the number is `NaN`
    ///
    /// # Examples
    ///
    /// ```rust
    /// # use float8::*;
    ///
    /// let f = F8E5M2::from_f32(3.5_f32);
    ///
    /// assert_eq!(f.signum(), F8E5M2::from_f32(1.0));
    /// assert_eq!(F8E5M2::NEG_INFINITY.signum(), F8E5M2::from_f32(-1.0));
    ///
    /// assert!(F8E5M2::NAN.signum().is_nan());
    /// ```
    pub const fn signum(self) -> Self {
        if self.is_nan() {
            self
        } else if self.0 & 0x80u8 != 0 {
            Self::NEG_ONE
        } else {
            Self::ONE
        }
    }

    /// Returns the floating point category of the number.
    ///
    /// If only one property is going to be tested, it is generally faster to use the specific
    /// predicate instead.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use std::num::FpCategory;
    /// # use float8::*;
    ///
    /// let num = F8E5M2::from_f32(12.4_f32);
    /// let inf = F8E5M2::INFINITY;
    ///
    /// assert_eq!(num.classify(), FpCategory::Normal);
    /// assert_eq!(inf.classify(), FpCategory::Infinite);
    /// ```
    pub const fn classify(&self) -> FpCategory {
        if self.is_infinite() {
            FpCategory::Infinite
        } else if !self.is_normal() {
            FpCategory::Subnormal
        } else if self.is_nan() {
            FpCategory::Nan
        } else if self.0 & 0x7Fu8 == 0 {
            FpCategory::Zero
        } else {
            FpCategory::Normal
        }
    }
}

#[cfg(feature = "serde")]
struct VisitorF8E5M2;

#[cfg(feature = "serde")]
impl<'de> Deserialize<'de> for F8E5M2 {
    fn deserialize<D>(deserializer: D) -> Result<F8E5M2, D::Error>
    where
        D: serde::de::Deserializer<'de>,
    {
        deserializer.deserialize_newtype_struct("f8e5m2", VisitorF8E5M2)
    }
}

#[cfg(feature = "serde")]
impl<'de> serde::de::Visitor<'de> for VisitorF8E5M2 {
    type Value = F8E5M2;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "tuple struct f8e5m2")
    }

    fn visit_newtype_struct<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        Ok(F8E5M2(<u8 as Deserialize>::deserialize(deserializer)?))
    }

    fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        v.parse().map_err(|_| {
            serde::de::Error::invalid_value(serde::de::Unexpected::Str(v), &"a float string")
        })
    }

    fn visit_f32<E>(self, v: f32) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        Ok(F8E5M2::from_f32(v))
    }

    fn visit_f64<E>(self, v: f64) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        Ok(F8E5M2::from_f64(v))
    }
}

macro_rules! comparison {
    ($t:ident) => {
        impl PartialEq for $t {
            fn eq(&self, other: &Self) -> bool {
                if self.is_nan() || other.is_nan() {
                    false
                } else {
                    (self.0 == other.0) || ((self.0 | other.0) & 0x7Fu8 == 0)
                }
            }
        }

        impl PartialOrd for $t {
            fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
                if self.is_nan() || other.is_nan() {
                    None
                } else {
                    let neg = self.0 & 0x80u8 != 0;
                    let other_neg = other.0 & 0x80u8 != 0;
                    match (neg, other_neg) {
                        (false, false) => Some(self.0.cmp(&other.0)),
                        (false, true) => {
                            if (self.0 | other.0) & 0x7Fu8 == 0 {
                                Some(Ordering::Equal)
                            } else {
                                Some(Ordering::Greater)
                            }
                        }
                        (true, false) => {
                            if (self.0 | other.0) & 0x7Fu8 == 0 {
                                Some(Ordering::Equal)
                            } else {
                                Some(Ordering::Less)
                            }
                        }
                        (true, true) => Some(other.0.cmp(&self.0)),
                    }
                }
            }

            fn lt(&self, other: &Self) -> bool {
                if self.is_nan() || other.is_nan() {
                    false
                } else {
                    let neg = self.0 & 0x80u8 != 0;
                    let other_neg = other.0 & 0x80u8 != 0;
                    match (neg, other_neg) {
                        (false, false) => self.0 < other.0,
                        (false, true) => false,
                        (true, false) => (self.0 | other.0) & 0x7Fu8 != 0,
                        (true, true) => self.0 > other.0,
                    }
                }
            }

            fn le(&self, other: &Self) -> bool {
                if self.is_nan() || other.is_nan() {
                    false
                } else {
                    let neg = self.0 & 0x80u8 != 0;
                    let other_neg = other.0 & 0x80u8 != 0;
                    match (neg, other_neg) {
                        (false, false) => self.0 <= other.0,
                        (false, true) => (self.0 | other.0) & 0x7Fu8 == 0,
                        (true, false) => true,
                        (true, true) => self.0 >= other.0,
                    }
                }
            }

            fn gt(&self, other: &Self) -> bool {
                if self.is_nan() || other.is_nan() {
                    false
                } else {
                    let neg = self.0 & 0x80u8 != 0;
                    let other_neg = other.0 & 0x80u8 != 0;
                    match (neg, other_neg) {
                        (false, false) => self.0 > other.0,
                        (false, true) => (self.0 | other.0) & 0x7Fu8 != 0,
                        (true, false) => false,
                        (true, true) => self.0 < other.0,
                    }
                }
            }

            fn ge(&self, other: &Self) -> bool {
                if self.is_nan() || other.is_nan() {
                    false
                } else {
                    let neg = self.0 & 0x80u8 != 0;
                    let other_neg = other.0 & 0x80u8 != 0;
                    match (neg, other_neg) {
                        (false, false) => self.0 >= other.0,
                        (false, true) => true,
                        (true, false) => (self.0 | other.0) & 0x7Fu8 == 0,
                        (true, true) => self.0 <= other.0,
                    }
                }
            }
        }
    };
}

comparison!(F8E4M3);
comparison!(F8E5M2);

macro_rules! constants {
    ($t:ident) => {
        impl $t {
            /// π
            pub const PI: Self = Self::from_f64(f64::consts::PI);

            /// The full circle constant (τ)
            ///
            /// Equal to 2π.
            pub const TAU: Self = Self::from_f64(f64::consts::TAU);

            /// π/2
            pub const FRAC_PI_2: Self = Self::from_f64(f64::consts::FRAC_PI_2);

            /// π/3
            pub const FRAC_PI_3: Self = Self::from_f64(f64::consts::FRAC_PI_3);

            /// π/4
            pub const FRAC_PI_4: Self = Self::from_f64(f64::consts::FRAC_PI_4);

            /// π/6
            pub const FRAC_PI_6: Self = Self::from_f64(f64::consts::FRAC_PI_6);

            /// π/8
            pub const FRAC_PI_8: Self = Self::from_f64(f64::consts::FRAC_PI_8);

            /// 1/π
            pub const FRAC_1_PI: Self = Self::from_f64(f64::consts::FRAC_1_PI);

            /// 2/π
            pub const FRAC_2_PI: Self = Self::from_f64(f64::consts::FRAC_2_PI);

            /// 2/sqrt(π)
            pub const FRAC_2_SQRT_PI: Self = Self::from_f64(f64::consts::FRAC_2_SQRT_PI);

            /// sqrt(2)
            pub const SQRT_2: Self = Self::from_f64(f64::consts::SQRT_2);

            /// 1/sqrt(2)
            pub const FRAC_1_SQRT_2: Self = Self::from_f64(f64::consts::FRAC_1_SQRT_2);

            /// Euler's number (e)
            pub const E: Self = Self::from_f64(f64::consts::E);

            /// log<sub>2</sub>(10)
            pub const LOG2_10: Self = Self::from_f64(f64::consts::LOG2_10);

            /// log<sub>2</sub>(e)
            pub const LOG2_E: Self = Self::from_f64(f64::consts::LOG2_E);

            /// log<sub>10</sub>(2)
            pub const LOG10_2: Self = Self::from_f64(f64::consts::LOG10_2);

            /// log<sub>10</sub>(e)
            pub const LOG10_E: Self = Self::from_f64(f64::consts::LOG10_E);

            /// ln(2)
            pub const LN_2: Self = Self::from_f64(f64::consts::LN_2);

            /// ln(10)
            pub const LN_10: Self = Self::from_f64(f64::consts::LN_10);
        }
    };
}

constants!(F8E4M3);
constants!(F8E5M2);

#[allow(clippy::unusual_byte_groupings)]
impl F8E4M3 {
    /// Number of mantissa digits
    pub const MANTISSA_DIGITS: u32 = 3;
    /// Maximum possible value
    pub const MAX: Self = Self::from_bits(0x7E - 1);
    /// Minimum possible value
    pub const MIN: Self = Self::from_bits(0xFE - 1);
    /// Positive infinity ∞
    pub const INFINITY: Self = Self::from_bits(0x7E);
    /// Negative infinity -∞
    pub const NEG_INFINITY: Self = Self::from_bits(0xFE);
    /// Smallest possible normal value
    pub const MIN_POSITIVE: Self = Self::from_bits(0b0_0001_000);
    /// Smallest possible subnormal value
    pub const MIN_POSITIVE_SUBNORMAL: Self = Self::from_bits(0b0_0000_001);
    /// Smallest possible subnormal value
    pub const MAX_SUBNORMAL: Self = Self::from_bits(0b0_0000_111);
    /// This is the difference between 1.0 and the next largest representable number.
    pub const EPSILON: Self = Self::from_bits(0b0_0100_000);
    /// NaN value
    pub const NAN: Self = Self::from_bits(0x7F);
    /// 1
    pub const ONE: Self = Self::from_bits(0b0_0111_000);
    /// 0
    pub const ZERO: Self = Self::from_bits(0b0_0000_000);
    /// -1
    pub const NEG_ONE: Self = Self::from_bits(0b1_0111_000);
    /// -0
    pub const NEG_ZERO: Self = Self::from_bits(0b1_0000_000);
    /// One greater than the minimum possible normal power of 2 exponent
    pub const MIN_EXP: i32 = -5;
    /// Minimum possible normal power of 10 exponent
    pub const MIN_10_EXP: i32 = -1;
    /// Maximum possible normal power of 2 exponent
    pub const MAX_EXP: i32 = 7;
    /// Maximum possible normal power of 10 exponent
    pub const MAX_10_EXP: i32 = 2;
    /// Approximate number of significant digits in base 10
    pub const DIGITS: u32 = 0;
}

#[allow(clippy::unusual_byte_groupings)]
impl F8E5M2 {
    /// Number of mantissa digits
    pub const MANTISSA_DIGITS: u32 = 2;
    /// Maximum possible value
    pub const MAX: Self = Self::from_bits(0x7B - 1);
    /// Minimum possible value
    pub const MIN: Self = Self::from_bits(0xFB - 1);
    /// Positive infinity ∞
    pub const INFINITY: Self = Self::from_bits(0x7B);
    /// Negative infinity -∞
    pub const NEG_INFINITY: Self = Self::from_bits(0xFB);
    /// Smallest possible normal value
    pub const MIN_POSITIVE: Self = Self::from_bits(0b0_00001_00);
    /// Smallest possible subnormal value
    pub const MIN_POSITIVE_SUBNORMAL: Self = Self::from_bits(0b0_00000_01);
    /// Smallest possible subnormal value
    pub const MAX_SUBNORMAL: Self = Self::from_bits(0b0_00000_11);
    /// This is the difference between 1.0 and the next largest representable number.
    pub const EPSILON: Self = Self::from_bits(0b0_01101_00);
    /// NaN value
    pub const NAN: Self = Self::from_bits(0x7E);
    /// 1
    pub const ONE: Self = Self::from_bits(0b0_01111_00);
    /// 0
    pub const ZERO: Self = Self::from_bits(0b0_00000_00);
    /// -1
    pub const NEG_ONE: Self = Self::from_bits(0b1_01111_00);
    /// -0
    pub const NEG_ZERO: Self = Self::from_bits(0b1_00000_00);
    /// One greater than the minimum possible normal power of 2 exponent
    pub const MIN_EXP: i32 = -13;
    /// Minimum possible normal power of 10 exponent
    pub const MIN_10_EXP: i32 = -4;
    /// Maximum possible normal power of 2 exponent
    pub const MAX_EXP: i32 = 15;
    /// Maximum possible normal power of 10 exponent
    pub const MAX_10_EXP: i32 = 4;
    /// Approximate number of significant digits in base 10
    pub const DIGITS: u32 = 0;
}

macro_rules! io {
    ($t:ident) => {
        impl Display for $t {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                Display::fmt(&self.to_f32(), f)
            }
        }
        impl Debug for $t {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                Debug::fmt(&self.to_f32(), f)
            }
        }
        impl FromStr for $t {
            type Err = ParseFloatError;
            fn from_str(src: &str) -> Result<$t, ParseFloatError> {
                f32::from_str(src).map($t::from_f32)
            }
        }
        impl From<f16> for $t {
            fn from(x: f16) -> $t {
                Self::from_f32(x.to_f32())
            }
        }
        impl From<f32> for $t {
            fn from(x: f32) -> $t {
                Self::from_f32(x)
            }
        }
        impl From<f64> for $t {
            fn from(x: f64) -> $t {
                Self::from_f64(x)
            }
        }
        impl LowerExp for $t {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                write!(f, "{:e}", self.to_f32())
            }
        }
        impl LowerHex for $t {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                write!(f, "{:x}", self.0)
            }
        }
        impl UpperExp for $t {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                write!(f, "{:E}", self.to_f32())
            }
        }
        impl UpperHex for $t {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                write!(f, "{:X}", self.0)
            }
        }
    };
}

io!(F8E4M3);
io!(F8E5M2);

macro_rules! binary {
    ($trait:ident, $fn_name:ident, $t:ident, $op:tt) => {
        impl $trait for $t {
            type Output = Self;

            fn $fn_name(self, rhs: Self) -> Self::Output {
                Self::from_f32(self.to_f32() $op rhs.to_f32())
            }
        }
    };
}

macro_rules! assign_binary {
    ($trait:ident, $fn_name:ident, $t:ident, $op:tt) => {
        impl $trait for $t {
            fn $fn_name(&mut self, rhs: Self) {
                *self = Self::from_f32(self.to_f32() $op rhs.to_f32())
            }
        }
    };
}

macro_rules! unary {
    ($trait:ident, $fn_name:ident, $t:ident, $op:tt) => {
        impl $trait for $t {
            type Output = Self;

            fn $fn_name(self) -> Self::Output {
                Self::from_f32($op self.to_f32())
            }
        }
    };
}

binary!(Add, add, F8E4M3, +);
binary!(Sub, sub, F8E4M3, -);
binary!(Mul, mul, F8E4M3, *);
binary!(Div, div, F8E4M3, /);
binary!(Rem, rem, F8E4M3, %);
assign_binary!(AddAssign, add_assign, F8E4M3, +);
assign_binary!(SubAssign, sub_assign, F8E4M3, -);
assign_binary!(MulAssign, mul_assign, F8E4M3, *);
assign_binary!(DivAssign, div_assign, F8E4M3, /);
assign_binary!(RemAssign, rem_assign, F8E4M3, %);
unary!(Neg, neg, F8E4M3, -);

binary!(Add, add, F8E5M2, +);
binary!(Sub, sub, F8E5M2, -);
binary!(Mul, mul, F8E5M2, *);
binary!(Div, div, F8E5M2, /);
binary!(Rem, rem, F8E5M2, %);
assign_binary!(AddAssign, add_assign, F8E5M2, +);
assign_binary!(SubAssign, sub_assign, F8E5M2, -);
assign_binary!(MulAssign, mul_assign, F8E5M2, *);
assign_binary!(DivAssign, div_assign, F8E5M2, /);
assign_binary!(RemAssign, rem_assign, F8E5M2, %);
unary!(Neg, neg, F8E5M2, -);

macro_rules! from_t {
    ($t:ident) => {
        impl From<$t> for f64 {
            fn from(value: $t) -> Self {
                value.to_f64()
            }
        }

        impl From<$t> for f32 {
            fn from(value: $t) -> Self {
                value.to_f32()
            }
        }

        impl From<$t> for f16 {
            fn from(value: $t) -> Self {
                value.to_f16()
            }
        }
    };
}

from_t!(F8E4M3);
from_t!(F8E5M2);

#[cfg(feature = "cuda")]
unsafe impl cudarc::driver::DeviceRepr for F8E4M3 {}
#[cfg(feature = "cuda")]
unsafe impl cudarc::driver::ValidAsZeroBits for F8E4M3 {}

#[cfg(feature = "cuda")]
unsafe impl cudarc::driver::safe::DeviceRepr for F8E5M2 {}
#[cfg(feature = "cuda")]
unsafe impl cudarc::driver::ValidAsZeroBits for F8E5M2 {}
