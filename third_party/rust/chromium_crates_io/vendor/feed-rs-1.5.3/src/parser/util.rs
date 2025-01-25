use std::error::Error;
use std::io::BufRead;
use std::ops::Add;
use std::sync::OnceLock;
use std::time::Duration;

use crate::model;
use chrono::{DateTime, Utc};
use model::{Link, Text};
use regex::{Captures, Regex};
use url::Url;
use uuid::Uuid;

use crate::parser::{ParseFeedResult, Parser};
use crate::xml::Element;

use fixes::PatSub;

/// Set of regular expressions we use to clean up broken dates
mod fixes {
    use super::OnceLock;
    use super::Regex;
    pub struct PatSub(pub Regex, pub &'static str);

    // Feeds may not comply with the specification
    pub fn rfc1123() -> &'static [PatSub] {
        static RFC1123: OnceLock<Vec<PatSub>> = OnceLock::new();
        RFC1123.get_or_init(|| {
            vec![
                // replaces the trailing " Z" with UTC offset
                PatSub(Regex::new(" Z$").unwrap(), " +0000"),
                // drop the week day name
                PatSub(Regex::new("^[[:alpha:]]{3}, ").unwrap(), ""),
            ]
        })
    }

    // Feeds may not comply with the specification in various ways (https://tools.ietf.org/html/rfc2822#page-14)
    pub fn rfc2822() -> &'static [PatSub] {
        static RFC2822: OnceLock<Vec<PatSub>> = OnceLock::new();
        RFC2822.get_or_init(|| {
            vec![
                // RFC 2822 mandates a +/- 4 digit offset, or UT/GMT (obsolete) but feeds have "UTC" or "-0000"
                // Suffixes that are not handled by the parser are trimmed and replaced with the corresponding value timezone.
                PatSub(Regex::new("(UTC|-0000$)").unwrap(), "+0000"),
                // The short weekday can be wrong e.g. "Wed, 25 Aug 2012" was actually a Saturday - https://www.timeanddate.com/calendar/monthly.html?year=2012&month=8
                // or it can be something other than a short weekday name e.g. "Thurs, 13 Jul 2011 07:38:00 GMT"
                // As its extraneous, we just remove it
                PatSub(Regex::new("(Sun|Mon|Tue|Wed|Thu|Fri|Sat)[a-z]*, ").unwrap(), ""),
                // Long month names are not allowed, so replace them with short
                PatSub(Regex::new("(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)[a-z]*").unwrap(), "$1"),
                // Some timestamps have an hours component adjusted by 24h, while not adjusting the day so we just reset to start of day
                #[allow(clippy::trivial_regex)]
                PatSub(Regex::new(" 24:").unwrap(), " 00:"),
                // Single digit hours are padded
                PatSub(Regex::new(" ([0-9]):").unwrap(), " 0${1}:"),
            ]
        })
    }

    // Feeds may not comply with the specification (https://tools.ietf.org/html/rfc3339)
    pub fn rfc3339() -> &'static [PatSub] {
        static RFC3339: OnceLock<Vec<PatSub>> = OnceLock::new();
        RFC3339.get_or_init(|| {
            vec![
                // inserts missing colon in timezone
                PatSub(Regex::new(r"(\+|-)(\d{2})(\d{2})").unwrap(), "${1}${2}:${3}"),
                // appends time (midnight) and timezone (utc) if missing
                PatSub(Regex::new(r"-\d{2}$").unwrap(), "${0}T00:00:00+00:00"),
            ]
        })
    }
}

// RFC-1123 format e.g. Tue, 15 Nov 2022 20:15:04 Z
// but without the day of week (since it is superfluous and often in languages other than English)
static RFC1123_FORMAT_STR: &str = "%d %b %Y %H:%M:%S %z";

/// Pluggable timestamp parser
pub(crate) type TimestampParser = dyn Fn(&str) -> Option<DateTime<Utc>> + 'static;

/// Pluggable ID (feed or entry) generator
pub(crate) type IdGenerator = dyn Fn(&[Link], &Option<Text>, Option<&str>) -> String;

/// Handles <content:encoded>
pub(crate) fn handle_encoded<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<Text>> {
    Ok(element.children_as_string()?.map(Text::html))
}

// Handles "xml:lang" as an attribute (e.g. in Atom feeds)
pub(crate) fn handle_language_attr<R: BufRead>(element: &Element<R>) -> Option<String> {
    element.attr_value("xml:lang")
}

// Handles "xml:base" as an attribute (e.g. in Atom feeds)
pub(crate) fn handle_base_attr<R: BufRead>(element: &Element<R>) -> Option<String> {
    element.attr_value("xml:base")
}

// Handles <link>
pub(crate) fn handle_link<R: BufRead>(element: Element<R>) -> Option<Link> {
    element.child_as_text().map(|s| Link::new(s, element.xml_base.as_ref()))
}

// Handles <title>, <description> etc
pub(crate) fn handle_text<R: BufRead>(element: Element<R>) -> Option<Text> {
    if let Ok(Some(text)) = element.children_as_string() {
        Some(Text::new(text))
    } else {
        None
    }
}

