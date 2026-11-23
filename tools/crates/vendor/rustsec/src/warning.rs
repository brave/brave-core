//! Warnings sourced from the Advisory DB

use crate::error::{Error, ErrorKind};
use crate::{advisory, package::Package};
use serde::{Deserialize, Serialize};
use std::{fmt, str::FromStr};

/// Warnings sourced from the Advisory DB
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Warning {
    /// Kind of warning
    pub kind: WarningKind,

    /// Name of the dependent package
    pub package: Package,

    /// Source advisory
    pub advisory: Option<advisory::Metadata>,

    /// More specific information about what the source advisory affects (if available)
    pub affected: Option<advisory::Affected>,

    /// Versions impacted by this warning
    pub versions: Option<advisory::Versions>,

    /// Whether this warning is denied by the audit's policy
    #[serde(default, skip_serializing_if = "std::ops::Not::not")]
    pub denied: bool,
}

impl Warning {
    /// Create `Warning` of the given kind
    pub fn new(
        kind: WarningKind,
        package: &Package,
        advisory: Option<advisory::Metadata>,
        affected: Option<advisory::Affected>,
        versions: Option<advisory::Versions>,
    ) -> Self {
        Self {
            kind,
            package: package.clone(),
            advisory,
            affected,
            versions,
            denied: false,
        }
    }

    /// Is this a warning a `notice` about a crate?
    pub fn is_notice(&self) -> bool {
        self.kind == WarningKind::Notice
    }

    /// Is this a warning about an `unmaintained` crate?
    pub fn is_unmaintained(&self) -> bool {
        self.kind == WarningKind::Unmaintained
    }

    /// Is this a warning about an `unsound` crate?
    pub fn is_unsound(&self) -> bool {
        self.kind == WarningKind::Unsound
    }

    /// Is this a warning about a yanked crate?
    pub fn is_yanked(&self) -> bool {
        self.kind == WarningKind::Yanked
    }
}

/// Kinds of warnings
#[derive(Copy, Clone, Debug, Deserialize, Eq, Hash, PartialEq, PartialOrd, Serialize, Ord)]
#[non_exhaustive]
pub enum WarningKind {
    /// Informational notices about packages
    #[serde(rename = "notice")]
    Notice,

    /// Unmaintained packages
    #[serde(rename = "unmaintained")]
    Unmaintained,

    /// Unsound packages
    #[serde(rename = "unsound")]
    Unsound,

    /// Yanked packages
    #[serde(rename = "yanked")]
    Yanked,
}

impl WarningKind {
    /// Get a `str` representing an warning [`WarningKind`]
    pub fn as_str(&self) -> &str {
        match self {
            Self::Notice => "notice",
            Self::Unmaintained => "unmaintained",
            Self::Unsound => "unsound",
            Self::Yanked => "yanked",
        }
    }
}

impl FromStr for WarningKind {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        Ok(match s {
            "notice" => Self::Notice,
            "unmaintained" => Self::Unmaintained,
            "unsound" => Self::Unsound,
            "yanked" => Self::Yanked,
            other => fail!(ErrorKind::Parse, "invalid warning type: {}", other),
        })
    }
}

impl fmt::Display for WarningKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn denied_json_roundtrip() {
        let package: Package =
            serde_json::from_str(r#"{"name":"example","version":"1.0.0"}"#).unwrap();
        let mut warning = Warning::new(WarningKind::Yanked, &package, None, None, None);
        assert!(!warning.denied);
        let json = serde_json::to_string(&warning).unwrap();
        assert!(!json.contains("\"denied\""));
        assert!(!serde_json::from_str::<Warning>(&json).unwrap().denied);

        warning.denied = true;
        let json = serde_json::to_string(&warning).unwrap();
        assert!(serde_json::from_str::<Warning>(&json).unwrap().denied);
    }
}
