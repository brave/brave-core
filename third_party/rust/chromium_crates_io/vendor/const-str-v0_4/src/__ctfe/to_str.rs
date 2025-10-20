#![allow(unsafe_code)]

use super::str::StrBuf;
use crate::utf8::CharEncodeUtf8;

pub struct ToStr<T>(pub T);

impl ToStr<&str> {
    pub const fn output_len(&self) -> usize {
        self.0.len()
    }

    pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
        StrBuf::from_str(self.0)
    }
}

impl ToStr<bool> {
    const fn bool_to_str(b: bool) -> &'static str {
        if b {
            "true"
        } else {
            "false"
        }
    }

    pub const fn output_len(&self) -> usize {
        Self::bool_to_str(self.0).len()
    }

    pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
        let bytes: &[u8] = Self::bool_to_str(self.0).as_bytes();
        let buf = crate::bytes::merge([0; N], bytes);
        unsafe { StrBuf::new_unchecked(buf) }
    }
}

impl ToStr<char> {
    pub const fn output_len(&self) -> usize {
        self.0.len_utf8()
    }

    pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
        let ch = CharEncodeUtf8::new(self.0);
        let buf = crate::bytes::merge([0; N], ch.as_bytes());
        unsafe { StrBuf::new_unchecked(buf) }
    }
}

macro_rules! impl_integer_to_str {
    ($unsigned: ty, $signed: ty) => {
        impl ToStr<$unsigned> {
            pub const fn output_len(&self) -> usize {
                let mut x = self.0;
                let mut ans = 1;
                while x > 9 {
                    ans += 1;
                    x /= 10;
                }
                ans
            }

            pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                let mut buf = [0; N];
                let mut pos = 0;
                let mut x = self.0;
                loop {
                    buf[pos] = b'0' + (x % 10) as u8;
                    pos += 1;
                    x /= 10;
                    if x == 0 {
                        break;
                    }
                }
                constfn_assert!(pos == N);
                let buf = crate::bytes::reversed(buf);
                unsafe { StrBuf::new_unchecked(buf) }
            }
        }

        impl ToStr<$signed> {
            pub const fn output_len(&self) -> usize {
                let x = self.0;
                let abs_len = ToStr(x.unsigned_abs()).output_len();
                abs_len + (x < 0) as usize
            }

            pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                let mut buf = [0; N];
                let mut pos = 0;

                let mut x = self.0.unsigned_abs();

                loop {
                    buf[pos] = b'0' + (x % 10) as u8;
                    pos += 1;
                    x /= 10;
                    if x == 0 {
                        break;
                    }
                }

                if self.0 < 0 {
                    buf[pos] = b'-';
                    pos += 1;
                }

                constfn_assert!(pos == N);
                let buf = crate::bytes::reversed(buf);
                unsafe { StrBuf::new_unchecked(buf) }
            }
        }
    };
}

impl_integer_to_str!(u8, i8);
impl_integer_to_str!(u16, i16);
impl_integer_to_str!(u32, i32);
impl_integer_to_str!(u64, i64);
impl_integer_to_str!(u128, i128);
impl_integer_to_str!(usize, isize);

#[test]
fn test_to_str() {
    macro_rules! test_to_str {
        ($ty: ty, $x: expr) => {{
            const X: $ty = $x;
            const OUTPUT_LEN: usize = ToStr(X).output_len();
            const OUTPUT_BUF: StrBuf<OUTPUT_LEN> = ToStr(X).const_eval();

            let output = OUTPUT_BUF.as_str();
            let ans = X.to_string();
            assert_eq!(OUTPUT_LEN, ans.len());
            assert_eq!(output, ans);
        }};
    }

    test_to_str!(&str, "lovelive superstar");

    test_to_str!(bool, true);
    test_to_str!(bool, false);

    test_to_str!(char, '鲤');
    test_to_str!(char, '鱼');

    test_to_str!(u8, 0);
    test_to_str!(u16, 0);
    test_to_str!(u32, 0);
    test_to_str!(u64, 0);
    test_to_str!(u128, 0);

    test_to_str!(u8, 10);
    test_to_str!(u8, 128);
    test_to_str!(u8, u8::MAX);

    test_to_str!(u64, 1);
    test_to_str!(u64, 10);
    test_to_str!(u64, 42);
    test_to_str!(u64, u64::MAX);

    test_to_str!(u128, u128::MAX);

    test_to_str!(i8, 0);
    test_to_str!(i16, 0);
    test_to_str!(i32, 0);
    test_to_str!(i64, 0);
    test_to_str!(i128, 0);

    test_to_str!(i8, -10);
    test_to_str!(i8, -42);
    test_to_str!(i8, i8::MAX);
    test_to_str!(i8, i8::MIN);

    test_to_str!(i64, 1);
    test_to_str!(i64, 10);
    test_to_str!(i64, -42);
    test_to_str!(i64, i64::MAX);
    test_to_str!(i64, i64::MIN);

    test_to_str!(i128, i128::MAX);
    test_to_str!(i128, i128::MIN);
}

/// Converts a value to a string slice.
///
/// The input type must be one of
///
/// + [`&str`]
/// + [`char`]
/// + [`bool`]
/// + [`u8`], [`u16`], [`u32`], [`u64`], [`u128`], [`usize`]
/// + [`i8`], [`i16`], [`i32`], [`i64`], [`i128`], [`isize`]
///
/// # Examples
///
/// ```
/// const A: &str = const_str::to_str!("A");
/// assert_eq!(A, "A");
///
/// const B: &str = const_str::to_str!('我');
/// assert_eq!(B, "我");
///
/// const C: &str = const_str::to_str!(true);
/// assert_eq!(C, "true");
///
/// const D: &str = const_str::to_str!(1_u8 + 1);
/// assert_eq!(D, "2");
///
/// const E: &str = const_str::to_str!(-21_i32 * 2);
/// assert_eq!(E, "-42")
/// ```
///
#[macro_export]
macro_rules! to_str {
    ($x: expr) => {{
        const OUTPUT_LEN: usize = $crate::__ctfe::ToStr($x).output_len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> =
            $crate::__ctfe::ToStr($x).const_eval();
        OUTPUT_BUF.as_str()
    }};
}
