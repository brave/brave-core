use core::convert::TryFrom;
use libipld_core::cid::Cid;
use libipld_core::ipld::Ipld;
use libipld_core::multibase::Base;
use serde::de::Error as SerdeError;
use serde::{de, ser, Deserialize, Serialize};
use serde_json::ser::Serializer;
use serde_json::Error;
use std::collections::BTreeMap;
use std::fmt;
use std::io::{Read, Write};

const RESERVED_KEY: &str = "/";
const BYTES_KEY: &str = "bytes";

pub fn encode<W: Write>(ipld: &Ipld, writer: &mut W) -> Result<(), Error> {
    let mut ser = Serializer::new(writer);
    serialize(ipld, &mut ser)?;
    Ok(())
}

pub fn decode<R: Read>(r: &mut R) -> Result<Ipld, Error> {
    let mut de = serde_json::Deserializer::from_reader(r);
    deserialize(&mut de)
}

fn serialize<S: ser::Serializer>(ipld: &Ipld, ser: S) -> Result<S::Ok, S::Error> {
    match &ipld {
        Ipld::Null => ser.serialize_none(),
        Ipld::Bool(bool) => ser.serialize_bool(*bool),
        Ipld::Integer(i128) => ser.serialize_i128(*i128),
        Ipld::Float(f64) => ser.serialize_f64(*f64),
        Ipld::String(string) => ser.serialize_str(string),
        Ipld::Bytes(bytes) => {
            let base_encoded = Base::Base64.encode(bytes);
            let byteskv = BTreeMap::from([("bytes", &base_encoded)]);
            let slashkv = BTreeMap::from([("/", byteskv)]);
            ser.collect_map(slashkv)
        }
        Ipld::List(list) => {
            let wrapped = list.iter().map(Wrapper);
            ser.collect_seq(wrapped)
        }
        Ipld::Map(map) => {
            let wrapped = map.iter().map(|(key, ipld)| (key, Wrapper(ipld)));
            ser.collect_map(wrapped)
        }
        Ipld::Link(link) => {
            let mut map = BTreeMap::new();
            map.insert("/", link.to_string());

            ser.collect_map(map)
        }
    }
}

fn deserialize<'de, D: de::Deserializer<'de>>(deserializer: D) -> Result<Ipld, D::Error> {
    // Sadly such a PhantomData hack is needed
    deserializer.deserialize_any(JsonVisitor)
}

// Needed for `collect_seq` and `collect_map` in Seserializer
struct Wrapper<'a>(&'a Ipld);

impl<'a> Serialize for Wrapper<'a> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: ser::Serializer,
    {
        serialize(self.0, serializer)
    }
}

// serde deserializer visitor that is used by Deseraliazer to decode
// json into IPLD.
struct JsonVisitor;
impl<'de> de::Visitor<'de> for JsonVisitor {
    type Value = Ipld;

    fn expecting(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt.write_str("any valid JSON value")
    }

    fn visit_str<E>(self, value: &str) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.visit_string(String::from(value))
    }

    fn visit_string<E>(self, value: String) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Ipld::String(value))
    }
    fn visit_bytes<E>(self, v: &[u8]) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.visit_byte_buf(v.to_owned())
    }

    fn visit_byte_buf<E>(self, v: Vec<u8>) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Ipld::Bytes(v))
    }

    fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Ipld::Integer(v.into()))
    }

    fn visit_i64<E>(self, v: i64) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Ipld::Integer(v.into()))
    }

    fn visit_i128<E>(self, v: i128) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Ipld::Integer(v))
    }

    fn visit_bool<E>(self, v: bool) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Ipld::Bool(v))
    }

    fn visit_none<E>(self) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.visit_unit()
    }

    fn visit_unit<E>(self) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Ipld::Null)
    }

    fn visit_seq<V>(self, mut visitor: V) -> Result<Self::Value, V::Error>
    where
        V: de::SeqAccess<'de>,
    {
        let mut vec: Vec<WrapperOwned> = Vec::new();

        while let Some(elem) = visitor.next_element()? {
            vec.push(elem);
        }

        let unwrapped = vec.into_iter().map(|WrapperOwned(ipld)| ipld).collect();
        Ok(Ipld::List(unwrapped))
    }

    fn visit_map<V>(self, mut visitor: V) -> Result<Self::Value, V::Error>
    where
        V: de::MapAccess<'de>,
    {
        let mut values: Vec<(String, WrapperOwned)> = Vec::new();

        while let Some((key, value)) = visitor.next_entry()? {
            values.push((key, value));
        }

        // JSON Object represents an IPLD Link if it is a slash, followed by a string
        // (`{ "/": "...." }`) therefore we validate if that is the case here.
        if let Some((key, WrapperOwned(Ipld::String(value)))) = values.first() {
            if key == RESERVED_KEY && values.len() == 1 {
                let cid = Cid::try_from(value.clone()).map_err(SerdeError::custom)?;
                return Ok(Ipld::Link(cid));
            }
        }
        // JSON Object represents IPLD bytes if it is a slash, followed by an object which contains
        // only a single key called "bytes", where the value is a string.
        if let Some((key, WrapperOwned(Ipld::Map(map)))) = values.first() {
            // Object with a slash
            if key == RESERVED_KEY && values.len() == 1 {
                if let Some((bytes_key, Ipld::String(bytes_value))) = map.iter().next() {
                    if bytes_key == BYTES_KEY && values.len() == 1 {
                        let decoded_bytes = Base::Base64.decode(bytes_value).map_err(|_| {
                            SerdeError::custom("bytes kind must be base-64 encoded")
                        })?;
                        return Ok(Ipld::Bytes(decoded_bytes));
                    }
                }
            }
        }

        let mut unwrapped = BTreeMap::new();
        for (key, WrapperOwned(value)) in values {
            let prev_value = unwrapped.insert(key, value);
            if prev_value.is_some() {
                return Err(SerdeError::custom("duplicate map key".to_string()));
            }
        }
        Ok(Ipld::Map(unwrapped))
    }

    fn visit_f64<E>(self, v: f64) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Ipld::Float(v))
    }
}

// Needed for `visit_seq` and `visit_map` in Deserializer
/// We cannot directly implement `serde::Deserializer` for `Ipld` as it is a remote type.
/// Instead wrap it into a newtype struct and implement `serde::Deserialize` for that one.
/// All the deserializer does is calling the `deserialize()` function we defined which returns
/// an unwrapped `Ipld` instance. Wrap that `Ipld` instance in `Wrapper` and return it.
/// Users of this wrapper will then unwrap it again so that they can return the expected `Ipld`
/// instance.
struct WrapperOwned(Ipld);

impl<'de> Deserialize<'de> for WrapperOwned {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        let deserialized = deserialize(deserializer);
        // Better version of Ok(Wrapper(deserialized.unwrap()))
        deserialized.map(Self)
    }
}
