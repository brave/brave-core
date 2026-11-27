//! CVSS v2.0 Environmental Metric - Target Distribution (TD)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Target Distribution (TD) - CVSS v2.0 Environmental Metric
///
/// Described in CVSS v2.0 Specification: Section 2.3.2:
/// <https://www.first.org/cvss/v2/guide#2-3-2-Target-Distribution-TD>
///
/// > This metric measures the proportion of vulnerable systems. It is meant as
/// > an environment-specific indicator in order to approximate the percentage
/// > of systems that could be affected by the vulnerability.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum TargetDistribution {
    /// None (N)
    /// > No target systems exist, or targets are so highly specialized that they only exist in a laboratory setting. Effectively 0% of the environment is at risk.
    None,

    /// Low (L)
    /// > Targets exist inside the environment, but on a small scale. Between 1%
    /// > - 25% of the total environment is at risk.
    Low,

    /// Medium (M)
    /// > Targets exist inside the environment, but on a medium scale. Between
    /// > 26% - 75% of the total environment is at risk.
    Medium,

    /// High (H)
    /// > Targets exist inside the environment on a considerable scale. Between
    /// > 76% - 100% of the total environment is considered at risk.
    High,

    /// Not Defined (ND)
    /// > Assigning this value to the metric will not influence the score. It is
    /// > a signal to the equation to skip this metric.
    NotDefined,
}

impl Metric for TargetDistribution {
    fn score(self) -> f64 {
        match self {
            Self::None => 0.0,
            Self::Low => 0.25,
            Self::Medium => 0.75,
            Self::High => 1.0,
            Self::NotDefined => 1.0,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::None => "N",
            Self::Low => "L",
            Self::Medium => "M",
            Self::High => "H",
            Self::NotDefined => "ND",
        }
    }

    const TYPE: MetricType = MetricType::TD;
}

impl fmt::Display for TargetDistribution {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for TargetDistribution {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "N" => Ok(Self::None),
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
