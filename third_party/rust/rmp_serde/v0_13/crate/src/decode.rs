use std::error;
use std::fmt::{self, Display, Formatter};
use std::io::{self, Cursor};
use std::str::{self, Utf8Error};

use byteorder::{self, ReadBytesExt};

use serde;
use serde::de::{self, Deserialize, DeserializeOwned, DeserializeSeed, Visitor};

use rmp;
use rmp::Marker;
use rmp::decode::{MarkerReadError, DecodeStringError, ValueReadError, NumValueReadError,
                  read_array_len};

/// Enum representing errors that can occur while decoding MessagePack data.
#[derive(Debug)]
pub enum Error {
    /// The enclosed I/O error occured while trying to read a MessagePack
    /// marker.
    InvalidMarkerRead(io::Error),
    /// The enclosed I/O error occured while trying to read the encoded
    /// MessagePack data.
    InvalidDataRead(io::Error),
    /// A mismatch occured between the decoded and expected value types.
    TypeMismatch(Marker),
    /// A numeric cast failed due to an out-of-range error.
    OutOfRange,
    /// A decoded array did not have the enclosed expected length.
    LengthMismatch(u32),
    /// An otherwise uncategorized error occured. See the enclosed `String` for
    /// details.
    Uncategorized(String),
    /// A general error occured while deserializing the expected type. See the
    /// enclosed `String` for details.
    Syntax(String),
    /// An encoded string could not be parsed as UTF-8.
    Utf8Error(Utf8Error),
    /// The depth limit was exceeded; not currently used.
    DepthLimitExceeded,
}

impl error::Error for Error {
    fn description(&self) -> &str {
        "error while decoding value"
    }

    fn cause(&self) -> Option<&error::Error> {
        match *self {
            Error::TypeMismatch(..) => None,
            Error::InvalidMarkerRead(ref err) => Some(err),
            Error::InvalidDataRead(ref err) => Some(err),
            Error::LengthMismatch(..) => None,
            Error::OutOfRange => None,
            Error::Uncategorized(..) => None,
            Error::Syntax(..) => None,
            Error::Utf8Error(ref err) => Some(err),
            Error::DepthLimitExceeded => None,
        }
    }
}

impl de::Error for Error {
    fn custom<T: Display>(msg: T) -> Self {
        Error::Syntax(format!("{}", msg))
    }
}

impl Display for Error {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), fmt::Error> {
        error::Error::description(self).fmt(fmt)
    }
}

impl From<MarkerReadError> for Error {
    fn from(err: MarkerReadError) -> Error {
        Error::InvalidMarkerRead(err.0)
    }
}

impl From<Utf8Error> for Error {
    fn from(err: Utf8Error) -> Error {
        Error::Utf8Error(err)
    }
}

impl From<ValueReadError> for Error {
    fn from(err: ValueReadError) -> Error {
        match err {
            ValueReadError::TypeMismatch(marker)   => Error::TypeMismatch(marker),
            ValueReadError::InvalidMarkerRead(err) => Error::InvalidMarkerRead(err),
            ValueReadError::InvalidDataRead(err)   => Error::InvalidDataRead(err),
        }
    }
}

impl From<NumValueReadError> for Error {
    fn from(err: NumValueReadError) -> Error {
        match err {
            NumValueReadError::TypeMismatch(marker)   => Error::TypeMismatch(marker),
            NumValueReadError::InvalidMarkerRead(err) => Error::InvalidMarkerRead(err),
            NumValueReadError::InvalidDataRead(err)   => Error::InvalidDataRead(err),
            NumValueReadError::OutOfRange => Error::OutOfRange,
        }
    }
}

impl<'a> From<DecodeStringError<'a>> for Error {
    fn from(err: DecodeStringError) -> Error {
        match err {
            DecodeStringError::InvalidMarkerRead(err) => Error::InvalidMarkerRead(err),
            DecodeStringError::InvalidDataRead(..) => Error::Uncategorized("InvalidDataRead".to_string()),
            DecodeStringError::TypeMismatch(..) => Error::Uncategorized("TypeMismatch".to_string()),
            DecodeStringError::BufferSizeTooSmall(..) => Error::Uncategorized("BufferSizeTooSmall".to_string()),
            DecodeStringError::InvalidUtf8(..) => Error::Uncategorized("InvalidUtf8".to_string()),
        }
    }
}

