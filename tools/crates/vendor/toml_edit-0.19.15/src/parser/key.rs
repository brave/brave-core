use std::ops::RangeInclusive;

use winnow::combinator::peek;
use winnow::combinator::separated1;
use winnow::token::any;
use winnow::token::take_while;
use winnow::trace::trace;

use crate::key::Key;
use crate::parser::errors::CustomError;
use crate::parser::prelude::*;
use crate::parser::strings::{basic_string, literal_string};
use crate::parser::trivia::{from_utf8_unchecked, ws};
use crate::repr::{Decor, Repr};
use crate::InternalString;
use crate::RawString;

// key = simple-key / dotted-key
// dotted-key = simple-key 1*( dot-sep simple-key )
pub(crate) fn key(input: &mut Input<'_>) -> PResult<Vec<Key>> {
    trace(
        "dotted-key",
        separated1(
            (ws.span(), simple_key, ws.span()).map(|(pre, (raw, key), suffix)| {
                Key::new(key)
                    .with_repr_unchecked(Repr::new_unchecked(raw))
                    .with_decor(Decor::new(
                        RawString::with_span(pre),
                        RawString::with_span(suffix),
                    ))
            }),
            DOT_SEP,
        )
        .context(StrContext::Label("key"))
        .try_map(|k: Vec<_>| {
            // Inserting the key will require recursion down the line
            RecursionCheck::check_depth(k.len())?;
            Ok::<_, CustomError>(k)
        }),
    )
    .parse_next(input)
}

// simple-key = quoted-key / unquoted-key
// quoted-key = basic-string / literal-string
pub(crate) fn simple_key(input: &mut Input<'_>) -> PResult<(RawString, InternalString)> {
    trace(
        "simple-key",
        dispatch! {peek(any);
            crate::parser::strings::QUOTATION_MARK => basic_string
                .map(|s: std::borrow::Cow<'_, str>| s.as_ref().into()),
            crate::parser::strings::APOSTROPHE => literal_string.map(|s: &str| s.into()),
            _ => unquoted_key.map(|s: &str| s.into()),
        }
        .with_span()
        .map(|(k, span)| {
            let raw = RawString::with_span(span);
            (raw, k)
        }),
    )
    .parse_next(input)
}

// unquoted-key = 1*( ALPHA / DIGIT / %x2D / %x5F ) ; A-Z / a-z / 0-9 / - / _
fn unquoted_key<'i>(input: &mut Input<'i>) -> PResult<&'i str> {
    trace(
        "unquoted-key",
        take_while(1.., UNQUOTED_CHAR)
            .map(|b| unsafe { from_utf8_unchecked(b, "`is_unquoted_char` filters out on-ASCII") }),
    )
    .parse_next(input)
}

pub(crate) fn is_unquoted_char(c: u8) -> bool {
    use winnow::stream::ContainsToken;
    UNQUOTED_CHAR.contains_token(c)
}

const UNQUOTED_CHAR: (
    RangeInclusive<u8>,
    RangeInclusive<u8>,
    RangeInclusive<u8>,
    u8,
    u8,
) = (b'A'..=b'Z', b'a'..=b'z', b'0'..=b'9', b'-', b'_');

// dot-sep   = ws %x2E ws  ; . Period
const DOT_SEP: u8 = b'.';

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn keys() {
        let cases = [
            ("a", "a"),
            (r#""hello\n ""#, "hello\n "),
            (r#"'hello\n '"#, "hello\\n "),
        ];

        for (input, expected) in cases {
            dbg!(input);
            let parsed = simple_key.parse(new_input(input));
            assert_eq!(
                parsed,
                Ok((RawString::with_span(0..(input.len())), expected.into())),
                "Parsing {input:?}"
            );
        }
    }
}
