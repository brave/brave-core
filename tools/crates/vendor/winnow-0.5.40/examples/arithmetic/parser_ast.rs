use std::fmt;
use std::fmt::{Debug, Display, Formatter};

use std::str::FromStr;

use winnow::prelude::*;
use winnow::{
    ascii::{digit1 as digits, multispace0 as multispaces},
    combinator::alt,
    combinator::delimited,
    combinator::repeat,
    token::one_of,
};

#[derive(Debug, Clone)]
pub enum Expr {
    Value(i64),
    Add(Box<Expr>, Box<Expr>),
    Sub(Box<Expr>, Box<Expr>),
    Mul(Box<Expr>, Box<Expr>),
    Div(Box<Expr>, Box<Expr>),
    Paren(Box<Expr>),
}

impl Expr {
    pub fn eval(&self) -> i64 {
        match self {
            Self::Value(v) => *v,
            Self::Add(lhs, rhs) => lhs.eval() + rhs.eval(),
            Self::Sub(lhs, rhs) => lhs.eval() - rhs.eval(),
            Self::Mul(lhs, rhs) => lhs.eval() * rhs.eval(),
            Self::Div(lhs, rhs) => lhs.eval() / rhs.eval(),
            Self::Paren(expr) => expr.eval(),
        }
    }
}

impl Display for Expr {
    fn fmt(&self, format: &mut Formatter<'_>) -> fmt::Result {
        use Expr::{Add, Div, Mul, Paren, Sub, Value};
        match *self {
            Value(val) => write!(format, "{}", val),
            Add(ref left, ref right) => write!(format, "{} + {}", left, right),
            Sub(ref left, ref right) => write!(format, "{} - {}", left, right),
            Mul(ref left, ref right) => write!(format, "{} * {}", left, right),
            Div(ref left, ref right) => write!(format, "{} / {}", left, right),
            Paren(ref expr) => write!(format, "({})", expr),
        }
    }
}

pub fn expr(i: &mut &str) -> PResult<Expr> {
    let init = term.parse_next(i)?;

    repeat(0.., (one_of(['+', '-']), term))
        .fold(
            move || init.clone(),
            |acc, (op, val): (char, Expr)| {
                if op == '+' {
                    Expr::Add(Box::new(acc), Box::new(val))
                } else {
                    Expr::Sub(Box::new(acc), Box::new(val))
                }
            },
        )
        .parse_next(i)
}

fn term(i: &mut &str) -> PResult<Expr> {
    let init = factor.parse_next(i)?;

    repeat(0.., (one_of(['*', '/']), factor))
        .fold(
            move || init.clone(),
            |acc, (op, val): (char, Expr)| {
                if op == '*' {
                    Expr::Mul(Box::new(acc), Box::new(val))
                } else {
                    Expr::Div(Box::new(acc), Box::new(val))
                }
            },
        )
        .parse_next(i)
}

fn factor(i: &mut &str) -> PResult<Expr> {
    delimited(
        multispaces,
        alt((digits.try_map(FromStr::from_str).map(Expr::Value), parens)),
        multispaces,
    )
    .parse_next(i)
}

fn parens(i: &mut &str) -> PResult<Expr> {
    delimited("(", expr, ")")
        .map(|e| Expr::Paren(Box::new(e)))
        .parse_next(i)
}

#[test]
fn factor_test() {
    let input = "3";
    let expected = Ok(("", String::from("Value(3)")));
    assert_eq!(factor.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = " 12";
    let expected = Ok(("", String::from("Value(12)")));
    assert_eq!(factor.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = "537 ";
    let expected = Ok(("", String::from("Value(537)")));
    assert_eq!(factor.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = "  24     ";
    let expected = Ok(("", String::from("Value(24)")));
    assert_eq!(factor.map(|e| format!("{e:?}")).parse_peek(input), expected);
}

#[test]
fn term_test() {
    let input = " 12 *2 /  3";
    let expected = Ok(("", String::from("Div(Mul(Value(12), Value(2)), Value(3))")));
    assert_eq!(term.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = " 12 *2 /  3";
    let expected = Ok(("", String::from("Div(Mul(Value(12), Value(2)), Value(3))")));
    assert_eq!(term.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = " 2* 3  *2 *2 /  3";
    let expected = Ok((
        "",
        String::from("Div(Mul(Mul(Mul(Value(2), Value(3)), Value(2)), Value(2)), Value(3))"),
    ));
    assert_eq!(term.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = " 48 /  3/2";
    let expected = Ok(("", String::from("Div(Div(Value(48), Value(3)), Value(2))")));
    assert_eq!(term.map(|e| format!("{e:?}")).parse_peek(input), expected);
}

#[test]
fn expr_test() {
    let input = " 1 +  2 ";
    let expected = Ok(("", String::from("Add(Value(1), Value(2))")));
    assert_eq!(expr.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = " 12 + 6 - 4+  3";
    let expected = Ok((
        "",
        String::from("Add(Sub(Add(Value(12), Value(6)), Value(4)), Value(3))"),
    ));
    assert_eq!(expr.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = " 1 + 2*3 + 4";
    let expected = Ok((
        "",
        String::from("Add(Add(Value(1), Mul(Value(2), Value(3))), Value(4))"),
    ));
    assert_eq!(expr.map(|e| format!("{e:?}")).parse_peek(input), expected);
}

#[test]
fn parens_test() {
    let input = " (  2 )";
    let expected = Ok(("", String::from("Paren(Value(2))")));
    assert_eq!(expr.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = " 2* (  3 + 4 ) ";
    let expected = Ok((
        "",
        String::from("Mul(Value(2), Paren(Add(Value(3), Value(4))))"),
    ));
    assert_eq!(expr.map(|e| format!("{e:?}")).parse_peek(input), expected);

    let input = "  2*2 / ( 5 - 1) + 3";
    let expected = Ok((
        "",
        String::from("Add(Div(Mul(Value(2), Value(2)), Paren(Sub(Value(5), Value(1)))), Value(3))"),
    ));
    assert_eq!(expr.map(|e| format!("{e:?}")).parse_peek(input), expected);
}
