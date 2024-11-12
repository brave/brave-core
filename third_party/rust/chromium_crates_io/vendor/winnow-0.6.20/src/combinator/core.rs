use crate::combinator::trace;
use crate::error::{ErrMode, ErrorKind, Needed, ParserError};
use crate::stream::Stream;
use crate::*;

/// Return the remaining input.
///
/// # Effective Signature
///
/// Assuming you are parsing a `&str` [Stream]:
/// ```rust
/// # use winnow::prelude::*;;
/// pub fn rest<'i>(input: &mut &'i str) -> PResult<&'i str>
/// # {
/// #     winnow::combinator::rest.parse_next(input)
/// # }
/// ```
///
/// # Example
///
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::error::ErrorKind;
/// # use winnow::error::InputError;
/// use winnow::combinator::rest;
/// assert_eq!(rest::<_,InputError<_>>.parse_peek("abc"), Ok(("", "abc")));
/// assert_eq!(rest::<_,InputError<_>>.parse_peek(""), Ok(("", "")));
/// ```
#[inline]
pub fn rest<Input, Error>(input: &mut Input) -> PResult<<Input as Stream>::Slice, Error>
where
    Input: Stream,
    Error: ParserError<Input>,
{
    trace("rest", move |input: &mut Input| Ok(input.finish())).parse_next(input)
}

/// Return the length of the remaining input.
///
/// Note: this does not advance the [`Stream`]
///
/// # Effective Signature
///
/// Assuming you are parsing a `&str` [Stream]:
/// ```rust
/// # use winnow::prelude::*;;
/// pub fn rest_len(input: &mut &str) -> PResult<usize>
/// # {
/// #     winnow::combinator::rest_len.parse_next(input)
/// # }
/// ```
///
/// # Example
///
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::error::ErrorKind;
/// # use winnow::error::InputError;
/// use winnow::combinator::rest_len;
/// assert_eq!(rest_len::<_,InputError<_>>.parse_peek("abc"), Ok(("abc", 3)));
/// assert_eq!(rest_len::<_,InputError<_>>.parse_peek(""), Ok(("", 0)));
/// ```
#[inline]
pub fn rest_len<Input, Error>(input: &mut Input) -> PResult<usize, Error>
where
    Input: Stream,
    Error: ParserError<Input>,
{
    trace("rest_len", move |input: &mut Input| {
        let len = input.eof_offset();
        Ok(len)
    })
    .parse_next(input)
}

/// Apply a [`Parser`], producing `None` on [`ErrMode::Backtrack`].
///
/// To chain an error up, see [`cut_err`].
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError};
/// # use winnow::prelude::*;
/// use winnow::combinator::opt;
/// use winnow::ascii::alpha1;
/// # fn main() {
///
/// fn parser(i: &str) -> IResult<&str, Option<&str>> {
///   opt(alpha1).parse_peek(i)
/// }
///
/// assert_eq!(parser("abcd;"), Ok((";", Some("abcd"))));
/// assert_eq!(parser("123;"), Ok(("123;", None)));
/// # }
/// ```
pub fn opt<Input: Stream, Output, Error, ParseNext>(
    mut parser: ParseNext,
) -> impl Parser<Input, Option<Output>, Error>
where
    ParseNext: Parser<Input, Output, Error>,
    Error: ParserError<Input>,
{
    trace("opt", move |input: &mut Input| {
        let start = input.checkpoint();
        match parser.parse_next(input) {
            Ok(o) => Ok(Some(o)),
            Err(ErrMode::Backtrack(_)) => {
                input.reset(&start);
                Ok(None)
            }
            Err(e) => Err(e),
        }
    })
}

/// Calls the parser if the condition is met.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, IResult};
/// # use winnow::prelude::*;
/// use winnow::combinator::cond;
/// use winnow::ascii::alpha1;
/// # fn main() {
///
/// fn parser(b: bool, i: &str) -> IResult<&str, Option<&str>> {
///   cond(b, alpha1).parse_peek(i)
/// }
///
/// assert_eq!(parser(true, "abcd;"), Ok((";", Some("abcd"))));
/// assert_eq!(parser(false, "abcd;"), Ok(("abcd;", None)));
/// assert_eq!(parser(true, "123;"), Err(ErrMode::Backtrack(InputError::new("123;", ErrorKind::Slice))));
/// assert_eq!(parser(false, "123;"), Ok(("123;", None)));
/// # }
/// ```
pub fn cond<Input, Output, Error, ParseNext>(
    cond: bool,
    mut parser: ParseNext,
) -> impl Parser<Input, Option<Output>, Error>
where
    Input: Stream,
    ParseNext: Parser<Input, Output, Error>,
    Error: ParserError<Input>,
{
    trace("cond", move |input: &mut Input| {
        if cond {
            parser.parse_next(input).map(Some)
        } else {
            Ok(None)
        }
    })
}

