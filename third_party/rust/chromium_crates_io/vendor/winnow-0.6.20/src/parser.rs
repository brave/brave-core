//! Basic types to build the parsers

use crate::ascii::Caseless as AsciiCaseless;
use crate::combinator::*;
#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
use crate::error::FromRecoverableError;
use crate::error::{AddContext, FromExternalError, IResult, PResult, ParseError, ParserError};
use crate::stream::{Compare, Location, ParseSlice, Stream, StreamIsPartial};
#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
use crate::stream::{Recover, Recoverable};

/// Core trait for parsing
///
/// The simplest way to implement a `Parser` is with a function
/// ```rust
/// use winnow::prelude::*;
///
/// fn empty(input: &mut &str) -> PResult<()> {
///     let output = ();
///     Ok(output)
/// }
///
/// let (input, output) = empty.parse_peek("Hello").unwrap();
/// assert_eq!(input, "Hello");  // We didn't consume any input
/// ```
///
/// which can be made stateful by returning a function
/// ```rust
/// use winnow::prelude::*;
///
/// fn empty<O: Clone>(output: O) -> impl FnMut(&mut &str) -> PResult<O> {
///     move |input: &mut &str| {
///         let output = output.clone();
///         Ok(output)
///     }
/// }
///
/// let (input, output) = empty("World").parse_peek("Hello").unwrap();
/// assert_eq!(input, "Hello");  // We didn't consume any input
/// assert_eq!(output, "World");
/// ```
///
/// Additionally, some basic types implement `Parser` as well, including
/// - `u8` and `char`, see [`winnow::token::one_of`][crate::token::one_of]
/// - `&[u8]` and `&str`, see [`winnow::token::literal`][crate::token::literal]
pub trait Parser<I, O, E> {
    /// Parse all of `input`, generating `O` from it
    #[inline]
    fn parse(&mut self, mut input: I) -> Result<O, ParseError<I, E>>
    where
        Self: core::marker::Sized,
        I: Stream,
        // Force users to deal with `Incomplete` when `StreamIsPartial<true>`
        I: StreamIsPartial,
        E: ParserError<I>,
    {
        debug_assert!(
            !I::is_partial_supported(),
            "partial streams need to handle `ErrMode::Incomplete`"
        );

        let start = input.checkpoint();
        let (o, _) = (self.by_ref(), crate::combinator::eof)
            .parse_next(&mut input)
            .map_err(|e| {
                let e = e
                    .into_inner()
                    .expect("complete parsers should not report `ErrMode::Incomplete(_)`");
                ParseError::new(input, start, e)
            })?;
        Ok(o)
    }

    /// Take tokens from the [`Stream`], turning it into the output
    ///
    /// This includes advancing the [`Stream`] to the next location.
    ///
    /// On error, `input` will be left pointing at the error location.
    fn parse_next(&mut self, input: &mut I) -> PResult<O, E>;

    /// Take tokens from the [`Stream`], turning it into the output
    ///
    /// This includes advancing the [`Stream`] to the next location.
    #[inline(always)]
    fn parse_peek(&mut self, mut input: I) -> IResult<I, O, E> {
        match self.parse_next(&mut input) {
            Ok(o) => Ok((input, o)),
            Err(err) => Err(err),
        }
    }

    /// Treat `&mut Self` as a parser
    ///
    /// This helps when needing to move a `Parser` when all you have is a `&mut Parser`.
    ///
    /// # Example
    ///
    /// Because parsers are `FnMut`, they can be called multiple times. This prevents moving `f`
    /// into [`length_take`][crate::binary::length_take] and `g` into
    /// [`Parser::complete_err`]:
    /// ```rust,compile_fail
    /// # use winnow::prelude::*;
    /// # use winnow::Parser;
    /// # use winnow::error::ParserError;
    /// # use winnow::binary::length_take;
    /// pub fn length_value<'i, O, E: ParserError<&'i [u8]>>(
    ///     mut f: impl Parser<&'i [u8], usize, E>,
    ///     mut g: impl Parser<&'i [u8], O, E>
    /// ) -> impl Parser<&'i [u8], O, E> {
    ///   move |i: &mut &'i [u8]| {
    ///     let mut data = length_take(f).parse_next(i)?;
    ///     let o = g.complete_err().parse_next(&mut data)?;
    ///     Ok(o)
    ///   }
    /// }
    /// ```
    ///
    /// By adding `by_ref`, we can make this work:
    /// ```rust
    /// # use winnow::prelude::*;
    /// # use winnow::Parser;
    /// # use winnow::error::ParserError;
    /// # use winnow::binary::length_take;
    /// pub fn length_value<'i, O, E: ParserError<&'i [u8]>>(
    ///     mut f: impl Parser<&'i [u8], usize, E>,
    ///     mut g: impl Parser<&'i [u8], O, E>
    /// ) -> impl Parser<&'i [u8], O, E> {
    ///   move |i: &mut &'i [u8]| {
    ///     let mut data = length_take(f.by_ref()).parse_next(i)?;
    ///     let o = g.by_ref().complete_err().parse_next(&mut data)?;
    ///     Ok(o)
    ///   }
    /// }
    /// ```
    #[inline(always)]
    fn by_ref(&mut self) -> ByRef<'_, Self>
    where
        Self: core::marker::Sized,
    {
        ByRef::new(self)
    }

