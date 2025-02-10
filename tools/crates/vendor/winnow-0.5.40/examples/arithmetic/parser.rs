use std::str::FromStr;

use winnow::prelude::*;
use winnow::{
    ascii::{digit1 as digits, multispace0 as multispaces},
    combinator::alt,
    combinator::delimited,
    combinator::repeat,
    token::one_of,
};

// Parser definition

pub fn expr(i: &mut &str) -> PResult<i64> {
    let init = term.parse_next(i)?;

    repeat(0.., (one_of(['+', '-']), term))
        .fold(
            move || init,
            |acc, (op, val): (char, i64)| {
                if op == '+' {
                    acc + val
                } else {
                    acc - val
                }
            },
        )
        .parse_next(i)
}

// We read an initial factor and for each time we find
// a * or / operator followed by another factor, we do
// the math by folding everything
fn term(i: &mut &str) -> PResult<i64> {
    let init = factor.parse_next(i)?;

    repeat(0.., (one_of(['*', '/']), factor))
        .fold(
            move || init,
            |acc, (op, val): (char, i64)| {
                if op == '*' {
                    acc * val
                } else {
                    acc / val
                }
            },
        )
        .parse_next(i)
}

// We transform an integer string into a i64, ignoring surrounding whitespace
// We look for a digit suite, and try to convert it.
// If either str::from_utf8 or FromStr::from_str fail,
// we fallback to the parens parser defined above
fn factor(i: &mut &str) -> PResult<i64> {
    delimited(
        multispaces,
        alt((digits.try_map(FromStr::from_str), parens)),
        multispaces,
    )
    .parse_next(i)
}

// We parse any expr surrounded by parens, ignoring all whitespace around those
fn parens(i: &mut &str) -> PResult<i64> {
    delimited('(', expr, ')').parse_next(i)
}

#[test]
fn factor_test() {
    let input = "3";
    let expected = Ok(("", 3));
    assert_eq!(factor.parse_peek(input), expected);

    let input = " 12";
    let expected = Ok(("", 12));
    assert_eq!(factor.parse_peek(input), expected);

    let input = "537 ";
    let expected = Ok(("", 537));
    assert_eq!(factor.parse_peek(input), expected);

    let input = "  24     ";
    let expected = Ok(("", 24));
    assert_eq!(factor.parse_peek(input), expected);
}

#[test]
fn term_test() {
    let input = " 12 *2 /  3";
    let expected = Ok(("", 8));
    assert_eq!(term.parse_peek(input), expected);

    let input = " 12 *2 /  3";
    let expected = Ok(("", 8));
    assert_eq!(term.parse_peek(input), expected);

    let input = " 2* 3  *2 *2 /  3";
    let expected = Ok(("", 8));
    assert_eq!(term.parse_peek(input), expected);

    let input = " 48 /  3/2";
    let expected = Ok(("", 8));
    assert_eq!(term.parse_peek(input), expected);
}

#[test]
fn expr_test() {
    let input = " 1 +  2 ";
    let expected = Ok(("", 3));
    assert_eq!(expr.parse_peek(input), expected);

    let input = " 12 + 6 - 4+  3";
    let expected = Ok(("", 17));
    assert_eq!(expr.parse_peek(input), expected);

    let input = " 1 + 2*3 + 4";
    let expected = Ok(("", 11));
    assert_eq!(expr.parse_peek(input), expected);
}

#[test]
fn parens_test() {
    let input = " (  2 )";
    let expected = Ok(("", 2));
    assert_eq!(expr.parse_peek(input), expected);

    let input = " 2* (  3 + 4 ) ";
    let expected = Ok(("", 14));
    assert_eq!(expr.parse_peek(input), expected);

    let input = "  2*2 / ( 5 - 1) + 3";
    let expected = Ok(("", 4));
    assert_eq!(expr.parse_peek(input), expected);
}
