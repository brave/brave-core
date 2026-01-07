#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![doc = include_str!("../README.md")]
#![doc(html_logo_url = "https://raw.githubusercontent.com/RustSec/logos/main/rustsec-logo-lg.png")]
#![forbid(unsafe_code)]
#![warn(missing_docs, rust_2018_idioms, unused_qualifications)]

//! ## Usage
//!
//! The [`Cvss`] type provides a unified interface for working with CVSS
//! vectors.
//!
//! The [`v3::Base`] type provides the main functionality currently implemented
//! for CVSS v3, namely: support for parsing, serializing, and scoring
//! `CVSS:3.0` and `CVSS:3.1` Base Metric Group vector strings as described in
//! the [CVSS v3.1 Specification].
//!
//! The [`v4::Vector`] type provides a fully-featured implementation of CVSS
//! v4.0, as described in the [CVSS v4.0 Specification].
//!
//! Serde support is available through the optional `serde` Cargo feature.
//!
//! [CVSS v3.1 Specification]: https://www.first.org/cvss/v3.1/specification-document
//! [CVSS v4.0 Specification]: https://www.first.org/cvss/v4.0/specification-document

// TODO(tarcieri): CVSS v2.0, CVSS v3.1 Temporal and Environmental Groups

extern crate alloc;

#[cfg(feature = "std")]
extern crate std;

// A part of the v3 API is exposed even without the feature for compatibility.
pub mod v3;
#[cfg(feature = "v4")]
pub mod v4;

#[cfg(any(feature = "v3", feature = "v4"))]
mod cvss;
mod error;
mod severity;

// For compatibility
pub use crate::v3::metric::{Metric, MetricType};

#[cfg(any(feature = "v3", feature = "v4"))]
pub use crate::cvss::Cvss;

pub use crate::{
    error::{Error, Result},
    severity::Severity,
};

/// Prefix used by all CVSS strings
pub const PREFIX: &str = "CVSS";
