use crate::{F8E4M3, F8E5M2};
use core::cmp::Ordering;
use core::{num::FpCategory, ops::Div};
use num_traits::{
    AsPrimitive, Bounded, FloatConst, FromPrimitive, Num, NumCast, One, ToPrimitive, Zero,
};

impl ToPrimitive for F8E5M2 {
    #[inline]
    fn to_i64(&self) -> Option<i64> {
        Self::to_f32(self).to_i64()
    }
    #[inline]
    fn to_u64(&self) -> Option<u64> {
        Self::to_f32(self).to_u64()
    }
    #[inline]
    fn to_i8(&self) -> Option<i8> {
        Self::to_f32(self).to_i8()
    }
    #[inline]
    fn to_u8(&self) -> Option<u8> {
        Self::to_f32(self).to_u8()
    }
    #[inline]
    fn to_i16(&self) -> Option<i16> {
        Self::to_f32(self).to_i16()
    }
    #[inline]
    fn to_u16(&self) -> Option<u16> {
        Self::to_f32(self).to_u16()
    }
    #[inline]
    fn to_i32(&self) -> Option<i32> {
        Self::to_f32(self).to_i32()
    }
    #[inline]
    fn to_u32(&self) -> Option<u32> {
        Self::to_f32(self).to_u32()
    }
    #[inline]
    fn to_f32(&self) -> Option<f32> {
        Some(Self::to_f32(self))
    }
    #[inline]
    fn to_f64(&self) -> Option<f64> {
        Some(Self::to_f64(self))
    }
}

impl FromPrimitive for F8E5M2 {
    #[inline]
    fn from_i64(n: i64) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_u64(n: u64) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_i8(n: i8) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_u8(n: u8) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_i16(n: i16) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_u16(n: u16) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_i32(n: i32) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_u32(n: u32) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_f32(n: f32) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_f64(n: f64) -> Option<Self> {
        n.to_f64().map(Self::from_f64)
    }
}

impl Num for F8E5M2 {
    type FromStrRadixErr = <f32 as Num>::FromStrRadixErr;

    #[inline]
    fn from_str_radix(str: &str, radix: u32) -> Result<Self, Self::FromStrRadixErr> {
        Ok(Self::from_f32(f32::from_str_radix(str, radix)?))
    }
}

impl One for F8E5M2 {
    #[inline]
    fn one() -> Self {
        Self::ONE
    }
}

impl Zero for F8E5M2 {
    #[inline]
    fn zero() -> Self {
        Self::ZERO
    }

    #[inline]
    fn is_zero(&self) -> bool {
        *self == Self::ZERO
    }
}

impl NumCast for F8E5M2 {
    #[inline]
    fn from<T: ToPrimitive>(n: T) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
}

impl num_traits::float::FloatCore for F8E5M2 {
    #[inline]
    fn infinity() -> Self {
        Self::INFINITY
    }

    #[inline]
    fn neg_infinity() -> Self {
        Self::NEG_INFINITY
    }

    #[inline]
    fn nan() -> Self {
        Self::NAN
    }

    #[inline]
    fn neg_zero() -> Self {
        Self::NEG_ZERO
    }

    #[inline]
    fn min_value() -> Self {
        Self::MIN
    }

    #[inline]
    fn min_positive_value() -> Self {
        Self::MIN_POSITIVE
    }

    #[inline]
    fn epsilon() -> Self {
        Self::EPSILON
    }

    #[inline]
    fn max_value() -> Self {
        Self::MAX
    }

    #[inline]
    fn is_nan(self) -> bool {
        Self::is_nan(&self)
    }

    #[inline]
    fn is_infinite(self) -> bool {
        Self::is_infinite(&self)
    }

    #[inline]
    fn is_finite(self) -> bool {
        Self::is_finite(&self)
    }

    #[inline]
    fn is_normal(self) -> bool {
        Self::is_normal(&self)
    }

    #[inline]
    fn classify(self) -> FpCategory {
        Self::classify(&self)
    }

    #[inline]
    fn floor(self) -> Self {
        Self::from_f32(self.to_f32().floor())
    }

    #[inline]
    fn ceil(self) -> Self {
        Self::from_f32(self.to_f32().ceil())
    }

    #[inline]
    fn round(self) -> Self {
        Self::from_f32(self.to_f32().round())
    }

