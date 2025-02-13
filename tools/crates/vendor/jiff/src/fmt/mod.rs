/*!
Configurable support for printing and parsing datetimes and durations.

Note that for most use cases, you should be using the corresponding
[`Display`](std::fmt::Display) or [`FromStr`](std::str::FromStr) trait
implementations for printing and parsing respectively. The APIs in this module
provide more configurable support for printing and parsing.
*/

use crate::{
    error::{err, Error},
    util::escape,
};

use self::util::{Decimal, DecimalFormatter, Fractional, FractionalFormatter};

pub mod friendly;
mod offset;
pub mod rfc2822;
mod rfc9557;
#[cfg(feature = "serde")]
pub mod serde;
pub mod strtime;
pub mod temporal;
mod util;

/// The result of parsing a value out of a slice of bytes.
///
/// This contains both the parsed value and the offset at which the value
/// ended in the input given. This makes it possible to parse, for example, a
/// datetime value as a prefix of some larger string without knowing ahead of
/// time where it ends.
#[derive(Clone)]
pub(crate) struct Parsed<'i, V> {
    /// The value parsed.
    value: V,
    /// The remaining unparsed input.
    input: &'i [u8],
}

impl<'i, V: core::fmt::Display> Parsed<'i, V> {
    /// Ensures that the parsed value represents the entire input. This occurs
    /// precisely when the `input` on this parsed value is empty.
    ///
    /// This is useful when one expects a parsed value to consume the entire
    /// input, and to consider it an error if it doesn't.
    #[inline]
    fn into_full(self) -> Result<V, Error> {
        if self.input.is_empty() {
            return Ok(self.value);
        }
        Err(err!(
            "parsed value '{value}', but unparsed input {unparsed:?} \
             remains (expected no unparsed input)",
            value = self.value,
            unparsed = escape::Bytes(self.input),
        ))
    }
}

impl<'i, V: core::fmt::Debug> core::fmt::Debug for Parsed<'i, V> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        f.debug_struct("Parsed")
            .field("value", &self.value)
            .field("input", &escape::Bytes(self.input))
            .finish()
    }
}

/// A trait for printing datetimes or spans into Unicode-accepting buffers or
/// streams.
///
/// The most useful implementations of this trait are for the `String` and
/// `Vec<u8>` types. But any implementation of [`std::fmt::Write`] and
/// [`std::io::Write`] can be used via the [`StdFmtWrite`] and [`StdIoWrite`]
/// adapters, respectively.
///
/// Most users of Jiff should not need to interact with this trait directly.
/// Instead, printing is handled via the [`Display`](std::fmt::Display)
/// implementation of the relevant type.
///
/// # Design
///
/// This trait is a near-clone of the `std::fmt::Write` trait. It's also very
/// similar to the `std::io::Write` trait, but like `std::fmt::Write`, this
/// trait is limited to writing valid UTF-8. The UTF-8 restriction was adopted
/// because we really want to support printing datetimes and spans to `String`
/// buffers. If we permitted writing `&[u8]` data, then writing to a `String`
/// buffer would always require a costly UTF-8 validation check.
///
/// The `std::fmt::Write` trait wasn't used itself because:
///
/// 1. Using a custom trait allows us to require using Jiff's error type.
/// (Although this extra flexibility isn't currently used, since printing only
/// fails when writing to the underlying buffer or stream fails.)
/// 2. Using a custom trait allows us more control over the implementations of
/// the trait. For example, a custom trait means we can format directly into
/// a `Vec<u8>` buffer, which isn't possible with `std::fmt::Write` because
/// there is no `std::fmt::Write` trait implementation for `Vec<u8>`.
pub trait Write {
    /// Write the given string to this writer, returning whether the write
    /// succeeded or not.
    fn write_str(&mut self, string: &str) -> Result<(), Error>;

    /// Write the given character to this writer, returning whether the write
    /// succeeded or not.
    #[inline]
    fn write_char(&mut self, char: char) -> Result<(), Error> {
        self.write_str(char.encode_utf8(&mut [0; 4]))
    }
}

#[cfg(any(test, feature = "alloc"))]
impl Write for alloc::string::String {
    #[inline]
    fn write_str(&mut self, string: &str) -> Result<(), Error> {
        self.push_str(string);
        Ok(())
    }
}

#[cfg(any(test, feature = "alloc"))]
impl Write for alloc::vec::Vec<u8> {
    #[inline]
    fn write_str(&mut self, string: &str) -> Result<(), Error> {
        self.extend_from_slice(string.as_bytes());
        Ok(())
    }
}

impl<W: Write> Write for &mut W {
    fn write_str(&mut self, string: &str) -> Result<(), Error> {
        (**self).write_str(string)
    }

