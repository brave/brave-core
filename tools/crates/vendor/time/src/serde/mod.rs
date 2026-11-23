//! Differential formats for serde.
// This also includes the serde implementations for all types. This doesn't need to be externally
// documented, though.

// Types with guaranteed stable serde representations. Strings are avoided to allow for optimal
// representations in various binary forms.

/// Consume the next item in a sequence.
macro_rules! item {
    ($seq:expr, $name:literal) => {
        $seq.next_element()?
            .ok_or_else(|| <A::Error as serde_core::de::Error>::custom(concat!("expected ", $name)))
    };
}

#[cfg(any(feature = "formatting", feature = "parsing"))]
pub mod iso8601;
#[cfg(any(feature = "formatting", feature = "parsing"))]
pub mod rfc2822;
#[cfg(any(feature = "formatting", feature = "parsing"))]
pub mod rfc3339;
pub mod timestamp;
mod visitor;

#[cfg(feature = "serde-human-readable")]
use alloc::string::ToString;
use core::marker::PhantomData;

#[cfg(feature = "serde-human-readable")]
use serde_core::ser::Error as _;
use serde_core::{Deserialize, Deserializer, Serialize, Serializer};
/// Generate a custom serializer and deserializer from a format string or an existing format.
///
/// The format description syntax accepted by this macro is the same as
/// [`format_description::parse()`], which can be found in [the
/// book][format-description-syntax].
///
/// [format-description-syntax]: https://time-rs.github.io/book/api/format-description.html
///
/// # Syntax
///
/// **It is recommended to use version 3.** For backwards compatibility, this macro defaults to
/// version 1. To use version 2 or version 3, you can specify the version as the first
/// argument: `serde::format_description!(version = 2, mod_name, Date, FORMAT)`.
///
/// For versions 1 and 2, this macro is invoked as `serde::format_description!(mod_name, Date,
/// FORMAT)` where `FORMAT` is either a `"<format string>"` or something that implements
#[cfg_attr(
    all(feature = "formatting", feature = "parsing"),
    doc = "[`Formattable`](crate::formatting::Formattable) and \
           [`Parsable`](crate::parsing::Parsable)."
)]
#[cfg_attr(
    all(feature = "formatting", not(feature = "parsing")),
    doc = "[`Formattable`](crate::formatting::Formattable)."
)]
#[cfg_attr(
    all(not(feature = "formatting"), feature = "parsing"),
    doc = "[`Parsable`](crate::parsing::Parsable)."
)]
/// This puts a module named `mod_name` in the current scope that can be used to format `Date`
/// structs. A submodule (`mod_name::option`) is also generated for `Option<Date>`. Both
/// modules are only visible in the current scope by default. To increase visibility, you can
/// specify `pub`, `pub(crate)`, or similar before the module name:
/// `serde::format_description!(pub mod_name, Date, FORMAT)`.
///
/// For version 3, this macro is invoked as `serde::format_description!(mod mod_name [Date] =
/// FORMAT)`. As with versions 1 and 2, visibility can be specified before the `mod` keyword.
/// The type being formatted and/or parsed must be in scope, as the macro will not import it
/// for you. Note: the `mod` keyword indicates that this is version 3 of the macro; specifying
/// `version = 3` is accepted but unnecessary.
///
/// # Semantics
///
/// The returned `Option` will contain a deserialized value if present and `None` if the field
/// is present but the value is `null` (or the equivalent in other formats). To return `None`
/// when the field is not present, you should use `#[serde(default)]` on the field.
///
/// Note: Due to [serde-rs/serde#2878](https://github.com/serde-rs/serde#2878), you will need to
/// apply `#[serde(default)]` if you want a missing field to deserialize as `None`.
///
/// # Examples
///
/// Using a format string:
///
/// ```rust,no_run
/// # use time::OffsetDateTime;
#[cfg_attr(
    all(feature = "formatting", feature = "parsing"),
    doc = "use ::serde::{Serialize, Deserialize};"
)]
#[cfg_attr(
    all(feature = "formatting", not(feature = "parsing")),
    doc = "use ::serde::Serialize;"
)]
#[cfg_attr(
    all(not(feature = "formatting"), feature = "parsing"),
    doc = "use ::serde::Deserialize;"
)]
/// use time::serde;
///
/// // Makes a module `mod my_format { ... }`.
/// serde::format_description!(my_format, OffsetDateTime, "hour=[hour], minute=[minute]");
///
/// # #[allow(dead_code)]
#[cfg_attr(
    all(feature = "formatting", feature = "parsing"),
    doc = "#[derive(Serialize, Deserialize)]"
)]
#[cfg_attr(
    all(feature = "formatting", not(feature = "parsing")),
    doc = "#[derive(Serialize)]"
)]
#[cfg_attr(
    all(not(feature = "formatting"), feature = "parsing"),
    doc = "#[derive(Deserialize)]"
)]
/// struct SerializesWithCustom {
///     #[serde(with = "my_format")]
///     dt: OffsetDateTime,
///     #[serde(with = "my_format::option", default)]
///     maybe_dt: Option<OffsetDateTime>,
/// }
/// ```
/// 
/// Define the format separately to be used in multiple places:
/// ```rust,no_run
/// # use time::OffsetDateTime;
#[cfg_attr(
    all(feature = "formatting", feature = "parsing"),
    doc = "use ::serde::{Serialize, Deserialize};"
)]
#[cfg_attr(
    all(feature = "formatting", not(feature = "parsing")),
    doc = "use ::serde::Serialize;"
)]
#[cfg_attr(
    all(not(feature = "formatting"), feature = "parsing"),
    doc = "use ::serde::Deserialize;"
)]
/// use time::serde;
/// use time::format_description::StaticFormatDescription;
///
/// const DATE_TIME_FORMAT: StaticFormatDescription = time::macros::format_description!(
///     "hour=[hour], minute=[minute]"
/// );
///
/// // Makes a module `mod my_format { ... }`.
/// serde::format_description!(my_format, OffsetDateTime, DATE_TIME_FORMAT);
///
/// # #[allow(dead_code)]
#[cfg_attr(
    all(feature = "formatting", feature = "parsing"),
    doc = "#[derive(Serialize, Deserialize)]"
)]
#[cfg_attr(
    all(feature = "formatting", not(feature = "parsing")),
    doc = "#[derive(Serialize)]"
)]
#[cfg_attr(
    all(not(feature = "formatting"), feature = "parsing"),
    doc = "#[derive(Deserialize)]"
)]
/// struct SerializesWithCustom {
///     #[serde(with = "my_format")]
///     dt: OffsetDateTime,
///     #[serde(with = "my_format::option", default)]
///     maybe_dt: Option<OffsetDateTime>,
/// }
///
/// fn main() {
///     # #[expect(unused_variables)]
///     let str_ts = OffsetDateTime::now_utc().format(DATE_TIME_FORMAT).unwrap();
/// }
/// ```
/// 
/// Customize the configuration of ISO 8601 formatting/parsing:
/// ```rust,no_run
/// # use time::OffsetDateTime;
#[cfg_attr(
    all(feature = "formatting", feature = "parsing"),
    doc = "use ::serde::{Serialize, Deserialize};"
)]
#[cfg_attr(
    all(feature = "formatting", not(feature = "parsing")),
    doc = "use ::serde::Serialize;"
)]
#[cfg_attr(
    all(not(feature = "formatting"), feature = "parsing"),
    doc = "use ::serde::Deserialize;"
)]
/// use time::serde;
/// use time::format_description::well_known::{iso8601, Iso8601};
///
/// # #[allow(dead_code)]
/// const CONFIG: iso8601::EncodedConfig = iso8601::Config::DEFAULT
///     .set_year_is_six_digits(false)
///     .encode();
/// # #[allow(dead_code)]
/// const FORMAT: Iso8601<CONFIG> = Iso8601::<CONFIG>;
///
/// // Makes a module `mod my_format { ... }`.
/// serde::format_description!(my_format, OffsetDateTime, FORMAT);
///
/// # #[allow(dead_code)]
#[cfg_attr(
    all(feature = "formatting", feature = "parsing"),
    doc = "#[derive(Serialize, Deserialize)]"
)]
#[cfg_attr(
    all(feature = "formatting", not(feature = "parsing")),
    doc = "#[derive(Serialize)]"
)]
#[cfg_attr(
    all(not(feature = "formatting"), feature = "parsing"),
    doc = "#[derive(Deserialize)]"
)]
/// struct SerializesWithCustom {
///     #[serde(with = "my_format")]
///     dt: OffsetDateTime,
///     #[serde(with = "my_format::option", default)]
///     maybe_dt: Option<OffsetDateTime>,
/// }
/// # fn main() {}
/// ```
/// 
/// [`format_description::parse()`]: crate::format_description::parse()
#[cfg(all(feature = "macros", any(feature = "formatting", feature = "parsing")))]
pub use time_macros::serde_format_description as format_description;

