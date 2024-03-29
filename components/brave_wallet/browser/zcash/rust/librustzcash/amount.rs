// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// https://github.com/zcash/librustzcash/blob/zcash_primitives-0.14.0/zcash_primitives/src/transaction/components/amount.rs

use std::iter::Sum;
use std::ops::{Add, AddAssign, Mul, Neg, Sub, SubAssign};

use orchard::value as orchard;

pub const COIN: i64 = 1_0000_0000;
pub const MAX_MONEY: i64 = 21_000_000 * COIN;

#[derive(Clone, Copy, Debug, PartialEq, PartialOrd, Eq, Ord)]
pub struct Amount(i64);
use memuse::DynamicUsage;

memuse::impl_no_dynamic_usage!(Amount);

impl Amount {
    /// Returns a zero-valued Amount.
    pub const fn zero() -> Self {
        Amount(0)
    }

    /// Creates a constant Amount from an i64.
    ///
    /// Panics: if the amount is outside the range `{-MAX_MONEY..MAX_MONEY}`.
    pub const fn const_from_i64(amount: i64) -> Self {
        assert!(-MAX_MONEY <= amount && amount <= MAX_MONEY); // contains is not const
        Amount(amount)
    }

    /// Creates a constant Amount from a u64.
    ///
    /// Panics: if the amount is outside the range `{0..MAX_MONEY}`.
    const fn const_from_u64(amount: u64) -> Self {
        assert!(amount <= (MAX_MONEY as u64)); // contains is not const
        Amount(amount as i64)
    }

    /// Creates an Amount from an i64.
    ///
    /// Returns an error if the amount is outside the range `{-MAX_MONEY..MAX_MONEY}`.
    pub fn from_i64(amount: i64) -> Result<Self, ()> {
        if (-MAX_MONEY..=MAX_MONEY).contains(&amount) {
            Ok(Amount(amount))
        } else {
            Err(())
        }
    }

    /// Creates a non-negative Amount from an i64.
    ///
    /// Returns an error if the amount is outside the range `{0..MAX_MONEY}`.
    pub fn from_nonnegative_i64(amount: i64) -> Result<Self, ()> {
        if (0..=MAX_MONEY).contains(&amount) {
            Ok(Amount(amount))
        } else {
            Err(())
        }
    }

    /// Creates an Amount from a u64.
    ///
    /// Returns an error if the amount is outside the range `{0..MAX_MONEY}`.
    pub fn from_u64(amount: u64) -> Result<Self, ()> {
        if amount <= MAX_MONEY as u64 {
            Ok(Amount(amount as i64))
        } else {
            Err(())
        }
    }

    /// Reads an Amount from a signed 64-bit little-endian integer.
    ///
    /// Returns an error if the amount is outside the range `{-MAX_MONEY..MAX_MONEY}`.
    pub fn from_i64_le_bytes(bytes: [u8; 8]) -> Result<Self, ()> {
        let amount = i64::from_le_bytes(bytes);
        Amount::from_i64(amount)
    }

    /// Reads a non-negative Amount from a signed 64-bit little-endian integer.
    ///
    /// Returns an error if the amount is outside the range `{0..MAX_MONEY}`.
    pub fn from_nonnegative_i64_le_bytes(bytes: [u8; 8]) -> Result<Self, ()> {
        let amount = i64::from_le_bytes(bytes);
        Amount::from_nonnegative_i64(amount)
    }

    /// Reads an Amount from an unsigned 64-bit little-endian integer.
    ///
    /// Returns an error if the amount is outside the range `{0..MAX_MONEY}`.
    pub fn from_u64_le_bytes(bytes: [u8; 8]) -> Result<Self, ()> {
        let amount = u64::from_le_bytes(bytes);
        Amount::from_u64(amount)
    }

    /// Returns the Amount encoded as a signed 64-bit little-endian integer.
    pub fn to_i64_le_bytes(self) -> [u8; 8] {
        self.0.to_le_bytes()
    }

    /// Returns `true` if `self` is positive and `false` if the Amount is zero or
    /// negative.
    pub const fn is_positive(self) -> bool {
        self.0.is_positive()
    }

    /// Returns `true` if `self` is negative and `false` if the Amount is zero or
    /// positive.
    pub const fn is_negative(self) -> bool {
        self.0.is_negative()
    }

    pub fn sum<I: IntoIterator<Item = Amount>>(values: I) -> Option<Amount> {
        let mut result = Amount::zero();
        for value in values {
            result = (result + value)?;
        }
        Some(result)
    }
}

