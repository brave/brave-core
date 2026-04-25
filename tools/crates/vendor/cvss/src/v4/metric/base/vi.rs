//! Integrity Impact to the Vulnerable System (VI)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Integrity Impact to the Vulnerable System (VI) - CVSS v4.0 Base Metric Group
///
/// Described in CVSS v4.0 Specification: Section 2.2.4
///
/// > This metric measures the impact to integrity of a successfully exploited
/// > vulnerability. Integrity refers to the trustworthiness and veracity of
/// > information. Integrity of a system is impacted when an attacker causes
/// > unauthorized modification of system data. Integrity is also impacted when
/// > a system user can repudiate critical actions taken in the context of the
/// > system (e.g. due to insufficient logging).
/// > The resulting score is greatest when the consequence to the system is
/// > highest.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum IntegrityImpactToTheVulnerableSystem {
    /// None (N)
    ///
    /// > There is no loss of integrity within the Vulnerable System.
    None,

    /// Low (L)
    ///
    /// > Modification of data is possible, but the attacker does not have
    /// > control over the consequence of a modification, or the amount of
    /// > modification is limited. The data modification does not have a direct,
    /// > serious impact to the Vulnerable System.
    Low,

    /// High (H)
    ///
    /// > There is a total loss of integrity, or a complete loss of protection.
    /// > For example, the attacker is able to modify any/all files protected by
    /// > the Vulnerable System. Alternatively, only some files can be modified,
    /// > but malicious modification would present a direct, serious consequence
    /// > to the Vulnerable System.
    High,
}

impl Default for IntegrityImpactToTheVulnerableSystem {
    fn default() -> Self {
        Self::High
    }
}

impl Metric for IntegrityImpactToTheVulnerableSystem {
    const TYPE: MetricType = MetricType::VI;

    fn as_str(self) -> &'static str {
        match self {
            IntegrityImpactToTheVulnerableSystem::None => "N",
            IntegrityImpactToTheVulnerableSystem::Low => "L",
            IntegrityImpactToTheVulnerableSystem::High => "H",
        }
    }
}

impl fmt::Display for IntegrityImpactToTheVulnerableSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for IntegrityImpactToTheVulnerableSystem {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(IntegrityImpactToTheVulnerableSystem::None),
            "L" => Ok(IntegrityImpactToTheVulnerableSystem::Low),
            "H" => Ok(IntegrityImpactToTheVulnerableSystem::High),
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
            metric::{MetricLevel, environmental::ModifiedIntegrityImpactToTheVulnerableSystem},
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedIntegrityImpactToTheVulnerableSystem {
        High,
        Low,
        None,
    }

    impl Default for MergedIntegrityImpactToTheVulnerableSystem {
        fn default() -> Self {
            Self::High
        }
    }

    impl FromStr for MergedIntegrityImpactToTheVulnerableSystem {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "H" => Ok(MergedIntegrityImpactToTheVulnerableSystem::High),
                "L" => Ok(MergedIntegrityImpactToTheVulnerableSystem::Low),
                "N" => Ok(MergedIntegrityImpactToTheVulnerableSystem::None),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::VI,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl MetricLevel for MergedIntegrityImpactToTheVulnerableSystem {
        fn level(self) -> f64 {
            // VI_levels = {'H': 0.0, 'L': 0.1, 'N': 0.2}
            match self {
                Self::High => 0.0,
                Self::Low => 0.1,
                Self::None => 0.2,
            }
        }
    }

    impl IntegrityImpactToTheVulnerableSystem {
        pub(crate) fn merge(
            self,
            value: Option<ModifiedIntegrityImpactToTheVulnerableSystem>,
        ) -> MergedIntegrityImpactToTheVulnerableSystem {
            match value {
                Some(ModifiedIntegrityImpactToTheVulnerableSystem::NotDefined) | None => match self
                {
                    Self::High => MergedIntegrityImpactToTheVulnerableSystem::High,
                    Self::Low => MergedIntegrityImpactToTheVulnerableSystem::Low,
                    Self::None => MergedIntegrityImpactToTheVulnerableSystem::None,
                },
                Some(ModifiedIntegrityImpactToTheVulnerableSystem::High) => {
                    MergedIntegrityImpactToTheVulnerableSystem::High
                }
                Some(ModifiedIntegrityImpactToTheVulnerableSystem::Low) => {
                    MergedIntegrityImpactToTheVulnerableSystem::Low
                }
                Some(ModifiedIntegrityImpactToTheVulnerableSystem::None) => {
                    MergedIntegrityImpactToTheVulnerableSystem::None
                }
            }
        }
    }
}
