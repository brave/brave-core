use std::io;
use std::num::NonZero;

use rstest::rstest;
use time::format_description::well_known::iso8601::{DateKind, OffsetPrecision, TimePrecision};
use time::format_description::well_known::{Iso8601, Rfc2822, Rfc3339, iso8601};
use time::format_description::{self, BorrowedFormatItem, OwnedFormatItem};
use time::formatting::Formattable;
use time::macros::{date, datetime, format_description as fd, offset, time, utc_datetime};
use time::{Date, OffsetDateTime, PlainDateTime, Time, UtcDateTime, UtcOffset};

#[rstest]
#[case(datetime!(2021-01-02 03:04:05 UTC), "Sat, 02 Jan 2021 03:04:05 +0000")]
#[case(datetime!(2021-01-02 03:04:05 +06:07), "Sat, 02 Jan 2021 03:04:05 +0607")]
#[case(datetime!(2021-01-02 03:04:05 -06:07), "Sat, 02 Jan 2021 03:04:05 -0607")]
fn rfc_2822_odt(#[case] dt: OffsetDateTime, #[case] expected: &str) {
    assert_eq!(dt.format(&Rfc2822).ok().as_deref(), Some(expected));
}

#[rstest]
#[case(utc_datetime!(2021-01-02 03:04:05), "Sat, 02 Jan 2021 03:04:05 +0000")]
fn rfc_2822_udt(#[case] dt: UtcDateTime, #[case] expected: &str) {
    assert_eq!(dt.format(&Rfc2822).ok().as_deref(), Some(expected));
}

#[rstest]
#[case(datetime!(1885-01-01 01:01:01 UTC), "year")]
#[case(datetime!(2000-01-01 00:00:00 +00:00:01), "offset_second")]
fn rfc_2822_invalid_odt(#[case] odt: OffsetDateTime, #[case] component: &str) {
    assert!(matches!(
        odt.format(&Rfc2822),
        Err(time::error::Format::InvalidComponent(c)) if c == component
    ));
}

#[rstest]
#[case(utc_datetime!(1885-01-01 01:01:01), "year")]
fn rfc_2822_invalid_udt(#[case] udt: UtcDateTime, #[case] component: &str) {
    assert!(matches!(
        udt.format(&Rfc2822),
        Err(time::error::Format::InvalidComponent(c)) if c == component
    ));
}

#[rstest]
#[case(datetime!(2021-01-02 03:04:05 UTC), "2021-01-02T03:04:05Z")]
#[case(datetime!(2021-01-02 03:04:05.1 UTC), "2021-01-02T03:04:05.1Z")]
#[case(datetime!(2021-01-02 03:04:05.12 UTC), "2021-01-02T03:04:05.12Z")]
#[case(datetime!(2021-01-02 03:04:05.123 UTC), "2021-01-02T03:04:05.123Z")]
#[case(datetime!(2021-01-02 03:04:05.123_4 UTC), "2021-01-02T03:04:05.1234Z")]
#[case(datetime!(2021-01-02 03:04:05.123_45 UTC), "2021-01-02T03:04:05.12345Z")]
#[case(datetime!(2021-01-02 03:04:05.123_456 UTC), "2021-01-02T03:04:05.123456Z")]
#[case(datetime!(2021-01-02 03:04:05.123_456_7 UTC), "2021-01-02T03:04:05.1234567Z")]
#[case(datetime!(2021-01-02 03:04:05.123_456_78 UTC), "2021-01-02T03:04:05.12345678Z")]
#[case(datetime!(2021-01-02 03:04:05.123_456_789 UTC), "2021-01-02T03:04:05.123456789Z")]
#[case(datetime!(2021-01-02 03:04:05.123_456_789 -01:02), "2021-01-02T03:04:05.123456789-01:02")]
#[case(datetime!(2021-01-02 03:04:05.123_456_789 +01:02), "2021-01-02T03:04:05.123456789+01:02")]
fn rfc_3339(#[case] dt: OffsetDateTime, #[case] expected: &str) {
    assert_eq!(dt.format(&Rfc3339).ok(), Some(expected.to_string()));
}

