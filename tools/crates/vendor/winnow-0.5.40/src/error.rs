//! # Error management
//!
//! Errors are designed with multiple needs in mind:
//! - Accumulate more [context][Parser::context] as the error goes up the parser chain
//! - Distinguish between [recoverable errors,
//!   unrecoverable errors, and more data is needed][ErrMode]
//! - Have a very low overhead, as errors are often discarded by the calling parser (examples: `repeat`, `alt`)
//! - Can be modified according to the user's needs, because some languages need a lot more information
//! - Help thread-through the [stream][crate::stream]
//!
//! To abstract these needs away from the user, generally `winnow` parsers use the [`PResult`]
//! alias, rather than [`Result`].  [`Parser::parse`] is a top-level operation
//! that can help convert to a `Result` for integrating with your application's error reporting.
//!
//! Error types include:
//! - `()`
//! - [`ErrorKind`]
//! - [`InputError`] (mostly for testing)
//! - [`ContextError`]
//! - [`TreeError`] (mostly for testing)
//! - [Custom errors][crate::_topic::error]

#[cfg(feature = "alloc")]
use crate::lib::std::borrow::ToOwned;
use crate::lib::std::fmt;
use core::num::NonZeroUsize;

use crate::stream::AsBStr;
use crate::stream::Stream;
#[allow(unused_imports)] // Here for intra-doc links
use crate::Parser;

/// For use with [`Parser::parse_peek`] which allows the input stream to be threaded through a
/// parser.
///
/// - `Ok((I, O))` is the remaining [input][crate::stream] and the parsed value
/// - [`Err(ErrMode<E>)`][ErrMode] is the error along with how to respond to it
///
/// By default, the error type (`E`) is [`InputError`]
///
/// When integrating into the result of the application, see
/// - [`Parser::parse`]
/// - [`ErrMode::into_inner`]
pub type IResult<I, O, E = InputError<I>> = PResult<(I, O), E>;

/// For use with [`Parser::parse_next`]
///
/// - `Ok(O)` is the parsed value
/// - [`Err(ErrMode<E>)`][ErrMode] is the error along with how to respond to it
///
/// By default, the error type (`E`) is [`ContextError`].
///
/// When integrating into the result of the application, see
/// - [`Parser::parse`]
/// - [`ErrMode::into_inner`]
pub type PResult<O, E = ContextError> = Result<O, ErrMode<E>>;

/// Contains information on needed data if a parser returned `Incomplete`
///
/// **Note:** This is only possible for `Stream` that are [partial][`crate::stream::StreamIsPartial`],
/// like [`Partial`][crate::Partial].
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
#[cfg_attr(nightly, warn(rustdoc::missing_doc_code_examples))]
pub enum Needed {
    /// Needs more data, but we do not know how much
    Unknown,
    /// Contains the required data size in bytes
    Size(NonZeroUsize),
}

impl Needed {
    /// Creates `Needed` instance, returns `Needed::Unknown` if the argument is zero
    pub fn new(s: usize) -> Self {
        match NonZeroUsize::new(s) {
            Some(sz) => Needed::Size(sz),
            None => Needed::Unknown,
        }
    }

    /// Indicates if we know how many bytes we need
    pub fn is_known(&self) -> bool {
        *self != Needed::Unknown
    }

    /// Maps a `Needed` to `Needed` by applying a function to a contained `Size` value.
    #[inline]
    pub fn map<F: Fn(NonZeroUsize) -> usize>(self, f: F) -> Needed {
        match self {
            Needed::Unknown => Needed::Unknown,
            Needed::Size(n) => Needed::new(f(n)),
        }
    }
}

/// Add parse error state to [`ParserError`]s
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(nightly, warn(rustdoc::missing_doc_code_examples))]
pub enum ErrMode<E> {
    /// There was not enough data to determine the appropriate action
    ///
    /// More data needs to be buffered before retrying the parse.
    ///
    /// This must only be set when the [`Stream`] is [partial][`crate::stream::StreamIsPartial`], like with
    /// [`Partial`][crate::Partial]
    ///
    /// Convert this into an `Backtrack` with [`Parser::complete_err`]
    Incomplete(Needed),
    /// The parser failed with a recoverable error (the default).
    ///
    /// For example, a parser for json values might include a
    /// [`dec_uint`][crate::ascii::dec_uint] as one case in an [`alt`][crate::combinator::alt]
    /// combinator. If it fails, the next case should be tried.
    Backtrack(E),
    /// The parser had an unrecoverable error.
    ///
    /// The parser was on the right branch, so directly report it to the user rather than trying
    /// other branches. You can use [`cut_err()`][crate::combinator::cut_err] combinator to switch
    /// from `ErrMode::Backtrack` to `ErrMode::Cut`.
    ///
    /// For example, one case in an [`alt`][crate::combinator::alt] combinator found a unique prefix
    /// and you want any further errors parsing the case to be reported to the user.
    Cut(E),
}

impl<E> ErrMode<E> {
    /// Tests if the result is Incomplete
    #[inline]
    pub fn is_incomplete(&self) -> bool {
        matches!(self, ErrMode::Incomplete(_))
    }

    /// Prevent backtracking, bubbling the error up to the top
    pub fn cut(self) -> Self {
        match self {
            ErrMode::Backtrack(e) => ErrMode::Cut(e),
            rest => rest,
        }
    }

