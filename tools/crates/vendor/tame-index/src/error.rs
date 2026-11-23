//! Provides the various error types for this crate

#[cfg(feature = "local")]
pub use crate::index::local::LocalRegistryError;

/// The core error type for this library
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Failed to deserialize a local cache entry
    #[error(transparent)]
    Cache(#[from] CacheError),
    /// This library assumes utf-8 paths in all cases, a path was provided that
    /// was not valid utf-8
    #[error("unable to use non-utf8 path {:?}", .0)]
    NonUtf8Path(std::path::PathBuf),
    /// An environment variable was located, but had a non-utf8 value
    #[error("environment variable {} has a non-utf8 value", .0)]
    NonUtf8EnvVar(std::borrow::Cow<'static, str>),
    /// A user-provided string was not a valid crate name
    #[error(transparent)]
    InvalidKrateName(#[from] InvalidKrateName),
    /// The user specified a registry name that did not exist in any searched
    /// .cargo/config.toml
    #[error("registry '{}' was not located in any .cargo/config.toml", .0)]
    UnknownRegistry(String),
    /// An I/O error
    #[error(transparent)]
    Io(#[from] std::io::Error),
    /// An I/O error occurred trying to access a specific path
    #[error("I/O operation failed for path '{}': {}", .1, .0)]
    IoPath(#[source] std::io::Error, crate::PathBuf),
    /// A user provided URL was invalid
    #[error(transparent)]
    InvalidUrl(#[from] InvalidUrl),
    /// Failed to de/serialize JSON
    #[error(transparent)]
    Json(#[from] serde_json::Error),
    /// Failed to deserialize TOML
    #[error(transparent)]
    Toml(#[from] Box<toml_span::Error>),
    /// An index entry did not contain any versions
    #[error("index entry contained no versions for the crate")]
    NoCrateVersions,
    /// Failed to handle an HTTP response or request
    #[error(transparent)]
    Http(#[from] HttpError),
    /// Failed to parse a semver version or requirement
    #[error(transparent)]
    Semver(#[from] semver::Error),
    /// A local registry is invalid
    #[cfg(feature = "local")]
    #[error(transparent)]
    Local(#[from] LocalRegistryError),
    /// Failed to lock a file
    #[error(transparent)]
    Lock(#[from] crate::utils::flock::FileLockError),
}

impl From<std::path::PathBuf> for Error {
    fn from(p: std::path::PathBuf) -> Self {
        Self::NonUtf8Path(p)
    }
}

/// Various kinds of reserved names disallowed by cargo
#[derive(Debug, Copy, Clone)]
pub enum ReservedNameKind {
    /// The name is a Rust keyword
    Keyword,
    /// The name conflicts with a cargo artifact directory
    Artifact,
    /// The name has a special meaning on Windows
    Windows,
    /// The name conflicts with a Rust std library name
    Standard,
}

impl std::fmt::Display for ReservedNameKind {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Keyword => f.write_str("rustlang keyword"),
            Self::Artifact => f.write_str("cargo artifact"),
            Self::Windows => f.write_str("windows reserved"),
            Self::Standard => f.write_str("rustlang std library"),
        }
    }
}

/// Errors that can occur when validating a crate name
#[derive(Debug, thiserror::Error)]
pub enum InvalidKrateName {
    /// The name had an invalid length
    #[error("crate name had an invalid length of '{0}'")]
    InvalidLength(usize),
    /// The name contained an invalid character
    #[error("invalid character '{invalid}` @ {index}")]
    InvalidCharacter {
        /// The invalid character
        invalid: char,
        /// The index of the character in the provided string
        index: usize,
    },
    /// The name was one of the reserved names disallowed by cargo
    #[error("the name '{reserved}' is reserved as '{kind}`")]
    ReservedName {
        /// The name that was reserved
        reserved: &'static str,
        /// The kind of the reserved name
        kind: ReservedNameKind,
    },
}

/// An error pertaining to a bad URL provided to the API
#[derive(Debug, thiserror::Error)]
#[error("the url '{url}' is invalid")]
pub struct InvalidUrl {
    /// The invalid url
    pub url: String,
    /// The reason it is invalid
    pub source: InvalidUrlError,
}

/// The specific reason for the why the URL is invalid
#[derive(Debug, thiserror::Error)]
pub enum InvalidUrlError {
    /// Sparse HTTP registry urls must be of the form `sparse+http(s)://`
    #[error("sparse indices require the use of a url that starts with `sparse+http`")]
    MissingSparse,
    /// The `<modifier>+<scheme>://` is not supported
    #[error("the scheme modifier is unknown")]
    UnknownSchemeModifier,
    /// Unable to find the `<scheme>://`
    #[error("the scheme is missing")]
    MissingScheme,
    /// Attempted to construct a git index with a sparse URL
    #[error("attempted to create a git index for a sparse URL")]
    SparseForGit,
}

/// Errors related to a local index cache
#[derive(Debug, thiserror::Error)]
pub enum CacheError {
    /// The cache entry is malformed
    #[error("the cache entry is malformed")]
    InvalidCacheEntry,
    /// The cache version is old
    #[error("the cache entry is an old, unsupported version")]
    OutdatedCacheVersion,
    /// The cache version is newer than the version supported by this crate
    #[error("the cache entry is an unknown version, possibly written by a newer cargo version")]
    UnknownCacheVersion,
    /// The index version is newer than the version supported by this crate
    #[error(
        "the cache entry's index version is unknown, possibly written by a newer cargo version"
    )]
    UnknownIndexVersion,
    /// The revision in the cache entry did match the requested revision
    ///
    /// This can occur when a git index is fetched and a newer revision is pulled
    /// from the remote index, invalidating all local cache entries
    #[error("the cache entry's revision does not match the current revision")]
    OutdatedRevision,
    /// A crate version in the cache file was malformed
    #[error("a specific version in the cache entry is malformed")]
    InvalidCrateVersion,
}

/// Errors related to HTTP requests or responses
#[derive(Debug, thiserror::Error)]
pub enum HttpError {
    /// A [`reqwest::Error`]
    #[cfg(any(feature = "sparse", feature = "local-builder"))]
    #[error(transparent)]
    Reqwest(#[from] reqwest::Error),
    /// A status code was received that indicates user error, or possibly a
    /// remote index that does not follow the protocol supported by this crate
    #[error("status code '{code}': {msg}")]
    StatusCode {
        /// The status code
        code: http::StatusCode,
        /// The reason the status code raised an error
        msg: &'static str,
    },
    /// A [`http::Error`]
    #[error(transparent)]
    Http(#[from] http::Error),
    /// A string could not be parsed as a valid header value
    #[error(transparent)]
    InvalidHeaderValue(#[from] http::header::InvalidHeaderValue),
    /// Unable to complete an async request for an `AsyncRemoteSparseIndex` within
    /// the user allotted time
    #[error("request could not be completed in the allotted timeframe")]
    Timeout,
}