#[rstest]
#[case(datetime!(-0001-01-01 0:00 UTC), "year")]
#[case(datetime!(0000-01-01 0:00 +00:00:01), "offset_second")]
fn rfc_3339_err(#[case] dt: OffsetDateTime, #[case] component: &str) {
    assert!(matches!(
        dt.format(&Rfc3339),
        Err(time::error::Format::InvalidComponent(c)) if c == component
    ));
}

#[rstest]
#[case(time!(01:02:03.123_456_789), Iso8601::TIME, "01:02:03.123456789")]
fn iso_8601_time(#[case] value: Time, #[case] format: impl Formattable, #[case] expected: &str) {
    assert!(matches!(value.format(&format), Ok(e) if e == expected));
}

#[rstest]
#[case(
    datetime!(-123_456-01-02 03:04:05 UTC),
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_year_is_six_digits(true)
            .encode()
    }>,
    "-123456-01-02T03:04:05.000000000Z",
)]
#[case(
    datetime!(-123_456-01-02 03:04:05 UTC),
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_date_kind(DateKind::Ordinal)
            .set_year_is_six_digits(true)
            .encode()
    }>,
    "-123456-002T03:04:05.000000000Z",
)]
#[case(
    datetime!(-123_456-01-02 03:04:05 UTC),
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_date_kind(DateKind::Week)
            .set_year_is_six_digits(true)
            .encode()
    }>,
    "-123456-W01-4T03:04:05.000000000Z",
)]
#[case(
    datetime!(2021-01-02 03:04:05+1:00),
    Iso8601::DEFAULT,
    "2021-01-02T03:04:05.000000000+01:00",
)]
#[case(
    datetime!(2021-01-02 03:04:05+1:00),
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_offset_precision(OffsetPrecision::Hour)
            .encode()
    }>,
    "2021-01-02T03:04:05.000000000+01",
)]
#[case(
    datetime!(2021-01-02 03:04:05 UTC),
    Iso8601::DEFAULT,
    "2021-01-02T03:04:05.000000000Z",
)]
#[case(
    datetime!(2021-01-02 03:04:05 UTC),
    Iso8601::<{ iso8601::Config::DEFAULT.set_use_separators(false).encode() }>,
    "20210102T030405.000000000Z",
)]
#[case(
    datetime!(2021-01-02 03:04:05 UTC),
    Iso8601::<{ iso8601::Config::DEFAULT.set_year_is_six_digits(true).encode() }>,
    "+002021-01-02T03:04:05.000000000Z",
)]
#[case(
    datetime!(2021-01-02 03:04:05 UTC),
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_time_precision(TimePrecision::Hour { decimal_digits: None })
            .encode()
    }>,
    "2021-01-02T03Z",
)]
#[case(
    datetime!(2021-01-02 03:04:05 UTC),
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_time_precision(TimePrecision::Minute { decimal_digits: None })
            .encode()
    }>,
    "2021-01-02T03:04Z",
)]
#[case(
    datetime!(2021-01-02 03:04:05 UTC),
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_time_precision(TimePrecision::Second { decimal_digits: None })
            .encode()
    }>,
    "2021-01-02T03:04:05Z",
)]
#[case(
    datetime!(2021-01-02 03:04:05 UTC),
    Iso8601::<{ iso8601::Config::DEFAULT.set_date_kind(DateKind::Ordinal).encode() }>,
    "2021-002T03:04:05.000000000Z",
)]
#[case(
    datetime!(2021-01-02 03:04:05 UTC),
    Iso8601::<{ iso8601::Config::DEFAULT.set_date_kind(DateKind::Week).encode() }>,
    "2020-W53-6T03:04:05.000000000Z",
)]
#[case::issue_773(
    datetime!(2026-03-23 09:27:03.631670 UTC),
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_time_precision(TimePrecision::Second { decimal_digits: NonZero::new(6) })
            .encode()
    }>,
    "2026-03-23T09:27:03.631670Z",
)]
fn iso_8601_odt(
    #[case] value: OffsetDateTime,
    #[case] format: impl Formattable,
    #[case] expected: &str,
) {
    assert!(matches!(value.format(&format), Ok(e) if e == expected));
}

