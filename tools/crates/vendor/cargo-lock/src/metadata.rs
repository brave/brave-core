//! Package metadata

use crate::{
    error::{Error, Result},
    lockfile::encoding::EncodableDependency,
    Checksum, Dependency, Map,
};
use serde::{de, ser, Deserialize, Serialize};
use std::{fmt, str::FromStr};

/// Prefix of metadata keys for checksum entries
const CHECKSUM_PREFIX: &str = "checksum ";

/// Package metadata
pub type Metadata = Map<MetadataKey, MetadataValue>;

/// Keys for the `[metadata]` table
#[derive(Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub struct MetadataKey(String);

impl MetadataKey {
    /// Create a metadata key for a checksum for the given dependency
    pub fn for_checksum(dep: &Dependency) -> Self {
        MetadataKey(format!("{CHECKSUM_PREFIX}{dep}"))
    }

    /// Is this metadata key a checksum entry?
    pub fn is_checksum(&self) -> bool {
        self.0.starts_with(CHECKSUM_PREFIX)
    }

    /// Get the dependency for a particular checksum value (if applicable)
    pub fn checksum_dependency(&self) -> Result<Dependency> {
        self.try_into()
    }
}

impl AsRef<str> for MetadataKey {
    fn as_ref(&self) -> &str {
        &self.0
    }
}

impl fmt::Display for MetadataKey {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl FromStr for MetadataKey {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        Ok(MetadataKey(s.to_owned()))
    }
}

impl TryFrom<&MetadataKey> for Dependency {
    type Error = Error;

    fn try_from(key: &MetadataKey) -> Result<Dependency> {
        if !key.is_checksum() {
            return Err(Error::Parse(
                "can only parse dependencies from `checksum` metadata".to_owned(),
            ));
        }

        let dep = EncodableDependency::from_str(&key.as_ref()[CHECKSUM_PREFIX.len()..])?;
        (&dep).try_into()
    }
}

impl<'de> Deserialize<'de> for MetadataKey {
    fn deserialize<D: de::Deserializer<'de>>(
        deserializer: D,
    ) -> std::result::Result<Self, D::Error> {
        String::deserialize(deserializer)?
            .parse()
            .map_err(de::Error::custom)
    }
}

impl Serialize for MetadataKey {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> std::result::Result<S::Ok, S::Error> {
        self.to_string().serialize(serializer)
    }
}

/// Values in the `[metadata]` table
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct MetadataValue(String);

impl MetadataValue {
    /// Get the associated checksum for this value (if applicable)
    pub fn checksum(&self) -> Result<Checksum> {
        self.try_into()
    }
}

impl AsRef<str> for MetadataValue {
    fn as_ref(&self) -> &str {
        &self.0
    }
}

impl fmt::Display for MetadataValue {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl FromStr for MetadataValue {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        Ok(MetadataValue(s.to_owned()))
    }
}

impl TryFrom<&MetadataValue> for Checksum {
    type Error = Error;

    fn try_from(value: &MetadataValue) -> Result<Checksum> {
        value.as_ref().parse()
    }
}

impl<'de> Deserialize<'de> for MetadataValue {
    fn deserialize<D: de::Deserializer<'de>>(
        deserializer: D,
    ) -> std::result::Result<Self, D::Error> {
        String::deserialize(deserializer)?
            .parse()
            .map_err(de::Error::custom)
    }
}

impl Serialize for MetadataValue {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> std::result::Result<S::Ok, S::Error> {
        self.to_string().serialize(serializer)
    }
}
