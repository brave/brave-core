//! User Interaction (MUI)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// User Interaction (MUI) - CVSS v4.0 Environmental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 4.2
///
/// > This metric captures the requirement for a human user, other than the
/// > attacker, to participate in the successful compromise of the vulnerable
/// > system. This metric determines whether the vulnerability can be exploited
/// > solely at the will of the attacker, or whether a separate user (or
/// > user-initiated process) must participate in some manner. The resulting
/// > score is greatest when no user interaction is required.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ModifiedUserInteraction {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,
    /// Active (A)
    ///
    /// > Successful exploitation of this vulnerability requires a targeted user
    /// > to perform specific, conscious interactions with the vulnerable system
    /// > and the attacker’s payload, or the user’s interactions would actively
    /// > subvert protection mechanisms which would lead to exploitation of the
    /// > vulnerability. Examples include: importing a file into a vulnerable
    /// > system in a specific manner placing files into a specific directory
    /// > prior to executing code submitting a specific string into a web
    /// > application (e.g. reflected or self XSS) dismiss or accept prompts or
    /// > security warnings prior to taking an action (e.g. opening/editing a
    /// > file, connecting a device).
    Active,
    /// Passive (P)
    ///
    /// > Successful exploitation of this vulnerability requires limited
    /// > interaction by the targeted user with the vulnerable system and the
    /// > attacker’s payload. These interactions would be considered involuntary
    /// > and do not require that the user actively subvert protections built
    /// > into the vulnerable system. Examples include: utilizing a website that
    /// > has been modified to display malicious content when the page is
    /// > rendered (most stored XSS or CSRF) running an application that calls a
    /// > malicious binary that has been planted on the system using an
    /// > application which generates traffic over an untrusted or compromised
    /// > network (vulnerabilities requiring an on-path attacker)
    Passive,
    /// None (N)
    ///
    /// > The vulnerable system can be exploited without interaction from any
    /// > human user, other than the attacker. Examples include: a remote
    /// > attacker is able to send packets to a target system a locally
    /// > authenticated attacker executes code to elevate privileges
    None,
}

impl Default for ModifiedUserInteraction {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for ModifiedUserInteraction {
    const TYPE: MetricType = MetricType::MUI;

    fn as_str(self) -> &'static str {
        match self {
            ModifiedUserInteraction::NotDefined => "X",
            ModifiedUserInteraction::None => "N",
            ModifiedUserInteraction::Passive => "P",
            ModifiedUserInteraction::Active => "A",
        }
    }
}

impl fmt::Display for ModifiedUserInteraction {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ModifiedUserInteraction {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(ModifiedUserInteraction::NotDefined),
            "N" => Ok(ModifiedUserInteraction::None),
            "P" => Ok(ModifiedUserInteraction::Passive),
            "A" => Ok(ModifiedUserInteraction::Active),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