#[rstest]
#[case(datetime!(+10_000-01-01 0:00 UTC), Iso8601::DEFAULT, "year")]
#[case(
    datetime!(+10_000-W01-1 0:00 UTC),
    Iso8601::<{ iso8601::Config::DEFAULT.set_date_kind(DateKind::Week).encode() }>,
    "year",
)]
#[case(
    datetime!(+10_000-001 0:00 UTC),
    Iso8601::<{ iso8601::Config::DEFAULT.set_date_kind(DateKind::Ordinal).encode() }>,
    "year",
)]
#[case(datetime!(2021-01-02 03:04:05 +0:00:01), Iso8601::DEFAULT, "offset_second")]
#[case(
    datetime!(2021-01-02 03:04:05 +0:01),
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_offset_precision(OffsetPrecision::Hour)
            .encode()
    }>,
    "offset_minute",
)]
fn iso_8601_invalid(
    #[case] value: OffsetDateTime,
    #[case] format: impl Formattable,
    #[case] component: &str,
) {
    assert!(matches!(
        value.format(&format),
        Err(time::error::Format::InvalidComponent(c)) if c == component
    ));
}

#[rstest]
#[case(Iso8601::DEFAULT, "2021-01-02T03:04:05.999999999Z")]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_time_precision(TimePrecision::Second { decimal_digits: NonZero::new(9) })
            .encode()
    }>,
    "2021-01-02T03:04:05.999999999Z",
)]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_time_precision(TimePrecision::Second { decimal_digits: NonZero::new(6) })
            .encode()
    }>,
    "2021-01-02T03:04:05.999999Z",
)]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_time_precision(TimePrecision::Second { decimal_digits: NonZero::new(3) })
            .encode()
    }>,
    "2021-01-02T03:04:05.999Z",
)]
fn iso_8601_issue_678(#[case] format: impl Formattable, #[case] expected: &str) {
    assert!(matches!(
        datetime!(2021-01-02 03:04:05.999_999_999 UTC).format(&format),
        Ok(e) if e == expected
    ));
}

