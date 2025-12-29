//! Confidentiality Impact to the Vulnerable System (MVC)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Confidentiality Impact to the Vulnerable System (MVC) - CVSS v4.0
/// Environmental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 4.2
///
/// > This metric measures the impact to the confidentiality of the information
/// > managed by the system due to a successfully exploited vulnerability.
/// > Confidentiality refers to limiting information access and disclosure to
/// > only authorized users, as well as preventing access by, or disclosure to,
/// > unauthorized ones. The resulting score is greatest when the loss to the
/// > system is highest.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ModifiedConfidentialityImpactToTheVulnerableSystem {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,
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

impl Default for ModifiedConfidentialityImpactToTheVulnerableSystem {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for ModifiedConfidentialityImpactToTheVulnerableSystem {
    const TYPE: MetricType = MetricType::MVC;

    fn as_str(self) -> &'static str {
        match self {
            ModifiedConfidentialityImpactToTheVulnerableSystem::NotDefined => "X",
            ModifiedConfidentialityImpactToTheVulnerableSystem::None => "N",
            ModifiedConfidentialityImpactToTheVulnerableSystem::Low => "L",
            ModifiedConfidentialityImpactToTheVulnerableSystem::High => "H",
        }
    }
}

impl fmt::Display for ModifiedConfidentialityImpactToTheVulnerableSystem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ModifiedConfidentialityImpactToTheVulnerableSystem {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(ModifiedConfidentialityImpactToTheVulnerableSystem::NotDefined),
            "N" => Ok(ModifiedConfidentialityImpactToTheVulnerableSystem::None),
            "L" => Ok(ModifiedConfidentialityImpactToTheVulnerableSystem::Low),
            "H" => Ok(ModifiedConfidentialityImpactToTheVulnerableSystem::High),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
