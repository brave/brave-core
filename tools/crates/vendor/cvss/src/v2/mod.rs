//! Common Vulnerability Scoring System (v2.0)
//!
//! <https://www.first.org/cvss/v2/guide>

pub mod metric;

mod score;
mod vector;

pub use self::{metric::Metric, metric::MetricType, score::Score, vector::Vector};
