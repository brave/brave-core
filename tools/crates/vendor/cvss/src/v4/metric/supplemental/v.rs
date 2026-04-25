//! Value Density (V)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Value Density (V) - CVSS v4.0 Supplemental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 5.5
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ValueDensity {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,

    /// Diffuse (D)
    ///
    /// > The vulnerable system has limited resources. That is, the resources
    /// > that the attacker will gain control over with a single exploitation
    /// > event are relatively small. An example of Diffuse (think: limited)
    /// > Value Density would be an attack on a single email client
    /// > vulnerability.
    Diffuse,

    /// Concentrated (C)
    ///
    /// > The vulnerable system is rich in resources. Heuristically, such
    /// > systems are often the direct responsibility of “system operators”
    /// > rather than users. An example of Concentrated (think: broad) Value
    /// > Density would be an attack on a central email server.
    Concentrated,
}

impl Default for ValueDensity {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for ValueDensity {
    const TYPE: MetricType = MetricType::V;

    fn as_str(self) -> &'static str {
        match self {
            ValueDensity::NotDefined => "X",
            ValueDensity::Diffuse => "D",
            ValueDensity::Concentrated => "C",
        }
    }
}

impl fmt::Display for ValueDensity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ValueDensity {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(ValueDensity::NotDefined),
            "D" => Ok(ValueDensity::Diffuse),
            "C" => Ok(ValueDensity::Concentrated),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
