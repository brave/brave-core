mod token_outline;

use crate::base::{Bytes, Range};
use std::fmt::{self, Debug, Write};

pub use self::token_outline::*;

pub struct Lexeme<'i, T> {
    input: Bytes<'i>,
    raw_range: Range,
    pub(super) token_outline: T,
}

pub type TagLexeme<'i> = Lexeme<'i, TagTokenOutline>;
pub type NonTagContentLexeme<'i> = Lexeme<'i, Option<NonTagContentTokenOutline>>;

impl<'i, T> Lexeme<'i, T> {
    pub fn new(input: Bytes<'i>, token_outline: T, raw_range: Range) -> Self {
        Lexeme {
            input,
            raw_range,
            token_outline,
        }
    }

    #[inline]
    pub fn input(&self) -> &Bytes<'i> {
        &self.input
    }

    #[inline]
    pub fn token_outline(&self) -> &T {
        &self.token_outline
    }

    #[inline]
    pub fn raw_range(&self) -> Range {
        self.raw_range
    }

    #[inline]
    pub fn part(&self, range: Range) -> Bytes {
        self.input.slice(range)
    }

    #[inline]
    pub fn opt_part(&self, range: Option<Range>) -> Option<Bytes> {
        self.input.opt_slice(range)
    }

    #[inline]
    pub fn raw(&self) -> Bytes {
        self.input.slice(self.raw_range())
    }
}

impl<T: Debug> Debug for Lexeme<'_, T> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let mut builder = f.debug_struct("Lexeme");
        let mut pretty_raw = self.input.as_debug_string();
        let mut start = String::new();
        let mut end = String::new();

        write!(start, "|{}|", self.raw_range.start)?;
        write!(end, "|{}|", self.raw_range.end)?;

        pretty_raw.insert_str(self.raw_range.end, &end);
        pretty_raw.insert_str(self.raw_range.start, &start);

        builder
            .field("raw", &pretty_raw)
            .field("token_outline", self.token_outline())
            .finish()
    }
}
