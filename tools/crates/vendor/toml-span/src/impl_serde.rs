#![cfg_attr(docsrs, doc(cfg(feature = "serde")))]

//! Provides [`serde::Serialize`] support for [`Value`] and [`Spanned`]

use crate::{
    value::{Value, ValueInner},
    Spanned,
};
use serde::ser::{SerializeMap, SerializeSeq};

impl<'de> serde::Serialize for Value<'de> {
    fn serialize<S>(&self, ser: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        match self.as_ref() {
            ValueInner::String(s) => ser.serialize_str(s),
            ValueInner::Integer(i) => ser.serialize_i64(*i),
            ValueInner::Float(f) => ser.serialize_f64(*f),
            ValueInner::Boolean(b) => ser.serialize_bool(*b),
            ValueInner::Array(arr) => {
                let mut seq = ser.serialize_seq(Some(arr.len()))?;
                for ele in arr {
                    seq.serialize_element(ele)?;
                }
                seq.end()
            }
            ValueInner::Table(tab) => {
                let mut map = ser.serialize_map(Some(tab.len()))?;
                for (k, v) in tab {
                    map.serialize_entry(&k.name, v)?;
                }
                map.end()
            }
        }
    }
}

impl<T> serde::Serialize for Spanned<T>
where
    T: serde::Serialize,
{
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        self.value.serialize(serializer)
    }
}
