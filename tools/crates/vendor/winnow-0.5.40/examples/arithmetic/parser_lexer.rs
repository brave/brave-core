use std::fmt;
use std::fmt::{Debug, Display, Formatter};

use std::str::FromStr;

use winnow::prelude::*;
use winnow::{
    ascii::{digit1 as digits, multispace0 as multispaces},
    combinator::alt,
    combinator::dispatch,
    combinator::fail,
    combinator::peek,
    combinator::repeat,
    combinator::{delimited, preceded, terminated},
    token::any,
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

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum Token {
    Value(i64),
    Oper(Oper),
    OpenParen,
    CloseParen,
}

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum Oper {
    Add,
    Sub,
    Mul,
    Div,
}

impl winnow::stream::ContainsToken<Token> for Token {
    #[inline(always)]
    fn contains_token(&self, token: Token) -> bool {
        *self == token
    }
}

impl winnow::stream::ContainsToken<Token> for &'_ [Token] {
    #[inline]
    fn contains_token(&self, token: Token) -> bool {
        self.iter().any(|t| *t == token)
    }
}

impl<const LEN: usize> winnow::stream::ContainsToken<Token> for &'_ [Token; LEN] {
    #[inline]
    fn contains_token(&self, token: Token) -> bool {
        self.iter().any(|t| *t == token)
    }
}

impl<const LEN: usize> winnow::stream::ContainsToken<Token> for [Token; LEN] {
    #[inline]
    fn contains_token(&self, token: Token) -> bool {
        self.iter().any(|t| *t == token)
    }
}

#[allow(dead_code)]
pub fn expr2(i: &mut &str) -> PResult<Expr> {
    let tokens = lex.parse_next(i)?;
    expr.parse_next(&mut tokens.as_slice())
}

pub fn lex(i: &mut &str) -> PResult<Vec<Token>> {
    preceded(multispaces, repeat(1.., terminated(token, multispaces))).parse_next(i)
}

fn token(i: &mut &str) -> PResult<Token> {
    dispatch! {peek(any);
        '0'..='9' => digits.try_map(FromStr::from_str).map(Token::Value),
        '(' => '('.value(Token::OpenParen),
        ')' => ')'.value(Token::CloseParen),
        '+' => '+'.value(Token::Oper(Oper::Add)),
        '-' => '-'.value(Token::Oper(Oper::Sub)),
        '*' => '*'.value(Token::Oper(Oper::Mul)),
        '/' => '/'.value(Token::Oper(Oper::Div)),
        _ => fail,
    }
    .parse_next(i)
}

pub fn expr(i: &mut &[Token]) -> PResult<Expr> {
    let init = term.parse_next(i)?;

    repeat(
        0..,
        (
            one_of([Token::Oper(Oper::Add), Token::Oper(Oper::Sub)]),
            term,
        ),
    )
    .fold(
        move || init.clone(),
        |acc, (op, val): (Token, Expr)| {
            if op == Token::Oper(Oper::Add) {
                Expr::Add(Box::new(acc), Box::new(val))
            } else {
                Expr::Sub(Box::new(acc), Box::new(val))
            }
        },
    )
    .parse_next(i)
}

fn term(i: &mut &[Token]) -> PResult<Expr> {
    let init = factor.parse_next(i)?;

    repeat(
        0..,
        (
            one_of([Token::Oper(Oper::Mul), Token::Oper(Oper::Div)]),
            factor,
        ),
    )
    .fold(
        move || init.clone(),
        |acc, (op, val): (Token, Expr)| {
            if op == Token::Oper(Oper::Mul) {
                Expr::Mul(Box::new(acc), Box::new(val))
            } else {
                Expr::Div(Box::new(acc), Box::new(val))
            }
        },
    )
    .parse_next(i)
}

fn factor(i: &mut &[Token]) -> PResult<Expr> {
    alt((
        one_of(|t| matches!(t, Token::Value(_))).map(|t| match t {
            Token::Value(v) => Expr::Value(v),
            _ => unreachable!(),
        }),
        parens,
    ))
    .parse_next(i)
}

fn parens(i: &mut &[Token]) -> PResult<Expr> {
    delimited(one_of(Token::OpenParen), expr, one_of(Token::CloseParen))
        .map(|e| Expr::Paren(Box::new(e)))
        .parse_next(i)
}

