//! Stream capability for combinators to parse
//!
//! Stream types include:
//! - `&[u8]` and [`Bytes`] for binary data
//! - `&str` (aliased as [`Str`]) and [`BStr`] for UTF-8 data
//! - [`Located`] can track the location within the original buffer to report
//!   [spans][crate::Parser::with_span]
//! - [`Stateful`] to thread global state through your parsers
//! - [`Partial`] can mark an input as partial buffer that is being streamed into
//! - [Custom stream types][crate::_topic::stream]

use core::hash::BuildHasher;
use core::num::NonZeroUsize;

use crate::ascii::Caseless as AsciiCaseless;
#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
use crate::error::FromRecoverableError;
use crate::error::Needed;
use crate::lib::std::iter::{Cloned, Enumerate};
use crate::lib::std::slice::Iter;
use crate::lib::std::str::from_utf8;
use crate::lib::std::str::CharIndices;
use crate::lib::std::str::FromStr;

#[allow(unused_imports)]
#[cfg(any(feature = "unstable-doc", feature = "unstable-recover"))]
use crate::error::ErrMode;

#[cfg(feature = "alloc")]
use crate::lib::std::collections::BTreeMap;
#[cfg(feature = "alloc")]
use crate::lib::std::collections::BTreeSet;
#[cfg(feature = "std")]
use crate::lib::std::collections::HashMap;
#[cfg(feature = "std")]
use crate::lib::std::collections::HashSet;
#[cfg(feature = "alloc")]
use crate::lib::std::string::String;
#[cfg(feature = "alloc")]
use crate::lib::std::vec::Vec;

mod impls;
#[cfg(test)]
mod tests;

/// UTF-8 Stream
pub type Str<'i> = &'i str;

/// Improved `Debug` experience for `&[u8]` byte streams
#[allow(clippy::derived_hash_with_manual_eq)]
#[derive(Hash)]
#[repr(transparent)]
pub struct Bytes([u8]);

impl Bytes {
    /// Make a stream out of a byte slice-like.
    #[inline]
    pub fn new<B: ?Sized + AsRef<[u8]>>(bytes: &B) -> &Self {
        Self::from_bytes(bytes.as_ref())
    }

    #[inline]
    fn from_bytes(slice: &[u8]) -> &Self {
        unsafe { crate::lib::std::mem::transmute(slice) }
    }

    #[inline]
    fn as_bytes(&self) -> &[u8] {
        &self.0
    }
}

/// Improved `Debug` experience for `&[u8]` UTF-8-ish streams
#[allow(clippy::derived_hash_with_manual_eq)]
#[derive(Hash)]
#[repr(transparent)]
pub struct BStr([u8]);

impl BStr {
    /// Make a stream out of a byte slice-like.
    #[inline]
    pub fn new<B: ?Sized + AsRef<[u8]>>(bytes: &B) -> &Self {
        Self::from_bytes(bytes.as_ref())
    }

    #[inline]
    fn from_bytes(slice: &[u8]) -> &Self {
        unsafe { crate::lib::std::mem::transmute(slice) }
    }

    #[inline]
    fn as_bytes(&self) -> &[u8] {
        &self.0
    }
}

/// Allow collecting the span of a parsed token
///
/// Spans are tracked as a [`Range<usize>`] of byte offsets.
///
/// Converting byte offsets to line or column numbers is left up to the user, as computing column
/// numbers requires domain knowledge (are columns byte-based, codepoint-based, or grapheme-based?)
/// and O(n) iteration over the input to determine codepoint and line boundaries.
///
/// [The `line-span` crate](https://docs.rs/line-span/latest/line_span/) can help with converting
/// byte offsets to line numbers.
///
/// See [`Parser::span`][crate::Parser::span] and [`Parser::with_span`][crate::Parser::with_span] for more details
#[derive(Copy, Clone, Default, PartialEq, Eq, PartialOrd, Ord)]
#[doc(alias = "LocatedSpan")]
pub struct Located<I> {
    initial: I,
    input: I,
}

impl<I> Located<I>
where
    I: Clone + Offset,
{
    /// Wrap another Stream with span tracking
    pub fn new(input: I) -> Self {
        let initial = input.clone();
        Self { initial, input }
    }

    fn location(&self) -> usize {
        self.input.offset_from(&self.initial)
    }
}

impl<I> Located<I>
where
    I: Clone + Stream + Offset,
{
    /// Reset the stream to the start
    ///
    /// This is useful for formats that encode a graph with addresses relative to the start of the
    /// input.
    #[doc(alias = "fseek")]
    pub fn reset_to_start(&mut self) {
        let start = self.initial.checkpoint();
        self.input.reset(&start);
    }
}

impl<I> AsRef<I> for Located<I> {
    #[inline(always)]
    fn as_ref(&self) -> &I {
        &self.input
    }
}

impl<I> crate::lib::std::ops::Deref for Located<I> {
    type Target = I;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        &self.input
    }
}

impl<I: crate::lib::std::fmt::Display> crate::lib::std::fmt::Display for Located<I> {
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        self.input.fmt(f)
    }
}

impl<I: crate::lib::std::fmt::Debug> crate::lib::std::fmt::Debug for Located<I> {
    #[inline]
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        self.input.fmt(f)
    }
}

/// Allow recovering from parse errors, capturing them as the parser continues
///
/// Generally, this will be used indirectly via
/// [`RecoverableParser::recoverable_parse`][crate::RecoverableParser::recoverable_parse].
#[cfg(feature = "unstable-recover")]
#[derive(Clone)]
#[cfg(feature = "std")]
pub struct Recoverable<I, E>
where
    I: Stream,
{
    input: I,
    errors: Vec<E>,
    is_recoverable: bool,
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> Default for Recoverable<I, E>
where
    I: Default + Stream,
{
    fn default() -> Self {
        Self::new(I::default())
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> Recoverable<I, E>
where
    I: Stream,
{
    /// Track recoverable errors with the stream
    pub fn new(input: I) -> Self {
        Self {
            input,
            errors: Default::default(),
            is_recoverable: true,
        }
    }

    /// Act as a normal stream
    pub fn unrecoverable(input: I) -> Self {
        Self {
            input,
            errors: Default::default(),
            is_recoverable: false,
        }
    }

    /// Access the current input and errors
    pub fn into_parts(self) -> (I, Vec<E>) {
        (self.input, self.errors)
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> AsRef<I> for Recoverable<I, E>
where
    I: Stream,
{
    #[inline(always)]
    fn as_ref(&self) -> &I {
        &self.input
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> crate::lib::std::ops::Deref for Recoverable<I, E>
where
    I: Stream,
{
    type Target = I;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        &self.input
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I: crate::lib::std::fmt::Display, E> crate::lib::std::fmt::Display for Recoverable<I, E>
where
    I: Stream,
{
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        crate::lib::std::fmt::Display::fmt(&self.input, f)
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I: Stream + crate::lib::std::fmt::Debug, E: crate::lib::std::fmt::Debug>
    crate::lib::std::fmt::Debug for Recoverable<I, E>
{
    #[inline]
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        if f.alternate() {
            self.input.fmt(f)
        } else {
            f.debug_struct("Recoverable")
                .field("input", &self.input)
                .field("errors", &self.errors)
                .field("is_recoverable", &self.is_recoverable)
                .finish()
        }
    }
}

/// Thread global state through your parsers
///
/// Use cases
/// - Recursion checks
/// - Error recovery
/// - Debugging
///
/// # Example
///
/// ```
/// # use std::cell::Cell;
/// # use winnow::prelude::*;
/// # use winnow::stream::Stateful;
/// # use winnow::ascii::alpha1;
/// # type Error = ();
///
/// #[derive(Debug)]
/// struct State<'s>(&'s mut u32);
///
/// impl<'s> State<'s> {
///     fn count(&mut self) {
///         *self.0 += 1;
///     }
/// }
///
/// type Stream<'is> = Stateful<&'is str, State<'is>>;
///
/// fn word<'s>(i: &mut Stream<'s>) -> PResult<&'s str> {
///   i.state.count();
///   alpha1.parse_next(i)
/// }
///
/// let data = "Hello";
/// let mut state = 0;
/// let input = Stream { input: data, state: State(&mut state) };
/// let output = word.parse(input).unwrap();
/// assert_eq!(state, 1);
/// ```
#[derive(Clone, Copy, Default, Eq, PartialEq)]
#[doc(alias = "LocatedSpan")]
pub struct Stateful<I, S> {
    /// Inner input being wrapped in state
    pub input: I,
    /// User-provided state
    pub state: S,
}

impl<I, S> AsRef<I> for Stateful<I, S> {
    #[inline(always)]
    fn as_ref(&self) -> &I {
        &self.input
    }
}

impl<I, S> crate::lib::std::ops::Deref for Stateful<I, S> {
    type Target = I;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl<I: crate::lib::std::fmt::Display, S> crate::lib::std::fmt::Display for Stateful<I, S> {
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        self.input.fmt(f)
    }
}

impl<I: crate::lib::std::fmt::Debug, S: crate::lib::std::fmt::Debug> crate::lib::std::fmt::Debug
    for Stateful<I, S>
{
    #[inline]
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        if f.alternate() {
            self.input.fmt(f)
        } else {
            f.debug_struct("Stateful")
                .field("input", &self.input)
                .field("state", &self.state)
                .finish()
        }
    }
}

/// Mark the input as a partial buffer for streaming input.
///
/// Complete input means that we already have all of the data. This will be the common case with
/// small files that can be read entirely to memory.
///
/// In contrast, streaming input assumes that we might not have all of the data.
/// This can happen with some network protocol or large file parsers, where the
/// input buffer can be full and need to be resized or refilled.
/// - [`ErrMode::Incomplete`] will report how much more data is needed.
/// - [`Parser::complete_err`][crate::Parser::complete_err] transform [`ErrMode::Incomplete`] to
///   [`ErrMode::Backtrack`]
///
/// See also [`StreamIsPartial`] to tell whether the input supports complete or partial parsing.
///
/// See also [Special Topics: Parsing Partial Input][crate::_topic::partial].
///
/// # Example
///
/// Here is how it works in practice:
///
/// ```rust
/// # use winnow::{PResult, error::ErrMode, error::Needed, error::{InputError, ErrorKind}, token, ascii, stream::Partial};
/// # use winnow::prelude::*;
///
/// fn take_partial<'s>(i: &mut Partial<&'s [u8]>) -> PResult<&'s [u8], InputError<Partial<&'s [u8]>>> {
///   token::take(4u8).parse_next(i)
/// }
///
/// fn take_complete<'s>(i: &mut &'s [u8]) -> PResult<&'s [u8], InputError<&'s [u8]>> {
///   token::take(4u8).parse_next(i)
/// }
///
/// // both parsers will take 4 bytes as expected
/// assert_eq!(take_partial.parse_peek(Partial::new(&b"abcde"[..])), Ok((Partial::new(&b"e"[..]), &b"abcd"[..])));
/// assert_eq!(take_complete.parse_peek(&b"abcde"[..]), Ok((&b"e"[..], &b"abcd"[..])));
///
/// // if the input is smaller than 4 bytes, the partial parser
/// // will return `Incomplete` to indicate that we need more data
/// assert_eq!(take_partial.parse_peek(Partial::new(&b"abc"[..])), Err(ErrMode::Incomplete(Needed::new(1))));
///
/// // but the complete parser will return an error
/// assert_eq!(take_complete.parse_peek(&b"abc"[..]), Err(ErrMode::Backtrack(InputError::new(&b"abc"[..], ErrorKind::Slice))));
///
/// // the alpha0 function takes 0 or more alphabetic characters
/// fn alpha0_partial<'s>(i: &mut Partial<&'s str>) -> PResult<&'s str, InputError<Partial<&'s str>>> {
///   ascii::alpha0.parse_next(i)
/// }
///
/// fn alpha0_complete<'s>(i: &mut &'s str) -> PResult<&'s str, InputError<&'s str>> {
///   ascii::alpha0.parse_next(i)
/// }
///
/// // if there's a clear limit to the taken characters, both parsers work the same way
/// assert_eq!(alpha0_partial.parse_peek(Partial::new("abcd;")), Ok((Partial::new(";"), "abcd")));
/// assert_eq!(alpha0_complete.parse_peek("abcd;"), Ok((";", "abcd")));
///
/// // but when there's no limit, the partial version returns `Incomplete`, because it cannot
/// // know if more input data should be taken. The whole input could be "abcd;", or
/// // "abcde;"
/// assert_eq!(alpha0_partial.parse_peek(Partial::new("abcd")), Err(ErrMode::Incomplete(Needed::new(1))));
///
/// // while the complete version knows that all of the data is there
/// assert_eq!(alpha0_complete.parse_peek("abcd"), Ok(("", "abcd")));
/// ```
#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct Partial<I> {
    input: I,
    partial: bool,
}

impl<I> Partial<I>
where
    I: StreamIsPartial,
{
    /// Create a partial input
    pub fn new(input: I) -> Self {
        debug_assert!(
            !I::is_partial_supported(),
            "`Partial` can only wrap complete sources"
        );
        let partial = true;
        Self { input, partial }
    }

    /// Extract the original [`Stream`]
    #[inline(always)]
    pub fn into_inner(self) -> I {
        self.input
    }
}

impl<I> Default for Partial<I>
where
    I: Default + StreamIsPartial,
{
    fn default() -> Self {
        Self::new(I::default())
    }
}

impl<I> crate::lib::std::ops::Deref for Partial<I> {
    type Target = I;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        &self.input
    }
}

impl<I: crate::lib::std::fmt::Display> crate::lib::std::fmt::Display for Partial<I> {
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        self.input.fmt(f)
    }
}

impl<I: crate::lib::std::fmt::Debug> crate::lib::std::fmt::Debug for Partial<I> {
    #[inline]
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        if f.alternate() {
            self.input.fmt(f)
        } else {
            f.debug_struct("Partial")
                .field("input", &self.input)
                .field("partial", &self.partial)
                .finish()
        }
    }
}

/// Abstract method to calculate the input length
pub trait SliceLen {
    /// Calculates the input length, as indicated by its name,
    /// and the name of the trait itself
    fn slice_len(&self) -> usize;
}

impl<S: SliceLen> SliceLen for AsciiCaseless<S> {
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.0.slice_len()
    }
}

impl<'a, T> SliceLen for &'a [T] {
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.len()
    }
}

impl<T, const LEN: usize> SliceLen for [T; LEN] {
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.len()
    }
}

