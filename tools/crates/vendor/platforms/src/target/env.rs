//! Rust target environments

use core::{fmt, str::FromStr};

#[cfg(feature = "serde")]
use serde::{de, de::Error as DeError, ser, Deserialize, Serialize};

use crate::error::Error;

/// `target_env`: target environment that disambiguates the target platform by ABI / libc.
///
/// This value is closely related to the fourth element of the platform target triple,
/// though it is not identical. For example, embedded ABIs such as `gnueabihf` will simply
/// define `target_env` as `"gnu"` (i.e. `target::Env::GNU`)
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum Env {
    /// ``: None
    None,

    /// `gnu`: The GNU C Library (glibc)
    Gnu,

    /// `macabi`
    Macabi,

    /// `mlibc`
    Mlibc,

    /// `msvc`: Microsoft Visual C(++)
    Msvc,

    /// `musl`: Clean, efficient, standards-conformant libc implementation.
    Musl,

    /// `newlib`
    Newlib,

    /// `nto70`
    Nto70,

    /// `nto71`
    Nto71,

    /// `nto71_iosock`
    Nto71Iosock,

    /// `ohos`
    OhOS,

    /// `p1`
    P1,

    /// `p2`
    P2,

    /// `p3`
    P3,

    /// `relibc`
    Relibc,

    /// `sgx`: Intel Software Guard Extensions (SGX) Enclave
    Sgx,

    /// `sim`
    Sim,

    /// `uclibc`: C library for developing embedded Linux systems
    UClibc,

    /// `v5`
    V5,
}

impl Env {
    /// String representing this `Env` which matches `#[cfg(target_env)]`
    pub fn as_str(self) -> &'static str {
        match self {
            Self::None => "",
            Self::Gnu => "gnu",
            Self::Macabi => "macabi",
            Self::Mlibc => "mlibc",
            Self::Msvc => "msvc",
            Self::Musl => "musl",
            Self::Newlib => "newlib",
            Self::Nto70 => "nto70",
            Self::Nto71 => "nto71",
            Self::Nto71Iosock => "nto71_iosock",
            Self::OhOS => "ohos",
            Self::P1 => "p1",
            Self::P2 => "p2",
            Self::P3 => "p3",
            Self::Relibc => "relibc",
            Self::Sgx => "sgx",
            Self::Sim => "sim",
            Self::UClibc => "uclibc",
            Self::V5 => "v5",
        }
    }
}

impl FromStr for Env {
    type Err = Error;

    /// Create a new `Env` from the given string
    fn from_str(name: &str) -> Result<Self, Self::Err> {
        let result = match name {
            "" => Self::None,
            "gnu" => Self::Gnu,
            "macabi" => Self::Macabi,
            "mlibc" => Self::Mlibc,
            "msvc" => Self::Msvc,
            "musl" => Self::Musl,
            "newlib" => Self::Newlib,
            "nto70" => Self::Nto70,
            "nto71" => Self::Nto71,
            "nto71_iosock" => Self::Nto71Iosock,
            "ohos" => Self::OhOS,
            "p1" => Self::P1,
            "p2" => Self::P2,
            "p3" => Self::P3,
            "relibc" => Self::Relibc,
            "sgx" => Self::Sgx,
            "sim" => Self::Sim,
            "uclibc" => Self::UClibc,
            "v5" => Self::V5,
            _ => return Err(Error),
        };

        Ok(result)
    }
}

impl fmt::Display for Env {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl Serialize for Env {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(self.as_str())
    }
}

#[cfg(all(feature = "serde", feature = "std"))]
impl<'de> Deserialize<'de> for Env {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        let string = std::string::String::deserialize(deserializer)?;
        string.parse().map_err(|_| {
            D::Error::custom(std::format!(
                "Unrecognized value '{}' for target_env",
                string
            ))
        })
    }
}
