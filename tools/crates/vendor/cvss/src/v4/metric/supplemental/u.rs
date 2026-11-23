//! Provider Urgency (U)

use crate::{
    Error,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Provider Urgency (U) - CVSS v4.0 Supplemental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 5.3
///
/// > Many vendors currently provide supplemental severity ratings to consumers
/// > via product security advisories. Other vendors publish Qualitative
/// > Severity Ratings from the CVSS Specification Document in their advisories.
/// >
/// > To facilitate a standardized method to incorporate additional
/// > provider-supplied assessment, an optional “pass-through” Supplemental
/// > Metric called Provider Urgency is available.
/// >
/// > Note: While any assessment provider along the product supply chain may
/// > provide a Provider Urgency rating:
/// >
/// > Library Maintainer → OS/Distro Maintainer → Provider 1 … Provider n (PPP)
/// > → Consumer
/// >
/// > The Penultimate Product Provider (PPP) is best positioned to provide a
/// > direct assessment of Provider Urgency.
#[derive(Copy, Clone, Debug, Default, Eq, PartialEq, PartialOrd, Ord)]
pub enum ProviderUrgency {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    #[default]
    NotDefined,
    /// Red
    ///
    /// > Provider has assessed the impact of this vulnerability as having the
    /// > highest urgency.
    Red,
    /// Amber
    ///
    /// > Provider has assessed the impact of this vulnerability as having a
    /// > moderate urgency.
    Amber,
    /// Green
    ///
    /// > Provider has assessed the impact of this vulnerability as having a
    /// > reduced urgency.
    Green,
    /// Clear
    ///
    /// > Provider has assessed the impact of this vulnerability as having no
    /// > urgency (Informational).
    Clear,
}

impl Metric for ProviderUrgency {
    const TYPE: MetricType = MetricType::U;

    fn as_str(self) -> &'static str {
        match self {
            Self::NotDefined => "X",
            Self::Red => "Red",
            Self::Amber => "Amber",
            Self::Green => "Green",
            Self::Clear => "Clear",
        }
    }
}

impl fmt::Display for ProviderUrgency {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ProviderUrgency {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "X" => Ok(Self::NotDefined),
            "RED" => Ok(Self::Red),
            "AMBER" => Ok(Self::Amber),
            "GREEN" => Ok(Self::Green),
            "CLEAR" => Ok(Self::Clear),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