    #[inline]
    fn trunc(self) -> Self {
        Self::from_f32(self.to_f32().trunc())
    }

    #[inline]
    fn fract(self) -> Self {
        Self::from_f32(self.to_f32().fract())
    }

    #[inline]
    fn abs(self) -> Self {
        Self::from_bits(self.to_bits() & 0x7F)
    }

    #[inline]
    fn signum(self) -> Self {
        self.signum()
    }

    #[inline]
    fn is_sign_positive(self) -> bool {
        Self::is_sign_positive(&self)
    }

    #[inline]
    fn is_sign_negative(self) -> bool {
        Self::is_sign_negative(&self)
    }

    fn min(self, other: Self) -> Self {
        match self.partial_cmp(&other) {
            None => {
                if self.is_nan() {
                    other
                } else {
                    self
                }
            }
            Some(Ordering::Greater) | Some(Ordering::Equal) => other,
            Some(Ordering::Less) => self,
        }
    }

    fn max(self, other: Self) -> Self {
        match self.partial_cmp(&other) {
            None => {
                if self.is_nan() {
                    other
                } else {
                    self
                }
            }
            Some(Ordering::Greater) | Some(Ordering::Equal) => self,
            Some(Ordering::Less) => other,
        }
    }

    #[inline]
    fn recip(self) -> Self {
        Self::from_f32(self.to_f32().recip())
    }

    #[inline]
    fn powi(self, exp: i32) -> Self {
        Self::from_f32(self.to_f32().powi(exp))
    }

    #[inline]
    fn to_degrees(self) -> Self {
        Self::from_f32(self.to_f32().to_degrees())
    }

    #[inline]
    fn to_radians(self) -> Self {
        Self::from_f32(self.to_f32().to_radians())
    }

    #[inline]
    fn integer_decode(self) -> (u64, i16, i8) {
        num_traits::float::FloatCore::integer_decode(self.to_f32())
    }
}

impl num_traits::float::Float for F8E5M2 {
    #[inline]
    fn nan() -> Self {
        Self::NAN
    }

    #[inline]
    fn infinity() -> Self {
        Self::INFINITY
    }

    #[inline]
    fn neg_infinity() -> Self {
        Self::NEG_INFINITY
    }

    #[inline]
    fn neg_zero() -> Self {
        Self::NEG_ZERO
    }

    #[inline]
    fn min_value() -> Self {
        Self::MIN
    }

    #[inline]
    fn min_positive_value() -> Self {
        Self::MIN_POSITIVE
    }

    #[inline]
    fn epsilon() -> Self {
        Self::EPSILON
    }

    #[inline]
    fn max_value() -> Self {
        Self::MAX
    }

    #[inline]
    fn is_nan(self) -> bool {
        Self::is_nan(&self)
    }

    #[inline]
    fn is_infinite(self) -> bool {
        Self::is_infinite(&self)
    }

    #[inline]
    fn is_finite(self) -> bool {
        Self::is_finite(&self)
    }

    #[inline]
    fn is_normal(self) -> bool {
        Self::is_normal(&self)
    }

    #[inline]
    fn classify(self) -> FpCategory {
        Self::classify(&self)
    }

    #[inline]
    fn floor(self) -> Self {
        Self::from_f32(self.to_f32().floor())
    }

    #[inline]
    fn ceil(self) -> Self {
        Self::from_f32(self.to_f32().ceil())
    }

    #[inline]
    fn round(self) -> Self {
        Self::from_f32(self.to_f32().round())
    }

    #[inline]
    fn trunc(self) -> Self {
        Self::from_f32(self.to_f32().trunc())
    }

    #[inline]
    fn fract(self) -> Self {
        Self::from_f32(self.to_f32().fract())
    }

    #[inline]
    fn abs(self) -> Self {
        Self::from_f32(self.to_f32().abs())
    }

    #[inline]
    fn signum(self) -> Self {
        Self::from_f32(self.to_f32().signum())
    }

    #[inline]
    fn is_sign_positive(self) -> bool {
        Self::is_sign_positive(&self)
    }

    #[inline]
    fn is_sign_negative(self) -> bool {
        Self::is_sign_negative(&self)
    }

    #[inline]
    fn mul_add(self, a: Self, b: Self) -> Self {
        Self::from_f32(self.to_f32().mul_add(a.to_f32(), b.to_f32()))
    }

    #[inline]
    fn recip(self) -> Self {
        Self::from_f32(self.to_f32().recip())
    }

