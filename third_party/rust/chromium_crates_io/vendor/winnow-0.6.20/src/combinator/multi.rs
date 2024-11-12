//! Combinators applying their child parser multiple times

use crate::combinator::trace;
use crate::error::ErrMode;
use crate::error::ErrorKind;
use crate::error::ParserError;
use crate::stream::Accumulate;
use crate::stream::Range;
use crate::stream::Stream;
use crate::PResult;
use crate::Parser;

/// [`Accumulate`] the output of a parser into a container, like `Vec`
///
/// This stops before `n` when the parser returns [`ErrMode::Backtrack`]. To instead chain an error up, see
/// [`cut_err`][crate::combinator::cut_err].
///
/// To take a series of tokens, [`Accumulate`] into a `()`
/// (e.g. with [`.map(|()| ())`][Parser::map])
/// and then [`Parser::take`].
///
/// **Warning:** If the parser passed to `repeat` accepts empty inputs
/// (like `alpha0` or `digit0`), `repeat` will return an error,
/// to prevent going into an infinite loop.
///
/// # Example
///
/// Zero or more repetitions:
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::{error::ErrMode, error::ErrorKind, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::repeat;
///
/// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
///   repeat(0.., "abc").parse_peek(s)
/// }
///
/// assert_eq!(parser("abcabc"), Ok(("", vec!["abc", "abc"])));
/// assert_eq!(parser("abc123"), Ok(("123", vec!["abc"])));
/// assert_eq!(parser("123123"), Ok(("123123", vec![])));
/// assert_eq!(parser(""), Ok(("", vec![])));
/// # }
/// ```
///
/// One or more repetitions:
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::repeat;
///
/// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
///   repeat(1.., "abc").parse_peek(s)
/// }
///
/// assert_eq!(parser("abcabc"), Ok(("", vec!["abc", "abc"])));
/// assert_eq!(parser("abc123"), Ok(("123", vec!["abc"])));
/// assert_eq!(parser("123123"), Err(ErrMode::Backtrack(InputError::new("123123", ErrorKind::Tag))));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// # }
/// ```
///
/// Fixed number of repetitions:
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::repeat;
///
/// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
///   repeat(2, "abc").parse_peek(s)
/// }
///
/// assert_eq!(parser("abcabc"), Ok(("", vec!["abc", "abc"])));
/// assert_eq!(parser("abc123"), Err(ErrMode::Backtrack(InputError::new("123", ErrorKind::Tag))));
/// assert_eq!(parser("123123"), Err(ErrMode::Backtrack(InputError::new("123123", ErrorKind::Tag))));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// assert_eq!(parser("abcabcabc"), Ok(("abc", vec!["abc", "abc"])));
/// # }
/// ```
///
/// Arbitrary repetitions:
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::{error::ErrMode, error::ErrorKind, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::repeat;
///
/// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
///   repeat(0..=2, "abc").parse_peek(s)
/// }
///
/// assert_eq!(parser("abcabc"), Ok(("", vec!["abc", "abc"])));
/// assert_eq!(parser("abc123"), Ok(("123", vec!["abc"])));
/// assert_eq!(parser("123123"), Ok(("123123", vec![])));
/// assert_eq!(parser(""), Ok(("", vec![])));
/// assert_eq!(parser("abcabcabc"), Ok(("abc", vec!["abc", "abc"])));
/// # }
/// ```
#[doc(alias = "many0")]
#[doc(alias = "count")]
#[doc(alias = "many0_count")]
#[doc(alias = "many1")]
#[doc(alias = "many1_count")]
#[doc(alias = "many_m_n")]
#[doc(alias = "repeated")]
#[doc(alias = "skip_many")]
#[doc(alias = "skip_many1")]
#[inline(always)]
pub fn repeat<Input, Output, Accumulator, Error, ParseNext>(
    occurrences: impl Into<Range>,
    parser: ParseNext,
) -> Repeat<ParseNext, Input, Output, Accumulator, Error>
where
    Input: Stream,
    Accumulator: Accumulate<Output>,
    ParseNext: Parser<Input, Output, Error>,
    Error: ParserError<Input>,
{
    Repeat {
        occurrences: occurrences.into(),
        parser,
        i: Default::default(),
        o: Default::default(),
        c: Default::default(),
        e: Default::default(),
    }
}

/// Implementation of [`repeat`]
pub struct Repeat<P, I, O, C, E>
where
    P: Parser<I, O, E>,
    I: Stream,
    C: Accumulate<O>,
    E: ParserError<I>,
{
    occurrences: Range,
    parser: P,
    i: core::marker::PhantomData<I>,
    o: core::marker::PhantomData<O>,
    c: core::marker::PhantomData<C>,
    e: core::marker::PhantomData<E>,
}

impl<ParseNext, Input, Output, Error> Repeat<ParseNext, Input, Output, (), Error>
where
    ParseNext: Parser<Input, Output, Error>,
    Input: Stream,
    Error: ParserError<Input>,
{
    /// Repeats the embedded parser, calling `g` to gather the results
    ///
    /// This stops before `n` when the parser returns [`ErrMode::Backtrack`]. To instead chain an error up, see
    /// [`cut_err`][crate::combinator::cut_err].
    ///
    /// # Arguments
    /// * `init` A function returning the initial value.
    /// * `g` The function that combines a result of `f` with
    ///       the current accumulator.
    ///
    /// **Warning:** If the parser passed to `fold` accepts empty inputs
    /// (like `alpha0` or `digit0`), `fold_repeat` will return an error,
    /// to prevent going into an infinite loop.
    ///
    /// # Example
    ///
    /// Zero or more repetitions:
    /// ```rust
    /// # use winnow::{error::ErrMode, error::ErrorKind, error::Needed};
    /// # use winnow::prelude::*;
    /// use winnow::combinator::repeat;
    ///
    /// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
    ///   repeat(
    ///     0..,
    ///     "abc"
    ///   ).fold(
    ///     Vec::new,
    ///     |mut acc: Vec<_>, item| {
    ///       acc.push(item);
    ///       acc
    ///     }
    ///   ).parse_peek(s)
    /// }
    ///
    /// assert_eq!(parser("abcabc"), Ok(("", vec!["abc", "abc"])));
    /// assert_eq!(parser("abc123"), Ok(("123", vec!["abc"])));
    /// assert_eq!(parser("123123"), Ok(("123123", vec![])));
    /// assert_eq!(parser(""), Ok(("", vec![])));
    /// ```
    ///
    /// One or more repetitions:
    /// ```rust
    /// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
    /// # use winnow::prelude::*;
    /// use winnow::combinator::repeat;
    ///
    /// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
    ///   repeat(
    ///     1..,
    ///     "abc",
    ///   ).fold(
    ///     Vec::new,
    ///     |mut acc: Vec<_>, item| {
    ///       acc.push(item);
    ///       acc
    ///     }
    ///   ).parse_peek(s)
    /// }
    ///
    /// assert_eq!(parser("abcabc"), Ok(("", vec!["abc", "abc"])));
    /// assert_eq!(parser("abc123"), Ok(("123", vec!["abc"])));
    /// assert_eq!(parser("123123"), Err(ErrMode::Backtrack(InputError::new("123123", ErrorKind::Many))));
    /// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Many))));
    /// ```
    ///
    /// Arbitrary number of repetitions:
    /// ```rust
    /// # use winnow::{error::ErrMode, error::ErrorKind, error::Needed};
    /// # use winnow::prelude::*;
    /// use winnow::combinator::repeat;
    ///
    /// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
    ///   repeat(
    ///     0..=2,
    ///     "abc",
    ///   ).fold(
    ///     Vec::new,
    ///     |mut acc: Vec<_>, item| {
    ///       acc.push(item);
    ///       acc
    ///     }
    ///   ).parse_peek(s)
    /// }
    ///
    /// assert_eq!(parser("abcabc"), Ok(("", vec!["abc", "abc"])));
    /// assert_eq!(parser("abc123"), Ok(("123", vec!["abc"])));
    /// assert_eq!(parser("123123"), Ok(("123123", vec![])));
    /// assert_eq!(parser(""), Ok(("", vec![])));
    /// assert_eq!(parser("abcabcabc"), Ok(("abc", vec!["abc", "abc"])));
    /// ```
    #[doc(alias = "fold_many0")]
    #[doc(alias = "fold_many1")]
    #[doc(alias = "fold_many_m_n")]
    #[doc(alias = "fold_repeat")]
    #[inline(always)]
    pub fn fold<Init, Op, Result>(
        mut self,
        mut init: Init,
        mut op: Op,
    ) -> impl Parser<Input, Result, Error>
    where
        Init: FnMut() -> Result,
        Op: FnMut(Result, Output) -> Result,
    {
        let Range {
            start_inclusive,
            end_inclusive,
        } = self.occurrences;
        trace("repeat_fold", move |i: &mut Input| {
            match (start_inclusive, end_inclusive) {
                (0, None) => fold_repeat0_(&mut self.parser, &mut init, &mut op, i),
                (1, None) => fold_repeat1_(&mut self.parser, &mut init, &mut op, i),
                (start, end) => fold_repeat_m_n_(
                    start,
                    end.unwrap_or(usize::MAX),
                    &mut self.parser,
                    &mut init,
                    &mut op,
                    i,
                ),
            }
        })
    }
}

impl<P, I, O, C, E> Parser<I, C, E> for Repeat<P, I, O, C, E>
where
    P: Parser<I, O, E>,
    I: Stream,
    C: Accumulate<O>,
    E: ParserError<I>,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<C, E> {
        let Range {
            start_inclusive,
            end_inclusive,
        } = self.occurrences;
        trace("repeat", move |i: &mut I| {
            match (start_inclusive, end_inclusive) {
                (0, None) => repeat0_(&mut self.parser, i),
                (1, None) => repeat1_(&mut self.parser, i),
                (start, end) if Some(start) == end => repeat_n_(start, &mut self.parser, i),
                (start, end) => repeat_m_n_(start, end.unwrap_or(usize::MAX), &mut self.parser, i),
            }
        })
        .parse_next(i)
    }
}

fn repeat0_<I, O, C, E, F>(f: &mut F, i: &mut I) -> PResult<C, E>
where
    I: Stream,
    C: Accumulate<O>,
    F: Parser<I, O, E>,
    E: ParserError<I>,
{
    let mut acc = C::initial(None);
    loop {
        let start = i.checkpoint();
        let len = i.eof_offset();
        match f.parse_next(i) {
            Err(ErrMode::Backtrack(_)) => {
                i.reset(&start);
                return Ok(acc);
            }
            Err(e) => return Err(e),
            Ok(o) => {
                // infinite loop check: the parser must always consume
                if i.eof_offset() == len {
                    return Err(ErrMode::assert(i, "`repeat` parsers must always consume"));
                }

                acc.accumulate(o);
            }
        }
    }
}

fn repeat1_<I, O, C, E, F>(f: &mut F, i: &mut I) -> PResult<C, E>
where
    I: Stream,
    C: Accumulate<O>,
    F: Parser<I, O, E>,
    E: ParserError<I>,
{
    let start = i.checkpoint();
    match f.parse_next(i) {
        Err(e) => Err(e.append(i, &start, ErrorKind::Many)),
        Ok(o) => {
            let mut acc = C::initial(None);
            acc.accumulate(o);

            loop {
                let start = i.checkpoint();
                let len = i.eof_offset();
                match f.parse_next(i) {
                    Err(ErrMode::Backtrack(_)) => {
                        i.reset(&start);
                        return Ok(acc);
                    }
                    Err(e) => return Err(e),
                    Ok(o) => {
                        // infinite loop check: the parser must always consume
                        if i.eof_offset() == len {
                            return Err(ErrMode::assert(i, "`repeat` parsers must always consume"));
                        }

                        acc.accumulate(o);
                    }
                }
            }
        }
    }
}

fn repeat_n_<I, O, C, E, F>(count: usize, f: &mut F, i: &mut I) -> PResult<C, E>
where
    I: Stream,
    C: Accumulate<O>,
    F: Parser<I, O, E>,
    E: ParserError<I>,
{
    let mut res = C::initial(Some(count));

    for _ in 0..count {
        let start = i.checkpoint();
        let len = i.eof_offset();
        match f.parse_next(i) {
            Ok(o) => {
                // infinite loop check: the parser must always consume
                if i.eof_offset() == len {
                    return Err(ErrMode::assert(i, "`repeat` parsers must always consume"));
                }

                res.accumulate(o);
            }
            Err(e) => {
                return Err(e.append(i, &start, ErrorKind::Many));
            }
        }
    }

    Ok(res)
}

fn repeat_m_n_<I, O, C, E, F>(min: usize, max: usize, parse: &mut F, input: &mut I) -> PResult<C, E>
where
    I: Stream,
    C: Accumulate<O>,
    F: Parser<I, O, E>,
    E: ParserError<I>,
{
    if min > max {
        return Err(ErrMode::assert(
            input,
            "range should be ascending, rather than descending",
        ));
    }

    let mut res = C::initial(Some(min));
    for count in 0..max {
        let start = input.checkpoint();
        let len = input.eof_offset();
        match parse.parse_next(input) {
            Ok(value) => {
                // infinite loop check: the parser must always consume
                if input.eof_offset() == len {
                    return Err(ErrMode::assert(
                        input,
                        "`repeat` parsers must always consume",
                    ));
                }

                res.accumulate(value);
            }
            Err(ErrMode::Backtrack(e)) => {
                if count < min {
                    return Err(ErrMode::Backtrack(e.append(input, &start, ErrorKind::Many)));
                } else {
                    input.reset(&start);
                    return Ok(res);
                }
            }
            Err(e) => {
                return Err(e);
            }
        }
    }

    Ok(res)
}

/// [`Accumulate`] the output of parser `f` into a container, like `Vec`, until the parser `g`
/// produces a result.
///
/// Returns a tuple of the results of `f` in a `Vec` and the result of `g`.
///
/// `f` keeps going so long as `g` produces [`ErrMode::Backtrack`]. To instead chain an error up, see [`cut_err`][crate::combinator::cut_err].
///
/// To take a series of tokens, [`Accumulate`] into a `()`
/// (e.g. with [`.map(|()| ())`][Parser::map])
/// and then [`Parser::take`].
///
/// See also
/// - [`take_till`][crate::token::take_till] for recognizing up-to a member of a [set of tokens][crate::stream::ContainsToken]
/// - [`take_until`][crate::token::take_until] for recognizing up-to a [`literal`][crate::token::literal] (w/ optional simd optimizations)
///
/// # Example
///
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::repeat_till;
///
/// fn parser(s: &str) -> IResult<&str, (Vec<&str>, &str)> {
///   repeat_till(0.., "abc", "end").parse_peek(s)
/// };
///
/// assert_eq!(parser("abcabcend"), Ok(("", (vec!["abc", "abc"], "end"))));
/// assert_eq!(parser("abc123end"), Err(ErrMode::Backtrack(InputError::new("123end", ErrorKind::Tag))));
/// assert_eq!(parser("123123end"), Err(ErrMode::Backtrack(InputError::new("123123end", ErrorKind::Tag))));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// assert_eq!(parser("abcendefg"), Ok(("efg", (vec!["abc"], "end"))));
/// # }
/// ```
#[doc(alias = "many_till0")]
pub fn repeat_till<Input, Output, Accumulator, Terminator, Error, ParseNext, TerminatorParser>(
    occurrences: impl Into<Range>,
    mut parse: ParseNext,
    mut terminator: TerminatorParser,
) -> impl Parser<Input, (Accumulator, Terminator), Error>
where
    Input: Stream,
    Accumulator: Accumulate<Output>,
    ParseNext: Parser<Input, Output, Error>,
    TerminatorParser: Parser<Input, Terminator, Error>,
    Error: ParserError<Input>,
{
    let Range {
        start_inclusive,
        end_inclusive,
    } = occurrences.into();
    trace("repeat_till", move |i: &mut Input| {
        match (start_inclusive, end_inclusive) {
            (0, None) => repeat_till0_(&mut parse, &mut terminator, i),
            (start, end) => repeat_till_m_n_(
                start,
                end.unwrap_or(usize::MAX),
                &mut parse,
                &mut terminator,
                i,
            ),
        }
    })
}

fn repeat_till0_<I, O, C, P, E, F, G>(f: &mut F, g: &mut G, i: &mut I) -> PResult<(C, P), E>
where
    I: Stream,
    C: Accumulate<O>,
    F: Parser<I, O, E>,
    G: Parser<I, P, E>,
    E: ParserError<I>,
{
    let mut res = C::initial(None);
    loop {
        let start = i.checkpoint();
        let len = i.eof_offset();
        match g.parse_next(i) {
            Ok(o) => return Ok((res, o)),
            Err(ErrMode::Backtrack(_)) => {
                i.reset(&start);
                match f.parse_next(i) {
                    Err(e) => return Err(e.append(i, &start, ErrorKind::Many)),
                    Ok(o) => {
                        // infinite loop check: the parser must always consume
                        if i.eof_offset() == len {
                            return Err(ErrMode::assert(i, "`repeat` parsers must always consume"));
                        }

                        res.accumulate(o);
                    }
                }
            }
            Err(e) => return Err(e),
        }
    }
}

fn repeat_till_m_n_<I, O, C, P, E, F, G>(
    min: usize,
    max: usize,
    f: &mut F,
    g: &mut G,
    i: &mut I,
) -> PResult<(C, P), E>
where
    I: Stream,
    C: Accumulate<O>,
    F: Parser<I, O, E>,
    G: Parser<I, P, E>,
    E: ParserError<I>,
{
    if min > max {
        return Err(ErrMode::assert(
            i,
            "range should be ascending, rather than descending",
        ));
    }

    let mut res = C::initial(Some(min));

    let start = i.checkpoint();
    for _ in 0..min {
        match f.parse_next(i) {
            Ok(o) => {
                res.accumulate(o);
            }
            Err(e) => {
                return Err(e.append(i, &start, ErrorKind::Many));
            }
        }
    }
    for count in min..=max {
        let start = i.checkpoint();
        let len = i.eof_offset();
        match g.parse_next(i) {
            Ok(o) => return Ok((res, o)),
            Err(ErrMode::Backtrack(err)) => {
                if count == max {
                    return Err(ErrMode::Backtrack(err));
                }
                i.reset(&start);
                match f.parse_next(i) {
                    Err(e) => {
                        return Err(e.append(i, &start, ErrorKind::Many));
                    }
                    Ok(o) => {
                        // infinite loop check: the parser must always consume
                        if i.eof_offset() == len {
                            return Err(ErrMode::assert(i, "`repeat` parsers must always consume"));
                        }

                        res.accumulate(o);
                    }
                }
            }
            Err(e) => return Err(e),
        }
    }
    unreachable!()
}

/// [`Accumulate`] the output of a parser, interleaved with `sep`
///
/// This stops when either parser returns [`ErrMode::Backtrack`]. To instead chain an error up, see
/// [`cut_err`][crate::combinator::cut_err].
///
/// To take a series of tokens, [`Accumulate`] into a `()`
/// (e.g. with [`.map(|()| ())`][Parser::map])
/// and then [`Parser::take`].
///
/// **Warning:** If the separator parser accepts empty inputs
/// (like `alpha0` or `digit0`), `separated` will return an error,
/// to prevent going into an infinite loop.
///
/// # Example
///
/// Zero or more repetitions:
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::{error::ErrMode, error::ErrorKind, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::separated;
///
/// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
///   separated(0.., "abc", "|").parse_peek(s)
/// }
///
/// assert_eq!(parser("abc|abc|abc"), Ok(("", vec!["abc", "abc", "abc"])));
/// assert_eq!(parser("abc123abc"), Ok(("123abc", vec!["abc"])));
/// assert_eq!(parser("abc|def"), Ok(("|def", vec!["abc"])));
/// assert_eq!(parser(""), Ok(("", vec![])));
/// assert_eq!(parser("def|abc"), Ok(("def|abc", vec![])));
/// # }
/// ```
///
/// One or more repetitions:
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::separated;
///
/// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
///   separated(1.., "abc", "|").parse_peek(s)
/// }
///
/// assert_eq!(parser("abc|abc|abc"), Ok(("", vec!["abc", "abc", "abc"])));
/// assert_eq!(parser("abc123abc"), Ok(("123abc", vec!["abc"])));
/// assert_eq!(parser("abc|def"), Ok(("|def", vec!["abc"])));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// assert_eq!(parser("def|abc"), Err(ErrMode::Backtrack(InputError::new("def|abc", ErrorKind::Tag))));
/// # }
/// ```
///
/// Fixed number of repetitions:
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::separated;
///
/// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
///   separated(2, "abc", "|").parse_peek(s)
/// }
///
/// assert_eq!(parser("abc|abc|abc"), Ok(("|abc", vec!["abc", "abc"])));
/// assert_eq!(parser("abc123abc"), Err(ErrMode::Backtrack(InputError::new("123abc", ErrorKind::Tag))));
/// assert_eq!(parser("abc|def"), Err(ErrMode::Backtrack(InputError::new("def", ErrorKind::Tag))));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// assert_eq!(parser("def|abc"), Err(ErrMode::Backtrack(InputError::new("def|abc", ErrorKind::Tag))));
/// # }
/// ```
///
/// Arbitrary repetitions:
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::separated;
///
/// fn parser(s: &str) -> IResult<&str, Vec<&str>> {
///   separated(0..=2, "abc", "|").parse_peek(s)
/// }
///
/// assert_eq!(parser("abc|abc|abc"), Ok(("|abc", vec!["abc", "abc"])));
/// assert_eq!(parser("abc123abc"), Ok(("123abc", vec!["abc"])));
/// assert_eq!(parser("abc|def"), Ok(("|def", vec!["abc"])));
/// assert_eq!(parser(""), Ok(("", vec![])));
/// assert_eq!(parser("def|abc"), Ok(("def|abc", vec![])));
/// # }
/// ```
#[doc(alias = "sep_by")]
#[doc(alias = "sep_by1")]
#[doc(alias = "separated_list0")]
#[doc(alias = "separated_list1")]
#[doc(alias = "separated_m_n")]
#[inline(always)]
pub fn separated<Input, Output, Accumulator, Sep, Error, ParseNext, SepParser>(
    occurrences: impl Into<Range>,
    mut parser: ParseNext,
    mut separator: SepParser,
) -> impl Parser<Input, Accumulator, Error>
where
    Input: Stream,
    Accumulator: Accumulate<Output>,
    ParseNext: Parser<Input, Output, Error>,
    SepParser: Parser<Input, Sep, Error>,
    Error: ParserError<Input>,
{
    let Range {
        start_inclusive,
        end_inclusive,
    } = occurrences.into();
    trace("separated", move |input: &mut Input| {
        match (start_inclusive, end_inclusive) {
            (0, None) => separated0_(&mut parser, &mut separator, input),
            (1, None) => separated1_(&mut parser, &mut separator, input),
            (start, end) if Some(start) == end => {
                separated_n_(start, &mut parser, &mut separator, input)
            }
            (start, end) => separated_m_n_(
                start,
                end.unwrap_or(usize::MAX),
                &mut parser,
                &mut separator,
                input,
            ),
        }
    })
}

fn separated0_<I, O, C, O2, E, P, S>(
    parser: &mut P,
    separator: &mut S,
    input: &mut I,
) -> PResult<C, E>
where
    I: Stream,
    C: Accumulate<O>,
    P: Parser<I, O, E>,
    S: Parser<I, O2, E>,
    E: ParserError<I>,
{
    let mut acc = C::initial(None);

    let start = input.checkpoint();
    match parser.parse_next(input) {
        Err(ErrMode::Backtrack(_)) => {
            input.reset(&start);
            return Ok(acc);
        }
        Err(e) => return Err(e),
        Ok(o) => {
            acc.accumulate(o);
        }
    }

    loop {
        let start = input.checkpoint();
        let len = input.eof_offset();
        match separator.parse_next(input) {
            Err(ErrMode::Backtrack(_)) => {
                input.reset(&start);
                return Ok(acc);
            }
            Err(e) => return Err(e),
            Ok(_) => {
                // infinite loop check
                if input.eof_offset() == len {
                    return Err(ErrMode::assert(
                        input,
                        "`separated` separator parser must always consume",
                    ));
                }

                match parser.parse_next(input) {
                    Err(ErrMode::Backtrack(_)) => {
                        input.reset(&start);
                        return Ok(acc);
                    }
                    Err(e) => return Err(e),
                    Ok(o) => {
                        acc.accumulate(o);
                    }
                }
            }
        }
    }
}

fn separated1_<I, O, C, O2, E, P, S>(
    parser: &mut P,
    separator: &mut S,
    input: &mut I,
) -> PResult<C, E>
where
    I: Stream,
    C: Accumulate<O>,
    P: Parser<I, O, E>,
    S: Parser<I, O2, E>,
    E: ParserError<I>,
{
    let mut acc = C::initial(None);

    // Parse the first element
    match parser.parse_next(input) {
        Err(e) => return Err(e),
        Ok(o) => {
            acc.accumulate(o);
        }
    }

    loop {
        let start = input.checkpoint();
        let len = input.eof_offset();
        match separator.parse_next(input) {
            Err(ErrMode::Backtrack(_)) => {
                input.reset(&start);
                return Ok(acc);
            }
            Err(e) => return Err(e),
            Ok(_) => {
                // infinite loop check
                if input.eof_offset() == len {
                    return Err(ErrMode::assert(
                        input,
                        "`separated` separator parser must always consume",
                    ));
                }

                match parser.parse_next(input) {
                    Err(ErrMode::Backtrack(_)) => {
                        input.reset(&start);
                        return Ok(acc);
                    }
                    Err(e) => return Err(e),
                    Ok(o) => {
                        acc.accumulate(o);
                    }
                }
            }
        }
    }
}

fn separated_n_<I, O, C, O2, E, P, S>(
    count: usize,
    parser: &mut P,
    separator: &mut S,
    input: &mut I,
) -> PResult<C, E>
where
    I: Stream,
    C: Accumulate<O>,
    P: Parser<I, O, E>,
    S: Parser<I, O2, E>,
    E: ParserError<I>,
{
    let mut acc = C::initial(Some(count));

    if count == 0 {
        return Ok(acc);
    }

    let start = input.checkpoint();
    match parser.parse_next(input) {
        Err(e) => {
            return Err(e.append(input, &start, ErrorKind::Many));
        }
        Ok(o) => {
            acc.accumulate(o);
        }
    }

    for _ in 1..count {
        let start = input.checkpoint();
        let len = input.eof_offset();
        match separator.parse_next(input) {
            Err(e) => {
                return Err(e.append(input, &start, ErrorKind::Many));
            }
            Ok(_) => {
                // infinite loop check
                if input.eof_offset() == len {
                    return Err(ErrMode::assert(
                        input,
                        "`separated` separator parser must always consume",
                    ));
                }

                match parser.parse_next(input) {
                    Err(e) => {
                        return Err(e.append(input, &start, ErrorKind::Many));
                    }
                    Ok(o) => {
                        acc.accumulate(o);
                    }
                }
            }
        }
    }

    Ok(acc)
}

fn separated_m_n_<I, O, C, O2, E, P, S>(
    min: usize,
    max: usize,
    parser: &mut P,
    separator: &mut S,
    input: &mut I,
) -> PResult<C, E>
where
    I: Stream,
    C: Accumulate<O>,
    P: Parser<I, O, E>,
    S: Parser<I, O2, E>,
    E: ParserError<I>,
{
    if min > max {
        return Err(ErrMode::assert(
            input,
            "range should be ascending, rather than descending",
        ));
    }

    let mut acc = C::initial(Some(min));

    let start = input.checkpoint();
    match parser.parse_next(input) {
        Err(ErrMode::Backtrack(e)) => {
            if min == 0 {
                input.reset(&start);
                return Ok(acc);
            } else {
                return Err(ErrMode::Backtrack(e.append(input, &start, ErrorKind::Many)));
            }
        }
        Err(e) => return Err(e),
        Ok(o) => {
            acc.accumulate(o);
        }
    }

    for index in 1..max {
        let start = input.checkpoint();
        let len = input.eof_offset();
        match separator.parse_next(input) {
            Err(ErrMode::Backtrack(e)) => {
                if index < min {
                    return Err(ErrMode::Backtrack(e.append(input, &start, ErrorKind::Many)));
                } else {
                    input.reset(&start);
                    return Ok(acc);
                }
            }
            Err(e) => {
                return Err(e);
            }
            Ok(_) => {
                // infinite loop check
                if input.eof_offset() == len {
                    return Err(ErrMode::assert(
                        input,
                        "`separated` separator parser must always consume",
                    ));
                }

                match parser.parse_next(input) {
                    Err(ErrMode::Backtrack(e)) => {
                        if index < min {
                            return Err(ErrMode::Backtrack(e.append(
                                input,
                                &start,
                                ErrorKind::Many,
                            )));
                        } else {
                            input.reset(&start);
                            return Ok(acc);
                        }
                    }
                    Err(e) => {
                        return Err(e);
                    }
                    Ok(o) => {
                        acc.accumulate(o);
                    }
                }
            }
        }
    }

    Ok(acc)
}

/// Alternates between two parsers, merging the results (left associative)
///
/// This stops when either parser returns [`ErrMode::Backtrack`]. To instead chain an error up, see
/// [`cut_err`][crate::combinator::cut_err].
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::separated_foldl1;
/// use winnow::ascii::dec_int;
///
/// fn parser(s: &str) -> IResult<&str, i32> {
///   separated_foldl1(dec_int, "-", |l, _, r| l - r).parse_peek(s)
/// }
///
/// assert_eq!(parser("9-3-5"), Ok(("", 1)));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Token))));
/// assert_eq!(parser("def|abc"), Err(ErrMode::Backtrack(InputError::new("def|abc", ErrorKind::Verify))));
/// ```
pub fn separated_foldl1<Input, Output, Sep, Error, ParseNext, SepParser, Op>(
    mut parser: ParseNext,
    mut sep: SepParser,
    mut op: Op,
) -> impl Parser<Input, Output, Error>
where
    Input: Stream,
    ParseNext: Parser<Input, Output, Error>,
    SepParser: Parser<Input, Sep, Error>,
    Error: ParserError<Input>,
    Op: FnMut(Output, Sep, Output) -> Output,
{
    trace("separated_foldl1", move |i: &mut Input| {
        let mut ol = parser.parse_next(i)?;

        loop {
            let start = i.checkpoint();
            let len = i.eof_offset();
            match sep.parse_next(i) {
                Err(ErrMode::Backtrack(_)) => {
                    i.reset(&start);
                    return Ok(ol);
                }
                Err(e) => return Err(e),
                Ok(s) => {
                    // infinite loop check: the parser must always consume
                    if i.eof_offset() == len {
                        return Err(ErrMode::assert(i, "`repeat` parsers must always consume"));
                    }

                    match parser.parse_next(i) {
                        Err(ErrMode::Backtrack(_)) => {
                            i.reset(&start);
                            return Ok(ol);
                        }
                        Err(e) => return Err(e),
                        Ok(or) => {
                            ol = op(ol, s, or);
                        }
                    }
                }
            }
        }
    })
}

/// Alternates between two parsers, merging the results (right associative)
///
/// This stops when either parser returns [`ErrMode::Backtrack`]. To instead chain an error up, see
/// [`cut_err`][crate::combinator::cut_err].
///
/// # Example
///
/// ```
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::separated_foldr1;
/// use winnow::ascii::dec_uint;
///
/// fn parser(s: &str) -> IResult<&str, u32> {
///   separated_foldr1(dec_uint, "^", |l: u32, _, r: u32| l.pow(r)).parse_peek(s)
/// }
///
/// assert_eq!(parser("2^3^2"), Ok(("", 512)));
/// assert_eq!(parser("2"), Ok(("", 2)));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Token))));
/// assert_eq!(parser("def|abc"), Err(ErrMode::Backtrack(InputError::new("def|abc", ErrorKind::Verify))));
/// ```
#[cfg(feature = "alloc")]
pub fn separated_foldr1<Input, Output, Sep, Error, ParseNext, SepParser, Op>(
    mut parser: ParseNext,
    mut sep: SepParser,
    mut op: Op,
) -> impl Parser<Input, Output, Error>
where
    Input: Stream,
    ParseNext: Parser<Input, Output, Error>,
    SepParser: Parser<Input, Sep, Error>,
    Error: ParserError<Input>,
    Op: FnMut(Output, Sep, Output) -> Output,
{
    trace("separated_foldr1", move |i: &mut Input| {
        let ol = parser.parse_next(i)?;
        let all: crate::lib::std::vec::Vec<(Sep, Output)> =
            repeat(0.., (sep.by_ref(), parser.by_ref())).parse_next(i)?;
        if let Some((s, or)) = all
            .into_iter()
            .rev()
            .reduce(|(sr, or), (sl, ol)| (sl, op(ol, sr, or)))
        {
            let merged = op(ol, s, or);
            Ok(merged)
        } else {
            Ok(ol)
        }
    })
}

/// Repeats the embedded parser, filling the given slice with results.
///
/// This parser fails if the input runs out before the given slice is full.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::prelude::*;
/// use winnow::combinator::fill;
///
/// fn parser(s: &str) -> IResult<&str, [&str; 2]> {
///   let mut buf = ["", ""];
///   let (rest, ()) = fill("abc", &mut buf).parse_peek(s)?;
///   Ok((rest, buf))
/// }
///
/// assert_eq!(parser("abcabc"), Ok(("", ["abc", "abc"])));
/// assert_eq!(parser("abc123"), Err(ErrMode::Backtrack(InputError::new("123", ErrorKind::Tag))));
/// assert_eq!(parser("123123"), Err(ErrMode::Backtrack(InputError::new("123123", ErrorKind::Tag))));
/// assert_eq!(parser(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// assert_eq!(parser("abcabcabc"), Ok(("abc", ["abc", "abc"])));
/// ```
pub fn fill<'i, Input, Output, Error, ParseNext>(
    mut parser: ParseNext,
    buf: &'i mut [Output],
) -> impl Parser<Input, (), Error> + 'i
where
    Input: Stream + 'i,
    ParseNext: Parser<Input, Output, Error> + 'i,
    Error: ParserError<Input> + 'i,
{
    trace("fill", move |i: &mut Input| {
        for elem in buf.iter_mut() {
            let start = i.checkpoint();
            match parser.parse_next(i) {
                Ok(o) => {
                    *elem = o;
                }
                Err(e) => {
                    return Err(e.append(i, &start, ErrorKind::Many));
                }
            }
        }

        Ok(())
    })
}

fn fold_repeat0_<I, O, E, F, G, H, R>(
    f: &mut F,
    init: &mut H,
    g: &mut G,
    input: &mut I,
) -> PResult<R, E>
where
    I: Stream,
    F: Parser<I, O, E>,
    G: FnMut(R, O) -> R,
    H: FnMut() -> R,
    E: ParserError<I>,
{
    let mut res = init();

    loop {
        let start = input.checkpoint();
        let len = input.eof_offset();
        match f.parse_next(input) {
            Ok(o) => {
                // infinite loop check: the parser must always consume
                if input.eof_offset() == len {
                    return Err(ErrMode::assert(
                        input,
                        "`repeat` parsers must always consume",
                    ));
                }

                res = g(res, o);
            }
            Err(ErrMode::Backtrack(_)) => {
                input.reset(&start);
                return Ok(res);
            }
            Err(e) => {
                return Err(e);
            }
        }
    }
}

fn fold_repeat1_<I, O, E, F, G, H, R>(
    f: &mut F,
    init: &mut H,
    g: &mut G,
    input: &mut I,
) -> PResult<R, E>
where
    I: Stream,
    F: Parser<I, O, E>,
    G: FnMut(R, O) -> R,
    H: FnMut() -> R,
    E: ParserError<I>,
{
    let init = init();
    match f.parse_next(input) {
        Err(ErrMode::Backtrack(_)) => Err(ErrMode::from_error_kind(input, ErrorKind::Many)),
        Err(e) => Err(e),
        Ok(o1) => {
            let mut acc = g(init, o1);

            loop {
                let start = input.checkpoint();
                let len = input.eof_offset();
                match f.parse_next(input) {
                    Err(ErrMode::Backtrack(_)) => {
                        input.reset(&start);
                        break;
                    }
                    Err(e) => return Err(e),
                    Ok(o) => {
                        // infinite loop check: the parser must always consume
                        if input.eof_offset() == len {
                            return Err(ErrMode::assert(
                                input,
                                "`repeat` parsers must always consume",
                            ));
                        }

                        acc = g(acc, o);
                    }
                }
            }

            Ok(acc)
        }
    }
}

fn fold_repeat_m_n_<I, O, E, F, G, H, R>(
    min: usize,
    max: usize,
    parse: &mut F,
    init: &mut H,
    fold: &mut G,
    input: &mut I,
) -> PResult<R, E>
where
    I: Stream,
    F: Parser<I, O, E>,
    G: FnMut(R, O) -> R,
    H: FnMut() -> R,
    E: ParserError<I>,
{
    if min > max {
        return Err(ErrMode::assert(
            input,
            "range should be ascending, rather than descending",
        ));
    }

    let mut acc = init();
    for count in 0..max {
        let start = input.checkpoint();
        let len = input.eof_offset();
        match parse.parse_next(input) {
            Ok(value) => {
                // infinite loop check: the parser must always consume
                if input.eof_offset() == len {
                    return Err(ErrMode::assert(
                        input,
                        "`repeat` parsers must always consume",
                    ));
                }

                acc = fold(acc, value);
            }
            //FInputXMError: handle failure properly
            Err(ErrMode::Backtrack(err)) => {
                if count < min {
                    return Err(ErrMode::Backtrack(err.append(
                        input,
                        &start,
                        ErrorKind::Many,
                    )));
                } else {
                    input.reset(&start);
                    break;
                }
            }
            Err(e) => return Err(e),
        }
    }

    Ok(acc)
}
