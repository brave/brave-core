//! CVSS v2.0 metrics.

pub mod base;
pub mod environmental;
pub mod temporal;

use alloc::borrow::ToOwned;
use core::{
    fmt::{self, Debug, Display},
    str::FromStr,
};

use crate::Error;

/// Trait for CVSS v2.0 metrics.
pub trait Metric: Copy + Clone + Debug + Display + Eq + FromStr + Ord {
    /// Get the name of this metric.
    fn name() -> &'static str {
        Self::TYPE.name()
    }

    /// Get CVSS v2.0 score for this metric.
    fn score(self) -> f64;

    /// Get `str` describing this metric's value
    fn as_str(self) -> &'static str;

    /// [`MetricType`] of this metric.
    const TYPE: MetricType;
}

/// Enum over all of the available v3.1 metrics.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum MetricType {
    /// Availability Impact (A)
    A,

    /// Access Complexity (AC)
    AC,

    /// Access Vector (AV)
    AV,

    /// Authentication (Au)
    Au,

    /// Confidentiality Impact (C)
    C,

    /// Integrity Impact (I)
    I,

    /// Exploitability (E)
    E,

    /// Remediation Level (RL)
    RL,

    /// Report Confidence (RC)
    RC,

    /// Collateral Damage Potential (CDP)
    CDP,

    /// Target Distribution (TD)
    TD,

    /// Confidentiality Requirement (CR)
    CR,

    /// Integrity Requirement (IR)
    IR,

    /// Availability Requirement (AR)
    AR,
}

impl MetricType {
    /// Get the name of this metric (i.e. acronym)
    pub fn name(self) -> &'static str {
        match self {
            // Base Metrics
            Self::A => "A",
            Self::AC => "AC",
            Self::Au => "Au",
            Self::AV => "AV",
            Self::C => "C",
            Self::I => "I",

            // Temporal Metrics
            Self::E => "E",
            Self::RL => "RL",
            Self::RC => "RC",

            // Environmental Metrics
            Self::CDP => "CDP",
            Self::TD => "TD",
            Self::CR => "CR",
            Self::IR => "IR",
            Self::AR => "AR",
        }
    }

    /// Get a description of this metric.
    pub fn description(self) -> &'static str {
        match self {
            // Base Metrics
            Self::A => "Availability Impact",
            Self::AC => "Access Complexity",
            Self::Au => "Authentication",
            Self::AV => "Access Vector",
            Self::C => "Confidentiality Impact",
            Self::I => "Integrity Impact",

            // Temporal Metrics
            Self::E => "Exploitability",
            Self::RL => "Remediation Level",
            Self::RC => "Report Confidence",

            // Environmental Metrics
            Self::CDP => "Collateral Damage Potential",
            Self::TD => "Target Distribution",
            Self::CR => "Confidentiality Requirement",
            Self::IR => "Integrity Requirement",
            Self::AR => "Availability Requirement",
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

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            // Base Metrics
            "A" => Ok(Self::A),
            "AC" => Ok(Self::AC),
            "Au" => Ok(Self::Au),
            "AV" => Ok(Self::AV),
            "C" => Ok(Self::C),
            "I" => Ok(Self::I),

            // Temporal Metrics
            "E" => Ok(Self::E),
            "RL" => Ok(Self::RL),
            "RC" => Ok(Self::RC),

            // Environmental Metrics
            "CDP" => Ok(Self::CDP),
            "TD" => Ok(Self::TD),
            "CR" => Ok(Self::CR),
            "IR" => Ok(Self::IR),
            "AR" => Ok(Self::AR),

            _ => Err(Error::UnknownMetric { name: s.to_owned() }),
        }
    }
}