    #[inline]
    fn powi(self, n: i32) -> Self {
        Self::from_f32(self.to_f32().powi(n))
    }

    #[inline]
    fn powf(self, n: Self) -> Self {
        Self::from_f32(self.to_f32().powf(n.to_f32()))
    }

    #[inline]
    fn sqrt(self) -> Self {
        Self::from_f32(self.to_f32().sqrt())
    }

    #[inline]
    fn exp(self) -> Self {
        Self::from_f32(self.to_f32().exp())
    }

    #[inline]
    fn exp2(self) -> Self {
        Self::from_f32(self.to_f32().exp2())
    }

    #[inline]
    fn ln(self) -> Self {
        Self::from_f32(self.to_f32().ln())
    }

    #[inline]
    fn log(self, base: Self) -> Self {
        Self::from_f32(self.to_f32().log(base.to_f32()))
    }

    #[inline]
    fn log2(self) -> Self {
        Self::from_f32(self.to_f32().log2())
    }

    #[inline]
    fn log10(self) -> Self {
        Self::from_f32(self.to_f32().log10())
    }

    #[inline]
    fn to_degrees(self) -> Self {
        Self::from_f32(self.to_f32().to_degrees())
    }

    #[inline]
    fn to_radians(self) -> Self {
        Self::from_f32(self.to_f32().to_radians())
    }

    #[inline]
    fn max(self, other: Self) -> Self {
        self.max(other)
    }

    #[inline]
    fn min(self, other: Self) -> Self {
        self.min(other)
    }

    #[inline]
    fn abs_sub(self, other: Self) -> Self {
        Self::from_f32((self.to_f32() - other.to_f32()).max(0.0))
    }

    #[inline]
    fn cbrt(self) -> Self {
        Self::from_f32(self.to_f32().cbrt())
    }

    #[inline]
    fn hypot(self, other: Self) -> Self {
        Self::from_f32(self.to_f32().hypot(other.to_f32()))
    }

    #[inline]
    fn sin(self) -> Self {
        Self::from_f32(self.to_f32().sin())
    }

    #[inline]
    fn cos(self) -> Self {
        Self::from_f32(self.to_f32().cos())
    }

    #[inline]
    fn tan(self) -> Self {
        Self::from_f32(self.to_f32().tan())
    }

    #[inline]
    fn asin(self) -> Self {
        Self::from_f32(self.to_f32().asin())
    }

    #[inline]
    fn acos(self) -> Self {
        Self::from_f32(self.to_f32().acos())
    }

    #[inline]
    fn atan(self) -> Self {
        Self::from_f32(self.to_f32().atan())
    }

    #[inline]
    fn atan2(self, other: Self) -> Self {
        Self::from_f32(self.to_f32().atan2(other.to_f32()))
    }

    #[inline]
    fn sin_cos(self) -> (Self, Self) {
        let (sin, cos) = self.to_f32().sin_cos();
        (Self::from_f32(sin), Self::from_f32(cos))
    }

    #[inline]
    fn exp_m1(self) -> Self {
        Self::from_f32(self.to_f32().exp_m1())
    }

    #[inline]
    fn ln_1p(self) -> Self {
        Self::from_f32(self.to_f32().ln_1p())
    }

    #[inline]
    fn sinh(self) -> Self {
        Self::from_f32(self.to_f32().sinh())
    }

    #[inline]
    fn cosh(self) -> Self {
        Self::from_f32(self.to_f32().cosh())
    }

    #[inline]
    fn tanh(self) -> Self {
        Self::from_f32(self.to_f32().tanh())
    }

    #[inline]
    fn asinh(self) -> Self {
        Self::from_f32(self.to_f32().asinh())
    }

    #[inline]
    fn acosh(self) -> Self {
        Self::from_f32(self.to_f32().acosh())
    }

    #[inline]
    fn atanh(self) -> Self {
        Self::from_f32(self.to_f32().atanh())
    }

    #[inline]
    fn integer_decode(self) -> (u64, i16, i8) {
        num_traits::float::Float::integer_decode(self.to_f32())
    }
}

impl FloatConst for F8E5M2 {
    #[inline]
    fn E() -> Self {
        Self::E
    }

    #[inline]
    fn FRAC_1_PI() -> Self {
        Self::FRAC_1_PI
    }