/// A Deserializer that reads bytes from a buffer.
///
/// # Note
///
/// All instances of `ErrorKind::Interrupted` are handled by this function and the underlying
/// operation is retried.
pub struct Deserializer<R> {
    rd: R,
    marker: Option<Marker>,
    depth: usize,
}

impl<'de> Deserializer<SliceReader<'de>> {
    pub fn from_slice(slice: &'de [u8]) -> Self {
        Deserializer {
            rd: SliceReader::new(slice),
            marker: None,
            depth: 1024,
        }
    }

    /// Gets a reference to the underlying reader in this decoder.
    pub fn get_ref(&self) -> &[u8] {
        self.rd.inner
    }
}

impl<R: io::Read> Deserializer<ReadReader<R>> {
    pub fn from_read(rd: R) -> Self {
        Deserializer {
            rd: ReadReader::new(rd),
            // Cached marker in case of deserializing options.
            marker: None,
            depth: 1024,
        }
    }

    /// Constructs a new deserializer by consuming the given reader.
    pub fn new(rd: R) -> Self {
        Self::from_read(rd)
    }

    /// Gets a reference to the underlying reader in this decoder.
    pub fn get_ref(&self) -> &R {
        &self.rd.inner
    }

    /// Gets a mutable reference to the underlying reader in this decoder.
    pub fn get_mut(&mut self) -> &mut R {
        &mut self.rd.inner
    }

    /// Consumes this decoder returning the underlying reader.
    pub fn into_inner(self) -> R {
        self.rd.inner
    }
}

impl<R: AsRef<[u8]>> Deserializer<ReadReader<Cursor<R>>> {
    /// Returns the current position of this deserializer, i.e. how many bytes were read.
    pub fn position(&self) -> u64 {
        self.rd.inner.position()
    }
}

impl<'de, R: Read<'de>> Deserializer<R> {
    /// Changes the maximum nesting depth that is allowed
    pub fn set_max_depth(&mut self, depth: usize) {
        self.depth = depth;
    }

    fn read_str_data<V>(&mut self, len: u32, visitor: V) -> Result<V::Value, Error>
        where V: Visitor<'de>
    {
        match self.read_bin_data(len as u32)? {
            Reference::Borrowed(buf) => {
                match str::from_utf8(buf) {
                    Ok(s) => visitor.visit_borrowed_str(s),
                    Err(err) => {
                        // Allow to unpack invalid UTF-8 bytes into a byte array.
                        match visitor.visit_borrowed_bytes::<Error>(buf) {
                            Ok(buf) => Ok(buf),
                            Err(..) => Err(Error::Utf8Error(err)),
                        }
                    }
                }
            }
            Reference::Copied(buf) => {
                match str::from_utf8(buf) {
                    Ok(s) => visitor.visit_str(s),
                    Err(err) => {
                        // Allow to unpack invalid UTF-8 bytes into a byte array.
                        match visitor.visit_bytes::<Error>(buf) {
                            Ok(buf) => Ok(buf),
                            Err(..) => Err(Error::Utf8Error(err)),
                        }
                    }
                }
            }
        }
    }

    fn read_bin_data<'a>(&'a mut self, len: u32) -> Result<Reference<'de,'a, [u8]>, Error> {
        self.rd.read_slice(len as usize).map_err(Error::InvalidDataRead)
    }

    fn read_array<V>(&mut self, len: u32, visitor: V) -> Result<V::Value, Error>
        where V: Visitor<'de>
    {
        visitor.visit_seq(SeqAccess::new(self, len as usize))
    }

    fn read_map<V>(&mut self, len: u32, visitor: V) -> Result<V::Value, Error>
        where V: Visitor<'de>
    {
        visitor.visit_map(MapAccess::new(self, len as usize))
    }

    fn read_bytes<V>(&mut self, len: u32, visitor: V) -> Result<V::Value, Error>
        where V: Visitor<'de>
    {
        match self.read_bin_data(len)? {
            Reference::Borrowed(buf) => visitor.visit_borrowed_bytes(buf),
            Reference::Copied(buf) => visitor.visit_bytes(buf),
        }
    }
}

