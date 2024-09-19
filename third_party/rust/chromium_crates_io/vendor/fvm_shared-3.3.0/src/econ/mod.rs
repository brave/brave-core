// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use std::cmp::Ordering;
use std::fmt;
use std::iter::Sum;
use std::ops::{Add, AddAssign, Mul, MulAssign, Neg, Sub, SubAssign};

use num_bigint::BigInt;
use num_integer::Integer;
use num_traits::{Signed, Zero};
use serde::{Deserialize, Serialize, Serializer};

use crate::bigint::bigint_ser;

/// A quantity of native tokens.
/// A token amount is an integer, but has a human interpretation as a value with
/// 18 decimal places.
/// This is a new-type in order to prevent accidental conversion from other BigInts.
/// From/Into BigInt is missing by design.
#[derive(Clone, PartialEq, Eq, Hash)]
pub struct TokenAmount {
    atto: BigInt,
}

// This type doesn't implement all the numeric traits (Num, Signed, etc),
// opting for a minimal useful set. Others can be added if needed.
impl TokenAmount {
    /// The logical number of decimal places of a token unit.
    pub const DECIMALS: usize = 18;

    /// The logical precision of a token unit.
    pub const PRECISION: u64 = 10u64.pow(Self::DECIMALS as u32);

    /// Creates a token amount from a quantity of indivisible units  (10^-18 whole units).
    pub fn from_atto(atto: impl Into<BigInt>) -> Self {
        Self { atto: atto.into() }
    }

    /// Creates a token amount from nanoFIL.
    pub fn from_nano(nano: impl Into<BigInt>) -> Self {
        const NANO_PRECISION: u64 = 10u64.pow((TokenAmount::DECIMALS as u32) - 9);
        Self {
            atto: nano.into() * NANO_PRECISION,
        }
    }

    /// Creates a token amount from a quantity of whole units (10^18 indivisible units).
    pub fn from_whole(tokens: impl Into<BigInt>) -> Self {
        Self::from_atto(tokens.into() * Self::PRECISION)
    }

    /// Returns the quantity of indivisible units.
    pub fn atto(&self) -> &BigInt {
        &self.atto
    }

    pub fn is_zero(&self) -> bool {
        self.atto.is_zero()
    }

    pub fn is_positive(&self) -> bool {
        self.atto.is_positive()
    }

    pub fn is_negative(&self) -> bool {
        self.atto.is_negative()
    }
}

impl Zero for TokenAmount {
    #[inline]
    fn zero() -> Self {
        Self {
            atto: BigInt::zero(),
        }
    }

    #[inline]
    fn is_zero(&self) -> bool {
        self.atto.is_zero()
    }
}

impl PartialOrd for TokenAmount {
    #[inline]
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        self.atto.partial_cmp(&other.atto)
    }
}

impl Ord for TokenAmount {
    #[inline]
    fn cmp(&self, other: &Self) -> Ordering {
        self.atto.cmp(&other.atto)
    }
}

impl Default for TokenAmount {
    #[inline]
    fn default() -> TokenAmount {
        TokenAmount::zero()
    }
}

impl fmt::Debug for TokenAmount {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "TokenAmount({})", self)
    }
}

#[cfg(feature = "arb")]
impl quickcheck::Arbitrary for TokenAmount {
    fn arbitrary(g: &mut quickcheck::Gen) -> Self {
        TokenAmount::from_atto(BigInt::arbitrary(g))
    }
}

/// Displays a token amount as a decimal in human units.
/// To avoid any confusion over whether the value is in human-scale or indivisible units,
/// the display always includes a decimal point.
impl fmt::Display for TokenAmount {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // Implementation based on the bigdecimal library.
        let (q, r) = self.atto.div_rem(&BigInt::from(Self::PRECISION));
        let before_decimal = q.abs().to_str_radix(10);
        let after_decimal = if r.is_zero() {
            "0".to_string()
        } else {
            let fraction_str = r.abs().to_str_radix(10);
            let render = "0".repeat(Self::DECIMALS - fraction_str.len()) + fraction_str.as_str();
            render.trim_end_matches('0').to_string()
        };

        // Alter precision after the decimal point
        let after_decimal = if let Some(precision) = f.precision() {
            let len = after_decimal.len();
            if len < precision {
                after_decimal + "0".repeat(precision - len).as_str()
            } else {
                after_decimal[0..precision].to_string()
            }
        } else {
            after_decimal
        };

        // Always show the decimal point, even with ".0".
        let complete_without_sign = before_decimal + "." + after_decimal.as_str();
        // Padding works even though we have a decimal point.
        f.pad_integral(!self.atto().is_negative(), "", &complete_without_sign)
    }
}

impl Neg for TokenAmount {
    type Output = TokenAmount;

    #[inline]
    fn neg(self) -> TokenAmount {
        TokenAmount { atto: -self.atto }
    }
}

