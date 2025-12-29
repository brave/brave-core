//! Informational advisories: ones which don't represent an immediate security
//! threat, but something users of a crate should be warned of/aware of

use crate::{error::Error, warning};
use serde::{Deserialize, Serialize, de, ser};
use std::{fmt, str::FromStr};

/// Categories of informational vulnerabilities
#[derive(Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum Informational {
    /// Security notices for a crate which are published on <https://rustsec.org>
    /// but don't represent a vulnerability in a crate itself.
    Notice,

    /// Crate is unmaintained / abandoned
    Unmaintained,

    /// Crate is not [sound], i.e., unsound.
    ///
    /// A crate is unsound if, using its public API from safe code, it is possible to cause [Undefined Behavior].
    ///
    /// [sound]: https://rust-lang.github.io/unsafe-code-guidelines/glossary.html#soundness-of-code--of-a-library
    /// [Undefined Behavior]: https://doc.rust-lang.org/reference/behavior-considered-undefined.html
    Unsound,

    /// Other types of informational advisories: left open-ended to add
    /// more of them in the future.
    Other(String),
}

impl Informational {
    /// Get a `str` representing an [`Informational`] category
    pub fn as_str(&self) -> &str {
        match self {
            Self::Notice => "notice",
            Self::Unmaintained => "unmaintained",
            Self::Unsound => "unsound",
            Self::Other(other) => other,
        }
    }

    /// Is this informational advisory a `notice`?
    pub fn is_notice(&self) -> bool {
        *self == Self::Notice
    }

    /// Is this informational advisory for an `unmaintained` crate?
    pub fn is_unmaintained(&self) -> bool {
        *self == Self::Unmaintained
    }

    /// Is this informational advisory for an `unsound` crate?
    pub fn is_unsound(&self) -> bool {
        *self == Self::Unsound
    }

    /// Is this informational advisory of an unknown kind?
    pub fn is_other(&self) -> bool {
        matches!(self, Self::Other(_))
    }

    /// Get a warning kind for this informational type (if applicable)
    pub fn warning_kind(&self) -> Option<warning::WarningKind> {
        match self {
            Self::Notice => Some(warning::WarningKind::Notice),
            Self::Unmaintained => Some(warning::WarningKind::Unmaintained),
            Self::Unsound => Some(warning::WarningKind::Unsound),
            Self::Other(_) => None,
        }
    }
}

impl fmt::Display for Informational {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

impl FromStr for Informational {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        Ok(match s {
            "notice" => Self::Notice,
            "unmaintained" => Self::Unmaintained,
            "unsound" => Self::Unsound,
            other => Self::Other(other.to_owned()),
        })
    }
}

impl<'de> Deserialize<'de> for Informational {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        use de::Error;
        let string = String::deserialize(deserializer)?;
        string.parse().map_err(D::Error::custom)
    }
}

impl Serialize for Informational {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        self.to_string().serialize(serializer)
    }
}

#[cfg(test)]
mod tests {
    use super::Informational;

    #[test]
    fn parse_notice() {
        let notice = "notice".parse::<Informational>().unwrap();
        assert_eq!(Informational::Notice, notice);
        assert_eq!("notice", notice.as_str());
    }

    #[test]
    fn parse_unmaintained() {
        let unmaintained = "unmaintained".parse::<Informational>().unwrap();
        assert_eq!(Informational::Unmaintained, unmaintained);
        assert_eq!("unmaintained", unmaintained.as_str());
    }

    #[test]
    fn parse_other() {
        let other = "foobar".parse::<Informational>().unwrap();
        assert_eq!(Informational::Other("foobar".to_owned()), other);
        assert_eq!("foobar", other.as_str());
    }
}
