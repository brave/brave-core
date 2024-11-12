use crate::ascii::dec_uint;
use crate::combinator::dispatch;
use crate::combinator::empty;
use crate::combinator::fail;
use crate::combinator::seq;
use crate::error::ErrMode;
use crate::error::ErrorKind;
use crate::error::ParserError;
use crate::prelude::*;
use crate::token::any;

#[test]
fn dispatch_basics() {
    fn escape_seq_char(input: &mut &str) -> PResult<char> {
        dispatch! {any;
            'b' => empty.value('\u{8}'),
            'f' => empty.value('\u{c}'),
            'n' => empty.value('\n'),
            'r' => empty.value('\r'),
            't' => empty.value('\t'),
            '\\' => empty.value('\\'),
            '"' => empty.value('"'),
            _ => fail::<_, char, _>,
        }
        .parse_next(input)
    }
    assert_eq!(escape_seq_char.parse_peek("b123"), Ok(("123", '\u{8}')));
    assert_eq!(
        escape_seq_char.parse_peek("error"),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &"rror",
            ErrorKind::Fail
        )))
    );
    assert_eq!(
        escape_seq_char.parse_peek(""),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &"",
            ErrorKind::Fail
        )))
    );
}

#[test]
fn seq_struct_basics() {
    #[derive(Debug, PartialEq)]
    struct Point {
        x: u32,
        y: u32,
    }

    fn parser(input: &mut &str) -> PResult<Point> {
        seq! {
            Point {
                x: dec_uint,
                _: ',',
                y: dec_uint,
            }
        }
        .parse_next(input)
    }
    assert_eq!(
        parser.parse_peek("123,4 remaining"),
        Ok((" remaining", Point { x: 123, y: 4 },)),
    );
    assert_eq!(
        parser.parse_peek("123, remaining"),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &" remaining",
            ErrorKind::Fail
        )))
    );
    assert_eq!(
        parser.parse_peek(""),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &"",
            ErrorKind::Fail
        )))
    );
}

#[test]
fn seq_struct_default_init() {
    #[derive(Debug, PartialEq, Default)]
    struct Point {
        x: u32,
        y: u32,
        z: u32,
    }

    fn parser(input: &mut &str) -> PResult<Point> {
        seq! {
            Point {
                x: dec_uint,
                _: ',',
                y: dec_uint,
                ..Default::default()
            }
        }
        .parse_next(input)
    }
    assert_eq!(
        parser.parse_peek("123,4 remaining"),
        Ok((" remaining", Point { x: 123, y: 4, z: 0 },)),
    );
    assert_eq!(
        parser.parse_peek("123, remaining"),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &" remaining",
            ErrorKind::Fail
        )))
    );
    assert_eq!(
        parser.parse_peek(""),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &"",
            ErrorKind::Fail
        )))
    );
}

#[test]
fn seq_struct_trailing_comma_elided() {
    #![allow(dead_code)]

    #[derive(Debug, PartialEq)]
    struct Point {
        x: u32,
        y: u32,
    }

    fn parser(input: &mut &str) -> PResult<Point> {
        seq! {
            Point {
                x: dec_uint,
                _: ',',
                y: dec_uint,
                _: empty,
            }
        }
        .parse_next(input)
    }
}

#[test]
fn seq_struct_no_trailing_comma() {
    #![allow(dead_code)]

    #[derive(Debug, PartialEq)]
    struct Point {
        x: u32,
        y: u32,
    }

    fn parser(input: &mut &str) -> PResult<Point> {
        seq! {
            Point {
                x: dec_uint,
                _: ',',
                y: dec_uint
            }
        }
        .parse_next(input)
    }
}

#[test]
fn seq_struct_no_trailing_comma_elided() {
    #![allow(dead_code)]

    #[derive(Debug, PartialEq)]
    struct Point {
        x: u32,
        y: u32,
    }

    fn parser(input: &mut &str) -> PResult<Point> {
        seq! {
            Point {
                x: dec_uint,
                _: ',',
                y: dec_uint,
                _: empty
            }
        }
        .parse_next(input)
    }
}

