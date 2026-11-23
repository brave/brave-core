#[derive(Clone, Copy, PartialEq, Eq)]
#[cfg_attr(feature = "defmt-03", derive(defmt::Format))]
pub enum Error {
    InvalidChar,
    InvalidLength(usize),
    Overflow,
}

impl ::core::fmt::Debug for Error {
    fn fmt(&self, f: &mut ::core::fmt::Formatter) -> ::core::fmt::Result {
        match *self {
            Error::InvalidLength(len) => write!(f, "Invalid input length {len}"),
            Error::InvalidChar => write!(f, "Invalid character"),
            Error::Overflow => write!(f, "Overflow"),
        }
    }
}

impl ::core::fmt::Display for Error {
    fn fmt(&self, f: &mut ::core::fmt::Formatter) -> ::core::fmt::Result {
        ::core::fmt::Debug::fmt(&self, f)
    }
}

#[cfg(all(not(feature = "std"), has_core_error))]
use core::error::Error as ErrorTrait;
#[cfg(feature = "std")]
use std::error::Error as ErrorTrait;

#[cfg(any(feature = "std", has_core_error))]
impl ErrorTrait for Error {
    fn description(&self) -> &str {
        match *self {
            Error::InvalidChar => "invalid character",
            Error::InvalidLength(_) => "invalid length",
            Error::Overflow => "overflow",
        }
    }
}
