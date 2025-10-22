#![allow(unsafe_code)]

use super::StrBuf;

pub struct Concat<'a>(pub &'a [&'a str]);

impl<'a> Concat<'a> {
    pub const fn output_len(&self) -> usize {
        let mut ans = 0;
        let mut iter = self.0;
        while let [x, xs @ ..] = iter {
            ans += x.len();
            iter = xs;
        }
        ans
    }

    pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
        let mut buf = [0; N];
        let mut pos = 0;

        let mut iter = self.0;
        while let [x, xs @ ..] = iter {
            let x = x.as_bytes();
            let mut i = 0;
            while i < x.len() {
                buf[pos] = x[i];
                pos += 1;
                i += 1;
            }
            iter = xs;
        }
        constfn_assert!(pos == N);

        unsafe { StrBuf::new_unchecked(buf) }
    }
}

/// Concatenates values into a string slice.
///
/// The input type must be one of
///
/// + [`&str`]
/// + [`char`]
/// + [`bool`]
/// + [`u8`], [`u16`], [`u32`], [`u64`], [`u128`], [`usize`]
/// + [`i8`], [`i16`], [`i32`], [`i64`], [`i128`], [`isize`]
///
///
/// # Examples
///
/// ```
/// const PROMPT: &str = "The answer is";
/// const ANSWER: usize = 42;
/// const MESSAGE: &str = const_str::concat!(PROMPT, " ", ANSWER);
///
/// assert_eq!(MESSAGE, "The answer is 42");
/// ```
///
#[macro_export]
macro_rules! concat {
    ($($x: expr),+ $(,)?) => {{
        const STRS: &[&str] = &[$( $crate::to_str!($x) ),+];
        const OUTPUT_LEN: usize = $crate::__ctfe::Concat(STRS).output_len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> = $crate::__ctfe::Concat(STRS).const_eval();
        OUTPUT_BUF.as_str()
    }}
}
