//! Macros for data-encoding
//!
//! This library provides macros to define compile-time byte arrays from encoded strings (using
//! common bases like [base64], [base32], or [hexadecimal], and also custom bases). It also provides
//! a macro to define compile-time custom encodings to be used with the [data-encoding] crate at
//! run-time.
//!
//! Up to Rust 1.50, you may need to add the following to your `.cargo/config.toml` to use this
//! library in no-std or no-alloc environments:
//!
//! ```toml
//! [unstable]
//! features = ["host_dep"]
//! ```
//!
//! From Rust 1.51, you may need to add the following to your `Cargo.toml`:
//!
//! ```toml
//! [package]
//! resolver = "2"
//! ```
//!
//! # Examples
//!
//! You can define a compile-time byte slice from an encoded string literal:
//!
//! ```rust
//! const HELLO_SLICE: &'static [u8] = &data_encoding_macro::hexlower!("68656c6c6f");
//! const FOOBAR_SLICE: &'static [u8] = &data_encoding_macro::base64!("Zm9vYmFy");
//! # fn main() {}
//! ```
//!
//! You can also define a compile-time byte array from an encoded string literal:
//!
//! ```rust
//! data_encoding_macro::hexlower_array!("const HELLO" = "68656c6c6f");
//! data_encoding_macro::base64_array!("const FOOBAR" = "Zm9vYmFy");
//! # fn main() {}
//! ```
//!
//! You can define a compile-time custom encoding from its specification:
//!
//! ```rust
//! const HEX: data_encoding::Encoding = data_encoding_macro::new_encoding! {
//!     symbols: "0123456789abcdef",
//!     translate_from: "ABCDEF",
//!     translate_to: "abcdef",
//! };
//! const BASE64: data_encoding::Encoding = data_encoding_macro::new_encoding! {
//!     symbols: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
//!     padding: '=',
//! };
//! # fn main() {}
//! ```
//!
//! [base32]: macro.base32.html
//! [base64]: macro.base64.html
//! [data-encoding]: https://crates.io/crates/data-encoding
//! [hexadecimal]: macro.hexlower_permissive.html

#![no_std]
#![warn(unused_results)]

#[doc(hidden)]
pub use data_encoding_macro_internal::{
    internal_decode_array, internal_decode_slice, internal_new_encoding,
};

/// Defines a compile-time byte array by decoding a string literal
///
/// This macro takes a list of `key: value,` pairs (the last comma is required). It takes the
/// key-value pairs specifying the encoding to use to decode the input (see [new_encoding] for the
/// possible key-value pairs), the input itself keyed by `input`, and the output keyed by `name`.
/// The output must be of the form `[pub] {const|static} <name>`.
///
/// # Examples
///
/// ```rust
/// data_encoding_macro::decode_array! {
///     name: "const OCTAL",
///     symbols: "01234567",
///     padding: '=',
///     input: "237610==",
/// }
/// # fn main() {}
/// ```
///
/// [new_encoding]: macro.new_encoding.html
#[macro_export]
macro_rules! decode_array {
    ($($arg: tt)*) => {
        $crate::internal_decode_array!($($arg)*);
    };
}

/// Defines a compile-time byte slice by decoding a string literal
///
/// This macro takes a list of `key: value,` pairs (the last comma is required). It takes the
/// key-value pairs specifying the encoding to use to decode the input (see [new_encoding] for the
/// possible key-value pairs), the input itself keyed by `input`, and the output keyed by `name`.
///
/// # Examples
///
/// ```rust
/// const OCTAL: &'static [u8] = &data_encoding_macro::decode_slice! {
///     symbols: "01234567",
///     padding: '=',
///     input: "237610==",
/// };
/// # fn main() {}
/// ```
///
/// [new_encoding]: macro.new_encoding.html
#[macro_export]
macro_rules! decode_slice {
    ($($arg: tt)*) => {
        $crate::internal_decode_slice!($($arg)*)
    };
}