impl<'a> Neg for &'a TokenAmount {
    type Output = TokenAmount;

    #[inline]
    fn neg(self) -> TokenAmount {
        TokenAmount {
            atto: (&self.atto).neg(),
        }
    }
}

// Implements Add for all combinations of value/reference receiver and parameter.
// (Pattern copied from BigInt multiplication).
macro_rules! impl_add {
    ($(impl<$($a:lifetime),*> Add<$Other:ty> for $Self:ty;)*) => {$(
        impl<$($a),*> Add<$Other> for $Self {
            type Output = TokenAmount;

            #[inline]
            fn add(self, other: $Other) -> TokenAmount {
                // automatically match value/ref
                let TokenAmount { atto: x, .. } = self;
                let TokenAmount { atto: y, .. } = other;
                TokenAmount {atto: x + y}
            }
        }
    )*}
}
impl_add! {
    impl<> Add<TokenAmount> for TokenAmount;
    impl<'b> Add<&'b TokenAmount> for TokenAmount;
    impl<'a> Add<TokenAmount> for &'a TokenAmount;
    impl<'a, 'b> Add<&'b TokenAmount> for &'a TokenAmount;
}

impl AddAssign<TokenAmount> for TokenAmount {
    #[inline]
    fn add_assign(&mut self, other: TokenAmount) {
        self.atto += &other.atto;
    }
}

impl<'a> AddAssign<&'a TokenAmount> for TokenAmount {
    #[inline]
    fn add_assign(&mut self, other: &TokenAmount) {
        self.atto += &other.atto;
    }
}

// Implements Sub for all combinations of value/reference receiver and parameter.
macro_rules! impl_sub {
    ($(impl<$($a:lifetime),*> Sub<$Other:ty> for $Self:ty;)*) => {$(
        impl<$($a),*> Sub<$Other> for $Self {
            type Output = TokenAmount;

            #[inline]
            fn sub(self, other: $Other) -> TokenAmount {
                // automatically match value/ref
                let TokenAmount { atto: x, .. } = self;
                let TokenAmount { atto: y, .. } = other;
                TokenAmount {atto: x - y}
            }
        }
    )*}
}
impl_sub! {
    impl<> Sub<TokenAmount> for TokenAmount;
    impl<'b> Sub<&'b TokenAmount> for TokenAmount;
    impl<'a> Sub<TokenAmount> for &'a TokenAmount;
    impl<'a, 'b> Sub<&'b TokenAmount> for &'a TokenAmount;
}

impl SubAssign<TokenAmount> for TokenAmount {
    #[inline]
    fn sub_assign(&mut self, other: TokenAmount) {
        self.atto -= &other.atto;
    }
}

impl<'a> SubAssign<&'a TokenAmount> for TokenAmount {
    #[inline]
    fn sub_assign(&mut self, other: &TokenAmount) {
        self.atto -= &other.atto;
    }
}

impl<T> Mul<T> for TokenAmount
where
    BigInt: Mul<T, Output = BigInt>,
{
    type Output = TokenAmount;

    fn mul(self, rhs: T) -> Self::Output {
        TokenAmount {
            atto: self.atto * rhs,
        }
    }
}

impl<'a, T> Mul<T> for &'a TokenAmount
where
    &'a BigInt: Mul<T, Output = BigInt>,
{
    type Output = TokenAmount;

    fn mul(self, rhs: T) -> Self::Output {
        TokenAmount {
            atto: &self.atto * rhs,
        }
    }
}

macro_rules! impl_mul {
    ($(impl<$($a:lifetime),*> Mul<$Other:ty> for $Self:ty;)*) => {$(
        impl<$($a),*> Mul<$Other> for $Self {
            type Output = TokenAmount;

            #[inline]
            fn mul(self, other: $Other) -> TokenAmount {
                other * self
            }
        }
    )*}
}

macro_rules! impl_muls {
    ($($t:ty,)*) => {$(
        impl_mul! {
            impl<> Mul<TokenAmount> for $t;
            impl<'b> Mul<&'b TokenAmount> for $t;
            impl<'a> Mul<TokenAmount> for &'a $t;
            impl<'a, 'b> Mul<&'b TokenAmount> for &'a $t;
        }
    )*};
}

impl_muls! {
    u8, u16, u32, u64, u128,
    i8, i16, i32, i64, i128,
    BigInt,
}

impl<T> MulAssign<T> for TokenAmount
where
    BigInt: MulAssign<T>,
{
    #[inline]
    fn mul_assign(&mut self, other: T) {
        self.atto *= other;
    }
}

// Only a single div/rem method is implemented, rather than the full Div and Rem traits.
// Division isn't a common operation with money-like units, and deserves to be treated carefully.
impl TokenAmount {
    #[inline]
    pub fn div_rem(&self, other: impl Into<BigInt>) -> (TokenAmount, TokenAmount) {
        let (q, r) = self.atto.div_rem(&other.into());
        (TokenAmount { atto: q }, TokenAmount { atto: r })
    }