    /// Produce the provided value
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// use winnow::ascii::alpha1;
    /// # fn main() {
    ///
    /// let mut parser = alpha1.value(1234);
    ///
    /// assert_eq!(parser.parse_peek("abcd"), Ok(("", 1234)));
    /// assert_eq!(parser.parse_peek("123abcd;"), Err(ErrMode::Backtrack(InputError::new("123abcd;", ErrorKind::Slice))));
    /// # }
    /// ```
    #[doc(alias = "to")]
    #[inline(always)]
    fn value<O2>(self, val: O2) -> Value<Self, I, O, O2, E>
    where
        Self: core::marker::Sized,
        O2: Clone,
    {
        Value::new(self, val)
    }

    /// Produce a type's default value
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// use winnow::ascii::alpha1;
    /// # fn main() {
    ///
    /// let mut parser = alpha1.default_value::<u32>();
    ///
    /// assert_eq!(parser.parse_peek("abcd"), Ok(("", 0)));
    /// assert_eq!(parser.parse_peek("123abcd;"), Err(ErrMode::Backtrack(InputError::new("123abcd;", ErrorKind::Slice))));
    /// # }
    /// ```
    #[inline(always)]
    fn default_value<O2>(self) -> DefaultValue<Self, I, O, O2, E>
    where
        Self: core::marker::Sized,
        O2: core::default::Default,
    {
        DefaultValue::new(self)
    }

    /// Discards the output of the `Parser`
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// use winnow::ascii::alpha1;
    /// # fn main() {
    ///
    /// let mut parser = alpha1.void();
    ///
    /// assert_eq!(parser.parse_peek("abcd"), Ok(("", ())));
    /// assert_eq!(parser.parse_peek("123abcd;"), Err(ErrMode::Backtrack(InputError::new("123abcd;", ErrorKind::Slice))));
    /// # }
    /// ```
    #[inline(always)]
    fn void(self) -> Void<Self, I, O, E>
    where
        Self: core::marker::Sized,
    {
        Void::new(self)
    }

    /// Convert the parser's output to another type using [`std::convert::From`]
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::prelude::*;
    /// # use winnow::error::InputError;
    /// use winnow::ascii::alpha1;
    /// # fn main() {
    ///
    /// fn parser1<'s>(i: &mut &'s str) -> PResult<&'s str, InputError<&'s str>> {
    ///   alpha1(i)
    /// }
    ///
    /// let mut parser2 = parser1.output_into();
    ///
    /// // the parser converts the &str output of the child parser into a Vec<u8>
    /// let bytes: IResult<&str, Vec<u8>> = parser2.parse_peek("abcd");
    /// assert_eq!(bytes, Ok(("", vec![97, 98, 99, 100])));
    /// # }
    /// ```
    #[inline(always)]
    fn output_into<O2>(self) -> OutputInto<Self, I, O, O2, E>
    where
        Self: core::marker::Sized,
        O: Into<O2>,
    {
        OutputInto::new(self)
    }

    /// Produce the consumed input as produced value.
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// use winnow::ascii::{alpha1};
    /// use winnow::combinator::separated_pair;
    /// # fn main() {
    ///
    /// let mut parser = separated_pair(alpha1, ',', alpha1).take();
    ///
    /// assert_eq!(parser.parse_peek("abcd,efgh"), Ok(("", "abcd,efgh")));
    /// assert_eq!(parser.parse_peek("abcd;"),Err(ErrMode::Backtrack(InputError::new(";", ErrorKind::Tag))));
    /// # }
    /// ```
    #[doc(alias = "concat")]
    #[doc(alias = "recognize")]
    #[inline(always)]
    fn take(self) -> Take<Self, I, O, E>
    where
        Self: core::marker::Sized,
        I: Stream,
    {
        Take::new(self)
    }

    /// Replaced with [`Parser::take`]
    #[inline(always)]
    #[deprecated(since = "0.6.14", note = "Replaced with `Parser::take`")]
    fn recognize(self) -> Take<Self, I, O, E>
    where
        Self: core::marker::Sized,
        I: Stream,
    {
        Take::new(self)
    }

