//! Bit level parsers
//!

#[cfg(test)]
mod tests;

use crate::combinator::trace;
use crate::error::{ErrMode, ErrorConvert, ErrorKind, Needed, ParserError};
use crate::lib::std::ops::{AddAssign, Div, Shl, Shr};
use crate::stream::{Stream, StreamIsPartial, ToUsize};
use crate::{unpeek, IResult, PResult, Parser};

/// Number of bits in a byte
const BYTE: usize = u8::BITS as usize;

/// Converts a byte-level input to a bit-level input
///
/// See [`bytes`] to convert it back.
///
/// # Example
/// ```
/// use winnow::prelude::*;
/// use winnow::Bytes;
/// use winnow::binary::bits::{bits, take};
/// use winnow::error::InputError;
///
/// type Stream<'i> = &'i Bytes;
///
/// fn stream(b: &[u8]) -> Stream<'_> {
///     Bytes::new(b)
/// }
///
/// fn parse(input: Stream<'_>) -> IResult<Stream<'_>, (u8, u8)> {
///     bits::<_, _, InputError<(_, usize)>, _, _>((take(4usize), take(8usize))).parse_peek(input)
/// }
///
/// let input = stream(&[0x12, 0x34, 0xff, 0xff]);
///
/// let output = parse(input).expect("We take 1.5 bytes and the input is longer than 2 bytes");
///
/// // The first byte is consumed, the second byte is partially consumed and dropped.
/// let remaining = output.0;
/// assert_eq!(remaining, stream(&[0xff, 0xff]));
///
/// let parsed = output.1;
/// assert_eq!(parsed.0, 0x01);
/// assert_eq!(parsed.1, 0x23);
/// ```
pub fn bits<Input, Output, BitError, ByteError, ParseNext>(
    mut parser: ParseNext,
) -> impl Parser<Input, Output, ByteError>
where
    BitError: ParserError<(Input, usize)> + ErrorConvert<ByteError>,
    ByteError: ParserError<Input>,
    (Input, usize): Stream,
    Input: Stream + Clone,
    ParseNext: Parser<(Input, usize), Output, BitError>,
{
    trace(
        "bits",
        unpeek(move |input: Input| {
            match parser.parse_peek((input, 0)) {
                Ok(((rest, offset), result)) => {
                    // If the next byte has been partially read, it will be sliced away as well.
                    // The parser functions might already slice away all fully read bytes.
                    // That's why `offset / BYTE` isn't necessarily needed at all times.
                    let remaining_bytes_index =
                        offset / BYTE + if offset % BYTE == 0 { 0 } else { 1 };
                    let (input, _) = rest.peek_slice(remaining_bytes_index);
                    Ok((input, result))
                }
                Err(ErrMode::Incomplete(n)) => {
                    Err(ErrMode::Incomplete(n.map(|u| u.get() / BYTE + 1)))
                }
                Err(e) => Err(e.convert()),
            }
        }),
    )
}

/// Convert a [`bits`] stream back into a byte stream
///
/// **Warning:** A partial byte remaining in the input will be ignored and the given parser will
/// start parsing at the next full byte.
///
/// ```
/// use winnow::prelude::*;
/// use winnow::Bytes;
/// use winnow::binary::bits::{bits, bytes, take};
/// use winnow::combinator::rest;
/// use winnow::error::InputError;
///
/// type Stream<'i> = &'i Bytes;
///
/// fn stream(b: &[u8]) -> Stream<'_> {
///     Bytes::new(b)
/// }
///
/// fn parse(input: Stream<'_>) -> IResult<Stream<'_>, (u8, u8, &[u8])> {
///   bits::<_, _, InputError<(_, usize)>, _, _>((
///     take(4usize),
///     take(8usize),
///     bytes::<_, _, InputError<_>, _, _>(rest)
///   )).parse_peek(input)
/// }
///
/// let input = stream(&[0x12, 0x34, 0xff, 0xff]);
///
/// assert_eq!(parse(input), Ok(( stream(&[]), (0x01, 0x23, &[0xff, 0xff][..]) )));
/// ```
pub fn bytes<Input, Output, ByteError, BitError, ParseNext>(
    mut parser: ParseNext,
) -> impl Parser<(Input, usize), Output, BitError>
where
    ByteError: ParserError<Input> + ErrorConvert<BitError>,
    BitError: ParserError<(Input, usize)>,
    Input: Stream<Token = u8> + Clone,
    ParseNext: Parser<Input, Output, ByteError>,
{
    trace(
        "bytes",
        unpeek(move |(input, offset): (Input, usize)| {
            let (inner, _) = if offset % BYTE != 0 {
                input.peek_slice(1 + offset / BYTE)
            } else {
                input.peek_slice(offset / BYTE)
            };
            let i = (input, offset);
            match parser.parse_peek(inner) {
                Ok((rest, res)) => Ok(((rest, 0), res)),
                Err(ErrMode::Incomplete(Needed::Unknown)) => {
                    Err(ErrMode::Incomplete(Needed::Unknown))
                }
                Err(ErrMode::Incomplete(Needed::Size(sz))) => {
                    Err(match sz.get().checked_mul(BYTE) {
                        Some(v) => ErrMode::Incomplete(Needed::new(v)),
                        None => ErrMode::Cut(BitError::assert(
                            &i,
                            "overflow in turning needed bytes into needed bits",
                        )),
                    })
                }
                Err(e) => Err(e.convert()),
            }
        }),
    )
}