impl<'a, T, const LEN: usize> SliceLen for &'a [T; LEN] {
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.len()
    }
}

impl<'a> SliceLen for &'a str {
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.len()
    }
}

impl SliceLen for u8 {
    #[inline(always)]
    fn slice_len(&self) -> usize {
        1
    }
}

impl SliceLen for char {
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.len_utf8()
    }
}

impl<'a> SliceLen for &'a Bytes {
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.len()
    }
}

impl<'a> SliceLen for &'a BStr {
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.len()
    }
}

impl<I> SliceLen for (I, usize, usize)
where
    I: SliceLen,
{
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.0.slice_len() * 8 + self.2 - self.1
    }
}

impl<I> SliceLen for Located<I>
where
    I: SliceLen,
{
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.input.slice_len()
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> SliceLen for Recoverable<I, E>
where
    I: SliceLen,
    I: Stream,
{
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.input.slice_len()
    }
}

impl<I, S> SliceLen for Stateful<I, S>
where
    I: SliceLen,
{
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.input.slice_len()
    }
}

impl<I> SliceLen for Partial<I>
where
    I: SliceLen,
{
    #[inline(always)]
    fn slice_len(&self) -> usize {
        self.input.slice_len()
    }
}

/// Core definition for parser input state
pub trait Stream: Offset<<Self as Stream>::Checkpoint> + crate::lib::std::fmt::Debug {
    /// The smallest unit being parsed
    ///
    /// Example: `u8` for `&[u8]` or `char` for `&str`
    type Token: crate::lib::std::fmt::Debug;
    /// Sequence of `Token`s
    ///
    /// Example: `&[u8]` for `Located<&[u8]>` or `&str` for `Located<&str>`
    type Slice: crate::lib::std::fmt::Debug;

    /// Iterate with the offset from the current location
    type IterOffsets: Iterator<Item = (usize, Self::Token)>;

    /// A parse location within the stream
    type Checkpoint: Offset + Clone + crate::lib::std::fmt::Debug;

    /// Iterate with the offset from the current location
    fn iter_offsets(&self) -> Self::IterOffsets;

    /// Returns the offset to the end of the input
    fn eof_offset(&self) -> usize;

    /// Split off the next token from the input
    fn next_token(&mut self) -> Option<Self::Token>;
    /// Split off the next token from the input
    #[inline(always)]
    fn peek_token(&self) -> Option<(Self, Self::Token)>
    where
        Self: Clone,
    {
        let mut peek = self.clone();
        let token = peek.next_token()?;
        Some((peek, token))
    }

    /// Finds the offset of the next matching token
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool;
    /// Get the offset for the number of `tokens` into the stream
    ///
    /// This means "0 tokens" will return `0` offset
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed>;
    /// Split off a slice of tokens from the input
    ///
    /// **NOTE:** For inputs with variable width tokens, like `&str`'s `char`, `offset` might not correspond
    /// with the number of tokens. To get a valid offset, use:
    /// - [`Stream::eof_offset`]
    /// - [`Stream::iter_offsets`]
    /// - [`Stream::offset_for`]
    /// - [`Stream::offset_at`]
    ///
    /// # Panic
    ///
    /// This will panic if
    ///
    /// * Indexes must be within bounds of the original input;
    /// * Indexes must uphold invariants of the stream, like for `str` they must lie on UTF-8
    ///   sequence boundaries.
    ///
    fn next_slice(&mut self, offset: usize) -> Self::Slice;
    /// Split off a slice of tokens from the input
    #[inline(always)]
    fn peek_slice(&self, offset: usize) -> (Self, Self::Slice)
    where
        Self: Clone,
    {
        let mut peek = self.clone();
        let slice = peek.next_slice(offset);
        (peek, slice)
    }

    /// Advance to the end of the stream
    #[inline(always)]
    fn finish(&mut self) -> Self::Slice {
        self.next_slice(self.eof_offset())
    }
    /// Advance to the end of the stream
    #[inline(always)]
    fn peek_finish(&self) -> (Self, Self::Slice)
    where
        Self: Clone,
    {
        let mut peek = self.clone();
        let slice = peek.finish();
        (peek, slice)
    }

    /// Save the current parse location within the stream
    fn checkpoint(&self) -> Self::Checkpoint;
    /// Revert the stream to a prior [`Self::Checkpoint`]
    ///
    /// # Panic
    ///
    /// May panic if an invalid [`Self::Checkpoint`] is provided
    fn reset(&mut self, checkpoint: &Self::Checkpoint);

    /// Return the inner-most stream
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug;
}

impl<'i, T> Stream for &'i [T]
where
    T: Clone + crate::lib::std::fmt::Debug,
{
    type Token = T;
    type Slice = &'i [T];

    type IterOffsets = Enumerate<Cloned<Iter<'i, T>>>;

    type Checkpoint = Checkpoint<Self, Self>;

    #[inline(always)]
    fn iter_offsets(&self) -> Self::IterOffsets {
        self.iter().cloned().enumerate()
    }
    #[inline(always)]
    fn eof_offset(&self) -> usize {
        self.len()
    }

    #[inline(always)]
    fn next_token(&mut self) -> Option<Self::Token> {
        let (token, next) = self.split_first()?;
        *self = next;
        Some(token.clone())
    }

    #[inline(always)]
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool,
    {
        self.iter().position(|b| predicate(b.clone()))
    }
    #[inline(always)]
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed> {
        if let Some(needed) = tokens.checked_sub(self.len()).and_then(NonZeroUsize::new) {
            Err(Needed::Size(needed))
        } else {
            Ok(tokens)
        }
    }
    #[inline(always)]
    fn next_slice(&mut self, offset: usize) -> Self::Slice {
        let (slice, next) = self.split_at(offset);
        *self = next;
        slice
    }

    #[inline(always)]
    fn checkpoint(&self) -> Self::Checkpoint {
        Checkpoint::<_, Self>::new(*self)
    }
    #[inline(always)]
    fn reset(&mut self, checkpoint: &Self::Checkpoint) {
        *self = checkpoint.inner;
    }

    #[inline(always)]
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug {
        self
    }
}

impl<'i> Stream for &'i str {
    type Token = char;
    type Slice = &'i str;

    type IterOffsets = CharIndices<'i>;

    type Checkpoint = Checkpoint<Self, Self>;

    #[inline(always)]
    fn iter_offsets(&self) -> Self::IterOffsets {
        self.char_indices()
    }
    #[inline(always)]
    fn eof_offset(&self) -> usize {
        self.len()
    }

    #[inline(always)]
    fn next_token(&mut self) -> Option<Self::Token> {
        let c = self.chars().next()?;
        let offset = c.len();
        *self = &self[offset..];
        Some(c)
    }

    #[inline(always)]
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool,
    {
        for (o, c) in self.iter_offsets() {
            if predicate(c) {
                return Some(o);
            }
        }
        None
    }
    #[inline]
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed> {
        let mut cnt = 0;
        for (offset, _) in self.iter_offsets() {
            if cnt == tokens {
                return Ok(offset);
            }
            cnt += 1;
        }

        if cnt == tokens {
            Ok(self.eof_offset())
        } else {
            Err(Needed::Unknown)
        }
    }
    #[inline(always)]
    fn next_slice(&mut self, offset: usize) -> Self::Slice {
        let (slice, next) = self.split_at(offset);
        *self = next;
        slice
    }

    #[inline(always)]
    fn checkpoint(&self) -> Self::Checkpoint {
        Checkpoint::<_, Self>::new(*self)
    }
    #[inline(always)]
    fn reset(&mut self, checkpoint: &Self::Checkpoint) {
        *self = checkpoint.inner;
    }

    #[inline(always)]
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug {
        self
    }
}

impl<'i> Stream for &'i Bytes {
    type Token = u8;
    type Slice = &'i [u8];

    type IterOffsets = Enumerate<Cloned<Iter<'i, u8>>>;

    type Checkpoint = Checkpoint<Self, Self>;

    #[inline(always)]
    fn iter_offsets(&self) -> Self::IterOffsets {
        self.iter().cloned().enumerate()
    }
    #[inline(always)]
    fn eof_offset(&self) -> usize {
        self.len()
    }

    #[inline(always)]
    fn next_token(&mut self) -> Option<Self::Token> {
        if self.is_empty() {
            None
        } else {
            let token = self[0];
            *self = &self[1..];
            Some(token)
        }
    }

    #[inline(always)]
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool,
    {
        self.iter().position(|b| predicate(*b))
    }
    #[inline(always)]
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed> {
        if let Some(needed) = tokens.checked_sub(self.len()).and_then(NonZeroUsize::new) {
            Err(Needed::Size(needed))
        } else {
            Ok(tokens)
        }
    }
    #[inline(always)]
    fn next_slice(&mut self, offset: usize) -> Self::Slice {
        let (slice, next) = self.0.split_at(offset);
        *self = Bytes::from_bytes(next);
        slice
    }

    #[inline(always)]
    fn checkpoint(&self) -> Self::Checkpoint {
        Checkpoint::<_, Self>::new(*self)
    }
    #[inline(always)]
    fn reset(&mut self, checkpoint: &Self::Checkpoint) {
        *self = checkpoint.inner;
    }

    #[inline(always)]
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug {
        self
    }
}

impl<'i> Stream for &'i BStr {
    type Token = u8;
    type Slice = &'i [u8];

    type IterOffsets = Enumerate<Cloned<Iter<'i, u8>>>;

    type Checkpoint = Checkpoint<Self, Self>;

    #[inline(always)]
    fn iter_offsets(&self) -> Self::IterOffsets {
        self.iter().cloned().enumerate()
    }
    #[inline(always)]
    fn eof_offset(&self) -> usize {
        self.len()
    }

    #[inline(always)]
    fn next_token(&mut self) -> Option<Self::Token> {
        if self.is_empty() {
            None
        } else {
            let token = self[0];
            *self = &self[1..];
            Some(token)
        }
    }

    #[inline(always)]
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool,
    {
        self.iter().position(|b| predicate(*b))
    }
    #[inline(always)]
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed> {
        if let Some(needed) = tokens.checked_sub(self.len()).and_then(NonZeroUsize::new) {
            Err(Needed::Size(needed))
        } else {
            Ok(tokens)
        }
    }
    #[inline(always)]
    fn next_slice(&mut self, offset: usize) -> Self::Slice {
        let (slice, next) = self.0.split_at(offset);
        *self = BStr::from_bytes(next);
        slice
    }

    #[inline(always)]
    fn checkpoint(&self) -> Self::Checkpoint {
        Checkpoint::<_, Self>::new(*self)
    }
    #[inline(always)]
    fn reset(&mut self, checkpoint: &Self::Checkpoint) {
        *self = checkpoint.inner;
    }

    #[inline(always)]
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug {
        self
    }
}

