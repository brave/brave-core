#[cfg(not(feature = "std"))]
use core2::{error::Error as StdError, io::Error as IoError};
#[cfg(feature = "std")]
use std::{error::Error as StdError, io::Error as IoError};

use unsigned_varint::decode::Error as DecodeError;
#[cfg(feature = "std")]
use unsigned_varint::io::ReadError;

/// Multihash error.
#[derive(Debug)]
pub enum Error {
    /// Io error.
    Io(IoError),
    /// Unsupported multihash code.
    UnsupportedCode(u64),
    /// Invalid multihash size.
    InvalidSize(u64),
    /// Invalid varint.
    Varint(DecodeError),
}

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match self {
            Self::Io(err) => write!(f, "{}", err),
            Self::UnsupportedCode(code) => write!(f, "Unsupported multihash code {}.", code),
            Self::InvalidSize(size) => write!(f, "Invalid multihash size {}.", size),
            Self::Varint(err) => write!(f, "{}", err),
        }
    }
}

impl StdError for Error {}

impl From<IoError> for Error {
    fn from(err: IoError) -> Self {
        Self::Io(err)
    }
}

#[cfg(feature = "std")]
impl From<ReadError> for Error {
    fn from(err: ReadError) -> Self {
        match err {
            ReadError::Io(err) => Self::Io(err),
            ReadError::Decode(err) => Self::Varint(err),
            _ => unreachable!(),
        }
    }
}

/// Multihash result.
pub type Result<T> = core::result::Result<T, Error>;
