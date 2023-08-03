//! Invalid variant error

use core::fmt;

/// An error type indicating that a [`FromStr`](core::str::FromStr) call failed because the value
/// was not a valid variant.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct InvalidVariant;

impl fmt::Display for InvalidVariant {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "value was not a valid variant")
    }
}

#[cfg(feature = "std")]
impl std::error::Error for InvalidVariant {}

impl From<InvalidVariant> for crate::Error {
    fn from(err: InvalidVariant) -> Self {
        Self::InvalidVariant(err)
    }
}

impl TryFrom<crate::Error> for InvalidVariant {
    type Error = crate::error::DifferentVariant;

    fn try_from(err: crate::Error) -> Result<Self, Self::Error> {
        match err {
            crate::Error::InvalidVariant(err) => Ok(err),
            _ => Err(crate::error::DifferentVariant),
        }
    }
}
