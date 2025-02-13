//! # List of parsers and combinators
//!
//! **Note**: this list is meant to provide a nicer way to find a parser than reading through the documentation on docs.rs. Function combinators are organized in module so they are a bit easier to find.
//!
//! ## Basic elements
//!
//! Those are used to recognize the lowest level elements of your grammar, like, "here is a dot", or "here is an big endian integer".
//!
//! | combinator | usage | input | new input | output | comment |
//! |---|---|---|---|---|---|
//! | [`one_of`][crate::token::one_of] | `one_of(['a', 'b', 'c'])` |  `"abc"` |  `"bc"` | `Ok('a')` |Matches one of the provided characters (works with non ASCII characters too)|
//! | [`none_of`][crate::token::none_of] | `none_of(['a', 'b', 'c'])` |  `"xyab"` |  `"yab"` | `Ok('x')` |Matches anything but the provided characters|
//! | [`tag`][crate::token::tag] | `"hello"` |  `"hello world"` |  `" world"` | `Ok("hello")` |Recognizes a specific suite of characters or bytes (see also [`Caseless`][crate::ascii::Caseless])|
//! | [`take`][crate::token::take] | `take(4)` |  `"hello"` |  `"o"` | `Ok("hell")` |Takes a specific number of bytes or characters|
//! | [`take_while`][crate::token::take_while] | `take_while(0.., is_alphabetic)` |  `"abc123"` |  `"123"` | `Ok("abc")` |Returns the longest list of bytes for which the provided pattern matches.|
//! | [`take_till0`][crate::token::take_till0] | `take_till0(is_alphabetic)` |  `"123abc"` |  `"abc"` | `Ok("123")` |Returns the longest list of bytes or characters until the provided pattern matches. `take_till1` does the same, but must return at least one character. This is the reverse behaviour from `take_while`: `take_till(f)` is equivalent to `take_while(0.., \|c\| !f(c))`|
//! | [`take_until`][crate::token::take_until] | `take_until(0.., "world")` |  `"Hello world"` |  `"world"` | `Ok("Hello ")` |Returns the longest list of bytes or characters until the provided tag is found.|
//!
//! ## Choice combinators
//!
//! | combinator | usage | input | new input | output | comment |
//! |---|---|---|---|---|---|
//! | [`alt`] | `alt(("ab", "cd"))` |  `"cdef"` |  `"ef"` | `Ok("cd")` |Try a list of parsers and return the result of the first successful one|
//! | [`dispatch`] | \- | \- | \- | \- | `match` for parsers |
//! | [`permutation`] | `permutation(("ab", "cd", "12"))` | `"cd12abc"` | `"c"` | `Ok(("ab", "cd", "12"))` |Succeeds when all its child parser have succeeded, whatever the order|
//!
//! ## Sequence combinators
//!
//! | combinator | usage | input | new input | output | comment |
//! |---|---|---|---|---|---|
//! | [`(...)` (tuples)][crate::Parser] | `("ab", "XY", take(1))` | `"abXYZ!"` | `"!"` | `Ok(("ab", "XY", "Z"))` |Chains parsers and assemble the sub results in a tuple. You can use as many child parsers as you can put elements in a tuple|
//! | [`seq!`] | `seq!(_: char('('), take(2), _: char(')'))` | `"(ab)cd"` | `"cd"` | `Ok("ab")` ||
//! | [`delimited`] | `delimited(char('('), take(2), char(')'))` | `"(ab)cd"` | `"cd"` | `Ok("ab")` ||
//! | [`preceded`] | `preceded("ab", "XY")` | `"abXYZ"` | `"Z"` | `Ok("XY")` ||
//! | [`terminated`] | `terminated("ab", "XY")` | `"abXYZ"` | `"Z"` | `Ok("ab")` ||
//! | [`separated_pair`] | `separated_pair("hello", char(','), "world")` | `"hello,world!"` | `"!"` | `Ok(("hello", "world"))` ||
//!
//! ## Applying a parser multiple times
//!
//! | combinator | usage | input | new input | output | comment |
//! |---|---|---|---|---|---|
//! | [`repeat`] | `repeat(1..=3, "ab")` | `"ababc"` | `"c"` | `Ok(vec!["ab", "ab"])` |Applies the parser between m and n times (n included) and returns the list of results in a Vec|
//! | [`repeat_till`] | `repeat_till(0.., tag( "ab" ), tag( "ef" ))` | `"ababefg"` | `"g"` | `Ok((vec!["ab", "ab"], "ef"))` |Applies the first parser until the second applies. Returns a tuple containing the list of results from the first in a Vec and the result of the second|
//! | [`separated`] | `separated(1..=3, "ab", ",")` | `"ab,ab,ab."` | `"."` | `Ok(vec!["ab", "ab", "ab"])` |Applies the parser and separator between m and n times (n included) and returns the list of results in a Vec|
//! | [`fold_repeat`] | `fold_repeat(1..=2, be_u8, \|\| 0, \|acc, item\| acc + item)` | `[1, 2, 3]` | `[3]` | `Ok(3)` |Applies the parser between m and n times (n included) and folds the list of return value|
//!
//! ## Partial related
//!
//! - [`eof`]: Returns its input if it is at the end of input data
//! - [`Parser::complete_err`]: Replaces an `Incomplete` returned by the child parser with an `Backtrack`
//!
//! ## Modifiers
//!
//! - [`cond`]: Conditional combinator. Wraps another parser and calls it if the condition is met
//! - [`Parser::flat_map`]: method to map a new parser from the output of the first parser, then apply that parser over the rest of the input
//! - [`Parser::value`]: method to replace the result of a parser
//! - [`Parser::default_value`]: method to replace the result of a parser
//! - [`Parser::void`]: method to discard the result of a parser
//! - [`Parser::map`]: method to map a function on the result of a parser
//! - [`Parser::and_then`]: Applies a second parser over the output of the first one
//! - [`Parser::verify_map`]: Maps a function returning an `Option` on the output of a parser
//! - [`Parser::try_map`]: Maps a function returning a `Result` on the output of a parser
//! - [`Parser::parse_to`]: Apply [`std::str::FromStr`] to the output of the parser
//! - [`not`]: Returns a result only if the embedded parser returns `Backtrack` or `Incomplete`. Does not consume the input
//! - [`opt`]: Make the underlying parser optional
//! - [`peek`]: Returns a result without consuming the input
//! - [`Parser::recognize`]: If the child parser was successful, return the consumed input as the produced value
//! - [`Parser::with_recognized`]: If the child parser was successful, return a tuple of the consumed input and the produced output.
//! - [`Parser::span`]: If the child parser was successful, return the location of the consumed input as the produced value
//! - [`Parser::with_span`]: If the child parser was successful, return a tuple of the location of the consumed input and the produced output.
//! - [`Parser::verify`]: Returns the result of the child parser if it satisfies a verification function
//!
//! ## Error management and debugging
//!
//! - [`cut_err`]: Commit the parse result, disallowing alternative parsers from being attempted
//! - [`backtrack_err`]: Attempts a parse, allowing alternative parsers to be attempted despite
//!   use of `cut_err`
//! - [`Parser::context`]: Add context to the error if the parser fails
//! - [`trace`]: Print the parse state with the `debug` feature flag
//! - [`todo()`]: Placeholder parser
//!
//! ## Remaining combinators
//!
//! - [`success`]: Returns a value without consuming any input, always succeeds
//! - [`fail`]: Inversion of `success`. Always fails.
//! - [`Parser::by_ref`]: Allow moving `&mut impl Parser` into other parsers
//!
//! ## Text parsing
//!
//! - [`any`][crate::token::any]: Matches one token
//! - [`tab`][crate::ascii::tab]: Matches a tab character `\t`
//! - [`crlf`][crate::ascii::crlf]: Recognizes the string `\r\n`
//! - [`line_ending`][crate::ascii::line_ending]: Recognizes an end of line (both `\n` and `\r\n`)
//! - [`newline`][crate::ascii::newline]: Matches a newline character `\n`
//! - [`till_line_ending`][crate::ascii::till_line_ending]: Recognizes a string of any char except `\r` or `\n`
//! - [`rest`]: Return the remaining input
//!
//! - [`alpha0`][crate::ascii::alpha0]: Recognizes zero or more lowercase and uppercase alphabetic characters: `[a-zA-Z]`. [`alpha1`][crate::ascii::alpha1] does the same but returns at least one character
//! - [`alphanumeric0`][crate::ascii::alphanumeric0]: Recognizes zero or more numerical and alphabetic characters: `[0-9a-zA-Z]`. [`alphanumeric1`][crate::ascii::alphanumeric1] does the same but returns at least one character
//! - [`space0`][crate::ascii::space0]: Recognizes zero or more spaces and tabs. [`space1`][crate::ascii::space1] does the same but returns at least one character
//! - [`multispace0`][crate::ascii::multispace0]: Recognizes zero or more spaces, tabs, carriage returns and line feeds. [`multispace1`][crate::ascii::multispace1] does the same but returns at least one character
//! - [`digit0`][crate::ascii::digit0]: Recognizes zero or more numerical characters: `[0-9]`. [`digit1`][crate::ascii::digit1] does the same but returns at least one character
//! - [`hex_digit0`][crate::ascii::hex_digit0]: Recognizes zero or more hexadecimal numerical characters: `[0-9A-Fa-f]`. [`hex_digit1`][crate::ascii::hex_digit1] does the same but returns at least one character
//! - [`oct_digit0`][crate::ascii::oct_digit0]: Recognizes zero or more octal characters: `[0-7]`. [`oct_digit1`][crate::ascii::oct_digit1] does the same but returns at least one character
//!
//! - [`float`][crate::ascii::float]: Parse a floating point number in a byte string
//! - [`dec_int`][crate::ascii::dec_int]: Decode a variable-width, decimal signed integer
//! - [`dec_uint`][crate::ascii::dec_uint]: Decode a variable-width, decimal unsigned integer
//! - [`hex_uint`][crate::ascii::hex_uint]: Decode a variable-width, hexadecimal integer
//!
//! - [`escaped`][crate::ascii::escaped]: Matches a byte string with escaped characters
//! - [`escaped_transform`][crate::ascii::escaped_transform]: Matches a byte string with escaped characters, and returns a new string with the escaped characters replaced
//!
//! ### Character test functions
//!
//! Use these functions with a combinator like `take_while`:
//!
//! - [`AsChar::is_alpha`][crate::stream::AsChar::is_alpha]: Tests if byte is ASCII alphabetic: `[A-Za-z]`
//! - [`AsChar::is_alphanum`][crate::stream::AsChar::is_alphanum]: Tests if byte is ASCII alphanumeric: `[A-Za-z0-9]`
//! - [`AsChar::is_dec_digit`][crate::stream::AsChar::is_dec_digit]: Tests if byte is ASCII digit: `[0-9]`
//! - [`AsChar::is_hex_digit`][crate::stream::AsChar::is_hex_digit]: Tests if byte is ASCII hex digit: `[0-9A-Fa-f]`
//! - [`AsChar::is_oct_digit`][crate::stream::AsChar::is_oct_digit]: Tests if byte is ASCII octal digit: `[0-7]`
//! - [`AsChar::is_space`][crate::stream::AsChar::is_space]: Tests if byte is ASCII space or tab: `[ \t]`
//! - [`AsChar::is_newline`][crate::stream::AsChar::is_newline]: Tests if byte is ASCII newline: `[\n]`
//!
//! ## Binary format parsing
//!
//! - [`length_count`][crate::binary::length_count] Gets a number from the first parser, then applies the second parser that many times
//! - [`length_data`][crate::binary::length_data]: Gets a number from the first parser, then takes a subslice of the input of that size, and returns that subslice
//! - [`length_value`][crate::binary::length_value]: Gets a number from the first parser, takes a subslice of the input of that size, then applies the second parser on that subslice. If the second parser returns `Incomplete`, `length_value` will return an error
//!
//! ### Integers
//!
//! Parsing integers from binary formats can be done in two ways: With parser functions, or combinators with configurable endianness.
//!
//! - **configurable endianness:** [`i16`][crate::binary::i16], [`i32`][crate::binary::i32],
//!   [`i64`][crate::binary::i64], [`u16`][crate::binary::u16], [`u32`][crate::binary::u32],
//!   [`u64`][crate::binary::u64] are combinators that take as argument a
//!   [`winnow::binary::Endianness`][crate::binary::Endianness], like this: `i16(endianness)`. If the
//!   parameter is `winnow::binary::Endianness::Big`, parse a big endian `i16` integer, otherwise a
//!   little endian `i16` integer.
//! - **fixed endianness**: The functions are prefixed by `be_` for big endian numbers, and by `le_` for little endian numbers, and the suffix is the type they parse to. As an example, `be_u32` parses a big endian unsigned integer stored in 32 bits.
//!   - [`be_f32`][crate::binary::be_f32], [`be_f64`][crate::binary::be_f64]: Big endian floating point numbers
//!   - [`le_f32`][crate::binary::le_f32], [`le_f64`][crate::binary::le_f64]: Little endian floating point numbers
//!   - [`be_i8`][crate::binary::be_i8], [`be_i16`][crate::binary::be_i16], [`be_i24`][crate::binary::be_i24], [`be_i32`][crate::binary::be_i32], [`be_i64`][crate::binary::be_i64], [`be_i128`][crate::binary::be_i128]: Big endian signed integers
//!   - [`be_u8`][crate::binary::be_u8], [`be_u16`][crate::binary::be_u16], [`be_u24`][crate::binary::be_u24], [`be_u32`][crate::binary::be_u32], [`be_u64`][crate::binary::be_u64], [`be_u128`][crate::binary::be_u128]: Big endian unsigned integers
//!   - [`le_i8`][crate::binary::le_i8], [`le_i16`][crate::binary::le_i16], [`le_i24`][crate::binary::le_i24], [`le_i32`][crate::binary::le_i32], [`le_i64`][crate::binary::le_i64], [`le_i128`][crate::binary::le_i128]: Little endian signed integers
//!   - [`le_u8`][crate::binary::le_u8], [`le_u16`][crate::binary::le_u16], [`le_u24`][crate::binary::le_u24], [`le_u32`][crate::binary::le_u32], [`le_u64`][crate::binary::le_u64], [`le_u128`][crate::binary::le_u128]: Little endian unsigned integers
//!
//! ### Bit stream parsing
//!
//! - [`bits`][crate::binary::bits::bits]: Transforms the current input type (byte slice `&[u8]`) to a bit stream on which bit specific parsers and more general combinators can be applied
//! - [`bytes`][crate::binary::bits::bytes]: Transforms its bits stream input back into a byte slice for the underlying parser
//! - [`take`][crate::binary::bits::take]: Take a set number of bits
//! - [`tag`][crate::binary::bits::tag]: Check if a set number of bits matches a pattern
//! - [`bool`][crate::binary::bits::bool]: Match any one bit

mod branch;
mod core;
mod debug;
mod multi;
mod parser;
mod sequence;

#[cfg(test)]
mod tests;

pub use self::branch::*;
pub use self::core::*;
pub use self::debug::*;
pub use self::multi::*;
pub use self::parser::*;
pub use self::sequence::*;

#[allow(unused_imports)]
use crate::Parser;
