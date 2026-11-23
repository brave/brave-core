use num_conv::prelude::*;

use crate::format_description::Period;
use crate::formatting::{
    Day, IsoWeekNumber, MondayBasedWeek, OptionDay, OptionIsoWeekNumber, OptionYear, Ordinal,
    SundayBasedWeek, Year,
};
use crate::time::{Hours, Minutes, Nanoseconds, Seconds};
use crate::utc_offset::{Hours as OffsetHours, Minutes as OffsetMinutes, Seconds as OffsetSeconds};
use crate::{
    Date, Month, OffsetDateTime, PlainDateTime, Time, Timestamp, UtcDateTime, UtcOffset, Weekday,
};

/// State used by date-providing types to cache computed values.
///
/// This is used to avoid redundant computations when multiple date components are almost certainly
/// going to be requested within the same formatting invocation.
#[derive(Debug, Default)]
pub(crate) struct DateState {
    day: OptionDay,
    month: Option<Month>,
    iso_week: OptionIsoWeekNumber,
    iso_year: OptionYear,
}

/// State used by `Timestamp` to cache computed date and time values.
///
/// `Date` and `Time` are cached separately, with the `Date`'s state being stored to allow for
/// reusing existing methods.
#[derive(Debug, Default)]
pub(crate) struct TimestampState {
    date: Option<Date>,
    time: Option<Time>,
    date_state: <Date as ComponentProvider>::State,
}

