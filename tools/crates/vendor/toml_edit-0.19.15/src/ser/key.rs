use crate::InternalString;

use super::Error;

pub(crate) struct KeySerializer;

impl serde::ser::Serializer for KeySerializer {
    type Ok = InternalString;
    type Error = Error;
    type SerializeSeq = serde::ser::Impossible<InternalString, Error>;
    type SerializeTuple = serde::ser::Impossible<InternalString, Error>;
    type SerializeTupleStruct = serde::ser::Impossible<InternalString, Error>;
    type SerializeTupleVariant = serde::ser::Impossible<InternalString, Error>;
    type SerializeMap = serde::ser::Impossible<InternalString, Error>;
    type SerializeStruct = serde::ser::Impossible<InternalString, Error>;
    type SerializeStructVariant = serde::ser::Impossible<InternalString, Error>;

    fn serialize_bool(self, _v: bool) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_i8(self, _v: i8) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_i16(self, _v: i16) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_i32(self, _v: i32) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_i64(self, _v: i64) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_u8(self, _v: u8) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_u16(self, _v: u16) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_u32(self, _v: u32) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_u64(self, _v: u64) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_f32(self, _v: f32) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_f64(self, _v: f64) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_char(self, _v: char) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_str(self, value: &str) -> Result<InternalString, Self::Error> {
        Ok(InternalString::from(value))
    }

    fn serialize_bytes(self, _value: &[u8]) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_none(self) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_some<T: ?Sized>(self, _value: &T) -> Result<InternalString, Self::Error>
    where
        T: serde::ser::Serialize,
    {
        Err(Error::KeyNotString)
    }

    fn serialize_unit(self) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_unit_struct(self, _name: &'static str) -> Result<InternalString, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_unit_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
    ) -> Result<InternalString, Self::Error> {
        Ok(variant.into())
    }

    fn serialize_newtype_struct<T: ?Sized>(
        self,
        _name: &'static str,
        value: &T,
    ) -> Result<InternalString, Self::Error>
    where
        T: serde::ser::Serialize,
    {
        value.serialize(self)
    }

    fn serialize_newtype_variant<T: ?Sized>(
        self,
        _name: &'static str,
        _variant_index: u32,
        _variant: &'static str,
        _value: &T,
    ) -> Result<InternalString, Self::Error>
    where
        T: serde::ser::Serialize,
    {
        Err(Error::KeyNotString)
    }

    fn serialize_seq(self, _len: Option<usize>) -> Result<Self::SerializeSeq, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_tuple(self, _len: usize) -> Result<Self::SerializeTuple, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_tuple_struct(
        self,
        _name: &'static str,
        _len: usize,
    ) -> Result<Self::SerializeTupleStruct, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_tuple_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        _variant: &'static str,
        _len: usize,
    ) -> Result<Self::SerializeTupleVariant, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_map(self, _len: Option<usize>) -> Result<Self::SerializeMap, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_struct(
        self,
        _name: &'static str,
        _len: usize,
    ) -> Result<Self::SerializeStruct, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_struct_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        _variant: &'static str,
        _len: usize,
    ) -> Result<Self::SerializeStructVariant, Self::Error> {
        Err(Error::KeyNotString)
    }
}
