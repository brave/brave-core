//! Parsers extracting tokens from the stream

#[cfg(test)]
mod tests;

use crate::combinator::trace;
use crate::error::ErrMode;
use crate::error::ErrorKind;
use crate::error::Needed;
use crate::error::ParserError;
use crate::lib::std::result::Result::Ok;
use crate::stream::Range;
use crate::stream::{Compare, CompareResult, ContainsToken, FindSlice, SliceLen, Stream};
use crate::stream::{StreamIsPartial, ToUsize};
use crate::PResult;
use crate::Parser;

/// Matches one token
///
/// *Complete version*: Will return an error if there's not enough input data.
///
/// *Partial version*: Will return `Err(winnow::error::ErrMode::Incomplete(_))` if there's not enough input data.
///
/// # Example
///
/// ```rust
/// # use winnow::{token::any, error::ErrMode, error::{InputError, ErrorKind}};
/// # use winnow::prelude::*;
/// fn parser(input: &str) -> IResult<&str, char> {
///     any.parse_peek(input)
/// }
///
/// assert_eq!(parser("abc"), Ok(("bc",'a')));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Token))));
/// ```
///
/// ```rust
/// # use winnow::{token::any, error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// assert_eq!(any::<_, InputError<_>>.parse_peek(Partial::new("abc")), Ok((Partial::new("bc"),'a')));
/// assert_eq!(any::<_, InputError<_>>.parse_peek(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(1))));
/// ```
#[inline(always)]
#[doc(alias = "token")]
pub fn any<I, E: ParserError<I>>(input: &mut I) -> PResult<<I as Stream>::Token, E>
where
    I: StreamIsPartial,
    I: Stream,
{
    trace("any", move |input: &mut I| {
        if <I as StreamIsPartial>::is_partial_supported() {
            any_::<_, _, true>(input)
        } else {
            any_::<_, _, false>(input)
        }
    })
    .parse_next(input)
}

fn any_<I, E: ParserError<I>, const PARTIAL: bool>(
    input: &mut I,
) -> PResult<<I as Stream>::Token, E>
where
    I: StreamIsPartial,
    I: Stream,
{
    input.next_token().ok_or_else(|| {
        if PARTIAL && input.is_partial() {
            ErrMode::Incomplete(Needed::new(1))
        } else {
            ErrMode::from_error_kind(input, ErrorKind::Token)
        }
    })
}

/// Recognizes a literal
///
/// The input data will be compared to the tag combinator's argument and will return the part of
/// the input that matches the argument
///
/// It will return `Err(ErrMode::Backtrack(InputError::new(_, ErrorKind::Tag)))` if the input doesn't match the pattern
///
/// **Note:** [`Parser`] is implemented for strings and byte strings as a convenience (complete
/// only)
///
/// # Example
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// use winnow::token::tag;
///
/// fn parser(s: &str) -> IResult<&str, &str> {
///   "Hello".parse_peek(s)
/// }
///
/// assert_eq!(parser("Hello, World!"), Ok((", World!", "Hello")));
/// assert_eq!(parser("Something"), Err(ErrMode::Backtrack(InputError::new("Something", ErrorKind::Tag))));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// ```
///
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::Partial;
/// use winnow::token::tag;
///
/// fn parser(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   "Hello".parse_peek(s)
/// }
///
/// assert_eq!(parser(Partial::new("Hello, World!")), Ok((Partial::new(", World!"), "Hello")));
/// assert_eq!(parser(Partial::new("Something")), Err(ErrMode::Backtrack(InputError::new(Partial::new("Something"), ErrorKind::Tag))));
/// assert_eq!(parser(Partial::new("S")), Err(ErrMode::Backtrack(InputError::new(Partial::new("S"), ErrorKind::Tag))));
/// assert_eq!(parser(Partial::new("H")), Err(ErrMode::Incomplete(Needed::new(4))));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::tag;
/// use winnow::ascii::Caseless;
///
/// fn parser(s: &str) -> IResult<&str, &str> {
///   tag(Caseless("hello")).parse_peek(s)
/// }
///
/// assert_eq!(parser("Hello, World!"), Ok((", World!", "Hello")));
/// assert_eq!(parser("hello, World!"), Ok((", World!", "hello")));
/// assert_eq!(parser("HeLlO, World!"), Ok((", World!", "HeLlO")));
/// assert_eq!(parser("Something"), Err(ErrMode::Backtrack(InputError::new("Something", ErrorKind::Tag))));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// ```
#[inline(always)]
#[doc(alias = "tag")]
#[doc(alias = "bytes")]
#[doc(alias = "just")]
pub fn literal<T, I, Error: ParserError<I>>(tag: T) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + Compare<T>,
    T: SliceLen + Clone,
{
    trace("tag", move |i: &mut I| {
        let t = tag.clone();
        if <I as StreamIsPartial>::is_partial_supported() {
            literal_::<_, _, _, true>(i, t)
        } else {
            literal_::<_, _, _, false>(i, t)
        }
    })
}

/// Deprecated, replaced with [`literal`]
#[deprecated(since = "0.5.38", note = "Replaced with `literal`")]
pub fn tag<T, I, Error: ParserError<I>>(tag: T) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + Compare<T>,
    T: SliceLen + Clone,
{
    literal(tag)
}

