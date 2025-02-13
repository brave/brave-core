//! Error types

use crate::MetricType;
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

    /// Invalid metric.
    InvalidMetric {
        /// The metric that was invalid.
        metric_type: MetricType,

        /// The value that was provided which is invalid.
        value: String,
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
                write!(f, "invalid CVSS metric group component: `{}`", component)
            }
            Error::InvalidMetric { metric_type, value } => {
                write!(
                    f,
                    "invalid CVSS {} ({}) metric: `{}`",
                    metric_type.name(),
                    metric_type.description(),
                    value
                )
            }
            Error::InvalidPrefix { prefix } => {
                write!(f, "invalid CVSS string prefix: `{}`", prefix)
            }
            Error::InvalidSeverity { name } => {
                write!(f, "invalid CVSS Qualitative Severity Rating: `{}`", name)
            }
            Error::UnknownMetric { name } => write!(f, "unknown CVSS metric name: `{}`", name),
            Error::UnsupportedVersion { version } => {
                write!(f, "unsupported CVSS version: {}", version)
            }
        }
    }
}

#[cfg(feature = "std")]
#[cfg_attr(docsrs, doc(cfg(feature = "std")))]
impl std::error::Error for Error {}
