//! Macro for opaque [`Debug`] trait implementation.
//!
//! In many cases it's convenient to have `Debug` implementation for all crate types,
//! e.g. to allow deriving of `Debug` in user-defined structs. But at the same time, using
//! the default derive macro can be a security hazard since it cause leaking of sensitive
//! information, for example, through uncareful logging.
//!
//! This crate introduces the [`implement!`] macro which creates an opaque [`Debug`]
//! implementation, which does not expose any internal type data.
//!
//! # Examples
//! ```
//! pub struct CryptoStuff {
//!     key: [u8; 16],
//! }
//!
//! opaque_debug::implement!(CryptoStuff);
//!
//! let val = CryptoStuff { key: [42; 16] };
//! assert_eq!(format!("{:?}", val), "CryptoStuff { ... }")
//! ```
//!
//! The macro also support generic paramters:
//! ```
//! pub struct GenricCryptoStuff<K> {
//!     key: K,
//! }
//!
//! opaque_debug::implement!(GenricCryptoStuff<K>);
//!
//! let val = GenricCryptoStuff { key: [42u8; 16] };
//! assert_eq!(format!("{:?}", val), "GenricCryptoStuff<[u8; 16]> { ... }")
//! ```
#![no_std]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/RustCrypto/media/6ee8e381/logo.svg",
    html_favicon_url = "https://raw.githubusercontent.com/RustCrypto/media/6ee8e381/logo.svg"
)]

#[doc(hidden)]
pub extern crate core as __core;

#[macro_export]
#[doc(hidden)]
macro_rules! format_params {
    ($single:ident) => {
        "{}"
    };
    ($first:ident, $($rest:ident),+) => {
        concat!("{}", ", ", $crate::format_params!($($rest),+))
    };
}

/// Macro for implementing an opaque `Debug` implementation.
#[macro_export]
macro_rules! implement {
    ($struct:ident <$($params:ident),+>) => {
        impl <$($params),+> $crate::__core::fmt::Debug for $struct <$($params),+> {
            fn fmt(
                &self,
                f: &mut $crate::__core::fmt::Formatter,
            ) -> Result<(), $crate::__core::fmt::Error> {
                write!(
                    f,
                    concat!(stringify!($struct), "<", $crate::format_params!($($params),+), "> {{ ... }}"),
                    $($crate::__core::any::type_name::<$params>()),+
                )
            }
        }
    };
    ($struct:ty) => {
        impl $crate::__core::fmt::Debug for $struct {
            fn fmt(
                &self,
                f: &mut $crate::__core::fmt::Formatter,
            ) -> Result<(), $crate::__core::fmt::Error> {
                write!(f, concat!(stringify!($struct), " {{ ... }}"))
            }
        }
    };
}
