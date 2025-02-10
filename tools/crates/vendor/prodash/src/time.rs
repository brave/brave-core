#[cfg(feature = "local-time")]
mod localtime {
    use std::time::SystemTime;

    /// Return a string representing the current date and time as localtime.
    ///
    /// Available with the `localtime` feature toggle.
    pub fn format_now_datetime_seconds() -> String {
        let t = time::OffsetDateTime::now_utc();
        t.to_offset(time::UtcOffset::local_offset_at(t).unwrap_or(time::UtcOffset::UTC))
            .format(&time::format_description::parse("%F %T").expect("format known to work"))
            .expect("formatting always works")
    }

    /// Return a string representing the current time as localtime.
    ///
    /// Available with the `localtime` feature toggle.
    pub fn format_time_for_messages(time: SystemTime) -> String {
        time::OffsetDateTime::from(time)
            .to_offset(time::UtcOffset::current_local_offset().unwrap_or(time::UtcOffset::UTC))
            .format(&time::format_description::parse("[hour]:[minute]:[second]").expect("format known to work"))
            .expect("formatting always works")
    }
}

/// An `hours:minute:seconds` format.
pub const DATE_TIME_HMS: usize = "00:51:45".len();

#[cfg(not(feature = "local-time"))]
mod utc {
    use std::time::SystemTime;

    use super::DATE_TIME_HMS;
    const DATE_TIME_YMD: usize = "2020-02-13T".len();

    /// Return a string representing the current date and time as UTC.
    ///
    /// Available without the `localtime` feature toggle.
    pub fn format_time_for_messages(time: SystemTime) -> String {
        String::from_utf8_lossy(
            &humantime::format_rfc3339_seconds(time).to_string().as_bytes()
                [DATE_TIME_YMD..DATE_TIME_YMD + DATE_TIME_HMS],
        )
        .into_owned()
    }

    /// Return a string representing the current time as UTC.
    ///
    /// Available without the `localtime` feature toggle.
    pub fn format_now_datetime_seconds() -> String {
        String::from_utf8_lossy(
            &humantime::format_rfc3339_seconds(std::time::SystemTime::now())
                .to_string()
                .as_bytes()[.."2020-02-13T00:51:45".len()],
        )
        .into_owned()
    }
}

#[cfg(feature = "local-time")]
pub use localtime::*;
#[cfg(not(feature = "local-time"))]
pub use utc::*;