    /// Produce the consumed input with the output
    ///
    /// Functions similarly to [take][Parser::take] except it
    /// returns the parser output as well.
    ///
    /// This can be useful especially in cases where the output is not the same type
    /// as the input, or the input is a user defined type.
    ///
    /// Returned tuple is of the format `(produced output, consumed input)`.
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::prelude::*;
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError};
    /// use winnow::ascii::{alpha1};
    /// use winnow::token::literal;
    /// use winnow::combinator::separated_pair;
    ///
    /// fn inner_parser<'s>(input: &mut &'s str) -> PResult<bool, InputError<&'s str>> {
    ///     "1234".value(true).parse_next(input)
    /// }
    ///
    /// let mut consumed_parser = separated_pair(alpha1, ',', alpha1).value(true).with_taken();
    ///
    /// assert_eq!(consumed_parser.parse_peek("abcd,efgh1"), Ok(("1", (true, "abcd,efgh"))));
    /// assert_eq!(consumed_parser.parse_peek("abcd;"),Err(ErrMode::Backtrack(InputError::new(";", ErrorKind::Tag))));
    ///
    /// // the second output (representing the consumed input)
    /// // should be the same as that of the `take` parser.
    /// let mut take_parser = inner_parser.take();
    /// let mut consumed_parser = inner_parser.with_taken().map(|(output, consumed)| consumed);
    ///
    /// assert_eq!(take_parser.parse_peek("1234"), consumed_parser.parse_peek("1234"));
    /// assert_eq!(take_parser.parse_peek("abcd"), consumed_parser.parse_peek("abcd"));
    /// ```
    #[doc(alias = "consumed")]
    #[doc(alias = "with_recognized")]
    #[inline(always)]
    fn with_taken(self) -> WithTaken<Self, I, O, E>
    where
        Self: core::marker::Sized,
        I: Stream,
    {
        WithTaken::new(self)
    }

    /// Replaced with [`Parser::with_taken`]
    #[inline(always)]
    #[deprecated(since = "0.6.14", note = "Replaced with `Parser::with_taken`")]
    fn with_recognized(self) -> WithTaken<Self, I, O, E>
    where
        Self: core::marker::Sized,
        I: Stream,
    {
        WithTaken::new(self)
    }

    /// Produce the location of the consumed input as produced value.
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::prelude::*;
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, stream::Stream};
    /// use winnow::stream::Located;
    /// use winnow::ascii::alpha1;
    /// use winnow::combinator::separated_pair;
    ///
    /// let mut parser = separated_pair(alpha1.span(), ',', alpha1.span());
    ///
    /// assert_eq!(parser.parse(Located::new("abcd,efgh")), Ok((0..4, 5..9)));
    /// assert_eq!(parser.parse_peek(Located::new("abcd;")),Err(ErrMode::Backtrack(InputError::new(Located::new("abcd;").peek_slice(4).0, ErrorKind::Tag))));
    /// ```
    #[inline(always)]
    fn span(self) -> Span<Self, I, O, E>
    where
        Self: core::marker::Sized,
        I: Stream + Location,
    {
        Span::new(self)
    }

    /// Produce the location of consumed input with the output
    ///
    /// Functions similarly to [`Parser::span`] except it
    /// returns the parser output as well.
    ///
    /// This can be useful especially in cases where the output is not the same type
    /// as the input, or the input is a user defined type.
    ///
    /// Returned tuple is of the format `(produced output, consumed input)`.
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::prelude::*;
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, stream::Stream};
    /// use winnow::stream::Located;
    /// use winnow::ascii::alpha1;
    /// use winnow::token::literal;
    /// use winnow::combinator::separated_pair;
    ///
    /// fn inner_parser<'s>(input: &mut Located<&'s str>) -> PResult<bool, InputError<Located<&'s str>>> {
    ///     "1234".value(true).parse_next(input)
    /// }
    ///
    /// # fn main() {
    ///
    /// let mut consumed_parser = separated_pair(alpha1.value(1).with_span(), ',', alpha1.value(2).with_span());
    ///
    /// assert_eq!(consumed_parser.parse(Located::new("abcd,efgh")), Ok(((1, 0..4), (2, 5..9))));
    /// assert_eq!(consumed_parser.parse_peek(Located::new("abcd;")),Err(ErrMode::Backtrack(InputError::new(Located::new("abcd;").peek_slice(4).0, ErrorKind::Tag))));
    ///
    /// // the second output (representing the consumed input)
    /// // should be the same as that of the `span` parser.
    /// let mut span_parser = inner_parser.span();
    /// let mut consumed_parser = inner_parser.with_span().map(|(output, consumed)| consumed);
    ///
    /// assert_eq!(span_parser.parse_peek(Located::new("1234")), consumed_parser.parse_peek(Located::new("1234")));
    /// assert_eq!(span_parser.parse_peek(Located::new("abcd")), consumed_parser.parse_peek(Located::new("abcd")));
    /// # }
    /// ```
    #[inline(always)]
    fn with_span(self) -> WithSpan<Self, I, O, E>
    where
        Self: core::marker::Sized,
        I: Stream + Location,
    {
        WithSpan::new(self)
    }

    /// Maps a function over the output of a parser
    ///
    /// # Example
    ///
    /// ```rust
    /// use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// use winnow::ascii::digit1;
    /// # fn main() {
    ///
    /// let mut parser = digit1.map(|s: &str| s.len());
    ///
    /// // the parser will count how many characters were returned by digit1
    /// assert_eq!(parser.parse_peek("123456"), Ok(("", 6)));
    ///
    /// // this will fail if digit1 fails
    /// assert_eq!(parser.parse_peek("abc"), Err(ErrMode::Backtrack(InputError::new("abc", ErrorKind::Slice))));
    /// # }
    /// ```
    #[inline(always)]
    fn map<G, O2>(self, map: G) -> Map<Self, G, I, O, O2, E>
    where
        G: FnMut(O) -> O2,
        Self: core::marker::Sized,
    {
        Map::new(self, map)
    }

    /// Applies a function returning a `Result` over the output of a parser.
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// use winnow::ascii::digit1;
    /// # fn main() {
    ///
    /// let mut parse = digit1.try_map(|s: &str| s.parse::<u8>());
    ///
    /// // the parser will convert the result of digit1 to a number
    /// assert_eq!(parse.parse_peek("123"), Ok(("", 123)));
    ///
    /// // this will fail if digit1 fails
    /// assert_eq!(parse.parse_peek("abc"), Err(ErrMode::Backtrack(InputError::new("abc", ErrorKind::Slice))));
    ///
    /// // this will fail if the mapped function fails (a `u8` is too small to hold `123456`)
    /// assert_eq!(parse.parse_peek("123456"), Err(ErrMode::Backtrack(InputError::new("123456", ErrorKind::Verify))));
    /// # }
    /// ```
    #[inline(always)]
    fn try_map<G, O2, E2>(self, map: G) -> TryMap<Self, G, I, O, O2, E, E2>
    where
        Self: core::marker::Sized,
        G: FnMut(O) -> Result<O2, E2>,
        I: Stream,
        E: FromExternalError<I, E2>,
    {
        TryMap::new(self, map)
    }

    /// Apply both [`Parser::verify`] and [`Parser::map`].
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// use winnow::ascii::digit1;
    /// # fn main() {
    ///
    /// let mut parse = digit1.verify_map(|s: &str| s.parse::<u8>().ok());
    ///
    /// // the parser will convert the result of digit1 to a number
    /// assert_eq!(parse.parse_peek("123"), Ok(("", 123)));
    ///
    /// // this will fail if digit1 fails
    /// assert_eq!(parse.parse_peek("abc"), Err(ErrMode::Backtrack(InputError::new("abc", ErrorKind::Slice))));
    ///
    /// // this will fail if the mapped function fails (a `u8` is too small to hold `123456`)
    /// assert_eq!(parse.parse_peek("123456"), Err(ErrMode::Backtrack(InputError::new("123456", ErrorKind::Verify))));
    /// # }
    /// ```
    #[doc(alias = "satisfy_map")]
    #[doc(alias = "filter_map")]
    #[doc(alias = "map_opt")]
    #[inline(always)]
    fn verify_map<G, O2>(self, map: G) -> VerifyMap<Self, G, I, O, O2, E>
    where
        Self: core::marker::Sized,
        G: FnMut(O) -> Option<O2>,
        I: Stream,
        E: ParserError<I>,
    {
        VerifyMap::new(self, map)
    }

    /// Creates a parser from the output of this one
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, PResult, Parser};
    /// use winnow::token::take;
    /// use winnow::binary::u8;
    ///
    /// fn length_take<'s>(input: &mut &'s [u8]) -> PResult<&'s [u8], InputError<&'s [u8]>> {
    ///     u8.flat_map(take).parse_next(input)
    /// }
    ///
    /// assert_eq!(length_take.parse_peek(&[2, 0, 1, 2][..]), Ok((&[2][..], &[0, 1][..])));
    /// assert_eq!(length_take.parse_peek(&[4, 0, 1, 2][..]), Err(ErrMode::Backtrack(InputError::new(&[0, 1, 2][..], ErrorKind::Slice))));
    /// ```
    ///
    /// which is the same as
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, PResult, Parser};
    /// use winnow::token::take;
    /// use winnow::binary::u8;
    ///
    /// fn length_take<'s>(input: &mut &'s [u8]) -> PResult<&'s [u8], InputError<&'s [u8]>> {
    ///     let length = u8.parse_next(input)?;
    ///     let data = take(length).parse_next(input)?;
    ///     Ok(data)
    /// }
    ///
    /// assert_eq!(length_take.parse_peek(&[2, 0, 1, 2][..]), Ok((&[2][..], &[0, 1][..])));
    /// assert_eq!(length_take.parse_peek(&[4, 0, 1, 2][..]), Err(ErrMode::Backtrack(InputError::new(&[0, 1, 2][..], ErrorKind::Slice))));
    /// ```
    #[inline(always)]
    fn flat_map<G, H, O2>(self, map: G) -> FlatMap<Self, G, H, I, O, O2, E>
    where
        Self: core::marker::Sized,
        G: FnMut(O) -> H,
        H: Parser<I, O2, E>,
    {
        FlatMap::new(self, map)
    }

    /// Applies a second parser over the output of the first one
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// use winnow::ascii::digit1;
    /// use winnow::token::take;
    /// # fn main() {
    ///
    /// let mut digits = take(5u8).and_then(digit1);
    ///
    /// assert_eq!(digits.parse_peek("12345"), Ok(("", "12345")));
    /// assert_eq!(digits.parse_peek("123ab"), Ok(("", "123")));
    /// assert_eq!(digits.parse_peek("123"), Err(ErrMode::Backtrack(InputError::new("123", ErrorKind::Slice))));
    /// # }
    /// ```
    #[inline(always)]
    fn and_then<G, O2>(self, inner: G) -> AndThen<Self, G, I, O, O2, E>
    where
        Self: core::marker::Sized,
        G: Parser<O, O2, E>,
        O: StreamIsPartial,
        I: Stream,
    {
        AndThen::new(self, inner)
    }

    /// Apply [`std::str::FromStr`] to the output of the parser
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::prelude::*;
    /// use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// use winnow::ascii::digit1;
    ///
    /// fn parser<'s>(input: &mut &'s str) -> PResult<u64, InputError<&'s str>> {
    ///     digit1.parse_to().parse_next(input)
    /// }
    ///
    /// // the parser will count how many characters were returned by digit1
    /// assert_eq!(parser.parse_peek("123456"), Ok(("", 123456)));
    ///
    /// // this will fail if digit1 fails
    /// assert_eq!(parser.parse_peek("abc"), Err(ErrMode::Backtrack(InputError::new("abc", ErrorKind::Slice))));
    /// ```
    #[doc(alias = "from_str")]
    #[inline(always)]
    fn parse_to<O2>(self) -> ParseTo<Self, I, O, O2, E>
    where
        Self: core::marker::Sized,
        I: Stream,
        O: ParseSlice<O2>,
        E: ParserError<I>,
    {
        ParseTo::new(self)
    }

    /// Returns the output of the child parser if it satisfies a verification function.
    ///
    /// The verification function takes as argument a reference to the output of the
    /// parser.
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode,error::ErrorKind, error::InputError, Parser};
    /// # use winnow::ascii::alpha1;
    /// # fn main() {
    ///
    /// let mut parser = alpha1.verify(|s: &str| s.len() == 4);
    ///
    /// assert_eq!(parser.parse_peek("abcd"), Ok(("", "abcd")));
    /// assert_eq!(parser.parse_peek("abcde"), Err(ErrMode::Backtrack(InputError::new("abcde", ErrorKind::Verify))));
    /// assert_eq!(parser.parse_peek("123abcd;"),Err(ErrMode::Backtrack(InputError::new("123abcd;", ErrorKind::Slice))));
    /// # }
    /// ```
    #[doc(alias = "satisfy")]
    #[doc(alias = "filter")]
    #[inline(always)]
    fn verify<G, O2>(self, filter: G) -> Verify<Self, G, I, O, O2, E>
    where
        Self: core::marker::Sized,
        G: FnMut(&O2) -> bool,
        I: Stream,
        O: crate::lib::std::borrow::Borrow<O2>,
        O2: ?Sized,
        E: ParserError<I>,
    {
        Verify::new(self, filter)
    }

    /// If parsing fails, add context to the error
    ///
    /// This is used mainly to add user friendly information
    /// to errors when backtracking through a parse tree.
    #[doc(alias = "labelled")]
    #[inline(always)]
    fn context<C>(self, context: C) -> Context<Self, I, O, E, C>
    where
        Self: core::marker::Sized,
        I: Stream,
        E: AddContext<I, C>,
        C: Clone + crate::lib::std::fmt::Debug,
    {
        Context::new(self, context)
    }

    /// Transforms [`Incomplete`][crate::error::ErrMode::Incomplete] into [`Backtrack`][crate::error::ErrMode::Backtrack]
    ///
    /// # Example
    ///
    /// ```rust
    /// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError, stream::Partial, Parser};
    /// # use winnow::token::take;
    /// # fn main() {
    ///
    /// let mut parser = take(5u8).complete_err();
    ///
    /// assert_eq!(parser.parse_peek(Partial::new("abcdefg")), Ok((Partial::new("fg"), "abcde")));
    /// assert_eq!(parser.parse_peek(Partial::new("abcd")), Err(ErrMode::Backtrack(InputError::new(Partial::new("abcd"), ErrorKind::Complete))));
    /// # }
    /// ```
    #[inline(always)]
    fn complete_err(self) -> CompleteErr<Self>
    where
        Self: core::marker::Sized,
    {
        CompleteErr::new(self)
    }

    /// Convert the parser's error to another type using [`std::convert::From`]
    #[inline(always)]
    fn err_into<E2>(self) -> ErrInto<Self, I, O, E, E2>
    where
        Self: core::marker::Sized,
        E: Into<E2>,
    {
        ErrInto::new(self)
    }

    /// Recover from an error by skipping everything `recover` consumes and trying again
    ///
    /// If `recover` consumes nothing, the error is returned, allowing an alternative recovery
    /// method.
    ///
    /// This commits the parse result, preventing alternative branch paths like with
    /// [`winnow::combinator::alt`][crate::combinator::alt].
    #[inline(always)]
    #[cfg(feature = "unstable-recover")]
    #[cfg(feature = "std")]
    fn retry_after<R>(self, recover: R) -> RetryAfter<Self, R, I, O, E>
    where
        Self: core::marker::Sized,
        R: Parser<I, (), E>,
        I: Stream,
        I: Recover<E>,
        E: FromRecoverableError<I, E>,
    {
        RetryAfter::new(self, recover)
    }

    /// Recover from an error by skipping this parse and everything `recover` consumes
    ///
    /// This commits the parse result, preventing alternative branch paths like with
    /// [`winnow::combinator::alt`][crate::combinator::alt].
    #[inline(always)]
    #[cfg(feature = "unstable-recover")]
    #[cfg(feature = "std")]
    fn resume_after<R>(self, recover: R) -> ResumeAfter<Self, R, I, O, E>
    where
        Self: core::marker::Sized,
        R: Parser<I, (), E>,
        I: Stream,
        I: Recover<E>,
        E: FromRecoverableError<I, E>,
    {
        ResumeAfter::new(self, recover)
    }
}