#[test]
fn lex_test() {
    let input = "3";
    let expected = Ok(String::from(r#"("", [Value(3)])"#));
    assert_eq!(lex.parse_peek(input).map(|e| format!("{e:?}")), expected);

    let input = "  24     ";
    let expected = Ok(String::from(r#"("", [Value(24)])"#));
    assert_eq!(lex.parse_peek(input).map(|e| format!("{e:?}")), expected);

    let input = " 12 *2 /  3";
    let expected = Ok(String::from(
        r#"("", [Value(12), Oper(Mul), Value(2), Oper(Div), Value(3)])"#,
    ));
    assert_eq!(lex.parse_peek(input).map(|e| format!("{e:?}")), expected);

    let input = "  2*2 / ( 5 - 1) + 3";
    let expected = Ok(String::from(
        r#"("", [Value(2), Oper(Mul), Value(2), Oper(Div), OpenParen, Value(5), Oper(Sub), Value(1), CloseParen, Oper(Add), Value(3)])"#,
    ));
    assert_eq!(lex.parse_peek(input).map(|e| format!("{e:?}")), expected);
}

#[test]
fn factor_test() {
    let input = "3";
    let expected = Ok(String::from("Value(3)"));
    let input = lex.parse(input).unwrap();
    assert_eq!(factor.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = " 12";
    let expected = Ok(String::from("Value(12)"));
    let input = lex.parse(input).unwrap();
    assert_eq!(factor.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = "537 ";
    let expected = Ok(String::from("Value(537)"));
    let input = lex.parse(input).unwrap();
    assert_eq!(factor.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = "  24     ";
    let expected = Ok(String::from("Value(24)"));
    let input = lex.parse(input).unwrap();
    assert_eq!(factor.map(|e| format!("{e:?}")).parse(&input), expected);
}

#[test]
fn term_test() {
    let input = " 12 *2 /  3";
    let expected = Ok(String::from("Div(Mul(Value(12), Value(2)), Value(3))"));
    let input = lex.parse(input).unwrap();
    assert_eq!(term.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = " 12 *2 /  3";
    let expected = Ok(String::from("Div(Mul(Value(12), Value(2)), Value(3))"));
    let input = lex.parse(input).unwrap();
    assert_eq!(term.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = " 2* 3  *2 *2 /  3";
    let expected = Ok(String::from(
        "Div(Mul(Mul(Mul(Value(2), Value(3)), Value(2)), Value(2)), Value(3))",
    ));
    let input = lex.parse(input).unwrap();
    assert_eq!(term.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = " 48 /  3/2";
    let expected = Ok(String::from("Div(Div(Value(48), Value(3)), Value(2))"));
    let input = lex.parse(input).unwrap();
    assert_eq!(term.map(|e| format!("{e:?}")).parse(&input), expected);
}

#[test]
fn expr_test() {
    let input = " 1 +  2 ";
    let expected = Ok(String::from("Add(Value(1), Value(2))"));
    let input = lex.parse(input).unwrap();
    assert_eq!(expr.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = " 12 + 6 - 4+  3";
    let expected = Ok(String::from(
        "Add(Sub(Add(Value(12), Value(6)), Value(4)), Value(3))",
    ));
    let input = lex.parse(input).unwrap();
    assert_eq!(expr.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = " 1 + 2*3 + 4";
    let expected = Ok(String::from(
        "Add(Add(Value(1), Mul(Value(2), Value(3))), Value(4))",
    ));
    let input = lex.parse(input).unwrap();
    assert_eq!(expr.map(|e| format!("{e:?}")).parse(&input), expected);
}

#[test]
fn parens_test() {
    let input = " (  2 )";
    let expected = Ok(String::from("Paren(Value(2))"));
    let input = lex.parse(input).unwrap();
    assert_eq!(expr.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = " 2* (  3 + 4 ) ";
    let expected = Ok(String::from(
        "Mul(Value(2), Paren(Add(Value(3), Value(4))))",
    ));
    let input = lex.parse(input).unwrap();
    assert_eq!(expr.map(|e| format!("{e:?}")).parse(&input), expected);

    let input = "  2*2 / ( 5 - 1) + 3";
    let expected = Ok(String::from(
        "Add(Div(Mul(Value(2), Value(2)), Paren(Sub(Value(5), Value(1)))), Value(3))",
    ));
    let input = lex.parse(input).unwrap();
    assert_eq!(expr.map(|e| format!("{e:?}")).parse(&input), expected);
}
