//! Package collections

use crate::error::{Error, ErrorKind};
use serde::{Deserialize, Serialize, de, ser};
use std::{fmt, str::FromStr};

/// Collections of packages (`crates` vs `rust`).
///
/// Advisories are either filed against crates published to <https://crates.io>
/// or packages provided by the Rust language itself (e.g. `std`, `rustdoc`)
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub enum Collection {
    /// Crates published through crates.io
    Crates,

    /// Rust core vulnerabilities
    Rust,
}

impl Collection {
    /// Get all collections as a slice
    pub fn all() -> &'static [Self] {
        &[Collection::Crates, Collection::Rust]
    }

    /// Get a `str` representing the kind of package
    pub fn as_str(&self) -> &str {
        match self {
            Collection::Crates => "crates",
            Collection::Rust => "rust",
        }
    }
}

impl fmt::Display for Collection {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

impl FromStr for Collection {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        Ok(match s {
            "crates" => Collection::Crates,
            "rust" => Collection::Rust,
            other => fail!(ErrorKind::Parse, "invalid package type: {}", other),
        })
    }
}

impl<'de> Deserialize<'de> for Collection {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        use de::Error;
        let string = String::deserialize(deserializer)?;
        string.parse().map_err(D::Error::custom)
    }
}

impl Serialize for Collection {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        self.to_string().serialize(serializer)
    }
}

#[cfg(test)]
mod tests {
    use super::Collection;

    #[test]
    fn parse_crate() {
        let crate_kind = "crates".parse::<Collection>().unwrap();
        assert_eq!(Collection::Crates, crate_kind);
        assert_eq!("crates", crate_kind.as_str());
    }

    #[test]
    fn parse_rust() {
        let rust_kind = "rust".parse::<Collection>().unwrap();
        assert_eq!(Collection::Rust, rust_kind);
        assert_eq!("rust", rust_kind.as_str());
    }

    #[test]
    fn parse_other() {
        assert!("foobar".parse::<Collection>().is_err());
    }
}
