//! > winnow, making parsing a breeze
//!
//! `winnow` is a parser combinator library
//!
//! Quick links:
//! - [List of combinators][crate::combinator]
//! - [Tutorial][_tutorial::chapter_0]
//! - [Special Topics][_topic]
//! - [Discussions](https://github.com/winnow-rs/winnow/discussions)
//! - [CHANGELOG](https://github.com/winnow-rs/winnow/blob/v0.5.40/CHANGELOG.md) (includes major version migration
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
// BEGIN - Embark standard lints v6 for Rust 1.55+
// do not change or add/remove here, but one can add exceptions after this section
// for more info see: <https://github.com/EmbarkStudios/rust-ecosystem/issues/59>
// "-Dunsafe_code",
#![warn(clippy::all)]
#![warn(clippy::await_holding_lock)]
#![warn(clippy::char_lit_as_u8)]
#![warn(clippy::checked_conversions)]
#![warn(clippy::dbg_macro)]
#![warn(clippy::debug_assert_with_mut_call)]
#![warn(clippy::doc_markdown)]
#![warn(clippy::empty_enum)]
#![warn(clippy::enum_glob_use)]
#![warn(clippy::exit)]
#![warn(clippy::expl_impl_clone_on_copy)]
#![warn(clippy::explicit_deref_methods)]
#![warn(clippy::explicit_into_iter_loop)]
#![warn(clippy::fallible_impl_from)]
#![warn(clippy::filter_map_next)]
#![warn(clippy::flat_map_option)]
#![warn(clippy::float_cmp_const)]
#![warn(clippy::fn_params_excessive_bools)]
#![warn(clippy::from_iter_instead_of_collect)]
#![warn(clippy::if_let_mutex)]
#![warn(clippy::implicit_clone)]
#![warn(clippy::imprecise_flops)]
#![warn(clippy::inefficient_to_string)]
#![warn(clippy::invalid_upcast_comparisons)]
#![warn(clippy::large_digit_groups)]
#![warn(clippy::large_stack_arrays)]
#![warn(clippy::large_types_passed_by_value)]
#![warn(clippy::let_unit_value)]
#![warn(clippy::linkedlist)]
#![warn(clippy::lossy_float_literal)]
#![warn(clippy::macro_use_imports)]
#![warn(clippy::manual_ok_or)]
#![warn(clippy::map_err_ignore)]
#![warn(clippy::map_flatten)]
#![warn(clippy::map_unwrap_or)]
#![warn(clippy::match_on_vec_items)]
#![warn(clippy::match_same_arms)]
#![warn(clippy::match_wild_err_arm)]
#![warn(clippy::match_wildcard_for_single_variants)]
#![warn(clippy::mem_forget)]
#![warn(clippy::mismatched_target_os)]
#![warn(clippy::missing_enforced_import_renames)]
#![warn(clippy::mut_mut)]
#![warn(clippy::mutex_integer)]
#![warn(clippy::needless_borrow)]
#![warn(clippy::needless_continue)]
#![warn(clippy::needless_for_each)]
#![warn(clippy::option_option)]
#![warn(clippy::path_buf_push_overwrite)]
#![warn(clippy::ptr_as_ptr)]
#![warn(clippy::rc_mutex)]
#![warn(clippy::ref_option_ref)]
#![warn(clippy::rest_pat_in_fully_bound_structs)]
#![warn(clippy::same_functions_in_if_condition)]
#![warn(clippy::semicolon_if_nothing_returned)]
#![warn(clippy::single_match_else)]
#![warn(clippy::string_add_assign)]
#![warn(clippy::string_add)]
#![warn(clippy::string_lit_as_bytes)]
#![warn(clippy::string_to_string)]
#![warn(clippy::todo)]
#![warn(clippy::trait_duplication_in_bounds)]
#![warn(clippy::unimplemented)]
#![warn(clippy::unnested_or_patterns)]
#![warn(clippy::unused_self)]
#![warn(clippy::useless_transmute)]
#![warn(clippy::verbose_file_reads)]
#![warn(clippy::zero_sized_map_values)]
#![warn(future_incompatible)]
#![warn(nonstandard_style)]
#![warn(rust_2018_idioms)]
// END - Embark standard lints v6 for Rust 1.55+
#![allow(clippy::branches_sharing_code)]
#![allow(clippy::collapsible_else_if)]
#![allow(clippy::if_same_then_else)]
#![allow(clippy::bool_assert_comparison)]
#![allow(clippy::let_and_return)]
#![allow(clippy::assertions_on_constants)]
#![allow(clippy::map_unwrap_or)]
#![allow(clippy::single_match_else)]
#![allow(clippy::single_match)]
#![allow(clippy::unnested_or_patterns)]
#![allow(deprecated)]
#[cfg_attr(nightly, warn(rustdoc::missing_doc_code_examples))]
#[cfg(feature = "alloc")]
#[cfg_attr(test, macro_use)]
extern crate alloc;
#[cfg(doctest)]
extern crate doc_comment;

#[cfg(doctest)]
doc_comment::doctest!("../README.md");

/// Lib module to re-export everything needed from `std` or `core`/`alloc`. This is how `serde` does
/// it, albeit there it is not public.
#[doc(hidden)]
pub(crate) mod lib {
    /// `std` facade allowing `std`/`core` to be interchangeable. Reexports `alloc` crate optionally,
    /// as well as `core` or `std`
    #[cfg(not(feature = "std"))]
    /// internal std exports for no_std compatibility
    pub mod std {
        #[doc(hidden)]
        #[cfg(not(feature = "alloc"))]
        pub use core::borrow;

        #[cfg(feature = "alloc")]
        #[doc(hidden)]
        pub use alloc::{borrow, boxed, collections, string, vec};

        #[doc(hidden)]
        pub use core::{cmp, convert, fmt, hash, iter, mem, ops, option, result, slice, str};

        /// internal reproduction of std prelude
        #[doc(hidden)]
        pub mod prelude {
            pub use core::prelude as v1;
        }
    }

    #[cfg(feature = "std")]
    /// internal std exports for `no_std` compatibility
    pub mod std {
        #![allow(clippy::std_instead_of_core)]
        #[doc(hidden)]
        pub use std::{
            alloc, borrow, boxed, cmp, collections, convert, fmt, hash, iter, mem, ops, option,
            result, slice, str, string, vec,
        };

        /// internal reproduction of std prelude
        #[doc(hidden)]
        pub mod prelude {
            pub use std::prelude as v1;
        }
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
pub mod trace;

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