    #[inline]
    fn FRAC_1_SQRT_2() -> Self {
        Self::FRAC_1_SQRT_2
    }

    #[inline]
    fn FRAC_2_PI() -> Self {
        Self::FRAC_2_PI
    }

    #[inline]
    fn FRAC_2_SQRT_PI() -> Self {
        Self::FRAC_2_SQRT_PI
    }

    #[inline]
    fn FRAC_PI_2() -> Self {
        Self::FRAC_PI_2
    }

    #[inline]
    fn FRAC_PI_3() -> Self {
        Self::FRAC_PI_3
    }

    #[inline]
    fn FRAC_PI_4() -> Self {
        Self::FRAC_PI_4
    }

    #[inline]
    fn FRAC_PI_6() -> Self {
        Self::FRAC_PI_6
    }

    #[inline]
    fn FRAC_PI_8() -> Self {
        Self::FRAC_PI_8
    }

    #[inline]
    fn LN_10() -> Self {
        Self::LN_10
    }

    #[inline]
    fn LN_2() -> Self {
        Self::LN_2
    }

    #[inline]
    fn LOG10_E() -> Self {
        Self::LOG10_E
    }

    #[inline]
    fn LOG2_E() -> Self {
        Self::LOG2_E
    }

    #[inline]
    fn PI() -> Self {
        Self::PI
    }

    fn SQRT_2() -> Self {
        Self::SQRT_2
    }

    #[inline]
    fn LOG10_2() -> Self
    where
        Self: Sized + Div<Self, Output = Self>,
    {
        Self::LOG10_2
    }

    #[inline]
    fn LOG2_10() -> Self
    where
        Self: Sized + Div<Self, Output = Self>,
    {
        Self::LOG2_10
    }
}

impl Bounded for F8E5M2 {
    #[inline]
    fn min_value() -> Self {
        F8E5M2::MIN
    }

    #[inline]
    fn max_value() -> Self {
        F8E5M2::MAX
    }
}

macro_rules! impl_as_primitive_to_F8E5M2 {
    ($ty:ty, $meth:ident) => {
        impl AsPrimitive<$ty> for F8E5M2 {
            #[inline]
            fn as_(self) -> $ty {
                self.$meth().as_()
            }
        }
    };
}

impl AsPrimitive<F8E5M2> for F8E5M2 {
    #[inline]
    fn as_(self) -> F8E5M2 {
        self
    }
}

impl_as_primitive_to_F8E5M2!(i64, to_f32);
impl_as_primitive_to_F8E5M2!(u64, to_f32);
impl_as_primitive_to_F8E5M2!(i8, to_f32);
impl_as_primitive_to_F8E5M2!(u8, to_f32);
impl_as_primitive_to_F8E5M2!(i16, to_f32);
impl_as_primitive_to_F8E5M2!(u16, to_f32);
impl_as_primitive_to_F8E5M2!(i32, to_f32);
impl_as_primitive_to_F8E5M2!(u32, to_f32);
impl_as_primitive_to_F8E5M2!(isize, to_f32);
impl_as_primitive_to_F8E5M2!(usize, to_f32);
impl_as_primitive_to_F8E5M2!(f32, to_f32);
impl_as_primitive_to_F8E5M2!(f64, to_f64);
impl_as_primitive_to_F8E5M2!(F8E4M3, to_f32);

macro_rules! impl_as_primitive_F8E5M2_from {
    ($ty:ty, $meth:ident) => {
        impl AsPrimitive<F8E5M2> for $ty {
            #[inline]
            fn as_(self) -> F8E5M2 {
                F8E5M2::$meth(self.as_())
            }
        }
    };
}

impl_as_primitive_F8E5M2_from!(i64, from_f32);
impl_as_primitive_F8E5M2_from!(u64, from_f32);
impl_as_primitive_F8E5M2_from!(i8, from_f32);
impl_as_primitive_F8E5M2_from!(u8, from_f32);
impl_as_primitive_F8E5M2_from!(i16, from_f32);
impl_as_primitive_F8E5M2_from!(u16, from_f32);
impl_as_primitive_F8E5M2_from!(i32, from_f32);
impl_as_primitive_F8E5M2_from!(u32, from_f32);
impl_as_primitive_F8E5M2_from!(isize, from_f32);
impl_as_primitive_F8E5M2_from!(usize, from_f32);
impl_as_primitive_F8E5M2_from!(f32, from_f32);
impl_as_primitive_F8E5M2_from!(f64, from_f64);

