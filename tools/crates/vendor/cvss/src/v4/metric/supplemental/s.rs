//! Safety (S)

use crate::{
    Error,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Safety (S) - CVSS v4.0 Supplemental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 5.1
///
/// > Like all Supplemental Metrics, providing a value for Safety is completely
/// > optional. Suppliers and vendors (AKA: scoring providers) may or may not
/// > provide Safety as a Supplemental Metric as they see fit.
/// > When a system does have an intended use or fitness of purpose aligned to
/// > safety, it is possible that exploiting a vulnerability within that system
/// > may have Safety impact which can be represented in the Supplemental
/// > Metrics group. Lack of a Safety metric value being supplied does NOT mean
/// > that there may not be any Safety-related impacts.
#[derive(Copy, Clone, Debug, Default, Eq, PartialEq, PartialOrd, Ord)]
pub enum Safety {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    #[default]
    NotDefined,

    /// Present (P)
    ///
    /// > Consequences of the vulnerability meet definition of IEC 61508
    /// > consequence categories of "marginal," "critical," or "catastrophic."
    Present,

    /// Negligible (N)
    ///
    /// > Consequences of the vulnerability meet definition of IEC 61508
    /// > consequence category "negligible."
    Negligible,
}

impl Metric for Safety {
    const TYPE: MetricType = MetricType::S;

    fn as_str(self) -> &'static str {
        match self {
            Self::NotDefined => "X",
            Self::Present => "P",
            Self::Negligible => "N",
        }
    }
}

impl fmt::Display for Safety {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for Safety {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "X" => Ok(Self::NotDefined),
            "P" => Ok(Self::Present),
            "N" => Ok(Self::Negligible),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