impl TryFrom<i64> for Amount {
    type Error = ();

    fn try_from(value: i64) -> Result<Self, ()> {
        Amount::from_i64(value)
    }
}

impl From<Amount> for i64 {
    fn from(amount: Amount) -> i64 {
        amount.0
    }
}

impl From<&Amount> for i64 {
    fn from(amount: &Amount) -> i64 {
        amount.0
    }
}

impl TryFrom<Amount> for u64 {
    type Error = ();

    fn try_from(value: Amount) -> Result<Self, Self::Error> {
        value.0.try_into().map_err(|_| ())
    }
}

impl Add<Amount> for Amount {
    type Output = Option<Amount>;

    fn add(self, rhs: Amount) -> Option<Amount> {
        Amount::from_i64(self.0 + rhs.0).ok()
    }
}

impl Add<Amount> for Option<Amount> {
    type Output = Self;

    fn add(self, rhs: Amount) -> Option<Amount> {
        self.and_then(|lhs| lhs + rhs)
    }
}

impl AddAssign<Amount> for Amount {
    fn add_assign(&mut self, rhs: Amount) {
        *self = (*self + rhs).expect("Addition must produce a valid amount value.")
    }
}

impl Sub<Amount> for Amount {
    type Output = Option<Amount>;

    fn sub(self, rhs: Amount) -> Option<Amount> {
        Amount::from_i64(self.0 - rhs.0).ok()
    }
}

impl Sub<Amount> for Option<Amount> {
    type Output = Self;

    fn sub(self, rhs: Amount) -> Option<Amount> {
        self.and_then(|lhs| lhs - rhs)
    }
}

impl SubAssign<Amount> for Amount {
    fn sub_assign(&mut self, rhs: Amount) {
        *self = (*self - rhs).expect("Subtraction must produce a valid amount value.")
    }
}

impl Sum<Amount> for Option<Amount> {
    fn sum<I: Iterator<Item = Amount>>(iter: I) -> Self {
        iter.fold(Some(Amount::zero()), |acc, a| acc? + a)
    }
}

impl<'a> Sum<&'a Amount> for Option<Amount> {
    fn sum<I: Iterator<Item = &'a Amount>>(iter: I) -> Self {
        iter.fold(Some(Amount::zero()), |acc, a| acc? + *a)
    }
}

impl Neg for Amount {
    type Output = Self;

    fn neg(self) -> Self {
        Amount(-self.0)
    }
}

impl Mul<usize> for Amount {
    type Output = Option<Amount>;

    fn mul(self, rhs: usize) -> Option<Amount> {
        let rhs: i64 = rhs.try_into().ok()?;
        self.0
            .checked_mul(rhs)
            .and_then(|i| Amount::try_from(i).ok())
    }
}

impl TryFrom<orchard::ValueSum> for Amount {
    type Error = ();

    fn try_from(v: orchard::ValueSum) -> Result<Amount, Self::Error> {
        i64::try_from(v).map_err(|_| ()).and_then(Amount::try_from)
    }
}

/// A type-safe representation of some nonnegative amount of Zcash.
///
/// A NonNegativeAmount can only be constructed from an integer that is within the valid monetary
/// range of `{0..MAX_MONEY}` (where `MAX_MONEY` = 21,000,000 × 10⁸ zatoshis).
#[derive(Clone, Copy, Debug, PartialEq, PartialOrd, Eq, Ord)]
pub struct NonNegativeAmount(Amount);

impl NonNegativeAmount {
    /// Returns the identity `NonNegativeAmount`
    pub const ZERO: Self = NonNegativeAmount(Amount(0));

    /// Creates a NonNegativeAmount from a u64.
    ///
    /// Returns an error if the amount is outside the range `{0..MAX_MONEY}`.
    pub fn from_u64(amount: u64) -> Result<Self, ()> {
        Amount::from_u64(amount).map(NonNegativeAmount)
    }

    /// Creates a constant NonNegativeAmount from a u64.
    ///
    /// Panics: if the amount is outside the range `{-MAX_MONEY..MAX_MONEY}`.
    pub const fn const_from_u64(amount: u64) -> Self {
        NonNegativeAmount(Amount::const_from_u64(amount))
    }

    /// Creates a NonNegativeAmount from an i64.
    ///
    /// Returns an error if the amount is outside the range `{0..MAX_MONEY}`.
    pub fn from_nonnegative_i64(amount: i64) -> Result<Self, ()> {
        Amount::from_nonnegative_i64(amount).map(NonNegativeAmount)
    }

