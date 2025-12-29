//! Privileges Required (PR)

use crate::{Metric, MetricType, error::Error};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Privileges Required (PR) - CVSS v3.1 Base Metric Group
///
/// Described in CVSS v3.1 Specification: Section 2.1.3:
/// <https://www.first.org/cvss/specification-document#t6>
///
/// > This metric describes the level of privileges an attacker must possess
/// > *before* successfully exploiting the vulnerability. The Base Score is
/// > greatest if no privileges are required.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum PrivilegesRequired {
    /// High (H)
    ///
    /// > The attacker requires privileges that provide significant
    /// > (e.g., administrative) control over the vulnerable component allowing
    /// > access to component-wide settings and files.
    High,

    /// Low (L)
    ///
    /// > The attacker requires privileges that provide basic user capabilities
    /// > that could normally affect only settings and files owned by a user.
    /// > Alternatively, an attacker with Low privileges has the ability to
    /// > access only non-sensitive resources.
    Low,

    /// None (N)
    ///
    /// > The attacker is unauthorized prior to attack, and therefore does not
    /// > require any access to settings or files of the vulnerable system
    /// > to carry out an attack.
    None,
}

impl PrivilegesRequired {
    /// Score when accounting for scope change
    pub fn scoped_score(self, scope_change: bool) -> f64 {
        match self {
            PrivilegesRequired::High => {
                if scope_change {
                    0.50
                } else {
                    0.27
                }
            }
            PrivilegesRequired::Low => {
                if scope_change {
                    0.68
                } else {
                    0.62
                }
            }
            PrivilegesRequired::None => 0.85,
        }
    }
}

impl Metric for PrivilegesRequired {
    const TYPE: MetricType = MetricType::PR;

    fn score(self) -> f64 {
        self.scoped_score(false)
    }

    fn as_str(self) -> &'static str {
        match self {
            PrivilegesRequired::High => "H",
            PrivilegesRequired::Low => "L",
            PrivilegesRequired::None => "N",
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

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "H" => Ok(PrivilegesRequired::High),
            "L" => Ok(PrivilegesRequired::Low),
            "N" => Ok(PrivilegesRequired::None),
            _ => Err(Error::InvalidMetric {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
