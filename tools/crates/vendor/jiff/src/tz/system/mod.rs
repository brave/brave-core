use std::{sync::RwLock, time::Duration};

use alloc::string::ToString;

use crate::{
    error::{err, Error, ErrorContext},
    tz::{posix::PosixTz, TimeZone, TimeZoneDatabase},
    util::cache::Expiration,
};

#[cfg(all(unix, not(target_os = "android")))]
#[path = "unix.rs"]
mod sys;

#[cfg(all(unix, target_os = "android"))]
#[path = "android.rs"]
mod sys;

#[cfg(windows)]
#[path = "windows/mod.rs"]
mod sys;

#[cfg(all(
    feature = "js",
    any(target_arch = "wasm32", target_arch = "wasm64"),
    target_os = "unknown"
))]
#[path = "wasm_js.rs"]
mod sys;

#[cfg(not(any(
    unix,
    windows,
    all(
        feature = "js",
        any(target_arch = "wasm32", target_arch = "wasm64"),
        target_os = "unknown"
    )
)))]
mod sys {
    use crate::tz::{TimeZone, TimeZoneDatabase};

    pub(super) fn get(_db: &TimeZoneDatabase) -> Option<TimeZone> {
        warn!("getting system time zone on this platform is unsupported");
        None
    }

    pub(super) fn read(
        _db: &TimeZoneDatabase,
        path: &str,
    ) -> Option<TimeZone> {
        match super::read_unnamed_tzif_file(path) {
            Ok(tz) => Some(tz),
            Err(_err) => {
                trace!("failed to read {path} as unnamed time zone: {_err}");
                None
            }
        }
    }
}

/// The duration of time that a cached time zone should be considered valid.
static TTL: Duration = Duration::new(5 * 60, 0);

/// A cached time zone.
///
/// When there's a cached time zone that hasn't expired, then we return what's
/// in the cache. This is because determining the time zone can be mildly
/// expensive. For example, doing syscalls and potentially parsing TZif data.
///
/// We could use a `thread_local!` for this instead which may perhaps be
/// faster.
///
/// Note that our cache here is somewhat simplistic because we lean on the
/// fact that: 1) in the vast majority of cases, our platform specific code is
/// limited to finding a time zone name, and 2) looking up a time zone name in
/// `TimeZoneDatabase` has its own cache. The main cases this doesn't really
/// cover are when we can't find a time zone name. In which case, we might be
/// re-parsing POSIX TZ strings or TZif data unnecessarily. But it's not clear
/// this matters much. It might matter more if we shrink our TTL though.
static CACHE: RwLock<Cache> = RwLock::new(Cache::empty());

/// A simple global mutable cache of the most recently created system
/// `TimeZone`.
///
/// This gets clearer periodically where subsequent calls to `get` must
/// re-create the time zone. This is likely wasted work in the vast majority
/// of cases, but the TTL should ensure it doesn't happen too often.
///
/// Of course, in those cases where you want it to happen faster, we provide
/// a way to reset this cache and force a re-creation of the time zone.
struct Cache {
    tz: Option<TimeZone>,
    expiration: Expiration,
}

impl Cache {
    /// Create an empty cache. The default state.
    const fn empty() -> Cache {
        Cache { tz: None, expiration: Expiration::expired() }
    }
}

/// Retrieve the "system" time zone.
///
/// If there is a cached time zone that isn't stale, then that is returned
/// instead.
///
/// If there is no cached time zone, then this tries to determine the system
/// time zone in a platform specific manner. This may involve reading files
/// or making system calls. If that fails then an error is returned.
///
/// Note that the `TimeZone` returned may not have an IANA name! In some cases,
/// it is just impractical to determine the time zone name. For example, when
/// `/etc/localtime` is a hard link to a TZif file instead of a symlink and
/// when the time zone name isn't recorded in any of the other obvious places.
pub(crate) fn get(db: &TimeZoneDatabase) -> Result<TimeZone, Error> {
    {
        let cache = CACHE.read().unwrap();
        if let Some(ref tz) = cache.tz {
            if !cache.expiration.is_expired() {
                return Ok(tz.clone());
            }
        }
    }
    let tz = get_force(db)?;
    {
        // It's okay that we race here. We basically assume that any
        // sufficiently close but approximately simultaneous detection of
        // "system" time will lead to the same result. Of course, this is not
        // strictly true, but since we invalidate the cache after a TTL, it
        // will eventually be true in any sane environment.
        let mut cache = CACHE.write().unwrap();
        cache.tz = Some(tz.clone());
        cache.expiration = Expiration::after(TTL);
    }
    Ok(tz)
}

/// Always attempt retrieve the system time zone. This never uses a cache.
pub(crate) fn get_force(db: &TimeZoneDatabase) -> Result<TimeZone, Error> {
    match get_env_tz(db) {
        Ok(Some(tz)) => {
            debug!("checked TZ environment variable and found {tz:?}");
            return Ok(tz);
        }
        Ok(None) => {
            trace!("checked TZ environment variable but found nothing");
        }
        Err(_err) => {
            trace!("checked TZ environment variable but got error: {_err}");
        }
    }
    if let Some(tz) = sys::get(db) {
        return Ok(tz);
    }
    Err(err!("failed to find system time zone"))
}