#[rstest]
#[case(time!(13:02:03.456_789_012), fd!("[hour]"), "13")]
#[case(time!(13:02:03.456_789_012), fd!("[hour repr:12]"), "01")]
#[case(time!(13:02:03.456_789_012), fd!("[hour repr:12 padding:none]"), "1")]
#[case(time!(13:02:03.456_789_012), fd!("[hour repr:12 padding:space]"), " 1")]
#[case(time!(13:02:03.456_789_012), fd!("[hour repr:24]"), "13")]
#[case(time!(13:02:03.456_789_012), fd!("[hour repr:24 padding:none]"), "13")]
#[case(time!(13:02:03.456_789_012), fd!("[hour repr:24 padding:space]"), "13")]
#[case(time!(13:02:03.456_789_012), fd!("[minute]"), "02")]
#[case(time!(13:02:03.456_789_012), fd!("[minute padding:none]"), "2")]
#[case(time!(13:02:03.456_789_012), fd!("[minute padding:space]"), " 2")]
#[case(time!(13:02:03.456_789_012), fd!("[minute padding:zero]"), "02")]
#[case(time!(13:02:03.456_789_012), fd!("[period]"), "PM")]
#[case(time!(13:02:03.456_789_012), fd!("[period case:upper]"), "PM")]
#[case(time!(13:02:03.456_789_012), fd!("[period case:lower]"), "pm")]
#[case(time!(13:02:03.456_789_012), fd!("[second]"), "03")]
#[case(time!(13:02:03.456_789_012), fd!("[second padding:none]"), "3")]
#[case(time!(13:02:03.456_789_012), fd!("[second padding:space]"), " 3")]
#[case(time!(13:02:03.456_789_012), fd!("[second padding:zero]"), "03")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond]"), "456789012")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:1]"), "4")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:2]"), "45")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:3]"), "456")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:4]"), "4567")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:5]"), "45678")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:6]"), "456789")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:7]"), "4567890")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:8]"), "45678901")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:9]"), "456789012")]
#[case(time!(13:02:03.456_789_012), fd!("[subsecond digits:1+]"), "456789012")]
#[case(time!(1:02:03), fd!("[hour repr:12][period]"), "01AM")]
#[case(Time::MIDNIGHT, fd!("[hour repr:12][period case:lower]"), "12am")]
#[case(Time::MIDNIGHT, fd!("[subsecond digits:1+]"), "0")]
#[case(time!(0:00:00.01), fd!("[subsecond digits:1+]"), "01")]
#[case(time!(0:00:00.001), fd!("[subsecond digits:1+]"), "001")]
#[case(time!(0:00:00.0001), fd!("[subsecond digits:1+]"), "0001")]
#[case(time!(0:00:00.00001), fd!("[subsecond digits:1+]"), "00001")]
#[case(time!(0:00:00.000001), fd!("[subsecond digits:1+]"), "000001")]
#[case(time!(0:00:00.0000001), fd!("[subsecond digits:1+]"), "0000001")]
#[case(time!(0:00:00.00000001), fd!("[subsecond digits:1+]"), "00000001")]
#[case(time!(0:00:00.000000001), fd!("[subsecond digits:1+]"), "000000001")]
fn format_time(
    #[case] value: Time,
    #[case] format_description: &[BorrowedFormatItem<'_>],
    #[case] output: &str,
) -> time::Result<()> {
    assert_eq!(value.format(format_description)?, output);
    assert!(
        value
            .format_into(&mut io::sink(), format_description)
            .is_ok()
    );
    assert_eq!(
        value.format(&OwnedFormatItem::from(format_description))?,
        output
    );
    assert!(
        value
            .format_into(&mut io::sink(), &OwnedFormatItem::from(format_description))
            .is_ok()
    );

    Ok(())
}

#[rstest]
#[case(time!(0:00), "0:00:00.0")]
#[case(time!(23:59), "23:59:00.0")]
#[case(time!(23:59:59), "23:59:59.0")]
#[case(time!(0:00:01), "0:00:01.0")]
#[case(time!(0:00:00.1), "0:00:00.1")]
#[case(time!(0:00:00.01), "0:00:00.01")]
#[case(time!(0:00:00.001), "0:00:00.001")]
#[case(time!(0:00:00.000_1), "0:00:00.0001")]
#[case(time!(0:00:00.000_01), "0:00:00.00001")]
#[case(time!(0:00:00.000_001), "0:00:00.000001")]
#[case(time!(0:00:00.000_000_1), "0:00:00.0000001")]
#[case(time!(0:00:00.000_000_01), "0:00:00.00000001")]
#[case(time!(0:00:00.000_000_001), "0:00:00.000000001")]
fn display_time(#[case] t: Time, #[case] expected: &str) {
    assert_eq!(t.to_string(), expected);
}

#[rstest]
fn display_time_padding() {
    assert_eq!(format!("{:>12}", time!(0:00)), "   0:00:00.0");
    assert_eq!(format!("{:x^14}", time!(0:00)), "xx0:00:00.0xxx");
}

