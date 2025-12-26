//! Privileges Required (MPR)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Privileges Required (MPR) - CVSS v4.0 Environmental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 4.2
///
/// > This metric describes the level of privileges an attacker must possess
/// > prior to successfully exploiting the vulnerability. The method by which
/// > the attacker obtains privileged credentials prior to the attack (e.g.,
/// > free trial accounts), is outside the scope of this metric. Generally,
/// > self-service provisioned accounts do not constitute a privilege
/// > requirement if the attacker can grant themselves privileges as part of the
/// > attack.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ModifiedPrivilegesRequired {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,

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

impl Default for ModifiedPrivilegesRequired {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for ModifiedPrivilegesRequired {
    const TYPE: MetricType = MetricType::MPR;

    fn as_str(self) -> &'static str {
        match self {
            ModifiedPrivilegesRequired::NotDefined => "X",
            ModifiedPrivilegesRequired::None => "N",
            ModifiedPrivilegesRequired::Low => "L",
            ModifiedPrivilegesRequired::High => "H",
        }
    }
}

impl fmt::Display for ModifiedPrivilegesRequired {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ModifiedPrivilegesRequired {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(ModifiedPrivilegesRequired::NotDefined),
            "N" => Ok(ModifiedPrivilegesRequired::None),
            "L" => Ok(ModifiedPrivilegesRequired::Low),
            "H" => Ok(ModifiedPrivilegesRequired::High),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
