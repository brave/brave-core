//! Report Confidence (RC)

use alloc::borrow::ToOwned;

use core::{fmt, str::FromStr};

use crate::Error;
use crate::v3::{Metric, MetricType};

/// Report Confidence (RC) - CVSS v3.1 Temporal Metric Group
/// > This metric measures the degree of confidence in the existence of the
/// > vulnerability and the credibility of the known technical details.
/// > Sometimes only the existence of vulnerabilities is publicized, but without
/// > specific details.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ReportConfidence {
    /// Not Defined (X)
    /// > Assigning this value indicates there is insufficient information to
    /// > choose one of the other values, and has no impact on the overall
    /// > Temporal Score, i.e., it has the same effect on scoring as assigning
    /// > Confirmed.
    NotDefined,

    /// Confirmed (C)
    /// > Detailed reports exist, or functional reproduction is possible
    /// > (functional exploits may provide this). Source code is available to
    /// > independently verify the assertions of the research, or the author or
    /// > vendor of the affected code has confirmed the presence of the
    /// > vulnerability.
    Confirmed,

    /// Reasonable (R)
    /// > Significant details are published, but researchers either do not have
    /// > full confidence in the root cause, or do not have access to source
    /// > code to fully confirm all of the interactions that may lead to the
    /// > result. Reasonable confidence exists, however, that the bug is
    /// > reproducible and at least one impact is able to be verified
    /// > (proof-of-concept exploits may provide this). An example is a detailed
    /// > write-up of research into a vulnerability with an explanation
    /// > (possibly obfuscated or “left as an exercise to the reader”) that
    /// > gives assurances on how to reproduce the results.
    Reasonable,

    /// Unknown (U)
    /// > There are reports of impacts that indicate a vulnerability is present.
    /// > The reports indicate that the cause of the vulnerability is unknown,
    /// > or reports may differ on the cause or impacts of the vulnerability.
    /// > Reporters are uncertain of the true nature of the vulnerability, and
    /// > there is little confidence in the validity of the reports or whether a
    /// > static Base Score can be applied given the differences described. An
    /// > example is a bug report which notes that an intermittent but
    /// > non-reproducible crash occurs, with evidence of memory corruption
    /// > suggesting that denial of service, or possible more serious impacts,
    /// > may result.
    Unknown,
}

impl Metric for ReportConfidence {
    fn score(self) -> f64 {
        match self {
            Self::NotDefined => 1.0,
            Self::Confirmed => 1.0,
            Self::Reasonable => 0.96,
            Self::Unknown => 0.92,
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::NotDefined => "X",
            Self::Confirmed => "C",
            Self::Reasonable => "R",
            Self::Unknown => "U",
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
            "X" => Ok(Self::NotDefined),
            "C" => Ok(Self::Confirmed),
            "R" => Ok(Self::Reasonable),
            "U" => Ok(Self::Unknown),
            _ => Err(Error::InvalidMetricV3 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
