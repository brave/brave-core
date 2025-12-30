//! Availability Impact to the Subsequent System (SA)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Availability Impact to the Subsequent System (SA) - CVSS v4.0 Base Metric
/// Group
///
/// Described in CVSS v4.0 Specification: Section 2.2.8
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
pub enum AvailabilityImpactToTheSubsequentSystem {
    /// None (N)
    ///
    /// > There is no impact to availability within the Subsequent System or all
    /// > availability impact is constrained to the Vulnerable System.
    None,

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
}

impl Default for AvailabilityImpactToTheSubsequentSystem {
    fn default() -> Self {
        Self::High
    }
}

impl Metric for AvailabilityImpactToTheSubsequentSystem {
    const TYPE: MetricType = MetricType::SA;

    fn as_str(self) -> &'static str {
        match self {
            AvailabilityImpactToTheSubsequentSystem::None => "N",
            AvailabilityImpactToTheSubsequentSystem::Low => "L",
            AvailabilityImpactToTheSubsequentSystem::High => "H",
        }
    }
}

impl fmt::Display for AvailabilityImpactToTheSubsequentSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for AvailabilityImpactToTheSubsequentSystem {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(AvailabilityImpactToTheSubsequentSystem::None),
            "L" => Ok(AvailabilityImpactToTheSubsequentSystem::Low),
            "H" => Ok(AvailabilityImpactToTheSubsequentSystem::High),
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
            metric::{MetricLevel, environmental::ModifiedAvailabilityImpactToTheSubsequentSystem},
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedAvailabilityImpactToTheSubsequentSystem {
        Safety,
        High,
        Low,
        None,
    }

    impl Default for MergedAvailabilityImpactToTheSubsequentSystem {
        fn default() -> Self {
            Self::High
        }
    }

    impl FromStr for MergedAvailabilityImpactToTheSubsequentSystem {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "S" => Ok(MergedAvailabilityImpactToTheSubsequentSystem::Safety),
                "H" => Ok(MergedAvailabilityImpactToTheSubsequentSystem::High),
                "L" => Ok(MergedAvailabilityImpactToTheSubsequentSystem::Low),
                "N" => Ok(MergedAvailabilityImpactToTheSubsequentSystem::None),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::SA,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl MetricLevel for MergedAvailabilityImpactToTheSubsequentSystem {
        fn level(self) -> f64 {
            // SA_levels = {'S': 0.0, 'H': 0.1, 'L': 0.2, 'N': 0.3}
            match self {
                Self::Safety => 0.0,
                Self::High => 0.1,
                Self::Low => 0.2,
                Self::None => 0.3,
            }
        }
    }

    impl AvailabilityImpactToTheSubsequentSystem {
        pub(crate) fn merge(
            self,
            value: Option<ModifiedAvailabilityImpactToTheSubsequentSystem>,
        ) -> MergedAvailabilityImpactToTheSubsequentSystem {
            match value {
                Some(ModifiedAvailabilityImpactToTheSubsequentSystem::NotDefined) | None => {
                    match self {
                        Self::High => MergedAvailabilityImpactToTheSubsequentSystem::High,
                        Self::Low => MergedAvailabilityImpactToTheSubsequentSystem::Low,
                        Self::None => MergedAvailabilityImpactToTheSubsequentSystem::None,
                    }
                }
                Some(ModifiedAvailabilityImpactToTheSubsequentSystem::High) => {
                    MergedAvailabilityImpactToTheSubsequentSystem::High
                }
                Some(ModifiedAvailabilityImpactToTheSubsequentSystem::Low) => {
                    MergedAvailabilityImpactToTheSubsequentSystem::Low
                }
                Some(ModifiedAvailabilityImpactToTheSubsequentSystem::Negligible) => {
                    MergedAvailabilityImpactToTheSubsequentSystem::None
                }
                Some(ModifiedAvailabilityImpactToTheSubsequentSystem::Safety) => {
                    MergedAvailabilityImpactToTheSubsequentSystem::Safety
                }
            }
        }
    }
}
