//! Confidentiality Impact to the Subsequent System (SC)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Confidentiality Impact to the Subsequent System (SC) - CVSS v4.0 Base Metric
/// Group
///
/// Described in CVSS v4.0 Specification: Section 2.2.3
///
/// > This metric measures the impact to the confidentiality of the information
/// > managed by the system due to a successfully exploited vulnerability.
/// > Confidentiality refers to limiting information access and disclosure to
/// > only authorized users, as well as preventing access by, or disclosure to,
/// > unauthorized ones. The resulting score is greatest when the loss to the
/// > system is highest.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ConfidentialityImpactToTheSubsequentSystem {
    /// None (N)
    ///
    /// > There is no loss of confidentiality within the Subsequent System or
    /// > all confidentiality impact is constrained to the Vulnerable System.
    None,

    /// Low (L)
    ///
    /// > There is some loss of confidentiality. Access to some restricted
    /// > information is obtained, but the attacker does not have control over
    /// > what information is obtained, or the amount or kind of loss is
    /// > limited. The information disclosure does not cause a direct, serious
    /// > loss to the Subsequent System.
    Low,

    /// High (H)
    ///
    /// > There is a total loss of confidentiality, resulting in all resources
    /// > within the Subsequent System being divulged to the attacker.
    /// > Alternatively, access to only some restricted information is obtained,
    /// > but the disclosed information presents a direct, serious impact. For
    /// > example, an attacker steals the administrator's password, or private
    /// > encryption keys of a web server.
    High,
}

impl Default for ConfidentialityImpactToTheSubsequentSystem {
    fn default() -> Self {
        Self::High
    }
}

impl Metric for ConfidentialityImpactToTheSubsequentSystem {
    const TYPE: MetricType = MetricType::SC;

    fn as_str(self) -> &'static str {
        match self {
            ConfidentialityImpactToTheSubsequentSystem::None => "N",
            ConfidentialityImpactToTheSubsequentSystem::Low => "L",
            ConfidentialityImpactToTheSubsequentSystem::High => "H",
        }
    }
}

impl fmt::Display for ConfidentialityImpactToTheSubsequentSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ConfidentialityImpactToTheSubsequentSystem {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(ConfidentialityImpactToTheSubsequentSystem::None),
            "L" => Ok(ConfidentialityImpactToTheSubsequentSystem::Low),
            "H" => Ok(ConfidentialityImpactToTheSubsequentSystem::High),
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
            metric::{
                MetricLevel, environmental::ModifiedConfidentialityImpactToTheSubsequentSystem,
            },
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedConfidentialityImpactToTheSubsequentSystem {
        High,
        Low,
        None,
    }

    impl Default for MergedConfidentialityImpactToTheSubsequentSystem {
        fn default() -> Self {
            Self::High
        }
    }

    impl FromStr for MergedConfidentialityImpactToTheSubsequentSystem {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "H" => Ok(MergedConfidentialityImpactToTheSubsequentSystem::High),
                "L" => Ok(MergedConfidentialityImpactToTheSubsequentSystem::Low),
                "N" => Ok(MergedConfidentialityImpactToTheSubsequentSystem::None),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::SC,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl MetricLevel for MergedConfidentialityImpactToTheSubsequentSystem {
        fn level(self) -> f64 {
            // SC_levels = {'H': 0.1, 'L': 0.2, 'N': 0.3}
            match self {
                Self::High => 0.1,
                Self::Low => 0.2,
                Self::None => 0.3,
            }
        }
    }

    impl ConfidentialityImpactToTheSubsequentSystem {
        pub(crate) fn merge(
            self,
            value: Option<ModifiedConfidentialityImpactToTheSubsequentSystem>,
        ) -> MergedConfidentialityImpactToTheSubsequentSystem {
            match value {
                Some(ModifiedConfidentialityImpactToTheSubsequentSystem::NotDefined) | None => {
                    match self {
                        Self::High => MergedConfidentialityImpactToTheSubsequentSystem::High,
                        Self::Low => MergedConfidentialityImpactToTheSubsequentSystem::Low,
                        Self::None => MergedConfidentialityImpactToTheSubsequentSystem::None,
                    }
                }
                Some(ModifiedConfidentialityImpactToTheSubsequentSystem::High) => {
                    MergedConfidentialityImpactToTheSubsequentSystem::High
                }
                Some(ModifiedConfidentialityImpactToTheSubsequentSystem::Low) => {
                    MergedConfidentialityImpactToTheSubsequentSystem::Low
                }
                Some(ModifiedConfidentialityImpactToTheSubsequentSystem::Negligible) => {
                    MergedConfidentialityImpactToTheSubsequentSystem::None
                }
            }
        }
    }
}
