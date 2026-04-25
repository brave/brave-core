//! Exploit Maturity (E)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Exploit Maturity (E) - CVSS v4.0 Threat Metric Group
///
/// Described in CVSS v4.0 Specification: Section 3.1
///
/// > This metric measures the likelihood of the vulnerability being attacked,
/// > and is based on the current state of exploit techniques, exploit code
/// > availability, or active, “in-the-wild” exploitation. Public availability
/// > of easy-to-use exploit code or exploitation instructions increases the
/// > number of potential attackers by including those who are unskilled.
/// > Initially, real-world exploitation may only be theoretical. Publication of
/// > proof-of-concept exploit code, functional exploit code, or sufficient
/// > technical details necessary to exploit the vulnerability may follow.
/// > Furthermore, the available exploit code or instructions may progress from
/// > a proof-of-concept demonstration to exploit code that is successful in
/// > exploiting the vulnerability consistently. In severe cases, it may be
/// > delivered as the payload of a network-based worm or virus or other
/// > automated attack tools.
///
/// > It is the responsibility of the CVSS consumer to populate the values of
/// > Exploit Maturity (E) based on information regarding the availability of
/// > exploitation code/processes and the state of exploitation techniques. This
/// > information will be referred to as “threat intelligence” throughout this
/// > document.
///
/// > Operational Recommendation: Threat intelligence sources that provide
/// > Exploit Maturity information for all vulnerabilities should be preferred
/// > over those with only partial coverage. Also, it is recommended to use
/// > multiple sources of threat intelligence as many are not comprehensive.
/// > This information should be updated as frequently as possible and its
/// > application to CVSS assessment should be automated.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum ExploitMaturity {
    /// Not Defined (X)
    ///
    /// > Reliable threat intelligence is not available to determine Exploit
    /// > Maturity characteristics. This is the default value and is equivalent
    /// > to Attacked (A) for the purposes of the calculation of the score by
    /// > assuming the worst case.
    NotDefined,
    /// Unreported (U)
    ///
    /// > Based on available threat intelligence each of the following must
    /// > apply: No knowledge of publicly available proof-of-concept exploit
    /// > code No knowledge of reported attempts to exploit this vulnerability
    /// > No knowledge of publicly available solutions used to simplify attempts
    /// > to exploit the vulnerability (i.e., neither the “POC” nor “Attacked”
    /// > values apply)
    Unreported,
    /// Proof-of-Concept (P)
    ///
    /// > Based on available threat intelligence each of the following must
    /// > apply: Proof-of-concept exploit code is publicly available No
    /// > knowledge of reported attempts to exploit this vulnerability No
    /// > knowledge of publicly available solutions used to simplify attempts to
    /// > exploit the vulnerability (i.e., the “Attacked” value does not apply)
    ProofOfConcept,
    /// Attacked (A)
    ///
    /// > Based on available threat intelligence either of the following must
    /// > apply: Attacks targeting this vulnerability (attempted or successful)
    /// > have been reported Solutions to simplify attempts to exploit the
    /// > vulnerability are publicly or privately available (such as exploit
    /// > toolkits)
    Attacked,
}

impl Default for ExploitMaturity {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for ExploitMaturity {
    const TYPE: MetricType = MetricType::E;

    fn as_str(self) -> &'static str {
        match self {
            ExploitMaturity::NotDefined => "X",
            ExploitMaturity::Attacked => "A",
            ExploitMaturity::ProofOfConcept => "P",
            ExploitMaturity::Unreported => "U",
        }
    }
}

impl fmt::Display for ExploitMaturity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for ExploitMaturity {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(ExploitMaturity::NotDefined),
            "A" => Ok(ExploitMaturity::Attacked),
            "P" => Ok(ExploitMaturity::ProofOfConcept),
            "U" => Ok(ExploitMaturity::Unreported),
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
    pub(crate) enum MergedExploitMaturity {
        Attacked,
        ProofOfConcept,
        Unreported,
    }

    impl Default for MergedExploitMaturity {
        fn default() -> Self {
            Self::Attacked
        }
    }

    impl FromStr for MergedExploitMaturity {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "A" => Ok(MergedExploitMaturity::Attacked),
                "P" => Ok(MergedExploitMaturity::ProofOfConcept),
                "U" => Ok(MergedExploitMaturity::Unreported),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::E,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl ExploitMaturity {
        pub(crate) fn merge(self) -> MergedExploitMaturity {
            match self {
                Self::Attacked => MergedExploitMaturity::Attacked,
                Self::ProofOfConcept => MergedExploitMaturity::ProofOfConcept,
                Self::Unreported => MergedExploitMaturity::Unreported,
                Self::NotDefined => MergedExploitMaturity::Attacked,
            }
        }
    }

    impl MetricLevel for MergedExploitMaturity {
        fn level(self) -> f64 {
            // E_levels = {'U': 0.2, 'P': 0.1, 'A': 0}
            match self {
                Self::Unreported => 0.2,
                Self::ProofOfConcept => 0.1,
                Self::Attacked => 0.0,
            }
        }
    }
}
