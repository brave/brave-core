//! Framework error types

use super::{context::Context, BoxError};
use std::{
    fmt::{self, Display},
    io,
    ops::Deref,
    path::PathBuf,
};

/// Abscissa-internal framework errors
#[derive(Debug)]
pub struct FrameworkError(Box<Context<FrameworkErrorKind>>);

impl Deref for FrameworkError {
    type Target = Context<FrameworkErrorKind>;

    fn deref(&self) -> &Context<FrameworkErrorKind> {
        &self.0
    }
}

impl Display for FrameworkError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0.fmt(f)
    }
}

impl From<Context<FrameworkErrorKind>> for FrameworkError {
    fn from(context: Context<FrameworkErrorKind>) -> Self {
        FrameworkError(Box::new(context))
    }
}

impl std::error::Error for FrameworkError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        self.0.source()
    }
}

/// Types of errors which occur internally within the framework
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum FrameworkErrorKind {
    /// Errors relating to components
    #[cfg(feature = "application")]
    ComponentError,

    /// Error reading configuration file
    ConfigError,

    /// I/O operation failed
    IoError,

    /// Couldn't parse the given value
    ParseError,

    /// Errors associated with filesystem paths
    PathError {
        /// Name of the affected path (if applicable)
        name: Option<PathBuf>,
    },

    /// Errors occurring in subprocess
    ProcessError,

    /// Errors involving multithreading
    ThreadError,

    /// Timeout performing operation
    TimeoutError,
}

impl FrameworkErrorKind {
    /// Create an error context from this error
    pub fn context(self, source: impl Into<BoxError>) -> Context<FrameworkErrorKind> {
        Context::new(self, Some(source.into()))
    }

    /// Get a message to display for this error
    pub fn msg(&self) -> &'static str {
        match self {
            #[cfg(feature = "application")]
            FrameworkErrorKind::ComponentError => "component error",
            FrameworkErrorKind::ConfigError => "config error",
            FrameworkErrorKind::IoError => "I/O operation failed",
            FrameworkErrorKind::ParseError => "parse error",
            FrameworkErrorKind::PathError { .. } => "path error",
            FrameworkErrorKind::ProcessError => "subprocess error",
            FrameworkErrorKind::ThreadError => "thread error",
            FrameworkErrorKind::TimeoutError => "operation timed out",
        }
    }
}

impl Display for FrameworkErrorKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.msg())?;

        if let FrameworkErrorKind::PathError { name: Some(name) } = self {
            write!(f, ": {}", name.display())?;
        }

        Ok(())
    }
}

impl std::error::Error for FrameworkErrorKind {}

impl From<io::Error> for FrameworkError {
    fn from(err: io::Error) -> Self {
        FrameworkErrorKind::IoError.context(err).into()
    }
}

#[cfg(feature = "toml")]
impl From<toml::de::Error> for FrameworkError {
    fn from(err: toml::de::Error) -> Self {
        FrameworkErrorKind::ParseError.context(err).into()
    }
}
