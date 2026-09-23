//! Integrity Impact to the Subsequent System (SI)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Integrity Impact to the Subsequent System (SI) - CVSS v4.0 Base Metric Group
///
/// Described in CVSS v4.0 Specification: Section 2.2.5
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
pub enum IntegrityImpactToTheSubsequentSystem {
    /// None (N)
    ///
    /// > There is no loss of integrity within the Subsequent System or all
    /// > integrity impact is constrained to the Vulnerable System.
    None,

    /// Low (L)
    ///
    /// > Modification of data is possible, but the attacker does not have
    /// > control over the consequence of a modification, or the amount of
    /// > modification is limited. The data modification does not have a direct,
    /// > serious impact to the Subsequent System.
    Low,

    /// High (H)
    ///
    /// > There is a total loss of integrity, or a complete loss of protection.
    /// > For example, the attacker is able to modify any/all files protected by
    /// > the Subsequent System. Alternatively, only some files can be modified,
    /// > but malicious modification would present a direct, serious consequence
    /// > to the Subsequent System.
    High,
}

impl Default for IntegrityImpactToTheSubsequentSystem {
    fn default() -> Self {
        Self::High
    }
}

impl Metric for IntegrityImpactToTheSubsequentSystem {
    const TYPE: MetricType = MetricType::SI;

    fn as_str(self) -> &'static str {
        match self {
            IntegrityImpactToTheSubsequentSystem::None => "N",
            IntegrityImpactToTheSubsequentSystem::Low => "L",
            IntegrityImpactToTheSubsequentSystem::High => "H",
        }
    }
}

impl fmt::Display for IntegrityImpactToTheSubsequentSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for IntegrityImpactToTheSubsequentSystem {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(IntegrityImpactToTheSubsequentSystem::None),
            "L" => Ok(IntegrityImpactToTheSubsequentSystem::Low),
            "H" => Ok(IntegrityImpactToTheSubsequentSystem::High),
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
            metric::{MetricLevel, environmental::ModifiedIntegrityImpactToTheSubsequentSystem},
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedIntegrityImpactToTheSubsequentSystem {
        Safety,
        High,
        Low,
        None,
    }

    impl Default for MergedIntegrityImpactToTheSubsequentSystem {
        fn default() -> Self {
            Self::High
        }
    }

    impl FromStr for MergedIntegrityImpactToTheSubsequentSystem {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "S" => Ok(MergedIntegrityImpactToTheSubsequentSystem::Safety),
                "H" => Ok(MergedIntegrityImpactToTheSubsequentSystem::High),
                "L" => Ok(MergedIntegrityImpactToTheSubsequentSystem::Low),
                "N" => Ok(MergedIntegrityImpactToTheSubsequentSystem::None),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::SI,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl MetricLevel for MergedIntegrityImpactToTheSubsequentSystem {
        fn level(self) -> f64 {
            // SI_levels = {'S': 0.0, 'H': 0.1, 'L': 0.2, 'N': 0.3}
            match self {
                Self::Safety => 0.0,
                Self::High => 0.1,
                Self::Low => 0.2,
                Self::None => 0.3,
            }
        }
    }

    impl IntegrityImpactToTheSubsequentSystem {
        pub(crate) fn merge(
            self,
            value: Option<ModifiedIntegrityImpactToTheSubsequentSystem>,
        ) -> MergedIntegrityImpactToTheSubsequentSystem {
            match value {
                Some(ModifiedIntegrityImpactToTheSubsequentSystem::NotDefined) | None => match self
                {
                    Self::High => MergedIntegrityImpactToTheSubsequentSystem::High,
                    Self::Low => MergedIntegrityImpactToTheSubsequentSystem::Low,
                    Self::None => MergedIntegrityImpactToTheSubsequentSystem::None,
                },
                Some(ModifiedIntegrityImpactToTheSubsequentSystem::High) => {
                    MergedIntegrityImpactToTheSubsequentSystem::High
                }
                Some(ModifiedIntegrityImpactToTheSubsequentSystem::Low) => {
                    MergedIntegrityImpactToTheSubsequentSystem::Low
                }
                Some(ModifiedIntegrityImpactToTheSubsequentSystem::Negligible) => {
                    MergedIntegrityImpactToTheSubsequentSystem::None
                }
                Some(ModifiedIntegrityImpactToTheSubsequentSystem::Safety) => {
                    MergedIntegrityImpactToTheSubsequentSystem::Safety
                }
            }
        }
    }
}
