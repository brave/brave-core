//! CVSS v3.1 Temporal Metric Group

mod e;
pub use e::ExploitCodeMaturity;

mod rc;
pub use rc::ReportConfidence;

mod rl;
pub use rl::RemediationLevel;

use crate::v3::Vector;

#[cfg(feature = "std")]
use crate::v3::{Metric, Score};

impl Vector {
    /// Calculates Temporal CVSS score.
    ///
    /// Described in CVSS v3.1 Specification: Section 7.2:
    /// <https://www.first.org/cvss/v3-1/specification-document#7-2-Temporal-Metrics-Equations>
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn temporal_score(&self) -> Score {
        let base_score = self.base_score().value();

        let e_score = self.e.unwrap_or(ExploitCodeMaturity::NotDefined).score();
        let rl_score = self.rl.unwrap_or(RemediationLevel::NotDefined).score();
        let rc_score = self.rc.unwrap_or(ReportConfidence::NotDefined).score();

        let temporal_score = base_score * e_score * rl_score * rc_score;

        Score::new(temporal_score).roundup_for_version(self.minor_version)
    }
}
