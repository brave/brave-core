//! Integrity Impact (I)

use crate::{Error, Metric, MetricType, Result};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Integrity Impact (I) - CVSS v3.1 Base Metric Group
///
/// Described in CVSS v3.1 Specification: Section 2.3.2:
/// <https://www.first.org/cvss/specification-document#t6>
///
/// > This metric measures the impact to integrity of a successfully exploited
/// > vulnerability. Integrity refers to the trustworthiness and veracity of
/// > information. The Base Score is greatest when the consequence to the
/// > impacted component is highest.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum Integrity {
    /// None (N)
    ///
    /// > There is no loss of integrity within the impacted component.
    None,

    /// Low (L)
    ///
    /// > Modification of data is possible, but the attacker does not have
    /// > control over the consequence of a modification, or the amount of
    /// > modification is limited. The data modification does not have a
    /// > direct, serious impact on the impacted component.
    Low,

    /// High (H)
    ///
    /// > There is a total loss of integrity, or a complete loss of protection.
    /// > For example, the attacker is able to modify any/all files protected
    /// > by the impacted component. Alternatively, only some files can be
    /// > modified, but malicious modification would present a direct, serious
    /// > consequence to the impacted component.
    High,
}

impl Metric for Integrity {
    const TYPE: MetricType = MetricType::I;

    fn score(self) -> f64 {
        match self {
            Integrity::None => 0.0,
            Integrity::Low => 0.22,
            Integrity::High => 0.56,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Integrity::None => "N",
            Integrity::Low => "L",
            Integrity::High => "H",
        }
    }
}

impl fmt::Display for Integrity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for Integrity {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(Integrity::None),
            "L" => Ok(Integrity::Low),
            "H" => Ok(Integrity::High),
            _ => Err(Error::InvalidMetric {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
