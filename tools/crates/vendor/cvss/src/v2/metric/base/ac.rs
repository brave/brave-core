//! CVSS v2.0 Base Metric Group - Access Complexity (AC)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Access Complexity (AC) - CVSS v2.0 Base Metric Group
///
/// Described in CVSS v2.0 Specification: Section 2.1.2:
/// <https://www.first.org/cvss/v2/guide#2-1-2-Access-Complexity-AC>
///
/// > This metric measures the complexity of the attack required to exploit the
/// > vulnerability once an attacker has gained access to the target system. For
/// > example, consider a buffer overflow in an Internet service: once the
/// > target system is located, the attacker can launch an exploit at will.
/// >
/// > Other vulnerabilities, however, may require additional steps in order to
/// > be exploited. For example, a vulnerability in an email client is only
/// > exploited after the user downloads and opens a tainted attachment. The
/// > possible values for this metric are listed in Table 2. The lower the
/// > required complexity, the higher the vulnerability score.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum AccessComplexity {
    /// High (H)
    ///
    /// > Specialized access conditions exist. For example:
    /// > - In most configurations, the attacking party must already have elevated
    /// >   privileges or spoof additional systems in addition to the attacking
    /// >   system (e.g., DNS hijacking).
    /// > - The attack depends on social engineering methods that would be easily
    /// >   detected by knowledgeable people. For example, the victim must perform
    /// >   several suspicious or atypical actions.
    /// > - The vulnerable configuration is seen very rarely in practice.
    /// > - If a race condition exists, the window is very narrow.
    High,

    /// Medium (M)
    ///
    /// > The access conditions are somewhat specialized; the following are
    /// > examples:
    /// > - The attacking party is limited to a group of systems or users at
    /// >   some level of authorization, possibly untrusted.
    /// > - Some information must be gathered before a successful attack can be
    /// >   launched.
    /// > - The affected configuration is non-default, and is not commonly
    /// >   configured (e.g., a vulnerability present when a server performs
    /// >   user account authentication via a specific scheme, but not present
    /// >   for another authentication scheme).
    /// > - The attack requires a small amount of social engineering that might
    /// >   occasionally fool cautious users (e.g., phishing attacks that modify
    /// >   a web browsers status bar to show a false link, having to be on
    /// >   someones buddy list before sending an IM exploit).
    Medium,

    /// Low (L)
    ///
    /// > Specialized access conditions or extenuating circumstances do not
    /// > exist. The following are examples:
    /// > - The affected product typically requires access to a wide range of
    /// >   systems and users, possibly anonymous and untrusted (e.g.,
    /// >   Internet-facing web or mail server).
    /// > - The affected configuration is default or ubiquitous.
    /// > - The attack can be performed manually and requires little skill or
    /// >   additional information gathering.
    /// > - The race condition is a lazy one (i.e., it is technically a race but
    /// >   easily winnable).
    Low,
}

impl Metric for AccessComplexity {
    fn score(self) -> f64 {
        match self {
            Self::High => 0.35,
            Self::Medium => 0.61,
            Self::Low => 0.71,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::High => "H",
            Self::Medium => "M",
            Self::Low => "L",
        }
    }

    const TYPE: MetricType = MetricType::AC;
}

impl fmt::Display for AccessComplexity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for AccessComplexity {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "H" => Ok(Self::High),
            "M" => Ok(Self::Medium),
            "L" => Ok(Self::Low),
            _ => Err(Error::InvalidMetricV2 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
