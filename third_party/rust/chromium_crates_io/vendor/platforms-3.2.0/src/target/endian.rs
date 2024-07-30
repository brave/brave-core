//! Endianness

use crate::error::Error;
use core::{fmt, str::FromStr};

#[cfg(feature = "serde")]
use serde::{de, de::Error as DeError, ser, Deserialize, Serialize};

/// `target_endian`: [Endianness](https://en.wikipedia.org/wiki/Endianness) of the target.
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum Endian {
    /// `big`
    Big,

    /// `little`
    Little,
}

impl Endian {
    /// String representing this `Endian` which matches `#[cfg(target_endian)]`
    pub fn as_str(self) -> &'static str {
        match self {
            Endian::Big => "big",
            Endian::Little => "little",
        }
    }
}

impl FromStr for Endian {
    type Err = Error;

    /// Create a new `Endian` from the given string
    fn from_str(name: &str) -> Result<Self, Self::Err> {
        let result = match name {
            "big" => Endian::Big,
            "little" => Endian::Little,
            _ => return Err(Error),
        };

        Ok(result)
    }
}

impl fmt::Display for Endian {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl Serialize for Endian {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(self.as_str())
    }
}

#[cfg(all(feature = "serde", feature = "std"))]
impl<'de> Deserialize<'de> for Endian {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        let string = std::string::String::deserialize(deserializer)?;
        string.parse().map_err(|_| {
            D::Error::custom(std::format!(
                "Unrecognized value '{}' for target_endian",
                string
            ))
        })
    }
}
