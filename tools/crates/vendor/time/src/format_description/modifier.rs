//! Various modifiers for components.

use core::num::NonZero;

/// Generate the provided code if and only if `pub` is present.
macro_rules! if_pub {
    (pub $(#[$attr:meta])*; $($x:tt)*) => {
        $(#[$attr])*
        ///
        /// This function exists since [`Default::default()`] cannot be used in a `const` context.
        /// It may be removed once that becomes possible. As the [`Default`] trait is in the
        /// prelude, removing this function in the future will not cause any resolution failures for
        /// the overwhelming majority of users; only users who use `#![no_implicit_prelude]` will be
        /// affected. As such it will not be considered a breaking change.
        $($x)*
    };
    ($($_:tt)*) => {};
}

/// Implement `Default` for the given type. This also generates an inherent implementation of a
/// `default` method that is `const fn`, permitting the default value to be used in const contexts.
// Every modifier should use this macro rather than a derived `Default`.
macro_rules! impl_const_default {
    ($($(
        #[doc = $doc:expr])*
        $(#[cfg($($cfg:tt)+)])?
        $(#[expect($($expected:tt)+)])?
        $(@$pub:ident)? $type:ty => $default:expr;
    )*) => {$(
        $(#[cfg($($cfg)+)])?
        $(#[expect($($expected)+)])?
        impl $type {
            if_pub! {
                $($pub)?
                $(#[doc = $doc])*;
                #[inline]
                pub const fn default() -> Self {
                    $default
                }
            }
        }

        $(#[doc = $doc])*
        $(#[cfg($($cfg)+)])?
        $(#[expect($($expected)+)])?
        impl Default for $type {
            #[inline]
            fn default() -> Self {
                $default
            }
        }
    )*};
}

// Keep this first so that it's shown at the top of documentation.
impl_const_default! {
    /// Creates a modifier that indicates the value is [padded with zeroes](Padding::Zero).
    @pub Day => Self { padding: Padding::Zero };
    /// Creates a modifier that indicates the value uses the
    /// [`Numerical`](Self::Numerical) representation.
    #[expect(deprecated)]
    MonthRepr => Self::Numerical;
    @pub MonthShort => Self { case_sensitive: true };
    @pub MonthLong => Self { case_sensitive: true };
    @pub MonthNumerical => Self { padding: Padding::Zero };
    /// Creates an instance of this type that indicates the value uses the
    /// [`Numerical`](MonthRepr::Numerical) representation, is [padded with zeroes](Padding::Zero),
    /// and is case-sensitive when parsing.
    #[expect(deprecated)]
    @pub Month => Self {
        padding: Padding::Zero,
        repr: MonthRepr::Numerical,
        case_sensitive: true,
    };
    /// Creates a modifier that indicates the value is [padded with zeroes](Padding::Zero).
    @pub Ordinal => Self { padding: Padding::Zero };
    /// Creates a modifier that indicates the value uses the [`Long`](Self::Long) representation.
    #[expect(deprecated)]
    WeekdayRepr => Self::Long;
    @pub WeekdayShort => Self { case_sensitive: true };
    @pub WeekdayLong => Self { case_sensitive: true };
    @pub WeekdaySunday => Self { one_indexed: true };
    @pub WeekdayMonday => Self { one_indexed: true };
    /// Creates a modifier that indicates the value uses the [`Long`](WeekdayRepr::Long)
    /// representation and is case-sensitive when parsing. If the representation is changed to a
    /// numerical one, the instance defaults to one-based indexing.
    #[expect(deprecated)]
    @pub Weekday => Self {
        repr: WeekdayRepr::Long,
        one_indexed: true,
        case_sensitive: true,
    };
    /// Creates a modifier that indicates that the value uses the [`Iso`](Self::Iso) representation.
    #[expect(deprecated)]
    WeekNumberRepr => Self::Iso;
    @pub WeekNumberIso => Self { padding: Padding::Zero };
    @pub WeekNumberSunday => Self { padding: Padding::Zero };
    @pub WeekNumberMonday => Self { padding: Padding::Zero };
    /// Creates a modifier that indicates that the value is [padded with zeroes](Padding::Zero)
    /// and uses the [`Iso`](WeekNumberRepr::Iso) representation.
    #[expect(deprecated)]
    @pub WeekNumber => Self {
        padding: Padding::Zero,
        repr: WeekNumberRepr::Iso,
    };
    /// Creates a modifier that indicates the value uses the [`Full`](Self::Full) representation.
    #[expect(deprecated)]
    YearRepr => Self::Full;
    /// Creates a modifier that indicates the value uses the [`Extended`](Self::Extended) range.
    ///
    /// Note that this is the default modifier for version 1 and 2 format descriptions. Version 3
    /// format descriptions (which cannot be manually constructed) use `Standard` as the default.
    #[expect(deprecated)]
    YearRange => Self::Extended;
    @pub CalendarYearFullExtendedRange => Self {
        padding: Padding::Zero,
        sign_is_mandatory: false,
    };
    @pub CalendarYearFullStandardRange => Self {
        padding: Padding::Zero,
        sign_is_mandatory: false,
    };
    @pub IsoYearFullExtendedRange => Self {
        padding: Padding::Zero,
        sign_is_mandatory: false,
    };
    @pub IsoYearFullStandardRange => Self {
        padding: Padding::Zero,
        sign_is_mandatory: false,
    };
    @pub CalendarYearCenturyExtendedRange => Self {
        padding: Padding::Zero,
        sign_is_mandatory: false,
    };
    @pub CalendarYearCenturyStandardRange => Self {
        padding: Padding::Zero,
        sign_is_mandatory: false,
    };
    @pub IsoYearCenturyExtendedRange => Self {
        padding: Padding::Zero,
        sign_is_mandatory: false,
    };
    @pub IsoYearCenturyStandardRange => Self {
        padding: Padding::Zero,
        sign_is_mandatory: false,
    };
    @pub CalendarYearLastTwo => Self {
        padding: Padding::Zero,
    };
    @pub IsoYearLastTwo => Self {
        padding: Padding::Zero,
    };
    /// Creates a modifier that indicates the value uses the [`Full`](YearRepr::Full)
    /// representation, is [padded with zeroes](Padding::Zero), uses the Gregorian calendar as its
    /// base, and only includes the year's sign if necessary.
    #[expect(deprecated)]
    @pub Year => Self {
        padding: Padding::Zero,
        repr: YearRepr::Full,
        range: YearRange::Extended,
        iso_week_based: false,
        sign_is_mandatory: false,
    };
    @pub Hour12 => Self { padding: Padding::Zero };
    @pub Hour24 => Self { padding: Padding::Zero };
    /// Creates a modifier that indicates the value is [padded with zeroes](Padding::Zero) and
    /// has the 24-hour representation.
    #[expect(deprecated)]
    @pub Hour => Self {
        padding: Padding::Zero,
        is_12_hour_clock: false,
    };
    /// Creates a modifier that indicates the value is [padded with zeroes](Padding::Zero).
    @pub Minute => Self { padding: Padding::Zero };
    /// Creates a modifier that indicates the value uses the upper-case representation and is
    /// case-sensitive when parsing.
    @pub Period => Self {
        is_uppercase: true,
        case_sensitive: true,
    };
    /// Creates a modifier that indicates the value is [padded with zeroes](Padding::Zero).
    @pub Second => Self { padding: Padding::Zero };
    /// Creates a modifier that indicates the stringified value contains [one or more
    /// digits](Self::OneOrMore).
    SubsecondDigits => Self::OneOrMore;
    /// Creates a modifier that indicates the stringified value contains [one or more
    /// digits](SubsecondDigits::OneOrMore).
    @pub Subsecond => Self { digits: SubsecondDigits::OneOrMore };
    /// Creates a modifier that indicates the value only uses a sign for negative values and is
    /// [padded with zeroes](Padding::Zero).
    @pub OffsetHour => Self {
        sign_is_mandatory: false,
        padding: Padding::Zero,
    };
    /// Creates a modifier that indicates the value is [padded with zeroes](Padding::Zero).
    @pub OffsetMinute => Self { padding: Padding::Zero };
    /// Creates a modifier that indicates the value is [padded with zeroes](Padding::Zero).
    @pub OffsetSecond => Self { padding: Padding::Zero };
    /// Creates a modifier that indicates the value is [padded with zeroes](Self::Zero).
    Padding => Self::Zero;
    /// Creates a modifier that indicates the value represents the [number of seconds](Self::Second)
    /// since the Unix epoch.
    #[expect(deprecated)]
    UnixTimestampPrecision => Self::Second;
    @pub UnixTimestampSecond => Self { sign_is_mandatory: false };
    @pub UnixTimestampMillisecond => Self { sign_is_mandatory: false };
    @pub UnixTimestampMicrosecond => Self { sign_is_mandatory: false };
    @pub UnixTimestampNanosecond => Self { sign_is_mandatory: false };
    /// Creates a modifier that indicates the value represents the [number of
    /// seconds](UnixTimestampPrecision::Second) since the Unix epoch. The sign is not mandatory.
    #[expect(deprecated)]
    @pub UnixTimestamp => Self {
        precision: UnixTimestampPrecision::Second,
        sign_is_mandatory: false,
    };
    /// Indicate that any trailing characters after the end of input are prohibited and will cause
    /// an error when used with `parse`.
    TrailingInput => Self::Prohibit;
    /// Creates a modifier used to represent the end of input, not allowing any trailing input (i.e.
    /// the input must be fully consumed).
    @pub End => Self { trailing_input: TrailingInput::Prohibit };
}

macro_rules! builder_methods {
    ($(
        $(#[$fn_attr:meta])*
        fn $method:ident($field:ident : $field_ty:ty);
    )+) => {$(
        $(#[$fn_attr])*
        #[allow(clippy::needless_update, reason = "needed for types with multiple fields")]
        #[inline]
        #[must_use = "this returns the result of the operation, without modifying the original"]
        pub const fn $method(self, $field: $field_ty) -> Self {
            Self { $field, ..self }
        }
    )+ };
}

/// Day of the month.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Day {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl Day {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// The representation of a month.
#[non_exhaustive]
#[deprecated(
    since = "0.3.48",
    note = "used only in the deprecated `Month` component"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MonthRepr {
    /// The number of the month (January is 1, December is 12).
    Numerical,
    /// The long form of the month name (e.g. "January").
    Long,
    /// The short form of the month name (e.g. "Jan").
    Short,
}

/// Month of the year using the short form of the month name (e.g. "Jan").
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MonthShort {
    /// Is the value case sensitive when parsing?
    pub(crate) case_sensitive: bool,
}

impl MonthShort {
    builder_methods! {
        /// Set whether the value is case sensitive when parsing.
        fn with_case_sensitive(case_sensitive: bool);
    }
}

/// Month of the year using the long form of the month name (e.g. "January").
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MonthLong {
    /// Is the value case sensitive when parsing?
    pub(crate) case_sensitive: bool,
}

impl MonthLong {
    builder_methods! {
        /// Set whether the value is case sensitive when parsing.
        fn with_case_sensitive(case_sensitive: bool);
    }
}

/// Month of the year using a numerical representation (e.g. "1" for January).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MonthNumerical {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
}

impl MonthNumerical {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Month of the year.
#[non_exhaustive]
#[allow(deprecated)]
#[deprecated(
    since = "0.3.48",
    note = "use `MonthShort`, `MonthLong`, or `MonthNumeric` instead"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Month {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
    /// What form of representation should be used?
    pub repr: MonthRepr,
    /// Is the value case sensitive when parsing?
    pub case_sensitive: bool,
}

#[expect(deprecated)]
impl Month {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set the manner in which the month is represented.
        fn with_repr(repr: MonthRepr);
        /// Set whether the value is case sensitive when parsing.
        fn with_case_sensitive(case_sensitive: bool);
    }
}

/// Ordinal day of the year.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Ordinal {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl Ordinal {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// The representation used for the day of the week.
#[non_exhaustive]
#[deprecated(
    since = "0.3.48",
    note = "used only in the deprecated `Weekday` component"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum WeekdayRepr {
    /// The short form of the weekday (e.g. "Mon").
    Short,
    /// The long form of the weekday (e.g. "Monday").
    Long,
    /// A numerical representation using Sunday as the first day of the week.
    ///
    /// Sunday is either 0 or 1, depending on the other modifier's value.
    Sunday,
    /// A numerical representation using Monday as the first day of the week.
    ///
    /// Monday is either 0 or 1, depending on the other modifier's value.
    Monday,
}

/// Day of the week using the short form of the weekday (e.g. "Mon").
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WeekdayShort {
    /// Is the value case sensitive when parsing?
    pub(crate) case_sensitive: bool,
}

impl WeekdayShort {
    builder_methods! {
        /// Set whether the value is case sensitive when parsing.
        fn with_case_sensitive(case_sensitive: bool);
    }
}

/// Day of the week using the long form of the weekday (e.g. "Monday").
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WeekdayLong {
    /// Is the value case sensitive when parsing?
    pub(crate) case_sensitive: bool,
}

impl WeekdayLong {
    builder_methods! {
        /// Set whether the value is case sensitive when parsing.
        fn with_case_sensitive(case_sensitive: bool);
    }
}

/// Day of the week using a numerical representation with Sunday as the first day of the week.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WeekdaySunday {
    /// Is the value zero or one-indexed?
    pub(crate) one_indexed: bool,
}

impl WeekdaySunday {
    builder_methods! {
        /// Set whether the value is one-indexed.
        fn with_one_indexed(one_indexed: bool);
    }
}

/// Day of the week using a numerical representation with Monday as the first day of the week.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WeekdayMonday {
    /// Is the value zero or one-indexed?
    pub(crate) one_indexed: bool,
}

impl WeekdayMonday {
    builder_methods! {
        /// Set whether the value is one-indexed.
        fn with_one_indexed(one_indexed: bool);
    }
}

/// Day of the week.
#[non_exhaustive]
#[allow(deprecated)]
#[deprecated(
    since = "0.3.48",
    note = "use `WeekdayShort`, `WeekdayLong`, `WeekdaySunday`, or `WeekdayMonday` instead"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Weekday {
    /// What form of representation should be used?
    pub repr: WeekdayRepr,
    /// When using a numerical representation, should it be zero or one-indexed?
    pub one_indexed: bool,
    /// Is the value case sensitive when parsing?
    pub case_sensitive: bool,
}

#[expect(deprecated)]
impl Weekday {
    builder_methods! {
        /// Set the manner in which the weekday is represented.
        fn with_repr(repr: WeekdayRepr);
        /// Set whether the value is one-indexed when using a numerical representation.
        fn with_one_indexed(one_indexed: bool);
        /// Set whether the value is case sensitive when parsing.
        fn with_case_sensitive(case_sensitive: bool);
    }
}

/// The representation used for the week number.
#[non_exhaustive]
#[deprecated(
    since = "0.3.48",
    note = "used only in the deprecated `WeekNumber` component"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum WeekNumberRepr {
    /// Week 1 is the week that contains January 4.
    Iso,
    /// Week 1 begins on the first Sunday of the calendar year.
    Sunday,
    /// Week 1 begins on the first Monday of the calendar year.
    Monday,
}

/// Week within the year using the ISO week calendar.
///
/// Week 1 is the week that contains January 4. All weeks begin on a Monday.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WeekNumberIso {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl WeekNumberIso {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Week within the calendar year.
///
/// Week 1 begins on the first Sunday of the calendar year.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WeekNumberSunday {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl WeekNumberSunday {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Week within the calendar year.
///
/// Week 1 begins on the first Monday of the calendar year.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WeekNumberMonday {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl WeekNumberMonday {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Week within the year.
#[non_exhaustive]
#[allow(deprecated)]
#[deprecated(
    since = "0.3.48",
    note = "use `WeekNumberIso`, `WeekNumberSunday`, or `WeekNumberMonday` instead"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WeekNumber {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
    /// What kind of representation should be used?
    pub repr: WeekNumberRepr,
}

#[expect(deprecated)]
impl WeekNumber {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set the manner in which the week number is represented.
        fn with_repr(repr: WeekNumberRepr);
    }
}

/// The representation used for a year value.
#[non_exhaustive]
#[deprecated(
    since = "0.3.48",
    note = "used only in the deprecated `Year` component"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum YearRepr {
    /// The full value of the year.
    Full,
    /// All digits except the last two. Includes the sign, if any.
    Century,
    /// Only the last two digits of the year.
    LastTwo,
}

/// The range of years that are supported.
///
/// This modifier has no effect when the year repr is [`LastTwo`](YearRepr::LastTwo).
#[non_exhaustive]
#[deprecated(
    since = "0.3.48",
    note = "used only in the deprecated `Year` component"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum YearRange {
    /// Years between -9999 and 9999 are supported.
    Standard,
    /// Years between -999_999 and 999_999 are supported, with the sign being required if the year
    /// contains more than four digits.
    ///
    /// If the `large-dates` feature is not enabled, this variant is equivalent to `Standard`.
    Extended,
}

/// Year of the date. All digits are included, the calendar year is used, and the range of years
/// supported is ±999,999.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CalendarYearFullExtendedRange {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
    /// Whether the `+` sign is present when a non-negative year contains fewer digits than
    /// necessary.
    pub(crate) sign_is_mandatory: bool,
}

impl CalendarYearFullExtendedRange {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set whether the `+` sign is mandatory for non-negative years with more than four digits.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Year of the date. All digits are included, the calendar year is used, and the range of years
/// supported is ±9,999.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CalendarYearFullStandardRange {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
    /// Whether the `+` sign is present when a non-negative year contains fewer digits than
    /// necessary.
    pub(crate) sign_is_mandatory: bool,
}

impl CalendarYearFullStandardRange {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set whether the `+` sign is present on non-negative years.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Year of the date. All digits are included, the ISO week-numbering year is used, and the range of
/// years supported is ±999,999.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct IsoYearFullExtendedRange {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
    /// Whether the `+` sign is present when a non-negative year contains fewer digits than
    /// necessary.
    pub(crate) sign_is_mandatory: bool,
}

impl IsoYearFullExtendedRange {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set whether the `+` sign is mandatory for non-negative years with more than four digits.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Year of the date. All digits are included, the ISO week-numbering year is used, and the range of
/// supported is ±9,999.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct IsoYearFullStandardRange {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
    /// Whether the `+` sign is present when a non-negative year contains fewer digits than
    /// necessary.
    pub(crate) sign_is_mandatory: bool,
}

impl IsoYearFullStandardRange {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set whether the `+` sign is present on non-negative years.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Year of the date. Only the century is included (i.e. all digits except the last two), the
/// calendar year is used, and the range of years supported is ±999,999.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CalendarYearCenturyExtendedRange {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
    /// Whether the `+` sign is present when a non-negative year contains fewer digits than
    /// necessary.
    pub(crate) sign_is_mandatory: bool,
}

impl CalendarYearCenturyExtendedRange {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set whether the `+` sign is mandatory for non-negative years with more than four digits.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Year of the date. Only the century is included (i.e. all digits except the last two), the
/// calendar year is used, and the range of years supported is ±9,999.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CalendarYearCenturyStandardRange {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
    /// Whether the `+` sign is present when a non-negative year contains fewer digits than
    /// necessary.
    pub(crate) sign_is_mandatory: bool,
}

impl CalendarYearCenturyStandardRange {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set whether the `+` sign is present on non-negative years.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Year of the date. Only the century is included (i.e. all digits except the last two), the ISO
/// week-numbering year is used, and the range of years supported is ±999,999.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct IsoYearCenturyExtendedRange {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
    /// Whether the `+` sign is present when a non-negative year contains fewer digits than
    /// necessary.
    pub(crate) sign_is_mandatory: bool,
}

impl IsoYearCenturyExtendedRange {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set whether the `+` sign is mandatory for non-negative years with more than four digits.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Year of the date. Only the century is included (i.e. all digits except the last two), the ISO
/// week-numbering year is used, and the range of years supported is ±9,999.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct IsoYearCenturyStandardRange {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
    /// Whether the `+` sign is present when a non-negative year contains fewer digits than
    /// necessary.
    pub(crate) sign_is_mandatory: bool,
}

impl IsoYearCenturyStandardRange {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set whether the `+` sign is present on non-negative years.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Year of the date. Only the last two digits are included, and the calendar year is used.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CalendarYearLastTwo {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
}

impl CalendarYearLastTwo {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Year of the date. Only the last two digits are included, and the ISO week-numbering year is
/// used.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct IsoYearLastTwo {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
}

impl IsoYearLastTwo {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Year of the date.
#[non_exhaustive]
#[allow(deprecated)]
#[deprecated(
    since = "0.3.48",
    note = "use one of the various `Year*` components instead"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Year {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
    /// What kind of representation should be used?
    pub repr: YearRepr,
    /// What range of years is supported?
    pub range: YearRange,
    /// Whether the value is based on the ISO week number or the Gregorian calendar.
    pub iso_week_based: bool,
    /// Whether the `+` sign is present when a non-negative year contains fewer digits than
    /// necessary.
    pub sign_is_mandatory: bool,
}

#[expect(deprecated)]
impl Year {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set the manner in which the year is represented.
        fn with_repr(repr: YearRepr);
        /// Set the range of years that are supported.
        fn with_range(range: YearRange);
        /// Set whether the year is based on the ISO week number.
        fn with_iso_week_based(iso_week_based: bool);
        /// Set whether the `+` sign is mandatory for positive years with fewer than five digits.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Hour of the day using a 12-hour clock.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Hour12 {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
}

impl Hour12 {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Hour of the day using a 24-hour clock.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Hour24 {
    /// The padding to obtain the minimum width.
    pub(crate) padding: Padding,
}

impl Hour24 {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Hour of the day.
#[non_exhaustive]
#[deprecated(since = "0.3.48", note = "use `Hour12` or `Hour24` instead")]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Hour {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
    /// Is the hour displayed using a 12 or 24-hour clock?
    pub is_12_hour_clock: bool,
}

#[expect(deprecated)]
impl Hour {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
        /// Set whether the hour uses a 12-hour clock.
        fn with_is_12_hour_clock(is_12_hour_clock: bool);
    }
}

/// Minute within the hour.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Minute {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl Minute {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// AM/PM part of the time.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Period {
    /// Is the period uppercase or lowercase?
    pub is_uppercase: bool,
    /// Is the value case sensitive when parsing?
    ///
    /// Note that when `false`, the `is_uppercase` field has no effect on parsing behavior.
    pub case_sensitive: bool,
}

impl Period {
    builder_methods! {
        /// Set whether the period is uppercase.
        fn with_is_uppercase(is_uppercase: bool);
        /// Set whether the value is case sensitive when parsing.
        fn with_case_sensitive(case_sensitive: bool);
    }
}

/// Second within the minute.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Second {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl Second {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// The number of digits present in a subsecond representation.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SubsecondDigits {
    /// Exactly one digit.
    One,
    /// Exactly two digits.
    Two,
    /// Exactly three digits.
    Three,
    /// Exactly four digits.
    Four,
    /// Exactly five digits.
    Five,
    /// Exactly six digits.
    Six,
    /// Exactly seven digits.
    Seven,
    /// Exactly eight digits.
    Eight,
    /// Exactly nine digits.
    Nine,
    /// Any number of digits (up to nine) that is at least one. When formatting, the minimum digits
    /// necessary will be used.
    OneOrMore,
}

/// Subsecond within the second.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Subsecond {
    /// How many digits are present in the component?
    pub digits: SubsecondDigits,
}

impl Subsecond {
    builder_methods! {
        /// Set the number of digits present in the subsecond representation.
        fn with_digits(digits: SubsecondDigits);
    }
}

/// Hour of the UTC offset.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct OffsetHour {
    /// Whether the `+` sign is present on positive values.
    pub sign_is_mandatory: bool,
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl OffsetHour {
    builder_methods! {
        /// Set whether the `+` sign is mandatory for positive values.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Minute within the hour of the UTC offset.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct OffsetMinute {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl OffsetMinute {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Second within the minute of the UTC offset.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct OffsetSecond {
    /// The padding to obtain the minimum width.
    pub padding: Padding,
}

impl OffsetSecond {
    builder_methods! {
        /// Set the padding type.
        fn with_padding(padding: Padding);
    }
}

/// Type of padding to ensure a minimum width.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Padding {
    /// A space character (` `) should be used as padding.
    Space,
    /// A zero character (`0`) should be used as padding.
    Zero,
    /// There is no padding. This can result in a width below the otherwise minimum number of
    /// characters.
    None,
}

/// Ignore some number of bytes.
///
/// This has no effect when formatting.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Ignore {
    /// The number of bytes to ignore.
    pub count: NonZero<u16>,
}

// Needed as `Default` is deliberately not implemented for `Ignore`. The number of bytes to ignore
// must be explicitly provided.
impl Ignore {
    /// Create an instance of `Ignore` with the provided number of bytes to ignore.
    #[inline]
    pub const fn count(count: NonZero<u16>) -> Self {
        Self { count }
    }

    builder_methods! {
        /// Set the number of bytes to ignore.
        fn with_count(count: NonZero<u16>);
    }
}

/// The precision of a Unix timestamp.
#[non_exhaustive]
#[deprecated(
    since = "0.3.48",
    note = "only used in the deprecated `UnixTimestamp` component"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum UnixTimestampPrecision {
    /// Seconds since the Unix epoch.
    Second,
    /// Milliseconds since the Unix epoch.
    Millisecond,
    /// Microseconds since the Unix epoch.
    Microsecond,
    /// Nanoseconds since the Unix epoch.
    Nanosecond,
}

/// A Unix timestamp in seconds.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct UnixTimestampSecond {
    /// Whether the `+` sign must be present for a non-negative timestamp.
    pub(crate) sign_is_mandatory: bool,
}

impl UnixTimestampSecond {
    builder_methods! {
        /// Set whether the `+` sign is mandatory for non-negative timestamps.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// A Unix timestamp in milliseconds.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct UnixTimestampMillisecond {
    /// Whether the `+` sign must be present for a non-negative timestamp.
    pub(crate) sign_is_mandatory: bool,
}

impl UnixTimestampMillisecond {
    builder_methods! {
        /// Set whether the `+` sign is mandatory for non-negative timestamps.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// A Unix timestamp in microseconds.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct UnixTimestampMicrosecond {
    /// Whether the `+` sign must be present for a non-negative timestamp.
    pub(crate) sign_is_mandatory: bool,
}

impl UnixTimestampMicrosecond {
    builder_methods! {
        /// Set whether the `+` sign is mandatory for non-negative timestamps.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// A Unix timestamp in nanoseconds.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct UnixTimestampNanosecond {
    /// Whether the `+` sign must be present for a non-negative timestamp.
    pub(crate) sign_is_mandatory: bool,
}

impl UnixTimestampNanosecond {
    builder_methods! {
        /// Set whether the `+` sign is mandatory for non-negative timestamps.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// A Unix timestamp.
#[non_exhaustive]
#[allow(deprecated)]
#[deprecated(
    since = "0.3.48",
    note = "use `UnixTimestampSeconds`, `UnixTimestampMilliseconds`, `UnixTimestampMicroseconds`, \
            or `UnixTimestampNanoseconds` instead"
)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct UnixTimestamp {
    /// The precision of the timestamp.
    pub precision: UnixTimestampPrecision,
    /// Whether the `+` sign must be present for a non-negative timestamp.
    pub sign_is_mandatory: bool,
}

#[expect(deprecated)]
impl UnixTimestamp {
    builder_methods! {
        /// Set the precision of the timestamp.
        fn with_precision(precision: UnixTimestampPrecision);
        /// Set whether the `+` sign is mandatory for non-negative timestamps.
        fn with_sign_is_mandatory(sign_is_mandatory: bool);
    }
}

/// Whether trailing input after the declared end is permitted.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TrailingInput {
    /// Trailing input is not permitted and will cause an error.
    Prohibit,
    /// Trailing input is permitted but discarded.
    Discard,
}

/// The end of input.
#[non_exhaustive]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct End {
    /// How to handle any input after this component.
    pub(crate) trailing_input: TrailingInput,
}

impl End {
    builder_methods! {
        /// Set how to handle any input after this component.
        fn with_trailing_input(trailing_input: TrailingInput);
    }
}
