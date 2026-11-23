//! CVSS v2.0 Base Metric Group - Integrity Impact (C)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Integrity Impact (C) - CVSS v2.0 Base Metric Group
///
/// Described in CVSS v2.0 Specification: Section 2.1.5:
/// <https://www.first.org/cvss/v2/guide#2-1-5-Integrity-Impact-I>
///
/// > This metric measures the impact to integrity of a successfully exploited
/// > vulnerability. Integrity refers to the trustworthiness and guaranteed
/// > veracity of information. The possible values for this metric are listed in
/// > Table 5. Increased integrity impact increases the vulnerability score.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum IntegrityImpact {
    /// None (N)
    /// > There is no impact to the integrity of the system.
    None,

    /// Partial (P)
    /// > Modification of some system files or information is possible, but the
    /// > attacker does not have control over what can be modified, or the scope
    /// > of what the attacker can affect is limited. For example, system or
    /// > application files may be overwritten or modified, but either the
    /// > attacker has no control over which files are affected or the attacker
    /// > can modify files within only a limited context or scope.
    Partial,

    /// Complete (C)
    /// > There is a total compromise of system integrity. There is a complete
    /// > loss of system protection, resulting in the entire system being
    /// > compromised. The attacker is able to modify any files on the target
    /// > system.
    Complete,
}

impl Metric for IntegrityImpact {
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

    const TYPE: MetricType = MetricType::I;
}

impl fmt::Display for IntegrityImpact {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for IntegrityImpact {
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
