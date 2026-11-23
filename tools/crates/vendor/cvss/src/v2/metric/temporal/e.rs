//! CVSS v2.0 Temporal Metric - Exploitability (E)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Exploitability (E) - CVSS v2.0 Temporal Metric Group
///
/// Described in CVSS v2.0 Specification: Section 2.2.1:
/// <https://www.first.org/cvss/v2/guide#2-2-1-Exploitability-E>
///
/// > This metric measures the current state of exploit techniques or code
/// > availability. Public availability of easy-to-use exploit code increases
/// > the number of potential attackers by including those who are unskilled,
/// > thereby increasing the severity of the vulnerability.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum Exploitability {
    /// Unproven (U)
    /// > No exploit code is available, or an exploit is entirely theoretical.
    Unproven,

    /// Proof-of-Concept (POC)
    /// > Proof-of-concept exploit code or an attack demonstration that is not
    /// > practical for most systems is available. The code or technique is not
    /// > functional in all situations and may require substantial modification
    /// > by a skilled attacker.
    ProofOfConcept,

    /// Functional (F)
    /// > Functional exploit code is available. The code works in most
    /// > situations where the vulnerability exists.
    Functional,

    /// High (H)
    /// > Either the vulnerability is exploitable by functional mobile
    /// > autonomous code, or no exploit is required (manual trigger) and
    /// > details are widely available. The code works in every situation, or is
    /// > actively being delivered via a mobile autonomous agent (such as a worm
    /// > or virus).
    High,

    /// Not Defined (ND)
    /// > Assigning this value to the metric will not influence the score. It is
    /// > a signal to the equation to skip this metric.
    NotDefined,
}

impl Metric for Exploitability {
    fn score(self) -> f64 {
        match self {
            Self::Unproven => 0.85,
            Self::ProofOfConcept => 0.9,
            Self::Functional => 0.95,
            Self::High => 1.0,
            Self::NotDefined => 1.0,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::Unproven => "U",
            Self::ProofOfConcept => "POC",
            Self::Functional => "F",
            Self::High => "H",
            Self::NotDefined => "ND",
        }
    }

    const TYPE: MetricType = MetricType::E;
}

impl fmt::Display for Exploitability {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for Exploitability {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "U" => Ok(Self::Unproven),
            "POC" => Ok(Self::ProofOfConcept),
            "F" => Ok(Self::Functional),
            "H" => Ok(Self::High),
            "ND" => Ok(Self::NotDefined),
            _ => Err(Error::InvalidMetricV2 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