    #[inline]
    fn write_char(&mut self, char: char) -> Result<(), Error> {
        (**self).write_char(char)
    }
}

/// An adapter for using `std::io::Write` implementations with `fmt::Write`.
///
/// This is useful when one wants to format a datetime or span directly
/// to something with a `std::io::Write` trait implementation but not a
/// `fmt::Write` implementation.
///
/// # Example
///
/// ```no_run
/// use std::{fs::File, io::{BufWriter, Write}, path::Path};
///
/// use jiff::{civil::date, fmt::{StdIoWrite, temporal::DateTimePrinter}};
///
/// let zdt = date(2024, 6, 15).at(7, 0, 0, 0).in_tz("America/New_York")?;
///
/// let path = Path::new("/tmp/output");
/// let mut file = BufWriter::new(File::create(path)?);
/// DateTimePrinter::new().print_zoned(&zdt, StdIoWrite(&mut file)).unwrap();
/// file.flush()?;
/// assert_eq!(
///     std::fs::read_to_string(path)?,
///     "2024-06-15T07:00:00-04:00[America/New_York]",
/// );
///
/// # Ok::<(), Box<dyn std::error::Error>>(())
/// ```
#[cfg(feature = "std")]
#[derive(Clone, Debug)]
pub struct StdIoWrite<W>(pub W);

#[cfg(feature = "std")]
impl<W: std::io::Write> Write for StdIoWrite<W> {
    #[inline]
    fn write_str(&mut self, string: &str) -> Result<(), Error> {
        self.0.write_all(string.as_bytes()).map_err(Error::adhoc)
    }
}

/// An adapter for using `std::fmt::Write` implementations with `fmt::Write`.
///
/// This is useful when one wants to format a datetime or span directly
/// to something with a `std::fmt::Write` trait implementation but not a
/// `fmt::Write` implementation.
///
/// (Despite using `Std` in this name, this type is available in `core`-only
/// configurations.)
///
/// # Example
///
/// This example shows the `std::fmt::Display` trait implementation for
/// [`civil::DateTime`](crate::civil::DateTime) (but using a wrapper type).
///
/// ```
/// use jiff::{civil::DateTime, fmt::{temporal::DateTimePrinter, StdFmtWrite}};
///
/// struct MyDateTime(DateTime);
///
/// impl std::fmt::Display for MyDateTime {
///     fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
///
///         static P: DateTimePrinter = DateTimePrinter::new();
///         P.print_datetime(&self.0, StdFmtWrite(f))
///             .map_err(|_| std::fmt::Error)
///     }
/// }
///
/// let dt = MyDateTime(DateTime::constant(2024, 6, 15, 17, 30, 0, 0));
/// assert_eq!(dt.to_string(), "2024-06-15T17:30:00");
/// ```
#[derive(Clone, Debug)]
pub struct StdFmtWrite<W>(pub W);

impl<W: core::fmt::Write> Write for StdFmtWrite<W> {
    #[inline]
    fn write_str(&mut self, string: &str) -> Result<(), Error> {
        self.0
            .write_str(string)
            .map_err(|_| err!("an error occurred when formatting an argument"))
    }
}

impl<W: Write> core::fmt::Write for StdFmtWrite<W> {
    #[inline]
    fn write_str(&mut self, string: &str) -> Result<(), core::fmt::Error> {
        self.0.write_str(string).map_err(|_| core::fmt::Error)
    }
}

/// An extension trait to `Write` that provides crate internal routines.
///
/// These routines aren't exposed because they make use of crate internal
/// types. Those types could perhaps be exposed if there was strong demand,
/// but I'm skeptical.
trait WriteExt: Write {
    /// Write the given number as a decimal using ASCII digits to this buffer.
    /// The given formatter controls how the decimal is formatted.
    #[inline]
    fn write_int(
        &mut self,
        formatter: &DecimalFormatter,
        n: impl Into<i64>,
    ) -> Result<(), Error> {
        self.write_decimal(&Decimal::new(formatter, n.into()))
    }

    /// Write the given fractional number using ASCII digits to this buffer.
    /// The given formatter controls how the fractional number is formatted.
    #[inline]
    fn write_fraction(
        &mut self,
        formatter: &FractionalFormatter,
        n: impl Into<i64>,
    ) -> Result<(), Error> {
        self.write_fractional(&Fractional::new(formatter, n.into()))
    }

    /// Write the given decimal number to this buffer.
    #[inline]
    fn write_decimal(&mut self, decimal: &Decimal) -> Result<(), Error> {
        self.write_str(decimal.as_str())
    }

    /// Write the given fractional number to this buffer.
    #[inline]
    fn write_fractional(
        &mut self,
        fractional: &Fractional,
    ) -> Result<(), Error> {
        self.write_str(fractional.as_str())
    }
}

impl<W: Write> WriteExt for W {}