impl ToPrimitive for F8E4M3 {
    #[inline]
    fn to_i64(&self) -> Option<i64> {
        Self::to_f32(self).to_i64()
    }
    #[inline]
    fn to_u64(&self) -> Option<u64> {
        Self::to_f32(self).to_u64()
    }
    #[inline]
    fn to_i8(&self) -> Option<i8> {
        Self::to_f32(self).to_i8()
    }
    #[inline]
    fn to_u8(&self) -> Option<u8> {
        Self::to_f32(self).to_u8()
    }
    #[inline]
    fn to_i16(&self) -> Option<i16> {
        Self::to_f32(self).to_i16()
    }
    #[inline]
    fn to_u16(&self) -> Option<u16> {
        Self::to_f32(self).to_u16()
    }
    #[inline]
    fn to_i32(&self) -> Option<i32> {
        Self::to_f32(self).to_i32()
    }
    #[inline]
    fn to_u32(&self) -> Option<u32> {
        Self::to_f32(self).to_u32()
    }
    #[inline]
    fn to_f32(&self) -> Option<f32> {
        Some(Self::to_f32(self))
    }
    #[inline]
    fn to_f64(&self) -> Option<f64> {
        Some(Self::to_f64(self))
    }
}

impl FromPrimitive for F8E4M3 {
    #[inline]
    fn from_i64(n: i64) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_u64(n: u64) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_i8(n: i8) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_u8(n: u8) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_i16(n: i16) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_u16(n: u16) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_i32(n: i32) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_u32(n: u32) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_f32(n: f32) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
    #[inline]
    fn from_f64(n: f64) -> Option<Self> {
        n.to_f64().map(Self::from_f64)
    }
}

impl Num for F8E4M3 {
    type FromStrRadixErr = <f32 as Num>::FromStrRadixErr;

    #[inline]
    fn from_str_radix(str: &str, radix: u32) -> Result<Self, Self::FromStrRadixErr> {
        Ok(Self::from_f32(f32::from_str_radix(str, radix)?))
    }
}

impl One for F8E4M3 {
    #[inline]
    fn one() -> Self {
        Self::ONE
    }
}

impl Zero for F8E4M3 {
    #[inline]
    fn zero() -> Self {
        Self::ZERO
    }

    #[inline]
    fn is_zero(&self) -> bool {
        *self == Self::ZERO
    }
}

impl NumCast for F8E4M3 {
    #[inline]
    fn from<T: ToPrimitive>(n: T) -> Option<Self> {
        n.to_f32().map(Self::from_f32)
    }
}

impl num_traits::float::FloatCore for F8E4M3 {
    #[inline]
    fn infinity() -> Self {
        Self::INFINITY
    }

    #[inline]
    fn neg_infinity() -> Self {
        Self::NEG_INFINITY
    }

    #[inline]
    fn nan() -> Self {
        Self::NAN
    }

    #[inline]
    fn neg_zero() -> Self {
        Self::NEG_ZERO
    }

    #[inline]
    fn min_value() -> Self {
        Self::MIN
    }

    #[inline]
    fn min_positive_value() -> Self {
        Self::MIN_POSITIVE
    }

    #[inline]
    fn epsilon() -> Self {
        Self::EPSILON
    }

    #[inline]
    fn max_value() -> Self {
        Self::MAX
    }

    #[inline]
    fn is_nan(self) -> bool {
        Self::is_nan(&self)
    }

    #[inline]
    fn is_infinite(self) -> bool {
        Self::is_infinite(&self)
    }

    #[inline]
    fn is_finite(self) -> bool {
        Self::is_finite(&self)
    }

    #[inline]
    fn is_normal(self) -> bool {
        Self::is_normal(&self)
    }

    #[inline]
    fn classify(self) -> FpCategory {
        Self::classify(&self)
    }

    #[inline]
    fn floor(self) -> Self {
        Self::from_f32(self.to_f32().floor())
    }

    #[inline]
    fn ceil(self) -> Self {
        Self::from_f32(self.to_f32().ceil())
    }

    #[inline]
    fn round(self) -> Self {
        Self::from_f32(self.to_f32().round())
    }

    #[inline]
    fn trunc(self) -> Self {
        Self::from_f32(self.to_f32().trunc())
    }

