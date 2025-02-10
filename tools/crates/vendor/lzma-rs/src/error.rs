//! Error handling.

use std::fmt::Display;
use std::{io, result};

/// Library errors.
#[derive(Debug)]
pub enum Error {
    /// I/O error.
    IoError(io::Error),
    /// Not enough bytes to complete header
    HeaderTooShort(io::Error),
    /// LZMA error.
    LzmaError(String),
    /// XZ error.
    XzError(String),
}

/// Library result alias.
pub type Result<T> = result::Result<T, Error>;

impl From<io::Error> for Error {
    fn from(e: io::Error) -> Error {
        Error::IoError(e)
    }
}

impl Display for Error {
    fn fmt(&self, fmt: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Error::IoError(e) => write!(fmt, "io error: {}", e),
            Error::HeaderTooShort(e) => write!(fmt, "header too short: {}", e),
            Error::LzmaError(e) => write!(fmt, "lzma error: {}", e),
            Error::XzError(e) => write!(fmt, "xz error: {}", e),
        }
    }
}

impl std::error::Error for Error {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            Error::IoError(e) | Error::HeaderTooShort(e) => Some(e),
            Error::LzmaError(_) | Error::XzError(_) => None,
        }
    }
}

#[cfg(test)]
mod test {
    use super::Error;

    #[test]
    fn test_display() {
        assert_eq!(
            Error::IoError(std::io::Error::new(
                std::io::ErrorKind::Other,
                "this is an error"
            ))
            .to_string(),
            "io error: this is an error"
        );
        assert_eq!(
            Error::LzmaError("this is an error".to_string()).to_string(),
            "lzma error: this is an error"
        );
        assert_eq!(
            Error::XzError("this is an error".to_string()).to_string(),
            "xz error: this is an error"
        );
    }
}