/// Parse taking `count` bits
///
/// # Effective Signature
///
/// Assuming you are parsing a `(&[u8], usize)` bit [Stream]:
/// ```rust
/// # use winnow::prelude::*;;
/// # use winnow::error::ContextError;
/// pub fn take<'i>(count: usize) -> impl Parser<(&'i [u8], usize), u8, ContextError>
/// # {
/// #     winnow::binary::bits::take(count)
/// # }
/// ```
///
/// # Example
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::Bytes;
/// # use winnow::error::{InputError, ErrorKind};
/// use winnow::binary::bits::take;
///
/// type Stream<'i> = &'i Bytes;
///
/// fn stream(b: &[u8]) -> Stream<'_> {
///     Bytes::new(b)
/// }
///
/// fn parser(input: (Stream<'_>, usize), count: usize)-> IResult<(Stream<'_>, usize), u8> {
///   take(count).parse_peek(input)
/// }
///
/// // Consumes 0 bits, returns 0
/// assert_eq!(parser((stream(&[0b00010010]), 0), 0), Ok(((stream(&[0b00010010]), 0), 0)));
///
/// // Consumes 4 bits, returns their values and increase offset to 4
/// assert_eq!(parser((stream(&[0b00010010]), 0), 4), Ok(((stream(&[0b00010010]), 4), 0b00000001)));
///
/// // Consumes 4 bits, offset is 4, returns their values and increase offset to 0 of next byte
/// assert_eq!(parser((stream(&[0b00010010]), 4), 4), Ok(((stream(&[]), 0), 0b00000010)));
///
/// // Tries to consume 12 bits but only 8 are available
/// assert_eq!(parser((stream(&[0b00010010]), 0), 12), Err(winnow::error::ErrMode::Backtrack(InputError::new((stream(&[0b00010010]), 0), ErrorKind::Eof))));
/// ```
#[inline(always)]
pub fn take<Input, Output, Count, Error>(count: Count) -> impl Parser<(Input, usize), Output, Error>
where
    Input: Stream<Token = u8> + StreamIsPartial + Clone,
    Output: From<u8> + AddAssign + Shl<usize, Output = Output> + Shr<usize, Output = Output>,
    Count: ToUsize,
    Error: ParserError<(Input, usize)>,
{
    let count = count.to_usize();
    trace(
        "take",
        unpeek(move |input: (Input, usize)| {
            if <Input as StreamIsPartial>::is_partial_supported() {
                take_::<_, _, _, true>(input, count)
            } else {
                take_::<_, _, _, false>(input, count)
            }
        }),
    )
}

fn take_<I, O, E: ParserError<(I, usize)>, const PARTIAL: bool>(
    (input, bit_offset): (I, usize),
    count: usize,
) -> IResult<(I, usize), O, E>
where
    I: StreamIsPartial,
    I: Stream<Token = u8> + Clone,
    O: From<u8> + AddAssign + Shl<usize, Output = O> + Shr<usize, Output = O>,
{
    if count == 0 {
        Ok(((input, bit_offset), 0u8.into()))
    } else {
        if input.eof_offset() * BYTE < count + bit_offset {
            if PARTIAL && input.is_partial() {
                Err(ErrMode::Incomplete(Needed::new(count)))
            } else {
                Err(ErrMode::from_error_kind(
                    &(input, bit_offset),
                    ErrorKind::Eof,
                ))
            }
        } else {
            let cnt = (count + bit_offset).div(BYTE);
            let mut acc: O = 0_u8.into();
            let mut offset: usize = bit_offset;
            let mut remaining: usize = count;
            let mut end_offset: usize = 0;

            for (_, byte) in input.iter_offsets().take(cnt + 1) {
                if remaining == 0 {
                    break;
                }
                let val: O = if offset == 0 {
                    byte.into()
                } else {
                    (byte << offset >> offset).into()
                };

                if remaining < BYTE - offset {
                    acc += val >> (BYTE - offset - remaining);
                    end_offset = remaining + offset;
                    break;
                } else {
                    acc += val << (remaining - (BYTE - offset));
                    remaining -= BYTE - offset;
                    offset = 0;
                }
            }
            let (input, _) = input.peek_slice(cnt);
            Ok(((input, end_offset), acc))
        }
    }
}