/// Tries to apply its parser without consuming the input.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, IResult};
/// # use winnow::prelude::*;
/// use winnow::combinator::peek;
/// use winnow::ascii::alpha1;
/// # fn main() {
///
/// let mut parser = peek(alpha1);
///
/// assert_eq!(parser.parse_peek("abcd;"), Ok(("abcd;", "abcd")));
/// assert_eq!(parser.parse_peek("123;"), Err(ErrMode::Backtrack(InputError::new("123;", ErrorKind::Slice))));
/// # }
/// ```
#[doc(alias = "look_ahead")]
#[doc(alias = "rewind")]
pub fn peek<Input, Output, Error, ParseNext>(
    mut parser: ParseNext,
) -> impl Parser<Input, Output, Error>
where
    Input: Stream,
    Error: ParserError<Input>,
    ParseNext: Parser<Input, Output, Error>,
{
    trace("peek", move |input: &mut Input| {
        let start = input.checkpoint();
        let res = parser.parse_next(input);
        input.reset(&start);
        res
    })
}

/// Match the end of the [`Stream`]
///
/// Otherwise, it will error.
///
/// # Effective Signature
///
/// Assuming you are parsing a `&str` [Stream]:
/// ```rust
/// # use winnow::prelude::*;;
/// pub fn eof<'i>(input: &mut &'i str) -> PResult<&'i str>
/// # {
/// #     winnow::combinator::eof.parse_next(input)
/// # }
/// ```
///
/// # Example
///
/// ```rust
/// # use std::str;
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError};
/// # use winnow::combinator::eof;
/// # use winnow::prelude::*;
///
/// let mut parser = eof;
/// assert_eq!(parser.parse_peek("abc"), Err(ErrMode::Backtrack(InputError::new("abc", ErrorKind::Eof))));
/// assert_eq!(parser.parse_peek(""), Ok(("", "")));
/// ```
#[doc(alias = "end")]
#[doc(alias = "eoi")]
pub fn eof<Input, Error>(input: &mut Input) -> PResult<<Input as Stream>::Slice, Error>
where
    Input: Stream,
    Error: ParserError<Input>,
{
    trace("eof", move |input: &mut Input| {
        if input.eof_offset() == 0 {
            Ok(input.next_slice(0))
        } else {
            Err(ErrMode::from_error_kind(input, ErrorKind::Eof))
        }
    })
    .parse_next(input)
}

/// Succeeds if the child parser returns an error.
///
/// **Note:** This does not advance the [`Stream`]
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, IResult};
/// # use winnow::prelude::*;
/// use winnow::combinator::not;
/// use winnow::ascii::alpha1;
/// # fn main() {
///
/// let mut parser = not(alpha1);
///
/// assert_eq!(parser.parse_peek("123"), Ok(("123", ())));
/// assert_eq!(parser.parse_peek("abcd"), Err(ErrMode::Backtrack(InputError::new("abcd", ErrorKind::Not))));
/// # }
/// ```
pub fn not<Input, Output, Error, ParseNext>(mut parser: ParseNext) -> impl Parser<Input, (), Error>
where
    Input: Stream,
    Error: ParserError<Input>,
    ParseNext: Parser<Input, Output, Error>,
{
    trace("not", move |input: &mut Input| {
        let start = input.checkpoint();
        let res = parser.parse_next(input);
        input.reset(&start);
        match res {
            Ok(_) => Err(ErrMode::from_error_kind(input, ErrorKind::Not)),
            Err(ErrMode::Backtrack(_)) => Ok(()),
            Err(e) => Err(e),
        }
    })
}

