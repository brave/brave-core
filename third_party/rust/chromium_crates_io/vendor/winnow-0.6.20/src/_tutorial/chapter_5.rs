//! # Chapter 5: Repetition
//!
//! In [`chapter_3`], we covered how to sequence different parsers into a tuple but sometimes you need to run a
//! single parser multiple times, collecting the result into a container, like [`Vec`].
//!
//! Let's collect the result of `parse_digits`:
//! ```rust
//! # use winnow::prelude::*;
//! # use winnow::token::take_while;
//! # use winnow::combinator::dispatch;
//! # use winnow::token::take;
//! # use winnow::combinator::fail;
//! use winnow::combinator::opt;
//! use winnow::combinator::repeat;
//! use winnow::combinator::terminated;
//!
//! fn parse_list(input: &mut &str) -> PResult<Vec<usize>> {
//!     let mut list = Vec::new();
//!     while let Some(output) = opt(terminated(parse_digits, opt(','))).parse_next(input)? {
//!         list.push(output);
//!     }
//!     Ok(list)
//! }
//!
//! // ...
//! # fn parse_digits(input: &mut &str) -> PResult<usize> {
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
//! #         ('0'..='1'),
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
//!     let mut input = "0x1a2b,0x3c4d,0x5e6f Hello";
//!
//!     let digits = parse_list.parse_next(&mut input).unwrap();
//!
//!     assert_eq!(input, " Hello");
//!     assert_eq!(digits, vec![0x1a2b, 0x3c4d, 0x5e6f]);
//!
//!     assert!(parse_digits(&mut "ghiWorld").is_err());
//! }
//! ```
//!
//! We can implement this declaratively with [`repeat`]:
//! ```rust
//! # use winnow::prelude::*;
//! # use winnow::token::take_while;
//! # use winnow::combinator::dispatch;
//! # use winnow::token::take;
//! # use winnow::combinator::fail;
//! use winnow::combinator::opt;
//! use winnow::combinator::repeat;
//! use winnow::combinator::terminated;
//!
//! fn parse_list(input: &mut &str) -> PResult<Vec<usize>> {
//!     repeat(0..,
//!         terminated(parse_digits, opt(','))
//!     ).parse_next(input)
//! }
//! #
//! # fn parse_digits(input: &mut &str) -> PResult<usize> {
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
//! #         ('0'..='1'),
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
//! #
//! # fn main() {
//! #     let mut input = "0x1a2b,0x3c4d,0x5e6f Hello";
//! #
//! #     let digits = parse_list.parse_next(&mut input).unwrap();
//! #
//! #     assert_eq!(input, " Hello");
//! #     assert_eq!(digits, vec![0x1a2b, 0x3c4d, 0x5e6f]);
//! #
//! #     assert!(parse_digits(&mut "ghiWorld").is_err());
//! # }
//! ```
//!
//! You'll notice that the above allows trailing `,` when we intended to not support that. We can
//! easily fix this by using [`separated`]:
//! ```rust
//! # use winnow::prelude::*;
//! # use winnow::token::take_while;
//! # use winnow::combinator::dispatch;
//! # use winnow::token::take;
//! # use winnow::combinator::fail;
//! use winnow::combinator::separated;
//!
//! fn parse_list(input: &mut &str) -> PResult<Vec<usize>> {
//!     separated(0.., parse_digits, ",").parse_next(input)
//! }
//!
//! // ...
//! # fn parse_digits(input: &mut &str) -> PResult<usize> {
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
//! #         ('0'..='1'),
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
//!     let mut input = "0x1a2b,0x3c4d,0x5e6f Hello";
//!
//!     let digits = parse_list.parse_next(&mut input).unwrap();
//!
//!     assert_eq!(input, " Hello");
//!     assert_eq!(digits, vec![0x1a2b, 0x3c4d, 0x5e6f]);
//!
//!     assert!(parse_digits(&mut "ghiWorld").is_err());
//! }
//! ```
//!
//! If you look closely at [`repeat`], it isn't collecting directly into a [`Vec`] but
//! anything that implements the [`Accumulate`] trait to gather the results. This lets us make more complex parsers than we did in
//! [`chapter_2`] by accumulating the results into a `()` and [`take`][Parser::take]-ing the captured input:
//! ```rust
//! # use winnow::prelude::*;
//! # use winnow::token::take_while;
//! # use winnow::combinator::dispatch;
//! # use winnow::token::take;
//! # use winnow::combinator::fail;
//! # use winnow::combinator::separated;
//! #
//! fn take_list<'s>(input: &mut &'s str) -> PResult<&'s str> {
//!     parse_list.take().parse_next(input)
//! }
//!
//! fn parse_list(input: &mut &str) -> PResult<()> {
//!     separated(0.., parse_digits, ",").parse_next(input)
//! }
//!
//! # fn parse_digits(input: &mut &str) -> PResult<usize> {
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
//! #         ('0'..='1'),
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
//!     let mut input = "0x1a2b,0x3c4d,0x5e6f Hello";
//!
//!     let digits = take_list.parse_next(&mut input).unwrap();
//!
//!     assert_eq!(input, " Hello");
//!     assert_eq!(digits, "0x1a2b,0x3c4d,0x5e6f");
//!
//!     assert!(parse_digits(&mut "ghiWorld").is_err());
//! }
//! ```
//! See [`combinator`] for more repetition parsers.

#![allow(unused_imports)]
use super::chapter_2;
use super::chapter_3;
use crate::combinator;
use crate::combinator::repeat;
use crate::combinator::separated;
use crate::stream::Accumulate;
use crate::Parser;
use std::vec::Vec;

pub use super::chapter_4 as previous;
pub use super::chapter_6 as next;
pub use crate::_tutorial as table_of_contents;
