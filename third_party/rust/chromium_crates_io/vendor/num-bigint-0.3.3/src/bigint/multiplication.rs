use super::CheckedUnsignedAbs::{Negative, Positive};
use super::Sign::{self, Minus, NoSign, Plus};
use super::{BigInt, UnsignedAbs};

use crate::{IsizePromotion, UsizePromotion};

use core::iter::Product;
use core::ops::{Mul, MulAssign};
use num_traits::{CheckedMul, One, Zero};

impl Mul<Sign> for Sign {
    type Output = Sign;

    #[inline]
    fn mul(self, other: Sign) -> Sign {
        match (self, other) {
            (NoSign, _) | (_, NoSign) => NoSign,
            (Plus, Plus) | (Minus, Minus) => Plus,
            (Plus, Minus) | (Minus, Plus) => Minus,
        }
    }
}

forward_all_binop_to_ref_ref!(impl Mul for BigInt, mul);

impl<'a, 'b> Mul<&'b BigInt> for &'a BigInt {
    type Output = BigInt;

    #[inline]
    fn mul(self, other: &BigInt) -> BigInt {
        BigInt::from_biguint(self.sign * other.sign, &self.data * &other.data)
    }
}

impl<'a> MulAssign<&'a BigInt> for BigInt {
    #[inline]
    fn mul_assign(&mut self, other: &BigInt) {
        *self = &*self * other;
    }
}
forward_val_assign!(impl MulAssign for BigInt, mul_assign);

promote_all_scalars!(impl Mul for BigInt, mul);
promote_all_scalars_assign!(impl MulAssign for BigInt, mul_assign);
forward_all_scalar_binop_to_val_val_commutative!(impl Mul<u32> for BigInt, mul);
forward_all_scalar_binop_to_val_val_commutative!(impl Mul<u64> for BigInt, mul);
forward_all_scalar_binop_to_val_val_commutative!(impl Mul<u128> for BigInt, mul);

impl Mul<u32> for BigInt {
    type Output = BigInt;

    #[inline]
    fn mul(self, other: u32) -> BigInt {
        BigInt::from_biguint(self.sign, self.data * other)
    }
}

impl MulAssign<u32> for BigInt {
    #[inline]
    fn mul_assign(&mut self, other: u32) {
        self.data *= other;
        if self.data.is_zero() {
            self.sign = NoSign;
        }
    }
}

impl Mul<u64> for BigInt {
    type Output = BigInt;

    #[inline]
    fn mul(self, other: u64) -> BigInt {
        BigInt::from_biguint(self.sign, self.data * other)
    }
}

impl MulAssign<u64> for BigInt {
    #[inline]
    fn mul_assign(&mut self, other: u64) {
        self.data *= other;
        if self.data.is_zero() {
            self.sign = NoSign;
        }
    }
}

impl Mul<u128> for BigInt {
    type Output = BigInt;

    #[inline]
    fn mul(self, other: u128) -> BigInt {
        BigInt::from_biguint(self.sign, self.data * other)
    }
}

impl MulAssign<u128> for BigInt {
    #[inline]
    fn mul_assign(&mut self, other: u128) {
        self.data *= other;
        if self.data.is_zero() {
            self.sign = NoSign;
        }
    }
}

forward_all_scalar_binop_to_val_val_commutative!(impl Mul<i32> for BigInt, mul);
forward_all_scalar_binop_to_val_val_commutative!(impl Mul<i64> for BigInt, mul);
forward_all_scalar_binop_to_val_val_commutative!(impl Mul<i128> for BigInt, mul);

impl Mul<i32> for BigInt {
    type Output = BigInt;

    #[inline]
    fn mul(self, other: i32) -> BigInt {
        match other.checked_uabs() {
            Positive(u) => self * u,
            Negative(u) => -self * u,
        }
    }
}

impl MulAssign<i32> for BigInt {
    #[inline]
    fn mul_assign(&mut self, other: i32) {
        match other.checked_uabs() {
            Positive(u) => *self *= u,
            Negative(u) => {
                self.sign = -self.sign;
                self.data *= u;
            }
        }
    }
}

impl Mul<i64> for BigInt {
    type Output = BigInt;

    #[inline]
    fn mul(self, other: i64) -> BigInt {
        match other.checked_uabs() {
            Positive(u) => self * u,
            Negative(u) => -self * u,
        }
    }
}

impl MulAssign<i64> for BigInt {
    #[inline]
    fn mul_assign(&mut self, other: i64) {
        match other.checked_uabs() {
            Positive(u) => *self *= u,
            Negative(u) => {
                self.sign = -self.sign;
                self.data *= u;
            }
        }
    }
}

impl Mul<i128> for BigInt {
    type Output = BigInt;

    #[inline]
    fn mul(self, other: i128) -> BigInt {
        match other.checked_uabs() {
            Positive(u) => self * u,
            Negative(u) => -self * u,
        }
    }
}

impl MulAssign<i128> for BigInt {
    #[inline]
    fn mul_assign(&mut self, other: i128) {
        match other.checked_uabs() {
            Positive(u) => *self *= u,
            Negative(u) => {
                self.sign = -self.sign;
                self.data *= u;
            }
        }
    }
}

impl CheckedMul for BigInt {
    #[inline]
    fn checked_mul(&self, v: &BigInt) -> Option<BigInt> {
        Some(self.mul(v))
    }
}

impl_product_iter_type!(BigInt);
