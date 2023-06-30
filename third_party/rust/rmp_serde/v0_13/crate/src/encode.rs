use std::error;
use std::fmt::{self, Display};
use std::io::Write;

use serde;
use serde::Serialize;
use serde::ser::{SerializeMap, SerializeSeq, SerializeStruct, SerializeStructVariant,
                 SerializeTuple, SerializeTupleStruct, SerializeTupleVariant};

use rmp;
use rmp::Marker;
use rmp::encode::{write_nil, write_bool, write_uint, write_sint, write_f32, write_f64, write_str,
                  write_array_len, write_map_len, write_bin_len, ValueWriteError};

#[derive(Debug)]
pub enum Error {
    InvalidValueWrite(ValueWriteError),

    /// Failed to serialize struct, sequence or map, because its length is unknown.
    UnknownLength,

    /// Depth limit exceeded
    DepthLimitExceeded,
    Syntax(String),
}

impl error::Error for Error {
    fn description(&self) -> &str {
        match *self {
            Error::InvalidValueWrite(..) => "invalid value write",
            Error::UnknownLength => {
                "attempt to serialize struct, sequence or map with unknown length"
            }
            Error::DepthLimitExceeded => "depth limit exceeded",
            Error::Syntax(..) => "syntax error",
        }
    }

    fn cause(&self) -> Option<&error::Error> {
        match *self {
            Error::InvalidValueWrite(ref err) => Some(err),
            Error::UnknownLength => None,
            Error::DepthLimitExceeded => None,
            Error::Syntax(..) => None,
        }
    }
}

impl Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        error::Error::description(self).fmt(f)
    }
}

impl From<ValueWriteError> for Error {
    fn from(err: ValueWriteError) -> Error {
        Error::InvalidValueWrite(err)
    }
}

impl serde::ser::Error for Error {
    /// Raised when there is general error when deserializing a type.
    fn custom<T: Display>(msg: T) -> Error {
        Error::Syntax(format!("{}", msg))
    }
}

// TODO: Great idea - drop this trait and make Serializer types configurable.
pub trait VariantWriter {
    fn write_struct_len<W>(&self, wr: &mut W, len: u32) -> Result<Marker, ValueWriteError>
        where W: Write;
    fn write_field_name<W>(&self, wr: &mut W, key: &str) -> Result<(), ValueWriteError>
        where W: Write;
}

/// Writes struct as MessagePack array with no field names
pub struct StructArrayWriter;

impl VariantWriter for StructArrayWriter {
    fn write_struct_len<W>(&self, wr: &mut W, len: u32) -> Result<Marker, ValueWriteError>
        where W: Write
    {
        write_array_len(wr, len)
    }

    /// This implementation does not write field names
    #[allow(unused_variables)]
    fn write_field_name<W>(&self, wr: &mut W, _key: &str) -> Result<(), ValueWriteError>
        where W: Write
    {
        Ok(())
    }
}

pub struct StructMapWriter;

impl VariantWriter for StructMapWriter {
    fn write_struct_len<W>(&self, wr: &mut W, len: u32) -> Result<Marker, ValueWriteError>
    where
        W: Write,
    {
        write_map_len(wr, len)
    }

    fn write_field_name<W>(&self, wr: &mut W, key: &str) -> Result<(), ValueWriteError>
    where
        W: Write,
    {
        write_str(wr, key)
    }
}
impl<W: Write> Serializer<W, StructMapWriter> {
    /// Constructs a new `MessagePack` serializer whose output will be written to the writer
    /// specified.
    ///
    /// # Note
    ///
    /// This is the default constructor, which returns a serializer that will serialize structs
    /// using large named representation.
    pub fn new_named(wr: W) -> Self {
        Serializer::with(wr, StructMapWriter)
    }
}
/// Represents MessagePack serialization implementation.
///
/// # Note
///
/// MessagePack has no specification about how to encode enum types. Thus we are free to do
/// whatever we want, so the given chose may be not ideal for you.
///
/// Every Rust enum value can be represented as a tuple of index with a value.
///
/// All instances of `ErrorKind::Interrupted` are handled by this function and the underlying
/// operation is retried.
// TODO: Docs. Examples.
pub struct Serializer<W, V> {
    wr: W,
    vw: V,
    depth: usize,
}

