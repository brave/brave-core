// SPDX-License-Identifier: MIT

//! GF32 - Galois Field over 32 elements.
//!
//! Implements GF32 arithmetic, defined and encoded as in [BIP-173] "bech32".
//!
//! > A finite field is a finite set which is a field; this means that multiplication, addition,
//! > subtraction and division (excluding division by zero) are defined and satisfy the rules of
//! > arithmetic known as the field axioms.
//!
//! ref: <https://en.wikipedia.org/wiki/Finite_field>
//!
//! [BIP-173]: <https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki>

use core::convert::{Infallible, TryFrom};
use core::{fmt, num, ops};

#[cfg(all(test, mutate))]
use mutagen::mutate;

use crate::error::write_err;

/// Logarithm table of each bech32 element, as a power of alpha = Z.
///
/// Includes Q as 0 but this is false; you need to exclude Q because it has no discrete log. If we
/// could have a 1-indexed array that would panic on a 0 index that would be better.
#[rustfmt::skip]
const LOG: [isize; 32] = [
     0,  0,  1, 14,  2, 28, 15, 22,
     3,  5, 29, 26, 16,  7, 23, 11,
     4, 25,  6, 10, 30, 13, 27, 21,
    17, 18,  8, 19, 24,  9, 12, 20,
];

/// Mapping of powers of 2 to the numeric value of the element.
#[rustfmt::skip]
const LOG_INV: [u8; 31] = [
     1,  2,  4,  8, 16,  9, 18, 13,
    26, 29, 19, 15, 30, 21,  3,  6,
    12, 24, 25, 27, 31, 23,  7, 14,
    28, 17, 11, 22,  5, 10, 20,
];

/// Mapping from numeric value to bech32 character.
#[rustfmt::skip]
const CHARS_LOWER: [char; 32] = [
    'q', 'p', 'z', 'r', 'y', '9', 'x', '8', //  +0
    'g', 'f', '2', 't', 'v', 'd', 'w', '0', //  +8
    's', '3', 'j', 'n', '5', '4', 'k', 'h', // +16
    'c', 'e', '6', 'm', 'u', 'a', '7', 'l', // +24
];

/// Mapping from bech32 character (either case) to numeric value.
///
/// E.g., 'z' is CHARS_LOWER[2] and is ASCII value 122 so CHARS_INV[122] == 2
#[rustfmt::skip]
const CHARS_INV: [i8; 128] = [
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
];

/// An element in GF(32), the finite field containing elements `[0,31]` inclusive.
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
#[repr(transparent)]
pub struct Fe32(pub(crate) u8);

impl Fe32 {
    // These are a little gratuitous for a reference implementation, but it makes me happy to do it.
    /// Numeric value maps to bech32 character: 0 == "q".
    pub const Q: Fe32 = Fe32(0);
    /// Numeric value maps to bech32 character: 1 == "p".
    pub const P: Fe32 = Fe32(1);
    /// Numeric value maps to bech32 character: 2 == "z".
    pub const Z: Fe32 = Fe32(2);
    /// Numeric value maps to bech32 character: 3 == "r".
    pub const R: Fe32 = Fe32(3);
    /// Numeric value maps to bech32 character: 4 == "y".
    pub const Y: Fe32 = Fe32(4);
    /// Numeric value maps to bech32 character: 5 == "9".
    pub const _9: Fe32 = Fe32(5);
    /// Numeric value maps to bech32 character: 6 == "x".
    pub const X: Fe32 = Fe32(6);
    /// Numeric value maps to bech32 character: 7 == "8".
    pub const _8: Fe32 = Fe32(7);
    /// Numeric value maps to bech32 character: 8 == "g".
    pub const G: Fe32 = Fe32(8);
    /// Numeric value maps to bech32 character: 9 == "f".
    pub const F: Fe32 = Fe32(9);
    /// Numeric value maps to bech32 character: 10 == "2".
    pub const _2: Fe32 = Fe32(10);
    /// Numeric value maps to bech32 character: 11 == "t".
    pub const T: Fe32 = Fe32(11);
    /// Numeric value maps to bech32 character: 12 == "v".
    pub const V: Fe32 = Fe32(12);
    /// Numeric value maps to bech32 character: 13 == "d".
    pub const D: Fe32 = Fe32(13);
    /// Numeric value maps to bech32 character: 14 == "w".
    pub const W: Fe32 = Fe32(14);
    /// Numeric value maps to bech32 character: 15 == "0".
    pub const _0: Fe32 = Fe32(15);
    /// Numeric value maps to bech32 character: 16 == "s".
    pub const S: Fe32 = Fe32(16);
    /// Numeric value maps to bech32 character: 17 == "3".
    pub const _3: Fe32 = Fe32(17);
    /// Numeric value maps to bech32 character: 18 == "j".
    pub const J: Fe32 = Fe32(18);
    /// Numeric value maps to bech32 character: 19 == "n".
    pub const N: Fe32 = Fe32(19);
    /// Numeric value maps to bech32 character: 20 == "5".
    pub const _5: Fe32 = Fe32(20);
    /// Numeric value maps to bech32 character: 21 == "4".
    pub const _4: Fe32 = Fe32(21);
    /// Numeric value maps to bech32 character: 22 == "k".
    pub const K: Fe32 = Fe32(22);
    /// Numeric value maps to bech32 character: 23 == "h".
    pub const H: Fe32 = Fe32(23);
    /// Numeric value maps to bech32 character: 24 == "c".
    pub const C: Fe32 = Fe32(24);
    /// Numeric value maps to bech32 character: 25 == "e".
    pub const E: Fe32 = Fe32(25);
    /// Numeric value maps to bech32 character: 26 == "6".
    pub const _6: Fe32 = Fe32(26);
    /// Numeric value maps to bech32 character: 27 == "m".
    pub const M: Fe32 = Fe32(27);
    /// Numeric value maps to bech32 character: 28 == "u".
    pub const U: Fe32 = Fe32(28);
    /// Numeric value maps to bech32 character: 29 == "a".
    pub const A: Fe32 = Fe32(29);
    /// Numeric value maps to bech32 character: 30 == "7".
    pub const _7: Fe32 = Fe32(30);
    /// Numeric value maps to bech32 character: 31 == "l".
    pub const L: Fe32 = Fe32(31);

