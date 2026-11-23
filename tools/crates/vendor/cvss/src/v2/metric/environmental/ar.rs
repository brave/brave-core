//! CVSS v2.0 Environmental Metric - Availability Requirement (AR)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Availability Requirement (AR) - CVSS v2.0 Environmental Metric
///
/// Described in CVSS v2.0 Specification: Section 2.3.3:
/// <https://www.first.org/cvss/v2/guide#2-3-3-Security-Requirements-CR-IR-AR>
///
/// > These metrics enable the analyst to customize the CVSS score depending on
/// > the importance of the affected IT asset to a users organization, measured
/// > in terms of confidentiality, integrity, and availability.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum AvailabilityRequirement {
    /// Low (L)
    /// > Loss of [confidentiality / integrity / availability] is likely to have
    /// > only a limited adverse effect on the organization or individuals
    /// > associated with the organization (e.g., employees, customers).
    Low,

    /// Medium (M)
    /// > Loss of [confidentiality / integrity / availability] is likely to have
    /// > a serious adverse effect on the organization or individuals associated
    /// > with the organization (e.g., employees, customers).
    Medium,

    /// High (H)
    /// > Loss of [confidentiality / integrity / availability] is likely to have
    /// > a catastrophic adverse effect on the organization or individuals
    /// > associated with the organization (e.g., employees, customers).
    High,

    /// Not Defined (ND)
    /// > Assigning this value to the metric will not influence the score. It is
    /// > a signal to the equation to skip this metric.
    NotDefined,
}

impl Metric for AvailabilityRequirement {
    fn score(self) -> f64 {
        match self {
            Self::Low => 0.5,
            Self::Medium => 1.0,
            Self::High => 1.51,
            Self::NotDefined => 1.0,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::Low => "L",
            Self::Medium => "M",
            Self::High => "H",
            Self::NotDefined => "ND",
        }
    }

    const TYPE: MetricType = MetricType::AR;
}

impl fmt::Display for AvailabilityRequirement {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for AvailabilityRequirement {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "L" => Ok(Self::Low),
            "M" => Ok(Self::Medium),
            "H" => Ok(Self::High),
            "ND" => Ok(Self::NotDefined),
            _ => Err(Error::InvalidMetricV2 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
