//! Availability Impact (A)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v3::{Metric, MetricType};

/// Availability Impact (A) - CVSS v3.1 Base Metric Group
///
/// Described in CVSS v3.1 Specification: Section 2.3.3:
/// <https://www.first.org/cvss/specification-document#t6>
///
/// > This metric measures the impact to the availability of the impacted
/// > component resulting from a successfully exploited vulnerability.
/// > While the Confidentiality and Integrity impact metrics apply to the
/// > loss of confidentiality or integrity of data (e.g., information,
/// > files) used by the impacted component, this metric refers to the loss
/// > of availability of the impacted component itself, such as a networked
/// > service (e.g., web, database, email). Since availability refers to the
/// > accessibility of information resources, attacks that consume network
/// > bandwidth, processor cycles, or disk space all impact the availability
/// > of an impacted component. The Base Score is greatest when the
/// > consequence to the impacted component is highest.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum Availability {
    /// None (N)
    ///
    /// > There is no impact to availability within the impacted component.
    None,

    /// Low (L)
    ///
    /// > Performance is reduced or there are interruptions in resource
    /// > availability. Even if repeated exploitation of the vulnerability
    /// > is possible, the attacker does not have the ability to completely
    /// > deny service to legitimate users. The resources in the impacted
    /// > component are either partially available all of the time, or fully
    /// > available only some of the time, but overall there is no direct,
    /// > serious consequence to the impacted component.
    Low,

    /// High (H)
    ///
    /// > There is a total loss of availability, resulting in the attacker
    /// > being able to fully deny access to resources in the impacted
    /// > component; this loss is either sustained (while the attacker
    /// > continues to deliver the attack) or persistent (the condition
    /// > persists even after the attack has completed). Alternatively,
    /// > the attacker has the ability to deny some availability, but
    /// > the loss of availability presents a direct, serious consequence
    /// > to the impacted component (e.g., the attacker cannot disrupt
    /// > existing connections, but can prevent new connections; the
    /// > attacker can repeatedly exploit a vulnerability that, in each
    /// > instance of a successful attack, leaks a only small amount of
    /// > memory, but after repeated exploitation causes a service to become
    /// > completely unavailable).
    High,
}

impl Metric for Availability {
    fn score(self) -> f64 {
        match self {
            Self::None => 0.0,
            Self::Low => 0.22,
            Self::High => 0.56,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::None => "N",
            Self::Low => "L",
            Self::High => "H",
        }
    }

    const TYPE: MetricType = MetricType::A;
}

impl fmt::Display for Availability {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for Availability {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "N" => Ok(Self::None),
            "L" => Ok(Self::Low),
            "H" => Ok(Self::High),
            _ => Err(Error::InvalidMetricV3 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
