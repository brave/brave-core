use std::fmt::Display;
use std::io;
use std::marker::PhantomData;

use rstest::rstest;
use time::error::{
    ComponentRange, ConversionRange, DifferentVariant, Error, Format, IndeterminateOffset,
    InvalidFormatDescription, InvalidVariant, Parse, ParseFromDescription, TryFromParsed,
};
use time::macros::format_description;
use time::parsing::Parsed;
use time::{Date, Time, format_description};

fn component_range() -> ComponentRange {
    Date::from_ordinal_date(0, 367).expect_err("367 is not a valid day")
}

fn insufficient_type_information() -> Format {
    Time::MIDNIGHT
        .format(format_description!("[year]"))
        .expect_err("missing date and UTC offset")
}

fn unexpected_trailing_characters() -> Parse {
    Time::parse("a", format_description!("")).expect_err("should fail to parse")
}

fn invalid_format_description() -> InvalidFormatDescription {
    format_description::parse_borrowed::<3>("[").expect_err("format description is invalid")
}

fn io_error() -> io::Error {
    io::Error::last_os_error()
}

fn invalid_literal() -> ParseFromDescription {
    Parsed::parse_literal(b"a", b"b").expect_err("should fail to parse")
}

#[rstest]
#[case(Parse::from(ParseFromDescription::InvalidComponent("a")))]
#[case(invalid_format_description())]
#[case(DifferentVariant)]
#[case(InvalidVariant)]
fn debug_reflexive<T>(#[case] value: T)
where
    T: std::fmt::Debug,
{
    assert_eq!(format!("{value:?}"), format!("{value:?}"))
}

#[rstest]
#[case(ConversionRange, Error::from(ConversionRange))]
#[case(component_range(), Error::from(component_range()))]
#[case(component_range(), TryFromParsed::from(component_range()))]
#[case(IndeterminateOffset, Error::from(IndeterminateOffset))]
#[case(
    TryFromParsed::InsufficientInformation,
    Error::from(TryFromParsed::InsufficientInformation)
)]
#[case(
    insufficient_type_information(),
    Error::from(insufficient_type_information())
)]
#[case(
    Format::InvalidComponent("a"),
    Error::from(Format::InvalidComponent("a"))
)]
#[case(
    ParseFromDescription::InvalidComponent("a"),
    Error::from(Parse::from(ParseFromDescription::InvalidComponent("a")))
)]
#[case(invalid_literal(), Parse::from(invalid_literal()))]
#[case(
    component_range(),
    Error::from(Parse::from(TryFromParsed::from(component_range())))
)]
#[case(
    ParseFromDescription::InvalidComponent("a"),
    Parse::from(ParseFromDescription::InvalidComponent("a"))
)]
#[case(component_range(), Parse::from(TryFromParsed::from(component_range())))]
#[case(
    unexpected_trailing_characters(),
    Error::from(unexpected_trailing_characters())
)]
#[case(
    invalid_format_description(),
    Error::from(invalid_format_description())
)]
#[case(io_error(), Format::from(io_error()))]
#[case(DifferentVariant, Error::from(DifferentVariant))]
#[case(InvalidVariant, Error::from(InvalidVariant))]
fn display_eq<T, U>(#[case] lhs: T, #[case] rhs: U)
where
    T: Display,
    U: Display,
{
    assert_eq!(lhs.to_string(), rhs.to_string());
}

#[rstest]
#[case(Error::from(ConversionRange), PhantomData::<ConversionRange>)]
#[case(Error::from(component_range()), PhantomData::<ComponentRange>)]
#[case(TryFromParsed::from(component_range()), PhantomData::<ComponentRange>)]
#[case(Error::from(insufficient_type_information()), PhantomData::<Format>)]
#[case(Error::from(IndeterminateOffset), PhantomData::<IndeterminateOffset>)]
#[case(Parse::from(TryFromParsed::InsufficientInformation), PhantomData::<TryFromParsed>)]
#[case(Error::from(TryFromParsed::InsufficientInformation), PhantomData::<TryFromParsed>)]
#[case(Parse::from(ParseFromDescription::InvalidComponent("a")), PhantomData::<ParseFromDescription>)]
#[case(Error::from(ParseFromDescription::InvalidComponent("a")), PhantomData::<ParseFromDescription>)]
#[case(unexpected_trailing_characters(), PhantomData::<ParseFromDescription>)]
#[case(Error::from(unexpected_trailing_characters()), PhantomData::<ParseFromDescription>)]
#[case(Error::from(invalid_format_description()), PhantomData::<InvalidFormatDescription>)]
#[case(Format::from(io_error()), PhantomData::<io::Error>)]
#[case(Error::from(DifferentVariant), PhantomData::<DifferentVariant>)]
#[case(Error::from(InvalidVariant), PhantomData::<InvalidVariant>)]
fn source<E, S>(#[case] err: E, #[case] _source: PhantomData<S>)
where
    E: std::error::Error + 'static,
    S: std::error::Error + 'static,
{
    assert!(
        err.source()
            .expect("error type should have source")
            .is::<S>()
    );
}

#[rstest]
#[case(TryFromParsed::InsufficientInformation)]
#[case(insufficient_type_information())]
#[case(Format::InvalidComponent("a"))]
fn source_none<T>(#[case] err: T)
where
    T: std::error::Error,
{
    assert!(err.source().is_none());
}

#[rstest]
fn component_name() {
    assert_eq!(component_range().name(), "ordinal");
}

#[rstest]
#[case(ComponentRange::try_from(Error::from(component_range())))]
#[case(ConversionRange::try_from(Error::from(ConversionRange)))]
#[case(Format::try_from(Error::from(insufficient_type_information())))]
#[case(IndeterminateOffset::try_from(Error::from(IndeterminateOffset)))]
#[case(InvalidFormatDescription::try_from(Error::from(invalid_format_description())))]
#[case(ParseFromDescription::try_from(Error::from(invalid_literal())))]
#[case(ParseFromDescription::try_from(Parse::from(invalid_literal())))]
#[case(ParseFromDescription::try_from(unexpected_trailing_characters()))]
#[case(Parse::try_from(Error::from(unexpected_trailing_characters())))]
#[case(Parse::try_from(Error::from(invalid_literal())))]
#[case(Parse::try_from(Error::from(TryFromParsed::InsufficientInformation)))]
#[case(DifferentVariant::try_from(Error::from(DifferentVariant)))]
#[case(InvalidVariant::try_from(Error::from(InvalidVariant)))]
#[case(ComponentRange::try_from(TryFromParsed::ComponentRange(component_range())))]
#[case(TryFromParsed::try_from(Error::from(TryFromParsed::InsufficientInformation)))]
#[case(TryFromParsed::try_from(Parse::from(TryFromParsed::InsufficientInformation)))]
#[case(io::Error::try_from(Format::from(io_error())))]
fn conversion<T, E>(#[case] res: Result<T, E>) {
    assert!(res.is_ok());
}

#[rstest]
#[case(ComponentRange::try_from(Error::from(IndeterminateOffset)))]
#[case(ConversionRange::try_from(Error::from(IndeterminateOffset)))]
#[case(Format::try_from(Error::from(IndeterminateOffset)))]
#[case(IndeterminateOffset::try_from(Error::from(ConversionRange)))]
#[case(InvalidFormatDescription::try_from(Error::from(IndeterminateOffset)))]
#[case(ParseFromDescription::try_from(Error::from(IndeterminateOffset)))]
#[case(Parse::try_from(Error::from(IndeterminateOffset)))]
#[case(DifferentVariant::try_from(Error::from(IndeterminateOffset)))]
#[case(InvalidVariant::try_from(Error::from(IndeterminateOffset)))]
#[case(ComponentRange::try_from(TryFromParsed::InsufficientInformation))]
#[case(TryFromParsed::try_from(Error::from(IndeterminateOffset)))]
#[case(TryFromParsed::try_from(unexpected_trailing_characters()))]
#[case(io::Error::try_from(insufficient_type_information()))]
fn conversion_err<T, E>(#[case] res: Result<T, E>) {
    assert!(res.is_err());
}
