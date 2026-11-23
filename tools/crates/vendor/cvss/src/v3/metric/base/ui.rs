//! User Interaction (UI)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v3::{Metric, MetricType};

/// User Interaction (UI) - CVSS v3.1 Base Metric Group
///
/// Described in CVSS v3.1 Specification: Section 2.1.4:
/// <https://www.first.org/cvss/specification-document#t6>
///
/// > This metric captures the requirement for a human user, other than the
/// > attacker, to participate in the successful compromise of the vulnerable
/// > component. This metric determines whether the vulnerability can be
/// > exploited solely at the will of the attacker, or whether a separate user
/// > (or user-initiated process) must participate in some manner.
/// > The Base Score is greatest when no user interaction is required.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum UserInteraction {
    /// Required (R)
    ///
    /// > Successful exploitation of this vulnerability requires a user to
    /// > take some action before the vulnerability can be exploited. For
    /// > example, a successful exploit may only be possible during the
    /// > installation of an application by a system administrator.
    Required,

    /// None (N)
    ///
    /// > The vulnerable system can be exploited without interaction from any user.
    None,
}

impl Metric for UserInteraction {
    fn score(self) -> f64 {
        match self {
            Self::Required => 0.62,
            Self::None => 0.85,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::Required => "R",
            Self::None => "N",
        }
    }

    const TYPE: MetricType = MetricType::UI;
}

impl fmt::Display for UserInteraction {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for UserInteraction {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "R" => Ok(Self::Required),
            "N" => Ok(Self::None),
            _ => Err(Error::InvalidMetricV3 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
