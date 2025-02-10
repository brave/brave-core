use crate::tz::TimeZone;

#[derive(Clone)]
pub(crate) struct Concatenated;

impl Concatenated {
    pub(crate) fn from_env() -> Concatenated {
        Concatenated
    }

    #[cfg(feature = "std")]
    pub(crate) fn from_path(
        path: &std::path::Path,
    ) -> Result<Concatenated, crate::Error> {
        Err(crate::error::err!(
            "system concatenated tzdb unavailable: \
             crate feature `tzdb-concatenated` is disabled, \
             opening tzdb at {path} has therefore failed",
            path = path.display(),
        ))
    }

    pub(crate) fn none() -> Concatenated {
        Concatenated
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

impl core::fmt::Debug for Concatenated {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "Concatenated(unavailable)")
    }
}
