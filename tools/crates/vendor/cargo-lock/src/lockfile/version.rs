//! Lockfile versions

use super::encoding::EncodablePackage;
use crate::{
    error::{Error, Result},
    metadata::Metadata,
};
use serde::{Deserialize, Serialize};
use std::str::FromStr;

/// Lockfile versions
#[derive(Copy, Clone, Debug, Deserialize, Eq, Hash, PartialEq, PartialOrd, Ord, Serialize)]
#[non_exhaustive]
#[repr(u32)]
pub enum ResolveVersion {
    /// Original `Cargo.lock` format which places checksums in the
    /// `[[metadata]]` table.
    V1 = 1,

    /// Revised `Cargo.lock` format which is optimized to prevent merge
    /// conflicts.
    ///
    /// For more information, see:
    /// <https://github.com/rust-lang/cargo/pull/7070>
    V2 = 2,

    /// Encodes Git dependencies with `branch = 'master'` in the manifest as
    /// `?branch=master` in their URLs.
    ///
    /// For more information, see:
    /// <https://internals.rust-lang.org/t/upcoming-changes-to-cargo-lock/14017>
    V3 = 3,

    /// SourceId URL serialization is aware of URL encoding.
    ///
    /// For more information, see:
    /// <https://github.com/rust-lang/cargo/pull/12852>
    V4 = 4,
}

impl ResolveVersion {
    /// Autodetect the version of a lockfile from the packages
    pub(super) fn detect(packages: &[EncodablePackage], metadata: &Metadata) -> Result<Self> {
        // V1: look for [[metadata]] keys beginning with checksum
        let is_v1 = metadata.keys().any(|key| key.is_checksum());

        // V2: look for `checksum` fields in `[package]`
        let is_v2 = packages.iter().any(|package| package.checksum.is_some());

        if is_v1 && is_v2 {
            return Err(Error::Parse("malformed lockfile: contains checksums in both [[package]] and [[metadata]] sections".to_string()));
        }

        if is_v1 {
            Ok(ResolveVersion::V1)
        } else {
            // Default to V2
            Ok(ResolveVersion::V2)
        }
    }

    /// Should this version be explicitly encoded?
    pub(super) fn is_explicit(self) -> bool {
        u32::from(self) >= 3
    }
}

/// V3 format is now the default.
impl Default for ResolveVersion {
    fn default() -> Self {
        ResolveVersion::V3
    }
}

impl From<ResolveVersion> for u32 {
    fn from(version: ResolveVersion) -> u32 {
        version as u32
    }
}

impl FromStr for ResolveVersion {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        u32::from_str(s)
            .map_err(|_| Error::Parse(format!("invalid Cargo.lock format version: `{s}`")))
            .and_then(Self::try_from)
    }
}

impl TryFrom<u32> for ResolveVersion {
    type Error = Error;

    fn try_from(num: u32) -> Result<Self> {
        match num {
            1 => Ok(ResolveVersion::V1),
            2 => Ok(ResolveVersion::V2),
            3 => Ok(ResolveVersion::V3),
            4 => Ok(ResolveVersion::V4),
            _ => Err(Error::Parse(format!(
                "invalid Cargo.lock format version: `{num}`"
            ))),
        }
    }
}
