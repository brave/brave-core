//! Error contexts

use super::BoxError;
use backtrace::Backtrace;
use std::fmt::{self, Debug, Display};

/// Error context
#[derive(Debug)]
pub struct Context<Kind>
where
    Kind: Clone + Debug + Display + Eq + PartialEq + Into<BoxError>,
{
    /// Kind of error
    kind: Kind,

    /// Backtrace where error occurred
    backtrace: Option<Backtrace>,

    /// Source of the error
    source: Option<BoxError>,
}

impl<Kind> Context<Kind>
where
    Kind: Clone + Debug + Display + Eq + PartialEq + Into<BoxError>,
{
    /// Create a new error context
    pub fn new(kind: Kind, source: Option<BoxError>) -> Self {
        let backtrace = Some(Backtrace::new_unresolved());
        Context {
            kind,
            backtrace,
            source,
        }
    }

    /// Get the kind of error
    pub fn kind(&self) -> &Kind {
        &self.kind
    }

    /// Get the backtrace associated with this error (if available)
    pub fn backtrace(&self) -> Option<&Backtrace> {
        self.backtrace.as_ref()
    }

    /// Extract the backtrace from the context, allowing it to be resolved.
    pub fn into_backtrace(self) -> Option<Backtrace> {
        self.backtrace
    }
}

impl<Kind> Display for Context<Kind>
where
    Kind: Clone + Debug + Display + Eq + PartialEq + Into<BoxError>,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", &self.kind)?;

        if let Some(ref source) = self.source {
            write!(f, ": {}", source)?;
        }

        Ok(())
    }
}

impl<Kind> From<Kind> for Context<Kind>
where
    Kind: Clone + Debug + Display + Eq + PartialEq + Into<BoxError>,
{
    fn from(kind: Kind) -> Context<Kind> {
        Self::new(kind, None)
    }
}

impl<Kind> std::error::Error for Context<Kind>
where
    Kind: Clone + Debug + Display + Eq + PartialEq + Into<BoxError>,
{
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        self.source
            .as_ref()
            .map(|source| source.as_ref() as &(dyn std::error::Error + 'static))
    }
}
