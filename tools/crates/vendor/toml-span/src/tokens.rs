#![allow(missing_docs)]
//! The tokenizer is publicly exposed if you wish to use it instead

use crate::{Span, value::Key};
use std::{borrow::Cow, char, str};

#[derive(Eq, PartialEq, Debug)]
pub enum Token<'a> {
    Whitespace(&'a str),
    Newline,
    Comment(&'a str),

    Equals,
    Period,
    Comma,
    Colon,
    Plus,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,

    Keylike(&'a str),
    String {
        src: &'a str,
        val: Cow<'a, str>,
        multiline: bool,
    },
}

#[derive(Eq, PartialEq, Debug)]
pub enum Error {
    InvalidCharInString(usize, char),
    InvalidEscape(usize, char),
    InvalidHexEscape(usize, char),
    InvalidEscapeValue(usize, usize, u32),
    NewlineInString(usize),
    Unexpected(usize, char),
    UnterminatedString(usize),
    MultilineStringKey(usize, usize),
    Wanted {
        at: usize,
        expected: &'static str,
        found: &'static str,
    },
}

#[derive(Clone)]
pub struct Tokenizer<'a> {
    input: &'a str,
    chars: CrlfFold<'a>,
}

#[derive(Clone)]
struct CrlfFold<'a> {
    chars: str::CharIndices<'a>,
}

#[derive(Debug)]
enum MaybeString {
    NotEscaped(usize),
    Owned(String),
}

impl<'a> Tokenizer<'a> {
    pub fn new(input: &'a str) -> Tokenizer<'a> {
        let mut t = Tokenizer {
            input,
            chars: CrlfFold {
                chars: input.char_indices(),
            },
        };
        // Eat utf-8 BOM
        t.eatc('\u{feff}');
        t
    }

    pub fn step(&mut self) -> Result<Option<(Span, Token<'a>)>, Error> {
        let (start, token) = match self.one() {
            Some((start, '\n')) => (start, Token::Newline),
            Some((start, ' ' | '\t')) => (start, self.whitespace_token(start)),
            Some((start, '#')) => (start, self.comment_token(start)),
            Some((start, '=')) => (start, Token::Equals),
            Some((start, '.')) => (start, Token::Period),
            Some((start, ',')) => (start, Token::Comma),
            Some((start, ':')) => (start, Token::Colon),
            Some((start, '+')) => (start, Token::Plus),
            Some((start, '{')) => (start, Token::LeftBrace),
            Some((start, '}')) => (start, Token::RightBrace),
            Some((start, '[')) => (start, Token::LeftBracket),
            Some((start, ']')) => (start, Token::RightBracket),
            Some((start, '\'')) => return self.literal_string(start).map(|(s, t)| Some((s, t))),
            Some((start, '"')) => return self.basic_string(start).map(|(s, t)| Some((s, t))),
            Some((start, ch)) if is_keylike(ch) => (start, self.keylike(start)),
            Some((start, ch)) => return Err(Error::Unexpected(start, ch)),
            None => return Ok(None),
        };

        let span = self.step_span(start);
        Ok(Some((span, token)))
    }

    pub fn peek(&mut self) -> Result<Option<(Span, Token<'a>)>, Error> {
        self.clone().step()
    }

    pub fn eat(&mut self, expected: Token<'a>) -> Result<bool, Error> {
        self.eat_spanned(expected).map(|s| s.is_some())
    }

    /// Eat a value, returning it's span if it was consumed.
    pub fn eat_spanned(&mut self, expected: Token<'a>) -> Result<Option<Span>, Error> {
        let span = match self.peek()? {
            Some((span, ref found)) if expected == *found => span,
            Some(_) | None => return Ok(None),
        };

        drop(self.step());
        Ok(Some(span))
    }

    pub fn expect(&mut self, expected: Token<'a>) -> Result<(), Error> {
        // ignore span
        let _ = self.expect_spanned(expected)?;
        Ok(())
    }

    /// Expect the given token returning its span.
    pub fn expect_spanned(&mut self, expected: Token<'a>) -> Result<Span, Error> {
        let current = self.current();
        match self.step()? {
            Some((span, found)) => {
                if expected == found {
                    Ok(span)
                } else {
                    Err(Error::Wanted {
                        at: current,
                        expected: expected.describe(),
                        found: found.describe(),
                    })
                }
            }
            None => Err(Error::Wanted {
                at: self.input.len(),
                expected: expected.describe(),
                found: "eof",
            }),
        }
    }

    pub fn table_key(&mut self) -> Result<Key<'a>, Error> {
        let current = self.current();
        match self.step()? {
            Some((span, Token::Keylike(k))) => Ok(Key {
                span,
                name: k.into(),
            }),
            Some((
                span,
                Token::String {
                    src,
                    val,
                    multiline,
                    ..
                },
            )) => {
                let offset = self.substr_offset(src);
                if multiline {
                    return Err(Error::MultilineStringKey(offset, offset + val.len()));
                }
                match src.find('\n') {
                    None => Ok(Key { span, name: val }),
                    // This is not reachable
                    Some(i) => Err(Error::InvalidCharInString(i, '\n')),
                }
            }
            Some((_, other)) => Err(Error::Wanted {
                at: current,
                expected: "a table key",
                found: other.describe(),
            }),
            None => Err(Error::Wanted {
                at: self.input.len(),
                expected: "a table key",
                found: "eof",
            }),
        }
    }

