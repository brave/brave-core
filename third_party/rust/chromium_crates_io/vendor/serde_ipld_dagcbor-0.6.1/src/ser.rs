//! Serialization.
#[cfg(not(feature = "std"))]
use alloc::{collections::TryReserveError, string::ToString, vec::Vec};
#[cfg(feature = "std")]
use std::collections::TryReserveError;

pub use cbor4ii::core::utils::BufWriter;
#[cfg(feature = "std")]
use cbor4ii::core::utils::IoWriter;
use cbor4ii::core::{
    enc::{self, Encode},
    types,
};
use ipld_core::cid::serde::CID_SERDE_PRIVATE_IDENTIFIER;
use serde::{ser, Serialize};

use crate::error::EncodeError;
use crate::CBOR_TAGS_CID;

/// Serializes a value to a vector.
pub fn to_vec<T>(value: &T) -> Result<Vec<u8>, EncodeError<TryReserveError>>
where
    T: Serialize + ?Sized,
{
    let writer = BufWriter::new(Vec::new());
    let mut serializer = Serializer::new(writer);
    value.serialize(&mut serializer)?;
    Ok(serializer.into_inner().into_inner())
}

/// Serializes a value to a writer.
#[cfg(feature = "std")]
pub fn to_writer<W, T>(writer: W, value: &T) -> Result<(), EncodeError<std::io::Error>>
where
    W: std::io::Write,
    T: Serialize,
{
    let mut serializer = Serializer::new(IoWriter::new(writer));
    value.serialize(&mut serializer)
}

/// A structure for serializing Rust values to DAG-CBOR.
pub struct Serializer<W> {
    writer: W,
}

impl<W> Serializer<W> {
    /// Creates a new CBOR serializer.
    pub fn new(writer: W) -> Serializer<W> {
        Serializer { writer }
    }

    /// Returns the underlying writer.
    pub fn into_inner(self) -> W {
        self.writer
    }
}

impl<'a, W: enc::Write> serde::Serializer for &'a mut Serializer<W> {
    type Ok = ();
    type Error = EncodeError<W::Error>;

    type SerializeSeq = CollectSeq<'a, W>;
    type SerializeTuple = BoundedCollect<'a, W>;
    type SerializeTupleStruct = BoundedCollect<'a, W>;
    type SerializeTupleVariant = BoundedCollect<'a, W>;
    type SerializeMap = CollectMap<'a, W>;
    type SerializeStruct = CollectMap<'a, W>;
    type SerializeStructVariant = CollectMap<'a, W>;

    #[inline]
    fn serialize_bool(self, v: bool) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_i8(self, v: i8) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_i16(self, v: i16) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_i32(self, v: i32) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_i64(self, v: i64) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_u8(self, v: u8) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_u16(self, v: u16) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_u32(self, v: u32) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_u64(self, v: u64) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_f32(self, v: f32) -> Result<Self::Ok, Self::Error> {
        // In DAG-CBOR floats are always encoded as f64.
        self.serialize_f64(f64::from(v))
    }

    #[inline]
    fn serialize_f64(self, v: f64) -> Result<Self::Ok, Self::Error> {
        // In DAG-CBOR only finite floats are supported.
        if !v.is_finite() {
            Err(EncodeError::Msg(
                "Float must be a finite number, not Infinity or NaN".into(),
            ))
        } else {
            v.encode(&mut self.writer)?;
            Ok(())
        }
    }

    #[inline]
    fn serialize_char(self, v: char) -> Result<Self::Ok, Self::Error> {
        let mut buf = [0; 4];
        self.serialize_str(v.encode_utf8(&mut buf))
    }