macro_rules! unimplemented_methods {
    ($(
        $(#[$meta:meta])*
        ($component:literal) $name:ident => $ret:ty;
    )*) => {
        $(
            $(#[$meta])*
            #[track_caller]
            #[expect(unused_variables, reason = "better for auto-generation of method stubs")]
            fn $name(&self, state: &mut Self::State) -> $ret {
                unimplemented!(concat!("type does not supply ", $component, " components"))
            }
        )*
    };
}

macro_rules! delegate_providers {
    (
        $target:ident {
            $($method:ident -> $return:ty)*
        }
    ) => {$(
        #[inline]
        fn $method(&self, state: &mut Self::State) -> $return {
            ComponentProvider::$method(&self.$target(), state)
        }
    )*};
    (
        $target:ident ($state:expr) {
            $($method:ident -> $return:ty)*
        }
    ) => {$(
        #[inline]
        fn $method(&self, _: &mut Self::State) -> $return {
            ComponentProvider::$method(&self.$target(), $state)
        }
    )*};
}

/// A type with the ability to provide date, time, offset, and/or timestamp components on demand.
///
/// Note that while all methods have a default body, implementations are expected to override the
/// body for all components that they provide. The default implementation exists solely for
/// convenience, avoiding the need to specify unprovided components.
pub(crate) trait ComponentProvider {
    /// The state type used by the provider, allowing for caching of computed values.
    type State: Default;

    /// Whether the type can provide date components, indicating that date-related methods can be
    /// called.
    const SUPPLIES_DATE: bool = false;
    /// Whether the type can provide time components, indicating that time-related methods can be
    /// called.
    const SUPPLIES_TIME: bool = false;
    /// Whether the type can provide offset components, indicating that offset-related methods can
    /// be called.
    const SUPPLIES_OFFSET: bool = false;
    /// Whether the type can provide timestamp components, indicating that timestamp-related methods
    /// can be called.
    const SUPPLIES_TIMESTAMP: bool = false;

    unimplemented_methods! {
        /// Obtain the day of the month.
        ("date") day => Day;
        /// Obtain the month of the year.
        ("date") month => Month;
        /// Obtain the ordinal day of the year.
        ("date") ordinal => Ordinal;
        /// Obtain the day of the week.
        ("date") weekday => Weekday;
        /// Obtain the ISO week number.
        ("date") iso_week_number => IsoWeekNumber;
        /// Obtain the Monday-based week number.
        ("date") monday_based_week => MondayBasedWeek;
        /// Obtain the Sunday-based week number.
        ("date") sunday_based_week => SundayBasedWeek;
        /// Obtain the calendar year.
        ("date") calendar_year => Year;
        /// Obtain the ISO week-based year.
        ("date") iso_year => Year;
        /// Obtain the hour within the day.
        ("time") hour => Hours;
        /// Obtain the minute within the hour.
        ("time") minute => Minutes;
        /// Obtain the period of the day (AM/PM).
        ("time") period => Period;
        /// Obtain the second within the minute.
        ("time") second => Seconds;
        /// Obtain the nanosecond within the second.
        ("time") nanosecond => Nanoseconds;
        /// Obtain whether the offset is negative.
        ("offset") offset_is_negative => bool;
        /// Obtain whether the offset is UTC.
        ("offset") offset_is_utc => bool;
        /// Obtain the hour component of the UTC offset.
        ("offset") offset_hour => OffsetHours;
        /// Obtain the minute component of the UTC offset.
        ("offset") offset_minute => OffsetMinutes;
        /// Obtain the second component of the UTC offset.
        ("offset") offset_second => OffsetSeconds;
        /// Obtain the Unix timestamp in seconds.
        ("timestamp") unix_timestamp_seconds => i64;
        /// Obtain the Unix timestamp in milliseconds.
        ("timestamp") unix_timestamp_milliseconds => i64;
        /// Obtain the Unix timestamp in microseconds.
        ("timestamp") unix_timestamp_microseconds => i128;
        /// Obtain the Unix timestamp in nanoseconds.
        ("timestamp") unix_timestamp_nanoseconds => i128;
    }
}

impl ComponentProvider for Time {
    type State = ();

    const SUPPLIES_TIME: bool = true;

    #[inline]
    fn hour(&self, _: &mut Self::State) -> Hours {
        self.as_hms_nano_ranged().0
    }

    #[inline]
    fn minute(&self, _: &mut Self::State) -> Minutes {
        self.as_hms_nano_ranged().1
    }

    #[inline]
    fn period(&self, _: &mut Self::State) -> Period {
        if (*self).hour() < 12 {
            Period::Am
        } else {
            Period::Pm
        }
    }

    #[inline]
    fn second(&self, _: &mut Self::State) -> Seconds {
        self.as_hms_nano_ranged().2
    }

    #[inline]
    fn nanosecond(&self, _: &mut Self::State) -> Nanoseconds {
        self.as_hms_nano_ranged().3
    }
}

impl ComponentProvider for Date {
    type State = DateState;

    const SUPPLIES_DATE: bool = true;

    #[inline]
    fn day(&self, state: &mut Self::State) -> Day {
        if let Some(day) = state.day.get() {
            return day;
        }

        let (_, month, day) = (*self).to_calendar_date();
        // Safety: `day` is guaranteed to be in range.
        let day = unsafe { Day::new_unchecked(day) };
        state.month = Some(month);
        state.day = OptionDay::Some(day);
        day
    }

    #[inline]
    fn month(&self, state: &mut Self::State) -> Month {
        *state.month.get_or_insert_with(|| (*self).month())
    }

    #[inline]
    fn ordinal(&self, _: &mut Self::State) -> Ordinal {
        // Safety: `self.ordinal()` is guaranteed to be in range.
        unsafe { Ordinal::new_unchecked((*self).ordinal()) }
    }

    #[inline]
    fn weekday(&self, _: &mut Self::State) -> Weekday {
        (*self).weekday()
    }

    #[inline]
    fn iso_week_number(&self, state: &mut Self::State) -> IsoWeekNumber {
        if let Some(week) = state.iso_week.get() {
            return week;
        }

        let (iso_year, iso_week) = (*self).iso_year_week();
        // Safety: `iso_week` is guaranteed to be non-zero.
        let iso_week = unsafe { IsoWeekNumber::new_unchecked(iso_week) };
        // Safety: `iso_year` is guaranteed to be in range.
        state.iso_year = OptionYear::Some(unsafe { Year::new_unchecked(iso_year) });
        state.iso_week = OptionIsoWeekNumber::Some(iso_week);
        iso_week
    }

    #[inline]
    fn monday_based_week(&self, _: &mut Self::State) -> MondayBasedWeek {
        // Safety: `self.monday_based_week()` is guaranteed to be in range.
        unsafe { MondayBasedWeek::new_unchecked((*self).monday_based_week()) }
    }

    #[inline]
    fn sunday_based_week(&self, _: &mut Self::State) -> SundayBasedWeek {
        // Safety: `self.sunday_based_week()` is guaranteed to be in range.
        unsafe { SundayBasedWeek::new_unchecked((*self).sunday_based_week()) }
    }

    #[inline]
    fn calendar_year(&self, _: &mut Self::State) -> Year {
        // Safety: `self.year()` is guaranteed to be in range.
        unsafe { Year::new_unchecked((*self).year()) }
    }

    #[inline]
    fn iso_year(&self, state: &mut Self::State) -> Year {
        if let Some(iso_year) = state.iso_year.get() {
            return iso_year;
        }

        let (iso_year, iso_week) = (*self).iso_year_week();
        // Safety: `iso_year_week` returns a valid ISO year.
        let iso_year = unsafe { Year::new_unchecked(iso_year) };
        state.iso_year = OptionYear::Some(iso_year);
        // Safety: `iso_week` is guaranteed to be non-zero.
        state.iso_week =
            OptionIsoWeekNumber::Some(unsafe { IsoWeekNumber::new_unchecked(iso_week) });
        iso_year
    }
}

impl ComponentProvider for PlainDateTime {
    type State = DateState;

    const SUPPLIES_DATE: bool = true;
    const SUPPLIES_TIME: bool = true;

    delegate_providers!(date {
        day -> Day
        month -> Month
        ordinal -> Ordinal
        weekday -> Weekday
        iso_week_number -> IsoWeekNumber
        monday_based_week -> MondayBasedWeek
        sunday_based_week -> SundayBasedWeek
        calendar_year -> Year
        iso_year -> Year
    });
    delegate_providers!(time (&mut ()) {
        hour -> Hours
        minute -> Minutes
        period -> Period
        second -> Seconds
        nanosecond -> Nanoseconds
    });
}

impl ComponentProvider for UtcOffset {
    type State = ();

    const SUPPLIES_OFFSET: bool = true;

    #[inline]
    fn offset_is_negative(&self, _: &mut Self::State) -> bool {
        (*self).is_negative()
    }

    #[inline]
    fn offset_is_utc(&self, _state: &mut Self::State) -> bool {
        (*self).is_utc()
    }

    #[inline]
    fn offset_hour(&self, _: &mut Self::State) -> OffsetHours {
        (*self).as_hms_ranged().0
    }

    #[inline]
    fn offset_minute(&self, _: &mut Self::State) -> OffsetMinutes {
        (*self).as_hms_ranged().1
    }

    #[inline]
    fn offset_second(&self, _: &mut Self::State) -> OffsetSeconds {
        (*self).as_hms_ranged().2
    }
}

impl ComponentProvider for UtcDateTime {
    type State = DateState;

    const SUPPLIES_DATE: bool = true;
    const SUPPLIES_TIME: bool = true;
    const SUPPLIES_OFFSET: bool = true;
    const SUPPLIES_TIMESTAMP: bool = true;

    delegate_providers!(date {
        day -> Day
        month -> Month
        ordinal -> Ordinal
        weekday -> Weekday
        iso_week_number -> IsoWeekNumber
        monday_based_week -> MondayBasedWeek
        sunday_based_week -> SundayBasedWeek
        calendar_year -> Year
        iso_year -> Year
    });
    delegate_providers!(time (&mut ()) {
        hour -> Hours
        minute -> Minutes
        period -> Period
        second -> Seconds
        nanosecond -> Nanoseconds
    });

    #[inline]
    fn offset_is_negative(&self, _: &mut Self::State) -> bool {
        false
    }

    #[inline]
    fn offset_is_utc(&self, _state: &mut Self::State) -> bool {
        true
    }

    #[inline]
    fn offset_hour(&self, _: &mut Self::State) -> OffsetHours {
        OffsetHours::new_static::<0>()
    }

    #[inline]
    fn offset_minute(&self, _: &mut Self::State) -> OffsetMinutes {
        OffsetMinutes::new_static::<0>()
    }

    #[inline]
    fn offset_second(&self, _: &mut Self::State) -> OffsetSeconds {
        OffsetSeconds::new_static::<0>()
    }

    #[inline]
    fn unix_timestamp_seconds(&self, _: &mut Self::State) -> i64 {
        (*self).unix_timestamp()
    }

    #[inline]
    fn unix_timestamp_milliseconds(&self, state: &mut Self::State) -> i64 {
        (ComponentProvider::unix_timestamp_nanoseconds(self, state) / 1_000_000).truncate()
    }

    #[inline]
    fn unix_timestamp_microseconds(&self, state: &mut Self::State) -> i128 {
        ComponentProvider::unix_timestamp_nanoseconds(self, state) / 1_000
    }

    #[inline]
    fn unix_timestamp_nanoseconds(&self, _: &mut Self::State) -> i128 {
        (*self).unix_timestamp_nanos()
    }
}

impl ComponentProvider for OffsetDateTime {
    type State = DateState;

    const SUPPLIES_DATE: bool = true;
    const SUPPLIES_TIME: bool = true;
    const SUPPLIES_OFFSET: bool = true;
    const SUPPLIES_TIMESTAMP: bool = true;

    delegate_providers!(date {
        day -> Day
        month -> Month
        ordinal -> Ordinal
        weekday -> Weekday
        iso_week_number -> IsoWeekNumber
        monday_based_week -> MondayBasedWeek
        sunday_based_week -> SundayBasedWeek
        calendar_year -> Year
        iso_year -> Year
    });
    delegate_providers!(time (&mut ()) {
        hour -> Hours
        minute -> Minutes
        period -> Period
        second -> Seconds
        nanosecond -> Nanoseconds
    });
    delegate_providers!(offset (&mut ()) {
        offset_is_negative -> bool
        offset_is_utc -> bool
        offset_hour -> OffsetHours
        offset_minute -> OffsetMinutes
        offset_second -> OffsetSeconds
    });

    #[inline]
    fn unix_timestamp_seconds(&self, _: &mut Self::State) -> i64 {
        (*self).unix_timestamp()
    }

    #[inline]
    fn unix_timestamp_milliseconds(&self, _: &mut Self::State) -> i64 {
        ((*self).unix_timestamp_nanos() / 1_000_000) as i64
    }

    #[inline]
    fn unix_timestamp_microseconds(&self, _: &mut Self::State) -> i128 {
        (*self).unix_timestamp_nanos() / 1_000
    }

    #[inline]
    fn unix_timestamp_nanoseconds(&self, _: &mut Self::State) -> i128 {
        (*self).unix_timestamp_nanos()
    }
}

impl ComponentProvider for Timestamp {
    type State = TimestampState;

    const SUPPLIES_DATE: bool = true;
    const SUPPLIES_TIME: bool = true;
    const SUPPLIES_OFFSET: bool = true;
    const SUPPLIES_TIMESTAMP: bool = true;

    #[inline]
    fn day(&self, state: &mut Self::State) -> Day {
        let date = state.date.get_or_insert_with(|| self.date());
        ComponentProvider::day(date, &mut state.date_state)
    }

    #[inline]
    fn month(&self, state: &mut Self::State) -> Month {
        let date = state.date.get_or_insert_with(|| self.date());
        ComponentProvider::month(date, &mut state.date_state)
    }

    #[inline]
    fn ordinal(&self, state: &mut Self::State) -> Ordinal {
        let date = state.date.get_or_insert_with(|| self.date());
        ComponentProvider::ordinal(date, &mut state.date_state)
    }

    #[inline]
    fn weekday(&self, state: &mut Self::State) -> Weekday {
        let date = state.date.get_or_insert_with(|| self.date());
        ComponentProvider::weekday(date, &mut state.date_state)
    }

    #[inline]
    fn iso_week_number(&self, state: &mut Self::State) -> IsoWeekNumber {
        let date = state.date.get_or_insert_with(|| self.date());
        ComponentProvider::iso_week_number(date, &mut state.date_state)
    }

    #[inline]
    fn monday_based_week(&self, state: &mut Self::State) -> MondayBasedWeek {
        let date = state.date.get_or_insert_with(|| self.date());
        ComponentProvider::monday_based_week(date, &mut state.date_state)
    }

    #[inline]
    fn sunday_based_week(&self, state: &mut Self::State) -> SundayBasedWeek {
        let date = state.date.get_or_insert_with(|| self.date());
        ComponentProvider::sunday_based_week(date, &mut state.date_state)
    }

    #[inline]
    fn calendar_year(&self, state: &mut Self::State) -> Year {
        let date = state.date.get_or_insert_with(|| self.date());
        ComponentProvider::calendar_year(date, &mut state.date_state)
    }

    #[inline]
    fn iso_year(&self, state: &mut Self::State) -> Year {
        let date = state.date.get_or_insert_with(|| self.date());
        ComponentProvider::iso_year(date, &mut state.date_state)
    }

    #[inline]
    fn hour(&self, state: &mut Self::State) -> Hours {
        let time = state.time.get_or_insert_with(|| self.time());
        ComponentProvider::hour(time, &mut ())
    }

    #[inline]
    fn minute(&self, state: &mut Self::State) -> Minutes {
        let time = state.time.get_or_insert_with(|| self.time());
        ComponentProvider::minute(time, &mut ())
    }

    #[inline]
    fn period(&self, state: &mut Self::State) -> Period {
        let time = state.time.get_or_insert_with(|| self.time());
        ComponentProvider::period(time, &mut ())
    }

    #[inline]
    fn second(&self, state: &mut Self::State) -> Seconds {
        let time = state.time.get_or_insert_with(|| self.time());
        ComponentProvider::second(time, &mut ())
    }

    #[inline]
    fn nanosecond(&self, _: &mut Self::State) -> Nanoseconds {
        // No need to cache time here, as nanosecond is stored separately in `Timestamp` and can be
        // directly accessed.
        self.as_parts_ranged().1
    }

    #[inline]
    fn offset_is_negative(&self, _: &mut Self::State) -> bool {
        false
    }

    #[inline]
    fn offset_is_utc(&self, _: &mut Self::State) -> bool {
        true
    }

    #[inline]
    fn offset_hour(&self, _: &mut Self::State) -> OffsetHours {
        OffsetHours::new_static::<0>()
    }

    #[inline]
    fn offset_minute(&self, _: &mut Self::State) -> OffsetMinutes {
        OffsetMinutes::new_static::<0>()
    }

    #[inline]
    fn offset_second(&self, _: &mut Self::State) -> OffsetSeconds {
        OffsetSeconds::new_static::<0>()
    }

    #[inline]
    fn unix_timestamp_seconds(&self, _: &mut Self::State) -> i64 {
        self.as_seconds()
    }

    #[inline]
    fn unix_timestamp_milliseconds(&self, _: &mut Self::State) -> i64 {
        self.as_milliseconds()
    }

    #[inline]
    fn unix_timestamp_microseconds(&self, _: &mut Self::State) -> i128 {
        self.as_microseconds()
    }

    #[inline]
    fn unix_timestamp_nanoseconds(&self, _: &mut Self::State) -> i128 {
        self.as_nanoseconds()
    }
}
