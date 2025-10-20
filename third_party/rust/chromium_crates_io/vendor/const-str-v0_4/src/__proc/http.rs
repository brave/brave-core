pub use const_str_proc_macro::verified_header_name;

/// Returns a compile-time verified header name string literal.
///
/// # Examples
///
/// ```
/// use http::header::HeaderName;
/// let name = const_str::verified_header_name!("content-md5");
/// assert_eq!(HeaderName::from_static(name).as_str(), "content-md5");
/// ```
///
#[cfg_attr(docsrs, doc(cfg(feature = "http")))]
#[macro_export]
macro_rules! verified_header_name {
    ($name: literal) => {
        $crate::__proc::verified_header_name!($name)
    };
}