/// Materializes a `TimeZone` from a `TZ` environment variable.
///
/// Basically, `TZ` is usually just an IANA Time Zone Database name like
/// `TZ=America/New_York` or `TZ=UTC`. But it can also be a POSIX time zone
/// transition string like `TZ=EST5EDT` or it can be a file path (absolute
/// or relative) to a TZif file.
///
/// We try very hard to extract a time zone name from `TZ` and use that to look
/// it up via `TimeZoneDatabase`. But we will fall back to unnamed TZif
/// `TimeZone` if necessary.
fn get_env_tz(db: &TimeZoneDatabase) -> Result<Option<TimeZone>, Error> {
    // This routine is pretty Unix-y, but there's no reason it can't
    // partially work on Windows. For example, setting TZ=America/New_York
    // should work totally fine on Windows. I don't see a good reason not to
    // support it anyway.

    let Some(tzenv) = std::env::var_os("TZ") else { return Ok(None) };
    if tzenv.is_empty() {
        return Ok(None);
    }
    let tz_name_or_path = match PosixTz::parse_os_str(&tzenv) {
        Err(_err) => {
            trace!(
                "failed to parse {tzenv:?} as POSIX TZ rule \
                 (attempting to treat it as an IANA time zone): {_err}",
            );
            tzenv
                .to_str()
                .ok_or_else(|| {
                    err!(
                        "failed to parse {tzenv:?} as a POSIX TZ transition \
                         string, or as valid UTF-8 \
                         (therefore ignoring TZ environment variable)",
                    )
                })?
                .to_string()
        }
        Ok(PosixTz::Implementation(string)) => string.to_string(),
        Ok(PosixTz::Rule(tz)) => match tz.reasonable() {
            Ok(reasonable_posix_tz) => {
                return Ok(Some(TimeZone::from_reasonable_posix_tz(
                    reasonable_posix_tz,
                )));
            }
            Err(_) => {
                warn!(
                    "parsed {tzenv:?} as POSIX TZ transition string, \
                     but Jiff considers it unreasonable since \
                     it specifies DST but without a rule \
                     (therefore ignoring TZ environment variable)",
                );
                return Ok(None);
            }
        },
    };
    // At this point, TZ is set to something that is definitively not a
    // POSIX TZ transition string. Some possible values at this point are:
    //
    //   TZ=America/New_York
    //   TZ=:America/New_York
    //   TZ=/usr/share/zoneinfo/America/New_York
    //   TZ=:/usr/share/zoneinfo/America/New_York
    //   TZ=../zoneinfo/America/New_York
    //   TZ=:../zoneinfo/America/New_York
    //
    // `zoneinfo` is the common thread here. So we look for that first. If we
    // can't find it, then we assume the entire string is a time zone name
    // that we can look up in the system zoneinfo database.
    let needle = "zoneinfo/";
    let Some(rpos) = tz_name_or_path.rfind(needle) else {
        // No zoneinfo means this is probably a IANA Time Zone name. But...
        // it could just be a file path.
        trace!(
            "could not find {needle:?} in TZ={tz_name_or_path:?}, \
             therefore attempting lookup in {db:?}",
        );
        return match db.get(&tz_name_or_path) {
            Ok(tz) => Ok(Some(tz)),
            Err(_err) => {
                trace!(
                    "using TZ={tz_name_or_path:?} as time zone name failed, \
                     could not find time zone in zoneinfo database {db:?} \
                     (continuing to try and use {tz_name_or_path:?}",
                );
                Ok(sys::read(db, &tz_name_or_path))
            }
        };
    };
    // We now try to be a little cute here and extract the IANA time zone name
    // from what we now believe is a file path by taking everything after
    // `zoneinfo/`. Once we have that, we try to look it up in our tzdb.
    let name = &tz_name_or_path[rpos + needle.len()..];
    trace!(
        "extracted {name:?} from TZ={tz_name_or_path:?} \
         and assuming it is an IANA time zone name",
    );
    match db.get(&name) {
        Ok(tz) => return Ok(Some(tz)),
        Err(_err) => {
            trace!(
                "using {name:?} from TZ={tz_name_or_path:?}, \
                 could not find time zone in zoneinfo database {db:?} \
                 (continuing to try and use {tz_name_or_path:?})",
            );
        }
    }
    // At this point, we have tried our hardest but we just cannot seem to
    // extract an IANA time zone name out of the `TZ` environment variable.
    // The only thing left for us to do is treat the value as a file path
    // and read the data as TZif. This will give us time zone data if it works,
    // but without a name.
    Ok(sys::read(db, &tz_name_or_path))
}

/// Returns the given file path as TZif data without a time zone name.
///
/// Normally we require TZif time zones to have a name associated with it.
/// But because there are likely platforms that hardlink /etc/localtime and
/// perhaps have no other way to get a time zone name, we choose to support
/// that use case. Although I cannot actually name such a platform...
fn read_unnamed_tzif_file(path: &str) -> Result<TimeZone, Error> {
    let data = std::fs::read(path)
        .map_err(Error::io)
        .with_context(|| err!("failed to read {path:?} as TZif file"))?;
    let tz = TimeZone::tzif_system(&data)
        .with_context(|| err!("found invalid TZif data at {path:?}"))?;
    Ok(tz)
}
