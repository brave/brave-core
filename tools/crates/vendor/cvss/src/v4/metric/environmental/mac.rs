//! Modified Attack Complexity (MAC)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Modified Attack Complexity (MAC) - CVSS v4.0 Environmental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 4.2
///
/// > This metric captures measurable actions that must be taken by the attacker
/// > to actively evade or circumvent **existing built-in security-enhancing
/// > conditions** in order to obtain a working exploit. These are conditions
/// > whose primary purpose is to increase security and/or increase exploit
/// > engineering complexity. A vulnerability exploitable without a
/// > target-specific variable has a lower complexity than a vulnerability that
/// > would require non-trivial customization. This metric is meant to capture
/// > security mechanisms utilized by the vulnerable system, and does not relate
/// > to the amount of time or attempts it would take for an attacker to
/// > succeed, e.g. a race condition. If the attacker does not take action to
/// > overcome these conditions, the attack will always fail.
/// >
/// > The evasion or satisfaction of authentication mechanisms or requisites is
/// > included in the Privileges Required assessment and is *not* considered
/// > here as a factor of relevance for Attack Complexity.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ModifiedAttackComplexity {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,

    /// High (H)
    ///
    /// > The successful attack depends on the evasion or circumvention of
    /// > security-enhancing techniques in place that would otherwise hinder the
    /// > attack. These include: Evasion of exploit mitigation techniques. The
    /// > attacker must have additional methods available to bypass security
    /// > measures in place. For example, circumvention of **address space
    /// > randomization (ASLR) or data execution prevention (DEP)** must be
    /// > performed for the attack to be successful. Obtaining target-specific
    /// > secrets. The attacker must gather some **target-specific secret**
    /// > before the attack can be successful. A secret is any piece of
    /// > information that cannot be obtained through any amount of
    /// > reconnaissance. To obtain the secret the attacker must perform
    /// > additional attacks or break otherwise secure measures (e.g. knowledge
    /// > of a secret key may be needed to break a crypto channel). This
    /// > operation must be performed for each attacked target.
    High,

    /// Low (L)
    ///
    /// > The attacker must take no measurable action to exploit the
    /// > vulnerability. The attack requires no target-specific circumvention to
    /// > exploit the vulnerability. An attacker can expect repeatable success
    /// > against the vulnerable system.
    Low,
}

impl Default for ModifiedAttackComplexity {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for ModifiedAttackComplexity {
    const TYPE: MetricType = MetricType::MAC;

    fn as_str(self) -> &'static str {
        match self {
            ModifiedAttackComplexity::NotDefined => "X",
            ModifiedAttackComplexity::High => "H",
            ModifiedAttackComplexity::Low => "L",
        }
    }
}

impl fmt::Display for ModifiedAttackComplexity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ModifiedAttackComplexity {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(ModifiedAttackComplexity::NotDefined),
            "H" => Ok(ModifiedAttackComplexity::High),
            "L" => Ok(ModifiedAttackComplexity::Low),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