impl<'a, I, O, E, F> Parser<I, O, E> for F
where
    F: FnMut(&mut I) -> PResult<O, E> + 'a,
    I: Stream,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<O, E> {
        self(i)
    }
}

/// This is a shortcut for [`one_of`][crate::token::one_of].
///
/// # Example
///
/// ```
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{ErrorKind, InputError}};
/// fn parser<'s>(i: &mut &'s [u8]) -> PResult<u8, InputError<&'s [u8]>>  {
///     b'a'.parse_next(i)
/// }
/// assert_eq!(parser.parse_peek(&b"abc"[..]), Ok((&b"bc"[..], b'a')));
/// assert_eq!(parser.parse_peek(&b" abc"[..]), Err(ErrMode::Backtrack(InputError::new(&b" abc"[..], ErrorKind::Tag))));
/// assert_eq!(parser.parse_peek(&b"bc"[..]), Err(ErrMode::Backtrack(InputError::new(&b"bc"[..], ErrorKind::Tag))));
/// assert_eq!(parser.parse_peek(&b""[..]), Err(ErrMode::Backtrack(InputError::new(&b""[..], ErrorKind::Tag))));
/// ```
impl<I, E> Parser<I, u8, E> for u8
where
    I: StreamIsPartial,
    I: Stream,
    I: Compare<u8>,
    E: ParserError<I>,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<u8, E> {
        crate::token::literal(*self).value(*self).parse_next(i)
    }
}