#[rstest]
#[case(fd!("[day]"), "31")]
#[case(fd!("[month]"), "12")]
#[case(fd!("[month repr:short]"), "Dec")]
#[case(fd!("[month repr:long]"), "December")]
#[case(fd!("[ordinal]"), "365")]
#[case(fd!("[weekday]"), "Tuesday")]
#[case(fd!("[weekday repr:short]"), "Tue")]
#[case(fd!("[weekday repr:sunday]"), "3")]
#[case(fd!("[weekday repr:sunday one_indexed:false]"), "2")]
#[case(fd!("[weekday repr:monday]"), "2")]
#[case(fd!("[weekday repr:monday one_indexed:false]"), "1")]
#[case(fd!("[week_number]"), "01")]
#[case(fd!("[week_number padding:none]"), "1")]
#[case(fd!("[week_number padding:space]"), " 1")]
#[case(fd!("[week_number repr:sunday]"), "52")]
#[case(fd!("[week_number repr:monday]"), "52")]
#[case(fd!("[year]"), "2019")]
#[case(fd!("[year base:iso_week]"), "2020")]
#[case(fd!("[year sign:mandatory]"), "+2019")]
#[case(fd!("[year base:iso_week sign:mandatory]"), "+2020")]
#[case(fd!("[year repr:century]"), "20")]
#[case(fd!("[year repr:last_two]"), "19")]
#[case(fd!("[year base:iso_week repr:last_two]"), "20")]
#[case(fd!("[year range:standard]"), "2019")]
#[case(fd!("[year range:standard repr:century]"), "20")]
#[case(fd!("[year range:standard repr:last_two]"), "19")]
fn format_date(
    #[case] format_description: &[BorrowedFormatItem<'_>],
    #[case] output: &str,
) -> time::Result<()> {
    assert_eq!(date!(2019-12-31).format(format_description)?, output);
    assert!(
        date!(2019-12-31)
            .format_into(&mut io::sink(), format_description)
            .is_ok()
    );
    assert_eq!(
        date!(2019-12-31).format(&OwnedFormatItem::from(format_description))?,
        output
    );
    assert!(
        date!(2019-12-31)
            .format_into(&mut io::sink(), &OwnedFormatItem::from(format_description))
            .is_ok()
    );

    Ok(())
}

#[rstest]
#[case(date!(+10_000-01-01), fd!("[year range:standard]"), "year")]
#[case(date!(+10_000-01-01), fd!("[year repr:century range:standard]"), "year")]
fn format_date_err(
    #[case] value: Date,
    #[case] format_description: &[BorrowedFormatItem<'_>],
    #[case] component: &str,
) {
    assert!(matches!(
        value.format(format_description),
        Err(time::error::Format::ComponentRange(cr)) if cr.name() == component
    ));
}

#[rstest]
#[case(date!(2019-01-01), "2019-01-01")]
#[case(date!(2019-12-31), "2019-12-31")]
#[case(date!(-4713-11-24), "-4713-11-24")]
#[case(date!(-0001-01-01), "-0001-01-01")]
#[case(date!(+10_000-01-01), "+10000-01-01")]
#[case(date!(+100_000-01-01), "+100000-01-01")]
#[case(date!(-10_000-01-01), "-10000-01-01")]
#[case(date!(-100_000-01-01), "-100000-01-01")]
fn display_date(#[case] d: Date, #[case] expected: &str) {
    assert_eq!(d.to_string(), expected);
}

#[rstest]
#[case(offset!(+01:02:03), fd!("[offset_hour sign:automatic]"), "01")]
#[case(offset!(+01:02:03), fd!("[offset_hour sign:mandatory]"), "+01")]
#[case(offset!(-01:02:03), fd!("[offset_hour sign:automatic]"), "-01")]
#[case(offset!(-01:02:03), fd!("[offset_hour sign:mandatory]"), "-01")]
#[case(offset!(+01:02:03), fd!("[offset_minute]"), "02")]
#[case(offset!(+01:02:03), fd!("[offset_second]"), "03")]
fn format_offset(
    #[case] value: UtcOffset,
    #[case] format_description: &[BorrowedFormatItem<'_>],
    #[case] output: &str,
) -> time::Result<()> {
    assert_eq!(value.format(format_description)?, output);
    assert!(
        value
            .format_into(&mut io::sink(), format_description)
            .is_ok()
    );
    assert_eq!(
        value.format(&OwnedFormatItem::from(format_description))?,
        output
    );
    assert!(
        value
            .format_into(&mut io::sink(), &OwnedFormatItem::from(format_description))
            .is_ok()
    );

    Ok(())
}

