//! CVSS v4 scores

#[cfg(feature = "std")]
use crate::v4::scoring::ScoringVector;
use crate::{Error, severity::Severity, v4::Vector};
use alloc::borrow::ToOwned;
#[cfg(feature = "serde")]
use alloc::string::String;
#[cfg(feature = "serde")]
use alloc::string::ToString;
use core::{fmt, fmt::Display, str::FromStr};
#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize, de, ser};

/// CVSS v4 scores
///
/// It consists of a floating point value, and a nomenclature indicating
/// the type of metrics used to calculate the score as recommended by the
/// specification.
///
/// Described in CVSS v4.0 Specification: Section 1.3
///
/// > This nomenclature should be used wherever a numerical CVSS value is displayed or communicated.
#[derive(Clone, Debug, PartialEq)]
pub struct Score {
    value: f64,
    nomenclature: Nomenclature,
}

/// > Numerical CVSS Scores have very different meanings based on the metrics
/// > used to calculate them. Regarding prioritization, the usefulness of a
/// > numerical CVSS score is directly proportional to the CVSS metrics
/// > leveraged to generate that score. Therefore, numerical CVSS scores should
/// > be labeled using nomenclature that communicates the metrics used in its
/// > generation.
#[derive(Clone, Debug, PartialEq)]
pub enum Nomenclature {
    ///Base metrics
    CvssB,
    ///  Base and Environmental metrics
    CvssBE,
    /// Base and Threat metrics
    CvssBT,
    /// Base, Threat, Environmental metrics
    CvssBTE,
}

impl Display for Nomenclature {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::CvssB => write!(f, "CVSS-B"),
            Self::CvssBE => write!(f, "CVSS-BE"),
            Self::CvssBT => write!(f, "CVSS-BT"),
            Self::CvssBTE => write!(f, "CVSS-BTE"),
        }
    }
}

impl FromStr for Nomenclature {
    type Err = Error;

    fn from_str(s: &str) -> crate::Result<Self> {
        match s {
            "CVSS-B" => Ok(Self::CvssB),
            "CVSS-BE" => Ok(Self::CvssBE),
            "CVSS-BT" => Ok(Self::CvssBT),
            "CVSS-BTE" => Ok(Self::CvssBTE),
            _ => Err(Error::InvalidNomenclatureV4 {
                nomenclature: s.to_owned(),
            }),
        }
    }
}

impl From<&Vector> for Nomenclature {
    fn from(vector: &Vector) -> Self {
        let has_threat = vector.e.is_some();
        let has_environmental = vector.ar.is_some()
            || vector.cr.is_some()
            || vector.ir.is_some()
            || vector.mac.is_some()
            || vector.mat.is_some()
            || vector.mav.is_some()
            || vector.mpr.is_some()
            || vector.msa.is_some()
            || vector.msc.is_some()
            || vector.msi.is_some()
            || vector.mui.is_some()
            || vector.mva.is_some()
            || vector.mvc.is_some()
            || vector.mvi.is_some();

        match (has_threat, has_environmental) {
            (true, true) => Nomenclature::CvssBTE,
            (true, false) => Nomenclature::CvssBT,
            (false, true) => Nomenclature::CvssBE,
            (false, false) => Nomenclature::CvssB,
        }
    }
}

#[cfg(feature = "serde")]
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
impl<'de> Deserialize<'de> for Nomenclature {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        String::deserialize(deserializer)?
            .parse()
            .map_err(de::Error::custom)
    }
}

#[cfg(feature = "serde")]
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
impl Serialize for Nomenclature {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        self.to_string().serialize(serializer)
    }
}

#[cfg(feature = "std")]
impl From<&Vector> for Score {
    fn from(vector: &Vector) -> Self {
        let nomenclature = Nomenclature::from(vector);
        let scoring = ScoringVector::from(vector);
        let value = Self::round_v4(scoring.score());

        Self {
            value,
            nomenclature,
        }
    }
}

impl Score {
    /// Create a new score
    pub fn new(value: f64, nomenclature: Nomenclature) -> Self {
        Self {
            value,
            nomenclature,
        }
    }

    /// The specification only states that the score should be rounded to one decimal.
    ///
    /// In order to stay compatible with Red Hat's test suite, so use the same
    /// rounding method.
    ///
    /// ```python
    /// from decimal import Decimal as D, ROUND_HALF_UP
    /// EPSILON = 10**-6
    /// return float(D(x + EPSILON).quantize(D("0.1"), rounding=ROUND_HALF_UP))
    /// ```
    #[cfg(feature = "std")]
    pub(crate) fn round_v4(value: f64) -> f64 {
        let value = f64::clamp(value, 0.0, 10.0);
        const EPSILON: f64 = 10e-6;
        ((value + EPSILON) * 10.).round() / 10.
    }

    /// Get the score as a floating point value
    pub fn value(self) -> f64 {
        self.value
    }

    /// Convert the numeric score into a `Severity`
    pub fn severity(self) -> Severity {
        if self.value < 0.1 {
            Severity::None
        } else if self.value < 4.0 {
            Severity::Low
        } else if self.value < 7.0 {
            Severity::Medium
        } else if self.value < 9.0 {
            Severity::High
        } else {
            Severity::Critical
        }
    }
}

/// There is no defined or recommended format in the specification, nor in existing implementations.
///
/// Using "4.5 (CVSS-BT)".
impl Display for Score {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // Always show exactly one decimal
        write!(f, "{:.1} ({})", self.value, self.nomenclature)
    }
}

impl From<Score> for f64 {
    fn from(score: Score) -> f64 {
        score.value()
    }
}

impl From<Score> for Severity {
    fn from(score: Score) -> Severity {
        score.severity()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use alloc::string::ToString;

    #[test]
    fn new_score() {
        let score = Score::new(5.5, Nomenclature::CvssB);
        assert_eq!(score.value(), 5.5);
    }

    #[test]
    #[cfg(feature = "std")]
    fn round_v4_round() {
        // 8.6 - 7.15 = 1.4499999999999993 (float) => 1.5
        assert_eq!(Score::round_v4(8.6 - 7.15), 1.5);
        assert_eq!(Score::round_v4(5.12345), 5.1);
    }

    #[test]
    fn into_severity() {
        let score = Score::new(5.0, Nomenclature::CvssB);
        let severity: Severity = score.into();
        assert_eq!(severity, Severity::Medium);
    }

    #[test]
    fn display_score() {
        let score = Score::new(4.5, Nomenclature::CvssB);
        assert_eq!(score.to_string(), "4.5 (CVSS-B)");
    }
}