/// Handles date/time
pub(crate) fn handle_timestamp<R: BufRead>(parser: &Parser, element: Element<R>) -> Option<DateTime<Utc>> {
    if let Some(text) = element.child_as_text() {
        parser.parse_timestamp(&text)
    } else {
        None
    }
}

/// Simplifies the "if let ... = parse ... assign" block
pub(crate) fn if_some_then<T, F: FnOnce(T)>(v: Option<T>, func: F) {
    if let Some(v) = v {
        func(v)
    }
}

/// Simplifies the "if let ... = parse ... assign" block
pub(crate) fn if_ok_then_some<T, F: FnOnce(Option<T>)>(v: Result<T, impl Error>, func: F) {
    if let Ok(v) = v {
        func(Some(v))
    }
}

/// Parses a timestamp using a variety of strategies to try and deal with the remarkable variability and non-standards
/// compliant text on the internet.
pub(crate) fn parse_timestamp_lenient(original: &str) -> Option<DateTime<Utc>> {
    // Curiously, we see RFC-3339 dates in RSS 2 feeds, and it is supposed to be the format for Atom and Json too so we try this first
    try_parse_timestamp_rfc3339_lenient(original)
        // Next is the format for RSS 2, which is used often
        .or_else(|| try_parse_timestamp_rfc2822_lenient(original))
        // And we also have RFC 1123 to complete the set of RFCs
        .or_else(|| try_parse_timestamp_rfc1123_lenient(original))
}

// Parses a URI, potentially resolving relative URIs against the base if provided
pub(crate) fn parse_uri(uri: &str, base: Option<&Url>) -> Option<Url> {
    match Url::parse(uri) {
        // Absolute URIs will parse correctly
        Ok(uri) => Some(uri),

        // If its a relative URL we need to add the base
        Err(url::ParseError::RelativeUrlWithoutBase) => {
            if let Some(base) = base {
                if let Ok(with_base) = base.join(uri) {
                    return Some(with_base);
                }
            }

            None
        }

        // Nothing to do if we have a different error
        _ => None,
    }
}

// Parses a timestamp from a potentially RFC-1123 formatted timestamp (which isn't part of any feed standard, but hey
// its the internet, why follow standards?
fn try_parse_timestamp_rfc1123_lenient(original: &str) -> Option<DateTime<Utc>> {
    let mut cleaned = original.trim().to_string();
    for PatSub(regex, replacement) in fixes::rfc1123() {
        cleaned = regex.replace(&cleaned, *replacement).to_string();
    }

    DateTime::parse_from_str(&cleaned, RFC1123_FORMAT_STR).map(|t| t.with_timezone(&Utc)).ok()
}

// Parses a timestamp from a potentially RFC-2822 formatted timestamp
fn try_parse_timestamp_rfc2822_lenient(original: &str) -> Option<DateTime<Utc>> {
    // Clean the input string by applying each of the regex fixes
    let mut cleaned = original.trim().to_string();
    for PatSub(regex, replacement) in fixes::rfc2822() {
        cleaned = regex.replace(&cleaned, *replacement).to_string();
    }

    DateTime::parse_from_rfc2822(&cleaned).map(|t| t.with_timezone(&Utc)).ok()
}

// Parses a timestamp from a potentially RFC-3339 formatted string
fn try_parse_timestamp_rfc3339_lenient(original: &str) -> Option<DateTime<Utc>> {
    // Clean the input string by applying each of the regex fixes
    let mut cleaned = original.trim().to_string();
    for PatSub(regex, replacement) in fixes::rfc3339() {
        cleaned = regex.replace(&cleaned, *replacement).to_string();
    }

    DateTime::parse_from_rfc3339(cleaned.trim()).map(|t| t.with_timezone(&Utc)).ok()
}

/// Generates a new UUID.
pub(crate) fn uuid_gen() -> String {
    Uuid::new_v4().to_string()
}

/// Parses "normal play time" per the RSS media spec
/// NPT has a second or sub-second resolution. It is specified as H:M:S.h (npt-hhmmss) or S.h (npt-sec), where H=hours, M=minutes, S=second and h=fractions of a second.
pub(crate) fn parse_npt(text: &str) -> Option<Duration> {
    // Try npt-hhmmss format first
    static NPT_HHMMSS: OnceLock<Regex> = OnceLock::new();
    let npt_hhmmss = NPT_HHMMSS.get_or_init(|| {
        // Extract hours (h), minutes (m), seconds (s) and fractional seconds (f)
        Regex::new(r"(?P<h>\d+):(?P<m>\d{2}):(?P<s>\d{2})(\.(?P<f>\d+))?").unwrap()
    });
    if let Some(captures) = npt_hhmmss.captures(text) {
        let h = captures.name("h");
        let m = captures.name("m");
        let s = captures.name("s");

        if let (Some(h), Some(m), Some(s)) = (h, m, s) {
            // Parse the hours, minutes and seconds
            let mut seconds = s.as_str().parse::<u64>().unwrap();
            seconds += m.as_str().parse::<u64>().unwrap() * 60;
            seconds += h.as_str().parse::<u64>().unwrap() * 3600;
            let mut duration = Duration::from_secs(seconds);

            // Add fractional seconds if present
            duration = parse_npt_add_frac_sec(duration, captures);

            return Some(duration);
        }
    }

    // Next try npt-sec
    static NPT_SEC: OnceLock<Regex> = OnceLock::new();
    let npt_sec = NPT_SEC.get_or_init(|| {
        // Extract seconds (s) and fractional seconds (f)
        Regex::new(r"(?P<s>\d+)(\.(?P<f>\d+))?").unwrap()
    });
    if let Some(captures) = npt_sec.captures(text) {
        if let Some(s) = captures.name("s") {
            // Parse the seconds
            let seconds = s.as_str().parse::<u64>().unwrap();
            let mut duration = Duration::from_secs(seconds);

            // Add fractional seconds if present
            duration = parse_npt_add_frac_sec(duration, captures);

            return Some(duration);
        }
    }

    // Just drop it
    None
}