    /// Iterator over all field elements, in alphabetical order.
    #[inline]
    pub fn iter_alpha() -> impl Iterator<Item = Fe32> {
        [
            Fe32::A,
            Fe32::C,
            Fe32::D,
            Fe32::E,
            Fe32::F,
            Fe32::G,
            Fe32::H,
            Fe32::J,
            Fe32::K,
            Fe32::L,
            Fe32::M,
            Fe32::N,
            Fe32::P,
            Fe32::Q,
            Fe32::R,
            Fe32::S,
            Fe32::T,
            Fe32::U,
            Fe32::V,
            Fe32::W,
            Fe32::X,
            Fe32::Y,
            Fe32::Z,
            Fe32::_0,
            Fe32::_2,
            Fe32::_3,
            Fe32::_4,
            Fe32::_5,
            Fe32::_6,
            Fe32::_7,
            Fe32::_8,
            Fe32::_9,
        ]
        .iter()
        .copied()
    }

    /// Creates a field element from a single bech32 character.
    ///
    /// # Errors
    ///
    /// If the input char is not part of the bech32 alphabet.
    #[inline]
    pub fn from_char(c: char) -> Result<Fe32, FromCharError> {
        use FromCharError::*;

        // i8::try_from gets a value in the range 0..=127 since char is unsigned.
        let byte = i8::try_from(u32::from(c)).map_err(|_| NotAscii(c))?;
        // Now we have a valid ASCII value cast is safe.
        let ascii = byte as usize;
        // We use -1 for any array element that is an invalid char to trigger error from u8::try_from
        let u5 = u8::try_from(CHARS_INV[ascii]).map_err(|_| Invalid(c))?;

        Ok(Fe32(u5))
    }

    /// Creates a field element from a single bech32 character.
    ///
    /// # Panics
    ///
    /// If the input character is not part of the bech32 alphabet.
    pub fn from_char_unchecked(c: u8) -> Fe32 { Fe32(CHARS_INV[usize::from(c)] as u8) }

    /// Converts the field element to a lowercase bech32 character.
    #[inline]
    pub fn to_char(self) -> char {
        // Indexing fine as we have self.0 in [0, 32) as an invariant.
        CHARS_LOWER[usize::from(self.0)]
    }

    /// Converts the field element to a 5-bit u8, with bits representing the coefficients
    /// of the polynomial representation.
    #[inline]
    pub fn to_u8(self) -> u8 { self.0 }

    fn _add(self, other: Fe32) -> Fe32 { Fe32(self.0 ^ other.0) }

    // Subtraction is the same as addition in a char-2 field.
    fn _sub(self, other: Fe32) -> Fe32 { self + other }

    #[cfg_attr(all(test, mutate), mutate)]
    fn _mul(self, other: Fe32) -> Fe32 {
        if self.0 == 0 || other.0 == 0 {
            Fe32(0)
        } else {
            let log1 = LOG[self.0 as usize];
            let log2 = LOG[other.0 as usize];
            Fe32(LOG_INV[((log1 + log2) % 31) as usize])
        }
    }