impl<I> Stream for (I, usize)
where
    I: Stream<Token = u8> + Clone,
{
    type Token = bool;
    type Slice = (I::Slice, usize, usize);

    type IterOffsets = BitOffsets<I>;

    type Checkpoint = Checkpoint<(I::Checkpoint, usize), Self>;

    #[inline(always)]
    fn iter_offsets(&self) -> Self::IterOffsets {
        BitOffsets {
            i: self.clone(),
            o: 0,
        }
    }
    #[inline(always)]
    fn eof_offset(&self) -> usize {
        let offset = self.0.eof_offset() * 8;
        if offset == 0 {
            0
        } else {
            offset - self.1
        }
    }

    #[inline(always)]
    fn next_token(&mut self) -> Option<Self::Token> {
        next_bit(self)
    }

    #[inline(always)]
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool,
    {
        self.iter_offsets()
            .find_map(|(o, b)| predicate(b).then_some(o))
    }
    #[inline(always)]
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed> {
        if let Some(needed) = tokens
            .checked_sub(self.eof_offset())
            .and_then(NonZeroUsize::new)
        {
            Err(Needed::Size(needed))
        } else {
            Ok(tokens)
        }
    }
    #[inline(always)]
    fn next_slice(&mut self, offset: usize) -> Self::Slice {
        let byte_offset = (offset + self.1) / 8;
        let end_offset = (offset + self.1) % 8;
        let s = self.0.next_slice(byte_offset);
        let start_offset = self.1;
        self.1 = end_offset;
        (s, start_offset, end_offset)
    }

    #[inline(always)]
    fn checkpoint(&self) -> Self::Checkpoint {
        Checkpoint::<_, Self>::new((self.0.checkpoint(), self.1))
    }
    #[inline(always)]
    fn reset(&mut self, checkpoint: &Self::Checkpoint) {
        self.0.reset(&checkpoint.inner.0);
        self.1 = checkpoint.inner.1;
    }

    #[inline(always)]
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug {
        &self.0
    }
}

/// Iterator for [bit][crate::binary::bits] stream (`(I, usize)`)
pub struct BitOffsets<I> {
    i: (I, usize),
    o: usize,
}

impl<I> Iterator for BitOffsets<I>
where
    I: Stream<Token = u8> + Clone,
{
    type Item = (usize, bool);
    fn next(&mut self) -> Option<Self::Item> {
        let b = next_bit(&mut self.i)?;
        let o = self.o;

        self.o += 1;

        Some((o, b))
    }
}

fn next_bit<I>(i: &mut (I, usize)) -> Option<bool>
where
    I: Stream<Token = u8> + Clone,
{
    if i.eof_offset() == 0 {
        return None;
    }
    let offset = i.1;

    let mut next_i = i.0.clone();
    let byte = next_i.next_token()?;
    let bit = (byte >> offset) & 0x1 == 0x1;

    let next_offset = offset + 1;
    if next_offset == 8 {
        i.0 = next_i;
        i.1 = 0;
        Some(bit)
    } else {
        i.1 = next_offset;
        Some(bit)
    }
}

impl<I: Stream> Stream for Located<I> {
    type Token = <I as Stream>::Token;
    type Slice = <I as Stream>::Slice;

    type IterOffsets = <I as Stream>::IterOffsets;

    type Checkpoint = Checkpoint<I::Checkpoint, Self>;

    #[inline(always)]
    fn iter_offsets(&self) -> Self::IterOffsets {
        self.input.iter_offsets()
    }
    #[inline(always)]
    fn eof_offset(&self) -> usize {
        self.input.eof_offset()
    }

    #[inline(always)]
    fn next_token(&mut self) -> Option<Self::Token> {
        self.input.next_token()
    }

    #[inline(always)]
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool,
    {
        self.input.offset_for(predicate)
    }
    #[inline(always)]
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed> {
        self.input.offset_at(tokens)
    }
    #[inline(always)]
    fn next_slice(&mut self, offset: usize) -> Self::Slice {
        self.input.next_slice(offset)
    }

    #[inline(always)]
    fn checkpoint(&self) -> Self::Checkpoint {
        Checkpoint::<_, Self>::new(self.input.checkpoint())
    }
    #[inline(always)]
    fn reset(&mut self, checkpoint: &Self::Checkpoint) {
        self.input.reset(&checkpoint.inner);
    }