    #[inline]
    fn serialize_str(self, v: &str) -> Result<Self::Ok, Self::Error> {
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_bytes(self, v: &[u8]) -> Result<Self::Ok, Self::Error> {
        types::Bytes(v).encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_none(self) -> Result<Self::Ok, Self::Error> {
        types::Null.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_some<T: Serialize + ?Sized>(self, value: &T) -> Result<Self::Ok, Self::Error> {
        value.serialize(self)
    }

    #[inline]
    fn serialize_unit(self) -> Result<Self::Ok, Self::Error> {
        // The cbor4ii Serde implementation encodes unit as an empty array, for DAG-CBOR we encode
        // it as `NULL`.
        types::Null.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_unit_struct(self, _name: &'static str) -> Result<Self::Ok, Self::Error> {
        self.serialize_unit()
    }

    #[inline]
    fn serialize_unit_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
    ) -> Result<Self::Ok, Self::Error> {
        self.serialize_str(variant)
    }

    #[inline]
    fn serialize_newtype_struct<T: Serialize + ?Sized>(
        self,
        name: &'static str,
        value: &T,
    ) -> Result<Self::Ok, Self::Error> {
        if name == CID_SERDE_PRIVATE_IDENTIFIER {
            value.serialize(&mut CidSerializer(self))
        } else {
            value.serialize(self)
        }
    }

    #[inline]
    fn serialize_newtype_variant<T: Serialize + ?Sized>(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
        value: &T,
    ) -> Result<Self::Ok, Self::Error> {
        enc::MapStartBounded(1).encode(&mut self.writer)?;
        variant.encode(&mut self.writer)?;
        value.serialize(self)
    }

    #[inline]
    fn serialize_seq(self, len: Option<usize>) -> Result<Self::SerializeSeq, Self::Error> {
        CollectSeq::new(self, len)
    }

    #[inline]
    fn serialize_tuple(self, len: usize) -> Result<Self::SerializeTuple, Self::Error> {
        enc::ArrayStartBounded(len).encode(&mut self.writer)?;
        Ok(BoundedCollect { ser: self })
    }

    #[inline]
    fn serialize_tuple_struct(
        self,
        _name: &'static str,
        len: usize,
    ) -> Result<Self::SerializeTupleStruct, Self::Error> {
        self.serialize_tuple(len)
    }

    #[inline]
    fn serialize_tuple_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
        len: usize,
    ) -> Result<Self::SerializeTupleVariant, Self::Error> {
        enc::MapStartBounded(1).encode(&mut self.writer)?;
        variant.encode(&mut self.writer)?;
        enc::ArrayStartBounded(len).encode(&mut self.writer)?;
        Ok(BoundedCollect { ser: self })
    }

    #[inline]
    fn serialize_map(self, _len: Option<usize>) -> Result<Self::SerializeMap, Self::Error> {
        Ok(CollectMap::new(self))
    }

    #[inline]
    fn serialize_struct(
        self,
        _name: &'static str,
        len: usize,
    ) -> Result<Self::SerializeStruct, Self::Error> {
        enc::MapStartBounded(len).encode(&mut self.writer)?;
        Ok(CollectMap::new(self))
    }

    #[inline]
    fn serialize_struct_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
        len: usize,
    ) -> Result<Self::SerializeStructVariant, Self::Error> {
        enc::MapStartBounded(1).encode(&mut self.writer)?;
        variant.encode(&mut self.writer)?;
        enc::MapStartBounded(len).encode(&mut self.writer)?;
        Ok(CollectMap::new(self))
    }

    #[inline]
    fn serialize_i128(self, v: i128) -> Result<Self::Ok, Self::Error> {
        if !(u64::MAX as i128 >= v && -(u64::MAX as i128 + 1) <= v) {
            return Err(EncodeError::Msg(
                "Integer must be within [-u64::MAX-1, u64::MAX] range".into(),
            ));
        }

        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn serialize_u128(self, v: u128) -> Result<Self::Ok, Self::Error> {
        if (u64::MAX as u128) < v {
            return Err(EncodeError::Msg(
                "Unsigned integer must be within [0, u64::MAX] range".into(),
            ));
        }
        v.encode(&mut self.writer)?;
        Ok(())
    }

    #[inline]
    fn is_human_readable(&self) -> bool {
        false
    }
}

/// Struct for implementign SerializeSeq.
pub struct CollectSeq<'a, W> {
    /// The number of elements. This is used in case the number of elements is not known
    /// beforehand.
    count: usize,
    ser: &'a mut Serializer<W>,
    /// An in-memory serializer in case the number of elements is not known beforehand.
    mem_ser: Option<Serializer<BufWriter>>,
}

impl<'a, W: enc::Write> CollectSeq<'a, W> {
    /// If the length of the sequence is given, use it. Else buffer the sequence in order to count
    /// the number of elements, which is then written before the elements are.
    fn new(ser: &'a mut Serializer<W>, len: Option<usize>) -> Result<Self, EncodeError<W::Error>> {
        let mem_ser = if let Some(len) = len {
            enc::ArrayStartBounded(len).encode(&mut ser.writer)?;
            None
        } else {
            Some(Serializer::new(BufWriter::new(Vec::new())))
        };
        Ok(Self {
            count: 0,
            ser,
            mem_ser,
        })
    }
}

/// Helper for processing collections.
pub struct BoundedCollect<'a, W> {
    ser: &'a mut Serializer<W>,
}

