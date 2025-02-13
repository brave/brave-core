/*!
Provides a centralized helper for getting the current system time.

We generally rely on the standard library for this, but the standard library
(by design) does not provide this for the `wasm{32,64}-unknown-unknown`
targets. Instead, Jiff provides an opt-in `js` feature that only applies on the
aforementioned target. Specifically, when enabled, it assumes a web context and
runs JavaScript code to get the current time.

This also exposes a "fallible" API for querying monotonic time. Since we only
use monotonic time for managing expiration for caches, in the case where we
can't get monotonic time (easily), we just consider the cache to always be
expired. ¯\_(ツ)_/¯
*/

pub(crate) use self::sys::*;

#[cfg(not(all(
    feature = "js",
    any(target_arch = "wasm32", target_arch = "wasm64"),
    target_os = "unknown"
)))]
mod sys {
    pub(crate) fn system_time() -> std::time::SystemTime {
        std::time::SystemTime::now()
    }

    #[cfg(any(
        feature = "tz-system",
        feature = "tzdb-zoneinfo",
        feature = "tzdb-concatenated"
    ))]
    pub(crate) fn monotonic_time() -> Option<std::time::Instant> {
        Some(std::time::Instant::now())
    }
}

#[cfg(all(
    feature = "js",
    any(target_arch = "wasm32", target_arch = "wasm64"),
    target_os = "unknown"
))]
mod sys {
    pub(crate) fn system_time() -> std::time::SystemTime {
        use std::time::{Duration, SystemTime};

        #[cfg(not(feature = "std"))]
        use crate::util::libm::Float;

        let millis = js_sys::Date::new_0().get_time();
        let sign = millis.signum();
        let millis = millis.abs() as u64;
        let duration = Duration::from_millis(millis);
        let result = if sign >= 0.0 {
            SystemTime::UNIX_EPOCH.checked_add(duration)
        } else {
            SystemTime::UNIX_EPOCH.checked_sub(duration)
        };
        // It's a little sad that we have to panic here, but the standard
        // SystemTime::now() API is infallible, so we kind of have to match it.
        // With that said, a panic here would be highly unusual. It would imply
        // that the system time is set to some extreme timestamp very far in the
        // future or the past.
        let Some(timestamp) = result else {
            panic!(
                "failed to get current time: \
             subtracting {duration:?} from Unix epoch overflowed"
            )
        };
        timestamp
    }

    #[cfg(any(
        feature = "tz-system",
        feature = "tzdb-zoneinfo",
        feature = "tzdb-concatenated"
    ))]
    pub(crate) fn monotonic_time() -> Option<std::time::Instant> {
        // :-(
        None
    }
}