/// This is a shortcut for [`one_of`][crate::token::one_of].
///
/// # Example
///
/// ```
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{ErrorKind, InputError}};
/// fn parser<'s>(i: &mut &'s str) -> PResult<char, InputError<&'s str>> {
///     'a'.parse_next(i)
/// }
/// assert_eq!(parser.parse_peek("abc"), Ok(("bc", 'a')));
/// assert_eq!(parser.parse_peek(" abc"), Err(ErrMode::Backtrack(InputError::new(" abc", ErrorKind::Tag))));
/// assert_eq!(parser.parse_peek("bc"), Err(ErrMode::Backtrack(InputError::new("bc", ErrorKind::Tag))));
/// assert_eq!(parser.parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Tag))));
/// ```
impl<I, E> Parser<I, char, E> for char
where
    I: StreamIsPartial,
    I: Stream,
    I: Compare<char>,
    E: ParserError<I>,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<char, E> {
        crate::token::literal(*self).value(*self).parse_next(i)
    }
}

/// This is a shortcut for [`literal`][crate::token::literal].
///
/// # Example
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::combinator::alt;
/// # use winnow::token::take;
///
/// fn parser<'s>(s: &mut &'s [u8]) -> PResult<&'s [u8], InputError<&'s [u8]>> {
///   alt((&"Hello"[..], take(5usize))).parse_next(s)
/// }
///
/// assert_eq!(parser.parse_peek(&b"Hello, World!"[..]), Ok((&b", World!"[..], &b"Hello"[..])));
/// assert_eq!(parser.parse_peek(&b"Something"[..]), Ok((&b"hing"[..], &b"Somet"[..])));
/// assert_eq!(parser.parse_peek(&b"Some"[..]), Err(ErrMode::Backtrack(InputError::new(&b"Some"[..], ErrorKind::Slice))));
/// assert_eq!(parser.parse_peek(&b""[..]), Err(ErrMode::Backtrack(InputError::new(&b""[..], ErrorKind::Slice))));
/// ```
impl<'s, I, E: ParserError<I>> Parser<I, <I as Stream>::Slice, E> for &'s [u8]
where
    I: Compare<&'s [u8]> + StreamIsPartial,
    I: Stream,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<<I as Stream>::Slice, E> {
        crate::token::literal(*self).parse_next(i)
    }
}