    /// Enable backtracking support
    pub fn backtrack(self) -> Self {
        match self {
            ErrMode::Cut(e) => ErrMode::Backtrack(e),
            rest => rest,
        }
    }

    /// Applies the given function to the inner error
    pub fn map<E2, F>(self, f: F) -> ErrMode<E2>
    where
        F: FnOnce(E) -> E2,
    {
        match self {
            ErrMode::Incomplete(n) => ErrMode::Incomplete(n),
            ErrMode::Cut(t) => ErrMode::Cut(f(t)),
            ErrMode::Backtrack(t) => ErrMode::Backtrack(f(t)),
        }
    }

    /// Automatically converts between errors if the underlying type supports it
    pub fn convert<F>(self) -> ErrMode<F>
    where
        E: ErrorConvert<F>,
    {
        self.map(ErrorConvert::convert)
    }

    /// Unwrap the mode, returning the underlying error
    ///
    /// Returns `None` for [`ErrMode::Incomplete`]
    #[cfg_attr(debug_assertions, track_caller)]
    #[inline(always)]
    pub fn into_inner(self) -> Option<E> {
        match self {
            ErrMode::Backtrack(e) | ErrMode::Cut(e) => Some(e),
            ErrMode::Incomplete(_) => None,
        }
    }
}

impl<I, E: ParserError<I>> ParserError<I> for ErrMode<E> {
    #[inline(always)]
    fn from_error_kind(input: &I, kind: ErrorKind) -> Self {
        ErrMode::Backtrack(E::from_error_kind(input, kind))
    }

    #[cfg_attr(debug_assertions, track_caller)]
    #[inline(always)]
    fn assert(input: &I, message: &'static str) -> Self
    where
        I: crate::lib::std::fmt::Debug,
    {
        ErrMode::Backtrack(E::assert(input, message))
    }

    #[inline]
    fn append(self, input: &I, kind: ErrorKind) -> Self {
        match self {
            ErrMode::Backtrack(e) => ErrMode::Backtrack(e.append(input, kind)),
            e => e,
        }
    }

    fn or(self, other: Self) -> Self {
        match (self, other) {
            (ErrMode::Backtrack(e), ErrMode::Backtrack(o)) => ErrMode::Backtrack(e.or(o)),
            (ErrMode::Incomplete(e), _) | (_, ErrMode::Incomplete(e)) => ErrMode::Incomplete(e),
            (ErrMode::Cut(e), _) | (_, ErrMode::Cut(e)) => ErrMode::Cut(e),
        }
    }
}

impl<I, EXT, E> FromExternalError<I, EXT> for ErrMode<E>
where
    E: FromExternalError<I, EXT>,
{
    #[inline(always)]
    fn from_external_error(input: &I, kind: ErrorKind, e: EXT) -> Self {
        ErrMode::Backtrack(E::from_external_error(input, kind, e))
    }
}

impl<I, C, E: AddContext<I, C>> AddContext<I, C> for ErrMode<E> {
    #[inline(always)]
    fn add_context(self, input: &I, ctx: C) -> Self {
        self.map(|err| err.add_context(input, ctx))
    }
}

impl<T: Clone> ErrMode<InputError<T>> {
    /// Maps `ErrMode<InputError<T>>` to `ErrMode<InputError<U>>` with the given `F: T -> U`
    pub fn map_input<U: Clone, F>(self, f: F) -> ErrMode<InputError<U>>
    where
        F: FnOnce(T) -> U,
    {
        match self {
            ErrMode::Incomplete(n) => ErrMode::Incomplete(n),
            ErrMode::Cut(InputError { input, kind }) => ErrMode::Cut(InputError {
                input: f(input),
                kind,
            }),
            ErrMode::Backtrack(InputError { input, kind }) => ErrMode::Backtrack(InputError {
                input: f(input),
                kind,
            }),
        }
    }
}

impl<E: Eq> Eq for ErrMode<E> {}

impl<E> fmt::Display for ErrMode<E>
where
    E: fmt::Debug,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ErrMode::Incomplete(Needed::Size(u)) => write!(f, "Parsing requires {} bytes/chars", u),
            ErrMode::Incomplete(Needed::Unknown) => write!(f, "Parsing requires more data"),
            ErrMode::Cut(c) => write!(f, "Parsing Failure: {:?}", c),
            ErrMode::Backtrack(c) => write!(f, "Parsing Error: {:?}", c),
        }
    }
}

/// The basic [`Parser`] trait for errors
///
/// It provides methods to create an error from some combinators,
/// and combine existing errors in combinators like `alt`.
pub trait ParserError<I>: Sized {
    /// Creates an error from the input position and an [`ErrorKind`]
    fn from_error_kind(input: &I, kind: ErrorKind) -> Self;

    /// Process a parser assertion
    #[cfg_attr(debug_assertions, track_caller)]
    fn assert(input: &I, _message: &'static str) -> Self
    where
        I: crate::lib::std::fmt::Debug,
    {
        #[cfg(debug_assertions)]
        panic!("assert `{}` failed at {:#?}", _message, input);
        #[cfg(not(debug_assertions))]
        Self::from_error_kind(input, ErrorKind::Assert)
    }

    /// Like [`ParserError::from_error_kind`] but merges it with the existing error.
    ///
    /// This is useful when backtracking through a parse tree, accumulating error context on the
    /// way.
    fn append(self, input: &I, kind: ErrorKind) -> Self;

    /// Combines errors from two different parse branches.
    ///
    /// For example, this would be used by [`alt`][crate::combinator::alt] to report the error from
    /// each case.
    #[inline]
    fn or(self, other: Self) -> Self {
        other
    }
}

/// Used by [`Parser::context`] to add custom data to error while backtracking
///
/// May be implemented multiple times for different kinds of context.
pub trait AddContext<I, C = &'static str>: Sized {
    /// Append to an existing error custom data
    ///
    /// This is used mainly by [`Parser::context`], to add user friendly information
    /// to errors when backtracking through a parse tree
    #[inline]
    fn add_context(self, _input: &I, _ctx: C) -> Self {
        self
    }
}