/// Defines a compile-time custom encoding
///
/// This macro takes a list of `key: value,` pairs (the last comma is required). The possible
/// key-value pairs are:
///
/// ```text
///             symbols: <string>,       // e.g. "01234567"
///             padding: [None]|<char>,  // e.g. '='
///           bit_order: [MostSignificantFirst]|LeastSignificantFirst,
/// check_trailing_bits: [true]|false,
///              ignore: [""]|<string>,  // e.g. " \t\n"
///          wrap_width: [0]|<int>,      // e.g. 76
///      wrap_separator: [""]|<string>,  // e.g. "\r\n"
///      translate_from: [""]|<string>,  // e.g. "ABCDEF"
///        translate_to: [""]|<string>,  // e.g. "abcdef"
/// ```
///
/// Only `symbols` is required. Everything else is optional and defaults to the value between square
/// brackets.
///
/// # Examples
///
/// ```rust
/// const HEX: data_encoding::Encoding = data_encoding_macro::new_encoding! {
///     symbols: "0123456789abcdef",
///     ignore: " \r\t\n",
///     wrap_width: 32,
///     wrap_separator: "\n",
///     translate_from: "ABCDEF",
///     translate_to: "abcdef",
/// };
/// # fn main() {}
/// ```
#[macro_export]
macro_rules! new_encoding {
    ($($arg: tt)*) => {
        data_encoding::Encoding::internal_new(&$crate::internal_new_encoding!{ $($arg)* })
    };
}

macro_rules! make {
    ($base: ident $base_array: ident = $ref: ident; $($spec: tt)*) => {
        #[macro_export]
        macro_rules! $base_array {
            ($n: tt = $x: tt) => {
                $crate::decode_array!(name: $n, input: $x, $($spec)*);
            };
        }
        #[macro_export]
        macro_rules! $base {
            ($x: tt) => {
                $crate::decode_slice!(input: $x, $($spec)*)
            };
        }
        #[test]
        fn $base() {
            assert_eq!(new_encoding!($($spec)*), data_encoding::$ref);
        }
    };
}

make! {
    hexlower hexlower_array = HEXLOWER;
    symbols: "0123456789abcdef",
}
make! {
    hexlower_permissive hexlower_permissive_array = HEXLOWER_PERMISSIVE;
    symbols: "0123456789abcdef",
    translate_from: "ABCDEF",
    translate_to: "abcdef",
}
make! {
    hexupper hexupper_array = HEXUPPER;
    symbols: "0123456789ABCDEF",
}
make! {
    hexupper_permissive hexupper_permissive_array = HEXUPPER_PERMISSIVE;
    symbols: "0123456789ABCDEF",
    translate_from: "abcdef",
    translate_to: "ABCDEF",
}
make! {
    base32 base32_array = BASE32;
    symbols: "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567",
    padding: '=',
}
make! {
    base32_nopad base32_nopad_array = BASE32_NOPAD;
    symbols: "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567",
}
make! {
    base32hex base32hex_array = BASE32HEX;
    symbols: "0123456789ABCDEFGHIJKLMNOPQRSTUV",
    padding: '=',
}
make! {
    base32hex_nopad base32hex_nopad_array = BASE32HEX_NOPAD;
    symbols: "0123456789ABCDEFGHIJKLMNOPQRSTUV",
}
make! {
    base32_dnssec base32_dnssec_array = BASE32_DNSSEC;
    symbols: "0123456789abcdefghijklmnopqrstuv",
    translate_from: "ABCDEFGHIJKLMNOPQRSTUV",
    translate_to: "abcdefghijklmnopqrstuv",
}
make! {
    base32_dnscurve base32_dnscurve_array = BASE32_DNSCURVE;
    symbols: "0123456789bcdfghjklmnpqrstuvwxyz",
    bit_order: LeastSignificantFirst,
    translate_from: "BCDFGHJKLMNPQRSTUVWXYZ",
    translate_to: "bcdfghjklmnpqrstuvwxyz",
}
make! {
    base64 base64_array = BASE64;
    symbols: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
    padding: '=',
}
make! {
    base64_nopad base64_nopad_array = BASE64_NOPAD;
    symbols: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
}
make! {
    base64_mime base64_mime_array = BASE64_MIME;
    symbols: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
    padding: '=',
    wrap_width: 76,
    wrap_separator: "\r\n",
}
make! {
    base64_mime_permissive base64_mime_permissive_array = BASE64_MIME_PERMISSIVE;
    symbols: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
    padding: '=',
    wrap_width: 76,
    wrap_separator: "\r\n",
    check_trailing_bits: false,
}
make! {
    base64url base64url_array = BASE64URL;
    symbols: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_",
    padding: '=',
}
make! {
    base64url_nopad base64url_nopad_array = BASE64URL_NOPAD;
    symbols: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_",
}
