//! Error that occurred at some stage of parsing

use core::fmt;

use crate::error::{self, ParseFromDescription, TryFromParsed};
use crate::internal_macros::bug;

/// An error that occurred at some stage of parsing.
#[allow(variant_size_differences)]
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Parse {
    #[allow(missing_docs)]
    TryFromParsed(TryFromParsed),
    #[allow(missing_docs)]
    ParseFromDescription(ParseFromDescription),
    /// The input should have ended, but there were characters remaining.
    #[non_exhaustive]
    #[deprecated(
        since = "0.3.28",
        note = "no longer output. moved to the `ParseFromDescription` variant"
    )]
    UnexpectedTrailingCharacters,
}

impl fmt::Display for Parse {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::TryFromParsed(err) => err.fmt(f),
            Self::ParseFromDescription(err) => err.fmt(f),
            #[allow(deprecated)]
            Self::UnexpectedTrailingCharacters => bug!("variant should not be used"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Parse {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            Self::TryFromParsed(err) => Some(err),
            Self::ParseFromDescription(err) => Some(err),
            #[allow(deprecated)]
            Self::UnexpectedTrailingCharacters => bug!("variant should not be used"),
        }
    }
}

impl From<TryFromParsed> for Parse {
    fn from(err: TryFromParsed) -> Self {
        Self::TryFromParsed(err)
    }
}

impl TryFrom<Parse> for TryFromParsed {
    type Error = error::DifferentVariant;

    fn try_from(err: Parse) -> Result<Self, Self::Error> {
        match err {
            Parse::TryFromParsed(err) => Ok(err),
            _ => Err(error::DifferentVariant),
        }
    }
}

impl From<ParseFromDescription> for Parse {
    fn from(err: ParseFromDescription) -> Self {
        Self::ParseFromDescription(err)
    }
}

impl TryFrom<Parse> for ParseFromDescription {
    type Error = error::DifferentVariant;

    fn try_from(err: Parse) -> Result<Self, Self::Error> {
        match err {
            Parse::ParseFromDescription(err) => Ok(err),
            _ => Err(error::DifferentVariant),
        }
    }
}

impl From<Parse> for crate::Error {
    fn from(err: Parse) -> Self {
        match err {
            Parse::TryFromParsed(err) => Self::TryFromParsed(err),
            Parse::ParseFromDescription(err) => Self::ParseFromDescription(err),
            #[allow(deprecated)]
            Parse::UnexpectedTrailingCharacters => bug!("variant should not be used"),
        }
    }
}

impl TryFrom<crate::Error> for Parse {
    type Error = error::DifferentVariant;

    fn try_from(err: crate::Error) -> Result<Self, Self::Error> {
        match err {
            crate::Error::ParseFromDescription(err) => Ok(Self::ParseFromDescription(err)),
            #[allow(deprecated)]
            crate::Error::UnexpectedTrailingCharacters => bug!("variant should not be used"),
            crate::Error::TryFromParsed(err) => Ok(Self::TryFromParsed(err)),
            _ => Err(error::DifferentVariant),
        }
    }
}
