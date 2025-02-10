use crate::tz::TimeZone;

#[derive(Clone)]
pub(crate) struct ZoneInfo;

impl ZoneInfo {
    pub(crate) fn from_env() -> ZoneInfo {
        ZoneInfo
    }

    #[cfg(feature = "std")]
    pub(crate) fn from_dir(
        dir: &std::path::Path,
    ) -> Result<ZoneInfo, crate::Error> {
        Err(crate::error::err!(
            "system tzdb unavailable: \
             crate feature `tzdb-zoneinfo` is disabled, \
             opening tzdb at {dir} has therefore failed",
            dir = dir.display(),
        ))
    }

    pub(crate) fn none() -> ZoneInfo {
        ZoneInfo
    }

    pub(crate) fn reset(&self) {}

    pub(crate) fn get(&self, _query: &str) -> Option<TimeZone> {
        None
    }

    #[cfg(feature = "alloc")]
    pub(crate) fn available(&self) -> alloc::vec::Vec<alloc::string::String> {
        alloc::vec::Vec::new()
    }

    pub(crate) fn is_definitively_empty(&self) -> bool {
        true
    }
}

impl core::fmt::Debug for ZoneInfo {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "ZoneInfo(unavailable)")
    }
}
