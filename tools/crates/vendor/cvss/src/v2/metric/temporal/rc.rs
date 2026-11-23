//! CVSS v2.0 Temporal Metric - Report Confidence (RC)

use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

use crate::Error;
use crate::v2::{Metric, MetricType};

/// Report Confidence (RC) - CVSS v2.0 Temporal Metric Group
///
/// Described in CVSS v2.0 Specification: Section 2.2.3:
/// <https://www.first.org/cvss/v2/guide#2-2-3-Report-Confidence-RC>
///
/// > This metric measures the degree of confidence in the existence of the
/// > vulnerability and the credibility of the known technical details.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ReportConfidence {
    /// Unconfirmed (UC)
    /// > There is a single unconfirmed source or possibly multiple conflicting
    /// > reports. There is little confidence in the validity of the reports. An
    /// > example is a rumor that surfaces from the hacker underground.
    Unconfirmed,

    /// Uncorroborated (UR)
    /// > There are multiple non-official sources, possibly including
    /// > independent security companies or research organizations. At this
    /// > point there may be conflicting technical details or some other
    /// > lingering ambiguity.
    Uncorroborated,

    /// Confirmed (C)
    /// > The vulnerability has been acknowledged by the vendor or author of the
    /// > affected technology. The vulnerability may also be Confirmed when its
    /// > existence is confirmed from an external event such as publication of
    /// > functional or proof-of-concept exploit code or widespread
    /// > exploitation.
    Confirmed,

    /// Not Defined (ND)
    /// > Assigning this value to the metric will not influence the score. It is
    /// > a signal to the equation to skip this metric.
    NotDefined,
}

impl Metric for ReportConfidence {
    fn score(self) -> f64 {
        match self {
            Self::Unconfirmed => 0.90,
            Self::Uncorroborated => 0.95,
            Self::Confirmed => 1.0,
            Self::NotDefined => 1.0,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::Unconfirmed => "UC",
            Self::Uncorroborated => "UR",
            Self::Confirmed => "C",
            Self::NotDefined => "ND",
        }
    }

    const TYPE: MetricType = MetricType::RC;
}

impl fmt::Display for ReportConfidence {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ReportConfidence {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "UC" => Ok(Self::Unconfirmed),
            "UR" => Ok(Self::Uncorroborated),
            "C" => Ok(Self::Confirmed),
            "ND" => Ok(Self::NotDefined),
            _ => Err(Error::InvalidMetricV2 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