/// This is a shortcut for [`literal`][crate::token::literal].
///
/// # Example
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::combinator::alt;
/// # use winnow::token::take;
/// use winnow::ascii::Caseless;
///
/// fn parser<'s>(s: &mut &'s [u8]) -> PResult<&'s [u8], InputError<&'s [u8]>> {
///   alt((Caseless(&"hello"[..]), take(5usize))).parse_next(s)
/// }
///
/// assert_eq!(parser.parse_peek(&b"Hello, World!"[..]), Ok((&b", World!"[..], &b"Hello"[..])));
/// assert_eq!(parser.parse_peek(&b"hello, World!"[..]), Ok((&b", World!"[..], &b"hello"[..])));
/// assert_eq!(parser.parse_peek(&b"HeLlo, World!"[..]), Ok((&b", World!"[..], &b"HeLlo"[..])));
/// assert_eq!(parser.parse_peek(&b"Something"[..]), Ok((&b"hing"[..], &b"Somet"[..])));
/// assert_eq!(parser.parse_peek(&b"Some"[..]), Err(ErrMode::Backtrack(InputError::new(&b"Some"[..], ErrorKind::Slice))));
/// assert_eq!(parser.parse_peek(&b""[..]), Err(ErrMode::Backtrack(InputError::new(&b""[..], ErrorKind::Slice))));
/// ```
impl<'s, I, E: ParserError<I>> Parser<I, <I as Stream>::Slice, E> for AsciiCaseless<&'s [u8]>
where
    I: Compare<AsciiCaseless<&'s [u8]>> + StreamIsPartial,
    I: Stream,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<<I as Stream>::Slice, E> {
        crate::token::literal(*self).parse_next(i)
    }
}

/// This is a shortcut for [`literal`][crate::token::literal].
///
/// # Example
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::combinator::alt;
/// # use winnow::token::take;
///
/// fn parser<'s>(s: &mut &'s [u8]) -> PResult<&'s [u8], InputError<&'s [u8]>> {
///   alt((b"Hello", take(5usize))).parse_next(s)
/// }
///
/// assert_eq!(parser.parse_peek(&b"Hello, World!"[..]), Ok((&b", World!"[..], &b"Hello"[..])));
/// assert_eq!(parser.parse_peek(&b"Something"[..]), Ok((&b"hing"[..], &b"Somet"[..])));
/// assert_eq!(parser.parse_peek(&b"Some"[..]), Err(ErrMode::Backtrack(InputError::new(&b"Some"[..], ErrorKind::Slice))));
/// assert_eq!(parser.parse_peek(&b""[..]), Err(ErrMode::Backtrack(InputError::new(&b""[..], ErrorKind::Slice))));
/// ```
impl<'s, I, E: ParserError<I>, const N: usize> Parser<I, <I as Stream>::Slice, E> for &'s [u8; N]
where
    I: Compare<&'s [u8; N]> + StreamIsPartial,
    I: Stream,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<<I as Stream>::Slice, E> {
        crate::token::literal(*self).parse_next(i)
    }
}

/// This is a shortcut for [`literal`][crate::token::literal].
///
/// # Example
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}, error::Needed};
/// # use winnow::combinator::alt;
/// # use winnow::token::take;
/// use winnow::ascii::Caseless;
///
/// fn parser<'s>(s: &mut &'s [u8]) -> PResult<&'s [u8], InputError<&'s [u8]>> {
///   alt((Caseless(b"hello"), take(5usize))).parse_next(s)
/// }
///
/// assert_eq!(parser.parse_peek(&b"Hello, World!"[..]), Ok((&b", World!"[..], &b"Hello"[..])));
/// assert_eq!(parser.parse_peek(&b"hello, World!"[..]), Ok((&b", World!"[..], &b"hello"[..])));
/// assert_eq!(parser.parse_peek(&b"HeLlo, World!"[..]), Ok((&b", World!"[..], &b"HeLlo"[..])));
/// assert_eq!(parser.parse_peek(&b"Something"[..]), Ok((&b"hing"[..], &b"Somet"[..])));
/// assert_eq!(parser.parse_peek(&b"Some"[..]), Err(ErrMode::Backtrack(InputError::new(&b"Some"[..], ErrorKind::Slice))));
/// assert_eq!(parser.parse_peek(&b""[..]), Err(ErrMode::Backtrack(InputError::new(&b""[..], ErrorKind::Slice))));
/// ```
impl<'s, I, E: ParserError<I>, const N: usize> Parser<I, <I as Stream>::Slice, E>
    for AsciiCaseless<&'s [u8; N]>
