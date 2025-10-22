pub use const_str_proc_macro::{regex_assert_match, verified_regex};

/// Returns a compile-time verified regex string literal.
///
/// # Examples
///
/// ```
/// use regex::Regex;
/// let re = const_str::verified_regex!(r"^\d{4}-\d{2}-\d{2}$");
/// assert!(Regex::new(re).is_ok());
/// ```
///
#[cfg_attr(docsrs, doc(cfg(feature = "regex")))]
#[macro_export]
macro_rules! verified_regex {
    ($re: literal) => {
        $crate::__proc::verified_regex!($re)
    };
}

/// Asserts that the string literal matches the pattern.
///
/// # Examples
/// ```
/// const_str::regex_assert_match!(r"^\d{4}-\d{2}-\d{2}$", "2014-01-01");
/// ```
#[cfg_attr(docsrs, doc(cfg(feature = "regex")))]
#[macro_export]
macro_rules! regex_assert_match {
    ($re: literal, $text: literal) => {
        $crate::__proc::regex_assert_match!($re, $text)
    };
}
