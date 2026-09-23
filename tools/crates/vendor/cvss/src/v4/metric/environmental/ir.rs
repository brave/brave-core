//! Integrity Requirements (CR)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Integrity Requirements (IR) - CVSS v4.0 Environmental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 4.1
///
/// > These metrics enable the consumer to customize the assessment depending on
/// > the importance of the affected IT asset to the analyst’s organization,
/// > measured in terms of Confidentiality, Integrity, and Availability. That
/// > is, if an IT asset supports a business function for which Availability is
/// > most important, the analyst can assign a greater value to Availability
/// > metrics relative to Confidentiality and Integrity. Each Security
/// > Requirement has three possible values: Low, Medium, or High, or the
/// > default value of Not Defined (X).
/// >
/// > The full effect on the environmental score is determined by the
/// > corresponding Modified Base Impact metrics. Following the concept of
/// > assuming “reasonable worst case”, in absence of explicit values, these
/// > metrics are set to the default value of Not Defined (X), which is
/// > equivalent to the metric value of High (H).
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum IntegrityRequirements {
    /// Not Defined (X)
    ///
    /// > This is the default value. Assigning this value indicates there is
    /// > insufficient information to choose one of the other values. This has
    /// > the same effect as assigning High as the worst case.
    NotDefined,
    /// Low (L)
    ///
    /// > Loss of Confidentiality is likely to have only a limited adverse
    /// > effect on the organization or individuals associated with the
    /// > organization (e.g., employees, customers).
    Low,
    /// Medium (M)
    ///
    /// > Loss of Confidentiality is likely to have a serious adverse effect on
    /// > the organization or individuals associated with the organization
    /// > (e.g., employees, customers).
    Medium,
    /// High (H)
    ///
    /// > Loss of Confidentiality is likely to have a catastrophic adverse
    /// > effect on the organization or individuals associated with the
    /// > organization (e.g., employees, customers).
    High,
}

impl Default for IntegrityRequirements {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for IntegrityRequirements {
    const TYPE: MetricType = MetricType::IR;

    fn as_str(self) -> &'static str {
        match self {
            IntegrityRequirements::NotDefined => "X",
            IntegrityRequirements::Low => "L",
            IntegrityRequirements::Medium => "M",
            IntegrityRequirements::High => "H",
        }
    }
}

impl fmt::Display for IntegrityRequirements {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for IntegrityRequirements {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(IntegrityRequirements::NotDefined),
            "L" => Ok(IntegrityRequirements::Low),
            "M" => Ok(IntegrityRequirements::Medium),
            "H" => Ok(IntegrityRequirements::High),
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
        v4::{MetricType, metric::MetricLevel},
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedIntegrityRequirements {
        Low,
        Medium,
        High,
    }

    impl Default for MergedIntegrityRequirements {
        fn default() -> Self {
            Self::High
        }
    }

    impl FromStr for MergedIntegrityRequirements {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "L" => Ok(MergedIntegrityRequirements::Low),
                "M" => Ok(MergedIntegrityRequirements::Medium),
                "H" => Ok(MergedIntegrityRequirements::High),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::IR,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl IntegrityRequirements {
        pub(crate) fn merge(self) -> MergedIntegrityRequirements {
            match self {
                Self::High => MergedIntegrityRequirements::High,
                Self::Medium => MergedIntegrityRequirements::Medium,
                Self::Low => MergedIntegrityRequirements::Low,
                Self::NotDefined => MergedIntegrityRequirements::High,
            }
        }
    }

    impl MetricLevel for MergedIntegrityRequirements {
        fn level(self) -> f64 {
            // IR_levels = {'H': 0.0, 'M': 0.1, 'L': 0.2}
            match self {
                Self::High => 0.0,
                Self::Medium => 0.1,
                Self::Low => 0.2,
            }
        }
    }
}
