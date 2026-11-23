//! Exploit Code Maturity (E)

use alloc::borrow::ToOwned;

use core::{fmt, str::FromStr};

use crate::Error;
use crate::v3::{Metric, MetricType};

/// Exploit Code Maturity (E) - CVSS v3.1 Temporal Metric Group
/// > This metric measures the likelihood of the vulnerability being attacked,
/// > and is typically based on the current state of exploit techniques, exploit
/// > code availability, or active, “in-the-wild” exploitation.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ExploitCodeMaturity {
    /// Not Defined (X)
    /// > Assigning this value indicates there is insufficient information to
    /// > choose one of the other values, and has no impact on the overall
    /// > Temporal Score, i.e., it has the same effect on scoring as assigning
    /// > High.
    NotDefined,

    /// High (H)
    /// > Functional autonomous code exists, or no exploit is required (manual
    /// > trigger) and details are widely available. Exploit code works in every
    /// > situation, or is actively being delivered via an autonomous agent
    /// > (such as a worm or virus). Network-connected systems are likely to
    /// > encounter scanning or exploitation attempts. Exploit development has
    /// > reached the level of reliable, widely available, easy-to-use automated
    /// > tools.
    High,

    /// Functional (F)
    /// > Functional exploit code is available. The code works in most
    /// > situations where the vulnerability exists.
    Functional,

    /// Proof-of-Concept (P)
    /// > Proof-of-concept exploit code is available, or an attack demonstration
    /// > is not practical for most systems. The code or technique is not
    /// > functional in all situations and may require substantial modification
    /// > by a skilled attacker.
    ProofOfConcept,

    /// Unproven (U)
    /// > No exploit code is available, or an exploit is theoretical.
    Unproven,
}

impl Metric for ExploitCodeMaturity {
    fn score(self) -> f64 {
        match self {
            Self::NotDefined => 1.0,
            Self::High => 1.0,
            Self::Functional => 0.97,
            Self::ProofOfConcept => 0.94,
            Self::Unproven => 0.91,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::NotDefined => "X",
            Self::High => "H",
            Self::Functional => "F",
            Self::ProofOfConcept => "P",
            Self::Unproven => "U",
        }
    }

    const TYPE: MetricType = MetricType::E;
}

impl fmt::Display for ExploitCodeMaturity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ExploitCodeMaturity {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "X" => Ok(Self::NotDefined),
            "H" => Ok(Self::High),
            "F" => Ok(Self::Functional),
            "P" => Ok(Self::ProofOfConcept),
            "U" => Ok(Self::Unproven),
            _ => Err(Error::InvalidMetricV3 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
