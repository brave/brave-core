use alloc::{vec, vec::Vec};
use core::fmt;

use cid::CidGeneric;
use serde::{de, Deserialize};
use serde_bytes::ByteBuf;

/// Extract links from an `ipld_serde_dag*` codec.
#[derive(Debug)]
pub struct ExtractLinks<const S: usize> {
    links: Vec<CidGeneric<S>>,
}

impl<const S: usize> ExtractLinks<S> {
    /// Get the extracted links (CIDs).
    pub fn into_vec(self) -> Vec<CidGeneric<S>> {
        self.links
    }
}

impl<'de, const S: usize> de::Visitor<'de> for ExtractLinks<S> {
    type Value = Vec<CidGeneric<S>>;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_str("anything at all")
    }

    #[inline]
    fn visit_bool<E>(self, _value: bool) -> Result<Self::Value, E> {
        Ok(Vec::new())
    }

    #[inline]
    fn visit_i64<E>(self, _value: i64) -> Result<Self::Value, E> {
        Ok(Vec::new())
    }

    #[inline]
    fn visit_i128<E>(self, _value: i128) -> Result<Self::Value, E> {
        Ok(Vec::new())
    }

    #[inline]
    fn visit_u64<E>(self, _value: u64) -> Result<Self::Value, E> {
        Ok(Vec::new())
    }

    #[inline]
    fn visit_u128<E>(self, _value: u128) -> Result<Self::Value, E> {
        Ok(Vec::new())
    }

    #[inline]
    fn visit_f64<E>(self, _value: f64) -> Result<Self::Value, E> {
        Ok(Vec::new())
    }

    #[inline]
    fn visit_str<E>(self, _value: &str) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Vec::new())
    }

    #[inline]
    fn visit_none<E>(self) -> Result<Self::Value, E> {
        Ok(Vec::new())
    }

    #[inline]
    fn visit_some<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        Ok(Self::deserialize(deserializer)?.links)
    }

    #[inline]
    fn visit_newtype_struct<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        // No DAG-* format has the idea of a newtyp struct. Therefore when visiting a newtype
        // struct, we can be sure that it's from deserializing a CID.
        let bytes = ByteBuf::deserialize(deserializer)?;
        let cid =
            CidGeneric::try_from(&bytes[..]).map_err(|_| de::Error::custom("Cannot decode CID"))?;
        Ok(vec![cid])
    }

    #[inline]
    fn visit_unit<E>(self) -> Result<Self::Value, E> {
        Ok(Vec::new())
    }

    #[inline]
    fn visit_seq<A>(self, mut seq: A) -> Result<Self::Value, A::Error>
    where
        A: de::SeqAccess<'de>,
    {
        let mut links = Vec::new();
        while let Some(mut maybe_links) = seq.next_element::<Self>()? {
            links.append(&mut maybe_links.links)
        }
        Ok(links)
    }

    #[inline]
    fn visit_map<A>(self, mut map: A) -> Result<Self::Value, A::Error>
    where
        A: de::MapAccess<'de>,
    {
        let mut links = Vec::new();
        while let Some((_, mut maybe_links)) = map.next_entry::<Self, Self>()? {
            links.append(&mut maybe_links.links)
        }
        Ok(links)
    }

    #[inline]
    fn visit_bytes<E>(self, _value: &[u8]) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(Vec::new())
    }

    fn visit_enum<A>(self, data: A) -> Result<Self::Value, A::Error>
    where
        A: de::EnumAccess<'de>,
    {
        use serde::de::VariantAccess;
        data.variant::<Self>()?.1.newtype_variant()
    }
}

impl<'de, const S: usize> de::Deserialize<'de> for ExtractLinks<S> {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        let links = deserializer.deserialize_any(Self { links: Vec::new() })?;
        Ok(Self { links })
    }
}
