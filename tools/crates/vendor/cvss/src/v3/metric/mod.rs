//! CVSS v3.0/v3.1 metrics.

pub mod base;
pub mod environmental;
pub mod temporal;

use alloc::borrow::ToOwned;
use core::{
    fmt::{self, Debug, Display},
    str::FromStr,
};

use crate::Error;

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

    /// Exploit Code Maturity (E)
    E,

    /// Remediation Level (RL)
    RL,

    /// Report Confidence (RC)
    RC,

    /// Confidentiality Requirements (CR)
    CR,

    /// Integrity Requirements (IR)
    IR,

    /// Availability Requirements (AR)
    AR,

    /// Modified Attack Vector (MAV)
    MAV,

    /// Modified Attack Complexity (MAC)
    MAC,

    /// Modified Privileges Required (MPR)
    MPR,

    /// Modified User Interaction (MUI)
    MUI,

    /// Modified Scope (MS)
    MS,

    /// Modified Confidentiality (MC)
    MC,

    /// Modified Integrity (MI)
    MI,

    /// Modified Availability (MA)
    MA,
}

impl MetricType {
    /// Get the name of this metric (i.e. acronym)
    pub fn name(self) -> &'static str {
        match self {
            // Base metrics
            Self::A => "A",
            Self::AC => "AC",
            Self::AV => "AV",
            Self::C => "C",
            Self::I => "I",
            Self::PR => "PR",
            Self::S => "S",
            Self::UI => "UI",

            // Temporal metrics
            Self::E => "E",
            Self::RL => "RL",
            Self::RC => "RC",

            // Environmental metrics
            Self::CR => "CR",
            Self::IR => "IR",
            Self::AR => "AR",
            Self::MAV => "MAV",
            Self::MAC => "MAC",
            Self::MPR => "MPR",
            Self::MUI => "MUI",
            Self::MS => "MS",
            Self::MC => "MC",
            Self::MI => "MI",
            Self::MA => "MA",
        }
    }

    /// Get a description of this metric.
    pub fn description(self) -> &'static str {
        match self {
            // Base metrics
            Self::A => "Availability Impact",
            Self::AC => "Attack Complexity",
            Self::AV => "Attack Vector",
            Self::C => "Confidentiality Impact",
            Self::I => "Integrity Impact",
            Self::PR => "Privileges Required",
            Self::S => "Scope",
            Self::UI => "User Interaction",

            // Temporal metrics
            Self::E => "Exploit Code Maturity",
            Self::RL => "Remediation Level",
            Self::RC => "Report Confidence",

            // Environmental metrics
            Self::CR => "Confidentiality Requirements",
            Self::IR => "Integrity Requirements",
            Self::AR => "Availability Requirements",
            Self::MAV => "Modified Attack Vector",
            Self::MAC => "Modified Attack Complexity",
            Self::MPR => "Modified Privileges Required",
            Self::MUI => "Modified User Interaction",
            Self::MS => "Modified Scope",
            Self::MC => "Modified Confidentiality",
            Self::MI => "Modified Integrity",
            Self::MA => "Modified Availability",
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
            // Base metrics
            "A" => Ok(Self::A),
            "AC" => Ok(Self::AC),
            "AV" => Ok(Self::AV),
            "C" => Ok(Self::C),
            "I" => Ok(Self::I),
            "PR" => Ok(Self::PR),
            "S" => Ok(Self::S),
            "UI" => Ok(Self::UI),

            // Temporal metrics
            "E" => Ok(Self::E),
            "RL" => Ok(Self::RL),
            "RC" => Ok(Self::RC),

            // Environmental metrics
            "CR" => Ok(Self::CR),
            "IR" => Ok(Self::IR),
            "AR" => Ok(Self::AR),
            "MAV" => Ok(Self::MAV),
            "MAC" => Ok(Self::MAC),
            "MPR" => Ok(Self::MPR),
            "MUI" => Ok(Self::MUI),
            "MS" => Ok(Self::MS),
            "MC" => Ok(Self::MC),
            "MI" => Ok(Self::MI),
            "MA" => Ok(Self::MA),
            _ => Err(Error::UnknownMetric { name: s.to_owned() }),
        }
    }
}
