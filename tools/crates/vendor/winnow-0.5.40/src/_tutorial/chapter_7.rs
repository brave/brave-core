//! # Chapter 7: Integrating the Parser
//!
//! So far, we've highlighted how to incrementally parse, but how do we bring this all together
//! into our application?
//!
//! Parsers we've been working with look like:
//! ```rust
//! # use winnow::error::ContextError;
//! # use winnow::error::ErrMode;
//! # use winnow::Parser;
//! #
//! pub fn parser<'s>(input: &mut &'s str) -> PResult<&'s str> {
//!     // ...
//! #     Ok("")
//! }
//!
//! type PResult<O> = Result<
//!     O,
//!     ErrMode<ContextError>
//! >;
//! ```
//! 1. We have to decide what to do about the "remainder" of the `input`.
//! 2. The [`ErrMode<ContextError>`] is not compatible with the rest of the Rust ecosystem.
//!     Normally, Rust applications want errors that are `std::error::Error + Send + Sync + 'static`
//!     meaning:
//!     - They implement the [`std::error::Error`] trait
//!     - They can be sent across threads
//!     - They are safe to be referenced across threads
//!     - They do not borrow
//!
//! winnow provides [`Parser::parse`] to help with this:
//! - Ensures we hit [`eof`]
//! - Removes the [`ErrMode`] wrapper
//! - Wraps the error in [`ParseError`]
//!   - Provides access to the original [`input`][ParseError::input] with the
//!     [`offset`][ParseError::offset] of where it failed
//!   - Provides a default renderer (via [`std::fmt::Display`])
//! ```rust
//! # use winnow::prelude::*;
//! # use winnow::token::take_while;
//! # use winnow::combinator::dispatch;
//! # use winnow::token::take;
//! # use winnow::combinator::fail;
//! use winnow::Parser;
//!
//! #[derive(Debug, PartialEq, Eq)]
//! pub struct Hex(usize);
//!
//! impl std::str::FromStr for Hex {
//!     type Err = String;
//!
//!     fn from_str(input: &str) -> Result<Self, Self::Err> {
//!         parse_digits
//!             .map(Hex)
//!             .parse(input)
//!             .map_err(|e| e.to_string())
//!     }
//! }
//!
//! // ...
//! # fn parse_digits<'s>(input: &mut &'s str) -> PResult<usize> {
//! #     dispatch!(take(2usize);
//! #         "0b" => parse_bin_digits.try_map(|s| usize::from_str_radix(s, 2)),
//! #         "0o" => parse_oct_digits.try_map(|s| usize::from_str_radix(s, 8)),
//! #         "0d" => parse_dec_digits.try_map(|s| usize::from_str_radix(s, 10)),
//! #         "0x" => parse_hex_digits.try_map(|s| usize::from_str_radix(s, 16)),
//! #         _ => fail,
//! #     ).parse_next(input)
//! # }
//! #
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
//!     let input = "0x1a2b";
//!     assert_eq!(input.parse::<Hex>().unwrap(), Hex(0x1a2b));
//!
//!     let input = "0x1a2b Hello";
//!     assert!(input.parse::<Hex>().is_err());
//!     let input = "ghiHello";
//!     assert!(input.parse::<Hex>().is_err());
//! }
//! ```

#![allow(unused_imports)]
use super::chapter_1;
use crate::combinator::eof;
use crate::error::ErrMode;
use crate::error::InputError;
use crate::error::ParseError;
use crate::PResult;
use crate::Parser;

pub use super::chapter_6 as previous;
pub use super::chapter_8 as next;
pub use crate::_tutorial as table_of_contents;
