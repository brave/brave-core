//! Availability Impact to the Vulnerable System (VA)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Availability Impact to the Vulnerable System (VA) - CVSS v4.0 Base Metric
/// Group
///
/// Described in CVSS v4.0 Specification: Section 2.2.7
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
pub enum AvailabilityImpactToTheVulnerableSystem {
    /// None (N)
    ///
    /// > There is no impact to availability within the Vulnerable System.
    None,

    /// Low (L)
    ///
    /// > Performance is reduced or there are interruptions in resource
    /// > availability. Even if repeated exploitation of the vulnerability is
    /// > possible, the attacker does not have the ability to completely deny
    /// > service to legitimate users. The resources in the Vulnerable System
    /// > are either partially available all of the time, or fully available
    /// > only some of the time, but overall there is no direct, serious
    /// > consequence to the Vulnerable System.
    Low,

    /// High (H)
    ///
    /// > There is a total loss of availability, resulting in the attacker being
    /// > able to fully deny access to resources in the Vulnerable System; this
    /// > loss is either sustained (while the attacker continues to deliver the
    /// > attack) or persistent (the condition persists even after the attack
    /// > has completed). Alternatively, the attacker has the ability to deny
    /// > some availability, but the loss of availability presents a direct,
    /// > serious consequence to the Vulnerable System (e.g., the attacker
    /// > cannot disrupt existing connections, but can prevent new connections;
    /// > the attacker can repeatedly exploit a vulnerability that, in each
    /// > instance of a successful attack, leaks a only small amount of memory,
    /// > but after repeated exploitation causes a service to become completely
    /// > unavailable).
    High,
}

impl Default for AvailabilityImpactToTheVulnerableSystem {
    fn default() -> Self {
        Self::High
    }
}

impl Metric for AvailabilityImpactToTheVulnerableSystem {
    const TYPE: MetricType = MetricType::VA;

    fn as_str(self) -> &'static str {
        match self {
            AvailabilityImpactToTheVulnerableSystem::None => "N",
            AvailabilityImpactToTheVulnerableSystem::Low => "L",
            AvailabilityImpactToTheVulnerableSystem::High => "H",
        }
    }
}

impl fmt::Display for AvailabilityImpactToTheVulnerableSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for AvailabilityImpactToTheVulnerableSystem {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(AvailabilityImpactToTheVulnerableSystem::None),
            "L" => Ok(AvailabilityImpactToTheVulnerableSystem::Low),
            "H" => Ok(AvailabilityImpactToTheVulnerableSystem::High),
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
            metric::{MetricLevel, environmental::ModifiedAvailabilityImpactToTheVulnerableSystem},
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedAvailabilityImpactToTheVulnerableSystem {
        High,
        Low,
        None,
    }

    impl Default for MergedAvailabilityImpactToTheVulnerableSystem {
        fn default() -> Self {
            Self::High
        }
    }

    impl FromStr for MergedAvailabilityImpactToTheVulnerableSystem {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "H" => Ok(MergedAvailabilityImpactToTheVulnerableSystem::High),
                "L" => Ok(MergedAvailabilityImpactToTheVulnerableSystem::Low),
                "N" => Ok(MergedAvailabilityImpactToTheVulnerableSystem::None),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::VA,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl MetricLevel for MergedAvailabilityImpactToTheVulnerableSystem {
        fn level(self) -> f64 {
            // VA_levels = {'H': 0.0, 'L': 0.1, 'N': 0.2}
            match self {
                Self::High => 0.0,
                Self::Low => 0.1,
                Self::None => 0.2,
            }
        }
    }

    impl AvailabilityImpactToTheVulnerableSystem {
        pub(crate) fn merge(
            self,
            value: Option<ModifiedAvailabilityImpactToTheVulnerableSystem>,
        ) -> MergedAvailabilityImpactToTheVulnerableSystem {
            match value {
                Some(ModifiedAvailabilityImpactToTheVulnerableSystem::NotDefined) | None => {
                    match self {
                        Self::High => MergedAvailabilityImpactToTheVulnerableSystem::High,
                        Self::Low => MergedAvailabilityImpactToTheVulnerableSystem::Low,
                        Self::None => MergedAvailabilityImpactToTheVulnerableSystem::None,
                    }
                }
                Some(ModifiedAvailabilityImpactToTheVulnerableSystem::High) => {
                    MergedAvailabilityImpactToTheVulnerableSystem::High
                }
                Some(ModifiedAvailabilityImpactToTheVulnerableSystem::Low) => {
                    MergedAvailabilityImpactToTheVulnerableSystem::Low
                }
                Some(ModifiedAvailabilityImpactToTheVulnerableSystem::None) => {
                    MergedAvailabilityImpactToTheVulnerableSystem::None
                }
            }
        }
    }
}
