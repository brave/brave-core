//! Error types

use crate::v3;
#[cfg(feature = "v4")]
use crate::v4;
use alloc::string::String;
use core::fmt;

/// Result type with the `cvss` crate's [`Error`] type.
pub type Result<T> = core::result::Result<T, Error>;

/// Kinds of errors
#[derive(Clone, Debug, Eq, PartialEq)]
#[non_exhaustive]
pub enum Error {
    /// Invalid component of a CVSS metric group.
    InvalidComponent {
        /// Invalid component.
        component: String,
    },

    /// Invalid metric for CVSSv3.
    InvalidMetric {
        /// The metric that was invalid.
        metric_type: v3::metric::MetricType,

        /// The value that was provided which is invalid.
        value: String,
    },

    #[cfg(feature = "v4")]
    /// Invalid metric for CVSSv4.
    InvalidMetricV4 {
        /// The metric that was invalid.
        metric_type: v4::MetricType,

        /// The value that was provided which is invalid.
        value: String,
    },

    #[cfg(feature = "v4")]
    /// Missing metric for CVSSv4.
    MissingMandatoryMetricV4 {
        /// Prefix which is missing.
        metric_type: v4::MetricType,
    },

    #[cfg(feature = "v4")]
    /// Metric is duplicated for CVSSv4.
    DuplicateMetricV4 {
        /// Prefix which is doubled.
        metric_type: v4::MetricType,
    },

    #[cfg(feature = "v4")]
    /// Invalid nomenclature for CVSSv4.
    InvalidNomenclatureV4 {
        /// Unknown CBSSv4 nomenclature.
        nomenclature: String,
    },

    /// Invalid CVSS string prefix.
    InvalidPrefix {
        /// Prefix which is invalid.
        prefix: String,
    },

    /// Invalid severity
    InvalidSeverity {
        /// Provided name which was unrecognized.
        name: String,
    },

    /// Unknown metric name.
    UnknownMetric {
        /// Provided name which was unrecognized.
        name: String,
    },

    /// Unsupported CVSS version
    UnsupportedVersion {
        /// Provided version string.
        version: String,
    },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::InvalidComponent { component } => {
                write!(f, "invalid CVSS metric group component: `{component}`")
            }
            Error::InvalidMetric { metric_type, value } => {
                write!(
                    f,
                    "invalid CVSSv4 {} ({}) metric: `{}`",
                    metric_type.name(),
                    metric_type.description(),
                    value
                )
            }
            #[cfg(feature = "v4")]
            Error::InvalidMetricV4 { metric_type, value } => {
                write!(
                    f,
                    "invalid CVSSv4 {} ({}) metric: `{}`",
                    metric_type.name(),
                    metric_type.description(),
                    value
                )
            }
            #[cfg(feature = "v4")]
            Error::DuplicateMetricV4 { metric_type } => {
                write!(
                    f,
                    "duplicate CVSSv4 {} ({}) metric",
                    metric_type.name(),
                    metric_type.description(),
                )
            }
            #[cfg(feature = "v4")]
            Error::MissingMandatoryMetricV4 { metric_type } => {
                write!(
                    f,
                    "missing mandatory CVSSv4 {} ({}) metric",
                    metric_type.name(),
                    metric_type.description(),
                )
            }
            #[cfg(feature = "v4")]
            Error::InvalidNomenclatureV4 { nomenclature } => {
                write!(f, "invalid CVSSv4 nomenclature: `{}`", nomenclature)
            }
            Error::InvalidPrefix { prefix } => {
                write!(f, "invalid CVSS string prefix: `{prefix}`")
            }
            Error::InvalidSeverity { name } => {
                write!(f, "invalid CVSS Qualitative Severity Rating: `{name}`")
            }
            Error::UnknownMetric { name } => write!(f, "unknown CVSS metric name: `{name}`"),
            Error::UnsupportedVersion { version } => {
                write!(f, "unsupported CVSS version: {version}")
            }
        }
    }
}

#[cfg(feature = "std")]
#[cfg_attr(docsrs, doc(cfg(feature = "std")))]
impl std::error::Error for Error {}
