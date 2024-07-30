//! Rust target environments

use crate::error::Error;
use core::{fmt, str::FromStr};

#[cfg(feature = "serde")]
use serde::{de, de::Error as DeError, ser, Deserialize, Serialize};

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

    /// `eabihf`
    Eabihf,

    /// `gnu`: The GNU C Library (glibc)
    Gnu,

    /// `gnueabihf`
    Gnueabihf,

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

    /// `ohos`
    OhOS,

    /// `psx`
    Psx,

    /// `relibc`
    Relibc,

    /// `sgx`: Intel Software Guard Extensions (SGX) Enclave
    Sgx,

    /// `uclibc`: C library for developing embedded Linux systems
    UClibc,
}

impl Env {
    /// String representing this `Env` which matches `#[cfg(target_env)]`
    pub fn as_str(self) -> &'static str {
        match self {
            Env::None => "",
            Env::Eabihf => "eabihf",
            Env::Gnu => "gnu",
            Env::Gnueabihf => "gnueabihf",
            Env::Msvc => "msvc",
            Env::Musl => "musl",
            Env::Newlib => "newlib",
            Env::Nto70 => "nto70",
            Env::Nto71 => "nto71",
            Env::OhOS => "ohos",
            Env::Psx => "psx",
            Env::Relibc => "relibc",
            Env::Sgx => "sgx",
            Env::UClibc => "uclibc",
        }
    }
}

impl FromStr for Env {
    type Err = Error;

    /// Create a new `Env` from the given string
    fn from_str(name: &str) -> Result<Self, Self::Err> {
        let result = match name {
            "" => Env::None,
            "eabihf" => Env::Eabihf,
            "gnu" => Env::Gnu,
            "gnueabihf" => Env::Gnueabihf,
            "msvc" => Env::Msvc,
            "musl" => Env::Musl,
            "newlib" => Env::Newlib,
            "nto70" => Env::Nto70,
            "nto71" => Env::Nto71,
            "ohos" => Env::OhOS,
            "psx" => Env::Psx,
            "relibc" => Env::Relibc,
            "sgx" => Env::Sgx,
            "uclibc" => Env::UClibc,
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