fn read_u8<'de, R: Read<'de>>(rd: &mut R) -> Result<u8, Error> {
    rd.read_u8().map_err(Error::InvalidDataRead)
}

fn read_u16<'de, R: Read<'de>>(rd: &mut R) -> Result<u16, Error> {
    rd.read_u16::<byteorder::BigEndian>().map_err(Error::InvalidDataRead)
}

fn read_u32<'de, R: Read<'de>>(rd: &mut R) -> Result<u32, Error> {
    rd.read_u32::<byteorder::BigEndian>().map_err(Error::InvalidDataRead)
}

impl<'de, 'a, R: Read<'de>> serde::Deserializer<'de> for &'a mut Deserializer<R> {
    type Error = Error;

    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Self::Error>
        where V: Visitor<'de>
    {
        let marker = match self.marker.take() {
            Some(marker) => marker,
            None => rmp::decode::read_marker(&mut self.rd)?,
        };

        match marker {
            Marker::Null => visitor.visit_unit(),
            Marker::True => visitor.visit_bool(true),
            Marker::False => visitor.visit_bool(false),
            Marker::FixPos(val) => visitor.visit_u8(val),
            Marker::FixNeg(val) => visitor.visit_i8(val),
            Marker::U8 => visitor.visit_u8(rmp::decode::read_data_u8(&mut self.rd)?),
            Marker::U16 => visitor.visit_u16(rmp::decode::read_data_u16(&mut self.rd)?),
            Marker::U32 => visitor.visit_u32(rmp::decode::read_data_u32(&mut self.rd)?),
            Marker::U64 => visitor.visit_u64(rmp::decode::read_data_u64(&mut self.rd)?),
            Marker::I8 => visitor.visit_i8(rmp::decode::read_data_i8(&mut self.rd)?),
            Marker::I16 => visitor.visit_i16(rmp::decode::read_data_i16(&mut self.rd)?),
            Marker::I32 => visitor.visit_i32(rmp::decode::read_data_i32(&mut self.rd)?),
            Marker::I64 => visitor.visit_i64(rmp::decode::read_data_i64(&mut self.rd)?),
            Marker::F32 => visitor.visit_f32(rmp::decode::read_data_f32(&mut self.rd)?),
            Marker::F64 => visitor.visit_f64(rmp::decode::read_data_f64(&mut self.rd)?),
            Marker::FixStr(len) => {
                self.read_str_data(len as u32, visitor)
            }
            Marker::Str8 => {
                let len = read_u8(&mut self.rd)?;
                self.read_str_data(len as u32, visitor)
            }
            Marker::Str16 => {
                let len = read_u16(&mut self.rd)?;
                self.read_str_data(len as u32, visitor)
            }
            Marker::Str32 => {
                let len = read_u32(&mut self.rd)?;
                self.read_str_data(len as u32, visitor)
            }
            Marker::FixArray(len) => {
                self.read_array(len as u32, visitor)
            }
            Marker::Array16 => {
                let len = read_u16(&mut self.rd)?;
                self.read_array(len as u32, visitor)
            }
            Marker::Array32 => {
                let len = read_u32(&mut self.rd)?;
                self.read_array(len, visitor)
            }
            Marker::FixMap(len) => {
                self.read_map(len as u32, visitor)
            }
            Marker::Map16 => {
                let len = read_u16(&mut self.rd)?;
                self.read_map(len as u32, visitor)
            }
            Marker::Map32 => {
                let len = read_u32(&mut self.rd)?;
                self.read_map(len, visitor)
            }
            Marker::Bin8 => {
                let len = read_u8(&mut self.rd)?;
                self.read_bytes(len as u32, visitor)
            }
            Marker::Bin16 => {
                let len = read_u16(&mut self.rd)?;
                self.read_bytes(len as u32, visitor)
            }
            Marker::Bin32 => {
                let len = read_u32(&mut self.rd)?;
                self.read_bytes(len, visitor)
            }
            Marker::Reserved => Err(Error::TypeMismatch(Marker::Reserved)),
            // TODO: Make something with exts.
            marker => Err(Error::TypeMismatch(marker)),
        }
    }

    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, Self::Error>
        where V: Visitor<'de>
    {
        let marker = rmp::decode::read_marker(&mut self.rd)?;

        if marker == Marker::Null {
            visitor.visit_none()
        } else {
            self.marker = Some(marker);
            visitor.visit_some(self)
        }
    }

    fn deserialize_enum<V>(self, _name: &str, _variants: &[&str], visitor: V) -> Result<V::Value, Error>
        where V: Visitor<'de>
    {
        match read_array_len(&mut self.rd)? {
            2 => visitor.visit_enum(VariantAccess::new(self)),
            n => Err(Error::LengthMismatch(n as u32)),
        }
    }

    fn deserialize_newtype_struct<V>(self, _name: &'static str, visitor: V) -> Result<V::Value, Error>
        where V: Visitor<'de>
    {
        match read_array_len(&mut self.rd)? {
            1 => visitor.visit_newtype_struct(self),
            n => Err(Error::LengthMismatch(n as u32)),
        }
    }

    forward_to_deserialize_any! {
        bool u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 char
        str string bytes byte_buf unit unit_struct seq map
        tuple_struct struct identifier tuple
        ignored_any
    }
}