    #[cfg_attr(all(test, mutate), mutate)]
    fn _div(self, other: Fe32) -> Fe32 {
        if self.0 == 0 {
            Fe32(0)
        } else if other.0 == 0 {
            panic!("Attempt to divide {} by 0 in GF32", self);
        } else {
            let log1 = LOG[self.0 as usize];
            let log2 = LOG[other.0 as usize];
            Fe32(LOG_INV[((31 + log1 - log2) % 31) as usize])
        }
    }
}

impl fmt::Display for Fe32 {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result { fmt::Display::fmt(&self.to_char(), f) }
}

impl From<Fe32> for u8 {
    #[inline]
    fn from(v: Fe32) -> u8 { v.0 }
}

macro_rules! impl_try_from {
    ($($ty:ident)+) => {
        $(
            impl TryFrom<$ty> for Fe32 {
                type Error = TryFromError;

                /// Tries to create an [`Fe32`] type from a signed source number type.
                ///
                /// # Errors
                ///
                /// Returns an error if `value` is outside of the range of an `Fe32`.
                #[inline]
                fn try_from(value: $ty) -> Result<Self, Self::Error> {
                    let byte = u8::try_from(value)?;
                    if byte > 31 {
                        Err(TryFromError::InvalidByte(byte))?;
                    }
                    Ok(Fe32(byte))
                }
            }
        )+
    }
}
impl_try_from!(u8 u16 u32 u64 u128 i8 i16 i32 i64 i128);

impl AsRef<u8> for Fe32 {
    #[inline]
    fn as_ref(&self) -> &u8 { &self.0 }
}

/// Implements $op for the 2x2 matrix of type by ref to type
macro_rules! impl_op_matrix {
    ($op:ident, $op_fn:ident, $call_fn:ident) => {
        impl ops::$op<Fe32> for Fe32 {
            type Output = Fe32;
            #[inline]
            fn $op_fn(self, other: Fe32) -> Fe32 { self.$call_fn(other) }
        }

        impl ops::$op<Fe32> for &Fe32 {
            type Output = Fe32;
            #[inline]
            fn $op_fn(self, other: Fe32) -> Fe32 { self.$call_fn(other) }
        }

        impl ops::$op<&Fe32> for Fe32 {
            type Output = Fe32;
            #[inline]
            fn $op_fn(self, other: &Fe32) -> Fe32 { self.$call_fn(*other) }
        }

        impl ops::$op<&Fe32> for &Fe32 {
            type Output = Fe32;
            #[inline]
            fn $op_fn(self, other: &Fe32) -> Fe32 { self.$call_fn(*other) }
        }
    };
}
impl_op_matrix!(Add, add, _add);
impl_op_matrix!(Sub, sub, _sub);
impl_op_matrix!(Mul, mul, _mul);
impl_op_matrix!(Div, div, _div);

impl ops::AddAssign for Fe32 {
    #[inline]
    fn add_assign(&mut self, other: Fe32) { *self = *self + other; }
}

impl ops::SubAssign for Fe32 {
    #[inline]
    fn sub_assign(&mut self, other: Fe32) { *self = *self - other; }
}

impl ops::MulAssign for Fe32 {
    #[inline]
    fn mul_assign(&mut self, other: Fe32) { *self = *self * other; }
}

impl ops::DivAssign for Fe32 {
    #[inline]
    fn div_assign(&mut self, other: Fe32) { *self = *self / other; }
}

/// A galois field error when converting from a character.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[non_exhaustive]
pub enum FromCharError {
    /// Tried to interpret a character as a GF32 element but it is not an ASCII character.
    NotAscii(char),
    /// Tried to interpret a character as a GF32 element but it is not part of the bech32 character set.
    Invalid(char),
}

impl fmt::Display for FromCharError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use FromCharError::*;

        match *self {
            NotAscii(c) => write!(f, "non-ascii char in field element: {}", c),
            Invalid(c) => write!(f, "invalid char in field element: {}", c),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for FromCharError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        use FromCharError::*;

        match *self {
            NotAscii(_) | Invalid(_) => None,
        }
    }
}

/// A galois field error when converting from an integer.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[non_exhaustive]
pub enum TryFromError {
    /// Tried to interpret an integer as a GF32 element but it could not be converted to an u8.
    NotAByte(num::TryFromIntError),
    /// Tried to interpret a byte as a GF32 element but its numeric value was outside of [0, 32).
    InvalidByte(u8),
}

impl fmt::Display for TryFromError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use TryFromError::*;

        match *self {
            NotAByte(ref e) => write_err!(f, "invalid field element"; e),
            InvalidByte(ref b) => write!(f, "invalid byte in field element: {:#04x}", b),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for TryFromError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        use TryFromError::*;

        match *self {
            NotAByte(ref e) => Some(e),
            InvalidByte(_) => None,
        }
    }
}

