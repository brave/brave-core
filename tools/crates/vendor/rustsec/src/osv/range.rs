use semver::Version;

/// A range of affected versions.
///
/// If any of the bounds is unspecified, that means ALL versions
/// in that direction are affected.
///
/// This format is defined by <https://github.com/google/osv>
#[derive(Debug, Clone)]
pub struct OsvRange {
    /// Inclusive
    pub introduced: Option<Version>,
    /// Exclusive
    pub fixed: Option<Version>,
}

impl OsvRange {
    /// Returns true if the given version is affected
    pub fn affects(&self, v: &Version) -> bool {
        (match &self.introduced {
            None => true,
            Some(start_v) => v >= start_v,
        }) && (match &self.fixed {
            None => true,
            Some(end_v) => v < end_v,
        })
    }
}
