#![allow(unknown_lints)] // non_local_definitions isn't in Rust 1.70
#![allow(non_local_definitions)]
//! Error types that can be emitted from this library

use displaydoc::Display;
use thiserror::Error;

use std::error::Error;
use std::fmt;
use std::io;
use std::num::TryFromIntError;
use std::string::FromUtf8Error;

/// Generic result type with ZipError as its error variant
pub type ZipResult<T> = Result<T, ZipError>;

/// Error type for Zip
#[derive(Debug, Display, Error)]
#[non_exhaustive]
pub enum ZipError {
    /// i/o error: {0}
    Io(#[from] io::Error),

    /// invalid Zip archive: {0}
    InvalidArchive(&'static str),

    /// unsupported Zip archive: {0}
    UnsupportedArchive(&'static str),

    /// specified file not found in archive
    FileNotFound,

    /// The password provided is incorrect
    InvalidPassword,
}

impl ZipError {
    /// The text used as an error when a password is required and not supplied
    ///
    /// ```rust,no_run
    /// # use zip::result::ZipError;
    /// # let mut archive = zip::ZipArchive::new(std::io::Cursor::new(&[])).unwrap();
    /// match archive.by_index(1) {
    ///     Err(ZipError::UnsupportedArchive(ZipError::PASSWORD_REQUIRED)) => eprintln!("a password is needed to unzip this file"),
    ///     _ => (),
    /// }
    /// # ()
    /// ```
    pub const PASSWORD_REQUIRED: &'static str = "Password required to decrypt file";
}

impl From<ZipError> for io::Error {
    fn from(err: ZipError) -> io::Error {
        let kind = match &err {
            ZipError::Io(err) => err.kind(),
            ZipError::InvalidArchive(_) => io::ErrorKind::InvalidData,
            ZipError::UnsupportedArchive(_) => io::ErrorKind::Unsupported,
            ZipError::FileNotFound => io::ErrorKind::NotFound,
            ZipError::InvalidPassword => io::ErrorKind::InvalidInput,
        };

        io::Error::new(kind, err)
    }
}

impl From<DateTimeRangeError> for ZipError {
    fn from(_: DateTimeRangeError) -> Self {
        ZipError::InvalidArchive("Invalid date or time")
    }
}

impl From<FromUtf8Error> for ZipError {
    fn from(_: FromUtf8Error) -> Self {
        ZipError::InvalidArchive("Invalid UTF-8")
    }
}

/// Error type for time parsing
#[derive(Debug)]
pub struct DateTimeRangeError;

// TryFromIntError is also an out-of-range error.
impl From<TryFromIntError> for DateTimeRangeError {
    fn from(_value: TryFromIntError) -> Self {
        DateTimeRangeError
    }
}

impl fmt::Display for DateTimeRangeError {
    fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        write!(
            fmt,
            "a date could not be represented within the bounds the MS-DOS date range (1980-2107)"
        )
    }
}

impl Error for DateTimeRangeError {}