/// Transforms an [`ErrMode::Backtrack`] (recoverable) to [`ErrMode::Cut`] (unrecoverable)
///
/// This commits the parse result, preventing alternative branch paths like with
/// [`winnow::combinator::alt`][crate::combinator::alt].
///
/// See the [tutorial][crate::_tutorial::chapter_7] for more details.
///
/// # Example
///
/// Without `cut_err`:
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError};
/// # use winnow::token::one_of;
/// # use winnow::ascii::digit1;
/// # use winnow::combinator::rest;
/// # use winnow::combinator::alt;
/// # use winnow::combinator::preceded;
/// # use winnow::prelude::*;
/// # fn main() {
///
/// fn parser(input: &str) -> IResult<&str, &str> {
///   alt((
///     preceded(one_of(['+', '-']), digit1),
///     rest
///   )).parse_peek(input)
/// }
///
/// assert_eq!(parser("+10 ab"), Ok((" ab", "10")));
/// assert_eq!(parser("ab"), Ok(("", "ab")));
/// assert_eq!(parser("+"), Ok(("", "+")));
/// # }
/// ```
///
/// With `cut_err`:
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError};
/// # use winnow::prelude::*;
/// # use winnow::token::one_of;
/// # use winnow::ascii::digit1;
/// # use winnow::combinator::rest;
/// # use winnow::combinator::alt;
/// # use winnow::combinator::preceded;
/// use winnow::combinator::cut_err;
/// # fn main() {
///
/// fn parser(input: &str) -> IResult<&str, &str> {
///   alt((
///     preceded(one_of(['+', '-']), cut_err(digit1)),
///     rest
///   )).parse_peek(input)
/// }
///
/// assert_eq!(parser("+10 ab"), Ok((" ab", "10")));
/// assert_eq!(parser("ab"), Ok(("", "ab")));
/// assert_eq!(parser("+"), Err(ErrMode::Cut(InputError::new("", ErrorKind::Slice ))));
/// # }
/// ```
pub fn cut_err<Input, Output, Error, ParseNext>(
    mut parser: ParseNext,
) -> impl Parser<Input, Output, Error>
where
    Input: Stream,
    Error: ParserError<Input>,
    ParseNext: Parser<Input, Output, Error>,
{
    trace("cut_err", move |input: &mut Input| {
        parser.parse_next(input).map_err(|e| e.cut())
    })
}

/// Transforms an [`ErrMode::Cut`] (unrecoverable) to [`ErrMode::Backtrack`] (recoverable)
///
/// This attempts the parse, allowing other parsers to be tried on failure, like with
/// [`winnow::combinator::alt`][crate::combinator::alt].
pub fn backtrack_err<Input, Output, Error, ParseNext>(
    mut parser: ParseNext,
) -> impl Parser<Input, Output, Error>
where
    Input: Stream,
    Error: ParserError<Input>,
    ParseNext: Parser<Input, Output, Error>,
{
    trace("backtrack_err", move |input: &mut Input| {
        parser.parse_next(input).map_err(|e| e.backtrack())
    })
}

/// A placeholder for a not-yet-implemented [`Parser`]
///
/// This is analogous to the [`todo!`] macro and helps with prototyping.
///
/// # Panic
///
/// This will panic when parsing
///
/// # Example
///
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::combinator::todo;
///
/// fn parser(input: &mut &str) -> PResult<u64> {
///     todo(input)
/// }
/// ```
#[track_caller]
pub fn todo<Input, Output, Error>(input: &mut Input) -> PResult<Output, Error>
where
    Input: Stream,
{
    #![allow(clippy::todo)]
    trace("todo", move |_input: &mut Input| {
        todo!("unimplemented parse")
    })
    .parse_next(input)
}

/// Repeats the embedded parser, lazily returning the results
///
/// Call the iterator's [`ParserIterator::finish`] method to get the remaining input if successful,
/// or the error value if we encountered an error.
///
/// On [`ErrMode::Backtrack`], iteration will stop. To instead chain an error up, see [`cut_err`].
///
/// # Example
///
/// ```rust
/// use winnow::{combinator::iterator, IResult, ascii::alpha1, combinator::terminated};
/// use std::collections::HashMap;
///
/// let data = "abc|defg|hijkl|mnopqr|123";
/// let mut it = iterator(data, terminated(alpha1, "|"));
///
/// let parsed = it.map(|v| (v, v.len())).collect::<HashMap<_,_>>();
/// let res: IResult<_,_> = it.finish();
///
/// assert_eq!(parsed, [("abc", 3usize), ("defg", 4), ("hijkl", 5), ("mnopqr", 6)].iter().cloned().collect());
/// assert_eq!(res, Ok(("123", ())));
/// ```
pub fn iterator<Input, Output, Error, ParseNext>(
    input: Input,
    parser: ParseNext,
) -> ParserIterator<ParseNext, Input, Output, Error>
where
    ParseNext: Parser<Input, Output, Error>,
    Input: Stream,
    Error: ParserError<Input>,
{
    ParserIterator {
        parser,
        input,
        state: Some(State::Running),
        o: Default::default(),
    }
}

