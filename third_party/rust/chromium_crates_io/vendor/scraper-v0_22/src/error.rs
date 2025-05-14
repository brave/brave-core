//! Custom error types for diagnostics
//! Includes re-exported error types from dependencies

mod utils;

use std::{error::Error, fmt::Display};

use cssparser::{BasicParseErrorKind, ParseErrorKind, Token};
use selectors::parser::SelectorParseErrorKind;

/// Error type that is returned when calling `Selector::parse`
#[derive(Debug, Clone)]
pub enum SelectorErrorKind<'a> {
    /// A `Token` was not expected
    UnexpectedToken(Token<'a>),

    /// End-Of-Line was unexpected
    EndOfLine,

    /// `@` rule is invalid
    InvalidAtRule(String),

    /// The body of an `@` rule is invalid
    InvalidAtRuleBody,

    /// The qualified rule is invalid
    QualRuleInvalid,

    /// Expected a `::` for a pseudoelement
    ExpectedColonOnPseudoElement(Token<'a>),

    /// Expected an identity for a pseudoelement
    ExpectedIdentityOnPseudoElement(Token<'a>),

    /// A `SelectorParseErrorKind` error that isn't really supposed to happen did
    UnexpectedSelectorParseError(SelectorParseErrorKind<'a>),
}

impl<'a> From<cssparser::ParseError<'a, SelectorParseErrorKind<'a>>> for SelectorErrorKind<'a> {
    fn from(original: cssparser::ParseError<'a, SelectorParseErrorKind<'a>>) -> Self {
        // NOTE: This could be improved, but I dont
        // exactly know how
        match original.kind {
            ParseErrorKind::Basic(err) => SelectorErrorKind::from(err),
            ParseErrorKind::Custom(err) => SelectorErrorKind::from(err),
        }
    }
}

impl<'a> From<BasicParseErrorKind<'a>> for SelectorErrorKind<'a> {
    fn from(err: BasicParseErrorKind<'a>) -> Self {
        match err {
            BasicParseErrorKind::UnexpectedToken(token) => Self::UnexpectedToken(token),
            BasicParseErrorKind::EndOfInput => Self::EndOfLine,
            BasicParseErrorKind::AtRuleInvalid(rule) => Self::InvalidAtRule(rule.to_string()),
            BasicParseErrorKind::AtRuleBodyInvalid => Self::InvalidAtRuleBody,
            BasicParseErrorKind::QualifiedRuleInvalid => Self::QualRuleInvalid,
        }
    }
}

impl<'a> From<SelectorParseErrorKind<'a>> for SelectorErrorKind<'a> {
    fn from(err: SelectorParseErrorKind<'a>) -> Self {
        match err {
            SelectorParseErrorKind::PseudoElementExpectedColon(token) => {
                Self::ExpectedColonOnPseudoElement(token)
            }
            SelectorParseErrorKind::PseudoElementExpectedIdent(token) => {
                Self::ExpectedIdentityOnPseudoElement(token)
            }
            other => Self::UnexpectedSelectorParseError(other),
        }
    }
}

impl Display for SelectorErrorKind<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{}",
            match self {
                Self::UnexpectedToken(token) => {
                    format!("Token {:?} was not expected", utils::render_token(token))
                }
                Self::EndOfLine => "Unexpected EOL".to_string(),
                Self::InvalidAtRule(rule) => format!("Invalid @-rule {:?}", rule),
                Self::InvalidAtRuleBody => "The body of an @-rule was invalid".to_string(),
                Self::QualRuleInvalid => "The qualified name was invalid".to_string(),
                Self::ExpectedColonOnPseudoElement(token) => format!(
                    "Expected a ':' token for pseudoelement, got {:?} instead",
                    utils::render_token(token)
                ),
                Self::ExpectedIdentityOnPseudoElement(token) => format!(
                    "Expected identity for pseudoelement, got {:?} instead",
                    utils::render_token(token)
                ),
                Self::UnexpectedSelectorParseError(err) => format!(
                    "Unexpected error occurred. Please report this to the developer\n{:#?}",
                    err
                ),
            }
        )
    }
}

impl Error for SelectorErrorKind<'_> {
    fn description(&self) -> &str {
        match self {
            Self::UnexpectedToken(_) => "Token was not expected",
            Self::EndOfLine => "Unexpected EOL",
            Self::InvalidAtRule(_) => "Invalid @-rule",
            Self::InvalidAtRuleBody => "The body of an @-rule was invalid",
            Self::QualRuleInvalid => "The qualified name was invalid",
            Self::ExpectedColonOnPseudoElement(_) => "Missing colon character on pseudoelement",
            Self::ExpectedIdentityOnPseudoElement(_) => "Missing pseudoelement identity",
            Self::UnexpectedSelectorParseError(_) => "Unexpected error",
        }
    }
}