fn literal_<T, I, Error: ParserError<I>, const PARTIAL: bool>(
    i: &mut I,
    t: T,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + Compare<T>,
    T: SliceLen,
{
    let tag_len = t.slice_len();
    match i.compare(t) {
        CompareResult::Ok => Ok(i.next_slice(tag_len)),
        CompareResult::Incomplete if PARTIAL && i.is_partial() => {
            Err(ErrMode::Incomplete(Needed::new(tag_len - i.eof_offset())))
        }
        CompareResult::Incomplete | CompareResult::Error => {
            let e: ErrorKind = ErrorKind::Tag;
            Err(ErrMode::from_error_kind(i, e))
        }
    }
}

/// Recognizes a case insensitive literal.
///
/// The input data will be compared to the tag combinator's argument and will return the part of
/// the input that matches the argument with no regard to case.
///
/// It will return `Err(ErrMode::Backtrack(InputError::new(_, ErrorKind::Tag)))` if the input doesn't match the pattern.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::tag_no_case;
///
/// fn parser(s: &str) -> IResult<&str, &str> {
///   tag_no_case("hello").parse_peek(s)
/// }
///
/// assert_eq!(parser("Hello, World!"), Ok((", World!", "Hello")));
/// assert_eq!(parser("hello, World!"), Ok((", World!", "hello")));
/// assert_eq!(parser("HeLlO, World!"), Ok((", World!", "HeLlO")));
/// assert_eq!(parser("Something"), Err(ErrMode::Backtrack(InputError::new("Something", ErrorKind::Tag))));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// use winnow::token::tag_no_case;
///
/// fn parser(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   tag_no_case("hello").parse_peek(s)
/// }
///
/// assert_eq!(parser(Partial::new("Hello, World!")), Ok((Partial::new(", World!"), "Hello")));
/// assert_eq!(parser(Partial::new("hello, World!")), Ok((Partial::new(", World!"), "hello")));
/// assert_eq!(parser(Partial::new("HeLlO, World!")), Ok((Partial::new(", World!"), "HeLlO")));
/// assert_eq!(parser(Partial::new("Something")), Err(ErrMode::Backtrack(InputError::new(Partial::new("Something"), ErrorKind::Tag))));
/// assert_eq!(parser(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(5))));
/// ```
#[inline(always)]
#[doc(alias = "literal")]
#[doc(alias = "bytes")]
#[doc(alias = "just")]
#[deprecated(since = "0.5.20", note = "Replaced with `tag(ascii::Caseless(_))`")]
pub fn tag_no_case<T, I, Error: ParserError<I>>(
    tag: T,
) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + Compare<T>,
    T: SliceLen + Clone,
{
    trace("tag_no_case", move |i: &mut I| {
        let t = tag.clone();
        if <I as StreamIsPartial>::is_partial_supported() {
            tag_no_case_::<_, _, _, true>(i, t)
        } else {
            tag_no_case_::<_, _, _, false>(i, t)
        }
    })
}

#[allow(deprecated)]
fn tag_no_case_<T, I, Error: ParserError<I>, const PARTIAL: bool>(
    i: &mut I,
    t: T,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + Compare<T>,
    T: SliceLen,
{
    let tag_len = t.slice_len();

    match i.compare_no_case(t) {
        CompareResult::Ok => Ok(i.next_slice(tag_len)),
        CompareResult::Incomplete if PARTIAL && i.is_partial() => {
            Err(ErrMode::Incomplete(Needed::new(tag_len - i.eof_offset())))
        }
        CompareResult::Incomplete | CompareResult::Error => {
            let e: ErrorKind = ErrorKind::Tag;
            Err(ErrMode::from_error_kind(i, e))
        }
    }
}

/// Recognize a token that matches the [pattern][ContainsToken]
///
/// **Note:** [`Parser`] is implemented as a convenience (complete
/// only) for
/// - `u8`
/// - `char`
///
/// *Complete version*: Will return an error if there's not enough input data.
///
/// *Partial version*: Will return `Err(winnow::error::ErrMode::Incomplete(_))` if there's not enough input data.
///
/// # Example
///
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError};
/// # use winnow::token::one_of;
/// assert_eq!(one_of::<_, _, InputError<_>>(['a', 'b', 'c']).parse_peek("b"), Ok(("", 'b')));
/// assert_eq!(one_of::<_, _, InputError<_>>('a').parse_peek("bc"), Err(ErrMode::Backtrack(InputError::new("bc", ErrorKind::Verify))));
/// assert_eq!(one_of::<_, _, InputError<_>>('a').parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Token))));
///
/// fn parser_fn(i: &str) -> IResult<&str, char> {
///     one_of(|c| c == 'a' || c == 'b').parse_peek(i)
/// }
/// assert_eq!(parser_fn("abc"), Ok(("bc", 'a')));
/// assert_eq!(parser_fn("cd"), Err(ErrMode::Backtrack(InputError::new("cd", ErrorKind::Verify))));
/// assert_eq!(parser_fn(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Token))));
/// ```
///
/// ```
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::Partial;
/// # use winnow::token::one_of;
/// assert_eq!(one_of::<_, _, InputError<_>>(['a', 'b', 'c']).parse_peek(Partial::new("b")), Ok((Partial::new(""), 'b')));
/// assert_eq!(one_of::<_, _, InputError<_>>('a').parse_peek(Partial::new("bc")), Err(ErrMode::Backtrack(InputError::new(Partial::new("bc"), ErrorKind::Verify))));
/// assert_eq!(one_of::<_, _, InputError<_>>('a').parse_peek(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(1))));
///
/// fn parser_fn(i: Partial<&str>) -> IResult<Partial<&str>, char> {
///     one_of(|c| c == 'a' || c == 'b').parse_peek(i)
/// }
/// assert_eq!(parser_fn(Partial::new("abc")), Ok((Partial::new("bc"), 'a')));
/// assert_eq!(parser_fn(Partial::new("cd")), Err(ErrMode::Backtrack(InputError::new(Partial::new("cd"), ErrorKind::Verify))));
/// assert_eq!(parser_fn(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(1))));
/// ```
#[inline(always)]
#[doc(alias = "char")]
#[doc(alias = "token")]
#[doc(alias = "satisfy")]
pub fn one_of<I, T, Error: ParserError<I>>(list: T) -> impl Parser<I, <I as Stream>::Token, Error>
where
    I: StreamIsPartial,
    I: Stream,
    <I as Stream>::Token: Clone,
    T: ContainsToken<<I as Stream>::Token>,
{
    trace(
        "one_of",
        any.verify(move |t: &<I as Stream>::Token| list.contains_token(t.clone())),
    )
}

/// Recognize a token that does not match the [pattern][ContainsToken]
///
/// *Complete version*: Will return an error if there's not enough input data.
///
/// *Partial version*: Will return `Err(winnow::error::ErrMode::Incomplete(_))` if there's not enough input data.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError};
/// # use winnow::prelude::*;
/// # use winnow::token::none_of;
/// assert_eq!(none_of::<_, _, InputError<_>>(['a', 'b', 'c']).parse_peek("z"), Ok(("", 'z')));
/// assert_eq!(none_of::<_, _, InputError<_>>(['a', 'b']).parse_peek("a"), Err(ErrMode::Backtrack(InputError::new("a", ErrorKind::Verify))));
/// assert_eq!(none_of::<_, _, InputError<_>>('a').parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Token))));
/// ```
///
/// ```
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// # use winnow::token::none_of;
/// assert_eq!(none_of::<_, _, InputError<_>>(['a', 'b', 'c']).parse_peek(Partial::new("z")), Ok((Partial::new(""), 'z')));
/// assert_eq!(none_of::<_, _, InputError<_>>(['a', 'b']).parse_peek(Partial::new("a")), Err(ErrMode::Backtrack(InputError::new(Partial::new("a"), ErrorKind::Verify))));
/// assert_eq!(none_of::<_, _, InputError<_>>('a').parse_peek(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(1))));
/// ```
#[inline(always)]
pub fn none_of<I, T, Error: ParserError<I>>(list: T) -> impl Parser<I, <I as Stream>::Token, Error>
where
    I: StreamIsPartial,
    I: Stream,
    <I as Stream>::Token: Clone,
    T: ContainsToken<<I as Stream>::Token>,
{
    trace(
        "none_of",
        any.verify(move |t: &<I as Stream>::Token| !list.contains_token(t.clone())),
    )
}

/// Recognize the longest (m <= len <= n) input slice that matches the [pattern][ContainsToken]
///
/// It will return an `ErrMode::Backtrack(InputError::new(_, ErrorKind::Slice))` if the pattern wasn't met or is out
/// of range (m <= len <= n).
///
/// *Partial version* will return a `ErrMode::Incomplete(Needed::new(1))` if the pattern reaches the end of the input or is too short.
///
/// To recognize a series of tokens, use [`repeat`][crate::combinator::repeat] to [`Accumulate`][crate::stream::Accumulate] into a `()` and then [`Parser::recognize`].
///
/// # Example
///
/// Zero or more tokens:
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::take_while;
/// use winnow::stream::AsChar;
///
/// fn alpha(s: &[u8]) -> IResult<&[u8], &[u8]> {
///   take_while(0.., AsChar::is_alpha).parse_peek(s)
/// }
///
/// assert_eq!(alpha(b"latin123"), Ok((&b"123"[..], &b"latin"[..])));
/// assert_eq!(alpha(b"12345"), Ok((&b"12345"[..], &b""[..])));
/// assert_eq!(alpha(b"latin"), Ok((&b""[..], &b"latin"[..])));
/// assert_eq!(alpha(b""), Ok((&b""[..], &b""[..])));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// use winnow::token::take_while;
/// use winnow::stream::AsChar;
///
/// fn alpha(s: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
///   take_while(0.., AsChar::is_alpha).parse_peek(s)
/// }
///
/// assert_eq!(alpha(Partial::new(b"latin123")), Ok((Partial::new(&b"123"[..]), &b"latin"[..])));
/// assert_eq!(alpha(Partial::new(b"12345")), Ok((Partial::new(&b"12345"[..]), &b""[..])));
/// assert_eq!(alpha(Partial::new(b"latin")), Err(ErrMode::Incomplete(Needed::new(1))));
/// assert_eq!(alpha(Partial::new(b"")), Err(ErrMode::Incomplete(Needed::new(1))));
/// ```
///
/// One or more tokens:
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::take_while;
/// use winnow::stream::AsChar;
///
/// fn alpha(s: &[u8]) -> IResult<&[u8], &[u8]> {
///   take_while(1.., AsChar::is_alpha).parse_peek(s)
/// }
///
/// assert_eq!(alpha(b"latin123"), Ok((&b"123"[..], &b"latin"[..])));
/// assert_eq!(alpha(b"latin"), Ok((&b""[..], &b"latin"[..])));
/// assert_eq!(alpha(b"12345"), Err(ErrMode::Backtrack(InputError::new(&b"12345"[..], ErrorKind::Slice))));
///
/// fn hex(s: &str) -> IResult<&str, &str> {
///   take_while(1.., ('0'..='9', 'A'..='F')).parse_peek(s)
/// }
///
/// assert_eq!(hex("123 and voila"), Ok((" and voila", "123")));
/// assert_eq!(hex("DEADBEEF and others"), Ok((" and others", "DEADBEEF")));
/// assert_eq!(hex("BADBABEsomething"), Ok(("something", "BADBABE")));
/// assert_eq!(hex("D15EA5E"), Ok(("", "D15EA5E")));
/// assert_eq!(hex(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Slice))));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// use winnow::token::take_while;
/// use winnow::stream::AsChar;
///
/// fn alpha(s: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
///   take_while(1.., AsChar::is_alpha).parse_peek(s)
/// }
///
/// assert_eq!(alpha(Partial::new(b"latin123")), Ok((Partial::new(&b"123"[..]), &b"latin"[..])));
/// assert_eq!(alpha(Partial::new(b"latin")), Err(ErrMode::Incomplete(Needed::new(1))));
/// assert_eq!(alpha(Partial::new(b"12345")), Err(ErrMode::Backtrack(InputError::new(Partial::new(&b"12345"[..]), ErrorKind::Slice))));
///
/// fn hex(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   take_while(1.., ('0'..='9', 'A'..='F')).parse_peek(s)
/// }
///
/// assert_eq!(hex(Partial::new("123 and voila")), Ok((Partial::new(" and voila"), "123")));
/// assert_eq!(hex(Partial::new("DEADBEEF and others")), Ok((Partial::new(" and others"), "DEADBEEF")));
/// assert_eq!(hex(Partial::new("BADBABEsomething")), Ok((Partial::new("something"), "BADBABE")));
/// assert_eq!(hex(Partial::new("D15EA5E")), Err(ErrMode::Incomplete(Needed::new(1))));
/// assert_eq!(hex(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(1))));
/// ```
///
/// Arbitrary amount of tokens:
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::take_while;
/// use winnow::stream::AsChar;
///
/// fn short_alpha(s: &[u8]) -> IResult<&[u8], &[u8]> {
///   take_while(3..=6, AsChar::is_alpha).parse_peek(s)
/// }
///
/// assert_eq!(short_alpha(b"latin123"), Ok((&b"123"[..], &b"latin"[..])));
/// assert_eq!(short_alpha(b"lengthy"), Ok((&b"y"[..], &b"length"[..])));
/// assert_eq!(short_alpha(b"latin"), Ok((&b""[..], &b"latin"[..])));
/// assert_eq!(short_alpha(b"ed"), Err(ErrMode::Backtrack(InputError::new(&b"ed"[..], ErrorKind::Slice))));
/// assert_eq!(short_alpha(b"12345"), Err(ErrMode::Backtrack(InputError::new(&b"12345"[..], ErrorKind::Slice))));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// use winnow::token::take_while;
/// use winnow::stream::AsChar;
///
/// fn short_alpha(s: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
///   take_while(3..=6, AsChar::is_alpha).parse_peek(s)
/// }
///
/// assert_eq!(short_alpha(Partial::new(b"latin123")), Ok((Partial::new(&b"123"[..]), &b"latin"[..])));
/// assert_eq!(short_alpha(Partial::new(b"lengthy")), Ok((Partial::new(&b"y"[..]), &b"length"[..])));
/// assert_eq!(short_alpha(Partial::new(b"latin")), Err(ErrMode::Incomplete(Needed::new(1))));
/// assert_eq!(short_alpha(Partial::new(b"ed")), Err(ErrMode::Incomplete(Needed::new(1))));
/// assert_eq!(short_alpha(Partial::new(b"12345")), Err(ErrMode::Backtrack(InputError::new(Partial::new(&b"12345"[..]), ErrorKind::Slice))));
/// ```
#[inline(always)]
#[doc(alias = "is_a")]
#[doc(alias = "take_while0")]
#[doc(alias = "take_while1")]
pub fn take_while<T, I, Error: ParserError<I>>(
    range: impl Into<Range>,
    list: T,
) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
    T: ContainsToken<<I as Stream>::Token>,
{
    let Range {
        start_inclusive,
        end_inclusive,
    } = range.into();
    trace("take_while", move |i: &mut I| {
        match (start_inclusive, end_inclusive) {
            (0, None) => {
                if <I as StreamIsPartial>::is_partial_supported() {
                    take_while0_::<_, _, _, true>(i, &list)
                } else {
                    take_while0_::<_, _, _, false>(i, &list)
                }
            }
            (1, None) => {
                if <I as StreamIsPartial>::is_partial_supported() {
                    take_while1_::<_, _, _, true>(i, &list)
                } else {
                    take_while1_::<_, _, _, false>(i, &list)
                }
            }
            (start, end) => {
                let end = end.unwrap_or(usize::MAX);
                if <I as StreamIsPartial>::is_partial_supported() {
                    take_while_m_n_::<_, _, _, true>(i, start, end, &list)
                } else {
                    take_while_m_n_::<_, _, _, false>(i, start, end, &list)
                }
            }
        }
    })
}

fn take_while0_<T, I, Error: ParserError<I>, const PARTIAL: bool>(
    input: &mut I,
    list: &T,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
    T: ContainsToken<<I as Stream>::Token>,
{
    if PARTIAL && input.is_partial() {
        take_till0_partial(input, |c| !list.contains_token(c))
    } else {
        take_till0_complete(input, |c| !list.contains_token(c))
    }
}

fn take_while1_<T, I, Error: ParserError<I>, const PARTIAL: bool>(
    input: &mut I,
    list: &T,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
    T: ContainsToken<<I as Stream>::Token>,
{
    if PARTIAL && input.is_partial() {
        take_till1_partial(input, |c| !list.contains_token(c))
    } else {
        take_till1_complete(input, |c| !list.contains_token(c))
    }
}

fn take_while_m_n_<T, I, Error: ParserError<I>, const PARTIAL: bool>(
    input: &mut I,
    m: usize,
    n: usize,
    list: &T,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
    T: ContainsToken<<I as Stream>::Token>,
{
    take_till_m_n::<_, _, _, PARTIAL>(input, m, n, |c| !list.contains_token(c))
}

/// Looks for the first element of the input type for which the condition returns true,
/// and returns the input up to this position.
///
/// *Partial version*: If no element is found matching the condition, this will return `Incomplete`
fn take_till0_partial<P, I: Stream, E: ParserError<I>>(
    input: &mut I,
    predicate: P,
) -> PResult<<I as Stream>::Slice, E>
where
    P: Fn(I::Token) -> bool,
{
    let offset = input
        .offset_for(predicate)
        .ok_or_else(|| ErrMode::Incomplete(Needed::new(1)))?;
    Ok(input.next_slice(offset))
}

/// Looks for the first element of the input type for which the condition returns true
/// and returns the input up to this position.
///
/// Fails if the produced slice is empty.
///
/// *Partial version*: If no element is found matching the condition, this will return `Incomplete`
fn take_till1_partial<P, I: Stream, E: ParserError<I>>(
    input: &mut I,
    predicate: P,
) -> PResult<<I as Stream>::Slice, E>
where
    P: Fn(I::Token) -> bool,
{
    let e: ErrorKind = ErrorKind::Slice;
    let offset = input
        .offset_for(predicate)
        .ok_or_else(|| ErrMode::Incomplete(Needed::new(1)))?;
    if offset == 0 {
        Err(ErrMode::from_error_kind(input, e))
    } else {
        Ok(input.next_slice(offset))
    }
}

/// Looks for the first element of the input type for which the condition returns true,
/// and returns the input up to this position.
///
/// *Complete version*: If no element is found matching the condition, this will return the whole input
fn take_till0_complete<P, I: Stream, E: ParserError<I>>(
    input: &mut I,
    predicate: P,
) -> PResult<<I as Stream>::Slice, E>
where
    P: Fn(I::Token) -> bool,
{
    let offset = input
        .offset_for(predicate)
        .unwrap_or_else(|| input.eof_offset());
    Ok(input.next_slice(offset))
}

/// Looks for the first element of the input type for which the condition returns true
/// and returns the input up to this position.
///
/// Fails if the produced slice is empty.
///
/// *Complete version*: If no element is found matching the condition, this will return the whole input
fn take_till1_complete<P, I: Stream, E: ParserError<I>>(
    input: &mut I,
    predicate: P,
) -> PResult<<I as Stream>::Slice, E>
where
    P: Fn(I::Token) -> bool,
{
    let e: ErrorKind = ErrorKind::Slice;
    let offset = input
        .offset_for(predicate)
        .unwrap_or_else(|| input.eof_offset());
    if offset == 0 {
        Err(ErrMode::from_error_kind(input, e))
    } else {
        Ok(input.next_slice(offset))
    }
}

fn take_till_m_n<P, I, Error: ParserError<I>, const PARTIAL: bool>(
    input: &mut I,
    m: usize,
    n: usize,
    predicate: P,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
    P: Fn(I::Token) -> bool,
{
    if n < m {
        return Err(ErrMode::assert(input, "`m` should be <= `n`"));
    }

    let mut final_count = 0;
    for (processed, (offset, token)) in input.iter_offsets().enumerate() {
        if predicate(token) {
            if processed < m {
                return Err(ErrMode::from_error_kind(input, ErrorKind::Slice));
            } else {
                return Ok(input.next_slice(offset));
            }
        } else {
            if processed == n {
                return Ok(input.next_slice(offset));
            }
            final_count = processed + 1;
        }
    }
    if PARTIAL && input.is_partial() {
        if final_count == n {
            Ok(input.finish())
        } else {
            let needed = if m > input.eof_offset() {
                m - input.eof_offset()
            } else {
                1
            };
            Err(ErrMode::Incomplete(Needed::new(needed)))
        }
    } else {
        if m <= final_count {
            Ok(input.finish())
        } else {
            Err(ErrMode::from_error_kind(input, ErrorKind::Slice))
        }
    }
}

/// Recognize the longest input slice (if any) till a [pattern][ContainsToken] is met.
///
/// *Partial version* will return a `ErrMode::Incomplete(Needed::new(1))` if the match reaches the
/// end of input or if there was not match.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::take_till;
///
/// fn till_colon(s: &str) -> IResult<&str, &str> {
///   take_till(0.., |c| c == ':').parse_peek(s)
/// }
///
/// assert_eq!(till_colon("latin:123"), Ok((":123", "latin")));
/// assert_eq!(till_colon(":empty matched"), Ok((":empty matched", ""))); //allowed
/// assert_eq!(till_colon("12345"), Ok(("", "12345")));
/// assert_eq!(till_colon(""), Ok(("", "")));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// use winnow::token::take_till;
///
/// fn till_colon(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   take_till(0.., |c| c == ':').parse_peek(s)
/// }
///
/// assert_eq!(till_colon(Partial::new("latin:123")), Ok((Partial::new(":123"), "latin")));
/// assert_eq!(till_colon(Partial::new(":empty matched")), Ok((Partial::new(":empty matched"), ""))); //allowed
/// assert_eq!(till_colon(Partial::new("12345")), Err(ErrMode::Incomplete(Needed::new(1))));
/// assert_eq!(till_colon(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(1))));
/// ```
#[inline(always)]
#[doc(alias = "is_not")]
pub fn take_till<T, I, Error: ParserError<I>>(
    range: impl Into<Range>,
    list: T,
) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
    T: ContainsToken<<I as Stream>::Token>,
{
    let Range {
        start_inclusive,
        end_inclusive,
    } = range.into();
    trace("take_till", move |i: &mut I| {
        match (start_inclusive, end_inclusive) {
            (0, None) => {
                if <I as StreamIsPartial>::is_partial_supported() {
                    take_till0_partial(i, |c| list.contains_token(c))
                } else {
                    take_till0_complete(i, |c| list.contains_token(c))
                }
            }
            (1, None) => {
                if <I as StreamIsPartial>::is_partial_supported() {
                    take_till1_partial(i, |c| list.contains_token(c))
                } else {
                    take_till1_complete(i, |c| list.contains_token(c))
                }
            }
            (start, end) => {
                let end = end.unwrap_or(usize::MAX);
                if <I as StreamIsPartial>::is_partial_supported() {
                    take_till_m_n::<_, _, _, true>(i, start, end, |c| list.contains_token(c))
                } else {
                    take_till_m_n::<_, _, _, false>(i, start, end, |c| list.contains_token(c))
                }
            }
        }
    })
}

/// Recognize the longest input slice (if any) till a [pattern][ContainsToken] is met.
///
/// *Partial version* will return a `ErrMode::Incomplete(Needed::new(1))` if the match reaches the
/// end of input or if there was not match.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::take_till0;
///
/// fn till_colon(s: &str) -> IResult<&str, &str> {
///   take_till0(|c| c == ':').parse_peek(s)
/// }
///
/// assert_eq!(till_colon("latin:123"), Ok((":123", "latin")));
/// assert_eq!(till_colon(":empty matched"), Ok((":empty matched", ""))); //allowed
/// assert_eq!(till_colon("12345"), Ok(("", "12345")));
/// assert_eq!(till_colon(""), Ok(("", "")));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// use winnow::token::take_till0;
///
/// fn till_colon(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   take_till0(|c| c == ':').parse_peek(s)
/// }
///
/// assert_eq!(till_colon(Partial::new("latin:123")), Ok((Partial::new(":123"), "latin")));
/// assert_eq!(till_colon(Partial::new(":empty matched")), Ok((Partial::new(":empty matched"), ""))); //allowed
/// assert_eq!(till_colon(Partial::new("12345")), Err(ErrMode::Incomplete(Needed::new(1))));
/// assert_eq!(till_colon(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(1))));
/// ```
#[deprecated(since = "0.5.21", note = "Replaced with `take_till(0.., ...)`")]
#[inline(always)]
pub fn take_till0<T, I, Error: ParserError<I>>(
    list: T,
) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
    T: ContainsToken<<I as Stream>::Token>,
{
    trace("take_till0", move |i: &mut I| {
        if <I as StreamIsPartial>::is_partial_supported() && i.is_partial() {
            take_till0_partial(i, |c| list.contains_token(c))
        } else {
            take_till0_complete(i, |c| list.contains_token(c))
        }
    })
}

/// Recognize the longest (at least 1) input slice till a [pattern][ContainsToken] is met.
///
/// It will return `Err(ErrMode::Backtrack(InputError::new(_, ErrorKind::Slice)))` if the input is empty or the
/// predicate matches the first input.
///
/// *Partial version* will return a `ErrMode::Incomplete(Needed::new(1))` if the match reaches the
/// end of input or if there was not match.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::take_till1;
///
/// fn till_colon(s: &str) -> IResult<&str, &str> {
///   take_till1(|c| c == ':').parse_peek(s)
/// }
///
/// assert_eq!(till_colon("latin:123"), Ok((":123", "latin")));
/// assert_eq!(till_colon(":empty matched"), Err(ErrMode::Backtrack(InputError::new(":empty matched", ErrorKind::Slice))));
/// assert_eq!(till_colon("12345"), Ok(("", "12345")));
/// assert_eq!(till_colon(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Slice))));
///
/// fn not_space(s: &str) -> IResult<&str, &str> {
///   take_till1([' ', '\t', '\r', '\n']).parse_peek(s)
/// }
///
/// assert_eq!(not_space("Hello, World!"), Ok((" World!", "Hello,")));
/// assert_eq!(not_space("Sometimes\t"), Ok(("\t", "Sometimes")));
/// assert_eq!(not_space("Nospace"), Ok(("", "Nospace")));
/// assert_eq!(not_space(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Slice))));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// use winnow::token::take_till1;
///
/// fn till_colon(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   take_till1(|c| c == ':').parse_peek(s)
/// }
///
/// assert_eq!(till_colon(Partial::new("latin:123")), Ok((Partial::new(":123"), "latin")));
/// assert_eq!(till_colon(Partial::new(":empty matched")), Err(ErrMode::Backtrack(InputError::new(Partial::new(":empty matched"), ErrorKind::Slice))));
/// assert_eq!(till_colon(Partial::new("12345")), Err(ErrMode::Incomplete(Needed::new(1))));
/// assert_eq!(till_colon(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(1))));
///
/// fn not_space(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   take_till1([' ', '\t', '\r', '\n']).parse_peek(s)
/// }
///
/// assert_eq!(not_space(Partial::new("Hello, World!")), Ok((Partial::new(" World!"), "Hello,")));
/// assert_eq!(not_space(Partial::new("Sometimes\t")), Ok((Partial::new("\t"), "Sometimes")));
/// assert_eq!(not_space(Partial::new("Nospace")), Err(ErrMode::Incomplete(Needed::new(1))));
/// assert_eq!(not_space(Partial::new("")), Err(ErrMode::Incomplete(Needed::new(1))));
/// ```
#[inline(always)]
#[deprecated(since = "0.5.21", note = "Replaced with `take_till(1.., ...)`")]
pub fn take_till1<T, I, Error: ParserError<I>>(
    list: T,
) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
    T: ContainsToken<<I as Stream>::Token>,
{
    trace("take_till1", move |i: &mut I| {
        if <I as StreamIsPartial>::is_partial_supported() && i.is_partial() {
            take_till1_partial(i, |c| list.contains_token(c))
        } else {
            take_till1_complete(i, |c| list.contains_token(c))
        }
    })
}

/// Recognize an input slice containing the first N input elements (I[..N]).
///
/// *Complete version*: It will return `Err(ErrMode::Backtrack(InputError::new(_, ErrorKind::Slice)))` if the input is shorter than the argument.
///
/// *Partial version*: if the input has less than N elements, `take` will
/// return a `ErrMode::Incomplete(Needed::new(M))` where M is the number of
/// additional bytes the parser would need to succeed.
/// It is well defined for `&[u8]` as the number of elements is the byte size,
/// but for types like `&str`, we cannot know how many bytes correspond for
/// the next few chars, so the result will be `ErrMode::Incomplete(Needed::Unknown)`
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::take;
///
/// fn take6(s: &str) -> IResult<&str, &str> {
///   take(6usize).parse_peek(s)
/// }
///
/// assert_eq!(take6("1234567"), Ok(("7", "123456")));
/// assert_eq!(take6("things"), Ok(("", "things")));
/// assert_eq!(take6("short"), Err(ErrMode::Backtrack(InputError::new("short", ErrorKind::Slice))));
/// assert_eq!(take6(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Slice))));
/// ```
///
/// The units that are taken will depend on the input type. For example, for a
/// `&str` it will take a number of `char`'s, whereas for a `&[u8]` it will
/// take that many `u8`'s:
///
/// ```rust
/// # use winnow::prelude::*;
/// use winnow::error::InputError;
/// use winnow::token::take;
///
/// assert_eq!(take::<_, _, InputError<_>>(1usize).parse_peek("💙"), Ok(("", "💙")));
/// assert_eq!(take::<_, _, InputError<_>>(1usize).parse_peek("💙".as_bytes()), Ok((b"\x9F\x92\x99".as_ref(), b"\xF0".as_ref())));
/// ```
///
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::Partial;
/// use winnow::token::take;
///
/// fn take6(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   take(6usize).parse_peek(s)
/// }
///
/// assert_eq!(take6(Partial::new("1234567")), Ok((Partial::new("7"), "123456")));
/// assert_eq!(take6(Partial::new("things")), Ok((Partial::new(""), "things")));
/// // `Unknown` as we don't know the number of bytes that `count` corresponds to
/// assert_eq!(take6(Partial::new("short")), Err(ErrMode::Incomplete(Needed::Unknown)));
/// ```
#[inline(always)]
pub fn take<C, I, Error: ParserError<I>>(count: C) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
    C: ToUsize,
{
    let c = count.to_usize();
    trace("take", move |i: &mut I| {
        if <I as StreamIsPartial>::is_partial_supported() {
            take_::<_, _, true>(i, c)
        } else {
            take_::<_, _, false>(i, c)
        }
    })
}

fn take_<I, Error: ParserError<I>, const PARTIAL: bool>(
    i: &mut I,
    c: usize,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream,
{
    match i.offset_at(c) {
        Ok(offset) => Ok(i.next_slice(offset)),
        Err(e) if PARTIAL && i.is_partial() => Err(ErrMode::Incomplete(e)),
        Err(_needed) => Err(ErrMode::from_error_kind(i, ErrorKind::Slice)),
    }
}

/// Recognize the input slice up to the first occurrence of the literal.
///
/// It doesn't consume the pattern.
///
/// *Complete version*: It will return `Err(ErrMode::Backtrack(InputError::new(_, ErrorKind::Slice)))`
/// if the pattern wasn't met.
///
/// *Partial version*: will return a `ErrMode::Incomplete(Needed::new(N))` if the input doesn't
/// contain the pattern or if the input is smaller than the pattern.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::take_until;
///
/// fn until_eof(s: &str) -> IResult<&str, &str> {
///   take_until(0.., "eof").parse_peek(s)
/// }
///
/// assert_eq!(until_eof("hello, worldeof"), Ok(("eof", "hello, world")));
/// assert_eq!(until_eof("hello, world"), Err(ErrMode::Backtrack(InputError::new("hello, world", ErrorKind::Slice))));
/// assert_eq!(until_eof(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Slice))));
/// assert_eq!(until_eof("1eof2eof"), Ok(("eof2eof", "1")));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// use winnow::token::take_until;
///
/// fn until_eof(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   take_until(0.., "eof").parse_peek(s)
/// }
///
/// assert_eq!(until_eof(Partial::new("hello, worldeof")), Ok((Partial::new("eof"), "hello, world")));
/// assert_eq!(until_eof(Partial::new("hello, world")), Err(ErrMode::Incomplete(Needed::Unknown)));
/// assert_eq!(until_eof(Partial::new("hello, worldeo")), Err(ErrMode::Incomplete(Needed::Unknown)));
/// assert_eq!(until_eof(Partial::new("1eof2eof")), Ok((Partial::new("eof2eof"), "1")));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::token::take_until;
///
/// fn until_eof(s: &str) -> IResult<&str, &str> {
///   take_until(1.., "eof").parse_peek(s)
/// }
///
/// assert_eq!(until_eof("hello, worldeof"), Ok(("eof", "hello, world")));
/// assert_eq!(until_eof("hello, world"), Err(ErrMode::Backtrack(InputError::new("hello, world", ErrorKind::Slice))));
/// assert_eq!(until_eof(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Slice))));
/// assert_eq!(until_eof("1eof2eof"), Ok(("eof2eof", "1")));
/// assert_eq!(until_eof("eof"), Err(ErrMode::Backtrack(InputError::new("eof", ErrorKind::Slice))));
/// ```
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::Partial;
/// use winnow::token::take_until;
///
/// fn until_eof(s: Partial<&str>) -> IResult<Partial<&str>, &str> {
///   take_until(1.., "eof").parse_peek(s)
/// }
///
/// assert_eq!(until_eof(Partial::new("hello, worldeof")), Ok((Partial::new("eof"), "hello, world")));
/// assert_eq!(until_eof(Partial::new("hello, world")), Err(ErrMode::Incomplete(Needed::Unknown)));
/// assert_eq!(until_eof(Partial::new("hello, worldeo")), Err(ErrMode::Incomplete(Needed::Unknown)));
/// assert_eq!(until_eof(Partial::new("1eof2eof")), Ok((Partial::new("eof2eof"), "1")));
/// assert_eq!(until_eof(Partial::new("eof")), Err(ErrMode::Backtrack(InputError::new(Partial::new("eof"), ErrorKind::Slice))));
/// ```
#[inline(always)]
pub fn take_until<T, I, Error: ParserError<I>>(
    range: impl Into<Range>,
    tag: T,
) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + FindSlice<T>,
    T: SliceLen + Clone,
{
    let Range {
        start_inclusive,
        end_inclusive,
    } = range.into();
    trace("take_until", move |i: &mut I| {
        match (start_inclusive, end_inclusive) {
            (0, None) => {
                if <I as StreamIsPartial>::is_partial_supported() {
                    take_until0_::<_, _, _, true>(i, tag.clone())
                } else {
                    take_until0_::<_, _, _, false>(i, tag.clone())
                }
            }
            (1, None) => {
                if <I as StreamIsPartial>::is_partial_supported() {
                    take_until1_::<_, _, _, true>(i, tag.clone())
                } else {
                    take_until1_::<_, _, _, false>(i, tag.clone())
                }
            }
            (start, end) => {
                let end = end.unwrap_or(usize::MAX);
                if <I as StreamIsPartial>::is_partial_supported() {
                    take_until_m_n_::<_, _, _, true>(i, start, end, tag.clone())
                } else {
                    take_until_m_n_::<_, _, _, false>(i, start, end, tag.clone())
                }
            }
        }
    })
}

/// Deprecated, see [`take_until`]
#[deprecated(since = "0.5.35", note = "Replaced with `take_until`")]
pub fn take_until0<T, I, Error: ParserError<I>>(
    tag: T,
) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + FindSlice<T>,
    T: SliceLen + Clone,
{
    take_until(0.., tag)
}

fn take_until0_<T, I, Error: ParserError<I>, const PARTIAL: bool>(
    i: &mut I,
    t: T,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + FindSlice<T>,
    T: SliceLen,
{
    match i.find_slice(t) {
        Some(offset) => Ok(i.next_slice(offset)),
        None if PARTIAL && i.is_partial() => Err(ErrMode::Incomplete(Needed::Unknown)),
        None => Err(ErrMode::from_error_kind(i, ErrorKind::Slice)),
    }
}

/// Deprecated, see [`take_until`]
#[deprecated(since = "0.5.35", note = "Replaced with `take_until`")]
#[inline(always)]
pub fn take_until1<T, I, Error: ParserError<I>>(
    tag: T,
) -> impl Parser<I, <I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + FindSlice<T>,
    T: SliceLen + Clone,
{
    take_until(1.., tag)
}

fn take_until1_<T, I, Error: ParserError<I>, const PARTIAL: bool>(
    i: &mut I,
    t: T,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + FindSlice<T>,
    T: SliceLen,
{
    match i.find_slice(t) {
        None if PARTIAL && i.is_partial() => Err(ErrMode::Incomplete(Needed::Unknown)),
        None | Some(0) => Err(ErrMode::from_error_kind(i, ErrorKind::Slice)),
        Some(offset) => Ok(i.next_slice(offset)),
    }
}

fn take_until_m_n_<T, I, Error: ParserError<I>, const PARTIAL: bool>(
    i: &mut I,
    start: usize,
    end: usize,
    t: T,
) -> PResult<<I as Stream>::Slice, Error>
where
    I: StreamIsPartial,
    I: Stream + FindSlice<T>,
    T: SliceLen,
{
    if end < start {
        return Err(ErrMode::assert(i, "`start` should be <= `end`"));
    }

    match i.find_slice(t) {
        Some(offset) => {
            let start_offset = i.offset_at(start);
            let end_offset = i.offset_at(end).unwrap_or_else(|_err| i.eof_offset());
            if start_offset.map(|s| offset < s).unwrap_or(true) {
                if PARTIAL && i.is_partial() {
                    return Err(ErrMode::Incomplete(Needed::Unknown));
                } else {
                    return Err(ErrMode::from_error_kind(i, ErrorKind::Slice));
                }
            }
            if end_offset < offset {
                return Err(ErrMode::from_error_kind(i, ErrorKind::Slice));
            }
            Ok(i.next_slice(offset))
        }
        None if PARTIAL && i.is_partial() => Err(ErrMode::Incomplete(Needed::Unknown)),
        None => Err(ErrMode::from_error_kind(i, ErrorKind::Slice)),
    }
}