struct SeqAccess<'a, R: 'a> {
    de: &'a mut Deserializer<R>,
    left: usize,
}

impl<'a, R: 'a> SeqAccess<'a, R> {
    fn new(de: &'a mut Deserializer<R>, len: usize) -> Self {
        SeqAccess {
            de: de,
            left: len,
        }
    }
}

impl<'de, 'a, R: Read<'de> + 'a> de::SeqAccess<'de> for SeqAccess<'a, R> {
    type Error = Error;

    fn next_element_seed<T>(&mut self, seed: T) -> Result<Option<T::Value>, Self::Error>
        where T: DeserializeSeed<'de>
    {
        if self.left > 0 {
            self.left -= 1;
            Ok(Some(seed.deserialize(&mut *self.de)?))
        } else {
            Ok(None)
        }
    }

    fn size_hint(&self) -> Option<usize> {
        Some(self.left)
    }
}

struct MapAccess<'a, R: 'a> {
    de: &'a mut Deserializer<R>,
    left: usize,
}

impl<'a, R: 'a> MapAccess<'a, R> {
    fn new(de: &'a mut Deserializer<R>, len: usize) -> Self {
        MapAccess {
            de: de,
            left: len,
        }
    }
}

impl<'de, 'a, R: Read<'de> + 'a> de::MapAccess<'de> for MapAccess<'a, R> {
    type Error = Error;

    fn next_key_seed<K>(&mut self, seed: K) -> Result<Option<K::Value>, Self::Error>
        where K: DeserializeSeed<'de>
    {
        if self.left > 0 {
            self.left -= 1;
            Ok(Some(seed.deserialize(&mut *self.de)?))
        } else {
            Ok(None)
        }
    }

    fn next_value_seed<V>(&mut self, seed: V) -> Result<V::Value, Self::Error>
        where V: DeserializeSeed<'de>
    {
        Ok(seed.deserialize(&mut *self.de)?)
    }

    fn size_hint(&self) -> Option<usize> {
        Some(self.left)
    }
}

/// Default variant visitor.
///
/// # Note
///
/// We use default behaviour for new type, which decodes enums with a single value as a tuple.
pub struct VariantAccess<'a, R: 'a> {
    de: &'a mut Deserializer<R>,
}

impl<'a, R: 'a> VariantAccess<'a, R> {
    pub fn new(de: &'a mut Deserializer<R>) -> Self {
        VariantAccess {
            de: de,
        }
    }
}

impl<'de, 'a, R: Read<'de>> de::EnumAccess<'de> for VariantAccess<'a, R> {
    type Error = Error;
    type Variant = Self;

    fn variant_seed<V>(self, seed: V) -> Result<(V::Value, Self), Error>
        where V: de::DeserializeSeed<'de>,
    {
        use serde::de::IntoDeserializer;

        let idx: u32 = serde::Deserialize::deserialize(&mut *self.de)?;
        let val: Result<_, Error> = seed.deserialize(idx.into_deserializer());
        Ok((val?, self))
    }
}

