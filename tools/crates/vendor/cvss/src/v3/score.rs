//! CVSS v3.1 scores

use crate::severity::Severity;

/// CVSS v3.1 scores.
///
/// Formula described in CVSS v3.1 Specification: Section 5:
/// <https://www.first.org/cvss/specification-document#t20>
#[derive(Copy, Clone, Debug, Default, PartialEq, PartialOrd)]
pub struct Score(f64);

impl Score {
    /// Create a new score object
    pub fn new(score: f64) -> Score {
        Score(score)
    }

    /// Get the score as a floating point value
    pub fn value(self) -> f64 {
        self.0
    }

    /// Round the score up to the algorithm described in
    /// CVSS v3.1: Appendix A - Floating Point Rounding.
    ///
    /// <https://www.first.org/cvss/specification-document#t25>
    #[cfg(feature = "std")]
    pub fn roundup(self) -> Score {
        let score_int = (self.0 * 100_000.0) as u64;

        if score_int % 10000 == 0 {
            Score((score_int as f64) / 100_000.0)
        } else {
            let score_floor = ((score_int as f64) / 10_000.0).floor();
            Score((score_floor + 1.0) / 10.0)
        }
    }

    /// Convert the numeric score into a `Severity`
    pub fn severity(self) -> Severity {
        if self.0 < 0.1 {
            Severity::None
        } else if self.0 < 4.0 {
            Severity::Low
        } else if self.0 < 7.0 {
            Severity::Medium
        } else if self.0 < 9.0 {
            Severity::High
        } else {
            Severity::Critical
        }
    }
}

impl From<f64> for Score {
    fn from(score: f64) -> Score {
        Score(score)
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
