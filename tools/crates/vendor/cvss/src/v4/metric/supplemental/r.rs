//! Recovery (R)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Recovery (R) - CVSS v4.0 Supplemental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 5.4
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum Recovery {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,

    /// Automatic (A)
    ///
    /// > The system recovers services automatically after an attack has been
    /// > performed.
    Automatic,

    /// User (U)
    ///
    /// > The system requires manual intervention by the user to recover
    /// > services, after an attack has been performed.
    User,

    /// Irrecoverable (I)
    ///
    /// > The system services are irrecoverable by the user, after an attack has
    /// > been performed.
    Irrecoverable,
}

impl Default for Recovery {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for Recovery {
    const TYPE: MetricType = MetricType::R;

    fn as_str(self) -> &'static str {
        match self {
            Recovery::NotDefined => "X",
            Recovery::Automatic => "A",
            Recovery::User => "U",
            Recovery::Irrecoverable => "I",
        }
    }
}

impl fmt::Display for Recovery {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for Recovery {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(Recovery::NotDefined),
            "A" => Ok(Recovery::Automatic),
            "U" => Ok(Recovery::User),
            "I" => Ok(Recovery::Irrecoverable),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