use self::visitor::Visitor;
#[cfg(feature = "parsing")]
use crate::format_description::__private::FormatDescriptionV3Inner;
#[cfg(feature = "parsing")]
use crate::format_description::{FormatDescriptionV3, modifier};
use crate::{
    Date, Month, OffsetDateTime, PlainDateTime, SignedDuration, Time, Timestamp, UtcDateTime,
    UtcOffset, Weekday,
};

/// The format used when serializing and deserializing a human-readable `Date`.
#[cfg(feature = "parsing")]
const DATE_FORMAT: FormatDescriptionV3<'_> = FormatDescriptionV3Inner::BorrowedCompound(&[
    #[cfg(feature = "large-dates")]
    FormatDescriptionV3Inner::CalendarYearFullExtendedRange(
        modifier::CalendarYearFullExtendedRange::default(),
    ),
    #[cfg(not(feature = "large-dates"))]
    FormatDescriptionV3Inner::CalendarYearFullStandardRange(
        modifier::CalendarYearFullStandardRange::default(),
    ),
    FormatDescriptionV3Inner::BorrowedLiteral("-"),
    FormatDescriptionV3Inner::MonthNumerical(modifier::MonthNumerical::default()),
    FormatDescriptionV3Inner::BorrowedLiteral("-"),
    FormatDescriptionV3Inner::Day(modifier::Day::default()),
])
.into_opaque();

