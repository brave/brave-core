#![allow(unsafe_code)]

use super::str::StrBuf;

pub struct Replace<I, P, O>(pub I, pub P, pub O);

impl<'input, 'from, 'to> Replace<&'input str, &'from str, &'to str> {
    pub const fn output_len(&self) -> usize {
        let Self(mut input, replace_from, replace_to) = *self;

        if replace_from.is_empty() {
            let input_chars = crate::utf8::str_count_chars(self.0);
            input.len() + (input_chars + 1) * replace_to.len()
        } else {
            let mut ans = 0;
            while let Some((pos, remain)) = crate::str::next_match(input, replace_from) {
                ans += pos + replace_to.len();
                input = remain;
            }
            ans += input.len();
            ans
        }
    }

    pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
        let Self(input, replace_from, replace_to) = *self;

        let mut buf = [0; N];
        let mut pos = 0;

        macro_rules! push {
            ($x: expr) => {{
                buf[pos] = $x;
                pos += 1;
            }};
        }

        if replace_from.is_empty() {
            let mut input = input.as_bytes();
            let replace_to = replace_to.as_bytes();
            loop {
                let mut k = 0;
                while k < replace_to.len() {
                    push!(replace_to[k]);
                    k += 1;
                }

                let count = match crate::utf8::next_char(input) {
                    Some((_, count)) => count,
                    None => break,
                };

                let mut i = 0;
                while i < count {
                    push!(input[i]);
                    i += 1;
                }

                input = crate::bytes::advance(input, count);
            }
        } else {
            let mut input = input;
            let replace_to = replace_to.as_bytes();

            while let Some((pos, remain)) = crate::str::next_match(input, replace_from) {
                let mut i = 0;
                while i < pos {
                    push!(input.as_bytes()[i]);
                    i += 1;
                }
                let mut k = 0;
                while k < replace_to.len() {
                    push!(replace_to[k]);
                    k += 1;
                }
                input = remain;
            }

            let input = input.as_bytes();
            let mut i = 0;
            while i < input.len() {
                push!(input[i]);
                i += 1;
            }
        }

        constfn_assert!(pos == N);
        unsafe { StrBuf::new_unchecked(buf) }
    }
}

#[test]
fn test_replace() {
    macro_rules! test_replace_str {
        ($input: expr, $replace_from: expr, $replace_to: expr) => {{
            const INPUT: &str = $input;
            const REPLACE_FROM: &str = $replace_from;
            const REPLACE_TO: &str = $replace_to;

            const CONSTFN: Replace<&str, &str, &str> = Replace(INPUT, REPLACE_FROM, REPLACE_TO);
            const OUTPUT_LEN: usize = CONSTFN.output_len();

            let ans = INPUT.replace(REPLACE_FROM, REPLACE_TO);
            assert_eq!(OUTPUT_LEN, ans.len());

            let output_buf = CONSTFN.const_eval::<OUTPUT_LEN>();
            let output = output_buf.as_str();
            assert_eq!(output, ans);
        }};
    }

    test_replace_str!("", "", "");
    test_replace_str!("", "", "a");
    test_replace_str!("", "a", "");
    test_replace_str!("", "a", "b");
    test_replace_str!("a", "", "b");
    test_replace_str!("asd", "", "b");
    test_replace_str!("aba", "a", "c");
    test_replace_str!("this is old", "old", "new");
    test_replace_str!("我", "", "1");
    test_replace_str!("我", "", "我");
    test_replace_str!("aaaa", "aa", "bb");
    test_replace_str!("run / v4", " ", "");
    test_replace_str!("token", " ", "");
    test_replace_str!("v4 / udp", " ", "");
    test_replace_str!("v4 / upnp", "p", "");
}

/// Replaces all matches of a pattern with another string slice.
///
/// See [`str::replace`](https://doc.rust-lang.org/std/primitive.str.html#method.replace).
///
/// # Examples
///
/// ```
/// assert_eq!("this is new", const_str::replace!("this is old", "old", "new"));
/// ```
///
#[macro_export]
macro_rules! replace {
    ($s: expr, $from: expr, $to: expr) => {{
        const OUTPUT_LEN: usize = $crate::__ctfe::Replace($s, $from, $to).output_len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> =
            $crate::__ctfe::Replace($s, $from, $to).const_eval();
        OUTPUT_BUF.as_str()
    }};
}
