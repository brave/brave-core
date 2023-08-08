//! Error types

use abscissa_core::error::{BoxError, Context};
use std::{
    fmt::{self, Display},
    io,
    ops::Deref,
};
use thiserror::Error;

/// Kinds of errors
#[derive(Copy, Clone, Debug, Eq, Error, PartialEq)]
pub enum ErrorKind {
    /// Error in configuration file
    #[error("config error")]
    Config,

    /// Input/output error
    #[error("I/O error")]
    Io,

    /// Parse errors
    #[error("parse error")]
    Parse,

    /// Repository errors
    #[error("git repo error")]
    Repo,

    /// Version errors
    #[error("version error")]
    Version,

    /// Other kinds of errors
    #[error("other error")]
    Other,
}

impl ErrorKind {
    /// Create an error context from this error
    pub fn context(self, source: impl Into<BoxError>) -> Context<ErrorKind> {
        Context::new(self, Some(source.into()))
    }
}

/// Error type
#[derive(Debug)]
pub struct Error(Box<Context<ErrorKind>>);

impl Error {
    /// Get the kind of error that occurred
    pub fn kind(&self) -> ErrorKind {
        *self.0.kind()
    }
}

impl Deref for Error {
    type Target = Context<ErrorKind>;

    fn deref(&self) -> &Context<ErrorKind> {
        &self.0
    }
}

impl Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0.fmt(f)
    }
}

impl From<Context<ErrorKind>> for Error {
    fn from(context: Context<ErrorKind>) -> Self {
        Error(Box::new(context))
    }
}

impl From<io::Error> for Error {
    fn from(other: io::Error) -> Self {
        ErrorKind::Io.context(other).into()
    }
}

impl From<rustsec::Error> for Error {
    fn from(err: rustsec::Error) -> Self {
        match err.kind() {
            rustsec::ErrorKind::Io => ErrorKind::Io,
            rustsec::ErrorKind::Parse => ErrorKind::Parse,
            rustsec::ErrorKind::Repo => ErrorKind::Repo,
            rustsec::ErrorKind::Version => ErrorKind::Version,
            _ => ErrorKind::Other,
        }
        .context(err)
        .into()
    }
}

impl From<rustsec::cargo_lock::Error> for Error {
    fn from(err: rustsec::cargo_lock::Error) -> Self {
        match err {
            rustsec::cargo_lock::Error::Io(_) => ErrorKind::Io,
            rustsec::cargo_lock::Error::Parse(_) => ErrorKind::Parse,
            rustsec::cargo_lock::Error::Version(_) => ErrorKind::Version,
            _ => ErrorKind::Other,
        }
        .context(err)
        .into()
    }
}

impl std::error::Error for Error {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        self.0.source()
    }
}
