//! Attack Complexity (AC)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v3::{Metric, MetricType};

/// Attack Complexity (AC) - CVSS v3.1 Base Metric Group
///
/// Described in CVSS v3.1 Specification: Section 2.1.2:
/// <https://www.first.org/cvss/specification-document#t6>
///
/// > This metric describes the conditions beyond the attacker’s control that
/// > must exist in order to exploit the vulnerability. As described below,
/// > such conditions may require the collection of more information about the
/// > target, or computational exceptions. Importantly, the assessment of this
/// > metric excludes any requirements for user interaction in order to exploit
/// > the vulnerability (such conditions are captured in the User Interaction
/// > metric). If a specific configuration is required for an attack to
/// > succeed, the Base metrics should be scored assuming the vulnerable
/// > component is in that configuration. The Base Score is greatest for the
/// > least complex attacks.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum AttackComplexity {
    /// High (H)
    ///
    /// > A successful attack depends on conditions beyond the attacker's control.
    /// > That is, a successful attack cannot be accomplished at will, but requires
    /// > the attacker to invest in some measurable amount of effort in preparation
    /// > or execution against the vulnerable component before a successful attack
    /// > can be expected. For example, a successful attack may depend on an
    /// > attacker overcoming any of the following conditions:
    /// >
    /// > - The attacker must gather knowledge about the environment in which
    /// >   the vulnerable target/component exists. For example, a requirement
    /// >   to collect details on target configuration settings, sequence numbers,
    /// >   or shared secrets.
    /// > - The attacker must prepare the target environment to improve exploit
    /// >   reliability. For example, repeated exploitation to win a race
    /// >   condition, or overcoming advanced exploit mitigation techniques.
    /// > - The attacker must inject themselves into the logical network path
    /// >   between the target and the resource requested by the victim in order
    /// >   to read and/or modify network communications (e.g., a man in the middle
    /// >   attack).
    High,

    /// Low (L)
    ///
    /// > Specialized access conditions or extenuating circumstances do not exist.
    /// > An attacker can expect repeatable success when attacking the vulnerable component.
    Low,
}

#[allow(clippy::derivable_impls)]
impl Default for AttackComplexity {
    fn default() -> Self {
        Self::High
    }
}

impl Metric for AttackComplexity {
    fn score(self) -> f64 {
        match self {
            Self::High => 0.44,
            Self::Low => 0.77,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::High => "H",
            Self::Low => "L",
        }
    }

    const TYPE: MetricType = MetricType::AC;
}

impl fmt::Display for AttackComplexity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for AttackComplexity {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "H" => Ok(Self::High),
            "L" => Ok(Self::Low),
            _ => Err(Error::InvalidMetricV3 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
