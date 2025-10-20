#![allow(unsafe_code)]

pub struct ToCStr<T>(pub T);

impl ToCStr<&str> {
    const fn check_nul(&self) {
        let bytes = self.0.as_bytes();
        let mut i = 0;
        while i < bytes.len() {
            constfn_assert!(bytes[i] != 0);
            i += 1;
        }
    }

    pub const fn output_len(&self) -> usize {
        self.check_nul();
        self.0.as_bytes().len() + 1
    }

    pub const fn const_eval<const N: usize>(&self) -> [u8; N] {
        let mut buf = [0; N];
        let mut pos = 0;
        let bytes = self.0.as_bytes();
        let mut i = 0;
        while i < bytes.len() {
            constfn_assert!(bytes[i] != 0);
            buf[pos] = bytes[i];
            pos += 1;
            i += 1;
        }
        pos += 1;
        constfn_assert!(pos == N);
        buf
    }
}

/// Converts a string slice to [`*const c_char`](std::os::raw::c_char).
///
/// The C-style string is guaranteed to be terminated by a nul byte.
/// This trailing nul byte will be appended by this macro.
/// The provided data should not contain any nul bytes in it.
/// 
/// # Examples
///
/// ```
/// use std::os::raw::c_char;
/// const PRINTF_FMT: *const c_char = const_str::raw_cstr!("%d\n");
/// ```
#[cfg_attr(docsrs, doc(cfg(feature = "std")))]
#[macro_export]
macro_rules! raw_cstr {
    ($s: expr) => {{
        const OUTPUT_LEN: ::core::primitive::usize = $crate::__ctfe::ToCStr($s).output_len();
        const OUTPUT_BUF: [u8; OUTPUT_LEN] = $crate::__ctfe::ToCStr($s).const_eval();
        const OUTPUT: *const ::std::os::raw::c_char = OUTPUT_BUF.as_ptr().cast();
        OUTPUT
    }};
}

#[test]
fn test_raw_cstr() {
    const FMT: &str = "%d\n";
    let fmt = raw_cstr!(FMT);
    let len = FMT.len() + 1;
    let bytes: &[u8] = unsafe { core::slice::from_raw_parts(fmt.cast(), len) };
    assert_eq!(bytes, b"%d\n\0");
}
