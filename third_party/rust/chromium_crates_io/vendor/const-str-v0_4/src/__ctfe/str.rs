#![allow(unsafe_code)]

pub struct StrBuf<const N: usize>([u8; N]);

impl<const N: usize> StrBuf<N> {
    /// # Safety
    /// `buf` must contain valid utf-8 bytes.
    pub const unsafe fn new_unchecked(buf: [u8; N]) -> Self {
        #[cfg(debug_assertions)]
        {
            constfn_assert!(crate::utf8::run_utf8_validation(&buf).is_ok());
        }
        Self(buf)
    }

    // const since 1.55
    pub const fn as_str(&self) -> &str {
        unsafe { core::str::from_utf8_unchecked(&self.0) }
    }

    pub const fn as_bytes(&self) -> &[u8] {
        &self.0
    }

    pub const fn from_str(s: &str) -> Self {
        let buf = crate::bytes::clone::<N>(s.as_bytes());
        unsafe { Self::new_unchecked(buf) }
    }
}

pub const fn from_utf8<const N: usize>(v: &[u8]) -> StrBuf<N> {
    constfn_assert!(v.len() == N);

    match crate::utf8::run_utf8_validation(v) {
        Ok(()) => {
            let mut buf = [0; N];
            let mut i = 0;
            while i < v.len() {
                buf[i] = v[i];
                i += 1;
            }
            unsafe { StrBuf::new_unchecked(buf) }
        }
        Err(_) => {
            constfn_panic!("invalid utf-8 sequence")
        }
    }
}

/// Converts a byte string to a string slice
///
/// # Examples
/// ```
/// const BYTE_PATH: &[u8] = b"/tmp/file";
/// const PATH: &str = const_str::from_utf8!(BYTE_PATH);
///
/// assert_eq!(PATH, "/tmp/file");
/// ```
///
#[macro_export]
macro_rules! from_utf8 {
    ($s: expr) => {{
        use ::core::primitive::{str, u8, usize};
        const INPUT: &[u8] = $s;
        const N: usize = INPUT.len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<N> = $crate::__ctfe::from_utf8(INPUT);
        OUTPUT_BUF.as_str()
    }};
}
