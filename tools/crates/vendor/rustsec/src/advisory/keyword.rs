//! Advisory keywords

use crate::error::Error;
use serde::{Deserialize, Deserializer, Serialize, de::Error as DeError};
use std::str::FromStr;

/// Keywords on advisories, similar to Cargo keywords
#[derive(Clone, Debug, Eq, PartialEq, PartialOrd, Ord, Serialize)]
pub struct Keyword(String);

impl Keyword {
    /// Borrow this keyword as a string slice
    pub fn as_str(&self) -> &str {
        self.0.as_ref()
    }
}

impl AsRef<str> for Keyword {
    fn as_ref(&self) -> &str {
        self.as_str()
    }
}

impl<'de> Deserialize<'de> for Keyword {
    fn deserialize<D: Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        Self::from_str(&String::deserialize(deserializer)?)
            .map_err(|e| D::Error::custom(format!("{e}")))
    }
}

impl FromStr for Keyword {
    type Err = Error;

    /// Create a new keyword
    fn from_str(keyword: &str) -> Result<Self, Error> {
        // TODO: validate keywords according to Cargo-like rules
        Ok(Keyword(keyword.into()))
    }
}