#[rstest]
#[case(offset!(UTC), "+00:00:00")]
#[case(offset!(+0:00:01), "+00:00:01")]
#[case(offset!(-0:00:01), "-00:00:01")]
#[case(offset!(+1), "+01:00:00")]
#[case(offset!(-1), "-01:00:00")]
#[case(offset!(+23:59), "+23:59:00")]
#[case(offset!(-23:59), "-23:59:00")]
#[case(offset!(+23:59:59), "+23:59:59")]
#[case(offset!(-23:59:59), "-23:59:59")]
fn display_offset(#[case] offset: UtcOffset, #[case] expected: &str) {
    assert_eq!(offset.to_string(), expected);
}

#[rstest]
fn display_offset_padding() {
    assert_eq!(format!("{:>10}", offset!(UTC)), " +00:00:00");
    assert_eq!(format!("{:x^14}", offset!(UTC)), "xx+00:00:00xxx");
}

#[rstest]
#[case(
    datetime!(1970-01-01 0:00),
    fd!("[year]-[month]-[day] [hour]:[minute]:[second].[subsecond]"),
    "1970-01-01 00:00:00.0",
)]
fn format_pdt(
    #[case] pdt: PlainDateTime,
    #[case] format_description: &[BorrowedFormatItem<'_>],
    #[case] expected: &str,
) -> time::Result<()> {
    assert_eq!(pdt.format(format_description)?, expected);
    assert!(pdt.format_into(&mut io::sink(), format_description).is_ok());
    assert_eq!(
        pdt.format(&OwnedFormatItem::from(format_description))?,
        expected
    );
    assert!(
        pdt.format_into(&mut io::sink(), &OwnedFormatItem::from(format_description))
            .is_ok()
    );

    Ok(())
}

#[rstest]
#[case(datetime!(1970-01-01 0:00), "1970-01-01 0:00:00.0")]
#[case(datetime!(1970-01-01 0:00:01), "1970-01-01 0:00:01.0")]
fn display_pdt(#[case] pdt: PlainDateTime, #[case] expected: &str) {
    assert_eq!(pdt.to_string(), expected);
}

#[rstest]
#[case(datetime!(1970-01-01 0:00 UTC), "1970-01-01 00:00:00.0 +00:00:00")]
fn format_odt(#[case] odt: OffsetDateTime, #[case] expected: &str) -> time::Result<()> {
    let format_description = format_description::parse_borrowed::<2>(
        "[year]-[month]-[day] [hour]:[minute]:[second].[subsecond digits:1+] [offset_hour \
         sign:mandatory]:[offset_minute]:[offset_second]",
    )?;

    assert_eq!(odt.format(&format_description)?, expected);
    assert!(
        odt.format_into(&mut io::sink(), &format_description)
            .is_ok()
    );
    assert_eq!(
        odt.format(&OwnedFormatItem::from(&format_description))?,
        expected
    );
    assert!(
        odt.format_into(&mut io::sink(), &OwnedFormatItem::from(format_description))
            .is_ok()
    );

    let format_description_v3 = format_description::parse_borrowed::<3>(
        "[year]-[month]-[day] [hour]:[minute]:[second].[subsecond digits:1+] [offset_hour \
         sign:mandatory]:[offset_minute]:[offset_second]",
    )?;

    assert_eq!(odt.format(&format_description_v3)?, expected);
    assert!(
        odt.format_into(&mut io::sink(), &format_description_v3)
            .is_ok()
    );
    let format_description_v3 = format_description_v3.to_owned();
    assert_eq!(odt.format(&format_description_v3)?, expected);
    assert!(
        odt.format_into(&mut io::sink(), &format_description_v3)
            .is_ok()
    );

    Ok(())
}

#[rstest]
#[case(datetime!(1970-01-01 0:00 UTC), "1970-01-01 0:00:00.0 +00:00:00")]
fn display_odt(#[case] odt: OffsetDateTime, #[case] expected: &str) {
    assert_eq!(odt.to_string(), expected);
}