/// Capture context from when an error was recovered
pub trait FromRecoverableError<I: Stream, E> {
    /// Capture context from when an error was recovered
    fn from_recoverable_error(
        token_start: &<I as Stream>::Checkpoint,
        err_start: &<I as Stream>::Checkpoint,
        input: &I,
        e: E,
    ) -> Self;
}

/// Create a new error with an external error, from [`std::str::FromStr`]
///
/// This trait is required by the [`Parser::try_map`] combinator.
pub trait FromExternalError<I, E> {
    /// Like [`ParserError::from_error_kind`] but also include an external error.
    fn from_external_error(input: &I, kind: ErrorKind, e: E) -> Self;
}

/// Equivalent of `From` implementation to avoid orphan rules in bits parsers
pub trait ErrorConvert<E> {
    /// Transform to another error type
    fn convert(self) -> E;
}

/// Capture input on error
///
/// This is useful for testing of generic parsers to ensure the error happens at the right
/// location.
///
/// **Note:** [context][Parser::context] and inner errors (like from [`Parser::try_map`]) will be
/// dropped.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct InputError<I: Clone> {
    /// The input stream, pointing to the location where the error occurred
    pub input: I,
    /// A rudimentary error kind
    pub kind: ErrorKind,
}

impl<I: Clone> InputError<I> {
    /// Creates a new basic error
    #[inline]
    pub fn new(input: I, kind: ErrorKind) -> Self {
        Self { input, kind }
    }

    /// Translate the input type
    #[inline]
    pub fn map_input<I2: Clone, O: Fn(I) -> I2>(self, op: O) -> InputError<I2> {
        InputError {
            input: op(self.input),
            kind: self.kind,
        }
    }
}

#[cfg(feature = "alloc")]
impl<'i, I: ToOwned> InputError<&'i I>
where
    <I as ToOwned>::Owned: Clone,
{
    /// Obtaining ownership
    pub fn into_owned(self) -> InputError<<I as ToOwned>::Owned> {
        self.map_input(ToOwned::to_owned)
    }
}

impl<I: Clone> ParserError<I> for InputError<I> {
    #[inline]
    fn from_error_kind(input: &I, kind: ErrorKind) -> Self {
        Self {
            input: input.clone(),
            kind,
        }
    }

    #[inline]
    fn append(self, _: &I, _: ErrorKind) -> Self {
        self
    }
}

impl<I: Clone, C> AddContext<I, C> for InputError<I> {}

impl<I: Clone + Stream> FromRecoverableError<I, Self> for InputError<I> {
    #[inline]
    fn from_recoverable_error(
        _token_start: &<I as Stream>::Checkpoint,
        _err_start: &<I as Stream>::Checkpoint,
        _input: &I,
        e: Self,
    ) -> Self {
        e
    }
}

impl<I: Clone, E> FromExternalError<I, E> for InputError<I> {
    /// Create a new error from an input position and an external error
    #[inline]
    fn from_external_error(input: &I, kind: ErrorKind, _e: E) -> Self {
        Self {
            input: input.clone(),
            kind,
        }
    }
}

impl<I: Clone> ErrorConvert<InputError<(I, usize)>> for InputError<I> {
    #[inline]
    fn convert(self) -> InputError<(I, usize)> {
        InputError {
            input: (self.input, 0),
            kind: self.kind,
        }
    }
}

impl<I: Clone> ErrorConvert<InputError<I>> for InputError<(I, usize)> {
    #[inline]
    fn convert(self) -> InputError<I> {
        InputError {
            input: self.input.0,
            kind: self.kind,
        }
    }
}

/// The Display implementation allows the `std::error::Error` implementation
impl<I: Clone + fmt::Display> fmt::Display for InputError<I> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{} error starting at: {}",
            self.kind.description(),
            self.input
        )
    }
}

#[cfg(feature = "std")]
impl<I: Clone + fmt::Debug + fmt::Display + Sync + Send + 'static> std::error::Error
    for InputError<I>
{
}

impl<I> ParserError<I> for () {
    #[inline]
    fn from_error_kind(_: &I, _: ErrorKind) -> Self {}

    #[inline]
    fn append(self, _: &I, _: ErrorKind) -> Self {}
}

impl<I, C> AddContext<I, C> for () {}

impl<I: Stream> FromRecoverableError<I, Self> for () {
    #[inline]
    fn from_recoverable_error(
        _token_start: &<I as Stream>::Checkpoint,
        _err_start: &<I as Stream>::Checkpoint,
        _input: &I,
        (): Self,
    ) -> Self {
    }
}

impl<I, E> FromExternalError<I, E> for () {
    #[inline]
    fn from_external_error(_input: &I, _kind: ErrorKind, _e: E) -> Self {}
}

impl ErrorConvert<()> for () {
    #[inline]
    fn convert(self) {}
}

