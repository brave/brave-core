//! User Interaction (UI)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// User Interaction (UI) - CVSS v4.0 Base Metric Group
///
/// Described in CVSS v4.0 Specification: Section 2.1.5
///
/// > This metric captures the requirement for a human user, other than the
/// > attacker, to participate in the successful compromise of the vulnerable
/// > system. This metric determines whether the vulnerability can be exploited
/// > solely at the will of the attacker, or whether a separate user (or
/// > user-initiated process) must participate in some manner. The resulting
/// > score is greatest when no user interaction is required.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum UserInteraction {
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

impl Default for UserInteraction {
    fn default() -> Self {
        Self::None
    }
}

impl Metric for UserInteraction {
    const TYPE: MetricType = MetricType::UI;

    fn as_str(self) -> &'static str {
        match self {
            UserInteraction::None => "N",
            UserInteraction::Passive => "P",
            UserInteraction::Active => "A",
        }
    }
}

impl fmt::Display for UserInteraction {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for UserInteraction {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(UserInteraction::None),
            "P" => Ok(UserInteraction::Passive),
            "A" => Ok(UserInteraction::Active),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}

#[cfg(feature = "std")]
pub(crate) mod merge {
    use super::*;
    use crate::{
        Error,
        v4::{
            MetricType,
            metric::{MetricLevel, environmental::ModifiedUserInteraction},
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedUserInteraction {
        Active,
        Passive,
        None,
    }

    impl Default for MergedUserInteraction {
        fn default() -> Self {
            Self::None
        }
    }

    impl FromStr for MergedUserInteraction {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "A" => Ok(MergedUserInteraction::Active),
                "P" => Ok(MergedUserInteraction::Passive),
                "N" => Ok(MergedUserInteraction::None),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::UI,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl MetricLevel for MergedUserInteraction {
        fn level(self) -> f64 {
            // UI_levels = {"N": 0.0, "P": 0.1, "A": 0.2}
            match self {
                Self::Active => 0.2,
                Self::Passive => 0.1,
                Self::None => 0.0,
            }
        }
    }

    impl UserInteraction {
        pub(crate) fn merge(self, value: Option<ModifiedUserInteraction>) -> MergedUserInteraction {
            match value {
                Some(ModifiedUserInteraction::NotDefined) | None => match self {
                    Self::Passive => MergedUserInteraction::Passive,
                    Self::Active => MergedUserInteraction::Active,
                    Self::None => MergedUserInteraction::None,
                },
                Some(ModifiedUserInteraction::Passive) => MergedUserInteraction::Passive,
                Some(ModifiedUserInteraction::Active) => MergedUserInteraction::Active,
                Some(ModifiedUserInteraction::None) => MergedUserInteraction::None,
            }
        }
    }
}
