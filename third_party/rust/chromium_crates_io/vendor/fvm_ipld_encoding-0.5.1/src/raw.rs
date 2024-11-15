// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use thiserror::Error;

/// Serialize the given value to a vec. This method rejects all types except "raw bytes".
pub fn to_vec<T: serde::Serialize + ?Sized>(value: &T) -> Result<Vec<u8>, super::Error> {
    serde::Serialize::serialize(value, Serializer).map_err(|e| super::Error {
        description: e.to_string(),
        protocol: crate::CodecProtocol::Raw,
    })
}

#[derive(Error, Debug)]
enum Error {
    #[error("IPLD kind not supported by the raw codec")]
    KindNotSupported,
    #[error("i/o error when serializing: {0}")]
    Other(String),
}

impl serde::ser::Error for Error {
    fn custom<T>(msg: T) -> Self
    where
        T: std::fmt::Display,
    {
        Error::Other(msg.to_string())
    }
}

struct Serializer;

macro_rules! reject {
    ($($method:ident $t:ty),*) => {
        $(
            fn $method(self, _: $t) -> Result<Self::Ok, Self::Error> {
                Err(Error::KindNotSupported)
            }
        )*
    };
}

impl serde::ser::Serializer for Serializer {
    type Ok = Vec<u8>;
    type Error = Error;
    type SerializeSeq = serde::ser::Impossible<Vec<u8>, Error>;
    type SerializeTuple = serde::ser::Impossible<Vec<u8>, Error>;
    type SerializeTupleStruct = serde::ser::Impossible<Vec<u8>, Error>;
    type SerializeTupleVariant = serde::ser::Impossible<Vec<u8>, Error>;
    type SerializeMap = serde::ser::Impossible<Vec<u8>, Error>;
    type SerializeStruct = serde::ser::Impossible<Vec<u8>, Error>;
    type SerializeStructVariant = serde::ser::Impossible<Vec<u8>, Error>;

    fn serialize_bytes(self, v: &[u8]) -> Result<Self::Ok, Self::Error> {
        Ok(v.to_owned())
    }

    reject! {
        serialize_bool bool,
        serialize_i8 i8,
        serialize_i16 i16,
        serialize_i32 i32,
        serialize_i64 i64,
        serialize_u8 u8,
        serialize_u16 u16,
        serialize_u32 u32,
        serialize_u64 u64,
        serialize_f32 f32,
        serialize_f64 f64,
        serialize_char char,
        serialize_str &str,
        serialize_unit_struct &'static str
    }

    fn serialize_none(self) -> Result<Self::Ok, Self::Error> {
        Err(Error::KindNotSupported)
    }

    fn serialize_some<T>(self, _: &T) -> Result<Self::Ok, Self::Error>
    where
        T: serde::Serialize + ?Sized,
    {
        Err(Error::KindNotSupported)
    }

    fn serialize_unit(self) -> Result<Self::Ok, Self::Error> {
        Err(Error::KindNotSupported)
    }

    fn serialize_unit_variant(
        self,
        _: &'static str,
        _: u32,
        _: &'static str,
    ) -> Result<Self::Ok, Self::Error> {
        Err(Error::KindNotSupported)
    }

    fn serialize_newtype_struct<T>(self, _: &'static str, _: &T) -> Result<Self::Ok, Self::Error>
    where
        T: serde::Serialize + ?Sized,
    {
        Err(Error::KindNotSupported)
    }

    fn serialize_newtype_variant<T>(
        self,
        _: &'static str,
        _: u32,
        _: &'static str,
        _: &T,
    ) -> Result<Self::Ok, Self::Error>
    where
        T: serde::Serialize + ?Sized,
    {
        Err(Error::KindNotSupported)
    }

    fn serialize_seq(self, _: Option<usize>) -> Result<Self::SerializeSeq, Self::Error> {
        Err(Error::KindNotSupported)
    }

    fn serialize_tuple(self, _: usize) -> Result<Self::SerializeTuple, Self::Error> {
        Err(Error::KindNotSupported)
    }

    fn serialize_tuple_struct(
        self,
        _: &'static str,
        _: usize,
    ) -> Result<Self::SerializeTupleStruct, Self::Error> {
        Err(Error::KindNotSupported)
    }

    fn serialize_tuple_variant(
        self,
        _: &'static str,
        _: u32,
        _: &'static str,
        _: usize,
    ) -> Result<Self::SerializeTupleVariant, Self::Error> {
        Err(Error::KindNotSupported)
    }

    fn serialize_map(self, _: Option<usize>) -> Result<Self::SerializeMap, Self::Error> {
        Err(Error::KindNotSupported)
    }

    fn serialize_struct(
        self,
        _: &'static str,
        _: usize,
    ) -> Result<Self::SerializeStruct, Self::Error> {
        Err(Error::KindNotSupported)
    }

    fn serialize_struct_variant(
        self,
        _: &'static str,
        _: u32,
        _: &'static str,
        _: usize,
    ) -> Result<Self::SerializeStructVariant, Self::Error> {
        Err(Error::KindNotSupported)
    }
}
