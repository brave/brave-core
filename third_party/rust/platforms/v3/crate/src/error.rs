//! Error type

use core::fmt::{self, Display};

/// Error type
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Error;

impl Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("platforms error")
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}