/// Accumulate context while backtracking errors
#[derive(Debug)]
pub struct ContextError<C = StrContext> {
    #[cfg(feature = "alloc")]
    context: crate::lib::std::vec::Vec<C>,
    #[cfg(not(feature = "alloc"))]
    context: core::marker::PhantomData<C>,
    #[cfg(feature = "std")]
    cause: Option<Box<dyn std::error::Error + Send + Sync + 'static>>,
}

impl<C> ContextError<C> {
    /// Create an empty error
    #[inline]
    pub fn new() -> Self {
        Self {
            context: Default::default(),
            #[cfg(feature = "std")]
            cause: None,
        }
    }

    /// Access context from [`Parser::context`]
    #[inline]
    #[cfg(feature = "alloc")]
    pub fn context(&self) -> impl Iterator<Item = &C> {
        self.context.iter()
    }

    /// Originating [`std::error::Error`]
    #[inline]
    #[cfg(feature = "std")]
    pub fn cause(&self) -> Option<&(dyn std::error::Error + Send + Sync + 'static)> {
        self.cause.as_deref()
    }
}

impl<C: Clone> Clone for ContextError<C> {
    fn clone(&self) -> Self {
        Self {
            context: self.context.clone(),
            #[cfg(feature = "std")]
            cause: self.cause.as_ref().map(|e| e.to_string().into()),
        }
    }
}

impl<C> Default for ContextError<C> {
    #[inline]
    fn default() -> Self {
        Self::new()
    }
}

impl<I, C> ParserError<I> for ContextError<C> {
    #[inline]
    fn from_error_kind(_input: &I, _kind: ErrorKind) -> Self {
        Self::new()
    }

    #[inline]
    fn append(self, _input: &I, _kind: ErrorKind) -> Self {
        self
    }

    #[inline]
    fn or(self, other: Self) -> Self {
        other
    }
}

impl<C, I> AddContext<I, C> for ContextError<C> {
    #[inline]
    fn add_context(mut self, _input: &I, ctx: C) -> Self {
        #[cfg(feature = "alloc")]
        self.context.push(ctx);
        self
    }
}

impl<I: Stream, C> FromRecoverableError<I, Self> for ContextError<C> {
    #[inline]
    fn from_recoverable_error(
        _token_start: &<I as Stream>::Checkpoint,
        _err_start: &<I as Stream>::Checkpoint,
        _input: &I,
        e: Self,
    ) -> Self {
        e
    }
}

#[cfg(feature = "std")]
impl<C, I, E: std::error::Error + Send + Sync + 'static> FromExternalError<I, E>
    for ContextError<C>
{
    #[inline]
    fn from_external_error(_input: &I, _kind: ErrorKind, e: E) -> Self {
        let mut err = Self::new();
        {
            err.cause = Some(Box::new(e));
        }
        err
    }
}

// HACK: This is more general than `std`, making the features non-additive
#[cfg(not(feature = "std"))]
impl<C, I, E: Send + Sync + 'static> FromExternalError<I, E> for ContextError<C> {
    #[inline]
    fn from_external_error(_input: &I, _kind: ErrorKind, _e: E) -> Self {
        let err = Self::new();
        err
    }
}

// For tests
impl<C: core::cmp::PartialEq> core::cmp::PartialEq for ContextError<C> {
    fn eq(&self, other: &Self) -> bool {
        #[cfg(feature = "alloc")]
        {
            if self.context != other.context {
                return false;
            }
        }
        #[cfg(feature = "std")]
        {
            if self.cause.as_ref().map(ToString::to_string)
                != other.cause.as_ref().map(ToString::to_string)
            {
                return false;
            }
        }

        true
    }
}

impl crate::lib::std::fmt::Display for ContextError<StrContext> {
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        #[cfg(feature = "alloc")]
        {
            let expression = self.context().find_map(|c| match c {
                StrContext::Label(c) => Some(c),
                _ => None,
            });
            let expected = self
                .context()
                .filter_map(|c| match c {
                    StrContext::Expected(c) => Some(c),
                    _ => None,
                })
                .collect::<crate::lib::std::vec::Vec<_>>();

            let mut newline = false;

            if let Some(expression) = expression {
                newline = true;

                write!(f, "invalid {}", expression)?;
            }

            if !expected.is_empty() {
                if newline {
                    writeln!(f)?;
                }
                newline = true;

                write!(f, "expected ")?;
                for (i, expected) in expected.iter().enumerate() {
                    if i != 0 {
                        write!(f, ", ")?;
                    }
                    write!(f, "{}", expected)?;
                }
            }
            #[cfg(feature = "std")]
            {
                if let Some(cause) = self.cause() {
                    if newline {
                        writeln!(f)?;
                    }
                    write!(f, "{}", cause)?;
                }
            }
        }

        Ok(())
    }
}

/// Additional parse context for [`ContextError`] added via [`Parser::context`]
#[derive(Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum StrContext {
    /// Description of what is currently being parsed
    Label(&'static str),
    /// Grammar item that was expected
    Expected(StrContextValue),
}

impl crate::lib::std::fmt::Display for StrContext {
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        match self {
            Self::Label(name) => write!(f, "invalid {name}"),
            Self::Expected(value) => write!(f, "expected {value}"),
        }
    }
}