    pub fn eat_whitespace(&mut self) {
        while self.eatc(' ') || self.eatc('\t') {
            // ...
        }
    }

    pub fn eat_comment(&mut self) -> Result<bool, Error> {
        if !self.eatc('#') {
            return Ok(false);
        }
        drop(self.comment_token(0));
        self.eat_newline_or_eof().map(|()| true)
    }

    pub fn eat_newline_or_eof(&mut self) -> Result<(), Error> {
        let current = self.current();
        match self.step()? {
            None | Some((_, Token::Newline)) => Ok(()),
            Some((_, other)) => Err(Error::Wanted {
                at: current,
                expected: "newline",
                found: other.describe(),
            }),
        }
    }

    pub fn skip_to_newline(&mut self) {
        loop {
            match self.one() {
                Some((_, '\n')) | None => break,
                _ => {}
            }
        }
    }

    fn eatc(&mut self, ch: char) -> bool {
        match self.chars.clone().next() {
            Some((_, ch2)) if ch == ch2 => {
                self.one();
                true
            }
            _ => false,
        }
    }

    pub fn current(&mut self) -> usize {
        match self.chars.clone().next() {
            Some(i) => i.0,
            None => self.input.len(),
        }
    }

    fn whitespace_token(&mut self, start: usize) -> Token<'a> {
        while self.eatc(' ') || self.eatc('\t') {
            // ...
        }
        Token::Whitespace(&self.input[start..self.current()])
    }

    fn comment_token(&mut self, start: usize) -> Token<'a> {
        while let Some((_, ch)) = self.chars.clone().next() {
            if ch != '\t' && !('\u{20}'..='\u{10ffff}').contains(&ch) {
                break;
            }
            self.one();
        }
        Token::Comment(&self.input[start..self.current()])
    }

    /// String spans are treated slightly differently, as we only want the
    /// characters in the string, not the quotes, as once the user gets the
    /// string and its span they won't know the actual begin/end which can
    /// be needed for doing substring indices (eg reporting error messages
    /// when parsing a string)
    #[allow(clippy::type_complexity)]
    fn read_string(
        &mut self,
        delim: char,
        start: usize,
        new_ch: &mut dyn FnMut(
            &mut Tokenizer<'_>,
            &mut MaybeString,
            bool,
            usize,
            char,
        ) -> Result<(), Error>,
    ) -> Result<(Span, Token<'a>), Error> {
        let mut multiline = false;
        if self.eatc(delim) {
            if self.eatc(delim) {
                multiline = true;
            } else {
                return Ok((
                    // Point the caret at the beginning of the quote, that looks
                    // better than the end quote
                    (start..start + 1).into(),
                    Token::String {
                        src: &self.input[start..start + 2],
                        val: Cow::Borrowed(""),
                        multiline: false,
                    },
                ));
            }
        }
        let mut val = MaybeString::NotEscaped(self.current());
        let mut n = 0;
        loop {
            n += 1;
            match self.one() {
                Some((i, '\n')) => {
                    if multiline {
                        if self.input.as_bytes()[i] == b'\r' {
                            val.make_owned(&self.input[..i]);
                        }
                        if n == 1 {
                            val = MaybeString::NotEscaped(self.current());
                        } else {
                            val.push('\n');
                        }
                    } else {
                        return Err(Error::NewlineInString(i));
                    }
                }
                Some((mut i, ch)) if ch == delim => {
                    let span = if multiline {
                        if !self.eatc(delim) {
                            val.push(delim);
                            continue;
                        }
                        if !self.eatc(delim) {
                            val.push(delim);
                            val.push(delim);
                            continue;
                        }
                        if self.eatc(delim) {
                            val.push(delim);
                            i += 1;
                        }
                        if self.eatc(delim) {
                            val.push(delim);
                            i += 1;
                        }

                        // Also skip the first newline after the opening delimiters
                        let maybe_nl = self.input.as_bytes()[start + 3];
                        let start_off = if maybe_nl == b'\n' {
                            4
                        } else if maybe_nl == b'\r' {
                            5
                        } else {
                            3
                        };

                        start + start_off..self.current() - 3
                    } else {
                        start + 1..self.current() - 1
                    }
                    .into();

                    return Ok((
                        span,
                        Token::String {
                            src: &self.input[start..self.current()],
                            val: val.into_cow(&self.input[..i]),
                            multiline,
                        },
                    ));
                }
                Some((i, c)) => new_ch(self, &mut val, multiline, i, c)?,
                None => return Err(Error::UnterminatedString(start)),
            }
        }
    }

    fn literal_string(&mut self, start: usize) -> Result<(Span, Token<'a>), Error> {
        self.read_string('\'', start, &mut |_me, val, _multi, i, ch| {
            if ch == '\u{09}' || (ch != '\u{7f}' && ('\u{20}'..='\u{10ffff}').contains(&ch)) {
                val.push(ch);
                Ok(())
            } else {
                Err(Error::InvalidCharInString(i, ch))
            }
        })
    }

    fn basic_string(&mut self, start: usize) -> Result<(Span, Token<'a>), Error> {
        self.read_string('"', start, &mut |me, val, multi, i, ch| match ch {
            '\\' => {
                val.make_owned(&me.input[..i]);
                match me.chars.next() {
                    Some((_, '"')) => val.push('"'),
                    Some((_, '\\')) => val.push('\\'),
                    Some((_, 'b')) => val.push('\u{8}'),
                    Some((_, 'f')) => val.push('\u{c}'),
                    Some((_, 'n')) => val.push('\n'),
                    Some((_, 'r')) => val.push('\r'),
                    Some((_, 't')) => val.push('\t'),
                    Some((_, 'e')) => val.push('\u{1b}'), // TOML 1.1: escape character
                    Some((i, c @ ('u' | 'U'))) => {
                        let c = if c == 'u' {
                            me.hex::<4>(start, i)
                        } else {
                            me.hex::<8>(start, i)
                        };
                        val.push(c?);
                    }
                    Some((i, 'x')) => {
                        // TOML 1.1: \xHH for codepoints < 255
                        val.push(me.hex::<2>(start, i)?);
                    }
                    Some((i, c @ (' ' | '\t' | '\n'))) if multi => {
                        if c != '\n' {
                            while let Some((_, ch)) = me.chars.clone().next() {
                                match ch {
                                    ' ' | '\t' => {
                                        me.chars.next();
                                    }
                                    '\n' => {
                                        me.chars.next();
                                        break;
                                    }
                                    _ => return Err(Error::InvalidEscape(i, c)),
                                }
                            }
                        }
                        while let Some((_, ch)) = me.chars.clone().next() {
                            match ch {
                                ' ' | '\t' | '\n' => {
                                    me.chars.next();
                                }
                                _ => break,
                            }
                        }
                    }
                    Some((i, c)) => return Err(Error::InvalidEscape(i, c)),
                    None => return Err(Error::UnterminatedString(start)),
                }
                Ok(())
            }
            ch if ch == '\u{09}' || (ch != '\u{7f}' && ('\u{20}'..='\u{10ffff}').contains(&ch)) => {
                val.push(ch);
                Ok(())
            }
            _ => Err(Error::InvalidCharInString(i, ch)),
        })
    }

    fn hex<const N: usize>(&mut self, start: usize, i: usize) -> Result<char, Error> {
        let mut buf = [0; N];
        for b in buf.iter_mut() {
            match self.one() {
                Some((_, ch)) if ch as u32 <= 0x7F && ch.is_ascii_hexdigit() => *b = ch as u8,
                Some((i, ch)) => return Err(Error::InvalidHexEscape(i, ch)),
                None => return Err(Error::UnterminatedString(start)),
            }
        }
        let val = u32::from_str_radix(std::str::from_utf8(&buf).unwrap(), 16).unwrap();
        match char::from_u32(val) {
            Some(ch) => Ok(ch),
            None => Err(Error::InvalidEscapeValue(i, N, val)),
        }
    }

    fn keylike(&mut self, start: usize) -> Token<'a> {
        while let Some((_, ch)) = self.peek_one() {
            if !is_keylike(ch) {
                break;
            }
            self.one();
        }
        Token::Keylike(&self.input[start..self.current()])
    }

    pub fn substr_offset(&self, s: &'a str) -> usize {
        assert!(s.len() <= self.input.len());
        let a = self.input.as_ptr() as usize;
        let b = s.as_ptr() as usize;
        assert!(a <= b);
        b - a
    }

    /// Calculate the span of a single character.
    fn step_span(&mut self, start: usize) -> Span {
        let end = match self.peek_one() {
            Some(t) => t.0,
            None => self.input.len(),
        };
        Span { start, end }
    }

    /// Peek one char without consuming it.
    fn peek_one(&mut self) -> Option<(usize, char)> {
        self.chars.clone().next()
    }

    /// Take one char.
    pub fn one(&mut self) -> Option<(usize, char)> {
        self.chars.next()
    }
}