where
    I: Compare<AsciiCaseless<&'s [u8; N]>> + StreamIsPartial,
    I: Stream,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<<I as Stream>::Slice, E> {
        crate::token::literal(*self).parse_next(i)
    }
}

/// This is a shortcut for [`literal`][crate::token::literal].
///
/// # Example
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}};
/// # use winnow::combinator::alt;
/// # use winnow::token::take;
///
/// fn parser<'s>(s: &mut &'s str) -> PResult<&'s str, InputError<&'s str>> {
///   alt(("Hello", take(5usize))).parse_next(s)
/// }
///
/// assert_eq!(parser.parse_peek("Hello, World!"), Ok((", World!", "Hello")));
/// assert_eq!(parser.parse_peek("Something"), Ok(("hing", "Somet")));
/// assert_eq!(parser.parse_peek("Some"), Err(ErrMode::Backtrack(InputError::new("Some", ErrorKind::Slice))));
/// assert_eq!(parser.parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Slice))));
/// ```
impl<'s, I, E: ParserError<I>> Parser<I, <I as Stream>::Slice, E> for &'s str
where
    I: Compare<&'s str> + StreamIsPartial,
    I: Stream,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<<I as Stream>::Slice, E> {
        crate::token::literal(*self).parse_next(i)
    }
}

/// This is a shortcut for [`literal`][crate::token::literal].
///
/// # Example
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::{InputError, ErrorKind}};
/// # use winnow::combinator::alt;
/// # use winnow::token::take;
/// # use winnow::ascii::Caseless;
///
/// fn parser<'s>(s: &mut &'s str) -> PResult<&'s str, InputError<&'s str>> {
///   alt((Caseless("hello"), take(5usize))).parse_next(s)
/// }
///
/// assert_eq!(parser.parse_peek("Hello, World!"), Ok((", World!", "Hello")));
/// assert_eq!(parser.parse_peek("hello, World!"), Ok((", World!", "hello")));
/// assert_eq!(parser.parse_peek("HeLlo, World!"), Ok((", World!", "HeLlo")));
/// assert_eq!(parser.parse_peek("Something"), Ok(("hing", "Somet")));
/// assert_eq!(parser.parse_peek("Some"), Err(ErrMode::Backtrack(InputError::new("Some", ErrorKind::Slice))));
/// assert_eq!(parser.parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Slice))));
/// ```
impl<'s, I, E: ParserError<I>> Parser<I, <I as Stream>::Slice, E> for AsciiCaseless<&'s str>
where
    I: Compare<AsciiCaseless<&'s str>> + StreamIsPartial,
    I: Stream,
{
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<<I as Stream>::Slice, E> {
        crate::token::literal(*self).parse_next(i)
    }
}

impl<I: Stream, E: ParserError<I>> Parser<I, (), E> for () {
    #[inline(always)]
    fn parse_next(&mut self, _i: &mut I) -> PResult<(), E> {
        Ok(())
    }
}

macro_rules! impl_parser_for_tuple {
  ($($parser:ident $output:ident),+) => (
    #[allow(non_snake_case)]
    impl<I: Stream, $($output),+, E: ParserError<I>, $($parser),+> Parser<I, ($($output),+,), E> for ($($parser),+,)
    where
      $($parser: Parser<I, $output, E>),+
    {
      #[inline(always)]
      fn parse_next(&mut self, i: &mut I) -> PResult<($($output),+,), E> {
        let ($(ref mut $parser),+,) = *self;

        $(let $output = $parser.parse_next(i)?;)+

        Ok(($($output),+,))
      }
    }
  )
}

macro_rules! impl_parser_for_tuples {
    ($parser1:ident $output1:ident, $($parser:ident $output:ident),+) => {
        impl_parser_for_tuples!(__impl $parser1 $output1; $($parser $output),+);
    };
    (__impl $($parser:ident $output:ident),+; $parser1:ident $output1:ident $(,$parser2:ident $output2:ident)*) => {
        impl_parser_for_tuple!($($parser $output),+);
        impl_parser_for_tuples!(__impl $($parser $output),+, $parser1 $output1; $($parser2 $output2),*);
    };
    (__impl $($parser:ident $output:ident),+;) => {
        impl_parser_for_tuple!($($parser $output),+);
    }
}

