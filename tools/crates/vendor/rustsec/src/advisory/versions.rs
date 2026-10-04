//! The `[versions]` subsection of an advisory.

use crate::{Error, osv};
use semver::{Version, VersionReq};
use serde::{Deserialize, Serialize};

/// The `[versions]` subsection of an advisory: future home to information
/// about which versions are patched and/or unaffected.
#[derive(Clone, Debug, Default, Deserialize, Eq, PartialEq, Serialize)]
#[serde(try_from = "RawVersions")]
pub struct Versions {
    /// Versions which are patched and not vulnerable (expressed as semantic version requirements)
    patched: Vec<VersionReq>,

    /// Versions which were never affected in the first place
    #[serde(default)]
    unaffected: Vec<VersionReq>,
}

impl Versions {
    /// Is the given version of a package vulnerable?
    pub fn is_vulnerable(&self, version: &Version) -> bool {
        for range in osv::ranges_for_advisory(self).iter() {
            if range.affects(version) {
                return true;
            }
        }
        false
    }

    /// Creates a new `[versions]` entry.
    /// Checks consistency of the passed version requirements.
    pub fn new(patched: Vec<VersionReq>, unaffected: Vec<VersionReq>) -> Result<Self, Error> {
        RawVersions {
            patched,
            unaffected,
        }
        .try_into()
    }

    /// Versions which are patched and not vulnerable (expressed as semantic version requirements)
    pub fn patched(&self) -> &[VersionReq] {
        self.patched.as_slice()
    }

    /// Versions which were never affected in the first place
    pub fn unaffected(&self) -> &[VersionReq] {
        self.unaffected.as_slice()
    }
}

impl TryFrom<RawVersions> for Versions {
    type Error = Error;

    fn try_from(raw: RawVersions) -> Result<Self, Self::Error> {
        validate_ranges(&raw)?;
        Ok(Versions {
            patched: raw.patched,
            unaffected: raw.unaffected,
        })
    }
}

#[derive(Clone, Debug, Default, Deserialize, Eq, PartialEq, Serialize)]
/// Raw deserialized data that didn't pass validation yet
pub(crate) struct RawVersions {
    pub patched: Vec<VersionReq>,

    #[serde(default)]
    pub unaffected: Vec<VersionReq>,
}

fn validate_ranges(versions: &RawVersions) -> Result<(), Error> {
    let _ = osv::ranges_for_unvalidated_advisory(versions)?;
    Ok(())
}