    #[inline]
    fn fract(self) -> Self {
        Self::from_f32(self.to_f32().fract())
    }

    #[inline]
    fn abs(self) -> Self {
        Self::from_bits(self.to_bits() & 0x7F)
    }

    #[inline]
    fn signum(self) -> Self {
        self.signum()
    }

    #[inline]
    fn is_sign_positive(self) -> bool {
        Self::is_sign_positive(&self)
    }

    #[inline]
    fn is_sign_negative(self) -> bool {
        Self::is_sign_negative(&self)
    }

    fn min(self, other: Self) -> Self {
        match self.partial_cmp(&other) {
            None => {
                if self.is_nan() {
                    other
                } else {
                    self
                }
            }
            Some(Ordering::Greater) | Some(Ordering::Equal) => other,
            Some(Ordering::Less) => self,
        }
    }

    fn max(self, other: Self) -> Self {
        match self.partial_cmp(&other) {
            None => {
                if self.is_nan() {
                    other
                } else {
                    self
                }
            }
            Some(Ordering::Greater) | Some(Ordering::Equal) => self,
            Some(Ordering::Less) => other,
        }
    }

    #[inline]
    fn recip(self) -> Self {
        Self::from_f32(self.to_f32().recip())
    }

    #[inline]
    fn powi(self, exp: i32) -> Self {
        Self::from_f32(self.to_f32().powi(exp))
    }

    #[inline]
    fn to_degrees(self) -> Self {
        Self::from_f32(self.to_f32().to_degrees())
    }

    #[inline]
    fn to_radians(self) -> Self {
        Self::from_f32(self.to_f32().to_radians())
    }

    #[inline]
    fn integer_decode(self) -> (u64, i16, i8) {
        num_traits::float::FloatCore::integer_decode(self.to_f32())
    }
}

impl num_traits::float::Float for F8E4M3 {
    #[inline]
    fn nan() -> Self {
        Self::NAN
    }

    #[inline]
    fn infinity() -> Self {
        Self::INFINITY
    }

    #[inline]
    fn neg_infinity() -> Self {
        Self::NEG_INFINITY
    }

    #[inline]
    fn neg_zero() -> Self {
        Self::NEG_ZERO
    }

    #[inline]
    fn min_value() -> Self {
        Self::MIN
    }

    #[inline]
    fn min_positive_value() -> Self {
        Self::MIN_POSITIVE
    }

    #[inline]
    fn epsilon() -> Self {
        Self::EPSILON
    }

    #[inline]
    fn max_value() -> Self {
        Self::MAX
    }

    #[inline]
    fn is_nan(self) -> bool {
        Self::is_nan(&self)
    }

    #[inline]
    fn is_infinite(self) -> bool {
        Self::is_infinite(&self)
    }

    #[inline]
    fn is_finite(self) -> bool {
        Self::is_finite(&self)
    }

    #[inline]
    fn is_normal(self) -> bool {
        Self::is_normal(&self)
    }

    #[inline]
    fn classify(self) -> FpCategory {
        Self::classify(&self)
    }

    #[inline]
    fn floor(self) -> Self {
        Self::from_f32(self.to_f32().floor())
    }

    #[inline]
    fn ceil(self) -> Self {
        Self::from_f32(self.to_f32().ceil())
    }

    #[inline]
    fn round(self) -> Self {
        Self::from_f32(self.to_f32().round())
    }

    #[inline]
    fn trunc(self) -> Self {
        Self::from_f32(self.to_f32().trunc())
    }

    #[inline]
    fn fract(self) -> Self {
        Self::from_f32(self.to_f32().fract())
    }

    #[inline]
    fn abs(self) -> Self {
        Self::from_f32(self.to_f32().abs())
    }

    #[inline]
    fn signum(self) -> Self {
        Self::from_f32(self.to_f32().signum())
    }

    #[inline]
    fn is_sign_positive(self) -> bool {
        Self::is_sign_positive(&self)
    }

    #[inline]
    fn is_sign_negative(self) -> bool {
        Self::is_sign_negative(&self)
    }

    #[inline]
    fn mul_add(self, a: Self, b: Self) -> Self {
        Self::from_f32(self.to_f32().mul_add(a.to_f32(), b.to_f32()))
    }

    #[inline]
    fn recip(self) -> Self {
        Self::from_f32(self.to_f32().recip())
    }

