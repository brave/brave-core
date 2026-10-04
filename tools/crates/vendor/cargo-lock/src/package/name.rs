//! Package names

use crate::Error;
use serde::{Deserialize, Serialize};
use std::{fmt, str::FromStr};

/// Name of a Rust `[[package]]`
#[derive(Clone, Debug, Deserialize, Eq, Hash, PartialEq, PartialOrd, Ord, Serialize)]
pub struct Name(String);

impl Name {
    /// Get package name as an `&str`
    pub fn as_str(&self) -> &str {
        &self.0
    }
}

impl AsRef<str> for Name {
    fn as_ref(&self) -> &str {
        self.as_str()
    }
}

impl fmt::Display for Name {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0.fmt(f)
    }
}

impl From<Name> for String {
    fn from(name: Name) -> String {
        name.0
    }
}

impl FromStr for Name {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        // TODO(tarcieri): ensure name is valid
        Ok(Name(s.into()))
    }
}