#[rstest]
#[case(utc_datetime!(1970-01-01 0:00), "1970-01-01 00:00:00.0")]
fn format_udt(#[case] udt: UtcDateTime, #[case] expected: &str) -> time::Result<()> {
    let format_description = fd!("[year]-[month]-[day] [hour]:[minute]:[second].[subsecond]");

    assert_eq!(udt.format(format_description)?, expected);
    assert!(udt.format_into(&mut io::sink(), format_description).is_ok());
    assert_eq!(
        udt.format(&OwnedFormatItem::from(format_description))?,
        expected
    );
    assert!(
        udt.format_into(&mut io::sink(), &OwnedFormatItem::from(format_description))
            .is_ok()
    );

    Ok(())
}

#[rstest]
#[case(utc_datetime!(1970-01-01 0:00), "1970-01-01 0:00:00.0 +00")]
fn display_udt(#[case] udt: UtcDateTime, #[case] expected: &str) {
    assert_eq!(udt.to_string(), expected);
}

#[rstest]
fn insufficient_type_information() {
    let assert_insufficient_type_information = |res| {
        assert!(matches!(
            res,
            Err(time::error::Format::InsufficientTypeInformation { .. })
        ));
    };
    assert_insufficient_type_information(Time::MIDNIGHT.format(fd!("[year]")));
    assert_insufficient_type_information(Time::MIDNIGHT.format(&BorrowedFormatItem::First(&[
        BorrowedFormatItem::Compound(fd!("[year]")),
    ])));
}

#[rstest]
#[case(fd!("foo"))]
#[case(OwnedFormatItem::from(fd!("foo")))]
#[case(BorrowedFormatItem::Compound(fd!("foo")))]
#[case(
    BorrowedFormatItem::Optional(&BorrowedFormatItem::Compound(fd!("foo")))
)]
#[case(
    OwnedFormatItem::from(
        BorrowedFormatItem::Optional(&BorrowedFormatItem::Compound(fd!("foo")))
    )
)]
fn failed_write_time(#[case] format: impl Formattable) -> time::Result<()> {
    let value = Time::MIDNIGHT;
    let success_len = value.format(&format)?.len();
    for len in 0..success_len {
        let mut buf = &mut vec![0; len][..];
        let res = value.format_into(&mut buf, &format);
        assert!(matches!(
            res,
            Err(time::error::Format::StdIo(e)) if e.kind() == io::ErrorKind::WriteZero
        ));
    }

    Ok(())
}

#[rstest]
#[case(Rfc3339)]
#[case(Rfc2822)]
#[case(Iso8601::DEFAULT)]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_year_is_six_digits(true)
            .encode()
    }>,
)]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_date_kind(DateKind::Ordinal)
            .encode()
    }>,
)]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_year_is_six_digits(true)
            .set_date_kind(DateKind::Ordinal)
            .encode()
    }>,
)]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_year_is_six_digits(true)
            .set_date_kind(DateKind::Week)
            .encode()
    }>,
)]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_date_kind(DateKind::Week)
            .encode()
    }>,
)]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_time_precision(TimePrecision::Minute { decimal_digits: None })
            .encode()
    }>,
)]
#[case(
    Iso8601::<{
        iso8601::Config::DEFAULT
            .set_time_precision(TimePrecision::Hour { decimal_digits: None })
            .encode()
    }>,
)]
fn failed_write_offset(#[case] format: impl Formattable) -> time::Result<()> {
    let value = OffsetDateTime::UNIX_EPOCH;
    let success_len = value.format(&format)?.len();
    for len in 0..success_len {
        let mut buf = &mut vec![0; len][..];
        let res = value.format_into(&mut buf, &format);
        assert!(matches!(
            res,
            Err(time::error::Format::StdIo(e)) if e.kind() == io::ErrorKind::WriteZero
        ));
    }

    Ok(())
}

#[rstest]
#[case(date!(-1-001), fd!("[year]"))]
#[case(date!(2021-001), fd!("[year sign:mandatory]"))]
#[case(date!(+999_999-001), fd!("[year]"))]
#[case(date!(+99_999-001), fd!("[year]"))]
fn failed_write_date(#[case] value: Date, #[case] format: impl Formattable) -> time::Result<()> {
    let success_len = value.format(&format)?.len();
    for len in 0..success_len {
        let mut buf = &mut vec![0; len][..];
        let res = value.format_into(&mut buf, &format);
        assert!(matches!(
            res,
            Err(time::error::Format::StdIo(e)) if e.kind() == io::ErrorKind::WriteZero
        ));
    }

    Ok(())
}