    #[inline]
    fn powi(self, n: i32) -> Self {
        Self::from_f32(self.to_f32().powi(n))
    }

    #[inline]
    fn powf(self, n: Self) -> Self {
        Self::from_f32(self.to_f32().powf(n.to_f32()))
    }

    #[inline]
    fn sqrt(self) -> Self {
        Self::from_f32(self.to_f32().sqrt())
    }

    #[inline]
    fn exp(self) -> Self {
        Self::from_f32(self.to_f32().exp())
    }

    #[inline]
    fn exp2(self) -> Self {
        Self::from_f32(self.to_f32().exp2())
    }

    #[inline]
    fn ln(self) -> Self {
        Self::from_f32(self.to_f32().ln())
    }

    #[inline]
    fn log(self, base: Self) -> Self {
        Self::from_f32(self.to_f32().log(base.to_f32()))
    }

    #[inline]
    fn log2(self) -> Self {
        Self::from_f32(self.to_f32().log2())
    }

    #[inline]
    fn log10(self) -> Self {
        Self::from_f32(self.to_f32().log10())
    }

    #[inline]
    fn to_degrees(self) -> Self {
        Self::from_f32(self.to_f32().to_degrees())
    }

    #[inline]
    fn to_radians(self) -> Self {
        Self::from_f32(self.to_f32().to_radians())
    }

    #[inline]
    fn max(self, other: Self) -> Self {
        self.max(other)
    }

    #[inline]
    fn min(self, other: Self) -> Self {
        self.min(other)
    }

    #[inline]
    fn abs_sub(self, other: Self) -> Self {
        Self::from_f32((self.to_f32() - other.to_f32()).max(0.0))
    }

    #[inline]
    fn cbrt(self) -> Self {
        Self::from_f32(self.to_f32().cbrt())
    }

    #[inline]
    fn hypot(self, other: Self) -> Self {
        Self::from_f32(self.to_f32().hypot(other.to_f32()))
    }

    #[inline]
    fn sin(self) -> Self {
        Self::from_f32(self.to_f32().sin())
    }

    #[inline]
    fn cos(self) -> Self {
        Self::from_f32(self.to_f32().cos())
    }

    #[inline]
    fn tan(self) -> Self {
        Self::from_f32(self.to_f32().tan())
    }

    #[inline]
    fn asin(self) -> Self {
        Self::from_f32(self.to_f32().asin())
    }

    #[inline]
    fn acos(self) -> Self {
        Self::from_f32(self.to_f32().acos())
    }

    #[inline]
    fn atan(self) -> Self {
        Self::from_f32(self.to_f32().atan())
    }

    #[inline]
    fn atan2(self, other: Self) -> Self {
        Self::from_f32(self.to_f32().atan2(other.to_f32()))
    }

    #[inline]
    fn sin_cos(self) -> (Self, Self) {
        let (sin, cos) = self.to_f32().sin_cos();
        (Self::from_f32(sin), Self::from_f32(cos))
    }

    #[inline]
    fn exp_m1(self) -> Self {
        Self::from_f32(self.to_f32().exp_m1())
    }

    #[inline]
    fn ln_1p(self) -> Self {
        Self::from_f32(self.to_f32().ln_1p())
    }

    #[inline]
    fn sinh(self) -> Self {
        Self::from_f32(self.to_f32().sinh())
    }

    #[inline]
    fn cosh(self) -> Self {
        Self::from_f32(self.to_f32().cosh())
    }

    #[inline]
    fn tanh(self) -> Self {
        Self::from_f32(self.to_f32().tanh())
    }

    #[inline]
    fn asinh(self) -> Self {
        Self::from_f32(self.to_f32().asinh())
    }

    #[inline]
    fn acosh(self) -> Self {
        Self::from_f32(self.to_f32().acosh())
    }

    #[inline]
    fn atanh(self) -> Self {
        Self::from_f32(self.to_f32().atanh())
    }

    #[inline]
    fn integer_decode(self) -> (u64, i16, i8) {
        num_traits::float::Float::integer_decode(self.to_f32())
    }
}

impl FloatConst for F8E4M3 {
    #[inline]
    fn E() -> Self {
        Self::E
    }

    #[inline]
    fn FRAC_1_PI() -> Self {
        Self::FRAC_1_PI
    }

    #[inline]
    fn FRAC_1_SQRT_2() -> Self {
        Self::FRAC_1_SQRT_2
    }

