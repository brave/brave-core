//! Error types

use alloc::string::String;
use core::fmt;

#[cfg(feature = "v2")]
use crate::v2;
#[cfg(feature = "v3")]
use crate::v3;
#[cfg(feature = "v4")]
use crate::v4;

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
    #[cfg(feature = "v3")]
    InvalidMetricV3 {
        /// The metric that was invalid.
        metric_type: v3::metric::MetricType,

        /// The value that was provided which is invalid.
        value: String,
    },

    #[cfg(feature = "v2")]
    /// Invalid metric for CVSS 2.0
    InvalidMetricV2 {
        /// The metric that was invalid.
        metric_type: v2::MetricType,

        /// The value that was provided which is invalid.
        value: String,
    },

    #[cfg(feature = "v2")]
    /// Metric is duplicated for CVSS v2.0.
    DuplicateMetricV2 {
        /// Prefix which is doubled.
        metric_type: v2::MetricType,
    },

    /// Metric is duplicated for CVSSv3.
    #[cfg(feature = "v3")]
    DuplicateMetricV3 {
        /// Prefix which is doubled.
        metric_type: v3::metric::MetricType,
    },

    #[cfg(feature = "v2")]
    /// Missing metric for CVSS v2.0.
    MissingMandatoryMetricV2 {
        /// Metric which is missing.
        metric_type: v2::MetricType,
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
        /// Unknown CVSSv4 nomenclature.
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
            Self::InvalidComponent { component } => {
                write!(f, "invalid CVSS metric group component: `{component}`")
            }
            #[cfg(feature = "v3")]
            Self::InvalidMetricV3 { metric_type, value } => {
                write!(
                    f,
                    "invalid CVSSv3 {} ({}) metric: `{}`",
                    metric_type.name(),
                    metric_type.description(),
                    value
                )
            }
            #[cfg(feature = "v2")]
            Self::InvalidMetricV2 { metric_type, value } => {
                write!(
                    f,
                    "invalid CVSSv2 {} ({}) metric: `{}`",
                    metric_type.name(),
                    metric_type.description(),
                    value
                )
            }
            #[cfg(feature = "v2")]
            Self::DuplicateMetricV2 { metric_type } => {
                write!(
                    f,
                    "duplicate CVSSv2 {} ({}) metric",
                    metric_type.name(),
                    metric_type.description(),
                )
            }
            #[cfg(feature = "v3")]
            Self::DuplicateMetricV3 { metric_type } => {
                write!(
                    f,
                    "duplicate CVSSv3 {} ({}) metric",
                    metric_type.name(),
                    metric_type.description(),
                )
            }
            #[cfg(feature = "v2")]
            Self::MissingMandatoryMetricV2 { metric_type } => {
                write!(
                    f,
                    "missing mandatory CVSSv2 {} ({}) metric",
                    metric_type.name(),
                    metric_type.description(),
                )
            }
            #[cfg(feature = "v4")]
            Self::InvalidMetricV4 { metric_type, value } => {
                write!(
                    f,
                    "invalid CVSSv4 {} ({}) metric: `{}`",
                    metric_type.name(),
                    metric_type.description(),
                    value
                )
            }
            #[cfg(feature = "v4")]
            Self::DuplicateMetricV4 { metric_type } => {
                write!(
                    f,
                    "duplicate CVSSv4 {} ({}) metric",
                    metric_type.name(),
                    metric_type.description(),
                )
            }
            #[cfg(feature = "v4")]
            Self::MissingMandatoryMetricV4 { metric_type } => {
                write!(
                    f,
                    "missing mandatory CVSSv4 {} ({}) metric",
                    metric_type.name(),
                    metric_type.description(),
                )
            }
            #[cfg(feature = "v4")]
            Self::InvalidNomenclatureV4 { nomenclature } => {
                write!(f, "invalid CVSSv4 nomenclature: `{}`", nomenclature)
            }
            Self::InvalidPrefix { prefix } => {
                write!(f, "invalid CVSS string prefix: `{prefix}`")
            }
            Self::InvalidSeverity { name } => {
                write!(f, "invalid CVSS Qualitative Severity Rating: `{name}`")
            }
            Self::UnknownMetric { name } => write!(f, "unknown CVSS metric name: `{name}`"),
            Self::UnsupportedVersion { version } => {
                write!(f, "unsupported CVSS version: {version}")
            }
        }
    }
}

#[cfg(feature = "std")]
#[cfg_attr(docsrs, doc(cfg(feature = "std")))]
impl std::error::Error for Error {}
