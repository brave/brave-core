//! > winnow, making parsing a breeze
//!
//! `winnow` is a parser combinator library
//!
//! Quick links:
//! - [List of combinators][crate::combinator]
//! - [Tutorial][_tutorial::chapter_0]
//! - [Special Topics][_topic]
//! - [Discussions](https://github.com/winnow-rs/winnow/discussions)
//! - [CHANGELOG](https://github.com/winnow-rs/winnow/blob/v0.6.20/CHANGELOG.md) (includes major version migration
//!   guides)
//!
//! ## Aspirations
//!
//! `winnow` aims to be your "do everything" parser, much like people treat regular expressions.
//!
//! In roughly priority order:
//! 1. Support writing parser declaratively while not getting in the way of imperative-style
//!    parsing when needed, working as an open-ended toolbox rather than a close-ended framework.
//! 2. Flexible enough to be used for any application, including parsing binary data, strings, or
//!    separate lexing and parsing phases
//! 3. Zero-cost abstractions, making it easy to write high performance parsers
//! 4. Easy to use, making it trivial for one-off uses
//!
//! In addition:
//! - Resilient maintainership, including
//!   - Willing to break compatibility rather than batching up breaking changes in large releases
//!   - Leverage feature flags to keep one active branch
//! - We will support the last 6 months of rust releases (MSRV, currently 1.64.0)
//!
//! See also [Special Topic: Why winnow?][crate::_topic::why]
//!
//! ## Example
//!
//! Run
//! ```console
//! $ cargo add winnow
//! ```
//!
//! Then use it to parse:
//! ```rust
//! # #[cfg(feature = "alloc")] {
#![doc = include_str!("../examples/css/parser.rs")]
//! # }
//! ```
//!
//! See also the [Tutorial][_tutorial::chapter_0] and [Special Topics][_topic]

#![cfg_attr(docsrs, feature(doc_auto_cfg))]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(docsrs, feature(extended_key_value_attributes))]
#![cfg_attr(not(feature = "std"), no_std)]
#![warn(missing_docs)]
#![warn(clippy::std_instead_of_core)]
#![warn(clippy::print_stderr)]
#![warn(clippy::print_stdout)]

#[cfg(feature = "alloc")]
#[cfg_attr(test, macro_use)]
#[allow(unused_extern_crates)]
extern crate alloc;
#[cfg(doctest)]
extern crate doc_comment;

#[cfg(doctest)]
doc_comment::doctest!("../README.md");

/// Lib module to re-export everything needed from `std` or `core`/`alloc`. This is how `serde` does
/// it, albeit there it is not public.
#[doc(hidden)]
pub(crate) mod lib {
    #![allow(unused_imports)]

    /// `std` facade allowing `std`/`core` to be interchangeable. Reexports `alloc` crate optionally,
    /// as well as `core` or `std`
    #[cfg(not(feature = "std"))]
    /// internal std exports for no_std compatibility
    pub(crate) mod std {
        #[doc(hidden)]
        #[cfg(not(feature = "alloc"))]
        pub(crate) use core::borrow;

        #[cfg(feature = "alloc")]
        #[doc(hidden)]
        pub(crate) use alloc::{borrow, boxed, collections, string, vec};

        #[doc(hidden)]
        pub(crate) use core::{
            cmp, convert, fmt, hash, iter, mem, ops, option, result, slice, str,
        };
    }

    #[cfg(feature = "std")]
    /// internal std exports for `no_std` compatibility
    pub(crate) mod std {
        #![allow(clippy::std_instead_of_core)]
        #[doc(hidden)]
        pub(crate) use std::{
            borrow, boxed, cmp, collections, convert, fmt, hash, iter, mem, ops, result, slice,
            str, string, vec,
        };
    }
}

#[macro_use]
mod macros;

#[macro_use]
pub mod error;

mod parser;

pub mod stream;

pub mod ascii;
pub mod binary;
pub mod combinator;
pub mod token;

#[cfg(feature = "unstable-doc")]
pub mod _topic;
#[cfg(feature = "unstable-doc")]
pub mod _tutorial;

/// Core concepts available for glob import
///
/// Including
/// - [`StreamIsPartial`][crate::stream::StreamIsPartial]
/// - [`Parser`]
///
/// ## Example
///
/// ```rust
/// use winnow::prelude::*;
///
/// fn parse_data(input: &mut &str) -> PResult<u64> {
///     // ...
/// #   winnow::ascii::dec_uint(input)
/// }
///
/// fn main() {
///   let result = parse_data.parse("100");
///   assert_eq!(result, Ok(100));
/// }
/// ```
pub mod prelude {
    pub use crate::stream::StreamIsPartial as _;
    pub use crate::IResult;
    pub use crate::PResult;
    pub use crate::Parser;
    #[cfg(feature = "unstable-recover")]
    #[cfg(feature = "std")]
    pub use crate::RecoverableParser as _;
}

pub use error::IResult;
pub use error::PResult;
pub use parser::*;
pub use stream::BStr;
pub use stream::Bytes;
pub use stream::Located;
pub use stream::Partial;
pub use stream::Stateful;
pub use stream::Str;
