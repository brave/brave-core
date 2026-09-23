//! Common Vulnerability Scoring System (v4.0)
//!
//! <https://www.first.org/cvss/v4.0/specification-document>

pub mod metric;
pub mod score;
#[cfg(feature = "std")]
mod scoring;
mod vector;

pub use self::{
    metric::MetricType,
    score::{Nomenclature, Score},
    vector::Vector,
};
