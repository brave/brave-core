use std::{fmt::{Debug, Display}, error::Error};

#[derive(Debug, Copy, Clone)]
pub enum ParseError {
    SymbolsSectionIsMissing,
    SectionIsMissing(&'static str),
    UnexpectedSectionType { expected: u32, actual: u32 },
    MalformedInput,
    UnexpectedEof,
}

impl Error for ParseError {}

impl Display for ParseError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ParseError::SymbolsSectionIsMissing => write!(f, "symbols section is missing"),
            ParseError::SectionIsMissing(name) => write!(f, "section {} is missing", name),
            ParseError::UnexpectedSectionType { expected, actual } =>
                write!(f, "expected section type {} but found {}", expected, actual),
            ParseError::MalformedInput => write!(f, "malformed input file"),
            ParseError::UnexpectedEof => write!(f, "unexpected end of file"),
        }
    }
}

impl From<std::num::TryFromIntError> for ParseError {
    fn from(_: std::num::TryFromIntError) -> Self {
        ParseError::MalformedInput
    }
}

impl From<crate::parser::UnexpectedEof> for ParseError {
    fn from(_: crate::parser::UnexpectedEof) -> Self {
        ParseError::UnexpectedEof
    }
}
