//! Availability Impact to the Subsequent System (MSA)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Availability Impact to the Subsequent System (MSA) - CVSS v4.0 Environmental
/// Metric Group
///
/// Described in CVSS v4.0 Specification: Section 4.2
///
/// > This metric measures the impact to the availability of the impacted system
/// > resulting from a successfully exploited vulnerability. While the
/// > Confidentiality and Integrity impact metrics apply to the loss of
/// > confidentiality or integrity of data (e.g., information, files) used by
/// > the system, this metric refers to the loss of availability of the impacted
/// > system itself, such as a networked service (e.g., web, database, email).
/// > Since availability refers to the accessibility of information resources,
/// > attacks that consume network bandwidth, processor cycles, or disk space
/// > all impact the availability of a system. The resulting score is greatest
/// > when the consequence to the system is highest.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ModifiedAvailabilityImpactToTheSubsequentSystem {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,

    /// Negligible (N)
    ///
    /// > There is no impact to availability within the Subsequent System or all
    /// > availability impact is constrained to the Vulnerable System.
    Negligible,

    /// Low (L)
    ///
    /// > Performance is reduced or there are interruptions in resource
    /// > availability. Even if repeated exploitation of the vulnerability is
    /// > possible, the attacker does not have the ability to completely deny
    /// > service to legitimate users. The resources in the Subsequent System
    /// > are either partially available all of the time, or fully available
    /// > only some of the time, but overall there is no direct, serious
    /// > consequence to the Subsequent System.
    Low,

    /// High (H)
    ///
    /// > There is a total loss of availability, resulting in the attacker being
    /// > able to fully deny access to resources in the Subsequent System; this
    /// > loss is either sustained (while the attacker continues to deliver the
    /// > attack) or persistent (the condition persists even after the attack
    /// > has completed). Alternatively, the attacker has the ability to deny
    /// > some availability, but the loss of availability presents a direct,
    /// > serious consequence to the Subsequent System (e.g., the attacker
    /// > cannot disrupt existing connections, but can prevent new connections;
    /// > the attacker can repeatedly exploit a vulnerability that, in each
    /// > instance of a successful attack, leaks a only small amount of memory,
    /// > but after repeated exploitation causes a service to become completely
    /// > unavailable).
    High,

    /// Safety (S)
    Safety,
}

impl Default for ModifiedAvailabilityImpactToTheSubsequentSystem {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for ModifiedAvailabilityImpactToTheSubsequentSystem {
    const TYPE: MetricType = MetricType::MSA;

    fn as_str(self) -> &'static str {
        match self {
            ModifiedAvailabilityImpactToTheSubsequentSystem::NotDefined => "X",
            ModifiedAvailabilityImpactToTheSubsequentSystem::Negligible => "N",
            ModifiedAvailabilityImpactToTheSubsequentSystem::Low => "L",
            ModifiedAvailabilityImpactToTheSubsequentSystem::High => "H",
            ModifiedAvailabilityImpactToTheSubsequentSystem::Safety => "S",
        }
    }
}

impl fmt::Display for ModifiedAvailabilityImpactToTheSubsequentSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ModifiedAvailabilityImpactToTheSubsequentSystem {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(ModifiedAvailabilityImpactToTheSubsequentSystem::NotDefined),
            "N" => Ok(ModifiedAvailabilityImpactToTheSubsequentSystem::Negligible),
            "L" => Ok(ModifiedAvailabilityImpactToTheSubsequentSystem::Low),
            "H" => Ok(ModifiedAvailabilityImpactToTheSubsequentSystem::High),
            "S" => Ok(ModifiedAvailabilityImpactToTheSubsequentSystem::Safety),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
