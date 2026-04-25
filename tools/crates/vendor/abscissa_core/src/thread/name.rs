//! Thread names.

use crate::{FrameworkError, FrameworkErrorKind::ThreadError};
use std::{fmt, str::FromStr};

/// Thread name.
///
/// Cannot contain null bytes.
#[derive(Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub struct Name(String);

impl Name {
    /// Create a new thread name
    pub fn new(name: impl ToString) -> Result<Self, FrameworkError> {
        let name = name.to_string();

        if name.contains('\0') {
            fail!(ThreadError, "name contains null bytes: {:?}", name)
        } else {
            Ok(Name(name))
        }
    }
}

impl AsRef<str> for Name {
    fn as_ref(&self) -> &str {
        &self.0
    }
}

impl FromStr for Name {
    type Err = FrameworkError;

    fn from_str(s: &str) -> Result<Self, FrameworkError> {
        Self::new(s)
    }
}

impl From<Name> for String {
    fn from(name: Name) -> String {
        name.0
    }
}

impl fmt::Display for Name {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}
