use crate::Error;
use serde::{Deserialize, Serialize, Serializer};
use std::fmt::{Display, Formatter};
use std::str::FromStr;

#[derive(Clone, Debug, Eq, PartialEq, Default, Deserialize)]
#[serde(from = "String")]
/// Type representing licenses used for advisory content
#[non_exhaustive]
pub enum License {
    /// Creative Commons Zero v1.0 Universal
    /// SPDX identifier: CC0-1.0
    #[default]
    CcZero10,
    /// Creative Commons Attribution 4.0 International
    /// SPDX identifier: CC-BY-4.0
    ///
    /// Note: For GitHub Security Advisories database,
    /// providing a link is [documented](https://docs.github.com/en/site-policy/github-terms/github-terms-for-additional-products-and-features#advisory-database)
    /// as fulfilling the attribution obligation for the CC-BY 4.0 license used.
    ///
    /// For advisories imported from a GitHub Security Advisory, we follow this by putting the
    /// original URL in the `url` filed of the RustSec advisory, as it assures the link will be
    /// visible to downstream users.
    CcBy40,
    /// Other SPDX requirement
    Other(String),
}

impl From<String> for License {
    fn from(s: String) -> Self {
        match s.as_str() {
            "CC0-1.0" => License::CcZero10,
            "CC-BY-4.0" => License::CcBy40,
            _ => License::Other(s),
        }
    }
}

impl Serialize for License {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_str(self.spdx())
    }
}

impl Display for License {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.spdx())
    }
}

impl License {
    /// Get license as an `&str` containing the SPDX identifier
    pub fn spdx(&self) -> &str {
        match &self {
            License::CcBy40 => "CC-BY-4.0",
            License::CcZero10 => "CC0-1.0",
            License::Other(l) => l,
        }
    }
}

impl FromStr for License {
    type Err = Error;

    // Parse standard SPDX identifiers
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Ok(match s {
            "CC0-1.0" => License::CcZero10,
            "CC-BY-4.0" => License::CcBy40,
            l => License::Other(l.to_string()),
        })
    }
}

#[cfg(test)]
mod tests {
    use crate::advisory::License;

    #[test]
    fn serialize_licenses() {
        assert_eq!(
            serde_json::to_string(&License::CcBy40).unwrap(),
            "\"CC-BY-4.0\"".to_string()
        );
        assert_eq!(
            serde_json::to_string(&License::Other("MPL-2.0".to_string())).unwrap(),
            "\"MPL-2.0\"".to_string()
        );
    }

    #[test]
    fn deserialize_licenses() {
        let l: License = serde_json::from_str("\"CC-BY-4.0\"").unwrap();
        assert_eq!(l, License::CcBy40);
        let l: License = serde_json::from_str("\"MPL-2.0\"").unwrap();
        assert_eq!(l, License::Other("MPL-2.0".to_string()));
    }
}