/// Collect all errors when parsing the input
///
/// [`Parser`]s will need to use [`Recoverable<I, _>`] for their input.
#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
pub trait RecoverableParser<I, O, R, E> {
    /// Collect all errors when parsing the input
    ///
    /// If `self` fails, this acts like [`Parser::resume_after`] and returns `Ok(None)`.
    /// Generally, this should be avoided by using
    /// [`Parser::retry_after`] and [`Parser::resume_after`] throughout your parser.
    ///
    /// The empty `input` is returned to allow turning the errors into [`ParserError`]s.
    fn recoverable_parse(&mut self, input: I) -> (I, Option<O>, Vec<R>);
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<P, I, O, R, E> RecoverableParser<I, O, R, E> for P
where
    P: Parser<Recoverable<I, R>, O, E>,
    I: Stream,
    I: StreamIsPartial,
    R: FromRecoverableError<Recoverable<I, R>, E>,
    R: crate::lib::std::fmt::Debug,
    E: FromRecoverableError<Recoverable<I, R>, E>,
    E: ParserError<Recoverable<I, R>>,
    E: crate::lib::std::fmt::Debug,
{
    #[inline]
    fn recoverable_parse(&mut self, input: I) -> (I, Option<O>, Vec<R>) {
        debug_assert!(
            !I::is_partial_supported(),
            "partial streams need to handle `ErrMode::Incomplete`"
        );

        let start = input.checkpoint();
        let mut input = Recoverable::new(input);
        let start_token = input.checkpoint();
        let result = (
            self.by_ref(),
            crate::combinator::eof.resume_after(rest.void()),
        )
            .parse_next(&mut input);

        let (o, err) = match result {
            Ok((o, _)) => (Some(o), None),
            Err(err) => {
                let err = err
                    .into_inner()
                    .expect("complete parsers should not report `ErrMode::Incomplete(_)`");
                let err_start = input.checkpoint();
                let err = R::from_recoverable_error(&start_token, &err_start, &input, err);
                (None, Some(err))
            }
        };

        let (mut input, mut errs) = input.into_parts();
        input.reset(&start);
        if let Some(err) = err {
            errs.push(err);
        }

        (input, o, errs)
    }
}

impl_parser_for_tuples!(
  P1 O1,
  P2 O2,
  P3 O3,
  P4 O4,
  P5 O5,
  P6 O6,
  P7 O7,
  P8 O8,
  P9 O9,
  P10 O10,
  P11 O11,
  P12 O12,
  P13 O13,
  P14 O14,
  P15 O15,
  P16 O16,
  P17 O17,
  P18 O18,
  P19 O19,
  P20 O20,
  P21 O21
);

#[cfg(feature = "alloc")]
use crate::lib::std::boxed::Box;

#[cfg(feature = "alloc")]
impl<'a, I, O, E> Parser<I, O, E> for Box<dyn Parser<I, O, E> + 'a> {
    #[inline(always)]
    fn parse_next(&mut self, i: &mut I) -> PResult<O, E> {
        (**self).parse_next(i)
    }
}

/// Convert a [`Parser::parse_peek`] style parse function to be a [`Parser`]
#[inline(always)]
pub fn unpeek<'a, I, O, E>(
    mut peek: impl FnMut(I) -> IResult<I, O, E> + 'a,
) -> impl FnMut(&mut I) -> PResult<O, E>
where
    I: Clone,
{
    move |input| match peek((*input).clone()) {
        Ok((i, o)) => {
            *input = i;
            Ok(o)
        }
        Err(err) => Err(err),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::binary::be_u16;
    use crate::error::ErrMode;
    use crate::error::ErrorKind;
    use crate::error::InputError;
    use crate::error::Needed;
    use crate::token::take;
    use crate::Partial;

    #[doc(hidden)]
    #[macro_export]
    macro_rules! assert_size (
    ($t:ty, $sz:expr) => (
      assert!($crate::lib::std::mem::size_of::<$t>() <= $sz, "{} <= {} failed", $crate::lib::std::mem::size_of::<$t>(), $sz);
    );
  );

    #[test]
    #[cfg(target_pointer_width = "64")]
    fn size_test() {
        assert_size!(IResult<&[u8], &[u8], (&[u8], u32)>, 40);
        assert_size!(IResult<&str, &str, u32>, 40);
        assert_size!(Needed, 8);
        assert_size!(ErrMode<u32>, 16);
        assert_size!(ErrorKind, 1);
    }

    #[test]
    fn err_map_test() {
        let e = ErrMode::Backtrack(1);
        assert_eq!(e.map(|v| v + 1), ErrMode::Backtrack(2));
    }

    #[test]
    fn single_element_tuples() {
        use crate::ascii::alpha1;
        use crate::error::ErrorKind;

        let mut parser = (alpha1,);
        assert_eq!(parser.parse_peek("abc123def"), Ok(("123def", ("abc",))));
        assert_eq!(
            parser.parse_peek("123def"),
            Err(ErrMode::Backtrack(InputError::new(
                "123def",
                ErrorKind::Slice
            )))
        );
    }

    #[test]
    fn tuple_test() {
        #[allow(clippy::type_complexity)]
        fn tuple_3(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, (u16, &[u8], &[u8])> {
            (be_u16, take(3u8), "fg").parse_peek(i)
        }

        assert_eq!(
            tuple_3(Partial::new(&b"abcdefgh"[..])),
            Ok((
                Partial::new(&b"h"[..]),
                (0x6162u16, &b"cde"[..], &b"fg"[..])
            ))
        );
        assert_eq!(
            tuple_3(Partial::new(&b"abcd"[..])),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            tuple_3(Partial::new(&b"abcde"[..])),
            Err(ErrMode::Incomplete(Needed::new(2)))
        );
        assert_eq!(
            tuple_3(Partial::new(&b"abcdejk"[..])),
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(&b"jk"[..]),
                ErrorKind::Tag
            )))
        );
    }

    #[test]
    fn unit_type() {
        fn parser(i: &mut &str) -> PResult<()> {
            ().parse_next(i)
        }
        assert_eq!(parser.parse_peek("abxsbsh"), Ok(("abxsbsh", ())));
        assert_eq!(parser.parse_peek("sdfjakdsas"), Ok(("sdfjakdsas", ())));
        assert_eq!(parser.parse_peek(""), Ok(("", ())));
    }
}
