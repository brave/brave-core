//! CVSS v2.0 Base Metric Group - Confidentiality Impact (C)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Confidentiality Impact (C) - CVSS v2.0 Base Metric Group
///
/// Described in CVSS v2.0 Specification: Section 2.1.4:
/// <https://www.first.org/cvss/v2/guide#2-1-4-Confidentiality-Impact-C>
///
/// > This metric measures the impact on confidentiality of a successfully
/// > exploited vulnerability. Confidentiality refers to limiting information
/// > access and disclosure to only authorized users, as well as preventing
/// > access by, or disclosure to, unauthorized ones. The possible values for
/// > this metric are listed in Table 4. Increased confidentiality impact
/// > increases the vulnerability score.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ConfidentialityImpact {
    /// None (N)
    /// > There is no impact to the confidentiality of the system.
    None,

    /// Partial (P)
    /// > There is considerable informational disclosure. Access to some system
    /// > files is possible, but the attacker does not have control over what is
    /// > obtained, or the scope of the loss is constrained. An example is a
    /// > vulnerability that divulges only certain tables in a database.
    Partial,

    /// Complete (C)
    /// > There is total information disclosure, resulting in all system files
    /// > being revealed. The attacker is able to read all of the system's data
    /// > (memory, files, etc.)
    Complete,
}

impl Metric for ConfidentialityImpact {
    fn score(self) -> f64 {
        match self {
            Self::None => 0.0,
            Self::Partial => 0.275,
            Self::Complete => 0.660,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::None => "N",
            Self::Partial => "P",
            Self::Complete => "C",
        }
    }

    const TYPE: MetricType = MetricType::C;
}

impl fmt::Display for ConfidentialityImpact {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ConfidentialityImpact {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "N" => Ok(Self::None),
            "P" => Ok(Self::Partial),
            "C" => Ok(Self::Complete),
            _ => Err(Error::InvalidMetricV2 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
