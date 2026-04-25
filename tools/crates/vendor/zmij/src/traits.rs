use core::fmt::Display;
use core::ops::{Add, BitAnd, BitOr, BitOrAssign, BitXorAssign, Div, Mul, Shl, Shr, Sub};

pub trait Float: Copy {
    type UInt: UInt;
    const MANTISSA_DIGITS: u32;
    const MAX_DIGITS10: u32;
    fn to_bits(self) -> Self::UInt;
}

impl Float for f32 {
    type UInt = u32;
    const MANTISSA_DIGITS: u32 = Self::MANTISSA_DIGITS;
    const MAX_DIGITS10: u32 = 9;
    fn to_bits(self) -> Self::UInt {
        self.to_bits()
    }
}

impl Float for f64 {
    type UInt = u64;
    const MANTISSA_DIGITS: u32 = Self::MANTISSA_DIGITS;
    const MAX_DIGITS10: u32 = 17;
    fn to_bits(self) -> Self::UInt {
        self.to_bits()
    }
}

pub trait UInt:
    Copy
    + From<u8>
    + From<bool>
    + Add<Output = Self>
    + Sub<Output = Self>
    + Mul<Output = Self>
    + Div<Output = Self>
    + BitAnd<Output = Self>
    + BitOr<Output = Self>
    + Shl<i32, Output = Self>
    + Shl<u32, Output = Self>
    + Shr<i32, Output = Self>
    + Shr<u32, Output = Self>
    + BitOrAssign
    + BitXorAssign
    + PartialOrd
    + Into<u64>
    + Display
{
}

impl UInt for u32 {}
impl UInt for u64 {}
