use crate::combinator::trace;
use crate::error::ParserError;
use crate::stream::Stream;
use crate::*;

#[doc(inline)]
pub use crate::seq;

/// Sequence two parsers, only returning the output from the second.
///
/// # Arguments
/// * `first` The opening parser.
/// * `second` The second parser to get object.
///
/// See also [`seq`] to generalize this across any number of fields.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::error::Needed::Size;
/// use winnow::combinator::preceded;
/// use winnow::token::tag;
///
/// let mut parser = preceded("abc", "efg");
///
/// assert_eq!(parser.parse_peek("abcefg"), Ok(("", "efg")));
/// assert_eq!(parser.parse_peek("abcefghij"), Ok(("hij", "efg")));
/// assert_eq!(parser.parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// assert_eq!(parser.parse_peek("123"), Err(ErrMode::Backtrack(InputError::new("123", ErrorKind::Tag))));
/// ```
#[doc(alias = "ignore_then")]
pub fn preceded<I, O1, O2, E: ParserError<I>, F, G>(
    mut first: F,
    mut second: G,
) -> impl Parser<I, O2, E>
where
    I: Stream,
    F: Parser<I, O1, E>,
    G: Parser<I, O2, E>,
{
    trace("preceded", move |input: &mut I| {
        let _ = first.parse_next(input)?;
        second.parse_next(input)
    })
}

/// Sequence two parsers, only returning the output of the first.
///
/// # Arguments
/// * `first` The first parser to apply.
/// * `second` The second parser to match an object.
///
/// See also [`seq`] to generalize this across any number of fields.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::prelude::*;
/// # use winnow::error::Needed::Size;
/// use winnow::combinator::terminated;
/// use winnow::token::tag;
///
/// let mut parser = terminated("abc", "efg");
///
/// assert_eq!(parser.parse_peek("abcefg"), Ok(("", "abc")));
/// assert_eq!(parser.parse_peek("abcefghij"), Ok(("hij", "abc")));
/// assert_eq!(parser.parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// assert_eq!(parser.parse_peek("123"), Err(ErrMode::Backtrack(InputError::new("123", ErrorKind::Tag))));
/// ```
#[doc(alias = "then_ignore")]
pub fn terminated<I, O1, O2, E: ParserError<I>, F, G>(
    mut first: F,
    mut second: G,
) -> impl Parser<I, O1, E>
where
    I: Stream,
    F: Parser<I, O1, E>,
    G: Parser<I, O2, E>,
{
    trace("terminated", move |input: &mut I| {
        let o1 = first.parse_next(input)?;
        second.parse_next(input).map(|_| o1)
    })
}

/// Sequence three parsers, only returning the values of the first and third.
///
/// # Arguments
/// * `first` The first parser to apply.
/// * `sep` The separator parser to apply.
/// * `second` The second parser to apply.
///
/// See also [`seq`] to generalize this across any number of fields.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::error::Needed::Size;
/// # use winnow::prelude::*;
/// use winnow::combinator::separated_pair;
/// use winnow::token::tag;
///
/// let mut parser = separated_pair("abc", "|", "efg");
///
/// assert_eq!(parser.parse_peek("abc|efg"), Ok(("", ("abc", "efg"))));
/// assert_eq!(parser.parse_peek("abc|efghij"), Ok(("hij", ("abc", "efg"))));
/// assert_eq!(parser.parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// assert_eq!(parser.parse_peek("123"), Err(ErrMode::Backtrack(InputError::new("123", ErrorKind::Tag))));
/// ```
pub fn separated_pair<I, O1, O2, O3, E: ParserError<I>, F, G, H>(
    mut first: F,
    mut sep: G,
    mut second: H,
) -> impl Parser<I, (O1, O3), E>
where
    I: Stream,
    F: Parser<I, O1, E>,
    G: Parser<I, O2, E>,
    H: Parser<I, O3, E>,
{
    trace("separated_pair", move |input: &mut I| {
        let o1 = first.parse_next(input)?;
        let _ = sep.parse_next(input)?;
        second.parse_next(input).map(|o2| (o1, o2))
    })
}

/// Sequence three parsers, only returning the output of the second.
///
/// # Arguments
/// * `first` The first parser to apply and discard.
/// * `second` The second parser to apply.
/// * `third` The third parser to apply and discard.
///
/// See also [`seq`] to generalize this across any number of fields.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, error::Needed};
/// # use winnow::error::Needed::Size;
/// # use winnow::prelude::*;
/// use winnow::combinator::delimited;
/// use winnow::token::tag;
///
/// let mut parser = delimited("(", "abc", ")");
///
/// assert_eq!(parser.parse_peek("(abc)"), Ok(("", "abc")));
/// assert_eq!(parser.parse_peek("(abc)def"), Ok(("def", "abc")));
/// assert_eq!(parser.parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// assert_eq!(parser.parse_peek("123"), Err(ErrMode::Backtrack(InputError::new("123", ErrorKind::Tag))));
/// ```
#[doc(alias = "between")]
#[doc(alias = "padded")]
pub fn delimited<I, O1, O2, O3, E: ParserError<I>, F, G, H>(
    mut first: F,
    mut second: G,
    mut third: H,
) -> impl Parser<I, O2, E>
where
    I: Stream,
    F: Parser<I, O1, E>,
    G: Parser<I, O2, E>,
    H: Parser<I, O3, E>,
{
    trace("delimited", move |input: &mut I| {
        let _ = first.parse_next(input)?;
        let o2 = second.parse_next(input)?;
        third.parse_next(input).map(|_| o2)
    })
}
