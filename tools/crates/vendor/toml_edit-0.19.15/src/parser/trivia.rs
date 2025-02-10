use std::ops::RangeInclusive;

use winnow::combinator::alt;
use winnow::combinator::eof;
use winnow::combinator::opt;
use winnow::combinator::repeat;
use winnow::combinator::terminated;
use winnow::prelude::*;
use winnow::token::one_of;
use winnow::token::take_while;

use crate::parser::prelude::*;

pub(crate) unsafe fn from_utf8_unchecked<'b>(
    bytes: &'b [u8],
    safety_justification: &'static str,
) -> &'b str {
    if cfg!(debug_assertions) {
        // Catch problems more quickly when testing
        std::str::from_utf8(bytes).expect(safety_justification)
    } else {
        std::str::from_utf8_unchecked(bytes)
    }
}

// wschar = ( %x20 /              ; Space
//            %x09 )              ; Horizontal tab
pub(crate) const WSCHAR: (u8, u8) = (b' ', b'\t');

// ws = *wschar
pub(crate) fn ws<'i>(input: &mut Input<'i>) -> PResult<&'i str> {
    take_while(0.., WSCHAR)
        .map(|b| unsafe { from_utf8_unchecked(b, "`is_wschar` filters out on-ASCII") })
        .parse_next(input)
}

// non-ascii = %x80-D7FF / %xE000-10FFFF
// - ASCII is 0xxxxxxx
// - First byte for UTF-8 is 11xxxxxx
// - Subsequent UTF-8 bytes are 10xxxxxx
pub(crate) const NON_ASCII: RangeInclusive<u8> = 0x80..=0xff;

// non-eol = %x09 / %x20-7E / non-ascii
pub(crate) const NON_EOL: (u8, RangeInclusive<u8>, RangeInclusive<u8>) =
    (0x09, 0x20..=0x7E, NON_ASCII);

// comment-start-symbol = %x23 ; #
pub(crate) const COMMENT_START_SYMBOL: u8 = b'#';

// comment = comment-start-symbol *non-eol
pub(crate) fn comment<'i>(input: &mut Input<'i>) -> PResult<&'i [u8]> {
    (COMMENT_START_SYMBOL, take_while(0.., NON_EOL))
        .recognize()
        .parse_next(input)
}

// newline = ( %x0A /              ; LF
//             %x0D.0A )           ; CRLF
pub(crate) fn newline(input: &mut Input<'_>) -> PResult<u8> {
    alt((
        one_of(LF).value(b'\n'),
        (one_of(CR), one_of(LF)).value(b'\n'),
    ))
    .parse_next(input)
}
pub(crate) const LF: u8 = b'\n';
pub(crate) const CR: u8 = b'\r';

// ws-newline       = *( wschar / newline )
pub(crate) fn ws_newline<'i>(input: &mut Input<'i>) -> PResult<&'i str> {
    repeat(
        0..,
        alt((newline.value(&b"\n"[..]), take_while(1.., WSCHAR))),
    )
    .map(|()| ())
    .recognize()
    .map(|b| unsafe { from_utf8_unchecked(b, "`is_wschar` and `newline` filters out on-ASCII") })
    .parse_next(input)
}

// ws-newlines      = newline *( wschar / newline )
pub(crate) fn ws_newlines<'i>(input: &mut Input<'i>) -> PResult<&'i str> {
    (newline, ws_newline)
        .recognize()
        .map(|b| unsafe {
            from_utf8_unchecked(b, "`is_wschar` and `newline` filters out on-ASCII")
        })
        .parse_next(input)
}

// note: this rule is not present in the original grammar
// ws-comment-newline = *( ws-newline-nonempty / comment )
pub(crate) fn ws_comment_newline<'i>(input: &mut Input<'i>) -> PResult<&'i [u8]> {
    repeat(
        0..,
        alt((
            repeat(
                1..,
                alt((take_while(1.., WSCHAR), newline.value(&b"\n"[..]))),
            )
            .map(|()| ()),
            comment.value(()),
        )),
    )
    .map(|()| ())
    .recognize()
    .parse_next(input)
}

// note: this rule is not present in the original grammar
// line-ending = newline / eof
pub(crate) fn line_ending<'i>(input: &mut Input<'i>) -> PResult<&'i str> {
    alt((newline.value("\n"), eof.value(""))).parse_next(input)
}

// note: this rule is not present in the original grammar
// line-trailing = ws [comment] skip-line-ending
pub(crate) fn line_trailing(input: &mut Input<'_>) -> PResult<std::ops::Range<usize>> {
    terminated((ws, opt(comment)).span(), line_ending).parse_next(input)
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn trivia() {
        let inputs = [
            "",
            r#" "#,
            r#"
"#,
            r#"
# comment

# comment2


"#,
            r#"
        "#,
            r#"# comment
# comment2


   "#,
        ];
        for input in inputs {
            dbg!(input);
            let parsed = ws_comment_newline.parse(new_input(input));
            assert!(parsed.is_ok(), "{:?}", parsed);
            let parsed = parsed.unwrap();
            assert_eq!(parsed, input.as_bytes());
        }
    }
}