#[rstest]
#[case(fd!("[day]"))]
#[case(fd!("[month]"))]
#[case(fd!("[ordinal]"))]
#[case(fd!("[weekday]"))]
#[case(fd!("[week_number]"))]
#[case(fd!("[year]"))]
#[case(fd!("[hour]"))]
#[case(fd!("[minute]"))]
#[case(fd!("[period]"))]
#[case(fd!("[second]"))]
#[case(fd!("[subsecond]"))]
#[case(fd!("[offset_hour]"))]
#[case(fd!("[offset_minute]"))]
#[case(fd!("[offset_second]"))]
fn failed_write_component(#[case] format: &[BorrowedFormatItem<'_>]) -> time::Result<()> {
    let value = OffsetDateTime::UNIX_EPOCH;
    let success_len = value.format(format)?.len();
    for len in 0..success_len {
        let mut buf = &mut vec![0; len][..];
        let res = value.format_into(&mut buf, format);
        assert!(
            matches!(res,Err(time::error::Format::StdIo(e))if e.kind() == io::ErrorKind::WriteZero)
        );
    }

    Ok(())
}

#[rstest]
#[case(BorrowedFormatItem::First(&[]), "")]
#[case(BorrowedFormatItem::First(&[BorrowedFormatItem::Compound(fd!("[hour]"))]), "00")]
fn first_borrowed(#[case] format_description: BorrowedFormatItem<'_>, #[case] expected: &str) {
    assert!(matches!(Time::MIDNIGHT.format(&format_description), Ok(e) if e == expected));
}

#[rstest]
#[case(OwnedFormatItem::First(Box::new([])), "")]
#[case(
    OwnedFormatItem::from(BorrowedFormatItem::First(&[
        BorrowedFormatItem::Compound(fd!("[hour]"))
    ])),
    "00"
)]
fn first_owned(#[case] format_description: OwnedFormatItem, #[case] expected: &str) {
    assert!(matches!(Time::MIDNIGHT.format(&format_description), Ok(e) if e == expected));
}

#[rstest]
#[case(fd!("[ignore count:2]"), "")]
fn ignore(#[case] format_description: &[BorrowedFormatItem<'_>], #[case] expected: &str) {
    assert!(matches!(Time::MIDNIGHT.format(format_description), Ok(e) if e == expected));
}

#[rstest]
#[case(fd!("[end]"), "")]
fn end(#[case] format_description: &[BorrowedFormatItem<'_>], #[case] expected: &str) {
    assert!(matches!(Time::MIDNIGHT.format(format_description), Ok(e) if e == expected));
}

#[rstest]
#[case(
    datetime!(2009-02-13 23:31:30.123456789 UTC),
    fd!("[unix_timestamp]"),
    "1234567890",
)]
#[case(
    datetime!(2009-02-13 23:31:30.123456789 UTC),
    fd!("[unix_timestamp sign:mandatory]"),
    "+1234567890",
)]
#[case(
    datetime!(2009-02-13 23:31:30.123456789 UTC),
    fd!("[unix_timestamp precision:millisecond]"),
    "1234567890123",
)]
#[case(
    datetime!(2009-02-13 23:31:30.123456789 UTC),
    fd!("[unix_timestamp precision:microsecond]"),
    "1234567890123456",
)]
#[case(
    datetime!(2009-02-13 23:31:30.123456789 UTC),
    fd!("[unix_timestamp precision:nanosecond]"),
    "1234567890123456789",
)]
#[case(
    datetime!(1969-12-31 23:59:59 UTC),
    fd!("[unix_timestamp]"),
    "-1",
)]
fn unix_timestamp(
    #[case] dt: OffsetDateTime,
    #[case] format_description: &[BorrowedFormatItem<'_>],
    #[case] expected: &str,
) -> time::Result<()> {
    assert_eq!(dt.format(format_description)?, expected);
    Ok(())
}
