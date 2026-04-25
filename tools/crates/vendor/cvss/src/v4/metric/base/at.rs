//! Attack Requirements (AT)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Attack Requirements (AT) - CVSS v4.0 Base Metric Group
///
/// Described in CVSS v4.0 Specification: Section 2.1.3
///
/// > This metric captures the prerequisite **deployment and execution
/// > conditions or variables** of the vulnerable system that enable the attack.
/// > These differ from security-enhancing techniques/technologies (ref _Attack
/// > Complexity_) as the primary purpose of these conditions is **not** to
/// > explicitly mitigate attacks, but rather, emerge naturally as a consequence
/// > of the deployment and execution of the vulnerable system. If the attacker
/// > does not take action to overcome these conditions, the attack may succeed
/// > only occasionally or not succeed at all.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum AttackRequirements {
    /// Present (P)
    ///
    /// > The successful attack depends on the presence of specific deployment
    /// > and execution conditions of the vulnerable system that enable the
    /// > attack. These include: A **race condition** must be won to
    /// > successfully exploit the vulnerability. The successfulness of the
    /// > attack is conditioned on execution conditions that are not under full
    /// > control of the attacker. The attack may need to be launched multiple
    /// > times against a single target before being successful. Network
    /// > injection. The attacker must inject themselves into the logical
    /// > network path between the target and the resource requested by the
    /// > victim (e.g. vulnerabilities requiring an on-path attacker).
    Present,

    /// None (N)
    ///
    /// > The successful attack does not depend on the deployment and execution
    /// > conditions of the vulnerable system. The attacker can expect to be
    /// > able to reach the vulnerability and execute the exploit under all or
    /// > most instances of the vulnerability.
    None,
}

impl Default for AttackRequirements {
    fn default() -> Self {
        Self::None
    }
}

impl Metric for AttackRequirements {
    const TYPE: MetricType = MetricType::AT;

    fn as_str(self) -> &'static str {
        match self {
            AttackRequirements::Present => "P",
            AttackRequirements::None => "N",
        }
    }
}

impl fmt::Display for AttackRequirements {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for AttackRequirements {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "P" => Ok(AttackRequirements::Present),
            "N" => Ok(AttackRequirements::None),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}

#[cfg(feature = "std")]
pub(crate) mod merge {
    use super::*;
    use crate::{
        Error,
        v4::{
            MetricType,
            metric::{MetricLevel, environmental::ModifiedAttackRequirements},
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedAttackRequirements {
        Present,
        None,
    }

    impl Default for MergedAttackRequirements {
        fn default() -> Self {
            Self::None
        }
    }

    impl FromStr for MergedAttackRequirements {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "P" => Ok(MergedAttackRequirements::Present),
                "N" => Ok(MergedAttackRequirements::None),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::AT,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl MetricLevel for MergedAttackRequirements {
        fn level(self) -> f64 {
            // AT_levels = {'N': 0.0, 'P': 0.1}
            match self {
                Self::Present => 0.1,
                Self::None => 0.0,
            }
        }
    }

    impl AttackRequirements {
        pub(crate) fn merge(
            self,
            value: Option<ModifiedAttackRequirements>,
        ) -> MergedAttackRequirements {
            match value {
                Some(ModifiedAttackRequirements::NotDefined) | None => match self {
                    Self::Present => MergedAttackRequirements::Present,
                    Self::None => MergedAttackRequirements::None,
                },
                Some(ModifiedAttackRequirements::Present) => MergedAttackRequirements::Present,
                Some(ModifiedAttackRequirements::None) => MergedAttackRequirements::None,
            }
        }
    }
}