/// See [`StrContext`]
#[derive(Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum StrContextValue {
    /// A [`char`] token
    CharLiteral(char),
    /// A [`&str`] token
    StringLiteral(&'static str),
    /// A description of what was being parsed
    Description(&'static str),
}

impl From<char> for StrContextValue {
    #[inline]
    fn from(inner: char) -> Self {
        Self::CharLiteral(inner)
    }
}

impl From<&'static str> for StrContextValue {
    #[inline]
    fn from(inner: &'static str) -> Self {
        Self::StringLiteral(inner)
    }
}

impl crate::lib::std::fmt::Display for StrContextValue {
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        match self {
            Self::CharLiteral('\n') => "newline".fmt(f),
            Self::CharLiteral('`') => "'`'".fmt(f),
            Self::CharLiteral(c) if c.is_ascii_control() => {
                write!(f, "`{}`", c.escape_debug())
            }
            Self::CharLiteral(c) => write!(f, "`{}`", c),
            Self::StringLiteral(c) => write!(f, "`{}`", c),
            Self::Description(c) => write!(f, "{}", c),
        }
    }
}

/// Trace all error paths, particularly for tests
#[derive(Debug)]
#[cfg(feature = "std")]
pub enum TreeError<I, C = StrContext> {
    /// Initial error that kicked things off
    Base(TreeErrorBase<I>),
    /// Traces added to the error while walking back up the stack
    Stack {
        /// Initial error that kicked things off
        base: Box<Self>,
        /// Traces added to the error while walking back up the stack
        stack: Vec<TreeErrorFrame<I, C>>,
    },
    /// All failed branches of an `alt`
    Alt(Vec<Self>),
}

/// See [`TreeError::Stack`]
#[derive(Debug)]
#[cfg(feature = "std")]
pub enum TreeErrorFrame<I, C = StrContext> {
    /// See [`ParserError::append`]
    Kind(TreeErrorBase<I>),
    /// See [`AddContext::add_context`]
    Context(TreeErrorContext<I, C>),
}

/// See [`TreeErrorFrame::Kind`], [`ParserError::append`]
#[derive(Debug)]
#[cfg(feature = "std")]
pub struct TreeErrorBase<I> {
    /// Parsed input, at the location where the error occurred
    pub input: I,
    /// Debug context
    pub kind: ErrorKind,
    /// See [`FromExternalError::from_external_error`]
    pub cause: Option<Box<dyn std::error::Error + Send + Sync + 'static>>,
}

/// See [`TreeErrorFrame::Context`], [`AddContext::add_context`]
#[derive(Debug)]
#[cfg(feature = "std")]
pub struct TreeErrorContext<I, C = StrContext> {
    /// Parsed input, at the location where the error occurred
    pub input: I,
    /// See [`AddContext::add_context`]
    pub context: C,
}

#[cfg(feature = "std")]
impl<'i, I: ToOwned, C> TreeError<&'i I, C>
where
    <I as ToOwned>::Owned: Clone,
{
    /// Obtaining ownership
    pub fn into_owned(self) -> TreeError<<I as ToOwned>::Owned, C> {
        self.map_input(ToOwned::to_owned)
    }
}

#[cfg(feature = "std")]
impl<I, C> TreeError<I, C>
where
    I: Clone,
{
    /// Translate the input type
    pub fn map_input<I2: Clone, O: Clone + Fn(I) -> I2>(self, op: O) -> TreeError<I2, C> {
        match self {
            TreeError::Base(base) => TreeError::Base(TreeErrorBase {
                input: op(base.input),
                kind: base.kind,
                cause: base.cause,
            }),
            TreeError::Stack { base, stack } => {
                let base = Box::new(base.map_input(op.clone()));
                let stack = stack
                    .into_iter()
                    .map(|frame| match frame {
                        TreeErrorFrame::Kind(kind) => TreeErrorFrame::Kind(TreeErrorBase {
                            input: op(kind.input),
                            kind: kind.kind,
                            cause: kind.cause,
                        }),
                        TreeErrorFrame::Context(context) => {
                            TreeErrorFrame::Context(TreeErrorContext {
                                input: op(context.input),
                                context: context.context,
                            })
                        }
                    })
                    .collect();
                TreeError::Stack { base, stack }
            }
            TreeError::Alt(alt) => {
                TreeError::Alt(alt.into_iter().map(|e| e.map_input(op.clone())).collect())
            }
        }
    }

    fn append_frame(self, frame: TreeErrorFrame<I, C>) -> Self {
        match self {
            TreeError::Stack { base, mut stack } => {
                stack.push(frame);
                TreeError::Stack { base, stack }
            }
            base => TreeError::Stack {
                base: Box::new(base),
                stack: vec![frame],
            },
        }
    }
}

#[cfg(feature = "std")]
impl<I, C> ParserError<I> for TreeError<I, C>
where
    I: Clone,
{
    fn from_error_kind(input: &I, kind: ErrorKind) -> Self {
        TreeError::Base(TreeErrorBase {
            input: input.clone(),
            kind,
            cause: None,
        })
    }

    fn append(self, input: &I, kind: ErrorKind) -> Self {
        let frame = TreeErrorFrame::Kind(TreeErrorBase {
            input: input.clone(),
            kind,
            cause: None,
        });
        self.append_frame(frame)
    }

    fn or(self, other: Self) -> Self {
        match (self, other) {
            (TreeError::Alt(mut first), TreeError::Alt(second)) => {
                // Just in case an implementation does a divide-and-conquer algorithm
                //
                // To prevent mixing `alt`s at different levels, parsers should
                // `alt_err.append(input, ErrorKind::Alt)`.
                first.extend(second);
                TreeError::Alt(first)
            }
            (TreeError::Alt(mut alt), new) | (new, TreeError::Alt(mut alt)) => {
                alt.push(new);
                TreeError::Alt(alt)
            }
            (first, second) => TreeError::Alt(vec![first, second]),
        }
    }
}