impl Iterator for CrlfFold<'_> {
    type Item = (usize, char);

    fn next(&mut self) -> Option<(usize, char)> {
        self.chars.next().map(|(i, c)| {
            if c == '\r' {
                let mut attempt = self.chars.clone();
                if let Some((_, '\n')) = attempt.next() {
                    self.chars = attempt;
                    return (i, '\n');
                }
            }
            (i, c)
        })
    }
}

impl MaybeString {
    fn push(&mut self, ch: char) {
        match *self {
            MaybeString::NotEscaped(..) => {}
            MaybeString::Owned(ref mut s) => s.push(ch),
        }
    }

    fn make_owned(&mut self, input: &str) {
        match *self {
            MaybeString::NotEscaped(start) => {
                *self = MaybeString::Owned(input[start..].to_owned());
            }
            MaybeString::Owned(..) => {}
        }
    }

    fn into_cow(self, input: &str) -> Cow<'_, str> {
        match self {
            MaybeString::NotEscaped(start) => Cow::Borrowed(&input[start..]),
            MaybeString::Owned(s) => Cow::Owned(s),
        }
    }
}

#[inline]
fn is_keylike(ch: char) -> bool {
    ch.is_ascii_alphanumeric() || ch == '-' || ch == '_'
}

impl Token<'_> {
    pub fn describe(&self) -> &'static str {
        match *self {
            Token::Keylike(_) => "an identifier",
            Token::Equals => "an equals",
            Token::Period => "a period",
            Token::Comment(_) => "a comment",
            Token::Newline => "a newline",
            Token::Whitespace(_) => "whitespace",
            Token::Comma => "a comma",
            Token::RightBrace => "a right brace",
            Token::LeftBrace => "a left brace",
            Token::RightBracket => "a right bracket",
            Token::LeftBracket => "a left bracket",
            Token::String { multiline, .. } => {
                if multiline {
                    "a multiline string"
                } else {
                    "a string"
                }
            }
            Token::Colon => "a colon",
            Token::Plus => "a plus",
        }
    }
}