impl<W: enc::Write> serde::ser::SerializeSeq for CollectSeq<'_, W> {
    type Ok = ();
    type Error = EncodeError<W::Error>;

    #[inline]
    fn serialize_element<T: Serialize + ?Sized>(&mut self, value: &T) -> Result<(), Self::Error> {
        self.count += 1;
        if let Some(ser) = self.mem_ser.as_mut() {
            value
                .serialize(&mut *ser)
                .map_err(|_| EncodeError::Msg("List element cannot be serialized".to_string()))
        } else {
            value.serialize(&mut *self.ser)
        }
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        // Data was buffered in order to be able to write out the number of elements before they
        // are serialized.
        if let Some(ser) = self.mem_ser {
            enc::ArrayStartBounded(self.count).encode(&mut self.ser.writer)?;
            self.ser.writer.push(&ser.into_inner().into_inner())?;
        }

        Ok(())
    }
}

impl<W: enc::Write> serde::ser::SerializeTuple for BoundedCollect<'_, W> {
    type Ok = ();
    type Error = EncodeError<W::Error>;

    #[inline]
    fn serialize_element<T: Serialize + ?Sized>(&mut self, value: &T) -> Result<(), Self::Error> {
        value.serialize(&mut *self.ser)
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<W: enc::Write> serde::ser::SerializeTupleStruct for BoundedCollect<'_, W> {
    type Ok = ();
    type Error = EncodeError<W::Error>;

    #[inline]
    fn serialize_field<T: Serialize + ?Sized>(&mut self, value: &T) -> Result<(), Self::Error> {
        value.serialize(&mut *self.ser)
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<W: enc::Write> serde::ser::SerializeTupleVariant for BoundedCollect<'_, W> {
    type Ok = ();
    type Error = EncodeError<W::Error>;

    #[inline]
    fn serialize_field<T: Serialize + ?Sized>(&mut self, value: &T) -> Result<(), Self::Error> {
        value.serialize(&mut *self.ser)
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

/// CBOR RFC-7049 specifies a canonical sort order, where keys are sorted by length first. This
/// was later revised with RFC-8949, but we need to stick to the original order to stay compatible
/// with existing data.
/// We first serialize each map entry (the key and the value) into a buffer and then sort those
/// buffers. Once sorted they are written to the actual output.
pub struct CollectMap<'a, W> {
    buffer: BufWriter,
    entries: Vec<Vec<u8>>,
    ser: &'a mut Serializer<W>,
}

impl<'a, W> CollectMap<'a, W>
where
    W: enc::Write,
{
    fn new(ser: &'a mut Serializer<W>) -> Self {
        Self {
            buffer: BufWriter::new(Vec::new()),
            entries: Vec::new(),
            ser,
        }
    }

    fn serialize<T: Serialize + ?Sized>(
        &mut self,
        maybe_key: Option<&'static str>,
        value: &T,
    ) -> Result<(), EncodeError<W::Error>> {
        // Instantiate a new serializer, so that the buffer can be re-used.
        let mut mem_serializer = Serializer::new(&mut self.buffer);
        if let Some(key) = maybe_key {
            key.serialize(&mut mem_serializer)
                .map_err(|_| EncodeError::Msg("Struct key cannot be serialized.".to_string()))?;
        }
        value
            .serialize(&mut mem_serializer)
            .map_err(|_| EncodeError::Msg("Struct value cannot be serialized.".to_string()))?;

        self.entries.push(self.buffer.buffer().to_vec());
        self.buffer.clear();

        Ok(())
    }

    fn end(mut self) -> Result<(), EncodeError<W::Error>> {
        // This sorting step makes sure we have the expected order of the keys. Byte-wise
        // comparison over the encoded forms gives us the right order as keys in DAG-CBOR are
        // always (text) strings, hence have the same CBOR major type 3. The length of the string
        // is encoded in the prefix bits along with the major type. This means that a shorter string
        // always sorts before a longer string even with the compact length representation.
        self.entries.sort_unstable();
        for entry in self.entries {
            self.ser.writer.push(&entry)?;
        }
        Ok(())
    }
}

impl<W> serde::ser::SerializeMap for CollectMap<'_, W>
where
    W: enc::Write,
{
    type Ok = ();
    type Error = EncodeError<W::Error>;

    #[inline]
    fn serialize_key<T: Serialize + ?Sized>(&mut self, key: &T) -> Result<(), Self::Error> {
        // The key needs to be add to the buffer without any further operations. Serializing the
        // value will then do the necessary flushing etc.
        let mut mem_serializer = Serializer::new(&mut self.buffer);
        key.serialize(&mut mem_serializer)
            .map_err(|_| EncodeError::Msg("Map key cannot be serialized.".to_string()))?;
        Ok(())
    }

    #[inline]
    fn serialize_value<T: Serialize + ?Sized>(&mut self, value: &T) -> Result<(), Self::Error> {
        self.serialize(None, value)
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        enc::MapStartBounded(self.entries.len()).encode(&mut self.ser.writer)?;
        self.end()
    }
}

impl<W> serde::ser::SerializeStruct for CollectMap<'_, W>
where
    W: enc::Write,
{
    type Ok = ();
    type Error = EncodeError<W::Error>;

    #[inline]
    fn serialize_field<T: Serialize + ?Sized>(
        &mut self,
        key: &'static str,
        value: &T,
    ) -> Result<(), Self::Error> {
        self.serialize(Some(key), value)
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        self.end()
    }
}

impl<W> serde::ser::SerializeStructVariant for CollectMap<'_, W>
where
    W: enc::Write,
{
    type Ok = ();
    type Error = EncodeError<W::Error>;

    #[inline]
    fn serialize_field<T: Serialize + ?Sized>(
        &mut self,
        key: &'static str,
        value: &T,
    ) -> Result<(), Self::Error> {
        self.serialize(Some(key), value)
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        self.end()
    }
}

/// Serializing a CID correctly as DAG-CBOR.
struct CidSerializer<'a, W>(&'a mut Serializer<W>);

impl<'a, W: enc::Write> ser::Serializer for &'a mut CidSerializer<'a, W>
where
    W::Error: core::fmt::Debug,
{
    type Ok = ();
    type Error = EncodeError<W::Error>;

    type SerializeSeq = ser::Impossible<Self::Ok, Self::Error>;
    type SerializeTuple = ser::Impossible<Self::Ok, Self::Error>;
    type SerializeTupleStruct = ser::Impossible<Self::Ok, Self::Error>;
    type SerializeTupleVariant = ser::Impossible<Self::Ok, Self::Error>;
    type SerializeMap = ser::Impossible<Self::Ok, Self::Error>;
    type SerializeStruct = ser::Impossible<Self::Ok, Self::Error>;
    type SerializeStructVariant = ser::Impossible<Self::Ok, Self::Error>;

    fn serialize_bool(self, _value: bool) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_i8(self, _value: i8) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_i16(self, _value: i16) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_i32(self, _value: i32) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_i64(self, _value: i64) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_u8(self, _value: u8) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_u16(self, _value: u16) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_u32(self, _value: u32) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_u64(self, _value: u64) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_f32(self, _value: f32) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_f64(self, _value: f64) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_char(self, _value: char) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_str(self, _value: &str) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }

    fn serialize_bytes(self, value: &[u8]) -> Result<Self::Ok, Self::Error> {
        // The bytes of the CID is prefixed with a null byte when encoded as CBOR.
        let prefixed = [&[0x00], value].concat();
        // CIDs are serialized with CBOR tag 42.
        types::Tag(CBOR_TAGS_CID, types::Bytes(&prefixed[..])).encode(&mut self.0.writer)?;
        Ok(())
    }

    fn serialize_none(self) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_some<T: ?Sized + ser::Serialize>(
        self,
        _value: &T,
    ) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_unit(self) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_unit_struct(self, _name: &str) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_unit_variant(
        self,
        _name: &str,
        _variant_index: u32,
        _variant: &str,
    ) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }

    fn serialize_newtype_struct<T: ?Sized + ser::Serialize>(
        self,
        _name: &str,
        _value: &T,
    ) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_newtype_variant<T: ?Sized + ser::Serialize>(
        self,
        _name: &str,
        _variant_index: u32,
        _variant: &str,
        _value: &T,
    ) -> Result<Self::Ok, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_seq(self, _len: Option<usize>) -> Result<Self::SerializeSeq, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_tuple(self, _len: usize) -> Result<Self::SerializeTuple, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_tuple_struct(
        self,
        _name: &str,
        _len: usize,
    ) -> Result<Self::SerializeTupleStruct, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_tuple_variant(
        self,
        _name: &str,
        _variant_index: u32,
        _variant: &str,
        _len: usize,
    ) -> Result<Self::SerializeTupleVariant, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_map(self, _len: Option<usize>) -> Result<Self::SerializeMap, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_struct(
        self,
        _name: &str,
        _len: usize,
    ) -> Result<Self::SerializeStruct, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
    fn serialize_struct_variant(
        self,
        _name: &str,
        _variant_index: u32,
        _variant: &str,
        _len: usize,
    ) -> Result<Self::SerializeStructVariant, Self::Error> {
        Err(ser::Error::custom("unreachable"))
    }
}
