//! # Chapter 1: The Winnow Way
//!
//! First of all, we need to understand the way that winnow thinks about parsing.
//! As discussed in the introduction, winnow lets us build simple parsers, and
//! then combine them (using "combinators").
//!
//! Let's discuss what a "parser" actually does. A parser takes an input and returns
//! a result, where:
//!  - `Ok` indicates the parser successfully found what it was looking for; or
//!  - `Err` indicates the parser could not find what it was looking for.
//!
//! Parsers do more than just return a binary "success"/"failure" code.
//! On success, the parser will return the processed data. The input will be left pointing to
//! data that still needs processing
//!
//! If the parser failed, then there are multiple errors that could be returned.
//! For simplicity, however, in the next chapters we will leave these unexplored.
//!
//! ```text
//!                                   ┌─► Ok(what matched the parser)
//!             ┌─────────┐           │
//! my input───►│my parser├──►either──┤
//!             └─────────┘           └─► Err(...)
//! ```
//!
//!
//! To represent this model of the world, winnow uses the [`PResult<O>`] type.
//! The `Ok` variant has `output: O`;
//! whereas the `Err` variant stores an error.
//!
//! You can import that from:
//!
//! ```rust
//! use winnow::PResult;
//! ```
//!
//! To combine parsers, we need a common way to refer to them which is where the [`Parser<I, O, E>`]
//! trait comes in with [`Parser::parse_next`] being the primary way to drive
//! parsing forward.
//!
//! You'll note that `I` and `O` are parameterized -- while most of the examples in this book
//! will be with `&str` (i.e. parsing a string); they do not have to be strings; nor do they
//! have to be the same type (consider the simple example where `I = &str`, and `O = u64` -- this
//! parses a string into an unsigned integer.)
//!
//!
//! # Let's write our first parser!
//!
//! The simplest parser we can write is one which successfully does nothing.
//!
//! To make it easier to implement a [`Parser`], the trait is implemented for
//! functions of the form `Fn(&mut I) -> PResult<O>`.
//!
//! This parser function should take in a `&str`:
//!
//!  - Since it is supposed to succeed, we know it will return the `Ok` variant.
//!  - Since it does nothing to our input, the remaining input is the same as the input.
//!  - Since it doesn't parse anything, it also should just return an empty string.
//!
//! ```rust
//! use winnow::PResult;
//! use winnow::Parser;
//!
//! pub fn do_nothing_parser<'s>(input: &mut &'s str) -> PResult<&'s str> {
//!     Ok("")
//! }
//!
//! fn main() {
//!     let mut input = "0x1a2b Hello";
//!
//!     let output = do_nothing_parser.parse_next(&mut input).unwrap();
//!     // Same as:
//!     // let output = do_nothing_parser(&mut input).unwrap();
//!
//!     assert_eq!(input, "0x1a2b Hello");
//!     assert_eq!(output, "");
//! }
//! ```

#![allow(unused_imports)]
use crate::PResult;
use crate::Parser;

pub use super::chapter_0 as previous;
pub use super::chapter_2 as next;
pub use crate::_tutorial as table_of_contents;
