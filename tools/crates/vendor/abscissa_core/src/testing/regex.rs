//! Regex newtype for simplifying conversions from the `regex` crate

use std::{fmt, ops::Deref};

/// Regex newtype (wraps `regex::Regex`)
#[derive(Clone)]
pub struct Regex(regex::Regex);

impl Regex {
    /// Compile a regular expression
    pub fn new(re: &str) -> Result<Self, regex::Error> {
        regex::Regex::new(re).map(Regex)
    }
}

impl From<regex::Regex> for Regex {
    fn from(re: regex::Regex) -> Regex {
        Regex(re)
    }
}

impl From<&str> for Regex {
    fn from(re: &str) -> Regex {
        let re_compiled = regex::Regex::new(re)
            .unwrap_or_else(|err| panic!("error compiling regex: {} ({})", re, err));

        Regex(re_compiled)
    }
}

impl Deref for Regex {
    type Target = regex::Regex;

    fn deref(&self) -> &regex::Regex {
        &self.0
    }
}

impl fmt::Debug for Regex {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Display::fmt(&self.0, f)
    }
}
