//! CVSS v4 metrics.

use crate::{Error, Result};
use alloc::borrow::ToOwned;
use core::{
    fmt::{self, Debug, Display},
    str::FromStr,
};

pub mod base;
pub mod environmental;
pub mod supplemental;
pub mod threat;

/// Trait for CVSS 4.0 metrics.
pub trait Metric: Copy + Clone + Debug + Display + Eq + FromStr + Ord + Default {
    /// [`MetricType`] of this metric.
    const TYPE: MetricType;

    /// Get the name of this metric.
    fn name() -> &'static str {
        Self::TYPE.name()
    }

    /// Get `str` describing this metric's value
    fn as_str(self) -> &'static str;
}

#[cfg(feature = "std")]
/// Some metrics have a level associated with them.
pub(crate) trait MetricLevel {
    /// Metric level used in scoring.
    fn level(self) -> f64;
}

/// Enum over all of the available metrics.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum MetricType {
    /// Attack Complexity (AC)
    AC,
    /// Attack Requirements (AT)
    AT,
    /// Attack Vector (AV)
    AV,
    /// Privileges Required (PR)
    PR,
    /// Availability Impact to the Subsequent System (SA)
    SA,
    /// Confidentiality Impact to the Subsequent System (SC)
    SC,
    /// Integrity Impact to the Subsequent System (SI)
    SI,
    /// User Interaction (UI)
    UI,
    /// Availability Impact to the Vulnerable System (VA)
    VA,
    /// Confidentiality Impact to the Vulnerable System (VC)
    VC,
    /// Integrity Impact to the Vulnerable System (VI)
    VI,
    /// Exploit Maturity (E)
    E,
    /// Availability Requirements (AR)
    AR,
    /// Confidentiality Requirements (CR)
    CR,
    /// Integrity Requirements (IR)
    IR,
    /// Modified Attack Complexity (AC)
    MAC,
    /// Modified Attack Requirements (MAT)
    MAT,
    /// Modified Attack Vector (MAV)
    MAV,
    /// Modified Privileges Required (MPR)
    MPR,
    /// Modified Availability Impact to the Subsequent System (MSA)
    MSA,
    /// Modified Confidentiality Impact to the Subsequent System (MSC)
    MSC,
    /// Modified Integrity Impact to the Subsequent System (MSI)
    MSI,
    /// Modified User Interaction (MUI)
    MUI,
    /// Modified Availability Impact to the Vulnerable System (MVA)
    MVA,
    /// Modified Confidentiality Impact to the Vulnerable System (MVC)
    MVC,
    /// Modified Integrity Impact to the Vulnerable System (MVI)
    MVI,
    /// Automatable (AU)
    AU,
    /// Recovery (R)
    R,
    /// Vulnerability Response Effort (RE)
    RE,
    /// Safety (S)
    S,
    /// Provider Urgency (U)
    U,
    /// Value Density (V)
    V,
}

impl MetricType {
    /// Get the name of this metric (i.e. acronym)
    pub fn name(self) -> &'static str {
        match self {
            Self::AC => "AC",
            Self::AT => "AT",
            Self::AV => "AV",
            Self::PR => "PR",
            Self::SA => "SA",
            Self::SC => "SC",
            Self::SI => "SI",
            Self::UI => "UI",
            Self::VA => "VA",
            Self::VC => "VC",
            Self::VI => "VI",
            Self::E => "E",
            Self::AR => "AR",
            Self::CR => "CR",
            Self::IR => "IR",
            Self::MAC => "MAC",
            Self::MAT => "MAT",
            Self::MAV => "MAV",
            Self::MPR => "MPR",
            Self::MSA => "MSA",
            Self::MSC => "MSC",
            Self::MSI => "MSI",
            Self::MUI => "MUI",
            Self::MVA => "MVA",
            Self::MVC => "MVC",
            Self::MVI => "MVI",
            Self::AU => "AU",
            Self::R => "R",
            Self::RE => "RE",
            Self::S => "S",
            Self::U => "U",
            Self::V => "V",
        }
    }

    /// Get a description of this metric.
    pub fn description(self) -> &'static str {
        match self {
            Self::AC => "Attack Complexity",
            Self::AT => "Attack Requirements",
            Self::AV => "Attack Vector",
            Self::PR => "Privileges Required",
            Self::SA => "Availability Impact to the Subsequent System",
            Self::SC => "Confidentiality Impact to the Subsequent System",
            Self::SI => "Integrity Impact to the Subsequent System",
            Self::UI => "User Interaction",
            Self::VA => "Availability Impact to the Vulnerable System",
            Self::VC => "Confidentiality Impact to the Vulnerable System",
            Self::VI => "Integrity Impact to the Vulnerable System",
            Self::E => "Exploit Maturity",
            Self::AR => "Availability Requirements",
            Self::CR => "Confidentiality Requirements",
            Self::IR => "Integrity Requirements",
            Self::MAC => "Modified Attack Complexity",
            Self::MAT => "Modified Attack Requirements",
            Self::MAV => "Modified Attack Vector",
            Self::MPR => "Modified Privileges Required",
            Self::MSA => "Modified Availability Impact to the Subsequent System",
            Self::MSC => "Modified Confidentiality Impact to the Subsequent System",
            Self::MSI => "Modified Integrity Impact to the Subsequent System",
            Self::MUI => "Modified User Interaction",
            Self::MVA => "Modified Availability Impact to the Vulnerable System",
            Self::MVC => "Modified Confidentiality Impact to the Vulnerable System",
            Self::MVI => "Modified Integrity Impact to the Vulnerable System",
            Self::AU => "Automatable",
            Self::R => "Recovery",
            Self::RE => "Vulnerability Response Effort",
            Self::S => "Safety",
            Self::U => "Provider Urgency",
            Self::V => "Value Density",
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
            "AC" => Ok(Self::AC),
            "AT" => Ok(Self::AT),
            "AV" => Ok(Self::AV),
            "PR" => Ok(Self::PR),
            "SA" => Ok(Self::SA),
            "SC" => Ok(Self::SC),
            "SI" => Ok(Self::SI),
            "UI" => Ok(Self::UI),
            "VA" => Ok(Self::VA),
            "VC" => Ok(Self::VC),
            "VI" => Ok(Self::VI),
            "E" => Ok(Self::E),
            "AR" => Ok(Self::AR),
            "CR" => Ok(Self::CR),
            "IR" => Ok(Self::IR),
            "MAC" => Ok(Self::MAC),
            "MAT" => Ok(Self::MAT),
            "MAV" => Ok(Self::MAV),
            "MPR" => Ok(Self::MPR),
            "MSA" => Ok(Self::MSA),
            "MSC" => Ok(Self::MSC),
            "MSI" => Ok(Self::MSI),
            "MUI" => Ok(Self::MUI),
            "MVA" => Ok(Self::MVA),
            "MVC" => Ok(Self::MVC),
            "MVI" => Ok(Self::MVI),
            "AU" => Ok(Self::AU),
            "R" => Ok(Self::R),
            "RE" => Ok(Self::RE),
            "S" => Ok(Self::S),
            "U" => Ok(Self::U),
            "V" => Ok(Self::V),
            _ => Err(Error::UnknownMetric { name: s.to_owned() }),
        }
    }
}
