//! CVSS v2.0 Base Metric Group - Authentication (Au)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Authentication (Au) - CVSS v2.0 Base Metric Group
///
/// Described in CVSS v2.0 Specification: Section 2.1.3:
/// <https://www.first.org/cvss/v2/guide#2-1-3-Authentication-Au>
///
/// > This metric measures the number of times an attacker must authenticate to
/// > a target in order to exploit a vulnerability. This metric does not gauge
/// > the strength or complexity of the authentication process, only that an
/// > attacker is required to provide credentials before an exploit may occur.
/// > The possible values for this metric are listed in Table 3. The fewer
/// > authentication instances that are required, the higher the vulnerability
/// > score.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum Authentication {
    /// Multiple (M)
    /// > Exploiting the vulnerability requires that the attacker authenticate
    /// > two or more times, even if the same credentials are used each time. An
    /// > example is an attacker authenticating to an operating system in
    /// > addition to providing credentials to access an application hosted on
    /// > that system.
    Multiple,

    /// Single (S)
    /// > The vulnerability requires an attacker to be logged into the system
    /// > (such as at a command line or via a desktop session or web interface).
    Single,

    /// None (N)
    /// > Authentication is not required to exploit the vulnerability.
    None,
}

impl Metric for Authentication {
    fn score(self) -> f64 {
        match self {
            Self::Multiple => 0.45,
            Self::Single => 0.56,
            Self::None => 0.704,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::Multiple => "M",
            Self::Single => "S",
            Self::None => "N",
        }
    }

    const TYPE: MetricType = MetricType::Au;
}

impl fmt::Display for Authentication {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for Authentication {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "M" => Ok(Self::Multiple),
            "S" => Ok(Self::Single),
            "N" => Ok(Self::None),
            _ => Err(Error::InvalidMetricV2 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
