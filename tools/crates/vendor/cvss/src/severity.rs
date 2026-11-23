//! Qualitative Severity Rating Scale

use crate::Error;
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

#[cfg(feature = "serde")]
use {
    alloc::string::String,
    serde::{Deserialize, Serialize, de, ser},
};

/// Qualitative Severity Rating Scale
///
/// Described in CVSS v3.1 Specification: Section 5:
/// <https://www.first.org/cvss/v3.1/specification-document#Qualitative-Severity-Rating-Scale>
///
/// And in CVSS v4.0 Specification: Section 6:
/// <https://www.first.org/cvss/v4.0/specification-document#Qualitative-Severity-Rating-Scale>
///
/// The rating scales in v3 and v4 are the same.
///
/// > For some purposes it is useful to have a textual representation of the
/// > scores.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord, Hash)]
pub enum Severity {
    /// None: CVSS Score 0.0
    None,

    /// Low: CVSS Score 0.1 - 3.9
    Low,

    /// Medium: CVSS Score 4.0 - 6.9
    Medium,

    /// High: CVSS Score 7.0 - 8.9
    High,

    /// Critical: CVSS Score 9.0 - 10.0
    Critical,
}

impl Severity {
    /// Get a `str` describing the severity level
    pub fn as_str(self) -> &'static str {
        match self {
            Self::None => "none",
            Self::Low => "low",
            Self::Medium => "medium",
            Self::High => "high",
            Self::Critical => "critical",
        }
    }
}

impl FromStr for Severity {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s.to_ascii_lowercase().as_str() {
            "none" => Ok(Self::None),
            "low" => Ok(Self::Low),
            "medium" => Ok(Self::Medium),
            "high" => Ok(Self::High),
            "critical" => Ok(Self::Critical),
            _ => Err(Error::InvalidSeverity { name: s.to_owned() }),
        }
    }
}

impl fmt::Display for Severity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
impl<'de> Deserialize<'de> for Severity {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        String::deserialize(deserializer)?
            .parse()
            .map_err(de::Error::custom)
    }
}

#[cfg(feature = "serde")]
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
impl Serialize for Severity {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        self.as_str().serialize(serializer)
    }
}