impl Serialize for Date {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[cfg(feature = "serde-human-readable")]
        if serializer.is_human_readable() {
            let Ok(s) = self.format(&DATE_FORMAT) else {
                return Err(S::Error::custom("failed formatting `Date`"));
            };
            return serializer.serialize_str(&s);
        }

        (self.year(), self.ordinal()).serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for Date {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        if cfg!(feature = "serde-human-readable") && deserializer.is_human_readable() {
            deserializer.deserialize_any(Visitor::<Self>(PhantomData))
        } else {
            deserializer.deserialize_tuple(2, Visitor::<Self>(PhantomData))
        }
    }
}

impl Serialize for SignedDuration {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[cfg(feature = "serde-human-readable")]
        if serializer.is_human_readable() {
            return serializer.collect_str(&format_args!(
                "{}{}.{:>09}",
                if self.is_negative() { "-" } else { "" },
                self.whole_seconds().unsigned_abs(),
                self.subsec_nanoseconds().abs(),
            ));
        }

        (self.whole_seconds(), self.subsec_nanoseconds()).serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for SignedDuration {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        if cfg!(feature = "serde-human-readable") && deserializer.is_human_readable() {
            deserializer.deserialize_any(Visitor::<Self>(PhantomData))
        } else {
            deserializer.deserialize_tuple(2, Visitor::<Self>(PhantomData))
        }
    }
}

/// The format used when serializing and deserializing a human-readable `OffsetDateTime`.
#[cfg(feature = "parsing")]
const OFFSET_DATE_TIME_FORMAT: FormatDescriptionV3<'_> =
    FormatDescriptionV3Inner::BorrowedCompound(&[
        #[cfg(feature = "large-dates")]
        FormatDescriptionV3Inner::CalendarYearFullExtendedRange(
            modifier::CalendarYearFullExtendedRange::default(),
        ),
        #[cfg(not(feature = "large-dates"))]
        FormatDescriptionV3Inner::CalendarYearFullStandardRange(
            modifier::CalendarYearFullStandardRange::default(),
        ),
        FormatDescriptionV3Inner::BorrowedLiteral("-"),
        FormatDescriptionV3Inner::MonthNumerical(modifier::MonthNumerical::default()),
        FormatDescriptionV3Inner::BorrowedLiteral("-"),
        FormatDescriptionV3Inner::Day(modifier::Day::default()),
        FormatDescriptionV3Inner::BorrowedLiteral(" "),
        FormatDescriptionV3Inner::Hour24(modifier::Hour24::default()),
        FormatDescriptionV3Inner::BorrowedLiteral(":"),
        FormatDescriptionV3Inner::Minute(modifier::Minute::default()),
        FormatDescriptionV3Inner::BorrowedLiteral(":"),
        FormatDescriptionV3Inner::Second(modifier::Second::default()),
        FormatDescriptionV3Inner::BorrowedLiteral("."),
        FormatDescriptionV3Inner::Subsecond(modifier::Subsecond::default()),
        FormatDescriptionV3Inner::BorrowedLiteral(" "),
        FormatDescriptionV3Inner::OffsetHour(
            modifier::OffsetHour::default().with_sign_is_mandatory(true),
        ),
        FormatDescriptionV3Inner::BorrowedLiteral(":"),
        FormatDescriptionV3Inner::OffsetMinute(modifier::OffsetMinute::default()),
        FormatDescriptionV3Inner::BorrowedLiteral(":"),
        FormatDescriptionV3Inner::OffsetSecond(modifier::OffsetSecond::default()),
    ])
    .into_opaque();