    #[inline]
    fn FRAC_2_PI() -> Self {
        Self::FRAC_2_PI
    }

    #[inline]
    fn FRAC_2_SQRT_PI() -> Self {
        Self::FRAC_2_SQRT_PI
    }

    #[inline]
    fn FRAC_PI_2() -> Self {
        Self::FRAC_PI_2
    }

    #[inline]
    fn FRAC_PI_3() -> Self {
        Self::FRAC_PI_3
    }

    #[inline]
    fn FRAC_PI_4() -> Self {
        Self::FRAC_PI_4
    }

    #[inline]
    fn FRAC_PI_6() -> Self {
        Self::FRAC_PI_6
    }

    #[inline]
    fn FRAC_PI_8() -> Self {
        Self::FRAC_PI_8
    }

    #[inline]
    fn LN_10() -> Self {
        Self::LN_10
    }

    #[inline]
    fn LN_2() -> Self {
        Self::LN_2
    }

    #[inline]
    fn LOG10_E() -> Self {
        Self::LOG10_E
    }

    #[inline]
    fn LOG2_E() -> Self {
        Self::LOG2_E
    }

    #[inline]
    fn PI() -> Self {
        Self::PI
    }

    #[inline]
    fn SQRT_2() -> Self {
        Self::SQRT_2
    }

    #[inline]
    fn LOG10_2() -> Self
    where
        Self: Sized + Div<Self, Output = Self>,
    {
        Self::LOG10_2
    }

    #[inline]
    fn LOG2_10() -> Self
    where
        Self: Sized + Div<Self, Output = Self>,
    {
        Self::LOG2_10
    }
}

impl Bounded for F8E4M3 {
    #[inline]
    fn min_value() -> Self {
        F8E4M3::MIN
    }

    #[inline]
    fn max_value() -> Self {
        F8E4M3::MAX
    }
}

impl AsPrimitive<F8E4M3> for F8E4M3 {
    #[inline]
    fn as_(self) -> F8E4M3 {
        self
    }
}

macro_rules! impl_as_primitive_to_F8E4M3 {
    ($ty:ty, $meth:ident) => {
        impl AsPrimitive<$ty> for F8E4M3 {
            #[inline]
            fn as_(self) -> $ty {
                self.$meth().as_()
            }
        }
    };
}

impl_as_primitive_to_F8E4M3!(i64, to_f32);
impl_as_primitive_to_F8E4M3!(u64, to_f32);
impl_as_primitive_to_F8E4M3!(i8, to_f32);
impl_as_primitive_to_F8E4M3!(u8, to_f32);
impl_as_primitive_to_F8E4M3!(i16, to_f32);
impl_as_primitive_to_F8E4M3!(u16, to_f32);
impl_as_primitive_to_F8E4M3!(i32, to_f32);
impl_as_primitive_to_F8E4M3!(u32, to_f32);
impl_as_primitive_to_F8E4M3!(isize, to_f32);
impl_as_primitive_to_F8E4M3!(usize, to_f32);
impl_as_primitive_to_F8E4M3!(f32, to_f32);
impl_as_primitive_to_F8E4M3!(f64, to_f64);
impl_as_primitive_to_F8E4M3!(F8E5M2, to_f32);

macro_rules! impl_as_primitive_F8E4M3_from {
    ($ty:ty, $meth:ident) => {
        impl AsPrimitive<F8E4M3> for $ty {
            #[inline]
            fn as_(self) -> F8E4M3 {
                F8E4M3::$meth(self.as_())
            }
        }
    };
}

impl_as_primitive_F8E4M3_from!(i64, from_f32);
impl_as_primitive_F8E4M3_from!(u64, from_f32);
impl_as_primitive_F8E4M3_from!(i8, from_f32);
impl_as_primitive_F8E4M3_from!(u8, from_f32);
impl_as_primitive_F8E4M3_from!(i16, from_f32);
impl_as_primitive_F8E4M3_from!(u16, from_f32);
impl_as_primitive_F8E4M3_from!(i32, from_f32);
impl_as_primitive_F8E4M3_from!(u32, from_f32);
impl_as_primitive_F8E4M3_from!(isize, from_f32);
impl_as_primitive_F8E4M3_from!(usize, from_f32);
impl_as_primitive_F8E4M3_from!(f32, from_f32);
impl_as_primitive_F8E4M3_from!(f64, from_f64);