    #[inline]
    pub fn div_ceil(&self, other: impl Into<BigInt>) -> TokenAmount {
        TokenAmount {
            atto: self.atto.div_ceil(&other.into()),
        }
    }

    #[inline]
    pub fn div_floor(&self, other: impl Into<BigInt>) -> TokenAmount {
        TokenAmount {
            atto: self.atto.div_floor(&other.into()),
        }
    }
}

impl Sum for TokenAmount {
    fn sum<I: Iterator<Item = Self>>(iter: I) -> Self {
        Self::from_atto(iter.map(|t| t.atto).sum::<BigInt>())
    }
}

impl<'a> Sum<&'a TokenAmount> for TokenAmount {
    fn sum<I: Iterator<Item = &'a TokenAmount>>(iter: I) -> Self {
        Self::from_atto(iter.map(|t| &t.atto).sum::<BigInt>())
    }
}

// Serialisation

impl Serialize for TokenAmount {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        bigint_ser::serialize(&self.atto, serializer)
    }
}

impl<'de> Deserialize<'de> for TokenAmount {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        bigint_ser::deserialize(deserializer).map(|v| TokenAmount { atto: v })
    }
}

#[cfg(test)]
mod test {
    use num_bigint::BigInt;
    use num_traits::Zero;

    use crate::TokenAmount;

    fn whole(x: impl Into<BigInt>) -> TokenAmount {
        TokenAmount::from_whole(x)
    }

    fn atto(x: impl Into<BigInt>) -> TokenAmount {
        TokenAmount::from_atto(x.into())
    }

    #[test]
    fn display_basic() {
        fn basic(expected: &str, t: TokenAmount) {
            assert_eq!(expected, format!("{}", t));
        }

        basic("0.0", TokenAmount::zero());
        basic("0.000000000000000001", atto(1));
        basic("0.000000000000001", atto(1000));
        basic("0.1234", atto(123_400_000_000_000_000_u64));
        basic("0.10101", atto(101_010_000_000_000_000_u64));
        basic("1.0", whole(1));
        basic("1.0", atto(1_000_000_000_000_000_000_u128));
        basic("1.1", atto(1_100_000_000_000_000_000_u128));
        basic("1.000000000000000001", atto(1_000_000_000_000_000_001_u128));
        basic(
            "1234.000000000123456789",
            whole(1234) + atto(123_456_789_u64),
        );
    }

    #[test]
    fn display_precision() {
        assert_eq!("0.0", format!("{:.1}", TokenAmount::zero()));
        assert_eq!("0.000", format!("{:.3}", TokenAmount::zero()));
        assert_eq!("0.000", format!("{:.3}", atto(1))); // Truncated.
        assert_eq!(
            "0.123",
            format!("{:.3}", atto(123_456_789_000_000_000_u64)) // Truncated.
        );
        assert_eq!(
            "0.123456789000",
            format!("{:.12}", atto(123_456_789_000_000_000_u64))
        );
    }

    #[test]
    fn display_padding() {
        assert_eq!("0.0", format!("{:01}", TokenAmount::zero()));
        assert_eq!("0.0", format!("{:03}", TokenAmount::zero()));
        assert_eq!("000.0", format!("{:05}", TokenAmount::zero()));
        assert_eq!(
            "0.123",
            format!("{:01.3}", atto(123_456_789_000_000_000_u64))
        );
        assert_eq!(
            "00.123",
            format!("{:06.3}", atto(123_456_789_000_000_000_u64))
        );
    }

    #[test]
    fn display_negative() {
        assert_eq!("-0.000001", format!("{:01}", -TokenAmount::from_nano(1000)));
    }

    #[test]
    fn ops() {
        // Test the basic operations are wired up correctly.
        assert_eq!(atto(15), atto(10) + atto(5));
        assert_eq!(atto(3), atto(10) - atto(7));
        assert_eq!(atto(12), atto(3) * 4);
        let (q, r) = atto(14).div_rem(4);
        assert_eq!((atto(3), atto(2)), (q, r));

        let mut a = atto(1);
        a += atto(2);
        assert_eq!(atto(3), a);
        a *= 2;
        assert_eq!(atto(6), a);
        a -= atto(2);
        assert_eq!(atto(4), a);
    }

    #[test]
    fn nano_fil() {
        assert_eq!(
            TokenAmount::from_nano(1),
            TokenAmount::from_whole(1).div_floor(10u64.pow(9))
        )
    }

    #[test]
    fn test_mul() {
        let a = atto(2) * 3;
        let b = 3 * atto(2);
        assert_eq!(a, atto(6));
        assert_eq!(a, b);
    }

    #[test]
    fn test_sum() {
        assert_eq!(
            [1, 2, 3, 4].into_iter().map(atto).sum::<TokenAmount>(),
            atto(10)
        );
    }
}
