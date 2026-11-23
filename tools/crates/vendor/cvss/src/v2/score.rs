//! CVSS v2.0 scores

use crate::severity::Severity;

/// CVSS V2.0 scores.
///
/// Formula described in CVSS v2.0 Specification: Section 3.2:
/// <https://www.first.org/cvss/v2/guide#3-2-Equations>
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

    /// Round the score to 1 decimal (`round_to_1_decimal`), half up
    ///
    /// The reference implementations compute in exact decimal arithmetic and
    /// round half up, while `f64` products land one ULP below the tie
    /// (`9.0 * 0.95 == 8.549999…`), so a small epsilon lifts them onto it
    /// before rounding, the same approach the v4 reference calculator takes.
    #[cfg(feature = "std")]
    pub fn roundup(self) -> Self {
        const EPSILON: f64 = 1e-6;
        let rounded = ((self.0 + EPSILON) * 10.0).round() / 10.0;
        Self(rounded)
    }

    /// Convert the numeric score into a `Severity`
    ///
    /// CVSS v2.0 does not explictly define the severity levels, therefore the
    /// definition of NIST is used.
    pub fn severity(self) -> Severity {
        if self.0 < 4.0 {
            Severity::Low
        } else if self.0 < 7.0 {
            Severity::Medium
        } else {
            Severity::High
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

    #[test]
    fn roundup_epsilon_magnitude() {
        // One ULP below a tie lifts over it, which is the epsilon's purpose.
        assert_eq!(Score::new(9.0 * 0.95).roundup().value(), 8.6);
        // 5e-6 below the tie, beyond the documented 1e-6 epsilon, so it stays down.
        // No corpus vector lands in that band, so only this case notices a constant that is
        // too large, like the 10e-6 the v4 module once had.
        assert_eq!(Score::new(8.549_995).roundup().value(), 8.5);
    }
}
