//! CVSS v3.0/v3.1 metrics.

use crate::{Error, Result};
use alloc::borrow::ToOwned;
use core::{
    fmt::{self, Debug, Display},
    str::FromStr,
};

/// Trait for CVSSv3 metrics.
pub trait Metric: Copy + Clone + Debug + Display + Eq + FromStr + Ord {
    /// [`MetricType`] of this metric.
    const TYPE: MetricType;

    /// Get the name of this metric.
    fn name() -> &'static str {
        Self::TYPE.name()
    }

    /// Get CVSS v3.1 score for this metric.
    fn score(self) -> f64;

    /// Get `str` describing this metric's value
    fn as_str(self) -> &'static str;
}

/// Enum over all of the available CVSSv3 metrics.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum MetricType {
    /// Availability Impact (A)
    A,

    /// Attack Complexity (AC)
    AC,

    /// Attack Vector (AV)
    AV,

    /// Confidentiality Impact (C)
    C,

    /// Integrity Impact (I)
    I,

    /// Privileges Required (PR)
    PR,

    /// Scope (S)
    S,

    /// User Interaction (UI)
    UI,
}

impl MetricType {
    /// Get the name of this metric (i.e. acronym)
    pub fn name(self) -> &'static str {
        match self {
            Self::A => "A",
            Self::AC => "AC",
            Self::AV => "AV",
            Self::C => "C",
            Self::I => "I",
            Self::PR => "PR",
            Self::S => "S",
            Self::UI => "UI",
        }
    }

    /// Get a description of this metric.
    pub fn description(self) -> &'static str {
        match self {
            Self::A => "Availability Impact",
            Self::AC => "Attack Complexity",
            Self::AV => "Attack Vector",
            Self::C => "Confidentiality Impact",
            Self::I => "Integrity Impact",
            Self::PR => "Privileges Required",
            Self::S => "Scope",
            Self::UI => "User Interaction",
        }
    }
}

impl Display for MetricType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.name())
    }
}

impl FromStr for MetricType {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "A" => Ok(Self::A),
            "AC" => Ok(Self::AC),
            "AV" => Ok(Self::AV),
            "C" => Ok(Self::C),
            "I" => Ok(Self::I),
            "PR" => Ok(Self::PR),
            "S" => Ok(Self::S),
            "UI" => Ok(Self::UI),
            _ => Err(Error::UnknownMetric { name: s.to_owned() }),
        }
    }
}
