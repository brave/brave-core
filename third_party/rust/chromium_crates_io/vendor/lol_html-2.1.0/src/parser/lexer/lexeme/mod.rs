mod token_outline;

use crate::base::{Bytes, Range};
use std::fmt::{self, Debug};

pub(crate) use self::token_outline::*;

pub(crate) struct Lexeme<'i, T> {
    input: Bytes<'i>,
    raw_range: Range,
    pub(super) token_outline: T,
}

pub type TagLexeme<'i> = Lexeme<'i, TagTokenOutline>;
pub type NonTagContentLexeme<'i> = Lexeme<'i, Option<NonTagContentTokenOutline>>;

impl<'i, T> Lexeme<'i, T> {
    pub const fn new(input: Bytes<'i>, token_outline: T, raw_range: Range) -> Self {
        Lexeme {
            input,
            raw_range,
            token_outline,
        }
    }

    #[inline]
    pub const fn input(&self) -> &Bytes<'i> {
        &self.input
    }

    #[inline]
    pub const fn token_outline(&self) -> &T {
        &self.token_outline
    }

    #[inline]
    pub const fn raw_range(&self) -> Range {
        self.raw_range
    }

    #[inline]
    pub fn part(&self, range: Range) -> Bytes<'_> {
        self.input.slice(range)
    }

    #[inline]
    pub fn opt_part(&self, range: Option<Range>) -> Option<Bytes<'_>> {
        self.input.opt_slice(range)
    }

    #[inline]
    pub fn raw(&self) -> Bytes<'_> {
        self.input.slice(self.raw_range())
    }
}

impl<T: Debug> Debug for Lexeme<'_, T> {
    #[cold]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // as_debug_string() is UTF-8, and the range for the input encoding is not guaranteed to match it
        let (before_raw, rest) = self.input.split_at(self.raw_range.start);
        let (raw, after_raw) = rest.split_at(self.raw_range.end - self.raw_range.start);

        f.debug_struct("Lexeme")
            .field(
                "raw",
                &format_args!(
                    "{}|{}|{}|{}|{}",
                    before_raw.as_debug_string(),
                    self.raw_range.start,
                    raw.as_debug_string(),
                    self.raw_range.end,
                    after_raw.as_debug_string(),
                ),
            )
            .field("token_outline", self.token_outline())
            .finish()
    }
}
