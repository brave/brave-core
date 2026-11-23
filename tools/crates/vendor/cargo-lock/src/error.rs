//! Error types

use std::{fmt, io};

/// Result type with the `cargo-lock` crate's [`Error`] type.
pub type Result<T> = core::result::Result<T, Error>;

/// Error type.
#[derive(Debug)]
#[non_exhaustive]
pub enum Error {
    /// An error occurred performing an I/O operation (e.g. network, file)
    Io(io::ErrorKind),

    /// Couldn't parse response data
    Parse(String),

    /// Errors related to versions
    Version(semver::Error),

    /// Errors related to graph resolution
    Resolution(String),
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Io(kind) => write!(f, "I/O operation failed: {kind}"),
            Self::Parse(s) => write!(f, "parse error: {s}"),
            Self::Version(err) => write!(f, "version error: {err}"),
            Self::Resolution(err) => write!(f, "resolution error: {err}"),
        }
    }
}

impl From<io::Error> for Error {
    fn from(err: io::Error) -> Self {
        Self::Io(err.kind())
    }
}

impl From<semver::Error> for Error {
    fn from(err: semver::Error) -> Self {
        Self::Version(err)
    }
}

impl From<std::num::ParseIntError> for Error {
    fn from(err: std::num::ParseIntError) -> Self {
        Self::Parse(err.to_string())
    }
}

impl std::error::Error for Error {}
