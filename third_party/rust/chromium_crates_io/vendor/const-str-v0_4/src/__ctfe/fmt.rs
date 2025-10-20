#![allow(unsafe_code)]

use super::StrBuf;
use super::ToStr;

use crate::utf8::CharEscapeDebug;
use crate::utf8::CharEscapeDebugArgs;

#[derive(Clone, Copy)]
pub struct FmtSpec {
    pub alternate: bool,
}

pub struct Display<T>(pub T, pub FmtSpec);

macro_rules! delegate_display {
    ($($ty: ty,)+) => {
        $(
            impl Display<$ty> {
                pub const fn output_len(&self) -> usize {
                    ToStr(self.0).output_len()
                }

                pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                    ToStr(self.0).const_eval()
                }
            }
        )+
    };
}

delegate_display!(&str, char, bool, u8, u16, u32, u64, usize, i8, i16, i32, i64, isize,);

#[doc(hidden)]
#[macro_export]
macro_rules! __fmt_display {
    ($x: expr, $spec: expr) => {{
        const OUTPUT_LEN: usize = $crate::__ctfe::Display($x, $spec).output_len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> =
            $crate::__ctfe::Display($x, $spec).const_eval();
        OUTPUT_BUF.as_str()
    }};
}

pub struct Debug<T>(pub T, pub FmtSpec);

macro_rules! delegate_debug {
    ($($ty: ty,)+) => {
        $(
            impl Debug<$ty> {
                pub const fn output_len(&self) -> usize {
                    ToStr(self.0).output_len()
                }

                pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                    ToStr(self.0).const_eval()
                }
            }
        )+
    };
}

delegate_debug!(bool, u8, u16, u32, u64, usize, i8, i16, i32, i64, isize,);

impl Debug<char> {
    pub const fn output_len(&self) -> usize {
        let escape = CharEscapeDebug::new(
            self.0,
            CharEscapeDebugArgs {
                escape_single_quote: true,
                escape_double_quote: false,
            },
        );

        escape.as_bytes().len() + 2
    }

    pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
        let mut buf = [0; N];
        let mut pos = 0;

        macro_rules! push {
            ($x: expr) => {{
                buf[pos] = $x;
                pos += 1;
            }};
        }

        push!(b'\'');
        {
            let e = CharEscapeDebug::new(
                self.0,
                CharEscapeDebugArgs {
                    escape_single_quote: true,
                    escape_double_quote: false,
                },
            );
            let bytes = e.as_bytes();
            let mut i = 0;
            while i < bytes.len() {
                push!(bytes[i]);
                i += 1;
            }
        }
        push!(b'\'');

        constfn_assert!(pos == N);

        unsafe { StrBuf::new_unchecked(buf) }
    }
}

impl Debug<&str> {
    pub const fn output_len(&self) -> usize {
        let mut s = self.0.as_bytes();
        let mut ans = 2;
        while let Some((ch, count)) = crate::utf8::next_char(s) {
            s = crate::bytes::advance(s, count);
            let e = CharEscapeDebug::new(
                ch,
                CharEscapeDebugArgs {
                    escape_single_quote: false,
                    escape_double_quote: true,
                },
            );
            ans += e.as_bytes().len()
        }
        ans
    }

    pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
        let mut buf = [0; N];
        let mut pos = 0;

        macro_rules! push {
            ($x: expr) => {{
                buf[pos] = $x;
                pos += 1;
            }};
        }

        push!(b'"');

        let mut s = self.0.as_bytes();
        while let Some((ch, count)) = crate::utf8::next_char(s) {
            s = crate::bytes::advance(s, count);
            let e = CharEscapeDebug::new(
                ch,
                CharEscapeDebugArgs {
                    escape_single_quote: false,
                    escape_double_quote: true,
                },
            );
            let bytes = e.as_bytes();
            let mut i = 0;
            while i < bytes.len() {
                push!(bytes[i]);
                i += 1;
            }
        }

        push!(b'"');

        constfn_assert!(pos == N);

        unsafe { StrBuf::new_unchecked(buf) }
    }
}

#[doc(hidden)]
#[macro_export]
macro_rules! __fmt_debug {
    ($x: expr, $spec: expr) => {{
        const OUTPUT_LEN: usize = $crate::__ctfe::Debug($x, $spec).output_len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> =
            $crate::__ctfe::Debug($x, $spec).const_eval();
        OUTPUT_BUF.as_str()
    }};
}

struct Hex<T>(T, FmtSpec, bool);

pub struct LowerHex<T>(pub T, pub FmtSpec);
pub struct UpperHex<T>(pub T, pub FmtSpec);

