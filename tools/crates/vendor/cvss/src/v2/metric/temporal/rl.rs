//! CVSS v2.0 Temporal Metric - Remediation Level (RL)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Remediation Level (RL) - CVSS v2.0 Temporal Metric Group
///
/// Described in CVSS v2.0 Specification: Section 2.2.2:
/// <https://www.first.org/cvss/v2/guide#2-2-2-Remediation-Level-RL>
///
/// > The remediation level of a vulnerability is an important factor for
/// > prioritization. The typical vulnerability is unpatched when initially
/// > published.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum RemediationLevel {
    /// Official Fix (OF)
    /// > A complete vendor solution is available. Either the vendor has issued
    /// > an official patch, or an upgrade is available.
    OfficialFix,

    /// Temporary Fix (TF)
    /// > There is an official but temporary fix available. This includes
    /// > instances where the vendor issues a temporary hotfix, tool, or
    /// > workaround.
    TemporaryFix,

    /// Workaround (W)
    /// > There is an unofficial, non-vendor solution available. In some cases,
    /// > users of the affected technology will create a patch of their own or
    /// > provide steps to work around or otherwise mitigate the vulnerability.
    Workaround,

    //// Unavailable (U)
    /// > There is either no solution available or it is impossible to apply.
    Unavailable,

    /// Not Defined (ND)
    /// > Assigning this value to the metric will not influence the score. It is
    /// > a signal to the equation to skip this metric.
    NotDefined,
}

impl Metric for RemediationLevel {
    fn score(self) -> f64 {
        match self {
            Self::OfficialFix => 0.87,
            Self::TemporaryFix => 0.90,
            Self::Workaround => 0.95,
            Self::Unavailable => 1.0,
            Self::NotDefined => 1.0,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::OfficialFix => "OF",
            Self::TemporaryFix => "TF",
            Self::Workaround => "W",
            Self::Unavailable => "U",
            Self::NotDefined => "ND",
        }
    }

    const TYPE: MetricType = MetricType::RL;
}

impl fmt::Display for RemediationLevel {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for RemediationLevel {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "OF" => Ok(Self::OfficialFix),
            "TF" => Ok(Self::TemporaryFix),
            "W" => Ok(Self::Workaround),
            "U" => Ok(Self::Unavailable),
            "ND" => Ok(Self::NotDefined),
            _ => Err(Error::InvalidMetricV2 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
