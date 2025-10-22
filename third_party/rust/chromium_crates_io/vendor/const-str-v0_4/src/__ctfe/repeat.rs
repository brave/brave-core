#![allow(unsafe_code)]

use super::StrBuf;

pub struct Repeat<T>(pub T, pub usize);

impl Repeat<&str> {
    pub const fn const_eval<const N: usize>(&self) -> StrBuf<N> {
        let buf = bytes_repeat(self.0.as_bytes(), self.1);
        unsafe { StrBuf::new_unchecked(buf) }
    }
}

const fn bytes_repeat<const N: usize>(bytes: &[u8], n: usize) -> [u8; N] {
    constfn_assert!(bytes.len().checked_mul(n).is_some());
    constfn_assert!(bytes.len() * n == N);
    let mut buf = [0; N];
    let mut i = 0;
    let mut j = 0;
    while i < n {
        let mut k = 0;
        while k < bytes.len() {
            buf[j] = bytes[k];
            j += 1;
            k += 1;
        }
        i += 1;
    }
    buf
}

/// Creates a new string slice by repeating a string slice n times.
///
/// # Examples
///
/// ```
/// const S: &str = "abc";
/// const SSSS: &str = const_str::repeat!(S, 4);
/// assert_eq!(SSSS, "abcabcabcabc");
/// ```
///
#[macro_export]
macro_rules! repeat {
    ($s: expr, $n: expr) => {{
        const INPUT: &str = $s;
        const N: usize = $n;
        const OUTPUT_LEN: usize = INPUT.len() * N;
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> =
            $crate::__ctfe::Repeat(INPUT, N).const_eval();
        OUTPUT_BUF.as_str()
    }};
}
