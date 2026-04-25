//! Scope (S)

use crate::{Error, Metric, MetricType, Result};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Scope (S) - CVSS v3.1 Base Metric Group
///
/// Described in CVSS v3.1 Specification: Section 2.2:
/// <https://www.first.org/cvss/specification-document#t8>
///
/// > The Scope metric captures whether a vulnerability in one vulnerable
/// > component impacts resources in components beyond its security scope.
/// >
/// > Formally, a security authority is a mechanism (e.g., an application,
/// > an operating system, firmware, a sandbox environment) that defines and
/// > enforces access control in terms of how certain subjects/actors
/// > (e.g., human users, processes) can access certain restricted
/// > objects/resources (e.g., files, CPU, memory) in a controlled manner.
/// > All the subjects and objects under the jurisdiction of a single security
/// > authority are considered to be under one security scope. If a
/// > vulnerability in a vulnerable component can affect a component which is
/// > in a different security scope than the vulnerable component, a Scope
/// > change occurs. Intuitively, whenever the impact of a vulnerability
/// > breaches a security/trust boundary and impacts components outside the
/// > security scope in which vulnerable component resides, a Scope change occurs.
/// >
/// > The security scope of a component encompasses other components that
/// > provide functionality solely to that component, even if these other
/// > components have their own security authority. For example, a database
/// > used solely by one application is considered part of that application’s
/// > security scope even if the database has its own security authority,
/// > e.g., a mechanism controlling access to database records based on
/// > database users and associated database privileges.
/// >
/// > The Base Score is greatest when a scope change occurs.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum Scope {
    /// Unchanged (U)
    ///
    /// > An exploited vulnerability can only affect resources managed by the
    /// > same security authority. In this case, the vulnerable component and
    /// > the impacted component are either the same, or both are managed by
    /// > the same security authority.
    Unchanged,

    /// Changed (C)
    ///
    /// > An exploited vulnerability can affect resources beyond the security
    /// > scope managed by the security authority of the vulnerable component.
    /// > In this case, the vulnerable component and the impacted component
    /// > are different and managed by different security authorities.
    Changed,
}

impl Scope {
    /// Has the scope changed?
    pub fn is_changed(self) -> bool {
        self == Scope::Changed
    }
}

impl Metric for Scope {
    const TYPE: MetricType = MetricType::S;

    fn score(self) -> f64 {
        unimplemented!()
    }

    fn as_str(self) -> &'static str {
        match self {
            Scope::Unchanged => "U",
            Scope::Changed => "C",
        }
    }
}

impl fmt::Display for Scope {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for Scope {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "U" => Ok(Scope::Unchanged),
            "C" => Ok(Scope::Changed),
            _ => Err(Error::InvalidMetric {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