#[test]
fn seq_enum_struct_variant() {
    #[derive(Debug, PartialEq, Eq)]
    enum Expr {
        Add { lhs: u32, rhs: u32 },
        Mul(u32, u32),
    }

    fn add(input: &mut &[u8]) -> PResult<Expr> {
        seq! {Expr::Add {
            lhs: dec_uint::<_, u32, _>,
            _: b" + ",
            rhs: dec_uint::<_, u32, _>,
        }}
        .parse_next(input)
    }

    fn mul(input: &mut &[u8]) -> PResult<Expr> {
        seq!(Expr::Mul(
             dec_uint::<_, u32, _>,
             _: b" * ",
             dec_uint::<_, u32, _>,
        ))
        .parse_next(input)
    }

    assert_eq!(
        add.parse_peek(&b"1 + 2"[..]),
        Ok((&b""[..], Expr::Add { lhs: 1, rhs: 2 })),
    );

    assert_eq!(
        mul.parse_peek(&b"3 * 4"[..]),
        Ok((&b""[..], Expr::Mul(3, 4))),
    );
}

#[test]
fn seq_tuple_struct_basics() {
    #[derive(Debug, PartialEq)]
    struct Point(u32, u32);

    fn parser(input: &mut &str) -> PResult<Point> {
        seq! {
            Point(
                dec_uint,
                _: ',',
                dec_uint,
            )
        }
        .parse_next(input)
    }
    assert_eq!(
        parser.parse_peek("123,4 remaining"),
        Ok((" remaining", Point(123, 4),)),
    );
    assert_eq!(
        parser.parse_peek("123, remaining"),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &" remaining",
            ErrorKind::Fail
        )))
    );
    assert_eq!(
        parser.parse_peek(""),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &"",
            ErrorKind::Fail
        )))
    );
}

#[test]
fn seq_tuple_struct_trailing_comma_elided() {
    #![allow(dead_code)]

    #[derive(Debug, PartialEq)]
    struct Point(u32, u32);

    fn parser(input: &mut &str) -> PResult<Point> {
        seq! {
            Point(
                dec_uint,
                _: ',',
                dec_uint,
                _: empty,
            )
        }
        .parse_next(input)
    }
}

#[test]
fn seq_tuple_struct_no_trailing_comma() {
    #![allow(dead_code)]

    #[derive(Debug, PartialEq)]
    struct Point(u32, u32);

    fn parser(input: &mut &str) -> PResult<Point> {
        seq! {
            Point(
                dec_uint,
                _: ',',
                dec_uint
            )
        }
        .parse_next(input)
    }
}

#[test]
fn seq_tuple_struct_no_trailing_comma_elided() {
    #![allow(dead_code)]

    #[derive(Debug, PartialEq)]
    struct Point(u32, u32);

    fn parser(input: &mut &str) -> PResult<Point> {
        seq! {
            Point(
                dec_uint,
                _: ',',
                dec_uint,
                _: empty
            )
        }
        .parse_next(input)
    }
}

#[test]
fn seq_tuple_basics() {
    fn parser(input: &mut &str) -> PResult<(u32, u32)> {
        seq! {
            (
                dec_uint,
                _: ',',
                dec_uint,
            )
        }
        .parse_next(input)
    }
    assert_eq!(
        parser.parse_peek("123,4 remaining"),
        Ok((" remaining", (123, 4),)),
    );
    assert_eq!(
        parser.parse_peek("123, remaining"),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &" remaining",
            ErrorKind::Fail
        )))
    );
    assert_eq!(
        parser.parse_peek(""),
        Err(ErrMode::Backtrack(ParserError::from_error_kind(
            &"",
            ErrorKind::Fail
        )))
    );
}

#[test]
fn seq_tuple_trailing_comma_elided() {
    #![allow(dead_code)]

    fn parser(input: &mut &str) -> PResult<(u32, u32)> {
        seq! {
            (
                dec_uint,
                _: ',',
                dec_uint,
                _: empty,
            )
        }
        .parse_next(input)
    }
}

#[test]
fn seq_tuple_no_trailing_comma() {
    #![allow(dead_code)]

    fn parser(input: &mut &str) -> PResult<(u32, u32)> {
        seq! {
            (
                dec_uint,
                _: ',',
                dec_uint
            )
        }
        .parse_next(input)
    }
}

#[test]
fn seq_tuple_no_trailing_comma_elided() {
    #![allow(dead_code)]

    fn parser(input: &mut &str) -> PResult<(u32, u32)> {
        seq! {
            (
                dec_uint,
                _: ',',
                dec_uint,
                _: empty
            )
        }
        .parse_next(input)
    }
}

#[test]
fn seq_tuple_no_parens() {
    #![allow(dead_code)]

    fn parser(input: &mut &str) -> PResult<(u32, u32)> {
        seq! (
            dec_uint,
            _: ',',
            dec_uint,
        )
        .parse_next(input)
    }
}
