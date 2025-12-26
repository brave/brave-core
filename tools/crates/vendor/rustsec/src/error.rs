//! Error types used by this crate

use std::{
    fmt::{self, Display},
    io,
    str::Utf8Error,
    sync::Arc,
};
use thiserror::Error;

/// Create and return an error with a formatted message
macro_rules! fail {
    ($kind:path, $msg:expr) => {
        return Err(crate::error::Error::new($kind, $msg))
    };
    ($kind:path, $fmt:expr, $($arg:tt)+) => {
        fail!($kind, &format!($fmt, $($arg)+))
    };
}

/// Result alias with the `rustsec` crate's `Error` type.
pub type Result<T> = std::result::Result<T, Error>;

/// Error type
#[derive(Clone, Debug)]
pub struct Error {
    /// Kind of error
    kind: ErrorKind,

    /// Message providing a more specific explanation than `self.kind`.
    ///
    /// This may be a complete error by itself, or it may provide context for `self.source`.
    msg: String,

    /// Cause of this error.
    ///
    /// The specific type of this error should not be considered part of the stable interface of
    /// this crate.
    source: Option<Arc<dyn std::error::Error + Send + Sync>>,
}

impl Error {
    /// Creates a new [`Error`](struct@Error) with the given description.
    ///
    /// Do not use this for wrapping [`std::error::Error`]; use [`Error::with_source()`] instead.
    pub fn new(kind: ErrorKind, description: impl Display) -> Self {
        // TODO: In a semver-breaking release, deprecate accepting anything but a `String`,
        // or maybe `AsRef<str>`. This will discourage putting error types in the `description`
        // position, which makes it impossible to retrieve their `.source()` info. It will also
        // avoid an unnecessary clone in the common case where `S` is already a `String`.
        Self {
            kind,
            msg: description.to_string(),
            source: None,
        }
    }

    pub(crate) fn from_source(
        kind: ErrorKind,
        source: impl std::error::Error + Send + Sync + 'static,
    ) -> Self {
        Self::with_source(kind, source.to_string(), source)
    }

    /// Creates a new [`Error`](struct@Error) whose [`std::error::Error::source()`] is `source`.
    ///
    /// `msg` should describe the operation which failed so as to give context for how `source`
    /// is a meaningful error. For example, if `source` is a [`std::io::Error`] from trying to
    /// read a file, then `msg` should include the path of the file and why the file is relevant.
    pub fn with_source<E: std::error::Error + Send + Sync + 'static>(
        kind: ErrorKind,
        msg: String,
        source: E,
    ) -> Self {
        Self {
            kind,
            msg,
            source: Some(Arc::new(source)),
        }
    }

    /// Obtain the inner `ErrorKind` for this error
    pub fn kind(&self) -> ErrorKind {
        self.kind
    }
}

impl Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}: {}", self.kind, self.msg)
    }
}

impl std::error::Error for Error {
    /// The lower-level source of this error, if any.
    ///
    /// The specific type of the returned error should not be considered part of the stable
    /// interface of this crate; prefer to use this only for displaying error information.
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match &self.source {
            Some(boxed_error) => Some(&**boxed_error),
            None => None,
        }
    }
}

/// Custom error type for this library
#[derive(Copy, Clone, Debug, Error, Eq, PartialEq)]
#[non_exhaustive]
pub enum ErrorKind {
    /// Invalid argument or parameter
    #[error("bad parameter")]
    BadParam,

    /// An error occurred performing an I/O operation (e.g. network, file)
    #[error("I/O operation failed")]
    Io,

    /// Not found
    #[error("not found")]
    NotFound,

    /// Unable to acquire filesystem lock
    #[error("unable to acquire filesystem lock")]
    LockTimeout,

    /// Couldn't parse response data
    #[error("parse error")]
    Parse,

    /// Registry-related error
    #[error("registry")]
    Registry,

    /// Git operation failed
    #[error("git operation failed")]
    Repo,

    /// Errors related to versions
    #[error("bad version")]
    Version,
}

impl From<Utf8Error> for Error {
    fn from(other: Utf8Error) -> Self {
        Self::from_source(ErrorKind::Parse, other)
    }
}

impl From<cargo_lock::Error> for Error {
    fn from(other: cargo_lock::Error) -> Self {
        Self::from_source(ErrorKind::Io, other)
    }
}

impl From<fmt::Error> for Error {
    fn from(other: fmt::Error) -> Self {
        Self::from_source(ErrorKind::Io, other)
    }
}

impl From<io::Error> for Error {
    fn from(other: io::Error) -> Self {
        Self::from_source(ErrorKind::Io, other)
    }
}

impl Error {
    /// Converts from [`tame_index::Error`] to our `Error`.
    ///
    /// This is a separate function instead of a `From` impl
    /// because a trait impl would leak into the public API,
    /// and we need to keep it private because `tame_index` semver
    /// will be bumped frequently and we don't want to bump `rustsec` semver
    /// every time it changes.
    #[cfg(feature = "git")]
    pub(crate) fn from_tame(err: tame_index::Error) -> Self {
        // Separate lock timeouts into their own LockTimeout variant.
        use tame_index::utils::flock::LockError;
        match err {
            tame_index::Error::Lock(lock_err) => match &lock_err.source {
                LockError::TimedOut | LockError::Contested => {
                    Self::from_source(ErrorKind::LockTimeout, lock_err)
                }
                _ => Self::from_source(ErrorKind::Io, lock_err),
            },
            other => Self::from_source(ErrorKind::Registry, other),
        }
    }

    /// Converts from [`toml::de::Error`] to our `Error`.
    ///
    /// This is used so rarely that there is no need to `impl From`,
    /// and this way we can avoid leaking it into the public API.
    pub(crate) fn from_toml(other: toml::de::Error) -> Self {
        Self::with_source(ErrorKind::Parse, other.to_string(), other)
    }
}
