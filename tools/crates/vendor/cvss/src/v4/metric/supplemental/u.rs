//! Provider Urgency (U)

use crate::{
    Error, Result,
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
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ProviderUrgency {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
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

impl Default for ProviderUrgency {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for ProviderUrgency {
    const TYPE: MetricType = MetricType::U;

    fn as_str(self) -> &'static str {
        match self {
            ProviderUrgency::NotDefined => "X",
            ProviderUrgency::Red => "Red",
            ProviderUrgency::Amber => "Amber",
            ProviderUrgency::Green => "Green",
            ProviderUrgency::Clear => "Clear",
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

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(ProviderUrgency::NotDefined),
            "RED" => Ok(ProviderUrgency::Red),
            "AMBER" => Ok(ProviderUrgency::Amber),
            "GREEN" => Ok(ProviderUrgency::Green),
            "CLEAR" => Ok(ProviderUrgency::Clear),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
