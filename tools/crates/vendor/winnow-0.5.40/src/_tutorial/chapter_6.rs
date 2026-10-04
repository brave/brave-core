//! # Chapter 6: Error Reporting
//!
//! ## `Error`
//!
//! Back in [`chapter_1`], we glossed over the `Err` side of [`PResult`].  `PResult<O>` is
//! actually short for `PResult<O, E=ContextError>` where [`ContextError`] is a relatively cheap
//! way of building up reasonable errors for humans.
//!
//! You can use [`Parser::context`] to annotate the error with custom types
//! while unwinding to further improve the error quality.
//!
//! ```rust
//! # use winnow::prelude::*;
//! # use winnow::token::take_while;
//! # use winnow::combinator::alt;
//! use winnow::error::StrContext;
//!
//! fn parse_digits<'s>(input: &mut &'s str) -> PResult<(&'s str, &'s str)> {
//!     alt((
//!         ("0b", parse_bin_digits).context(StrContext::Label("binary")),
//!         ("0o", parse_oct_digits).context(StrContext::Label("octal")),
//!         ("0d", parse_dec_digits).context(StrContext::Label("decimal")),
//!         ("0x", parse_hex_digits).context(StrContext::Label("hexadecimal")),
//!     )).parse_next(input)
//! }
//!
//! // ...
//! # fn parse_bin_digits<'s>(input: &mut &'s str) -> PResult<&'s str> {
//! #     take_while(1.., (
//! #         ('0'..='7'),
//! #     )).parse_next(input)
//! # }
//! #
//! # fn parse_oct_digits<'s>(input: &mut &'s str) -> PResult<&'s str> {
//! #     take_while(1.., (
//! #         ('0'..='7'),
//! #     )).parse_next(input)
//! # }
//! #
//! # fn parse_dec_digits<'s>(input: &mut &'s str) -> PResult<&'s str> {
//! #     take_while(1.., (
//! #         ('0'..='9'),
//! #     )).parse_next(input)
//! # }
//! #
//! # fn parse_hex_digits<'s>(input: &mut &'s str) -> PResult<&'s str> {
//! #     take_while(1.., (
//! #         ('0'..='9'),
//! #         ('A'..='F'),
//! #         ('a'..='f'),
//! #     )).parse_next(input)
//! # }
//!
//! fn main() {
//!     let mut input = "0x1a2b Hello";
//!
//!     let (prefix, digits) = parse_digits.parse_next(&mut input).unwrap();
//!
//!     assert_eq!(input, " Hello");
//!     assert_eq!(prefix, "0x");
//!     assert_eq!(digits, "1a2b");
//! }
//! ```
//!
//! At first glance, this looks correct but what `context` will be reported when parsing `"0b5"`?
//! If you remember back to [`chapter_3`], [`alt`] will only report the last error by default which
//! means when parsing `"0b5"`, the `context` will be `"hexadecimal"`.
//!
//! ## `ErrMode`
//!
//! Let's break down `PResult<O, E>` one step further:
//! ```rust
//! # use winnow::error::ErrorKind;
//! # use winnow::error::ErrMode;
//! pub type PResult<O, E = ErrorKind> = Result<O, ErrMode<E>>;
//! ```
//! [`PResult`] is just a fancy wrapper around `Result` that wraps our error in an [`ErrMode`]
//! type.
//!
//! [`ErrMode`] is an enum with [`Backtrack`] and [`Cut`] variants (ignore [`Incomplete`] as its only
//! relevant for [streaming][_topic::stream]). By default, errors are [`Backtrack`], meaning that
//! other parsing branches will be attempted on failure, like the next case of an [`alt`].  [`Cut`]
//! shortcircuits all other branches, immediately reporting the error.
//!
//! So we can get the correct `context` by modifying the above example with [`cut_err`]:
//! ```rust
//! # use winnow::prelude::*;
//! # use winnow::token::take_while;
//! # use winnow::combinator::alt;
//! # use winnow::error::StrContext;
//! use winnow::combinator::cut_err;
//!
//! fn parse_digits<'s>(input: &mut &'s str) -> PResult<(&'s str, &'s str)> {
//!     alt((
//!         ("0b", cut_err(parse_bin_digits)).context(StrContext::Label("binary")),
//!         ("0o", cut_err(parse_oct_digits)).context(StrContext::Label("octal")),
//!         ("0d", cut_err(parse_dec_digits)).context(StrContext::Label("decimal")),
//!         ("0x", cut_err(parse_hex_digits)).context(StrContext::Label("hexadecimal")),
//!     )).parse_next(input)
//! }
//!
//! // ...
//! # fn parse_bin_digits<'s>(input: &mut &'s str) -> PResult<&'s str> {
//! #     take_while(1.., (
//! #         ('0'..='7'),
//! #     )).parse_next(input)
//! # }
//! #
//! # fn parse_oct_digits<'s>(input: &mut &'s str) -> PResult<&'s str> {
//! #     take_while(1.., (
//! #         ('0'..='7'),
//! #     )).parse_next(input)
//! # }
//! #
//! # fn parse_dec_digits<'s>(input: &mut &'s str) -> PResult<&'s str> {
//! #     take_while(1.., (
//! #         ('0'..='9'),
//! #     )).parse_next(input)
//! # }
//! #
//! # fn parse_hex_digits<'s>(input: &mut &'s str) -> PResult<&'s str> {
//! #     take_while(1.., (
//! #         ('0'..='9'),
//! #         ('A'..='F'),
//! #         ('a'..='f'),
//! #     )).parse_next(input)
//! # }
//!
//! fn main() {
//!     let mut input = "0x1a2b Hello";
//!
//!     let (prefix, digits) = parse_digits.parse_next(&mut input).unwrap();
//!
//!     assert_eq!(input, " Hello");
//!     assert_eq!(prefix, "0x");
//!     assert_eq!(digits, "1a2b");
//! }
//! ```
//! Now, when parsing `"0b5"`, the `context` will be `"binary"`.

#![allow(unused_imports)]
use super::chapter_1;
use super::chapter_3;
use crate::combinator::alt;
use crate::combinator::cut_err;
use crate::error::ContextError;
use crate::error::ErrMode;
use crate::error::ErrMode::*;
use crate::error::ErrorKind;
use crate::PResult;
use crate::Parser;
use crate::_topic;

pub use super::chapter_5 as previous;
pub use super::chapter_7 as next;
pub use crate::_tutorial as table_of_contents;
