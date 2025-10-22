pub use const_str_proc_macro::format_parts;

/// Creates a string slice using interpolation of const expressions.
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
/// use const_str::format as const_format;
///
/// const PROMPT: &str = "The answer is";
/// const ANSWER: usize = 42;
///
/// const MESSAGE_1: &str = const_format!("{PROMPT} {ANSWER}");
/// const MESSAGE_2: &str = const_format!("{} {}", PROMPT, ANSWER);
/// const MESSAGE_3: &str = const_format!("{0} {1}", PROMPT, ANSWER);
/// const MESSAGE_4: &str = const_format!("{a} {b}", a = PROMPT, b = ANSWER);
/// const MESSAGE_5: &str = const_format!("{} {a}", PROMPT, a = ANSWER);
///
/// assert_eq!(MESSAGE_1, "The answer is 42");
/// assert_eq!(MESSAGE_1, MESSAGE_2);
/// assert_eq!(MESSAGE_1, MESSAGE_3);
/// assert_eq!(MESSAGE_1, MESSAGE_4);
/// assert_eq!(MESSAGE_1, MESSAGE_5);
/// ```
///
#[cfg_attr(docsrs, doc(cfg(feature = "proc")))]
#[macro_export]
macro_rules! format {
    ($fmt: literal $($args:tt)*) => {{
        use ::core::primitive::{str, usize};
        use $crate::__ctfe::FmtSpec;
        #[allow(unused_imports)]
        use $crate::{__fmt_debug, __fmt_display, __fmt_lowerhex, __fmt_upperhex, __fmt_binary};
        const STRS: &[&str] = $crate::__proc::format_parts!($fmt $($args)*);
        const OUTPUT_LEN: usize = $crate::__ctfe::Concat(STRS).output_len();
        const OUTPUT_BUF: $crate::__ctfe::StrBuf<OUTPUT_LEN> = $crate::__ctfe::Concat(STRS).const_eval();
        OUTPUT_BUF.as_str()
    }};
}

#[test]
fn test_const_format() {
    use crate::format as const_format;

    {
        const A: usize = 1;
        const X: &str = const_format!("{:?}", A);
        let ans = std::format!("{:?}", A);
        assert_eq!(X, ans);
    }

    {
        const A: bool = true;
        const B: bool = false;
        const X: &str = const_format!("{1:?} {0:?} {:?}", A, B);
        let ans = std::format!("{1:?} {0:?} {:?}", A, B);
        assert_eq!(X, ans);
    }

    {
        const A: char = '我';
        const X: &str = const_format!("{a:?} {0}", A, a = A);
        let ans = std::format!("{a:?} {0}", A, a = A);
        assert_eq!(X, ans);
    }

    {
        const A: &str = "团长\0\t\r\n\"'and希望之花";
        const X: &str = const_format!("{:?}", A);
        let ans = format!("{:?}", A);
        assert_eq!(X, ans)
    }

    {
        const A: u32 = 42;
        const X: &str = const_format!("{0:x} {0:X} {0:#x} {0:#X} {0:b} {0:#b}", A);
        let ans = std::format!("{0:x} {0:X} {0:#x} {0:#X} {0:b} {0:#b}", A);
        assert_eq!(X, ans)
    }

    {
        const A: i32 = -42;
        const X: &str = const_format!("{A:x} {A:X} {A:#x} {A:#X} {A:b} {A:#b}");
        let ans = std::format!("{0:x} {0:X} {0:#x} {0:#X} {0:b} {0:#b}", A);
        assert_eq!(X, ans)
    }
}