impl<W, V> Serializer<W, V> {
    /// Changes the maximum nesting depth that is allowed
    pub fn set_max_depth(&mut self, depth: usize) {
        self.depth = depth;
    }
}

impl<W: Write> Serializer<W, StructArrayWriter> {
    /// Constructs a new `MessagePack` serializer whose output will be written to the writer
    /// specified.
    ///
    /// # Note
    ///
    /// This is the default constructor, which returns a serializer that will serialize structs
    /// using compact tuple representation, without field names.
    pub fn new(wr: W) -> Self {
        Serializer::with(wr, StructArrayWriter)
    }
    pub fn compact(wr: W) -> Self {
        Serializer::with(wr, StructArrayWriter)
    }
}

impl<W: Write, V> Serializer<W, V> {
    /// Gets a reference to the underlying writer.
    pub fn get_ref(&self) -> &W {
        &self.wr
    }

    /// Gets a mutable reference to the underlying writer.
    ///
    /// It is inadvisable to directly write to the underlying writer.
    pub fn get_mut(&mut self) -> &mut W {
        &mut self.wr
    }

    /// Unwraps this `Serializer`, returning the underlying writer.
    pub fn into_inner(self) -> W {
        self.wr
    }
}

impl<W: Write, V: VariantWriter> Serializer<W, V> {
    /// Creates a new MessagePack encoder whose output will be written to the writer specified.
    pub fn with(wr: W, vw: V) -> Self {
        Serializer {
            wr: wr,
            vw: vw,
            depth: 1024,
        }
    }
}

pub struct Compound<'a, W: 'a, V: 'a> {
    // Note, that the implementation is stateless.
    se: &'a mut Serializer<W, V>,
}

impl<'a, W: Write + 'a, V: VariantWriter + 'a> SerializeSeq for Compound<'a, W, V> {
    type Ok = ();
    type Error = Error;

