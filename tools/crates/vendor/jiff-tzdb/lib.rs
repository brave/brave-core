/*!
A crate that embeds data from the [IANA Time Zone Database].

This crate is meant to be a "raw data" library. That is, it primarily exposes
one routine that permits looking up the raw [TZif] data given a time zone name.
The data returned is embedded into the compiled library. In order to actually
use the data, you'll need a TZif parser, such as the one found in [Jiff] via
`TimeZone::tzif`.

This crate also exposes another routine, [`available`], for iterating over the
names of all time zones embedded into this crate.

# Should I use this crate?

In general, no. It's first and foremost an implementation detail of Jiff, but
if you 1) need raw access to the TZif data and 2) need to bundle it in your
binary, then it's plausible that using this crate is appropriate.

With that said, the _preferred_ way to read TZif data is from your system's
copy of the Time Zone Database. On macOS and most Linux installations, a copy
of this data can be found at `/usr/share/zoneinfo`. Indeed, Jiff will use this
system copy whenever possible, and not use this crate at all. The system copy
is preferred because the Time Zone Database is occasionally updated (perhaps a
few times per year), and it is usually better to rely on your system updates
for such things than some random Rust library.

However, some popular environments, like Windows, do not have a standard
system copy of the Time Zone Database. In those circumstances, Jiff will depend
on this crate and bundle the time zone data into the binary. This is not an
ideal solution, but it makes Most Things Just Work Most of the Time on all
major platforms.

[IANA Time Zone Database]: https://www.iana.org/time-zones
[TZif]: https://datatracker.ietf.org/doc/html/rfc8536
[Jiff]: https://docs.rs/jiff
*/

#![no_std]

mod tzname;

static TZIF_DATA: &[u8] = include_bytes!("concatenated-zoneinfo.dat");

/// The version of the IANA Time Zone Database that was bundled.
///
/// If this bundled database was generated from a pre-existing system copy
/// of the Time Zone Database, then it's possible no version information was
/// available.
pub static VERSION: Option<&str> = tzname::VERSION;

/// Returns the binary TZif data for the time zone name given.
///
/// This also returns the canonical name for the time zone. Namely, since this
/// lookup is performed without regard to ASCII case, the given name may not be
/// the canonical capitalization of the time zone.
///
/// If no matching time zone data exists, then `None` is returned.
///
/// In order to use the data returned, it must be fed to a TZif parser. For
/// example, if you're using [`jiff`](https://docs.rs/jiff), then this would
/// be the `TimeZone::tzif` constructor.
///
/// # Example
///
/// Some basic examples of time zones that exist:
///
/// ```
/// assert!(jiff_tzdb::get("America/New_York").is_some());
/// assert!(jiff_tzdb::get("america/new_york").is_some());
/// assert!(jiff_tzdb::get("America/NewYork").is_none());
/// ```
///
/// And an example of how the canonical name might differ from the name given:
///
/// ```
/// let (canonical_name, data) = jiff_tzdb::get("america/new_york").unwrap();
/// assert_eq!(canonical_name, "America/New_York");
/// // All TZif data starts with the `TZif` header.
/// assert_eq!(&data[..4], b"TZif");
/// ```
pub fn get(name: &str) -> Option<(&'static str, &'static [u8])> {
    let index = index(name)?;
    let (canonical_name, ref range) = tzname::TZNAME_TO_OFFSET[index];
    Some((canonical_name, &TZIF_DATA[range.clone()]))
}

/// Returns a list of all available time zone names bundled into this crate.
///
/// There are no API guarantees on the order of the sequence returned.
///
/// # Example
///
/// This example shows how to determine the total number of time zone names
/// available:
///
/// ```
/// assert_eq!(jiff_tzdb::available().count(), 597);
/// ```
///
/// Note that this number may change in subsequent releases of the Time Zone
/// Database.
pub fn available() -> TimeZoneNameIter {
    TimeZoneNameIter { it: tzname::TZNAME_TO_OFFSET.iter() }
}

/// An iterator over all time zone names embedded into this crate.
///
/// There are no API guarantees on the order of this iterator.
///
/// This iterator is created by the [`available`] function.
#[derive(Clone, Debug)]
pub struct TimeZoneNameIter {
    it: core::slice::Iter<'static, (&'static str, core::ops::Range<usize>)>,
}

impl Iterator for TimeZoneNameIter {
    type Item = &'static str;

    fn next(&mut self) -> Option<&'static str> {
        self.it.next().map(|&(name, _)| name)
    }
}

/// Finds the index of a matching entry in `TZNAME_TO_OFFSET`.
///
/// If the given time zone doesn't exist, then `None` is returned.
fn index(query_name: &str) -> Option<usize> {
    tzname::TZNAME_TO_OFFSET
        .binary_search_by(|(name, _)| cmp_ignore_ascii_case(name, query_name))
        .ok()
}

/// Like std's `eq_ignore_ascii_case`, but returns a full `Ordering`.
fn cmp_ignore_ascii_case(s1: &str, s2: &str) -> core::cmp::Ordering {
    let it1 = s1.as_bytes().iter().map(|&b| b.to_ascii_lowercase());
    let it2 = s2.as_bytes().iter().map(|&b| b.to_ascii_lowercase());
    it1.cmp(it2)
}

#[cfg(test)]
mod tests {
    use core::cmp::Ordering;

    use crate::tzname::TZNAME_TO_OFFSET;

    use super::*;

    /// This is a regression test where TZ names were sorted lexicographically
    /// but case sensitively, and this could subtly break binary search.
    #[test]
    fn sorted_ascii_case_insensitive() {
        for window in TZNAME_TO_OFFSET.windows(2) {
            let (name1, _) = window[0];
            let (name2, _) = window[1];
            assert_eq!(
                Ordering::Less,
                cmp_ignore_ascii_case(name1, name2),
                "{name1} should be less than {name2}",
            );
        }
    }
}