#[cfg(feature = "std")]
impl<I, C> AddContext<I, C> for TreeError<I, C>
where
    I: Clone,
{
    fn add_context(self, input: &I, context: C) -> Self {
        let frame = TreeErrorFrame::Context(TreeErrorContext {
            input: input.clone(),
            context,
        });
        self.append_frame(frame)
    }
}

#[cfg(feature = "std")]
impl<I: Clone + Stream, C> FromRecoverableError<I, Self> for TreeError<I, C> {
    #[inline]
    fn from_recoverable_error(
        _token_start: &<I as Stream>::Checkpoint,
        _err_start: &<I as Stream>::Checkpoint,
        _input: &I,
        e: Self,
    ) -> Self {
        e
    }
}

#[cfg(feature = "std")]
impl<I, C, E: std::error::Error + Send + Sync + 'static> FromExternalError<I, E> for TreeError<I, C>
where
    I: Clone,
{
    fn from_external_error(input: &I, kind: ErrorKind, e: E) -> Self {
        TreeError::Base(TreeErrorBase {
            input: input.clone(),
            kind,
            cause: Some(Box::new(e)),
        })
    }
}

#[cfg(feature = "std")]
impl<I, C> TreeError<I, C>
where
    I: Clone + crate::lib::std::fmt::Display,
    C: fmt::Display,
{
    fn write(&self, f: &mut fmt::Formatter<'_>, indent: usize) -> fmt::Result {
        let child_indent = indent + 2;
        match self {
            TreeError::Base(base) => {
                writeln!(f, "{:indent$}{base}", "")?;
            }
            TreeError::Stack { base, stack } => {
                base.write(f, indent)?;
                for (level, frame) in stack.iter().enumerate() {
                    match frame {
                        TreeErrorFrame::Kind(frame) => {
                            writeln!(f, "{:child_indent$}{level}: {frame}", "")?;
                        }
                        TreeErrorFrame::Context(frame) => {
                            writeln!(f, "{:child_indent$}{level}: {frame}", "")?;
                        }
                    }
                }
            }
            TreeError::Alt(alt) => {
                writeln!(f, "{:indent$}during one of:", "")?;
                for child in alt {
                    child.write(f, child_indent)?;
                }
            }
        }

        Ok(())
    }
}

#[cfg(feature = "std")]
impl<I: Clone + fmt::Display> fmt::Display for TreeErrorBase<I> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if let Some(cause) = self.cause.as_ref() {
            write!(f, "caused by {cause}")?;
        } else {
            let kind = self.kind.description();
            write!(f, "in {kind}")?;
        }
        let input = abbreviate(self.input.to_string());
        write!(f, " at '{input}'")?;
        Ok(())
    }
}

#[cfg(feature = "std")]
impl<I: Clone + fmt::Display, C: fmt::Display> fmt::Display for TreeErrorContext<I, C> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let context = &self.context;
        let input = abbreviate(self.input.to_string());
        write!(f, "{context} at '{input}'")?;
        Ok(())
    }
}

#[cfg(feature = "std")]
impl<
        I: Clone + fmt::Debug + fmt::Display + Sync + Send + 'static,
        C: fmt::Display + fmt::Debug,
    > std::error::Error for TreeError<I, C>
{
}

#[cfg(feature = "std")]
fn abbreviate(input: String) -> String {
    let mut abbrev = None;

    if let Some((line, _)) = input.split_once('\n') {
        abbrev = Some(line);
    }

    let max_len = 20;
    let current = abbrev.unwrap_or(&input);
    if max_len < current.len() {
        if let Some((index, _)) = current.char_indices().nth(max_len) {
            abbrev = Some(&current[..index]);
        }
    }

    if let Some(abbrev) = abbrev {
        format!("{abbrev}...")
    } else {
        input
    }
}

#[cfg(feature = "std")]
impl<I: Clone + fmt::Display, C: fmt::Display> fmt::Display for TreeError<I, C> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.write(f, 0)
    }
}

/// Deprecated, replaced with [`ContextError`]
#[cfg(feature = "std")]
#[allow(deprecated)]
#[deprecated(since = "0.5.8", note = "Replaced with `ContextError`")]
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct VerboseError<I: Clone, C = &'static str> {
    /// Accumulated error information
    pub errors: crate::lib::std::vec::Vec<(I, VerboseErrorKind<C>)>,
}

#[cfg(feature = "std")]
#[allow(deprecated)]
impl<'i, I: ToOwned, C> VerboseError<&'i I, C>
where
    <I as ToOwned>::Owned: Clone,
{
    /// Obtaining ownership
    pub fn into_owned(self) -> VerboseError<<I as ToOwned>::Owned, C> {
        self.map_input(ToOwned::to_owned)
    }
}

#[cfg(feature = "std")]
#[allow(deprecated)]
impl<I: Clone, C> VerboseError<I, C> {
    /// Translate the input type
    pub fn map_input<I2: Clone, O>(self, op: O) -> VerboseError<I2, C>
    where
        O: Fn(I) -> I2,
    {
        VerboseError {
            errors: self.errors.into_iter().map(|(i, k)| (op(i), k)).collect(),
        }
    }
}

