//! Automatable (AU)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Automatable (AU) - CVSS v4.0 Supplemental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 5.2
///
/// > The “Automatable” metric captures the answer to the question ”Can an
/// > attacker automate exploitation events for this vulnerability across
/// > multiple targets?” based on steps 1-4 of the kill chain [Hutchins et al.,
/// > 2011]. These steps are reconnaissance, weaponization, delivery, and
/// > exploitation.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum Automatable {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,

    /// Yes (Y)
    ///
    /// > Attackers can reliably automate all 4 steps of the kill chain. These
    /// > steps are reconnaissance, weaponization, delivery, and exploitation
    /// > (e.g., the vulnerability is “wormable”).
    Yes,

    /// No (N)
    ///
    /// > Attackers cannot reliably automate all 4 steps of the kill chain for
    /// > this vulnerability for some reason. These steps are reconnaissance,
    /// > weaponization, delivery, and exploitation.
    No,
}

impl Default for Automatable {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for Automatable {
    const TYPE: MetricType = MetricType::AU;

    fn as_str(self) -> &'static str {
        match self {
            Automatable::NotDefined => "X",
            Automatable::Yes => "Y",
            Automatable::No => "N",
        }
    }
}

impl fmt::Display for Automatable {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for Automatable {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(Automatable::NotDefined),
            "Y" => Ok(Automatable::Yes),
            "N" => Ok(Automatable::No),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
