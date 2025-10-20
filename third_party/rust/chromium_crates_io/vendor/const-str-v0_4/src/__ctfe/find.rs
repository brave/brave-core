use crate::utf8::CharEncodeUtf8;

pub struct Contains<'a, P>(pub &'a str, pub P);

impl<'a, 'b> Contains<'a, &'b str> {
    pub const fn const_eval(&self) -> bool {
        crate::str::contains(self.0, self.1)
    }
}

impl<'a> Contains<'a, char> {
    pub const fn const_eval(&self) -> bool {
        let haystack = self.0.as_bytes();
        let ch = CharEncodeUtf8::new(self.1);
        let needle = ch.as_bytes();
        crate::bytes::contains(haystack, needle)
    }
}

/// Returns [`true`] if the given pattern matches a sub-slice of this string slice.
///
/// Returns [`false`] if it does not.
///
/// The pattern type must be one of
///
/// + [`&str`]
/// + [`char`]
///
/// # Examples
///
/// ```
/// const BANANAS: &str = "bananas";
/// const A: bool = const_str::contains!(BANANAS, "nana");
/// const B: bool = const_str::contains!(BANANAS, "apples");
/// const C: bool = const_str::contains!(BANANAS, 'c');
/// assert_eq!([A, B, C], [true, false, false]);
/// ```
///
#[macro_export]
macro_rules! contains {
    ($haystack: expr, $pattern: expr) => {{
        $crate::__ctfe::Contains($haystack, $pattern).const_eval()
    }};
}

pub struct StartsWith<'a, P>(pub &'a str, pub P);

impl<'a, 'b> StartsWith<'a, &'b str> {
    pub const fn const_eval(&self) -> bool {
        crate::str::starts_with(self.0, self.1)
    }
}

impl<'a> StartsWith<'a, char> {
    pub const fn const_eval(&self) -> bool {
        let haystack = self.0.as_bytes();
        let ch = CharEncodeUtf8::new(self.1);
        let needle = ch.as_bytes();
        crate::bytes::starts_with(haystack, needle)
    }
}

/// Returns [`true`] if the given pattern matches a prefix of this string slice.
///
/// Returns [`false`] if it does not.
///
/// The pattern type must be one of
///
/// + [`&str`]
/// + [`char`]
///
/// # Examples
///
/// ```
/// const BANANAS: &str = "bananas";
/// const A: bool = const_str::starts_with!(BANANAS, "bana");
/// const B: bool = const_str::starts_with!(BANANAS, "nana");
/// const C: bool = const_str::starts_with!(BANANAS, 'b');
/// assert_eq!([A, B, C], [true, false, true]);
/// ```
///
#[macro_export]
macro_rules! starts_with {
    ($haystack: expr, $pattern: expr) => {{
        $crate::__ctfe::StartsWith($haystack, $pattern).const_eval()
    }};
}

pub struct EndsWith<'a, P>(pub &'a str, pub P);

impl<'a, 'b> EndsWith<'a, &'b str> {
    pub const fn const_eval(&self) -> bool {
        crate::str::ends_with(self.0, self.1)
    }
}

impl<'a> EndsWith<'a, char> {
    pub const fn const_eval(&self) -> bool {
        let haystack = self.0.as_bytes();
        let ch = CharEncodeUtf8::new(self.1);
        let needle = ch.as_bytes();
        crate::bytes::ends_with(haystack, needle)
    }
}

/// Returns [`true`] if the given pattern matches a suffix of this string slice.
///
/// Returns [`false`] if it does not.
///
/// The pattern type must be one of
///
/// + [`&str`]
/// + [`char`]
///
/// # Examples
///
/// ```
/// const BANANAS: &str = "bananas";
/// const A: bool = const_str::ends_with!(BANANAS, "anas");
/// const B: bool = const_str::ends_with!(BANANAS, "nana");
/// const C: bool = const_str::ends_with!(BANANAS, 's');
/// assert_eq!([A, B, C], [true, false, true]);
/// ```
///
#[macro_export]
macro_rules! ends_with {
    ($haystack: expr, $pattern: expr) => {{
        $crate::__ctfe::EndsWith($haystack, $pattern).const_eval()
    }};
}
