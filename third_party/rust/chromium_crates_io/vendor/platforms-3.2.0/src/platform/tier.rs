//! Rust platform tiers

use crate::error::Error;
use core::{convert::TryFrom, fmt, str::FromStr};

#[cfg(feature = "serde")]
use serde::{de, ser, Deserialize, Serialize};

/// Rust platform tiers: support levels are organized into three tiers, each
/// with a different set of guarantees.
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub enum Tier {
    /// Tier 1 platforms can be thought of as “guaranteed to work”.
    /// Specifically they will each satisfy the following requirements:
    ///
    /// * Official binary releases are provided for the platform.
    /// * Automated testing is set up to run tests for the platform.
    /// * Landing changes to the rust-lang/rust repository’s master branch
    ///   is gated on tests passing.
    /// * Documentation for how to use and how to build the platform is available.
    One,

    /// Tier 2 platforms can be thought of as “guaranteed to build”. Automated
    /// tests are not run so it’s not guaranteed to produce a working build,
    /// but platforms often work to quite a good degree and patches are always
    /// welcome!
    ///
    /// Specifically, these platforms are required to have each of the following:
    ///
    /// * Official binary releases are provided for the platform.
    /// * Automated building is set up, but may not be running tests.
    /// * Landing changes to the rust-lang/rust repository’s master branch is
    ///   gated on platforms building. For some platforms only the standard
    ///   library is compiled, but for others rustc and cargo are too.
    Two,

    /// Tier 3 platforms are those which the Rust codebase has support for, but
    /// which are not built or tested automatically, and may not work.
    /// Official builds are not available.
    Three,
}

impl Tier {
    /// Get a number identifying this tier
    pub fn to_usize(self) -> usize {
        match self {
            Tier::One => 1,
            Tier::Two => 2,
            Tier::Three => 3,
        }
    }

    /// Get a string identifying this tier
    pub fn as_str(self) -> &'static str {
        match self {
            Tier::One => "tier1",
            Tier::Two => "tier2",
            Tier::Three => "tier3",
        }
    }
}

impl From<Tier> for usize {
    fn from(tier: Tier) -> usize {
        tier.to_usize()
    }
}

impl TryFrom<usize> for Tier {
    type Error = Error;

    fn try_from(num: usize) -> Result<Tier, Error> {
        match num {
            1 => Ok(Tier::One),
            2 => Ok(Tier::Two),
            3 => Ok(Tier::Three),
            _ => Err(Error),
        }
    }
}

impl FromStr for Tier {
    type Err = Error;

    fn from_str(s: &str) -> Result<Tier, Error> {
        match s {
            "tier1" => Ok(Tier::One),
            "tier2" => Ok(Tier::Two),
            "tier3" => Ok(Tier::Three),
            _ => Err(Error),
        }
    }
}

impl fmt::Display for Tier {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl Serialize for Tier {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(self.as_str())
    }
}

#[cfg(all(feature = "serde", feature = "std"))]
impl<'de> Deserialize<'de> for Tier {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        use de::Error;
        std::string::String::deserialize(deserializer)?
            .parse()
            .map_err(D::Error::custom)
    }
}