    #[inline(always)]
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug {
        &self.input
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E: crate::lib::std::fmt::Debug> Stream for Recoverable<I, E>
where
    I: Stream,
{
    type Token = <I as Stream>::Token;
    type Slice = <I as Stream>::Slice;

    type IterOffsets = <I as Stream>::IterOffsets;

    type Checkpoint = Checkpoint<I::Checkpoint, Self>;

    #[inline(always)]
    fn iter_offsets(&self) -> Self::IterOffsets {
        self.input.iter_offsets()
    }
    #[inline(always)]
    fn eof_offset(&self) -> usize {
        self.input.eof_offset()
    }

    #[inline(always)]
    fn next_token(&mut self) -> Option<Self::Token> {
        self.input.next_token()
    }

    #[inline(always)]
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool,
    {
        self.input.offset_for(predicate)
    }
    #[inline(always)]
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed> {
        self.input.offset_at(tokens)
    }
    #[inline(always)]
    fn next_slice(&mut self, offset: usize) -> Self::Slice {
        self.input.next_slice(offset)
    }

    #[inline(always)]
    fn checkpoint(&self) -> Self::Checkpoint {
        Checkpoint::<_, Self>::new(self.input.checkpoint())
    }
    #[inline(always)]
    fn reset(&mut self, checkpoint: &Self::Checkpoint) {
        self.input.reset(&checkpoint.inner);
    }

    #[inline(always)]
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug {
        &self.input
    }
}

impl<I: Stream, S: crate::lib::std::fmt::Debug> Stream for Stateful<I, S> {
    type Token = <I as Stream>::Token;
    type Slice = <I as Stream>::Slice;

    type IterOffsets = <I as Stream>::IterOffsets;

    type Checkpoint = Checkpoint<I::Checkpoint, Self>;

    #[inline(always)]
    fn iter_offsets(&self) -> Self::IterOffsets {
        self.input.iter_offsets()
    }
    #[inline(always)]
    fn eof_offset(&self) -> usize {
        self.input.eof_offset()
    }

    #[inline(always)]
    fn next_token(&mut self) -> Option<Self::Token> {
        self.input.next_token()
    }

    #[inline(always)]
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool,
    {
        self.input.offset_for(predicate)
    }
    #[inline(always)]
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed> {
        self.input.offset_at(tokens)
    }
    #[inline(always)]
    fn next_slice(&mut self, offset: usize) -> Self::Slice {
        self.input.next_slice(offset)
    }

    #[inline(always)]
    fn checkpoint(&self) -> Self::Checkpoint {
        Checkpoint::<_, Self>::new(self.input.checkpoint())
    }
    #[inline(always)]
    fn reset(&mut self, checkpoint: &Self::Checkpoint) {
        self.input.reset(&checkpoint.inner);
    }

    #[inline(always)]
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug {
        &self.input
    }
}

impl<I: Stream> Stream for Partial<I> {
    type Token = <I as Stream>::Token;
    type Slice = <I as Stream>::Slice;

    type IterOffsets = <I as Stream>::IterOffsets;

    type Checkpoint = Checkpoint<I::Checkpoint, Self>;

    #[inline(always)]
    fn iter_offsets(&self) -> Self::IterOffsets {
        self.input.iter_offsets()
    }
    #[inline(always)]
    fn eof_offset(&self) -> usize {
        self.input.eof_offset()
    }

    #[inline(always)]
    fn next_token(&mut self) -> Option<Self::Token> {
        self.input.next_token()
    }

    #[inline(always)]
    fn offset_for<P>(&self, predicate: P) -> Option<usize>
    where
        P: Fn(Self::Token) -> bool,
    {
        self.input.offset_for(predicate)
    }
    #[inline(always)]
    fn offset_at(&self, tokens: usize) -> Result<usize, Needed> {
        self.input.offset_at(tokens)
    }
    #[inline(always)]
    fn next_slice(&mut self, offset: usize) -> Self::Slice {
        self.input.next_slice(offset)
    }

    #[inline(always)]
    fn checkpoint(&self) -> Self::Checkpoint {
        Checkpoint::<_, Self>::new(self.input.checkpoint())
    }
    #[inline(always)]
    fn reset(&mut self, checkpoint: &Self::Checkpoint) {
        self.input.reset(&checkpoint.inner);
    }

    #[inline(always)]
    fn raw(&self) -> &dyn crate::lib::std::fmt::Debug {
        &self.input
    }
}

/// Number of indices input has advanced since start of parsing
///
/// See [`Located`] for adding location tracking to your [`Stream`]
pub trait Location {
    /// Number of indices input has advanced since start of parsing
    fn location(&self) -> usize;
}

impl<I> Location for Located<I>
where
    I: Clone + Offset,
{
    #[inline(always)]
    fn location(&self) -> usize {
        self.location()
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> Location for Recoverable<I, E>
where
    I: Location,
    I: Stream,
{
    #[inline(always)]
    fn location(&self) -> usize {
        self.input.location()
    }
}

impl<I, S> Location for Stateful<I, S>
where
    I: Location,
{
    #[inline(always)]
    fn location(&self) -> usize {
        self.input.location()
    }
}

impl<I> Location for Partial<I>
where
    I: Location,
{
    #[inline(always)]
    fn location(&self) -> usize {
        self.input.location()
    }
}

/// Capture top-level errors in the middle of parsing so parsing can resume
///
/// See [`Recoverable`] for adding error recovery tracking to your [`Stream`]
#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
pub trait Recover<E>: Stream {
    /// Capture a top-level error
    ///
    /// May return `Err(err)` if recovery is not possible (e.g. if [`Recover::is_recovery_supported`]
    /// returns `false`).
    fn record_err(
        &mut self,
        token_start: &Self::Checkpoint,
        err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>>;

    /// Report whether the [`Stream`] can save off errors for recovery
    fn is_recovery_supported() -> bool;
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<'a, T, E> Recover<E> for &'a [T]
where
    &'a [T]: Stream,
{
    #[inline(always)]
    fn record_err(
        &mut self,
        _token_start: &Self::Checkpoint,
        _err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>> {
        Err(err)
    }

    /// Report whether the [`Stream`] can save off errors for recovery
    #[inline(always)]
    fn is_recovery_supported() -> bool {
        false
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<'a, E> Recover<E> for &'a str {
    #[inline(always)]
    fn record_err(
        &mut self,
        _token_start: &Self::Checkpoint,
        _err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>> {
        Err(err)
    }

    /// Report whether the [`Stream`] can save off errors for recovery
    #[inline(always)]
    fn is_recovery_supported() -> bool {
        false
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<'a, E> Recover<E> for &'a Bytes {
    #[inline(always)]
    fn record_err(
        &mut self,
        _token_start: &Self::Checkpoint,
        _err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>> {
        Err(err)
    }

    /// Report whether the [`Stream`] can save off errors for recovery
    #[inline(always)]
    fn is_recovery_supported() -> bool {
        false
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<'a, E> Recover<E> for &'a BStr {
    #[inline(always)]
    fn record_err(
        &mut self,
        _token_start: &Self::Checkpoint,
        _err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>> {
        Err(err)
    }

    /// Report whether the [`Stream`] can save off errors for recovery
    #[inline(always)]
    fn is_recovery_supported() -> bool {
        false
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> Recover<E> for (I, usize)
where
    I: Recover<E>,
    I: Stream<Token = u8> + Clone,
{
    #[inline(always)]
    fn record_err(
        &mut self,
        _token_start: &Self::Checkpoint,
        _err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>> {
        Err(err)
    }

    /// Report whether the [`Stream`] can save off errors for recovery
    #[inline(always)]
    fn is_recovery_supported() -> bool {
        false
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> Recover<E> for Located<I>
where
    I: Recover<E>,
    I: Stream,
{
    #[inline(always)]
    fn record_err(
        &mut self,
        _token_start: &Self::Checkpoint,
        _err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>> {
        Err(err)
    }

    /// Report whether the [`Stream`] can save off errors for recovery
    #[inline(always)]
    fn is_recovery_supported() -> bool {
        false
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E, R> Recover<E> for Recoverable<I, R>
where
    I: Stream,
    R: FromRecoverableError<Self, E>,
    R: crate::lib::std::fmt::Debug,
{
    fn record_err(
        &mut self,
        token_start: &Self::Checkpoint,
        err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>> {
        if self.is_recoverable {
            match err {
                ErrMode::Incomplete(need) => Err(ErrMode::Incomplete(need)),
                ErrMode::Backtrack(err) | ErrMode::Cut(err) => {
                    self.errors
                        .push(R::from_recoverable_error(token_start, err_start, self, err));
                    Ok(())
                }
            }
        } else {
            Err(err)
        }
    }

    /// Report whether the [`Stream`] can save off errors for recovery
    #[inline(always)]
    fn is_recovery_supported() -> bool {
        true
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E, S> Recover<E> for Stateful<I, S>
where
    I: Recover<E>,
    I: Stream,
    S: Clone + crate::lib::std::fmt::Debug,
{
    #[inline(always)]
    fn record_err(
        &mut self,
        _token_start: &Self::Checkpoint,
        _err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>> {
        Err(err)
    }

    /// Report whether the [`Stream`] can save off errors for recovery
    #[inline(always)]
    fn is_recovery_supported() -> bool {
        false
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> Recover<E> for Partial<I>
where
    I: Recover<E>,
    I: Stream,
{
    #[inline(always)]
    fn record_err(
        &mut self,
        _token_start: &Self::Checkpoint,
        _err_start: &Self::Checkpoint,
        err: ErrMode<E>,
    ) -> Result<(), ErrMode<E>> {
        Err(err)
    }

    /// Report whether the [`Stream`] can save off errors for recovery
    #[inline(always)]
    fn is_recovery_supported() -> bool {
        false
    }
}

/// Marks the input as being the complete buffer or a partial buffer for streaming input
///
/// See [`Partial`] for marking a presumed complete buffer type as a streaming buffer.
pub trait StreamIsPartial: Sized {
    /// Whether the stream is currently partial or complete
    type PartialState;

    /// Mark the stream is complete
    #[must_use]
    fn complete(&mut self) -> Self::PartialState;

    /// Restore the stream back to its previous state
    fn restore_partial(&mut self, state: Self::PartialState);

    /// Report whether the [`Stream`] is can ever be incomplete
    fn is_partial_supported() -> bool;

    /// Report whether the [`Stream`] is currently incomplete
    #[inline(always)]
    fn is_partial(&self) -> bool {
        Self::is_partial_supported()
    }
}

impl<'a, T> StreamIsPartial for &'a [T] {
    type PartialState = ();

    fn complete(&mut self) -> Self::PartialState {}

    fn restore_partial(&mut self, _state: Self::PartialState) {}

    #[inline(always)]
    fn is_partial_supported() -> bool {
        false
    }
}

impl<'a> StreamIsPartial for &'a str {
    type PartialState = ();

    fn complete(&mut self) -> Self::PartialState {
        // Already complete
    }

    fn restore_partial(&mut self, _state: Self::PartialState) {}

    #[inline(always)]
    fn is_partial_supported() -> bool {
        false
    }
}

impl<'a> StreamIsPartial for &'a Bytes {
    type PartialState = ();

    fn complete(&mut self) -> Self::PartialState {
        // Already complete
    }

    fn restore_partial(&mut self, _state: Self::PartialState) {}

    #[inline(always)]
    fn is_partial_supported() -> bool {
        false
    }
}

impl<'a> StreamIsPartial for &'a BStr {
    type PartialState = ();

    fn complete(&mut self) -> Self::PartialState {
        // Already complete
    }

    fn restore_partial(&mut self, _state: Self::PartialState) {}

    #[inline(always)]
    fn is_partial_supported() -> bool {
        false
    }
}

impl<I> StreamIsPartial for (I, usize)
where
    I: StreamIsPartial,
{
    type PartialState = I::PartialState;

    fn complete(&mut self) -> Self::PartialState {
        self.0.complete()
    }

    fn restore_partial(&mut self, state: Self::PartialState) {
        self.0.restore_partial(state);
    }

    #[inline(always)]
    fn is_partial_supported() -> bool {
        I::is_partial_supported()
    }

    #[inline(always)]
    fn is_partial(&self) -> bool {
        self.0.is_partial()
    }
}

impl<I> StreamIsPartial for Located<I>
where
    I: StreamIsPartial,
{
    type PartialState = I::PartialState;

    fn complete(&mut self) -> Self::PartialState {
        self.input.complete()
    }

    fn restore_partial(&mut self, state: Self::PartialState) {
        self.input.restore_partial(state);
    }

    #[inline(always)]
    fn is_partial_supported() -> bool {
        I::is_partial_supported()
    }

    #[inline(always)]
    fn is_partial(&self) -> bool {
        self.input.is_partial()
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> StreamIsPartial for Recoverable<I, E>
where
    I: StreamIsPartial,
    I: Stream,
{
    type PartialState = I::PartialState;

    fn complete(&mut self) -> Self::PartialState {
        self.input.complete()
    }

    fn restore_partial(&mut self, state: Self::PartialState) {
        self.input.restore_partial(state);
    }

    #[inline(always)]
    fn is_partial_supported() -> bool {
        I::is_partial_supported()
    }

    #[inline(always)]
    fn is_partial(&self) -> bool {
        self.input.is_partial()
    }
}

impl<I, S> StreamIsPartial for Stateful<I, S>
where
    I: StreamIsPartial,
{
    type PartialState = I::PartialState;

    fn complete(&mut self) -> Self::PartialState {
        self.input.complete()
    }

    fn restore_partial(&mut self, state: Self::PartialState) {
        self.input.restore_partial(state);
    }

    #[inline(always)]
    fn is_partial_supported() -> bool {
        I::is_partial_supported()
    }

    #[inline(always)]
    fn is_partial(&self) -> bool {
        self.input.is_partial()
    }
}

impl<I> StreamIsPartial for Partial<I>
where
    I: StreamIsPartial,
{
    type PartialState = bool;

    fn complete(&mut self) -> Self::PartialState {
        core::mem::replace(&mut self.partial, false)
    }

    fn restore_partial(&mut self, state: Self::PartialState) {
        self.partial = state;
    }

    #[inline(always)]
    fn is_partial_supported() -> bool {
        true
    }

    #[inline(always)]
    fn is_partial(&self) -> bool {
        self.partial
    }
}

/// Useful functions to calculate the offset between slices and show a hexdump of a slice
pub trait Offset<Start = Self> {
    /// Offset between the first byte of `start` and the first byte of `self`a
    ///
    /// **Note:** This is an offset, not an index, and may point to the end of input
    /// (`start.len()`) when `self` is exhausted.
    fn offset_from(&self, start: &Start) -> usize;
}

impl<'a, T> Offset for &'a [T] {
    #[inline]
    fn offset_from(&self, start: &Self) -> usize {
        let fst = (*start).as_ptr();
        let snd = (*self).as_ptr();

        debug_assert!(
            fst <= snd,
            "`Offset::offset_from({snd:?}, {fst:?})` only accepts slices of `self`"
        );
        (snd as usize - fst as usize) / crate::lib::std::mem::size_of::<T>()
    }
}

impl<'a, T> Offset<<&'a [T] as Stream>::Checkpoint> for &'a [T]
where
    T: Clone + crate::lib::std::fmt::Debug,
{
    #[inline(always)]
    fn offset_from(&self, other: &<&'a [T] as Stream>::Checkpoint) -> usize {
        self.checkpoint().offset_from(other)
    }
}

impl<'a> Offset for &'a str {
    #[inline(always)]
    fn offset_from(&self, start: &Self) -> usize {
        self.as_bytes().offset_from(&start.as_bytes())
    }
}

impl<'a> Offset<<&'a str as Stream>::Checkpoint> for &'a str {
    #[inline(always)]
    fn offset_from(&self, other: &<&'a str as Stream>::Checkpoint) -> usize {
        self.checkpoint().offset_from(other)
    }
}

impl<'a> Offset for &'a Bytes {
    #[inline(always)]
    fn offset_from(&self, start: &Self) -> usize {
        self.as_bytes().offset_from(&start.as_bytes())
    }
}

impl<'a> Offset<<&'a Bytes as Stream>::Checkpoint> for &'a Bytes {
    #[inline(always)]
    fn offset_from(&self, other: &<&'a Bytes as Stream>::Checkpoint) -> usize {
        self.checkpoint().offset_from(other)
    }
}

impl<'a> Offset for &'a BStr {
    #[inline(always)]
    fn offset_from(&self, start: &Self) -> usize {
        self.as_bytes().offset_from(&start.as_bytes())
    }
}

impl<'a> Offset<<&'a BStr as Stream>::Checkpoint> for &'a BStr {
    #[inline(always)]
    fn offset_from(&self, other: &<&'a BStr as Stream>::Checkpoint) -> usize {
        self.checkpoint().offset_from(other)
    }
}

impl<I> Offset for (I, usize)
where
    I: Offset,
{
    #[inline(always)]
    fn offset_from(&self, start: &Self) -> usize {
        self.0.offset_from(&start.0) * 8 + self.1 - start.1
    }
}

impl<I> Offset<<(I, usize) as Stream>::Checkpoint> for (I, usize)
where
    I: Stream<Token = u8> + Clone,
{
    #[inline(always)]
    fn offset_from(&self, other: &<(I, usize) as Stream>::Checkpoint) -> usize {
        self.checkpoint().offset_from(other)
    }
}

impl<I> Offset for Located<I>
where
    I: Stream,
{
    #[inline(always)]
    fn offset_from(&self, other: &Self) -> usize {
        self.offset_from(&other.checkpoint())
    }
}

impl<I> Offset<<Located<I> as Stream>::Checkpoint> for Located<I>
where
    I: Stream,
{
    #[inline(always)]
    fn offset_from(&self, other: &<Located<I> as Stream>::Checkpoint) -> usize {
        self.checkpoint().offset_from(other)
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> Offset for Recoverable<I, E>
where
    I: Stream,
    E: crate::lib::std::fmt::Debug,
{
    #[inline(always)]
    fn offset_from(&self, other: &Self) -> usize {
        self.offset_from(&other.checkpoint())
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> Offset<<Recoverable<I, E> as Stream>::Checkpoint> for Recoverable<I, E>
where
    I: Stream,
    E: crate::lib::std::fmt::Debug,
{
    #[inline(always)]
    fn offset_from(&self, other: &<Recoverable<I, E> as Stream>::Checkpoint) -> usize {
        self.checkpoint().offset_from(other)
    }
}

impl<I, S> Offset for Stateful<I, S>
where
    I: Stream,
    S: Clone + crate::lib::std::fmt::Debug,
{
    #[inline(always)]
    fn offset_from(&self, start: &Self) -> usize {
        self.offset_from(&start.checkpoint())
    }
}

impl<I, S> Offset<<Stateful<I, S> as Stream>::Checkpoint> for Stateful<I, S>
where
    I: Stream,
    S: crate::lib::std::fmt::Debug,
{
    #[inline(always)]
    fn offset_from(&self, other: &<Stateful<I, S> as Stream>::Checkpoint) -> usize {
        self.checkpoint().offset_from(other)
    }
}

impl<I> Offset for Partial<I>
where
    I: Stream,
{
    #[inline(always)]
    fn offset_from(&self, start: &Self) -> usize {
        self.offset_from(&start.checkpoint())
    }
}

impl<I> Offset<<Partial<I> as Stream>::Checkpoint> for Partial<I>
where
    I: Stream,
{
    #[inline(always)]
    fn offset_from(&self, other: &<Partial<I> as Stream>::Checkpoint) -> usize {
        self.checkpoint().offset_from(other)
    }
}

impl<I, S> Offset for Checkpoint<I, S>
where
    I: Offset,
{
    #[inline(always)]
    fn offset_from(&self, start: &Self) -> usize {
        self.inner.offset_from(&start.inner)
    }
}

/// Helper trait for types that can be viewed as a byte slice
pub trait AsBytes {
    /// Casts the input type to a byte slice
    fn as_bytes(&self) -> &[u8];
}

impl<'a> AsBytes for &'a [u8] {
    #[inline(always)]
    fn as_bytes(&self) -> &[u8] {
        self
    }
}

impl<'a> AsBytes for &'a Bytes {
    #[inline(always)]
    fn as_bytes(&self) -> &[u8] {
        (*self).as_bytes()
    }
}

impl<I> AsBytes for Located<I>
where
    I: AsBytes,
{
    #[inline(always)]
    fn as_bytes(&self) -> &[u8] {
        self.input.as_bytes()
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> AsBytes for Recoverable<I, E>
where
    I: Stream,
    I: AsBytes,
{
    #[inline(always)]
    fn as_bytes(&self) -> &[u8] {
        self.input.as_bytes()
    }
}

impl<I, S> AsBytes for Stateful<I, S>
where
    I: AsBytes,
{
    #[inline(always)]
    fn as_bytes(&self) -> &[u8] {
        self.input.as_bytes()
    }
}

impl<I> AsBytes for Partial<I>
where
    I: AsBytes,
{
    #[inline(always)]
    fn as_bytes(&self) -> &[u8] {
        self.input.as_bytes()
    }
}

/// Helper trait for types that can be viewed as a byte slice
pub trait AsBStr {
    /// Casts the input type to a byte slice
    fn as_bstr(&self) -> &[u8];
}

impl<'a> AsBStr for &'a [u8] {
    #[inline(always)]
    fn as_bstr(&self) -> &[u8] {
        self
    }
}

impl<'a> AsBStr for &'a BStr {
    #[inline(always)]
    fn as_bstr(&self) -> &[u8] {
        (*self).as_bytes()
    }
}

impl<'a> AsBStr for &'a str {
    #[inline(always)]
    fn as_bstr(&self) -> &[u8] {
        (*self).as_bytes()
    }
}

impl<I> AsBStr for Located<I>
where
    I: AsBStr,
{
    #[inline(always)]
    fn as_bstr(&self) -> &[u8] {
        self.input.as_bstr()
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> AsBStr for Recoverable<I, E>
where
    I: Stream,
    I: AsBStr,
{
    #[inline(always)]
    fn as_bstr(&self) -> &[u8] {
        self.input.as_bstr()
    }
}

impl<I, S> AsBStr for Stateful<I, S>
where
    I: AsBStr,
{
    #[inline(always)]
    fn as_bstr(&self) -> &[u8] {
        self.input.as_bstr()
    }
}

impl<I> AsBStr for Partial<I>
where
    I: AsBStr,
{
    #[inline(always)]
    fn as_bstr(&self) -> &[u8] {
        self.input.as_bstr()
    }
}

/// Result of [`Compare::compare`]
#[derive(Debug, Eq, PartialEq)]
pub enum CompareResult {
    /// Comparison was successful
    ///
    /// `usize` is the end of the successful match within the buffer.
    /// This is most relevant for caseless UTF-8 where `Compare::compare`'s parameter might be a different
    /// length than the match within the buffer.
    Ok(usize),
    /// We need more data to be sure
    Incomplete,
    /// Comparison failed
    Error,
}

/// Abstracts comparison operations
pub trait Compare<T> {
    /// Compares self to another value for equality
    fn compare(&self, t: T) -> CompareResult;
}

impl<'a, 'b> Compare<&'b [u8]> for &'a [u8] {
    #[inline]
    fn compare(&self, t: &'b [u8]) -> CompareResult {
        if t.iter().zip(*self).any(|(a, b)| a != b) {
            CompareResult::Error
        } else if self.len() < t.slice_len() {
            CompareResult::Incomplete
        } else {
            CompareResult::Ok(t.slice_len())
        }
    }
}

impl<'a, 'b> Compare<AsciiCaseless<&'b [u8]>> for &'a [u8] {
    #[inline]
    fn compare(&self, t: AsciiCaseless<&'b [u8]>) -> CompareResult {
        if t.0
            .iter()
            .zip(*self)
            .any(|(a, b)| !a.eq_ignore_ascii_case(b))
        {
            CompareResult::Error
        } else if self.len() < t.slice_len() {
            CompareResult::Incomplete
        } else {
            CompareResult::Ok(t.slice_len())
        }
    }
}

impl<'a, const LEN: usize> Compare<[u8; LEN]> for &'a [u8] {
    #[inline(always)]
    fn compare(&self, t: [u8; LEN]) -> CompareResult {
        self.compare(&t[..])
    }
}

impl<'a, const LEN: usize> Compare<AsciiCaseless<[u8; LEN]>> for &'a [u8] {
    #[inline(always)]
    fn compare(&self, t: AsciiCaseless<[u8; LEN]>) -> CompareResult {
        self.compare(AsciiCaseless(&t.0[..]))
    }
}

impl<'a, 'b, const LEN: usize> Compare<&'b [u8; LEN]> for &'a [u8] {
    #[inline(always)]
    fn compare(&self, t: &'b [u8; LEN]) -> CompareResult {
        self.compare(&t[..])
    }
}

impl<'a, 'b, const LEN: usize> Compare<AsciiCaseless<&'b [u8; LEN]>> for &'a [u8] {
    #[inline(always)]
    fn compare(&self, t: AsciiCaseless<&'b [u8; LEN]>) -> CompareResult {
        self.compare(AsciiCaseless(&t.0[..]))
    }
}

impl<'a, 'b> Compare<&'b str> for &'a [u8] {
    #[inline(always)]
    fn compare(&self, t: &'b str) -> CompareResult {
        self.compare(t.as_bytes())
    }
}

impl<'a, 'b> Compare<AsciiCaseless<&'b str>> for &'a [u8] {
    #[inline(always)]
    fn compare(&self, t: AsciiCaseless<&'b str>) -> CompareResult {
        self.compare(AsciiCaseless(t.0.as_bytes()))
    }
}

impl<'a> Compare<u8> for &'a [u8] {
    #[inline]
    fn compare(&self, t: u8) -> CompareResult {
        match self.first().copied() {
            Some(c) if t == c => CompareResult::Ok(t.slice_len()),
            Some(_) => CompareResult::Error,
            None => CompareResult::Incomplete,
        }
    }
}

impl<'a> Compare<AsciiCaseless<u8>> for &'a [u8] {
    #[inline]
    fn compare(&self, t: AsciiCaseless<u8>) -> CompareResult {
        match self.first() {
            Some(c) if t.0.eq_ignore_ascii_case(c) => CompareResult::Ok(t.slice_len()),
            Some(_) => CompareResult::Error,
            None => CompareResult::Incomplete,
        }
    }
}

impl<'a> Compare<char> for &'a [u8] {
    #[inline(always)]
    fn compare(&self, t: char) -> CompareResult {
        self.compare(t.encode_utf8(&mut [0; 4]).as_bytes())
    }
}

impl<'a> Compare<AsciiCaseless<char>> for &'a [u8] {
    #[inline(always)]
    fn compare(&self, t: AsciiCaseless<char>) -> CompareResult {
        self.compare(AsciiCaseless(t.0.encode_utf8(&mut [0; 4]).as_bytes()))
    }
}

impl<'a, 'b> Compare<&'b str> for &'a str {
    #[inline(always)]
    fn compare(&self, t: &'b str) -> CompareResult {
        self.as_bytes().compare(t.as_bytes())
    }
}

impl<'a, 'b> Compare<AsciiCaseless<&'b str>> for &'a str {
    #[inline(always)]
    fn compare(&self, t: AsciiCaseless<&'b str>) -> CompareResult {
        self.as_bytes().compare(t.as_bytes())
    }
}

impl<'a> Compare<char> for &'a str {
    #[inline(always)]
    fn compare(&self, t: char) -> CompareResult {
        self.as_bytes().compare(t)
    }
}

impl<'a> Compare<AsciiCaseless<char>> for &'a str {
    #[inline(always)]
    fn compare(&self, t: AsciiCaseless<char>) -> CompareResult {
        self.as_bytes().compare(t)
    }
}

impl<'a, T> Compare<T> for &'a Bytes
where
    &'a [u8]: Compare<T>,
{
    #[inline(always)]
    fn compare(&self, t: T) -> CompareResult {
        let bytes = (*self).as_bytes();
        bytes.compare(t)
    }
}

impl<'a, T> Compare<T> for &'a BStr
where
    &'a [u8]: Compare<T>,
{
    #[inline(always)]
    fn compare(&self, t: T) -> CompareResult {
        let bytes = (*self).as_bytes();
        bytes.compare(t)
    }
}

impl<I, U> Compare<U> for Located<I>
where
    I: Compare<U>,
{
    #[inline(always)]
    fn compare(&self, other: U) -> CompareResult {
        self.input.compare(other)
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E, U> Compare<U> for Recoverable<I, E>
where
    I: Stream,
    I: Compare<U>,
{
    #[inline(always)]
    fn compare(&self, other: U) -> CompareResult {
        self.input.compare(other)
    }
}

impl<I, S, U> Compare<U> for Stateful<I, S>
where
    I: Compare<U>,
{
    #[inline(always)]
    fn compare(&self, other: U) -> CompareResult {
        self.input.compare(other)
    }
}

impl<I, T> Compare<T> for Partial<I>
where
    I: Compare<T>,
{
    #[inline(always)]
    fn compare(&self, t: T) -> CompareResult {
        self.input.compare(t)
    }
}

/// Look for a slice in self
pub trait FindSlice<T> {
    /// Returns the offset of the slice if it is found
    fn find_slice(&self, substr: T) -> Option<crate::lib::std::ops::Range<usize>>;
}

impl<'i, 's> FindSlice<&'s [u8]> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: &'s [u8]) -> Option<crate::lib::std::ops::Range<usize>> {
        memmem(self, substr)
    }
}

impl<'i, 's> FindSlice<(&'s [u8],)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: (&'s [u8],)) -> Option<crate::lib::std::ops::Range<usize>> {
        memmem(self, substr.0)
    }
}

impl<'i, 's> FindSlice<(&'s [u8], &'s [u8])> for &'i [u8] {
    #[inline(always)]
    fn find_slice(
        &self,
        substr: (&'s [u8], &'s [u8]),
    ) -> Option<crate::lib::std::ops::Range<usize>> {
        memmem2(self, substr)
    }
}

impl<'i, 's> FindSlice<(&'s [u8], &'s [u8], &'s [u8])> for &'i [u8] {
    #[inline(always)]
    fn find_slice(
        &self,
        substr: (&'s [u8], &'s [u8], &'s [u8]),
    ) -> Option<crate::lib::std::ops::Range<usize>> {
        memmem3(self, substr)
    }
}

impl<'i> FindSlice<char> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: char) -> Option<crate::lib::std::ops::Range<usize>> {
        let mut b = [0; 4];
        let substr = substr.encode_utf8(&mut b);
        self.find_slice(&*substr)
    }
}

impl<'i> FindSlice<(char,)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: (char,)) -> Option<crate::lib::std::ops::Range<usize>> {
        let mut b = [0; 4];
        let substr0 = substr.0.encode_utf8(&mut b);
        self.find_slice((&*substr0,))
    }
}

impl<'i> FindSlice<(char, char)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: (char, char)) -> Option<crate::lib::std::ops::Range<usize>> {
        let mut b = [0; 4];
        let substr0 = substr.0.encode_utf8(&mut b);
        let mut b = [0; 4];
        let substr1 = substr.1.encode_utf8(&mut b);
        self.find_slice((&*substr0, &*substr1))
    }
}

impl<'i> FindSlice<(char, char, char)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: (char, char, char)) -> Option<crate::lib::std::ops::Range<usize>> {
        let mut b = [0; 4];
        let substr0 = substr.0.encode_utf8(&mut b);
        let mut b = [0; 4];
        let substr1 = substr.1.encode_utf8(&mut b);
        let mut b = [0; 4];
        let substr2 = substr.2.encode_utf8(&mut b);
        self.find_slice((&*substr0, &*substr1, &*substr2))
    }
}

impl<'i> FindSlice<u8> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: u8) -> Option<crate::lib::std::ops::Range<usize>> {
        memchr(substr, self).map(|i| i..i + 1)
    }
}

impl<'i> FindSlice<(u8,)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: (u8,)) -> Option<crate::lib::std::ops::Range<usize>> {
        memchr(substr.0, self).map(|i| i..i + 1)
    }
}

impl<'i> FindSlice<(u8, u8)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: (u8, u8)) -> Option<crate::lib::std::ops::Range<usize>> {
        memchr2(substr, self).map(|i| i..i + 1)
    }
}

impl<'i> FindSlice<(u8, u8, u8)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: (u8, u8, u8)) -> Option<crate::lib::std::ops::Range<usize>> {
        memchr3(substr, self).map(|i| i..i + 1)
    }
}

impl<'i, 's> FindSlice<&'s str> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: &'s str) -> Option<crate::lib::std::ops::Range<usize>> {
        self.find_slice(substr.as_bytes())
    }
}

impl<'i, 's> FindSlice<(&'s str,)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: (&'s str,)) -> Option<crate::lib::std::ops::Range<usize>> {
        memmem(self, substr.0.as_bytes())
    }
}

impl<'i, 's> FindSlice<(&'s str, &'s str)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(&self, substr: (&'s str, &'s str)) -> Option<crate::lib::std::ops::Range<usize>> {
        memmem2(self, (substr.0.as_bytes(), substr.1.as_bytes()))
    }
}

impl<'i, 's> FindSlice<(&'s str, &'s str, &'s str)> for &'i [u8] {
    #[inline(always)]
    fn find_slice(
        &self,
        substr: (&'s str, &'s str, &'s str),
    ) -> Option<crate::lib::std::ops::Range<usize>> {
        memmem3(
            self,
            (
                substr.0.as_bytes(),
                substr.1.as_bytes(),
                substr.2.as_bytes(),
            ),
        )
    }
}

impl<'i, 's> FindSlice<&'s str> for &'i str {
    #[inline(always)]
    fn find_slice(&self, substr: &'s str) -> Option<crate::lib::std::ops::Range<usize>> {
        self.as_bytes().find_slice(substr)
    }
}

impl<'i, 's> FindSlice<(&'s str,)> for &'i str {
    #[inline(always)]
    fn find_slice(&self, substr: (&'s str,)) -> Option<crate::lib::std::ops::Range<usize>> {
        self.as_bytes().find_slice(substr)
    }
}

impl<'i, 's> FindSlice<(&'s str, &'s str)> for &'i str {
    #[inline(always)]
    fn find_slice(&self, substr: (&'s str, &'s str)) -> Option<crate::lib::std::ops::Range<usize>> {
        self.as_bytes().find_slice(substr)
    }
}

impl<'i, 's> FindSlice<(&'s str, &'s str, &'s str)> for &'i str {
    #[inline(always)]
    fn find_slice(
        &self,
        substr: (&'s str, &'s str, &'s str),
    ) -> Option<crate::lib::std::ops::Range<usize>> {
        self.as_bytes().find_slice(substr)
    }
}

impl<'i> FindSlice<char> for &'i str {
    #[inline(always)]
    fn find_slice(&self, substr: char) -> Option<crate::lib::std::ops::Range<usize>> {
        self.as_bytes().find_slice(substr)
    }
}

impl<'i> FindSlice<(char,)> for &'i str {
    #[inline(always)]
    fn find_slice(&self, substr: (char,)) -> Option<crate::lib::std::ops::Range<usize>> {
        self.as_bytes().find_slice(substr)
    }
}

impl<'i> FindSlice<(char, char)> for &'i str {
    #[inline(always)]
    fn find_slice(&self, substr: (char, char)) -> Option<crate::lib::std::ops::Range<usize>> {
        self.as_bytes().find_slice(substr)
    }
}

impl<'i> FindSlice<(char, char, char)> for &'i str {
    #[inline(always)]
    fn find_slice(&self, substr: (char, char, char)) -> Option<crate::lib::std::ops::Range<usize>> {
        self.as_bytes().find_slice(substr)
    }
}

impl<'i, S> FindSlice<S> for &'i Bytes
where
    &'i [u8]: FindSlice<S>,
{
    #[inline(always)]
    fn find_slice(&self, substr: S) -> Option<crate::lib::std::ops::Range<usize>> {
        let bytes = (*self).as_bytes();
        let offset = bytes.find_slice(substr);
        offset
    }
}

impl<'i, S> FindSlice<S> for &'i BStr
where
    &'i [u8]: FindSlice<S>,
{
    #[inline(always)]
    fn find_slice(&self, substr: S) -> Option<crate::lib::std::ops::Range<usize>> {
        let bytes = (*self).as_bytes();
        let offset = bytes.find_slice(substr);
        offset
    }
}

impl<I, T> FindSlice<T> for Located<I>
where
    I: FindSlice<T>,
{
    #[inline(always)]
    fn find_slice(&self, substr: T) -> Option<crate::lib::std::ops::Range<usize>> {
        self.input.find_slice(substr)
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E, T> FindSlice<T> for Recoverable<I, E>
where
    I: Stream,
    I: FindSlice<T>,
{
    #[inline(always)]
    fn find_slice(&self, substr: T) -> Option<crate::lib::std::ops::Range<usize>> {
        self.input.find_slice(substr)
    }
}

impl<I, S, T> FindSlice<T> for Stateful<I, S>
where
    I: FindSlice<T>,
{
    #[inline(always)]
    fn find_slice(&self, substr: T) -> Option<crate::lib::std::ops::Range<usize>> {
        self.input.find_slice(substr)
    }
}

impl<I, T> FindSlice<T> for Partial<I>
where
    I: FindSlice<T>,
{
    #[inline(always)]
    fn find_slice(&self, substr: T) -> Option<crate::lib::std::ops::Range<usize>> {
        self.input.find_slice(substr)
    }
}

/// Used to integrate `str`'s `parse()` method
pub trait ParseSlice<R> {
    /// Succeeds if `parse()` succeeded
    ///
    /// The byte slice implementation will first convert it to a `&str`, then apply the `parse()`
    /// function
    fn parse_slice(&self) -> Option<R>;
}

impl<'a, R: FromStr> ParseSlice<R> for &'a [u8] {
    #[inline(always)]
    fn parse_slice(&self) -> Option<R> {
        from_utf8(self).ok().and_then(|s| s.parse().ok())
    }
}

impl<'a, R: FromStr> ParseSlice<R> for &'a str {
    #[inline(always)]
    fn parse_slice(&self) -> Option<R> {
        self.parse().ok()
    }
}

/// Convert a `Stream` into an appropriate `Output` type
pub trait UpdateSlice: Stream {
    /// Convert an `Output` type to be used as `Stream`
    fn update_slice(self, inner: Self::Slice) -> Self;
}

impl<'a, T> UpdateSlice for &'a [T]
where
    T: Clone + crate::lib::std::fmt::Debug,
{
    #[inline(always)]
    fn update_slice(self, inner: Self::Slice) -> Self {
        inner
    }
}

impl<'a> UpdateSlice for &'a str {
    #[inline(always)]
    fn update_slice(self, inner: Self::Slice) -> Self {
        inner
    }
}

impl<'a> UpdateSlice for &'a Bytes {
    #[inline(always)]
    fn update_slice(self, inner: Self::Slice) -> Self {
        Bytes::new(inner)
    }
}

impl<'a> UpdateSlice for &'a BStr {
    #[inline(always)]
    fn update_slice(self, inner: Self::Slice) -> Self {
        BStr::new(inner)
    }
}

impl<I> UpdateSlice for Located<I>
where
    I: UpdateSlice,
{
    #[inline(always)]
    fn update_slice(mut self, inner: Self::Slice) -> Self {
        self.input = I::update_slice(self.input, inner);
        self
    }
}

#[cfg(feature = "unstable-recover")]
#[cfg(feature = "std")]
impl<I, E> UpdateSlice for Recoverable<I, E>
where
    I: Stream,
    I: UpdateSlice,
    E: crate::lib::std::fmt::Debug,
{
    #[inline(always)]
    fn update_slice(mut self, inner: Self::Slice) -> Self {
        self.input = I::update_slice(self.input, inner);
        self
    }
}

impl<I, S> UpdateSlice for Stateful<I, S>
where
    I: UpdateSlice,
    S: Clone + crate::lib::std::fmt::Debug,
{
    #[inline(always)]
    fn update_slice(mut self, inner: Self::Slice) -> Self {
        self.input = I::update_slice(self.input, inner);
        self
    }
}

impl<I> UpdateSlice for Partial<I>
where
    I: UpdateSlice,
{
    #[inline(always)]
    fn update_slice(self, inner: Self::Slice) -> Self {
        Partial {
            input: I::update_slice(self.input, inner),
            partial: self.partial,
        }
    }
}

/// Ensure checkpoint details are kept private
pub struct Checkpoint<T, S> {
    inner: T,
    stream: core::marker::PhantomData<S>,
}

impl<T, S> Checkpoint<T, S> {
    fn new(inner: T) -> Self {
        Self {
            inner,
            stream: Default::default(),
        }
    }
}

impl<T: Copy, S> Copy for Checkpoint<T, S> {}

impl<T: Clone, S> Clone for Checkpoint<T, S> {
    #[inline(always)]
    fn clone(&self) -> Self {
        Self {
            inner: self.inner.clone(),
            stream: Default::default(),
        }
    }
}

impl<T: PartialOrd, S> PartialOrd for Checkpoint<T, S> {
    #[inline(always)]
    fn partial_cmp(&self, other: &Self) -> Option<core::cmp::Ordering> {
        self.inner.partial_cmp(&other.inner)
    }
}

impl<T: Ord, S> Ord for Checkpoint<T, S> {
    #[inline(always)]
    fn cmp(&self, other: &Self) -> core::cmp::Ordering {
        self.inner.cmp(&other.inner)
    }
}

impl<T: PartialEq, S> PartialEq for Checkpoint<T, S> {
    #[inline(always)]
    fn eq(&self, other: &Self) -> bool {
        self.inner.eq(&other.inner)
    }
}

impl<T: Eq, S> Eq for Checkpoint<T, S> {}

impl<T: crate::lib::std::fmt::Debug, S> crate::lib::std::fmt::Debug for Checkpoint<T, S> {
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        self.inner.fmt(f)
    }
}

/// A range bounded inclusively for counting parses performed
///
/// This is flexible in what can be converted to a [Range]:
/// ```rust
/// # #[cfg(feature = "std")] {
/// # use winnow::prelude::*;
/// # use winnow::token::any;
/// # use winnow::combinator::repeat;
/// # fn inner(input: &mut &str) -> PResult<char> {
/// #     any.parse_next(input)
/// # }
/// # let mut input = "0123456789012345678901234567890123456789";
/// # let input = &mut input;
/// let parser: Vec<_> = repeat(5, inner).parse_next(input).unwrap();
/// # let mut input = "0123456789012345678901234567890123456789";
/// # let input = &mut input;
/// let parser: Vec<_> = repeat(.., inner).parse_next(input).unwrap();
/// # let mut input = "0123456789012345678901234567890123456789";
/// # let input = &mut input;
/// let parser: Vec<_> = repeat(1.., inner).parse_next(input).unwrap();
/// # let mut input = "0123456789012345678901234567890123456789";
/// # let input = &mut input;
/// let parser: Vec<_> = repeat(5..8, inner).parse_next(input).unwrap();
/// # let mut input = "0123456789012345678901234567890123456789";
/// # let input = &mut input;
/// let parser: Vec<_> = repeat(5..=8, inner).parse_next(input).unwrap();
/// # }
/// ```
#[derive(PartialEq, Eq)]
pub struct Range {
    pub(crate) start_inclusive: usize,
    pub(crate) end_inclusive: Option<usize>,
}

impl Range {
    #[inline(always)]
    fn raw(start_inclusive: usize, end_inclusive: Option<usize>) -> Self {
        Self {
            start_inclusive,
            end_inclusive,
        }
    }
}

impl crate::lib::std::ops::RangeBounds<usize> for Range {
    #[inline(always)]
    fn start_bound(&self) -> crate::lib::std::ops::Bound<&usize> {
        crate::lib::std::ops::Bound::Included(&self.start_inclusive)
    }

    #[inline(always)]
    fn end_bound(&self) -> crate::lib::std::ops::Bound<&usize> {
        if let Some(end_inclusive) = &self.end_inclusive {
            crate::lib::std::ops::Bound::Included(end_inclusive)
        } else {
            crate::lib::std::ops::Bound::Unbounded
        }
    }
}

impl From<usize> for Range {
    #[inline(always)]
    fn from(fixed: usize) -> Self {
        (fixed..=fixed).into()
    }
}

impl From<crate::lib::std::ops::Range<usize>> for Range {
    #[inline(always)]
    fn from(range: crate::lib::std::ops::Range<usize>) -> Self {
        let start_inclusive = range.start;
        let end_inclusive = Some(range.end.saturating_sub(1));
        Self::raw(start_inclusive, end_inclusive)
    }
}

impl From<crate::lib::std::ops::RangeFull> for Range {
    #[inline(always)]
    fn from(_: crate::lib::std::ops::RangeFull) -> Self {
        let start_inclusive = 0;
        let end_inclusive = None;
        Self::raw(start_inclusive, end_inclusive)
    }
}

impl From<crate::lib::std::ops::RangeFrom<usize>> for Range {
    #[inline(always)]
    fn from(range: crate::lib::std::ops::RangeFrom<usize>) -> Self {
        let start_inclusive = range.start;
        let end_inclusive = None;
        Self::raw(start_inclusive, end_inclusive)
    }
}

impl From<crate::lib::std::ops::RangeTo<usize>> for Range {
    #[inline(always)]
    fn from(range: crate::lib::std::ops::RangeTo<usize>) -> Self {
        let start_inclusive = 0;
        let end_inclusive = Some(range.end.saturating_sub(1));
        Self::raw(start_inclusive, end_inclusive)
    }
}

impl From<crate::lib::std::ops::RangeInclusive<usize>> for Range {
    #[inline(always)]
    fn from(range: crate::lib::std::ops::RangeInclusive<usize>) -> Self {
        let start_inclusive = *range.start();
        let end_inclusive = Some(*range.end());
        Self::raw(start_inclusive, end_inclusive)
    }
}

impl From<crate::lib::std::ops::RangeToInclusive<usize>> for Range {
    #[inline(always)]
    fn from(range: crate::lib::std::ops::RangeToInclusive<usize>) -> Self {
        let start_inclusive = 0;
        let end_inclusive = Some(range.end);
        Self::raw(start_inclusive, end_inclusive)
    }
}

impl crate::lib::std::fmt::Display for Range {
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        self.start_inclusive.fmt(f)?;
        match self.end_inclusive {
            Some(e) if e == self.start_inclusive => {}
            Some(e) => {
                "..=".fmt(f)?;
                e.fmt(f)?;
            }
            None => {
                "..".fmt(f)?;
            }
        }
        Ok(())
    }
}

impl crate::lib::std::fmt::Debug for Range {
    fn fmt(&self, f: &mut crate::lib::std::fmt::Formatter<'_>) -> crate::lib::std::fmt::Result {
        write!(f, "{self}")
    }
}

/// Abstracts something which can extend an `Extend`.
/// Used to build modified input slices in `escaped_transform`
pub trait Accumulate<T>: Sized {
    /// Create a new `Extend` of the correct type
    fn initial(capacity: Option<usize>) -> Self;
    /// Accumulate the input into an accumulator
    fn accumulate(&mut self, acc: T);
}

impl<T> Accumulate<T> for () {
    #[inline(always)]
    fn initial(_capacity: Option<usize>) -> Self {}
    #[inline(always)]
    fn accumulate(&mut self, _acc: T) {}
}

impl<T> Accumulate<T> for usize {
    #[inline(always)]
    fn initial(_capacity: Option<usize>) -> Self {
        0
    }
    #[inline(always)]
    fn accumulate(&mut self, _acc: T) {
        *self += 1;
    }
}

#[cfg(feature = "alloc")]
impl<T> Accumulate<T> for Vec<T> {
    #[inline(always)]
    fn initial(capacity: Option<usize>) -> Self {
        match capacity {
            Some(capacity) => Vec::with_capacity(clamp_capacity::<T>(capacity)),
            None => Vec::new(),
        }
    }
    #[inline(always)]
    fn accumulate(&mut self, acc: T) {
        self.push(acc);
    }
}

#[cfg(feature = "alloc")]
impl<'i, T: Clone> Accumulate<&'i [T]> for Vec<T> {
    #[inline(always)]
    fn initial(capacity: Option<usize>) -> Self {
        match capacity {
            Some(capacity) => Vec::with_capacity(clamp_capacity::<T>(capacity)),
            None => Vec::new(),
        }
    }
    #[inline(always)]
    fn accumulate(&mut self, acc: &'i [T]) {
        self.extend(acc.iter().cloned());
    }
}

#[cfg(feature = "alloc")]
impl Accumulate<char> for String {
    #[inline(always)]
    fn initial(capacity: Option<usize>) -> Self {
        match capacity {
            Some(capacity) => String::with_capacity(clamp_capacity::<char>(capacity)),
            None => String::new(),
        }
    }
    #[inline(always)]
    fn accumulate(&mut self, acc: char) {
        self.push(acc);
    }
}

#[cfg(feature = "alloc")]
impl<'i> Accumulate<&'i str> for String {
    #[inline(always)]
    fn initial(capacity: Option<usize>) -> Self {
        match capacity {
            Some(capacity) => String::with_capacity(clamp_capacity::<char>(capacity)),
            None => String::new(),
        }
    }
    #[inline(always)]
    fn accumulate(&mut self, acc: &'i str) {
        self.push_str(acc);
    }
}

#[cfg(feature = "alloc")]
impl<K, V> Accumulate<(K, V)> for BTreeMap<K, V>
where
    K: crate::lib::std::cmp::Ord,
{
    #[inline(always)]
    fn initial(_capacity: Option<usize>) -> Self {
        BTreeMap::new()
    }
    #[inline(always)]
    fn accumulate(&mut self, (key, value): (K, V)) {
        self.insert(key, value);
    }
}

#[cfg(feature = "std")]
impl<K, V, S> Accumulate<(K, V)> for HashMap<K, V, S>
where
    K: crate::lib::std::cmp::Eq + crate::lib::std::hash::Hash,
    S: BuildHasher + Default,
{
    #[inline(always)]
    fn initial(capacity: Option<usize>) -> Self {
        let h = S::default();
        match capacity {
            Some(capacity) => {
                HashMap::with_capacity_and_hasher(clamp_capacity::<(K, V)>(capacity), h)
            }
            None => HashMap::with_hasher(h),
        }
    }
    #[inline(always)]
    fn accumulate(&mut self, (key, value): (K, V)) {
        self.insert(key, value);
    }
}

#[cfg(feature = "alloc")]
impl<K> Accumulate<K> for BTreeSet<K>
where
    K: crate::lib::std::cmp::Ord,
{
    #[inline(always)]
    fn initial(_capacity: Option<usize>) -> Self {
        BTreeSet::new()
    }
    #[inline(always)]
    fn accumulate(&mut self, key: K) {
        self.insert(key);
    }
}

#[cfg(feature = "std")]
impl<K, S> Accumulate<K> for HashSet<K, S>
where
    K: crate::lib::std::cmp::Eq + crate::lib::std::hash::Hash,
    S: BuildHasher + Default,
{
    #[inline(always)]
    fn initial(capacity: Option<usize>) -> Self {
        let h = S::default();
        match capacity {
            Some(capacity) => HashSet::with_capacity_and_hasher(clamp_capacity::<K>(capacity), h),
            None => HashSet::with_hasher(h),
        }
    }
    #[inline(always)]
    fn accumulate(&mut self, key: K) {
        self.insert(key);
    }
}

#[cfg(feature = "alloc")]
#[inline]
pub(crate) fn clamp_capacity<T>(capacity: usize) -> usize {
    /// Don't pre-allocate more than 64KiB when calling `Vec::with_capacity`.
    ///
    /// Pre-allocating memory is a nice optimization but count fields can't
    /// always be trusted. We should clamp initial capacities to some reasonable
    /// amount. This reduces the risk of a bogus count value triggering a panic
    /// due to an OOM error.
    ///
    /// This does not affect correctness. `winnow` will always read the full number
    /// of elements regardless of the capacity cap.
    const MAX_INITIAL_CAPACITY_BYTES: usize = 65536;

    let max_initial_capacity =
        MAX_INITIAL_CAPACITY_BYTES / crate::lib::std::mem::size_of::<T>().max(1);
    capacity.min(max_initial_capacity)
}

/// Helper trait to convert numbers to usize.
///
/// By default, usize implements `From<u8>` and `From<u16>` but not
/// `From<u32>` and `From<u64>` because that would be invalid on some
/// platforms. This trait implements the conversion for platforms
/// with 32 and 64 bits pointer platforms
pub trait ToUsize {
    /// converts self to usize
    fn to_usize(&self) -> usize;
}

impl ToUsize for u8 {
    #[inline(always)]
    fn to_usize(&self) -> usize {
        *self as usize
    }
}

impl ToUsize for u16 {
    #[inline(always)]
    fn to_usize(&self) -> usize {
        *self as usize
    }
}

impl ToUsize for usize {
    #[inline(always)]
    fn to_usize(&self) -> usize {
        *self
    }
}

#[cfg(any(target_pointer_width = "32", target_pointer_width = "64"))]
impl ToUsize for u32 {
    #[inline(always)]
    fn to_usize(&self) -> usize {
        *self as usize
    }
}

#[cfg(target_pointer_width = "64")]
impl ToUsize for u64 {
    #[inline(always)]
    fn to_usize(&self) -> usize {
        *self as usize
    }
}

/// Transforms a token into a char for basic string parsing
#[allow(clippy::len_without_is_empty)]
#[allow(clippy::wrong_self_convention)]
pub trait AsChar {
    /// Makes a char from self
    ///
    /// # Example
    ///
    /// ```
    /// use winnow::stream::AsChar as _;
    ///
    /// assert_eq!('a'.as_char(), 'a');
    /// assert_eq!(u8::MAX.as_char(), std::char::from_u32(u8::MAX as u32).unwrap());
    /// ```
    fn as_char(self) -> char;

    /// Tests that self is an alphabetic character
    ///
    /// **Warning:** for `&str` it matches alphabetic
    /// characters outside of the 52 ASCII letters
    fn is_alpha(self) -> bool;

    /// Tests that self is an alphabetic character
    /// or a decimal digit
    fn is_alphanum(self) -> bool;
    /// Tests that self is a decimal digit
    fn is_dec_digit(self) -> bool;
    /// Tests that self is an hex digit
    fn is_hex_digit(self) -> bool;
    /// Tests that self is an octal digit
    fn is_oct_digit(self) -> bool;
    /// Gets the len in bytes for self
    fn len(self) -> usize;
    /// Tests that self is ASCII space or tab
    fn is_space(self) -> bool;
    /// Tests if byte is ASCII newline: \n
    fn is_newline(self) -> bool;
}

impl AsChar for u8 {
    #[inline(always)]
    fn as_char(self) -> char {
        self as char
    }
    #[inline]
    fn is_alpha(self) -> bool {
        matches!(self, 0x41..=0x5A | 0x61..=0x7A)
    }
    #[inline]
    fn is_alphanum(self) -> bool {
        self.is_alpha() || self.is_dec_digit()
    }
    #[inline]
    fn is_dec_digit(self) -> bool {
        matches!(self, 0x30..=0x39)
    }
    #[inline]
    fn is_hex_digit(self) -> bool {
        matches!(self, 0x30..=0x39 | 0x41..=0x46 | 0x61..=0x66)
    }
    #[inline]
    fn is_oct_digit(self) -> bool {
        matches!(self, 0x30..=0x37)
    }
    #[inline]
    fn len(self) -> usize {
        1
    }
    #[inline]
    fn is_space(self) -> bool {
        self == b' ' || self == b'\t'
    }
    #[inline]
    fn is_newline(self) -> bool {
        self == b'\n'
    }
}

impl<'a> AsChar for &'a u8 {
    #[inline(always)]
    fn as_char(self) -> char {
        (*self).as_char()
    }
    #[inline(always)]
    fn is_alpha(self) -> bool {
        (*self).is_alpha()
    }
    #[inline(always)]
    fn is_alphanum(self) -> bool {
        (*self).is_alphanum()
    }
    #[inline(always)]
    fn is_dec_digit(self) -> bool {
        (*self).is_dec_digit()
    }
    #[inline(always)]
    fn is_hex_digit(self) -> bool {
        (*self).is_hex_digit()
    }
    #[inline(always)]
    fn is_oct_digit(self) -> bool {
        (*self).is_oct_digit()
    }
    #[inline(always)]
    fn len(self) -> usize {
        (*self).len()
    }
    #[inline(always)]
    fn is_space(self) -> bool {
        (*self).is_space()
    }
    #[inline(always)]
    fn is_newline(self) -> bool {
        (*self).is_newline()
    }
}

impl AsChar for char {
    #[inline(always)]
    fn as_char(self) -> char {
        self
    }
    #[inline]
    fn is_alpha(self) -> bool {
        self.is_ascii_alphabetic()
    }
    #[inline]
    fn is_alphanum(self) -> bool {
        self.is_alpha() || self.is_dec_digit()
    }
    #[inline]
    fn is_dec_digit(self) -> bool {
        self.is_ascii_digit()
    }
    #[inline]
    fn is_hex_digit(self) -> bool {
        self.is_ascii_hexdigit()
    }
    #[inline]
    fn is_oct_digit(self) -> bool {
        self.is_digit(8)
    }
    #[inline]
    fn len(self) -> usize {
        self.len_utf8()
    }
    #[inline]
    fn is_space(self) -> bool {
        self == ' ' || self == '\t'
    }
    #[inline]
    fn is_newline(self) -> bool {
        self == '\n'
    }
}

impl<'a> AsChar for &'a char {
    #[inline(always)]
    fn as_char(self) -> char {
        (*self).as_char()
    }
    #[inline(always)]
    fn is_alpha(self) -> bool {
        (*self).is_alpha()
    }
    #[inline(always)]
    fn is_alphanum(self) -> bool {
        (*self).is_alphanum()
    }
    #[inline(always)]
    fn is_dec_digit(self) -> bool {
        (*self).is_dec_digit()
    }
    #[inline(always)]
    fn is_hex_digit(self) -> bool {
        (*self).is_hex_digit()
    }
    #[inline(always)]
    fn is_oct_digit(self) -> bool {
        (*self).is_oct_digit()
    }
    #[inline(always)]
    fn len(self) -> usize {
        (*self).len()
    }
    #[inline(always)]
    fn is_space(self) -> bool {
        (*self).is_space()
    }
    #[inline(always)]
    fn is_newline(self) -> bool {
        (*self).is_newline()
    }
}

/// Check if a token is in a set of possible tokens
///
/// While this can be implemented manually, you can also build up sets using:
/// - `b'c'` and `'c'`
/// - `b""`
/// - `|c| true`
/// - `b'a'..=b'z'`, `'a'..='z'` (etc for each [range type][std::ops])
/// - `(set1, set2, ...)`
///
/// # Example
///
/// For example, you could implement `hex_digit0` as:
/// ```
/// # use winnow::prelude::*;
/// # use winnow::{error::ErrMode, error::ErrorKind, error::InputError};
/// # use winnow::token::take_while;
/// fn hex_digit1<'s>(input: &mut &'s str) -> PResult<&'s str, InputError<&'s str>> {
///     take_while(1.., ('a'..='f', 'A'..='F', '0'..='9')).parse_next(input)
/// }
///
/// assert_eq!(hex_digit1.parse_peek("21cZ"), Ok(("Z", "21c")));
/// assert_eq!(hex_digit1.parse_peek("H2"), Err(ErrMode::Backtrack(InputError::new("H2", ErrorKind::Slice))));
/// assert_eq!(hex_digit1.parse_peek(""), Err(ErrMode::Backtrack(InputError::new("", ErrorKind::Slice))));
/// ```
pub trait ContainsToken<T> {
    /// Returns true if self contains the token
    fn contains_token(&self, token: T) -> bool;
}

impl ContainsToken<u8> for u8 {
    #[inline(always)]
    fn contains_token(&self, token: u8) -> bool {
        *self == token
    }
}

impl<'a> ContainsToken<&'a u8> for u8 {
    #[inline(always)]
    fn contains_token(&self, token: &u8) -> bool {
        self.contains_token(*token)
    }
}

impl ContainsToken<char> for u8 {
    #[inline(always)]
    fn contains_token(&self, token: char) -> bool {
        self.as_char() == token
    }
}

impl<'a> ContainsToken<&'a char> for u8 {
    #[inline(always)]
    fn contains_token(&self, token: &char) -> bool {
        self.contains_token(*token)
    }
}

impl<C: AsChar> ContainsToken<C> for char {
    #[inline(always)]
    fn contains_token(&self, token: C) -> bool {
        *self == token.as_char()
    }
}

impl<C, F: Fn(C) -> bool> ContainsToken<C> for F {
    #[inline(always)]
    fn contains_token(&self, token: C) -> bool {
        self(token)
    }
}

impl<C1: AsChar, C2: AsChar + Clone> ContainsToken<C1> for crate::lib::std::ops::Range<C2> {
    #[inline(always)]
    fn contains_token(&self, token: C1) -> bool {
        let start = self.start.clone().as_char();
        let end = self.end.clone().as_char();
        (start..end).contains(&token.as_char())
    }
}

impl<C1: AsChar, C2: AsChar + Clone> ContainsToken<C1>
    for crate::lib::std::ops::RangeInclusive<C2>
{
    #[inline(always)]
    fn contains_token(&self, token: C1) -> bool {
        let start = self.start().clone().as_char();
        let end = self.end().clone().as_char();
        (start..=end).contains(&token.as_char())
    }
}

impl<C1: AsChar, C2: AsChar + Clone> ContainsToken<C1> for crate::lib::std::ops::RangeFrom<C2> {
    #[inline(always)]
    fn contains_token(&self, token: C1) -> bool {
        let start = self.start.clone().as_char();
        (start..).contains(&token.as_char())
    }
}

impl<C1: AsChar, C2: AsChar + Clone> ContainsToken<C1> for crate::lib::std::ops::RangeTo<C2> {
    #[inline(always)]
    fn contains_token(&self, token: C1) -> bool {
        let end = self.end.clone().as_char();
        (..end).contains(&token.as_char())
    }
}

impl<C1: AsChar, C2: AsChar + Clone> ContainsToken<C1>
    for crate::lib::std::ops::RangeToInclusive<C2>
{
    #[inline(always)]
    fn contains_token(&self, token: C1) -> bool {
        let end = self.end.clone().as_char();
        (..=end).contains(&token.as_char())
    }
}

impl<C1: AsChar> ContainsToken<C1> for crate::lib::std::ops::RangeFull {
    #[inline(always)]
    fn contains_token(&self, _token: C1) -> bool {
        true
    }
}

impl<C: AsChar> ContainsToken<C> for &'_ [u8] {
    #[inline]
    fn contains_token(&self, token: C) -> bool {
        let token = token.as_char();
        self.iter().any(|t| t.as_char() == token)
    }
}

impl<C: AsChar> ContainsToken<C> for &'_ [char] {
    #[inline]
    fn contains_token(&self, token: C) -> bool {
        let token = token.as_char();
        self.iter().any(|t| *t == token)
    }
}

impl<const LEN: usize, C: AsChar> ContainsToken<C> for &'_ [u8; LEN] {
    #[inline]
    fn contains_token(&self, token: C) -> bool {
        let token = token.as_char();
        self.iter().any(|t| t.as_char() == token)
    }
}

impl<const LEN: usize, C: AsChar> ContainsToken<C> for &'_ [char; LEN] {
    #[inline]
    fn contains_token(&self, token: C) -> bool {
        let token = token.as_char();
        self.iter().any(|t| *t == token)
    }
}

impl<const LEN: usize, C: AsChar> ContainsToken<C> for [u8; LEN] {
    #[inline]
    fn contains_token(&self, token: C) -> bool {
        let token = token.as_char();
        self.iter().any(|t| t.as_char() == token)
    }
}

impl<const LEN: usize, C: AsChar> ContainsToken<C> for [char; LEN] {
    #[inline]
    fn contains_token(&self, token: C) -> bool {
        let token = token.as_char();
        self.iter().any(|t| *t == token)
    }
}

impl<T> ContainsToken<T> for () {
    #[inline(always)]
    fn contains_token(&self, _token: T) -> bool {
        false
    }
}

macro_rules! impl_contains_token_for_tuple {
  ($($haystack:ident),+) => (
    #[allow(non_snake_case)]
    impl<T, $($haystack),+> ContainsToken<T> for ($($haystack),+,)
    where
    T: Clone,
      $($haystack: ContainsToken<T>),+
    {
    #[inline]
      fn contains_token(&self, token: T) -> bool {
        let ($(ref $haystack),+,) = *self;
        $($haystack.contains_token(token.clone()) || )+ false
      }
    }
  )
}

macro_rules! impl_contains_token_for_tuples {
    ($haystack1:ident, $($haystack:ident),+) => {
        impl_contains_token_for_tuples!(__impl $haystack1; $($haystack),+);
    };
    (__impl $($haystack:ident),+; $haystack1:ident $(,$haystack2:ident)*) => {
        impl_contains_token_for_tuple!($($haystack),+);
        impl_contains_token_for_tuples!(__impl $($haystack),+, $haystack1; $($haystack2),*);
    };
    (__impl $($haystack:ident),+;) => {
        impl_contains_token_for_tuple!($($haystack),+);
    }
}

impl_contains_token_for_tuples!(
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21
);

#[cfg(feature = "simd")]
#[inline(always)]
fn memchr(token: u8, slice: &[u8]) -> Option<usize> {
    memchr::memchr(token, slice)
}

#[cfg(feature = "simd")]
#[inline(always)]
fn memchr2(token: (u8, u8), slice: &[u8]) -> Option<usize> {
    memchr::memchr2(token.0, token.1, slice)
}

#[cfg(feature = "simd")]
#[inline(always)]
fn memchr3(token: (u8, u8, u8), slice: &[u8]) -> Option<usize> {
    memchr::memchr3(token.0, token.1, token.2, slice)
}

#[cfg(not(feature = "simd"))]
#[inline(always)]
fn memchr(token: u8, slice: &[u8]) -> Option<usize> {
    slice.iter().position(|t| *t == token)
}

#[cfg(not(feature = "simd"))]
#[inline(always)]
fn memchr2(token: (u8, u8), slice: &[u8]) -> Option<usize> {
    slice.iter().position(|t| *t == token.0 || *t == token.1)
}

#[cfg(not(feature = "simd"))]
#[inline(always)]
fn memchr3(token: (u8, u8, u8), slice: &[u8]) -> Option<usize> {
    slice
        .iter()
        .position(|t| *t == token.0 || *t == token.1 || *t == token.2)
}

#[inline(always)]
fn memmem(slice: &[u8], literal: &[u8]) -> Option<crate::lib::std::ops::Range<usize>> {
    match literal.len() {
        0 => Some(0..0),
        1 => memchr(literal[0], slice).map(|i| i..i + 1),
        _ => memmem_(slice, literal),
    }
}

#[inline(always)]
fn memmem2(slice: &[u8], literal: (&[u8], &[u8])) -> Option<crate::lib::std::ops::Range<usize>> {
    match (literal.0.len(), literal.1.len()) {
        (0, _) | (_, 0) => Some(0..0),
        (1, 1) => memchr2((literal.0[0], literal.1[0]), slice).map(|i| i..i + 1),
        _ => memmem2_(slice, literal),
    }
}

#[inline(always)]
fn memmem3(
    slice: &[u8],
    literal: (&[u8], &[u8], &[u8]),
) -> Option<crate::lib::std::ops::Range<usize>> {
    match (literal.0.len(), literal.1.len(), literal.2.len()) {
        (0, _, _) | (_, 0, _) | (_, _, 0) => Some(0..0),
        (1, 1, 1) => memchr3((literal.0[0], literal.1[0], literal.2[0]), slice).map(|i| i..i + 1),
        _ => memmem3_(slice, literal),
    }
}

#[cfg(feature = "simd")]
#[inline(always)]
fn memmem_(slice: &[u8], literal: &[u8]) -> Option<crate::lib::std::ops::Range<usize>> {
    let &prefix = match literal.first() {
        Some(x) => x,
        None => return Some(0..0),
    };
    #[allow(clippy::manual_find)] // faster this way
    for i in memchr::memchr_iter(prefix, slice) {
        if slice[i..].starts_with(literal) {
            let i_end = i + literal.len();
            return Some(i..i_end);
        }
    }
    None
}

#[cfg(feature = "simd")]
fn memmem2_(slice: &[u8], literal: (&[u8], &[u8])) -> Option<crate::lib::std::ops::Range<usize>> {
    let prefix = match (literal.0.first(), literal.1.first()) {
        (Some(&a), Some(&b)) => (a, b),
        _ => return Some(0..0),
    };
    #[allow(clippy::manual_find)] // faster this way
    for i in memchr::memchr2_iter(prefix.0, prefix.1, slice) {
        let subslice = &slice[i..];
        if subslice.starts_with(literal.0) {
            let i_end = i + literal.0.len();
            return Some(i..i_end);
        }
        if subslice.starts_with(literal.1) {
            let i_end = i + literal.1.len();
            return Some(i..i_end);
        }
    }
    None
}

#[cfg(feature = "simd")]
fn memmem3_(
    slice: &[u8],
    literal: (&[u8], &[u8], &[u8]),
) -> Option<crate::lib::std::ops::Range<usize>> {
    let prefix = match (literal.0.first(), literal.1.first(), literal.2.first()) {
        (Some(&a), Some(&b), Some(&c)) => (a, b, c),
        _ => return Some(0..0),
    };
    #[allow(clippy::manual_find)] // faster this way
    for i in memchr::memchr3_iter(prefix.0, prefix.1, prefix.2, slice) {
        let subslice = &slice[i..];
        if subslice.starts_with(literal.0) {
            let i_end = i + literal.0.len();
            return Some(i..i_end);
        }
        if subslice.starts_with(literal.1) {
            let i_end = i + literal.1.len();
            return Some(i..i_end);
        }
        if subslice.starts_with(literal.2) {
            let i_end = i + literal.2.len();
            return Some(i..i_end);
        }
    }
    None
}

#[cfg(not(feature = "simd"))]
fn memmem_(slice: &[u8], literal: &[u8]) -> Option<crate::lib::std::ops::Range<usize>> {
    for i in 0..slice.len() {
        let subslice = &slice[i..];
        if subslice.starts_with(literal) {
            let i_end = i + literal.len();
            return Some(i..i_end);
        }
    }
    None
}

#[cfg(not(feature = "simd"))]
fn memmem2_(slice: &[u8], literal: (&[u8], &[u8])) -> Option<crate::lib::std::ops::Range<usize>> {
    for i in 0..slice.len() {
        let subslice = &slice[i..];
        if subslice.starts_with(literal.0) {
            let i_end = i + literal.0.len();
            return Some(i..i_end);
        }
        if subslice.starts_with(literal.1) {
            let i_end = i + literal.1.len();
            return Some(i..i_end);
        }
    }
    None
}

#[cfg(not(feature = "simd"))]
fn memmem3_(
    slice: &[u8],
    literal: (&[u8], &[u8], &[u8]),
) -> Option<crate::lib::std::ops::Range<usize>> {
    for i in 0..slice.len() {
        let subslice = &slice[i..];
        if subslice.starts_with(literal.0) {
            let i_end = i + literal.0.len();
            return Some(i..i_end);
        }
        if subslice.starts_with(literal.1) {
            let i_end = i + literal.1.len();
            return Some(i..i_end);
        }
        if subslice.starts_with(literal.2) {
            let i_end = i + literal.2.len();
            return Some(i..i_end);
        }
    }
    None
}