/// Deprecated, replaced with [`ContextError`]
#[cfg(feature = "std")]
#[deprecated(since = "0.5.8", note = "Replaced with `ContextError`")]
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum VerboseErrorKind<C = &'static str> {
    /// Static string added by the `context` function
    Context(C),
    /// Error kind given by various parsers
    Winnow(ErrorKind),
}

#[cfg(feature = "std")]
#[allow(deprecated)]
impl<I: Clone, C> ParserError<I> for VerboseError<I, C> {
    fn from_error_kind(input: &I, kind: ErrorKind) -> Self {
        VerboseError {
            errors: vec![(input.clone(), VerboseErrorKind::Winnow(kind))],
        }
    }

    fn append(mut self, input: &I, kind: ErrorKind) -> Self {
        self.errors
            .push((input.clone(), VerboseErrorKind::Winnow(kind)));
        self
    }
}

#[cfg(feature = "std")]
#[allow(deprecated)]
impl<I: Clone, C> AddContext<I, C> for VerboseError<I, C> {
    fn add_context(mut self, input: &I, ctx: C) -> Self {
        self.errors
            .push((input.clone(), VerboseErrorKind::Context(ctx)));
        self
    }
}

#[cfg(feature = "std")]
#[allow(deprecated)]
impl<I: Clone, C, E> FromExternalError<I, E> for VerboseError<I, C> {
    /// Create a new error from an input position and an external error
    fn from_external_error(input: &I, kind: ErrorKind, _e: E) -> Self {
        Self::from_error_kind(input, kind)
    }
}

#[cfg(feature = "std")]
#[allow(deprecated)]
impl<I: Clone + fmt::Display, C: fmt::Display> fmt::Display for VerboseError<I, C> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        writeln!(f, "Parse error:")?;
        for (input, error) in &self.errors {
            match error {
                VerboseErrorKind::Winnow(e) => writeln!(f, "{} at: {}", e.description(), input)?,
                VerboseErrorKind::Context(s) => writeln!(f, "in section '{}', at: {}", s, input)?,
            }
        }

        Ok(())
    }
}

#[cfg(feature = "std")]
#[allow(deprecated)]
impl<
        I: Clone + fmt::Debug + fmt::Display + Sync + Send + 'static,
        C: fmt::Display + fmt::Debug,
    > std::error::Error for VerboseError<I, C>
{
}

/// Provide some minor debug context for errors
#[rustfmt::skip]
#[derive(Debug,PartialEq,Eq,Hash,Clone,Copy)]
#[allow(missing_docs)]
pub enum ErrorKind {
  Assert,
  Token,
  Tag,
  Alt,
  Many,
  Eof,
  Slice,
  Complete,
  Not,
  Verify,
  Fail,
}

impl ErrorKind {
    #[rustfmt::skip]
    /// Converts an `ErrorKind` to a text description
    pub fn description(&self) -> &str {
    match *self {
      ErrorKind::Assert                    => "assert",
      ErrorKind::Token                     => "token",
      ErrorKind::Tag                       => "tag",
      ErrorKind::Alt                       => "alternative",
      ErrorKind::Many                      => "many",
      ErrorKind::Eof                       => "end of file",
      ErrorKind::Slice                     => "slice",
      ErrorKind::Complete                  => "complete",
      ErrorKind::Not                       => "negation",
      ErrorKind::Verify                    => "predicate verification",
      ErrorKind::Fail                      => "fail",
    }
  }
}

impl<I> ParserError<I> for ErrorKind {
    #[inline]
    fn from_error_kind(_input: &I, kind: ErrorKind) -> Self {
        kind
    }

    #[inline]
    fn append(self, _: &I, _: ErrorKind) -> Self {
        self
    }
}

impl<I, C> AddContext<I, C> for ErrorKind {}

impl<I, E> FromExternalError<I, E> for ErrorKind {
    /// Create a new error from an input position and an external error
    #[inline]
    fn from_external_error(_input: &I, kind: ErrorKind, _e: E) -> Self {
        kind
    }
}

/// The Display implementation allows the `std::error::Error` implementation
impl fmt::Display for ErrorKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "error {:?}", self)
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ErrorKind {}

/// See [`Parser::parse`]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ParseError<I, E> {
    input: I,
    offset: usize,
    inner: E,
}

impl<I: Stream, E: ParserError<I>> ParseError<I, E> {
    pub(crate) fn new(mut input: I, start: I::Checkpoint, inner: E) -> Self {
        let offset = input.offset_from(&start);
        input.reset(start);
        Self {
            input,
            offset,
            inner,
        }
    }
}

impl<I, E> ParseError<I, E> {
    /// The [`Stream`] at the initial location when parsing started
    #[inline]
    pub fn input(&self) -> &I {
        &self.input
    }

    /// The location in [`ParseError::input`] where parsing failed
    ///
    /// **Note:** This is an offset, not an index, and may point to the end of input
    /// (`input.len()`) on eof errors.
    #[inline]
    pub fn offset(&self) -> usize {
        self.offset
    }

    /// The original [`ParserError`]
    #[inline]
    pub fn inner(&self) -> &E {
        &self.inner
    }

    /// The original [`ParserError`]
    #[inline]
    pub fn into_inner(self) -> E {
        self.inner
    }
}

