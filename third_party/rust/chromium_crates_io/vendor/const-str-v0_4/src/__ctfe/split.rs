pub struct Split<T, P>(pub T, pub P);

impl<'input, 'pat> Split<&'input str, &'pat str> {
    pub const fn output_len(&self) -> usize {
        let Self(mut input, pat) = *self;

        if pat.is_empty() {
            crate::utf8::str_count_chars(input) + 2
        } else {
            let mut ans = 1;
            while let Some((_, remain)) = crate::str::next_match(input, pat) {
                ans += 1;
                input = remain
            }
            ans
        }
    }

    #[allow(unsafe_code)]
    pub const fn const_eval<const N: usize>(&self) -> [&str; N] {
        let Self(mut input, pat) = *self;

        let mut buf: [&str; N] = [""; N];
        let mut pos = 0;

        if pat.is_empty() {
            let mut input = input.as_bytes();

            {
                buf[pos] =
                    unsafe { core::str::from_utf8_unchecked(crate::bytes::subslice(input, 0..0)) };
                pos += 1;
            }

            while let Some((_, count)) = crate::utf8::next_char(input) {
                buf[pos] = unsafe {
                    core::str::from_utf8_unchecked(crate::bytes::subslice(input, 0..count))
                };
                pos += 1;
                input = crate::bytes::advance(input, count);
            }

            {
                buf[pos] =
                    unsafe { core::str::from_utf8_unchecked(crate::bytes::subslice(input, 0..0)) };
                pos += 1;
            }
        } else {
            while let Some((m, remain)) = crate::str::next_match(input, pat) {
                let substr = crate::bytes::subslice(input.as_bytes(), 0..m);
                buf[pos] = unsafe { core::str::from_utf8_unchecked(substr) };
                pos += 1;
                input = remain;
            }
            buf[pos] = input;
            pos += 1;
        }
        constfn_assert!(pos == N);
        buf
    }
}

#[test]
fn test_split() {
    macro_rules! test_split_str {
        ($input: expr, $pat: expr) => {{
            const INPUT: &str = $input;
            const PATTERN: &str = $pat;

            const CONSTFN: Split<&str, &str> = Split(INPUT, PATTERN);
            const OUTPUT_LEN: usize = CONSTFN.output_len();

            let ans = INPUT.split(PATTERN).collect::<Vec<_>>();
            assert_eq!(OUTPUT_LEN, ans.len(), "ans = {:?}", ans);

            let output = CONSTFN.const_eval::<OUTPUT_LEN>();
            assert_eq!(output.as_slice(), &*ans);
        }};
    }

    test_split_str!("", "");
    test_split_str!("a中1😂1!", "");
    test_split_str!("a中1😂1!", "a");
    test_split_str!("a中1😂1!", "中");
    test_split_str!("a中1😂1!", "1");
    test_split_str!("a中1😂1!", "😂");
    test_split_str!("a中1😂1!", "!");
    test_split_str!("11111", "1");
    test_split_str!("222", "22");
    test_split_str!("啊哈哈哈", "哈哈");
    test_split_str!("some string:another string", ":")
}

/// Returns an array of substrings of a string slice, separated by characters matched by a pattern.
///
/// See [`str::split`](https://doc.rust-lang.org/std/primitive.str.html#method.split).
///
/// # Examples
///
/// ```
/// const SEPARATOR: &str = ", ";
/// const TEXT: &str = "lion, tiger, leopard";
///
/// const ANIMALS_ARRAY: [&str;3] = const_str::split!(TEXT, SEPARATOR);
/// const ANIMALS_SLICE: &[&str] = &const_str::split!(TEXT, SEPARATOR);
///
/// assert_eq!(ANIMALS_ARRAY, ANIMALS_SLICE);
/// assert_eq!(ANIMALS_SLICE, &["lion", "tiger", "leopard"]);
/// ```
#[macro_export]
macro_rules! split {
    ($s: expr, $pat: expr) => {{
        const OUTPUT_LEN: usize = $crate::__ctfe::Split($s, $pat).output_len();
        const OUTPUT_BUF: [&str; OUTPUT_LEN] = $crate::__ctfe::Split($s, $pat).const_eval();
        OUTPUT_BUF
    }};
}
