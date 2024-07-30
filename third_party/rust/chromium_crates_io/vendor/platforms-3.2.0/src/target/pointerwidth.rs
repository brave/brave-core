//! Pointer width of the target architecture

use crate::error::Error;
use core::{fmt, str::FromStr};

#[cfg(feature = "serde")]
use serde::{de, de::Error as DeError, ser, Deserialize, Serialize};

/// `target_pointer_width`: Size of native pointer types (`usize`, `isize`) in bits
///
/// 64 bits for modern desktops and phones, 32-bits for older devices, 16 bits for certain microcontrollers
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum PointerWidth {
    /// `16`
    U16,

    /// `32`
    U32,

    /// `64`
    U64,
}

impl PointerWidth {
    /// String representing this `PointerWidth` which matches `#[cfg(target_pointer_width)]`
    pub fn as_str(self) -> &'static str {
        match self {
            PointerWidth::U16 => "16",
            PointerWidth::U32 => "32",
            PointerWidth::U64 => "64",
        }
    }
}

impl FromStr for PointerWidth {
    type Err = Error;

    /// Create a new `PointerWidth` from the given string
    fn from_str(name: &str) -> Result<Self, Self::Err> {
        let result = match name {
            "16" => PointerWidth::U16,
            "32" => PointerWidth::U32,
            "64" => PointerWidth::U64,
            _ => return Err(Error),
        };

        Ok(result)
    }
}

use core::convert::TryFrom;

impl TryFrom<u8> for PointerWidth {
    type Error = &'static str;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            64 => Ok(PointerWidth::U64),
            32 => Ok(PointerWidth::U32),
            16 => Ok(PointerWidth::U16),
            _ => Err("Invalid pointer width!"),
        }
    }
}

impl From<PointerWidth> for u8 {
    fn from(value: PointerWidth) -> Self {
        match value {
            PointerWidth::U64 => 64,
            PointerWidth::U32 => 32,
            PointerWidth::U16 => 16,
        }
    }
}

impl fmt::Display for PointerWidth {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl Serialize for PointerWidth {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(self.as_str())
    }
}

#[cfg(all(feature = "serde", feature = "std"))]
impl<'de> Deserialize<'de> for PointerWidth {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        let string = std::string::String::deserialize(deserializer)?;
        string.parse().map_err(|_| {
            D::Error::custom(std::format!(
                "Unrecognized value '{}' for target_pointer_width",
                string
            ))
        })
    }
}
