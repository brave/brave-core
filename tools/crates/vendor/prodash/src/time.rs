#[cfg(feature = "local-time")]
mod localtime {
    use std::time::SystemTime;

    use jiff::Zoned;

    /// Return a string representing the current date and time as localtime.
    ///
    /// Available with the `localtime` feature toggle.
    pub fn format_now_datetime_seconds() -> String {
        Zoned::now().strftime("%F %T %Z").to_string()
    }

    /// Return a string representing the current time as localtime.
    ///
    /// Available with the `localtime` feature toggle.
    pub fn format_time_for_messages(time: SystemTime) -> String {
        Zoned::try_from(time)
            .expect("system time is always in range -9999-01-01..=9999-12-31")
            .strftime("%T")
            .to_string()
    }
}

/// An `hours:minute:seconds` format.
pub const DATE_TIME_HMS: usize = "00:51:45".len();

#[cfg(not(feature = "local-time"))]
mod utc {
    use std::time::SystemTime;

    use super::DATE_TIME_HMS;

    /// Return a string representing the current date and time as UTC.
    ///
    /// Available without the `localtime` feature toggle.
    pub fn format_time_for_messages(time: SystemTime) -> String {
        let time = jiff::Timestamp::try_from(time).expect("reasonable system time");
        time.strftime("%T").to_string()
    }

    /// Return a string representing the current time as UTC.
    ///
    /// Available without the `localtime` feature toggle.
    pub fn format_now_datetime_seconds() -> String {
        jiff::Timestamp::now().strftime("%FT%T").to_string()
    }
}

#[cfg(feature = "local-time")]
pub use localtime::*;
#[cfg(not(feature = "local-time"))]
pub use utc::*;
