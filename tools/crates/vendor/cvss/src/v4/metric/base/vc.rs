//! Confidentiality Impact to the Vulnerable System (VC)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Confidentiality Impact to the Vulnerable System (VC) - CVSS v4.0 Base Metric
/// Group
///
/// Described in CVSS v4.0 Specification: Section 2.2.2
///
/// > This metric measures the impact to the confidentiality of the information
/// > managed by the system due to a successfully exploited vulnerability.
/// > Confidentiality refers to limiting information access and disclosure to
/// > only authorized users, as well as preventing access by, or disclosure to,
/// > unauthorized ones. The resulting score is greatest when the loss to the
/// > system is highest.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ConfidentialityImpactToTheVulnerableSystem {
    /// None (N)
    ///
    /// > There is no loss of confidentiality within the Vulnerable System.
    None,

    /// Low (L)
    ///
    /// > There is some loss of confidentiality. Access to some restricted
    /// > information is obtained, but the attacker does not have control over
    /// > what information is obtained, or the amount or kind of loss is
    /// > limited. The information disclosure does not cause a direct, serious
    /// > loss to the Vulnerable System.
    Low,

    /// High (H)
    ///
    /// > There is a total loss of confidentiality, resulting in all information
    /// > within the Vulnerable System being divulged to the attacker.
    /// > Alternatively, access to only some restricted information is obtained,
    /// > but the disclosed information presents a direct, serious impact. For
    /// > example, an attacker steals the administrator's password, or private
    /// > encryption keys of a web server.
    High,
}

impl Default for ConfidentialityImpactToTheVulnerableSystem {
    fn default() -> Self {
        Self::High
    }
}

impl Metric for ConfidentialityImpactToTheVulnerableSystem {
    const TYPE: MetricType = MetricType::VC;

    fn as_str(self) -> &'static str {
        match self {
            ConfidentialityImpactToTheVulnerableSystem::None => "N",
            ConfidentialityImpactToTheVulnerableSystem::Low => "L",
            ConfidentialityImpactToTheVulnerableSystem::High => "H",
        }
    }
}

impl fmt::Display for ConfidentialityImpactToTheVulnerableSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ConfidentialityImpactToTheVulnerableSystem {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(ConfidentialityImpactToTheVulnerableSystem::None),
            "L" => Ok(ConfidentialityImpactToTheVulnerableSystem::Low),
            "H" => Ok(ConfidentialityImpactToTheVulnerableSystem::High),
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
                MetricLevel, environmental::ModifiedConfidentialityImpactToTheVulnerableSystem,
            },
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedConfidentialityImpactToTheVulnerableSystem {
        High,
        Low,
        None,
    }

    impl Default for MergedConfidentialityImpactToTheVulnerableSystem {
        fn default() -> Self {
            Self::High
        }
    }

    impl MetricLevel for MergedConfidentialityImpactToTheVulnerableSystem {
        fn level(self) -> f64 {
            // VC_levels = {'H': 0.0, 'L': 0.1, 'N': 0.2}
            match self {
                Self::High => 0.0,
                Self::Low => 0.1,
                Self::None => 0.2,
            }
        }
    }

    impl FromStr for MergedConfidentialityImpactToTheVulnerableSystem {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "H" => Ok(MergedConfidentialityImpactToTheVulnerableSystem::High),
                "L" => Ok(MergedConfidentialityImpactToTheVulnerableSystem::Low),
                "N" => Ok(MergedConfidentialityImpactToTheVulnerableSystem::None),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::VC,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl ConfidentialityImpactToTheVulnerableSystem {
        pub(crate) fn merge(
            self,
            value: Option<ModifiedConfidentialityImpactToTheVulnerableSystem>,
        ) -> MergedConfidentialityImpactToTheVulnerableSystem {
            match value {
                Some(ModifiedConfidentialityImpactToTheVulnerableSystem::NotDefined) | None => {
                    match self {
                        Self::High => MergedConfidentialityImpactToTheVulnerableSystem::High,
                        Self::Low => MergedConfidentialityImpactToTheVulnerableSystem::Low,
                        Self::None => MergedConfidentialityImpactToTheVulnerableSystem::None,
                    }
                }
                Some(ModifiedConfidentialityImpactToTheVulnerableSystem::High) => {
                    MergedConfidentialityImpactToTheVulnerableSystem::High
                }
                Some(ModifiedConfidentialityImpactToTheVulnerableSystem::Low) => {
                    MergedConfidentialityImpactToTheVulnerableSystem::Low
                }
                Some(ModifiedConfidentialityImpactToTheVulnerableSystem::None) => {
                    MergedConfidentialityImpactToTheVulnerableSystem::None
                }
            }
        }
    }
}