impl Serialize for OffsetDateTime {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[cfg(feature = "serde-human-readable")]
        if serializer.is_human_readable() {
            let Ok(s) = self.format(&OFFSET_DATE_TIME_FORMAT) else {
                return Err(S::Error::custom("failed formatting `OffsetDateTime`"));
            };
            return serializer.serialize_str(&s);
        }

        (
            self.year(),
            self.ordinal(),
            self.hour(),
            self.minute(),
            self.second(),
            self.nanosecond(),
            self.offset().whole_hours(),
            self.offset().minutes_past_hour(),
            self.offset().seconds_past_minute(),
        )
            .serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for OffsetDateTime {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        if cfg!(feature = "serde-human-readable") && deserializer.is_human_readable() {
            deserializer.deserialize_any(Visitor::<Self>(PhantomData))
        } else {
            deserializer.deserialize_tuple(9, Visitor::<Self>(PhantomData))
        }
    }
}

/// The format used when serializing and deserializing a human-readable `PlainDateTime`.
#[cfg(feature = "parsing")]
const PLAIN_DATE_TIME_FORMAT: FormatDescriptionV3<'_> =
    FormatDescriptionV3Inner::BorrowedCompound(&[
        #[cfg(feature = "large-dates")]
        FormatDescriptionV3Inner::CalendarYearFullExtendedRange(
            modifier::CalendarYearFullExtendedRange::default(),
        ),
        #[cfg(not(feature = "large-dates"))]
        FormatDescriptionV3Inner::CalendarYearFullStandardRange(
            modifier::CalendarYearFullStandardRange::default(),
        ),
        FormatDescriptionV3Inner::BorrowedLiteral("-"),
        FormatDescriptionV3Inner::MonthNumerical(modifier::MonthNumerical::default()),
        FormatDescriptionV3Inner::BorrowedLiteral("-"),
        FormatDescriptionV3Inner::Day(modifier::Day::default()),
        FormatDescriptionV3Inner::BorrowedLiteral(" "),
        FormatDescriptionV3Inner::Hour24(modifier::Hour24::default()),
        FormatDescriptionV3Inner::BorrowedLiteral(":"),
        FormatDescriptionV3Inner::Minute(modifier::Minute::default()),
        FormatDescriptionV3Inner::BorrowedLiteral(":"),
        FormatDescriptionV3Inner::Second(modifier::Second::default()),
        FormatDescriptionV3Inner::BorrowedLiteral("."),
        FormatDescriptionV3Inner::Subsecond(modifier::Subsecond::default()),
    ])
    .into_opaque();

impl Serialize for PlainDateTime {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[cfg(feature = "serde-human-readable")]
        if serializer.is_human_readable() {
            let Ok(s) = self.format(&PLAIN_DATE_TIME_FORMAT) else {
                return Err(S::Error::custom("failed formatting `PlainDateTime`"));
            };
            return serializer.serialize_str(&s);
        }

        (
            self.year(),
            self.ordinal(),
            self.hour(),
            self.minute(),
            self.second(),
            self.nanosecond(),
        )
            .serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for PlainDateTime {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        if cfg!(feature = "serde-human-readable") && deserializer.is_human_readable() {
            deserializer.deserialize_any(Visitor::<Self>(PhantomData))
        } else {
            deserializer.deserialize_tuple(6, Visitor::<Self>(PhantomData))
        }
    }
}

/// The format used when serializing and deserializing a human-readable `UtcDateTime`.
#[cfg(feature = "parsing")]
const UTC_DATE_TIME_FORMAT: FormatDescriptionV3<'_> = PLAIN_DATE_TIME_FORMAT;

impl Serialize for UtcDateTime {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[cfg(feature = "serde-human-readable")]
        if serializer.is_human_readable() {
            let Ok(s) = self.format(&PLAIN_DATE_TIME_FORMAT) else {
                return Err(S::Error::custom("failed formatting `UtcDateTime`"));
            };
            return serializer.serialize_str(&s);
        }

        (
            self.year(),
            self.ordinal(),
            self.hour(),
            self.minute(),
            self.second(),
            self.nanosecond(),
        )
            .serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for UtcDateTime {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        if cfg!(feature = "serde-human-readable") && deserializer.is_human_readable() {
            deserializer.deserialize_any(Visitor::<Self>(PhantomData))
        } else {
            deserializer.deserialize_tuple(6, Visitor::<Self>(PhantomData))
        }
    }
}

