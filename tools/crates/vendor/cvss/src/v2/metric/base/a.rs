//! CVSS v2.0 Base Metric Group - Availability Impact (C)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Availability Impact (C) - CVSS v2.0 Base Metric Group
///
/// Described in CVSS v2.0 Specification: Section 2.1.6:
/// <https://www.first.org/cvss/v2/guide#2-1-6-Availability-Impact-A>
///
/// > This metric measures the impact to availability of a successfully
/// > exploited vulnerability. Availability refers to the accessibility of
/// > information resources. Attacks that consume network bandwidth, processor
/// > cycles, or disk space all impact the availability of a system. The
/// > possible values for this metric are listed in Table 6. Increased
/// > availability impact increases the vulnerability score.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum AvailabilityImpact {
    /// None (N)
    /// > There is no impact to the availability of the system.
    None,

    /// Partial (P)
    /// > There is reduced performance or interruptions in resource
    /// > availability. An example is a network-based flood attack that permits
    /// > a limited number of successful connections to an Internet service.
    Partial,

    /// Complete (C)
    /// > There is a total shutdown of the affected resource. The attacker can
    /// > render the resource completely unavailable.
    Complete,
}

impl Metric for AvailabilityImpact {
    fn score(self) -> f64 {
        match self {
            Self::None => 0.0,
            Self::Partial => 0.275,
            Self::Complete => 0.660,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::None => "N",
            Self::Partial => "P",
            Self::Complete => "C",
        }
    }

    const TYPE: MetricType = MetricType::A;
}

impl fmt::Display for AvailabilityImpact {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for AvailabilityImpact {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "N" => Ok(Self::None),
            "P" => Ok(Self::Partial),
            "C" => Ok(Self::Complete),
            _ => Err(Error::InvalidMetricV2 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
