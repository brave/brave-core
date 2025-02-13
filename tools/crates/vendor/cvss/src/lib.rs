#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![doc = include_str!("../README.md")]
#![doc(html_logo_url = "https://raw.githubusercontent.com/RustSec/logos/main/rustsec-logo-lg.png")]
#![forbid(unsafe_code)]
#![warn(missing_docs, rust_2018_idioms, unused_qualifications)]

//! ## Usage
//!
//! The [`v3::Base`] type provides the main functionality currently implemented
//! by this crate, namely: support for parsing, serializing, and scoring
//! `CVSS:3.0` and `CVSS:3.1` Base Metric Group vector strings as described in
//! the [CVSS v3.1 Specification].
//!
//! Serde support is available through the optional `serde` Cargo feature.
//!
//! [CVSS v3.1 Specification]: https://www.first.org/cvss/specification-document

// TODO(tarcieri): other CVSS versions, CVSS v3.1 Temporal and Environmental Groups

extern crate alloc;

#[cfg(feature = "std")]
extern crate std;

#[cfg(feature = "v3")]
pub mod v3;

mod error;
mod metric;
mod severity;

pub use crate::{
    error::{Error, Result},
    metric::{Metric, MetricType},
    severity::Severity,
};

/// Prefix used by all CVSS strings
pub const PREFIX: &str = "CVSS";
