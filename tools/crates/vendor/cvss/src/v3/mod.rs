//! Common Vulnerability Scoring System (v3.1)
//!
//! <https://www.first.org/cvss/specification-document>

pub mod metric;

mod score;
mod vector;

pub use self::{
    metric::{Metric, MetricType},
    score::Score,
    vector::Vector,
};
