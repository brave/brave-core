//! CVSS v2.0 Base Metric Group - Access Vector (AV)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Access Vector (AV) - CVSS v2.0 Base Metric Group
///
/// Described in CVSS v2.0 Specification: Section 2.1.1:
/// <https://www.first.org/cvss/v2/guide#2-1-1-Access-Vector-AV>
///
/// > This metric reflects how the vulnerability is exploited. The possible
/// > values for this metric are listed in Table 1. The more remote an attacker
/// > can be to attack a host, the greater the vulnerability score.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum AccessVector {
    /// Local (L)
    ///
    /// > A vulnerability exploitable with only local access requires the
    /// > attacker to have either physical access to the vulnerable system or a
    /// > local (shell) account. Examples of locally exploitable vulnerabilities
    /// > are peripheral attacks such as Firewire/USB DMA attacks, and local
    /// > privilege escalations (e.g., sudo).
    Local,

    /// AdjacentNetwork (A)
    ///
    /// > A vulnerability exploitable with adjacent network access requires the
    /// > attacker to have access to either the broadcast or collision domain of
    /// > the vulnerable software.  Examples of local networks include local IP
    /// > subnet, Bluetooth, IEEE 802.11, and local Ethernet segment.
    AdjacentNetwork,

    /// Network (N)
    ///
    /// > A vulnerability exploitable with network access means the vulnerable
    /// > software is bound to the network stack and the attacker does not
    /// > require local network access or local access. Such a vulnerability is
    /// > often termed "remotely exploitable". An example of a network attack is
    /// > an RPC buffer overflow.
    Network,
}

impl Metric for AccessVector {
    fn score(self) -> f64 {
        match self {
            Self::Local => 0.395,
            Self::AdjacentNetwork => 0.646,
            Self::Network => 1.0,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::Local => "L",
            Self::AdjacentNetwork => "A",
            Self::Network => "N",
        }
    }

    const TYPE: MetricType = MetricType::AV;
}

impl fmt::Display for AccessVector {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for AccessVector {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "L" => Ok(Self::Local),
            "A" => Ok(Self::AdjacentNetwork),
            "N" => Ok(Self::Network),
            _ => Err(Error::InvalidMetricV2 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