    fn serialize_element<T: ?Sized + Serialize>(&mut self, value: &T) -> Result<(), Self::Error> {
        value.serialize(&mut *self.se)
    }

    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<'a, W: Write + 'a, V: VariantWriter + 'a> SerializeTuple for Compound<'a, W, V> {
    type Ok = ();
    type Error = Error;

    fn serialize_element<T: ?Sized + Serialize>(&mut self, value: &T) -> Result<(), Self::Error> {
        value.serialize(&mut *self.se)
    }

    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<'a, W: Write + 'a, V: VariantWriter + 'a> SerializeTupleStruct for Compound<'a, W, V> {
    type Ok = ();
    type Error = Error;

    fn serialize_field<T: ?Sized + Serialize>(&mut self, value: &T) -> Result<(), Self::Error> {
        value.serialize(&mut *self.se)
    }

    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<'a, W: Write + 'a, V: VariantWriter + 'a> SerializeTupleVariant for Compound<'a, W, V> {
    type Ok = ();
    type Error = Error;

    fn serialize_field<T: ?Sized + Serialize>(&mut self, value: &T) -> Result<(), Self::Error> {
        value.serialize(&mut *self.se)
    }

    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<'a, W: Write + 'a, V: VariantWriter + 'a> SerializeMap for Compound<'a, W, V> {
    type Ok = ();
    type Error = Error;

    fn serialize_key<T: ?Sized + Serialize>(&mut self, key: &T) -> Result<(), Self::Error> {
        key.serialize(&mut *self.se)
    }

    fn serialize_value<T: ?Sized + Serialize>(&mut self, value: &T) -> Result<(), Self::Error> {
        value.serialize(&mut *self.se)
    }

    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<'a, W: Write + 'a, V: VariantWriter + 'a> SerializeStruct for Compound<'a, W, V> {
    type Ok = ();
    type Error = Error;

    fn serialize_field<T: ?Sized + Serialize>(&mut self, key: &'static str, value: &T) ->
        Result<(), Self::Error>
    {
        self.se.vw.write_field_name(&mut self.se.wr, key)?;
        value.serialize(&mut *self.se)
    }

    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<'a, W: Write + 'a, V: VariantWriter + 'a> SerializeStructVariant for Compound<'a, W, V> {
    type Ok = ();
    type Error = Error;

    fn serialize_field<T: ?Sized + Serialize>(&mut self, _key: &'static str, value: &T) ->
        Result<(), Self::Error>
    {
        value.serialize(&mut *self.se)
    }

    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<'a, W: Write, V: VariantWriter> serde::Serializer for &'a mut Serializer<W, V> {
    type Ok = ();
    type Error = Error;

    type SerializeSeq = Compound<'a, W, V>;
    type SerializeTuple = Compound<'a, W, V>;
    type SerializeTupleStruct = Compound<'a, W, V>;
    type SerializeTupleVariant = Compound<'a, W, V>;
    type SerializeMap = Compound<'a, W, V>;
    type SerializeStruct = Compound<'a, W, V>;
    type SerializeStructVariant = Compound<'a, W, V>;

    fn serialize_bool(self, v: bool) -> Result<Self::Ok, Self::Error> {
        write_bool(&mut self.wr, v)
            .map_err(|err| Error::InvalidValueWrite(ValueWriteError::InvalidMarkerWrite(err)))
    }

    fn serialize_i8(self, v: i8) -> Result<Self::Ok, Self::Error> {
        self.serialize_i64(v as i64)
    }

    fn serialize_i16(self, v: i16) -> Result<Self::Ok, Self::Error> {
        self.serialize_i64(v as i64)
    }

    fn serialize_i32(self, v: i32) -> Result<Self::Ok, Self::Error> {
        self.serialize_i64(v as i64)
    }

    fn serialize_i64(self, v: i64) -> Result<Self::Ok, Self::Error> {
        write_sint(&mut self.wr, v)?;
        Ok(())
    }

    fn serialize_u8(self, v: u8) -> Result<Self::Ok, Self::Error> {
        self.serialize_u64(v as u64)
    }

    fn serialize_u16(self, v: u16) -> Result<Self::Ok, Self::Error> {
        self.serialize_u64(v as u64)
    }

    fn serialize_u32(self, v: u32) -> Result<Self::Ok, Self::Error> {
        self.serialize_u64(v as u64)
    }

    fn serialize_u64(self, v: u64) -> Result<Self::Ok, Self::Error> {
        write_uint(&mut self.wr, v)?;
        Ok(())
    }

    fn serialize_f32(self, v: f32) -> Result<Self::Ok, Self::Error> {
        write_f32(&mut self.wr, v)?;
        Ok(())
    }

    fn serialize_f64(self, v: f64) -> Result<Self::Ok, Self::Error> {
        write_f64(&mut self.wr, v)?;
        Ok(())
    }

    fn serialize_char(self, v: char) -> Result<Self::Ok, Self::Error> {
        // A char encoded as UTF-8 takes 4 bytes at most.
        let mut buf = [0; 4];
        self.serialize_str(v.encode_utf8(&mut buf))
    }

    fn serialize_str(self, v: &str) -> Result<Self::Ok, Self::Error> {
        write_str(&mut self.wr, v)?;
        Ok(())
    }

    fn serialize_bytes(self, value: &[u8]) -> Result<Self::Ok, Self::Error> {
        write_bin_len(&mut self.wr, value.len() as u32)?;
        self.wr
            .write_all(value)
            .map_err(|err| Error::InvalidValueWrite(ValueWriteError::InvalidDataWrite(err)))
    }

    fn serialize_none(self) -> Result<(), Self::Error> {
        self.serialize_unit()
    }

    fn serialize_some<T: ?Sized + serde::Serialize>(self, v: &T) -> Result<(), Self::Error> {
        v.serialize(self)
    }

    fn serialize_unit(self) -> Result<Self::Ok, Self::Error> {
        write_nil(&mut self.wr)
            .map_err(|err| Error::InvalidValueWrite(ValueWriteError::InvalidMarkerWrite(err)))
    }

    fn serialize_unit_struct(self, _name: &'static str) -> Result<Self::Ok, Self::Error> {
        self.vw.write_struct_len(&mut self.wr, 0)?;
        Ok(())
    }

    fn serialize_unit_variant(self, _name: &str, idx: u32, _variant: &str) ->
        Result<Self::Ok, Self::Error>
    {
        write_array_len(&mut self.wr, 2)?;
        self.serialize_u32(idx)?;
        write_array_len(&mut self.wr, 0)?;
        Ok(())
    }

    fn serialize_newtype_struct<T: ?Sized + serde::Serialize>(self, name: &'static str, value: &T) -> Result<(), Self::Error> {
        self.serialize_tuple_struct(name, 1)?;
        value.serialize(self)
    }

    fn serialize_newtype_variant<T: ?Sized + serde::Serialize>(self, name: &'static str, variant_index: u32, variant: &'static str, value: &T) -> Result<Self::Ok, Self::Error> {
        self.serialize_tuple_variant(name, variant_index, variant, 1)?;
        value.serialize(self)
    }

    fn serialize_seq(self, len: Option<usize>) -> Result<Self::SerializeSeq, Error> {
        let len = match len {
            Some(len) => len,
            None => return Err(Error::UnknownLength),
        };

        write_array_len(&mut self.wr, len as u32)?;

        Ok(Compound { se: self })
    }

    fn serialize_tuple(self, len: usize) -> Result<Self::SerializeTuple, Self::Error> {
        self.serialize_seq(Some(len))
    }

    fn serialize_tuple_struct(self, _name: &'static str, len: usize) ->
        Result<Self::SerializeTupleStruct, Self::Error>
    {
        self.serialize_tuple(len)
    }

    fn serialize_tuple_variant(self,  name: &'static str,  idx: u32,  _variant: &'static str,  len: usize) ->
        Result<Self::SerializeTupleVariant, Error>
    {
        // We encode variant types as a tuple of id with array of args, like: [id, [args...]].
        rmp::encode::write_array_len(&mut self.wr, 2)?;
        self.serialize_u32(idx)?;
        self.serialize_tuple_struct(name, len)
    }

    fn serialize_map(self, len: Option<usize>) -> Result<Self::SerializeMap, Error> {
        match len {
            Some(len) => {
                write_map_len(&mut self.wr, len as u32)?;
                Ok(Compound { se: self })
            }
            None => Err(Error::UnknownLength),
        }
    }

    fn serialize_struct(self, _name: &'static str, len: usize) ->
        Result<Self::SerializeStruct, Self::Error>
    {
        self.vw.write_struct_len(&mut self.wr, len as u32)?;
        Ok(Compound { se: self })
    }

    fn serialize_struct_variant(self, name: &'static str, id: u32, _variant: &'static str, len: usize) ->
        Result<Self::SerializeStructVariant, Error>
    {
        write_array_len(&mut self.wr, 2)?;
        self.serialize_u32(id)?;
        self.serialize_struct(name, len)
    }
}

/// Serialize the given data structure as MessagePack into the I/O stream.
/// This fyunction uses compact representation - structures as arrays
///
/// Serialization can fail if `T`'s implementation of `Serialize` decides to fail.
#[inline]
pub fn write<W: ?Sized, T: ?Sized>(wr: &mut W, val: &T) -> Result<(), Error>
    where W: Write,
          T: Serialize
{
    val.serialize(&mut Serializer::compact(wr))
}

/// Serialize the given data structure as MessagePack into the I/O stream.
/// This function serializes structures as maps
///
/// Serialization can fail if `T`'s implementation of `Serialize` decides to fail.
#[inline]
pub fn write_named<W: ?Sized, T: ?Sized>(wr: &mut W, val: &T) -> Result<(), Error>
where
    W: Write,
    T: Serialize,
{
    val.serialize(&mut Serializer::new_named(wr))
}

/// Serialize the given data structure as a MessagePack byte vector.
/// This method uses compact representation, structs are serialized as arrays
///
/// Serialization can fail if `T`'s implementation of `Serialize` decides to fail.
#[inline]
pub fn to_vec<T: ?Sized>(val: &T) -> Result<Vec<u8>, Error>
    where T: Serialize
{
    let mut buf = Vec::with_capacity(128);
    write(&mut buf, val)?;
    Ok(buf)
}

/// Serializes data structure into byte vector as a map
/// Resulting MessagePack message will contain field names
///
/// Serialization can fail if `T`'s implementation of `Serialize` decides to fail.
#[inline]
pub fn to_vec_named<T>(value: &T) -> Result<Vec<u8>, Error>
where
    T: serde::Serialize,
{
    let mut buf = Vec::with_capacity(64);
    value.serialize(&mut Serializer::new_named(&mut buf))?;
    Ok(buf)
}

