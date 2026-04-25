//! Common Vulnerability Scoring System (v3.1)
//!
//! <https://www.first.org/cvss/specification-document>

// TODO(tarcieri): Environmental and Temporal Metrics

#[cfg(feature = "v3")]
pub mod base;

pub mod metric;

#[cfg(feature = "v3")]
mod score;

#[cfg(feature = "v3")]
pub use self::{
    base::Base,
    metric::{Metric, MetricType},
    score::Score,
};