impl From<num::TryFromIntError> for TryFromError {
    #[inline]
    fn from(e: num::TryFromIntError) -> Self { Self::NotAByte(e) }
}

impl From<Infallible> for TryFromError {
    #[inline]
    fn from(i: Infallible) -> Self { match i {} }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn numeric_string() {
        let s: String = (0..32).map(Fe32).map(Fe32::to_char).collect();
        assert_eq!(s, "qpzry9x8gf2tvdw0s3jn54khce6mua7l");
    }

    // For what a "translation wheel" is refer to the codex32 book:
    // https://github.com/BlockstreamResearch/codex32/blob/master/SSS32.ps
    #[test]
    fn translation_wheel() {
        // 1. Produce the translation wheel by multiplying
        let logbase = Fe32(20);
        let mut init = Fe32(1);
        let mut s = String::new();
        for _ in 0..31 {
            s.push(init.to_char());
            init *= logbase;
        }
        // Can be verified against the multiplication disk, starting with P and moving clockwise
        assert_eq!(s, "p529kt3uw8hlmecvxr470na6djfsgyz");

        // 2. By dividing
        let logbase = Fe32(20);
        let mut init = Fe32(1);
        let mut s = String::new();
        for _ in 0..31 {
            s.push(init.to_char());
            init /= logbase;
        }
        // Same deal, but counterclockwise
        assert_eq!(s, "pzygsfjd6an074rxvcemlh8wu3tk925");
    }

    // For what a "recovery wheel" is refer to the codex32 book:
    // https://github.com/BlockstreamResearch/codex32/blob/master/SSS32.ps
    #[test]
    fn recovery_wheel() {
        // Remarkably, the recovery wheel can be produced in the same way as the
        // multiplication wheel, though with a different log base and with every
        // element added by S.
        //
        // We spent quite some time deriving this, but honestly we probably could've
        // just guessed it if we'd known a priori that a wheel existed.
        let logbase = Fe32(10);
        let mut init = Fe32(1);
        let mut s = String::new();
        for _ in 0..31 {
            s.push((init + Fe32(16)).to_char());
            init *= logbase;
        }
        // To verify, start with 3 and move clockwise on the Recovery Wheel
        assert_eq!(s, "36xp78tgk9ldaecjy4mvh0funwr2zq5");
    }

    #[test]
    fn reverse_charset() {
        fn get_char_value(c: char) -> i8 {
            let charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
            match charset.find(c.to_ascii_lowercase()) {
                Some(x) => x as i8,
                None => -1,
            }
        }

        let expected_rev_charset =
            (0u8..128).map(|i| get_char_value(i as char)).collect::<Vec<_>>();

        assert_eq!(&(CHARS_INV[..]), expected_rev_charset.as_slice());
    }

    #[test]
    fn from_char() {
        for c in &CHARS_LOWER[..] {
            assert!(Fe32::from_char(*c).is_ok())
        }
    }

    #[test]
    fn from_upper_char() {
        let lower = Fe32::from_char('q').expect("failed to create fe32 from lowercase ascii char");
        let upper = Fe32::from_char('Q').expect("failed to create fe32 from uppercase ascii char");

        assert_eq!(lower, upper);
    }

    #[test]
    fn mul_zero() {
        for c in &CHARS_LOWER[..] {
            let fe = Fe32::from_char(*c).unwrap();
            assert_eq!(fe._mul(Fe32::Q), Fe32::Q) // Fe32::Q == Fe32(0)
        }
    }

    #[test]
    #[should_panic]
    fn div_zero() {
        let _ = Fe32::P / Fe32::Q; // Fe32::Q == Fe32(0)
    }

    #[test]
    fn div_self_zero() {
        let fe = Fe32::Z; // Value of Z not meaningful to the test.
        assert_eq!(Fe32::Q / fe, Fe32::Q) // Fe32::Q == Fe32(0)
    }

    #[test]
    fn mul_one() {
        for c in &CHARS_LOWER[..] {
            let fe = Fe32::from_char(*c).unwrap();
            assert_eq!(fe * Fe32::P, fe) // Fe32::P == Fe32(1)
        }
    }
}

#[cfg(kani)]
mod verification {
    use super::*;

    #[kani::proof]
    fn check_char_conversion() {
        let any: char = kani::any();
        // Checks that we can pass any char to from_char and not cause a panic ... I think.
        if let Ok(fe) = Fe32::from_char(any) {
            let got = fe.to_char();
            let want = any.to_ascii_lowercase();
            assert_eq!(got, want);
        }
    }
}
