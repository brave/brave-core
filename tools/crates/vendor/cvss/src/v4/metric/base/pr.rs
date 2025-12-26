//! Privileges Required (PR)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Privileges Required (PR) - CVSS v4.0 Base Metric Group
///
/// Described in CVSS v4.0 Specification: Section 2.1.4
///
/// > This metric describes the level of privileges an attacker must possess
/// > prior to successfully exploiting the vulnerability. The method by which
/// > the attacker obtains privileged credentials prior to the attack (e.g.,
/// > free trial accounts), is outside the scope of this metric. Generally,
/// > self-service provisioned accounts do not constitute a privilege
/// > requirement if the attacker can grant themselves privileges as part of the
/// > attack.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum PrivilegesRequired {
    /// High (H)
    ///
    /// > The attacker requires privileges that provide significant (e.g.,
    /// > administrative) control over the vulnerable system allowing full
    /// > access to the vulnerable system’s settings and files.
    High,

    /// Low (L)
    ///
    /// > The attacker requires privileges that provide basic capabilities that
    /// > are typically limited to settings and resources owned by a single
    /// > low-privileged user. Alternatively, an attacker with Low privileges
    /// > has the ability to access only non-sensitive resources.
    Low,

    /// None (N)
    ///
    /// > The attacker is unauthenticated prior to attack, and therefore does
    /// > not require any access to settings or files of the vulnerable system
    /// > to carry out an attack.
    None,
}

impl Default for PrivilegesRequired {
    fn default() -> Self {
        Self::None
    }
}

impl Metric for PrivilegesRequired {
    const TYPE: MetricType = MetricType::PR;

    fn as_str(self) -> &'static str {
        match self {
            PrivilegesRequired::None => "N",
            PrivilegesRequired::Low => "L",
            PrivilegesRequired::High => "H",
        }
    }
}

impl fmt::Display for PrivilegesRequired {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for PrivilegesRequired {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(PrivilegesRequired::None),
            "L" => Ok(PrivilegesRequired::Low),
            "H" => Ok(PrivilegesRequired::High),
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
            metric::{MetricLevel, environmental::ModifiedPrivilegesRequired},
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedPrivilegesRequired {
        High,
        Low,
        None,
    }

    impl Default for MergedPrivilegesRequired {
        fn default() -> Self {
            Self::None
        }
    }

    impl FromStr for MergedPrivilegesRequired {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "H" => Ok(MergedPrivilegesRequired::High),
                "L" => Ok(MergedPrivilegesRequired::Low),
                "N" => Ok(MergedPrivilegesRequired::None),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::PR,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl MetricLevel for MergedPrivilegesRequired {
        fn level(self) -> f64 {
            // PR_levels = {"N": 0.0, "L": 0.1, "H": 0.2}
            match self {
                Self::High => 0.2,
                Self::Low => 0.1,
                Self::None => 0.0,
            }
        }
    }

    impl PrivilegesRequired {
        pub(crate) fn merge(
            self,
            value: Option<ModifiedPrivilegesRequired>,
        ) -> MergedPrivilegesRequired {
            match value {
                Some(ModifiedPrivilegesRequired::NotDefined) | None => match self {
                    Self::High => MergedPrivilegesRequired::High,
                    Self::Low => MergedPrivilegesRequired::Low,
                    Self::None => MergedPrivilegesRequired::None,
                },
                Some(ModifiedPrivilegesRequired::High) => MergedPrivilegesRequired::High,
                Some(ModifiedPrivilegesRequired::Low) => MergedPrivilegesRequired::Low,
                Some(ModifiedPrivilegesRequired::None) => MergedPrivilegesRequired::None,
            }
        }
    }
}
