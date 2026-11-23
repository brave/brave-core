//! CVSS v3.1 Environmental Metric Group - Modified Base Metrics

use core::{fmt, str::FromStr};

use crate::Error;
use crate::v3::{
    Metric, MetricType,
    metric::base::{
        AttackComplexity, AttackVector, Availability, Confidentiality, Integrity,
        PrivilegesRequired, UserInteraction,
    },
};

/// A CVSS v3.1 Modified Base Metric: either left unspecified ("Not Defined")
/// or overriding the corresponding Base metric's value.
///
/// Described in CVSS v3.1 Specification: Section 4.2:
/// <https://www.first.org/cvss/v3-1/specification-document#4-2-Modified-Base-Metrics>
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum Modified<T> {
    /// Not Defined (X)
    NotDefined,

    /// Modified (see the wrapped Base metric)
    Modified(T),
}

impl<T: BaseMetric + FromStr<Err = Error>> Modified<T> {
    /// Get CVSS v3.1 modified score for this metric, given the base metric
    /// value.
    ///
    /// If the modified metric is `NotDefined`, the base metric value is used to
    /// compute the score.
    pub fn modified_score(self, base: Option<T>) -> f64 {
        match self {
            Self::Modified(v) => v.score(),
            Self::NotDefined => base.map(|v| v.score()).unwrap_or(0.0),
        }
    }
}

impl Modified<PrivilegesRequired> {
    /// Calculate the Scoped Score for the Modified Privileges Required (MPR)
    /// metric.
    ///
    /// Its value depends on whether the scope of the
    /// [crate::v3::metric::environmental::ModifiedScope] (or
    /// [crate::v3::metric::base::Scope] base) metric has changed.
    pub fn scoped_score(self, scope_changed: bool, base: Option<PrivilegesRequired>) -> f64 {
        match self {
            Self::Modified(v) => v.scoped_score(scope_changed),
            Self::NotDefined => base.map(|b| b.scoped_score(scope_changed)).unwrap_or(0.0),
        }
    }
}

impl<T: BaseMetric + FromStr<Err = Error>> Metric for Modified<T> {
    fn score(self) -> f64 {
        0.0
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::Modified(v) => v.as_str(),
            Self::NotDefined => "X",
        }
    }

    const TYPE: MetricType = T::MODIFIED_TYPE;
}

impl<T: BaseMetric + FromStr<Err = Error>> fmt::Display for Modified<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl<T: BaseMetric + FromStr<Err = Error>> FromStr for Modified<T> {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        Ok(match s {
            "X" => Self::NotDefined,
            _ => Self::Modified(T::from_str(s)?),
        })
    }
}

mod sealed {
    #[allow(unnameable_types)]
    pub trait Sealed {}
}

/// Trait for CVSSv3 Base metrics that have a corresponding "Modified"
/// Environmental metric.
///
/// This trait is sealed: it is only implemented for the Base metrics that
/// actually have a "Modified" counterpart, and cannot be implemented outside
/// of this crate.
pub trait BaseMetric: Metric + sealed::Sealed {
    /// [`MetricType`] of the corresponding modified metric.
    const MODIFIED_TYPE: MetricType;
}

impl sealed::Sealed for AttackVector {}

impl BaseMetric for AttackVector {
    const MODIFIED_TYPE: MetricType = MetricType::MAV;
}

impl sealed::Sealed for AttackComplexity {}

impl BaseMetric for AttackComplexity {
    const MODIFIED_TYPE: MetricType = MetricType::MAC;
}

impl sealed::Sealed for PrivilegesRequired {}

impl BaseMetric for PrivilegesRequired {
    const MODIFIED_TYPE: MetricType = MetricType::MPR;
}

impl sealed::Sealed for UserInteraction {}

impl BaseMetric for UserInteraction {
    const MODIFIED_TYPE: MetricType = MetricType::MUI;
}

impl sealed::Sealed for Confidentiality {}

impl BaseMetric for Confidentiality {
    const MODIFIED_TYPE: MetricType = MetricType::MC;
}

impl sealed::Sealed for Integrity {}

impl BaseMetric for Integrity {
    const MODIFIED_TYPE: MetricType = MetricType::MI;
}

impl sealed::Sealed for Availability {}

impl BaseMetric for Availability {
    const MODIFIED_TYPE: MetricType = MetricType::MA;
}
