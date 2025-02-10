use crate::Key;

use super::Error;

pub(crate) struct KeySerializer;

impl serde::ser::Serializer for KeySerializer {
    type Ok = Key;
    type Error = Error;
    type SerializeSeq = serde::ser::Impossible<Key, Error>;
    type SerializeTuple = serde::ser::Impossible<Key, Error>;
    type SerializeTupleStruct = serde::ser::Impossible<Key, Error>;
    type SerializeTupleVariant = serde::ser::Impossible<Key, Error>;
    type SerializeMap = serde::ser::Impossible<Key, Error>;
    type SerializeStruct = serde::ser::Impossible<Key, Error>;
    type SerializeStructVariant = serde::ser::Impossible<Key, Error>;

    fn serialize_bool(self, _v: bool) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_i8(self, _v: i8) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_i16(self, _v: i16) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_i32(self, _v: i32) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_i64(self, _v: i64) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_u8(self, _v: u8) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_u16(self, _v: u16) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_u32(self, _v: u32) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_u64(self, _v: u64) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_f32(self, _v: f32) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_f64(self, _v: f64) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_char(self, _v: char) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_str(self, value: &str) -> Result<Key, Self::Error> {
        Ok(Key::new(value))
    }

    fn serialize_bytes(self, _value: &[u8]) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_none(self) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_some<T>(self, _value: &T) -> Result<Key, Self::Error>
    where
        T: serde::ser::Serialize + ?Sized,
    {
        Err(Error::KeyNotString)
    }

    fn serialize_unit(self) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_unit_struct(self, _name: &'static str) -> Result<Key, Self::Error> {
        Err(Error::KeyNotString)
    }

    fn serialize_unit_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
    ) -> Result<Key, Self::Error> {
        Ok(variant.into())
    }

    fn serialize_newtype_struct<T>(self, _name: &'static str, value: &T) -> Result<Key, Self::Error>
    where
        T: serde::ser::Serialize + ?Sized,
    {
        value.serialize(self)
    }

    fn serialize_newtype_variant<T>(
        self,
        _name: &'static str,
        _variant_index: u32,
        _variant: &'static str,
        _value: &T,
    ) -> Result<Key, Self::Error>
    where
        T: serde::ser::Serialize + ?Sized,
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
