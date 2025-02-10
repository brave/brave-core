use crate::tz::TimeZone;

#[derive(Clone)]
pub(crate) struct BundledZoneInfo;

impl BundledZoneInfo {
    pub(crate) fn new() -> BundledZoneInfo {
        BundledZoneInfo
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

impl core::fmt::Debug for BundledZoneInfo {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "Bundled(unavailable)")
    }
}