impl<I, E> core::fmt::Display for ParseError<I, E>
where
    I: AsBStr,
    E: core::fmt::Display,
{
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let input = self.input.as_bstr();
        let span_start = self.offset;
        let span_end = span_start;
        #[cfg(feature = "std")]
        if input.contains(&b'\n') {
            let (line_idx, col_idx) = translate_position(input, span_start);
            let line_num = line_idx + 1;
            let col_num = col_idx + 1;
            let gutter = line_num.to_string().len();
            let content = input
                .split(|c| *c == b'\n')
                .nth(line_idx)
                .expect("valid line number");

            writeln!(f, "parse error at line {}, column {}", line_num, col_num)?;
            //   |
            for _ in 0..gutter {
                write!(f, " ")?;
            }
            writeln!(f, " |")?;

            // 1 | 00:32:00.a999999
            write!(f, "{} | ", line_num)?;
            writeln!(f, "{}", String::from_utf8_lossy(content))?;

            //   |          ^
            for _ in 0..gutter {
                write!(f, " ")?;
            }
            write!(f, " | ")?;
            for _ in 0..col_idx {
                write!(f, " ")?;
            }
            // The span will be empty at eof, so we need to make sure we always print at least
            // one `^`
            write!(f, "^")?;
            for _ in (span_start + 1)..(span_end.min(span_start + content.len())) {
                write!(f, "^")?;
            }
            writeln!(f)?;
        } else {
            let content = input;
            writeln!(f, "{}", String::from_utf8_lossy(content))?;
            for _ in 0..span_start {
                write!(f, " ")?;
            }
            // The span will be empty at eof, so we need to make sure we always print at least
            // one `^`
            write!(f, "^")?;
            for _ in (span_start + 1)..(span_end.min(span_start + content.len())) {
                write!(f, "^")?;
            }
            writeln!(f)?;
        }
        write!(f, "{}", self.inner)?;

        Ok(())
    }
}

#[cfg(feature = "std")]
fn translate_position(input: &[u8], index: usize) -> (usize, usize) {
    if input.is_empty() {
        return (0, index);
    }

    let safe_index = index.min(input.len() - 1);
    let column_offset = index - safe_index;
    let index = safe_index;

    let nl = input[0..index]
        .iter()
        .rev()
        .enumerate()
        .find(|(_, b)| **b == b'\n')
        .map(|(nl, _)| index - nl - 1);
    let line_start = match nl {
        Some(nl) => nl + 1,
        None => 0,
    };
    let line = input[0..line_start].iter().filter(|b| **b == b'\n').count();

    // HACK: This treats byte offset and column offsets the same
    let column = crate::lib::std::str::from_utf8(&input[line_start..=index])
        .map(|s| s.chars().count() - 1)
        .unwrap_or_else(|_| index - line_start);
    let column = column + column_offset;

    (line, column)
}

#[cfg(test)]
#[cfg(feature = "std")]
mod test_parse_error {
    use super::*;

    #[test]
    fn single_line() {
        let mut input = "0xZ123";
        let start = input.checkpoint();
        let _ = input.next_token().unwrap();
        let _ = input.next_token().unwrap();
        let inner = InputError::new(input, ErrorKind::Slice);
        let error = ParseError::new(input, start, inner);
        let expected = "\
0xZ123
  ^
slice error starting at: Z123";
        assert_eq!(error.to_string(), expected);
    }
}

#[cfg(test)]
#[cfg(feature = "std")]
mod test_translate_position {
    use super::*;

    #[test]
    fn empty() {
        let input = b"";
        let index = 0;
        let position = translate_position(&input[..], index);
        assert_eq!(position, (0, 0));
    }

    #[test]
    fn start() {
        let input = b"Hello";
        let index = 0;
        let position = translate_position(&input[..], index);
        assert_eq!(position, (0, 0));
    }

    #[test]
    fn end() {
        let input = b"Hello";
        let index = input.len() - 1;
        let position = translate_position(&input[..], index);
        assert_eq!(position, (0, input.len() - 1));
    }

    #[test]
    fn after() {
        let input = b"Hello";
        let index = input.len();
        let position = translate_position(&input[..], index);
        assert_eq!(position, (0, input.len()));
    }

    #[test]
    fn first_line() {
        let input = b"Hello\nWorld\n";
        let index = 2;
        let position = translate_position(&input[..], index);
        assert_eq!(position, (0, 2));
    }

    #[test]
    fn end_of_line() {
        let input = b"Hello\nWorld\n";
        let index = 5;
        let position = translate_position(&input[..], index);
        assert_eq!(position, (0, 5));
    }

    #[test]
    fn start_of_second_line() {
        let input = b"Hello\nWorld\n";
        let index = 6;
        let position = translate_position(&input[..], index);
        assert_eq!(position, (1, 0));
    }

    #[test]
    fn second_line() {
        let input = b"Hello\nWorld\n";
        let index = 8;
        let position = translate_position(&input[..], index);
        assert_eq!(position, (1, 2));
    }
}

/// Creates a parse error from a [`ErrorKind`]
/// and the position in the input
#[cfg(test)]
macro_rules! error_position(
  ($input:expr, $code:expr) => ({
    $crate::error::ParserError::from_error_kind($input, $code)
  });
);

#[cfg(test)]
macro_rules! error_node_position(
  ($input:expr, $code:expr, $next:expr) => ({
    $crate::error::ParserError::append($next, $input, $code)
  });
);