macro_rules! impl_integer_hex {
    ($unsigned: ty, $signed: ty) => {
        impl Hex<$unsigned> {
            const fn output_len(&self) -> usize {
                let mut x = self.0;
                let mut ans = 0;
                loop {
                    ans += 1;
                    x /= 16;
                    if x == 0 {
                        break;
                    }
                }
                if self.1.alternate {
                    ans += 2;
                }
                ans
            }

            const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                let mut buf = [0; N];
                let mut pos = 0;
                let mut x = self.0;
                loop {
                    let d = crate::ascii::num_to_hex_digit((x % 16) as u8);
                    buf[pos] = if self.2 { d.to_ascii_uppercase() } else { d };
                    pos += 1;
                    x /= 16;
                    if x == 0 {
                        break;
                    }
                }
                if self.1.alternate {
                    buf[pos] = b'x';
                    pos += 1;
                    buf[pos] = b'0';
                    pos += 1;
                }
                constfn_assert!(pos == N);
                let buf = crate::bytes::reversed(buf);
                unsafe { StrBuf::new_unchecked(buf) }
            }
        }

        impl LowerHex<$unsigned> {
            pub const fn output_len(&self) -> usize {
                let h = Hex(self.0, self.1, false);
                h.output_len()
            }

            pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                let h = Hex(self.0, self.1, false);
                h.const_eval()
            }
        }

        impl UpperHex<$unsigned> {
            pub const fn output_len(&self) -> usize {
                let h = Hex(self.0, self.1, true);
                h.output_len()
            }

            pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                let h = Hex(self.0, self.1, true);
                h.const_eval()
            }
        }

        impl LowerHex<$signed> {
            pub const fn output_len(&self) -> usize {
                let h = Hex(self.0 as $unsigned, self.1, false);
                h.output_len()
            }

            pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                let h = Hex(self.0 as $unsigned, self.1, false);
                h.const_eval()
            }
        }

        impl UpperHex<$signed> {
            pub const fn output_len(&self) -> usize {
                let h = Hex(self.0 as $unsigned, self.1, true);
                h.output_len()
            }

            pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                let h = Hex(self.0 as $unsigned, self.1, true);
                h.const_eval()
            }
        }
    };
}

impl_integer_hex!(u8, i8);
impl_integer_hex!(u16, i16);
impl_integer_hex!(u32, i32);
impl_integer_hex!(u64, i64);
impl_integer_hex!(u128, i128);
impl_integer_hex!(usize, isize);

#[doc(hidden)]
#[macro_export]
macro_rules! __fmt_lowerhex {
    ($x: expr, $spec: expr) => {{
        const OUTPUT_LEN: usize = $crate::__ctfe::LowerHex($x, $spec).output_len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> =
            $crate::__ctfe::LowerHex($x, $spec).const_eval();
        OUTPUT_BUF.as_str()
    }};
}

#[doc(hidden)]
#[macro_export]
macro_rules! __fmt_upperhex {
    ($x: expr, $spec: expr) => {{
        const OUTPUT_LEN: usize = $crate::__ctfe::UpperHex($x, $spec).output_len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> =
            $crate::__ctfe::UpperHex($x, $spec).const_eval();
        OUTPUT_BUF.as_str()
    }};
}

pub struct Binary<T>(pub T, pub FmtSpec);

macro_rules! impl_integer_binary {
    ($unsigned: ty, $signed: ty) => {
        impl Binary<$unsigned> {
            pub const fn output_len(&self) -> usize {
                let mut x = self.0;
                let mut ans = 0;
                loop {
                    ans += 1;
                    x /= 2;
                    if x == 0 {
                        break;
                    }
                }
                if self.1.alternate {
                    ans += 2;
                }
                ans
            }

            pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                let mut buf = [0; N];
                let mut pos = 0;
                let mut x = self.0;
                loop {
                    buf[pos] = b'0' + (x % 2) as u8;
                    pos += 1;
                    x /= 2;
                    if x == 0 {
                        break;
                    }
                }
                if self.1.alternate {
                    buf[pos] = b'b';
                    pos += 1;
                    buf[pos] = b'0';
                    pos += 1;
                }
                constfn_assert!(pos == N);
                let buf = crate::bytes::reversed(buf);
                unsafe { StrBuf::new_unchecked(buf) }
            }
        }

        impl Binary<$signed> {
            pub const fn output_len(&self) -> usize {
                let b = Binary(self.0 as $unsigned, self.1);
                b.output_len()
            }

            pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
                let b = Binary(self.0 as $unsigned, self.1);
                b.const_eval()
            }
        }
    };
}

impl_integer_binary!(u8, i8);
impl_integer_binary!(u16, i16);
impl_integer_binary!(u32, i32);
impl_integer_binary!(u64, i64);
impl_integer_binary!(u128, i128);
impl_integer_binary!(usize, isize);

#[doc(hidden)]
#[macro_export]
macro_rules! __fmt_binary {
    ($x: expr, $spec: expr) => {{
        const OUTPUT_LEN: usize = $crate::__ctfe::Binary($x, $spec).output_len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> =
            $crate::__ctfe::Binary($x, $spec).const_eval();
        OUTPUT_BUF.as_str()
    }};
}