/// Main structure associated to [`iterator`].
pub struct ParserIterator<F, I, O, E>
where
    F: Parser<I, O, E>,
    I: Stream,
{
    parser: F,
    input: I,
    state: Option<State<E>>,
    o: core::marker::PhantomData<O>,
}

impl<F, I, O, E> ParserIterator<F, I, O, E>
where
    F: Parser<I, O, E>,
    I: Stream,
{
    /// Returns the remaining input if parsing was successful, or the error if we encountered an error.
    pub fn finish(mut self) -> PResult<(I, ()), E> {
        match self.state.take().unwrap() {
            State::Running | State::Done => Ok((self.input, ())),
            State::Failure(e) => Err(ErrMode::Cut(e)),
            State::Incomplete(i) => Err(ErrMode::Incomplete(i)),
        }
    }
}

impl<'a, F, I, O, E> core::iter::Iterator for &'a mut ParserIterator<F, I, O, E>
where
    F: Parser<I, O, E>,
    I: Stream,
{
    type Item = O;

    fn next(&mut self) -> Option<Self::Item> {
        if let State::Running = self.state.take().unwrap() {
            let start = self.input.checkpoint();

            match self.parser.parse_next(&mut self.input) {
                Ok(o) => {
                    self.state = Some(State::Running);
                    Some(o)
                }
                Err(ErrMode::Backtrack(_)) => {
                    self.input.reset(&start);
                    self.state = Some(State::Done);
                    None
                }
                Err(ErrMode::Cut(e)) => {
                    self.state = Some(State::Failure(e));
                    None
                }
                Err(ErrMode::Incomplete(i)) => {
                    self.state = Some(State::Incomplete(i));
                    None
                }
            }
        } else {
            None
        }
    }
}

enum State<E> {
    Running,
    Done,
    Failure(E),
    Incomplete(Needed),
}

/// Succeed, consuming no input
///
/// For example, it can be used as the last alternative in `alt` to
/// specify the default case.
///
/// Useful with:
/// - [`Parser::value`]
/// - [`Parser::default_value`]
/// - [`Parser::map`]
///
/// **Note:** This never advances the [`Stream`]
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError};
/// # use winnow::prelude::*;
/// use winnow::combinator::alt;
/// use winnow::combinator::empty;
///
/// fn sign(input: &str) -> IResult<&str, isize> {
///     alt((
///         '-'.value(-1),
///         '+'.value(1),
///         empty.value(1)
///     )).parse_peek(input)
/// }
/// assert_eq!(sign("+10"), Ok(("10", 1)));
/// assert_eq!(sign("-10"), Ok(("10", -1)));
/// assert_eq!(sign("10"), Ok(("10", 1)));
/// ```
#[doc(alias = "value")]
#[doc(alias = "success")]
pub fn empty<Input, Error>(_input: &mut Input) -> PResult<(), Error>
where
    Input: Stream,
    Error: ParserError<Input>,
{
    Ok(())
}

/// A parser which always fails.
///
/// For example, it can be used as the last alternative in `alt` to
/// control the error message given.
///
/// # Example
///
/// ```rust
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, IResult};
/// # use winnow::prelude::*;
/// use winnow::combinator::fail;
///
/// let s = "string";
/// assert_eq!(fail::<_, &str, _>.parse_peek(s), Err(ErrMode::Backtrack(InputError::new(s, ErrorKind::Fail))));
/// ```
#[doc(alias = "unexpected")]
pub fn fail<Input, Output, Error>(i: &mut Input) -> PResult<Output, Error>
where
    Input: Stream,
    Error: ParserError<Input>,
{
    trace("fail", |i: &mut Input| {
        Err(ErrMode::from_error_kind(i, ErrorKind::Fail))
    })
    .parse_next(i)
}