// Adds the fractional seconds if present
fn parse_npt_add_frac_sec(duration: Duration, captures: Captures) -> Duration {
    if let Some(frac) = captures.name("f") {
        let frac = frac.as_str();
        let denom = 10f32.powi(frac.len() as i32);
        let num = frac.parse::<f32>().unwrap();
        let millis = (1000f32 * (num / denom)) as u64;
        duration.add(Duration::from_millis(millis))
    } else {
        duration
    }
}

#[cfg(test)]
mod tests {
    use chrono::{TimeZone, Utc};

    use super::*;

    // Verify we can parse non-spec compliant date strings
    // Regression tests for https://github.com/feed-rs/feed-rs/issues/7
    #[test]
    fn test_timestamp_rss2() {
        let tests = vec![
            //
            ("26 August 2019 10:00:00 +0000", Utc.with_ymd_and_hms(2019, 8, 26, 10, 0, 0).unwrap()),
            // UTC is not a valid timezone in RFC-2822
            ("Mon, 01 Jan 0001 00:00:00 UTC", Utc.with_ymd_and_hms(1, 1, 1, 0, 0, 0).unwrap()),
            // -0000 is not considered a timezone in the parser
            ("Wed, 22 Jan 2020 10:58:02 -0000", Utc.with_ymd_and_hms(2020, 1, 22, 10, 58, 2).unwrap()),
            // The 25th of August 2012 was a Saturday, not a Wednesday
            ("Wed, 25 Aug 2012 03:25:42 GMT", Utc.with_ymd_and_hms(2012, 8, 25, 3, 25, 42).unwrap()),
            // Long month names are not allowed
            ("2 September 2019 20:00:00 +0000", Utc.with_ymd_and_hms(2019, 9, 2, 20, 0, 0).unwrap()),
            // RSS2 should be RFC-2822 but we get Atom/RFC-3339 formats
            ("2016-10-01T00:00:00+10:00", Utc.with_ymd_and_hms(2016, 9, 30, 14, 0, 0).unwrap()),
            // Single digit hours should be padded
            ("24 Sep 2013 1:27 PDT", Utc.with_ymd_and_hms(2013, 9, 24, 8, 27, 0).unwrap()),
            // Consider an invalid hour specification as start-of-day
            ("5 Jun 2017 24:05 PDT", Utc.with_ymd_and_hms(2017, 6, 5, 7, 5, 0).unwrap()),
            // We even see RFC1123
            ("Tue, 15 Nov 2022 20:15:04 Z", Utc.with_ymd_and_hms(2022, 11, 15, 20, 15, 4).unwrap()),
            // And RFC1123 with languages other than English...
            ("mer, 16 nov 2022 00:38:15 +0100", Utc.with_ymd_and_hms(2022, 11, 15, 23, 38, 15).unwrap()),
        ];

        for (source, expected) in tests {
            let parsed = parse_timestamp_lenient(source).unwrap_or_else(|| panic!("failed to parse {}", source));
            assert_eq!(parsed, expected);
        }
    }

    #[test]
    fn test_timestamp_atom() {
        let tests = vec![
            // properly formated rfc3339 string
            ("2014-12-29T14:53:35+02:00", Utc.with_ymd_and_hms(2014, 12, 29, 12, 53, 35).unwrap()),
            // missing colon in timezone
            ("2014-12-29T14:53:35+0200", Utc.with_ymd_and_hms(2014, 12, 29, 12, 53, 35).unwrap()),
        ];

        for (source, expected) in tests {
            let parsed = parse_timestamp_lenient(source).unwrap_or_else(|| panic!("failed to parse {}", source));
            assert_eq!(parsed, expected);
        }
    }

    // Verify we can parse NPT times
    #[test]
    fn test_parse_npt() {
        assert_eq!(parse_npt("12:05:35").unwrap(), Duration::from_secs(12 * 3600 + 5 * 60 + 35));
        assert_eq!(
            parse_npt("12:05:35.123").unwrap(),
            Duration::from_millis(12 * 3600000 + 5 * 60000 + 35 * 1000 + 123)
        );
        assert_eq!(parse_npt("123.45").unwrap(), Duration::from_millis(123450));
    }
}
