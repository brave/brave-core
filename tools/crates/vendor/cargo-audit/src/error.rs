//! Error types

use std::{error::Error as ErrorTrait, io};
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
    /// Create an error from this kind with the given source
    pub fn context(self, source: impl Into<BoxError>) -> Error {
        Error {
            kind: self,
            source: source.into(),
        }
    }
}

/// Error type
#[derive(Debug, Error)]
#[error("{kind}: {source}")]
pub struct Error {
    /// Kind of error
    kind: ErrorKind,

    /// Source of the error
    source: BoxError,
}

impl Error {
    /// Get the kind of error that occurred
    pub fn kind(&self) -> ErrorKind {
        self.kind
    }
}

impl From<io::Error> for Error {
    fn from(other: io::Error) -> Self {
        ErrorKind::Io.context(other)
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
    }
}

impl From<cargo_lock::Error> for Error {
    fn from(err: cargo_lock::Error) -> Self {
        match err {
            cargo_lock::Error::Io(_) => ErrorKind::Io,
            cargo_lock::Error::Parse(_) => ErrorKind::Parse,
            cargo_lock::Error::Version(_) => ErrorKind::Version,
            _ => ErrorKind::Other,
        }
        .context(err)
    }
}

/// Displays the error and also follows the chain of the `.source` fields,
/// printing any errors that caused the top-level error.
///
/// This is required to properly present some `gix` errors to the user:
/// <https://github.com/rustsec/rustsec/issues/1029#issuecomment-1777487808>
pub fn display_err_with_source<E: ErrorTrait>(error: &E) -> String {
    display_error_chain::DisplayErrorChain::new(error).to_string()
}

/// Box containing a thread-safe + `'static` error suitable for use as a source
pub type BoxError = Box<dyn ErrorTrait + Send + Sync + 'static>;
