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
    pub fn new(score: f64) -> Self {
        Self(score)
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
    pub fn roundup(self) -> Self {
        // Default behavior is CVSS v3.1 rounding
        self.roundup_for_version(1)
    }

    /// Round according to the CVSS v3.0 / v3.1 specification depending on
    /// the minor version. `minor_version == 0` corresponds to 3.0, and
    /// `minor_version == 1` corresponds to 3.1.
    #[cfg(feature = "std")]
    pub fn roundup_for_version(self, minor_version: usize) -> Self {
        if minor_version == 0 {
            // CVSS v3.0: round up to nearest tenth
            return Self((self.0 * 10.0).ceil() / 10.0);
        }

        // CVSS v3.1 rounding algorithm (Appendix A)
        let score_int = (self.0 * 100_000.0) as u64;
        if score_int % 10000 == 0 {
            return Self((score_int as f64) / 100_000.0);
        }

        let score_floor = ((score_int as f64) / 10_000.0).floor();
        Self((score_floor + 1.0) / 10.0)
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
    fn from(score: f64) -> Self {
        Self(score)
    }
}

impl From<Score> for f64 {
    fn from(score: Score) -> Self {
        score.value()
    }
}

impl From<Score> for Severity {
    fn from(score: Score) -> Self {
        score.severity()
    }
}

#[cfg(all(feature = "std", test))]
mod tests {
    use super::Score;
    use crate::severity::Severity;

    #[test]
    fn roundup_exact_tenth_unchanged() {
        assert_eq!(Score::new(4.0).roundup().value(), 4.0);
    }

    #[test]
    fn roundup_rounds_up() {
        assert_eq!(Score::new(4.01).roundup().value(), 4.1);
    }

    #[test]
    fn severity_boundaries() {
        assert_eq!(Score::new(0.0).severity(), Severity::None);
        assert_eq!(Score::new(0.1).severity(), Severity::Low);
        assert_eq!(Score::new(4.0).severity(), Severity::Medium);
        assert_eq!(Score::new(7.0).severity(), Severity::High);
        assert_eq!(Score::new(9.0).severity(), Severity::Critical);
    }
}
