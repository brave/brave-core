//! Confidentiality Impact (C)

use crate::{Error, Metric, MetricType};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Confidentiality Impact (C) - CVSS v3.1 Base Metric Group
///
/// Described in CVSS v3.1 Specification: Section 2.3.1:
/// <https://www.first.org/cvss/specification-document#t6>
///
/// > This metric measures the impact to the confidentiality of the information
/// > resources managed by a software component due to a successfully exploited
/// > vulnerability. Confidentiality refers to limiting information access and
/// > disclosure to only authorized users, as well as preventing access by, or
/// > disclosure to, unauthorized ones. The Base Score is greatest when the loss
/// > to the impacted component is highest.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum Confidentiality {
    /// None (N)
    ///
    /// > There is no loss of confidentiality within the impacted component.
    None,

    /// Low (L)
    ///
    /// > There is some loss of confidentiality. Access to some restricted
    /// > information is obtained, but the attacker does not have control over
    /// > what information is obtained, or the amount or kind of loss is
    /// > limited. The information disclosure does not cause a direct, serious
    /// > loss to the impacted component.
    Low,

    /// High (H)
    ///
    /// > There is a total loss of confidentiality, resulting in all resources
    /// > within the impacted component being divulged to the attacker.
    /// > Alternatively, access to only some restricted information is
    /// > obtained, but the disclosed information presents a direct, serious
    /// > impact. For example, an attacker steals the administrator's password,
    /// > or private encryption keys of a web server.
    High,
}

impl Metric for Confidentiality {
    const TYPE: MetricType = MetricType::C;

    fn score(self) -> f64 {
        match self {
            Confidentiality::None => 0.0,
            Confidentiality::Low => 0.22,
            Confidentiality::High => 0.56,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Confidentiality::None => "N",
            Confidentiality::Low => "L",
            Confidentiality::High => "H",
        }
    }
}

impl fmt::Display for Confidentiality {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for Confidentiality {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "N" => Ok(Confidentiality::None),
            "L" => Ok(Confidentiality::Low),
            "H" => Ok(Confidentiality::High),
            _ => Err(Error::InvalidMetric {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
