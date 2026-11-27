/*!
Provides a `Display` impl wrapper for POSIX time zones.

This lives here because none of the formatting machinery is available
in `jiff-core`. And while we could still write out a `Display` impl in
`jiff-core`, it doesn't seem worth that duplication.
*/

use jcore::tz::posix;

use crate::fmt;

/// A wrapper around jcore's posix time zone implementation.
///
/// This wrapper provides `Display`, `Debug` and `Format` impls using the
/// formatter in this crate. (jcore does not do formatting.)
pub(crate) struct TimeZoneFormatter<'a>(pub(crate) &'a posix::TimeZone);

impl<'a> core::fmt::Display for TimeZoneFormatter<'a> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        fmt::temporal::DateTimePrinter::new()
            .print_posix_time_zone(self.0, fmt::StdFmtWrite(f))
            .map_err(|_| core::fmt::Error)
    }
}

impl<'a> core::fmt::Debug for TimeZoneFormatter<'a> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Display::fmt(self, f)
    }
}

#[cfg(feature = "defmt")]
impl<'a> defmt::Format for TimeZoneFormatter<'a> {
    fn format(&self, f: defmt::Formatter) {
        defmt::unwrap!(fmt::temporal::DateTimePrinter::new()
            .print_posix_time_zone(self.0, fmt::DefmtWrite(f)))
    }
}

// The tests below require parsing which requires alloc.
#[cfg(feature = "alloc")]
#[cfg(test)]
mod tests {
    use super::*;

    /// Assert that converting the TZ back to a string matches what we got. In
    /// the original version of the POSIX TZ parser, we were very meticulous
    /// about capturing the exact AST of the time zone. But I've since
    /// simplified the data structure considerably such that it is lossy in
    /// terms of what was actually parsed (but of course, not lossy in terms of
    /// the semantic meaning of the time zone).
    ///
    /// So to account for this, we serialize to a string and then parse it
    /// back. We should get what we started with.
    #[cfg(feature = "alloc")]
    #[test]
    fn roundtrips() {
        fn assert_roundtrip(tz: &str) {
            use alloc::string::{String, ToString};

            use jcore::tz::posix;

            fn to_string(tz: &posix::TimeZone) -> String {
                TimeZoneFormatter(tz).to_string()
            }

            let tz = posix::TimeZone::parse(tz).unwrap();
            let reparsed = posix::TimeZone::parse(to_string(&tz)).unwrap();
            assert_eq!(tz, reparsed);
            assert_eq!(to_string(&tz), to_string(&reparsed));
        }

        let cases = [
            "WART4WARST,J1/-3,J365/20",
            "WART4WARST,J1/-4,J365/21",
            "EST5EDT,M3.2.0,M11.1.0",
            "XXX-2<+01>-1,0/0,J365/23",
            "EST24EDT,J1,J365",
            "EST-24EDT,J1,J365",
            "EST5EDT,J1,J365/5:12:34",
            "EST+5EDT,M3.2.0/2,M11.1.0/2",
            "EST+5EDT,M1.1.1,M12.5.2",
            "EST5EDT,0/0,J365/25",
            "XXX3EDT4,0/0,J365/23",
            "XXX3EDT4,0/0,365",
            "XXX3EDT4,J1/-167:59:59,J365/167:59:59",
            "EST5EDT,J1,J365/5:12:34",
            "EST+5EDT,M3.2.0/2,M11.1.0/2",
            "EST+5EDT,J60,J365",
            "EST+5EDT,59,365",
            "EST+5EDT,M1.1.1,M12.5.2",
            "EST5EDT,J1,J365/5:12:34",
            "EST5EDT,J1/23:59:59,J365/24:00:00",
            "EST5EDT,J1/-1,J365/167:00:00",
        ];
        for tz in cases {
            assert_roundtrip(tz);
        }
    }
}
