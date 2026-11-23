//! CVSS v3.1 Environmental Metric Group - Modified Scope (MS)

use core::{fmt, str::FromStr};

use crate::Error;
use crate::v3::{Metric, MetricType, metric::base::Scope};

/// Modified Scope (MS) - CVSS v3.1 Environmental Metric Group
///
/// Described in CVSS v3.1 Specification: Section 4.2:
/// <https://www.first.org/cvss/v3-1/specification-document#4-2-Modified-Base-Metrics>
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ModifiedScope {
    /// Not Defined (X)
    NotDefined,

    /// Modified (see [Scope])
    Modified(Scope),
}

impl Metric for ModifiedScope {
    fn score(self) -> f64 {
        match self {
            Self::Modified(v) => v.score(),
            Self::NotDefined => 0.0,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::Modified(v) => v.as_str(),
            Self::NotDefined => "X",
        }
    }

    const TYPE: MetricType = MetricType::MS;
}

impl fmt::Display for ModifiedScope {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ModifiedScope {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        Ok(match s {
            "X" => Self::NotDefined,
            _ => Self::Modified(Scope::from_str(s)?),
        })
    }
}