    /// Reads an NonNegativeAmount from an unsigned 64-bit little-endian integer.
    ///
    /// Returns an error if the amount is outside the range `{0..MAX_MONEY}`.
    pub fn from_u64_le_bytes(bytes: [u8; 8]) -> Result<Self, ()> {
        let amount = u64::from_le_bytes(bytes);
        Self::from_u64(amount)
    }

    /// Reads a NonNegativeAmount from a signed integer represented as a two's
    /// complement 64-bit little-endian value.
    ///
    /// Returns an error if the amount is outside the range `{0..MAX_MONEY}`.
    pub fn from_nonnegative_i64_le_bytes(bytes: [u8; 8]) -> Result<Self, ()> {
        let amount = i64::from_le_bytes(bytes);
        Self::from_nonnegative_i64(amount)
    }

    /// Returns this NonNegativeAmount encoded as a signed two's complement 64-bit
    /// little-endian value.
    pub fn to_i64_le_bytes(self) -> [u8; 8] {
        self.0.to_i64_le_bytes()
    }

    /// Returns whether or not this `NonNegativeAmount` is the zero value.
    pub fn is_zero(&self) -> bool {
        self == &NonNegativeAmount::ZERO
    }

    /// Returns whether or not this `NonNegativeAmount` is positive.
    pub fn is_positive(&self) -> bool {
        self > &NonNegativeAmount::ZERO
    }
}

impl From<NonNegativeAmount> for Amount {
    fn from(n: NonNegativeAmount) -> Self {
        n.0
    }
}

impl From<&NonNegativeAmount> for Amount {
    fn from(n: &NonNegativeAmount) -> Self {
        n.0
    }
}

impl From<NonNegativeAmount> for u64 {
    fn from(n: NonNegativeAmount) -> Self {
        n.0.try_into().unwrap()
    }
}

impl From<NonNegativeAmount> for orchard::NoteValue {
    fn from(n: NonNegativeAmount) -> Self {
        orchard::NoteValue::from_raw(n.into())
    }
}

impl TryFrom<orchard::NoteValue> for NonNegativeAmount {
    type Error = ();

    fn try_from(value: orchard::NoteValue) -> Result<Self, Self::Error> {
        Self::from_u64(value.inner())
    }
}

impl TryFrom<Amount> for NonNegativeAmount {
    type Error = ();

    fn try_from(value: Amount) -> Result<Self, Self::Error> {
        if value.is_negative() {
            Err(())
        } else {
            Ok(NonNegativeAmount(value))
        }
    }
}

impl Add<NonNegativeAmount> for NonNegativeAmount {
    type Output = Option<NonNegativeAmount>;

    fn add(self, rhs: NonNegativeAmount) -> Option<NonNegativeAmount> {
        (self.0 + rhs.0).map(NonNegativeAmount)
    }
}

impl Add<NonNegativeAmount> for Option<NonNegativeAmount> {
    type Output = Self;

    fn add(self, rhs: NonNegativeAmount) -> Option<NonNegativeAmount> {
        self.and_then(|lhs| lhs + rhs)
    }
}

impl Sub<NonNegativeAmount> for NonNegativeAmount {
    type Output = Option<NonNegativeAmount>;

    fn sub(self, rhs: NonNegativeAmount) -> Option<NonNegativeAmount> {
        (self.0 - rhs.0).and_then(|amt| NonNegativeAmount::try_from(amt).ok())
    }
}

impl Sub<NonNegativeAmount> for Option<NonNegativeAmount> {
    type Output = Self;

    fn sub(self, rhs: NonNegativeAmount) -> Option<NonNegativeAmount> {
        self.and_then(|lhs| lhs - rhs)
    }
}

impl Mul<usize> for NonNegativeAmount {
    type Output = Option<Self>;

    fn mul(self, rhs: usize) -> Option<NonNegativeAmount> {
        (self.0 * rhs).and_then(|v| NonNegativeAmount::try_from(v).ok())
    }
}

impl Sum<NonNegativeAmount> for Option<NonNegativeAmount> {
    fn sum<I: Iterator<Item = NonNegativeAmount>>(iter: I) -> Self {
        iter.fold(Some(NonNegativeAmount::ZERO), |acc, a| acc? + a)
    }
}

impl<'a> Sum<&'a NonNegativeAmount> for Option<NonNegativeAmount> {
    fn sum<I: Iterator<Item = &'a NonNegativeAmount>>(iter: I) -> Self {
        iter.fold(Some(NonNegativeAmount::ZERO), |acc, a| acc? + *a)
    }
}