/// The format used when serializing and deserializing a human-readable `Time`.
#[cfg(feature = "parsing")]
const TIME_FORMAT: FormatDescriptionV3<'_> = FormatDescriptionV3Inner::BorrowedCompound(&[
    FormatDescriptionV3Inner::Hour24(modifier::Hour24::default()),
    FormatDescriptionV3Inner::BorrowedLiteral(":"),
    FormatDescriptionV3Inner::Minute(modifier::Minute::default()),
    FormatDescriptionV3Inner::BorrowedLiteral(":"),
    FormatDescriptionV3Inner::Second(modifier::Second::default()),
    FormatDescriptionV3Inner::BorrowedLiteral("."),
    FormatDescriptionV3Inner::Subsecond(modifier::Subsecond::default()),
])
.into_opaque();

impl Serialize for Time {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[cfg(feature = "serde-human-readable")]
        if serializer.is_human_readable() {
            let Ok(s) = self.format(&TIME_FORMAT) else {
                return Err(S::Error::custom("failed formatting `Time`"));
            };
            return serializer.serialize_str(&s);
        }

        (self.hour(), self.minute(), self.second(), self.nanosecond()).serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for Time {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        if cfg!(feature = "serde-human-readable") && deserializer.is_human_readable() {
            deserializer.deserialize_any(Visitor::<Self>(PhantomData))
        } else {
            deserializer.deserialize_tuple(4, Visitor::<Self>(PhantomData))
        }
    }
}

/// The format used when serializing and deserializing a human-readable `UtcOffset`.
#[cfg(feature = "parsing")]
const UTC_OFFSET_FORMAT: FormatDescriptionV3<'_> = FormatDescriptionV3Inner::BorrowedCompound(&[
    FormatDescriptionV3Inner::OffsetHour(
        modifier::OffsetHour::default().with_sign_is_mandatory(true),
    ),
    FormatDescriptionV3Inner::BorrowedLiteral(":"),
    FormatDescriptionV3Inner::OffsetMinute(modifier::OffsetMinute::default()),
    FormatDescriptionV3Inner::BorrowedLiteral(":"),
    FormatDescriptionV3Inner::OffsetSecond(modifier::OffsetSecond::default()),
])
.into_opaque();

impl Serialize for UtcOffset {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[cfg(feature = "serde-human-readable")]
        if serializer.is_human_readable() {
            let Ok(s) = self.format(&UTC_OFFSET_FORMAT) else {
                return Err(S::Error::custom("failed formatting `UtcOffset`"));
            };
            return serializer.serialize_str(&s);
        }

        (
            self.whole_hours(),
            self.minutes_past_hour(),
            self.seconds_past_minute(),
        )
            .serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for UtcOffset {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        if cfg!(feature = "serde-human-readable") && deserializer.is_human_readable() {
            deserializer.deserialize_any(Visitor::<Self>(PhantomData))
        } else {
            deserializer.deserialize_tuple(3, Visitor::<Self>(PhantomData))
        }
    }
}

impl Serialize for Timestamp {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        self.as_nanoseconds().serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for Timestamp {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        deserializer.deserialize_i128(Visitor::<Self>(PhantomData))
    }
}

impl Serialize for Weekday {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[cfg(feature = "serde-human-readable")]
        if serializer.is_human_readable() {
            #[cfg(not(feature = "std"))]
            use alloc::string::ToString;
            return self.to_string().serialize(serializer);
        }

        self.number_from_monday().serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for Weekday {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        if cfg!(feature = "serde-human-readable") && deserializer.is_human_readable() {
            deserializer.deserialize_any(Visitor::<Self>(PhantomData))
        } else {
            deserializer.deserialize_u8(Visitor::<Self>(PhantomData))
        }
    }
}

impl Serialize for Month {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[cfg(feature = "serde-human-readable")]
        if serializer.is_human_readable() {
            #[cfg(not(feature = "std"))]
            use alloc::string::String;
            return self.to_string().serialize(serializer);
        }

        u8::from(*self).serialize(serializer)
    }
}

impl<'a> Deserialize<'a> for Month {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'a>,
    {
        if cfg!(feature = "serde-human-readable") && deserializer.is_human_readable() {
            deserializer.deserialize_any(Visitor::<Self>(PhantomData))
        } else {
            deserializer.deserialize_u8(Visitor::<Self>(PhantomData))
        }
    }
}