/// Parse taking `count` bits and comparing them to `pattern`
///
/// # Effective Signature
///
/// Assuming you are parsing a `(&[u8], usize)` bit [Stream]:
/// ```rust
/// # use winnow::prelude::*;;
/// # use winnow::error::ContextError;
/// pub fn pattern<'i>(pattern: u8, count: usize) -> impl Parser<(&'i [u8], usize), u8, ContextError>
/// # {
/// #     winnow::binary::bits::pattern(pattern, count)
/// # }
/// ```
///
/// # Example
///
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::Bytes;
/// # use winnow::error::{InputError, ErrorKind};
/// use winnow::binary::bits::pattern;
///
/// type Stream<'i> = &'i Bytes;
///
/// fn stream(b: &[u8]) -> Stream<'_> {
///     Bytes::new(b)
/// }
///
/// /// Compare the lowest `count` bits of `input` against the lowest `count` bits of `pattern`.
/// /// Return Ok and the matching section of `input` if there's a match.
/// /// Return Err if there's no match.
/// fn parser(bits: u8, count: u8, input: (Stream<'_>, usize)) -> IResult<(Stream<'_>, usize), u8> {
///     pattern(bits, count).parse_peek(input)
/// }
///
/// // The lowest 4 bits of 0b00001111 match the lowest 4 bits of 0b11111111.
/// assert_eq!(
///     parser(0b0000_1111, 4, (stream(&[0b1111_1111]), 0)),
///     Ok(((stream(&[0b1111_1111]), 4), 0b0000_1111))
/// );
///
/// // The lowest bit of 0b00001111 matches the lowest bit of 0b11111111 (both are 1).
/// assert_eq!(
///     parser(0b00000001, 1, (stream(&[0b11111111]), 0)),
///     Ok(((stream(&[0b11111111]), 1), 0b00000001))
/// );
///
/// // The lowest 2 bits of 0b11111111 and 0b00000001 are different.
/// assert_eq!(
///     parser(0b000000_01, 2, (stream(&[0b111111_11]), 0)),
///     Err(winnow::error::ErrMode::Backtrack(InputError::new(
///         (stream(&[0b11111111]), 0),
///         ErrorKind::Tag
///     )))
/// );
///
/// // The lowest 8 bits of 0b11111111 and 0b11111110 are different.
/// assert_eq!(
///     parser(0b11111110, 8, (stream(&[0b11111111]), 0)),
///     Err(winnow::error::ErrMode::Backtrack(InputError::new(
///         (stream(&[0b11111111]), 0),
///         ErrorKind::Tag
///     )))
/// );
/// ```
#[inline(always)]
#[doc(alias = "literal")]
#[doc(alias = "just")]
#[doc(alias = "tag")]
pub fn pattern<Input, Output, Count, Error: ParserError<(Input, usize)>>(
    pattern: Output,
    count: Count,
) -> impl Parser<(Input, usize), Output, Error>
where
    Input: Stream<Token = u8> + StreamIsPartial + Clone,
    Count: ToUsize,
    Output: From<u8>
        + AddAssign
        + Shl<usize, Output = Output>
        + Shr<usize, Output = Output>
        + PartialEq,
{
    let count = count.to_usize();
    trace("pattern", move |input: &mut (Input, usize)| {
        let start = input.checkpoint();

        take(count).parse_next(input).and_then(|o| {
            if pattern == o {
                Ok(o)
            } else {
                input.reset(&start);
                Err(ErrMode::Backtrack(Error::from_error_kind(
                    input,
                    ErrorKind::Tag,
                )))
            }
        })
    })
}

/// Parses one specific bit as a bool.
///
/// # Effective Signature
///
/// Assuming you are parsing a `(&[u8], usize)` bit [Stream]:
/// ```rust
/// # use winnow::prelude::*;;
/// # use winnow::error::ContextError;
/// pub fn bool(input: &mut (&[u8], usize)) -> PResult<bool>
/// # {
/// #     winnow::binary::bits::bool.parse_next(input)
/// # }
/// ```
///
/// # Example
///
/// ```rust
/// # use winnow::prelude::*;
/// # use winnow::Bytes;
/// # use winnow::error::{InputError, ErrorKind};
/// use winnow::binary::bits::bool;
///
/// type Stream<'i> = &'i Bytes;
///
/// fn stream(b: &[u8]) -> Stream<'_> {
///     Bytes::new(b)
/// }
///
/// fn parse(input: (Stream<'_>, usize)) -> IResult<(Stream<'_>, usize), bool> {
///     bool.parse_peek(input)
/// }
///
/// assert_eq!(parse((stream(&[0b10000000]), 0)), Ok(((stream(&[0b10000000]), 1), true)));
/// assert_eq!(parse((stream(&[0b10000000]), 1)), Ok(((stream(&[0b10000000]), 2), false)));
/// ```
#[doc(alias = "any")]
pub fn bool<Input, Error: ParserError<(Input, usize)>>(
    input: &mut (Input, usize),
) -> PResult<bool, Error>
where
    Input: Stream<Token = u8> + StreamIsPartial + Clone,
{
    trace("bool", |input: &mut (Input, usize)| {
        let bit: u32 = take(1usize).parse_next(input)?;
        Ok(bit != 0)
    })
    .parse_next(input)
}
