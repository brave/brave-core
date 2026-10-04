//! Integrity Impact to the Subsequent System (MSI)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Integrity Impact to the Subsequent System (MSI) - CVSS v4.0 Environmental
/// Metric Group
///
/// Described in CVSS v4.0 Specification: Section 4.2
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
pub enum ModifiedIntegrityImpactToTheSubsequentSystem {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,

    /// Negligible (N)
    ///
    /// > There is no loss of integrity within the Subsequent System or all
    /// > integrity impact is constrained to the Vulnerable System.
    Negligible,

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

    /// Safety (S)
    Safety,
}

impl Default for ModifiedIntegrityImpactToTheSubsequentSystem {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for ModifiedIntegrityImpactToTheSubsequentSystem {
    const TYPE: MetricType = MetricType::MSI;

    fn as_str(self) -> &'static str {
        match self {
            ModifiedIntegrityImpactToTheSubsequentSystem::NotDefined => "X",
            ModifiedIntegrityImpactToTheSubsequentSystem::Negligible => "N",
            ModifiedIntegrityImpactToTheSubsequentSystem::Low => "L",
            ModifiedIntegrityImpactToTheSubsequentSystem::High => "H",
            ModifiedIntegrityImpactToTheSubsequentSystem::Safety => "S",
        }
    }
}

impl fmt::Display for ModifiedIntegrityImpactToTheSubsequentSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ModifiedIntegrityImpactToTheSubsequentSystem {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(ModifiedIntegrityImpactToTheSubsequentSystem::NotDefined),
            "N" => Ok(ModifiedIntegrityImpactToTheSubsequentSystem::Negligible),
            "L" => Ok(ModifiedIntegrityImpactToTheSubsequentSystem::Low),
            "H" => Ok(ModifiedIntegrityImpactToTheSubsequentSystem::High),
            "S" => Ok(ModifiedIntegrityImpactToTheSubsequentSystem::Safety),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
