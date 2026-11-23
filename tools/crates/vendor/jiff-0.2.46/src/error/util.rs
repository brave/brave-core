use crate::{error, util::escape::Byte};

#[derive(Clone, Debug)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub(crate) enum RoundingIncrementError {
    ForDateTime,
    ForOffset,
    ForSignedDuration,
    ForSpan,
    ForTime,
    ForTimestamp,
}

impl From<RoundingIncrementError> for error::Error {
    #[cold]
    #[inline(never)]
    fn from(err: RoundingIncrementError) -> error::Error {
        error::ErrorKind::RoundingIncrement(err).into()
    }
}

impl error::IntoError for RoundingIncrementError {
    fn into_error(self) -> error::Error {
        self.into()
    }
}

impl core::fmt::Display for RoundingIncrementError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        use self::RoundingIncrementError::*;

        match *self {
            ForDateTime => f.write_str("failed rounding datetime"),
            ForOffset => f.write_str("failed rounding time zone offset"),
            ForSignedDuration => {
                f.write_str("failed rounding signed duration")
            }
            ForSpan => f.write_str("failed rounding span"),
            ForTime => f.write_str("failed rounding time"),
            ForTimestamp => f.write_str("failed rounding timestamp"),
        }
    }
}

#[derive(Clone, Debug)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub(crate) enum ParseIntError {
    NoDigitsFound,
    InvalidDigit(u8),
    TooBig,
}

impl From<ParseIntError> for error::Error {
    #[cold]
    #[inline(never)]
    fn from(err: ParseIntError) -> error::Error {
        error::ErrorKind::ParseInt(err).into()
    }
}

impl error::IntoError for ParseIntError {
    fn into_error(self) -> error::Error {
        self.into()
    }
}

impl core::fmt::Display for ParseIntError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        use self::ParseIntError::*;

        match *self {
            NoDigitsFound => write!(f, "invalid number, no digits found"),
            InvalidDigit(got) => {
                write!(f, "invalid digit, expected 0-9 but got {}", Byte(got))
            }
            TooBig => {
                write!(f, "number too big to parse into 64-bit integer")
            }
        }
    }
}

#[derive(Clone, Debug)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub(crate) enum ParseFractionError {
    NoDigitsFound,
    TooManyDigits,
    InvalidDigit(u8),
    TooBig,
}

impl ParseFractionError {
    pub(crate) const MAX_PRECISION: usize = 9;
}

impl From<ParseFractionError> for error::Error {
    #[cold]
    #[inline(never)]
    fn from(err: ParseFractionError) -> error::Error {
        error::ErrorKind::ParseFraction(err).into()
    }
}

impl error::IntoError for ParseFractionError {
    fn into_error(self) -> error::Error {
        self.into()
    }
}

impl core::fmt::Display for ParseFractionError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        use self::ParseFractionError::*;

        match *self {
            NoDigitsFound => write!(f, "invalid fraction, no digits found"),
            TooManyDigits => write!(
                f,
                "invalid fraction, too many digits \
                 (at most {max} are allowed)",
                max = ParseFractionError::MAX_PRECISION,
            ),
            InvalidDigit(got) => {
                write!(
                    f,
                    "invalid fractional digit, expected 0-9 but got {}",
                    Byte(got)
                )
            }
            TooBig => {
                write!(
                    f,
                    "fractional number too big to parse into 64-bit integer"
                )
            }
        }
    }
}

#[derive(Clone, Debug)]
pub(crate) struct OsStrUtf8Error {
    #[cfg(feature = "std")]
    value: alloc::boxed::Box<std::ffi::OsStr>,
}

#[cfg(feature = "std")]
impl From<&std::ffi::OsStr> for OsStrUtf8Error {
    #[cold]
    #[inline(never)]
    fn from(value: &std::ffi::OsStr) -> OsStrUtf8Error {
        OsStrUtf8Error { value: value.into() }
    }
}

impl From<OsStrUtf8Error> for error::Error {
    #[cold]
    #[inline(never)]
    fn from(err: OsStrUtf8Error) -> error::Error {
        error::ErrorKind::OsStrUtf8(err).into()
    }
}

impl error::IntoError for OsStrUtf8Error {
    fn into_error(self) -> error::Error {
        self.into()
    }
}

impl core::fmt::Display for OsStrUtf8Error {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        #[cfg(feature = "std")]
        {
            write!(
                f,
                "environment value `{value:?}` is not valid UTF-8",
                value = self.value
            )
        }
        #[cfg(not(feature = "std"))]
        {
            write!(f, "<BUG: SHOULD NOT EXIST>")
        }
    }
}

#[cfg(feature = "defmt")]
impl defmt::Format for OsStrUtf8Error {
    fn format(&self, f: defmt::Formatter) {
        // `OsStr` does not implement `defmt::Format`. Since this error is std-only
        // and defmt is mainly used in embedded contexts, omitting the value is fine.
        defmt::write!(f, "OsStrUtf8Error(unavailable)");
    }
}