impl<'de, 'a, R: Read<'de>> de::VariantAccess<'de> for VariantAccess<'a, R> {
    type Error = Error;

    fn unit_variant(self) -> Result<(), Error> {
        read_array_len(&mut self.de.rd)?;
        Ok(())
    }

    fn newtype_variant_seed<T>(self, seed: T) -> Result<T::Value, Self::Error>
        where T: DeserializeSeed<'de>
    {
        read_array_len(&mut self.de.rd)?;
        seed.deserialize(self.de)
    }

    fn tuple_variant<V>(self, len: usize, visitor: V) -> Result<V::Value, Error>
        where V: Visitor<'de>
    {
        de::Deserializer::deserialize_tuple(self.de, len, visitor)
    }

    fn struct_variant<V>(self, fields: &'static [&'static str], visitor: V) -> Result<V::Value, Error>
        where V: Visitor<'de>
    {
        de::Deserializer::deserialize_tuple(self.de, fields.len(), visitor)
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Reference<'b, 'c, T: ?Sized + 'static> {
    Borrowed(&'b T),
    Copied(&'c T),
}

pub trait Read<'de>: io::Read {
    fn read_slice<'a>(&'a mut self, len: usize) -> Result<Reference<'de, 'a, [u8]>, io::Error>;
}

pub struct SliceReader<'a> {
    inner: &'a [u8],
}

impl<'a> SliceReader<'a> {
    fn new(slice: &'a [u8]) -> Self {
        SliceReader {
            inner: slice,
        }
    }
}

impl<'de> Read<'de> for SliceReader<'de> {
    #[inline]
    fn read_slice<'a>(&'a mut self, len: usize) -> Result<Reference<'de, 'a, [u8]>, io::Error> {
        if len > self.inner.len() {
            return Err(io::Error::new(io::ErrorKind::UnexpectedEof, "unexpected EOF"))
        }
        let (a, b) = self.inner.split_at(len);
        self.inner = b;
        Ok(Reference::Borrowed(a))
    }
}

impl<'a> io::Read for SliceReader<'a> {
    #[inline]
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        self.inner.read(buf)
    }

    #[inline]
    fn read_exact(&mut self, buf: &mut [u8]) -> io::Result<()> {
        self.inner.read_exact(buf)
    }
}

pub struct ReadReader<R: io::Read> {
    inner: R,
    buf: Vec<u8>
}

impl<R: io::Read> ReadReader<R> {
    fn new(rd: R) -> Self {
        ReadReader {
            inner: rd,
            buf: Vec::with_capacity(128),
        }
    }
}

impl<'de, R: io::Read> Read<'de> for ReadReader<R> {
    #[inline]
    fn read_slice<'a>(&'a mut self, len: usize) -> Result<Reference<'de, 'a, [u8]>, io::Error> {
        self.buf.resize(len, 0u8);

        self.inner.read_exact(&mut self.buf[..])?;

        Ok(Reference::Copied(&self.buf[..]))
    }
}

impl<R: io::Read> io::Read for ReadReader<R> {
    #[inline]
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        self.inner.read(buf)
    }

    #[inline]
    fn read_exact(&mut self, buf: &mut [u8]) -> io::Result<()> {
        self.inner.read_exact(buf)
    }
}

#[test]
fn test_slice_read() {
    let buf = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    let mut rd = SliceReader::new(&buf[..]);

    assert_eq!(rd.read_slice(1).unwrap(), Reference::Borrowed(&[0][..]));
    assert_eq!(rd.read_slice(6).unwrap(), Reference::Borrowed(&[1, 2, 3, 4, 5, 6][..]));
    assert!(rd.read_slice(5).is_err());
    assert_eq!(rd.read_slice(4).unwrap(), Reference::Borrowed(&[7, 8, 9, 10][..]));
}

/// Deserialize an instance of type `T` from an I/O stream of MessagePack.
///
/// This conversion can fail if the structure of the Value does not match the structure expected
/// by `T`. It can also fail if the structure is correct but `T`'s implementation of `Deserialize`
/// decides that something is wrong with the data, for example required struct fields are missing.
pub fn from_read<R, T>(rd: R) -> Result<T, Error>
    where R: io::Read,
          T: DeserializeOwned
{
    Deserialize::deserialize(&mut Deserializer::new(rd))
}

/// Deserializes a byte slice into the desired type.
pub fn from_slice<'a, T>(input: &'a [u8]) -> Result<T, Error>
    where T: serde::Deserialize<'a>
{
    let mut de = Deserializer::from_slice(input);
    serde::Deserialize::deserialize(&mut de)
}
