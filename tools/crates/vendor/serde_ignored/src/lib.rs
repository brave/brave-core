//! [![github]](https://github.com/dtolnay/serde-ignored)&ensp;[![crates-io]](https://crates.io/crates/serde_ignored)&ensp;[![docs-rs]](https://docs.rs/serde_ignored)
//!
//! [github]: https://img.shields.io/badge/github-8da0cb?style=for-the-badge&labelColor=555555&logo=github
//! [crates-io]: https://img.shields.io/badge/crates.io-fc8d62?style=for-the-badge&labelColor=555555&logo=rust
//! [docs-rs]: https://img.shields.io/badge/docs.rs-66c2a5?style=for-the-badge&labelColor=555555&logo=docs.rs
//!
//! <br>
//!
//! Find out about keys that are ignored when deserializing data. This crate
//! provides a wrapper that works with any existing Serde `Deserializer` and
//! invokes a callback on every ignored field.
//!
//! You can use this to warn users about extraneous keys in a config file, for
//! example.
//!
//! Note that if you want unrecognized fields to be an error, consider using the
//! `#[serde(deny_unknown_fields)]` [attribute] instead.
//!
//! [attribute]: https://serde.rs/attributes.html
//!
//! # Example
//!
//! ```
//! # use serde_derive::Deserialize;
//! #
//! use serde::Deserialize;
//! use std::collections::{BTreeSet as Set, BTreeMap as Map};
//!
//! #[derive(Debug, PartialEq, Deserialize)]
//! struct Package {
//!     name: String,
//!     dependencies: Map<String, Dependency>,
//! }
//!
//! #[derive(Debug, PartialEq, Deserialize)]
//! struct Dependency {
//!     version: String,
//! }
//!
//! # fn try_main() -> Result<(), Box<::std::error::Error>> {
//! let j = r#"{
//!     "name": "demo",
//!     "dependencies": {
//!         "serde": {
//!             "version": "1.0",
//!             "typo1": ""
//!         }
//!     },
//!     "typo2": {
//!         "inner": ""
//!     },
//!     "typo3": {}
//! }"#;
//!
//! // Some Deserializer.
//! let jd = &mut serde_json::Deserializer::from_str(j);
//!
//! // We will build a set of paths to the unused elements.
//! let mut unused = Set::new();
//!
//! let p: Package = serde_ignored::deserialize(jd, |path| {
//!     unused.insert(path.to_string());
//! })?;
//!
//! assert_eq!(p, Package {
//!     name: "demo".to_owned(),
//!     dependencies: {
//!         let mut map = Map::new();
//!         map.insert("serde".to_owned(), Dependency {
//!             version: "1.0".to_owned(),
//!         });
//!         map
//!     },
//! });
//!
//! assert_eq!(unused, {
//!     let mut expected = Set::new();
//!     expected.insert("dependencies.serde.typo1".to_owned());
//!     expected.insert("typo2".to_owned());
//!     expected.insert("typo3".to_owned());
//!     expected
//! });
//!
//! # Ok(()) }
//! # fn main() { try_main().unwrap() }
//! ```

#![no_std]
#![doc(html_root_url = "https://docs.rs/serde_ignored/0.1.10")]
#![allow(clippy::missing_errors_doc)]

extern crate alloc;

use alloc::borrow::ToOwned;
use alloc::string::{String, ToString};
use alloc::vec::Vec;
use core::fmt::{self, Display};
use serde::de::{self, Deserialize, DeserializeSeed, Visitor};

/// Entry point. See crate documentation for an example.
pub fn deserialize<'de, D, F, T>(deserializer: D, mut callback: F) -> Result<T, D::Error>
where
    D: de::Deserializer<'de>,
    F: FnMut(Path),
    T: Deserialize<'de>,
{
    T::deserialize(Deserializer::new(deserializer, &mut callback))
}

/// Deserializer adapter that invokes a callback with the path to every unused
/// field of the input.
pub struct Deserializer<'a, 'b, D, F: 'b> {
    de: D,
    callback: &'b mut F,
    path: Path<'a>,
}

impl<'a, 'b, D, F> Deserializer<'a, 'b, D, F>
where
    F: FnMut(Path),
{
    // The structs in this crate all hold their closure by &mut F. If they were
    // to contain F by value, any method taking &mut self (for example
    // SeqAccess::next_element_seed) would be forced to recurse with &mut
    // self.callback, even if F is instantiated with a &mut already. This way
    // they contain &mut F and the &mut self methods can recurse with
    // self.callback unchanged. This avoids blowing the recursion limit in
    // Cargo's use of this crate.
    //
    // https://github.com/dtolnay/serde-ignored/pull/1
    pub fn new(de: D, callback: &'b mut F) -> Self {
        Deserializer {
            de,
            callback,
            path: Path::Root,
        }
    }
}

/// Path to the current value in the input, like `dependencies.serde.typo1`.
pub enum Path<'a> {
    Root,
    Seq { parent: &'a Path<'a>, index: usize },
    Map { parent: &'a Path<'a>, key: String },
    Some { parent: &'a Path<'a> },
    NewtypeStruct { parent: &'a Path<'a> },
    NewtypeVariant { parent: &'a Path<'a> },
}

impl<'a> Display for Path<'a> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        struct Parent<'a>(&'a Path<'a>);

        impl<'a> Display for Parent<'a> {
            fn fmt(&self, formatter: &mut fmt::Formatter) -> Result<(), fmt::Error> {
                match *self.0 {
                    Path::Root => Ok(()),
                    ref path => write!(formatter, "{}.", path),
                }
            }
        }

        match *self {
            Path::Root => formatter.write_str("."),
            Path::Seq { parent, index } => write!(formatter, "{}{}", Parent(parent), index),
            Path::Map { parent, ref key } => write!(formatter, "{}{}", Parent(parent), key),
            Path::Some { parent }
            | Path::NewtypeStruct { parent }
            | Path::NewtypeVariant { parent } => write!(formatter, "{}?", Parent(parent)),
        }
    }
}

/// Plain old forwarding impl except for `deserialize_ignored_any` which invokes
/// the callback.
impl<'a, 'b, 'de, D, F> de::Deserializer<'de> for Deserializer<'a, 'b, D, F>
where
    D: de::Deserializer<'de>,
    F: FnMut(Path),
{
    type Error = D::Error;

    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_any(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_bool<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_bool(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_u8<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_u8(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_u16<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_u16(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_u32<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_u32(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_u64<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_u64(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_i8<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_i8(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_i16<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_i16(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_i32<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_i32(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_i64<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_i64(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_f32<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_f32(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_f64<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_f64(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_char<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_char(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_str<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_str(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_string<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_string(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_bytes<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_bytes(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_byte_buf<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_byte_buf(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_option(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_unit<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_unit(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_unit_struct<V>(
        self,
        name: &'static str,
        visitor: V,
    ) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_unit_struct(name, Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_newtype_struct<V>(
        self,
        name: &'static str,
        visitor: V,
    ) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_newtype_struct(name, Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_seq<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_seq(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_tuple<V>(self, len: usize, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_tuple(len, Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_tuple_struct<V>(
        self,
        name: &'static str,
        len: usize,
        visitor: V,
    ) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_tuple_struct(name, len, Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_map<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_map(Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_struct<V>(
        self,
        name: &'static str,
        fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_struct(name, fields, Wrap::new(visitor, self.callback, &self.path))
    }

    fn deserialize_enum<V>(
        self,
        name: &'static str,
        variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de.deserialize_enum(
            name,
            variants,
            Wrap::new(visitor, self.callback, &self.path),
        )
    }

    fn deserialize_ignored_any<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        (self.callback)(self.path);
        self.de.deserialize_ignored_any(visitor)
    }

    fn deserialize_identifier<V>(self, visitor: V) -> Result<V::Value, D::Error>
    where
        V: Visitor<'de>,
    {
        self.de
            .deserialize_identifier(Wrap::new(visitor, self.callback, &self.path))
    }

    fn is_human_readable(&self) -> bool {
        self.de.is_human_readable()
    }
}

/// Wrapper that attaches context to a `Visitor`, `SeqAccess`, `EnumAccess` or
/// `VariantAccess`.
struct Wrap<'a, 'b, X, F: 'b> {
    delegate: X,
    callback: &'b mut F,
    path: &'a Path<'a>,
}

impl<'a, 'b, X, F> Wrap<'a, 'b, X, F> {
    fn new(delegate: X, callback: &'b mut F, path: &'a Path<'a>) -> Self {
        Wrap {
            delegate,
            callback,
            path,
        }
    }
}

/// Forwarding impl to preserve context.
impl<'a, 'b, 'de, X, F> Visitor<'de> for Wrap<'a, 'b, X, F>
where
    X: Visitor<'de>,
    F: FnMut(Path),
{
    type Value = X::Value;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        self.delegate.expecting(formatter)
    }

    fn visit_bool<E>(self, v: bool) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_bool(v)
    }

    fn visit_i8<E>(self, v: i8) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_i8(v)
    }

    fn visit_i16<E>(self, v: i16) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_i16(v)
    }

    fn visit_i32<E>(self, v: i32) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_i32(v)
    }

    fn visit_i64<E>(self, v: i64) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_i64(v)
    }

    fn visit_u8<E>(self, v: u8) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_u8(v)
    }

    fn visit_u16<E>(self, v: u16) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_u16(v)
    }

    fn visit_u32<E>(self, v: u32) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_u32(v)
    }

    fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_u64(v)
    }

    fn visit_f32<E>(self, v: f32) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_f32(v)
    }

    fn visit_f64<E>(self, v: f64) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_f64(v)
    }

    fn visit_char<E>(self, v: char) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_char(v)
    }

    fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_str(v)
    }

    fn visit_borrowed_str<E>(self, v: &'de str) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_borrowed_str(v)
    }

    fn visit_string<E>(self, v: String) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_string(v)
    }

    fn visit_unit<E>(self) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_unit()
    }

    fn visit_none<E>(self) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_none()
    }

    fn visit_some<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        self.delegate.visit_some(Deserializer {
            de: deserializer,
            callback: self.callback,
            path: Path::Some { parent: self.path },
        })
    }

    fn visit_newtype_struct<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        self.delegate.visit_newtype_struct(Deserializer {
            de: deserializer,
            callback: self.callback,
            path: Path::NewtypeStruct { parent: self.path },
        })
    }

    fn visit_seq<V>(self, visitor: V) -> Result<Self::Value, V::Error>
    where
        V: de::SeqAccess<'de>,
    {
        self.delegate
            .visit_seq(SeqAccess::new(visitor, self.callback, self.path))
    }

    fn visit_map<V>(self, visitor: V) -> Result<Self::Value, V::Error>
    where
        V: de::MapAccess<'de>,
    {
        self.delegate
            .visit_map(MapAccess::new(visitor, self.callback, self.path))
    }

    fn visit_enum<V>(self, visitor: V) -> Result<Self::Value, V::Error>
    where
        V: de::EnumAccess<'de>,
    {
        self.delegate
            .visit_enum(Wrap::new(visitor, self.callback, self.path))
    }

    fn visit_bytes<E>(self, v: &[u8]) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_bytes(v)
    }

    fn visit_borrowed_bytes<E>(self, v: &'de [u8]) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_borrowed_bytes(v)
    }

    fn visit_byte_buf<E>(self, v: Vec<u8>) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_byte_buf(v)
    }
}

/// Forwarding impl to preserve context.
impl<'a, 'b, 'de, X, F> de::EnumAccess<'de> for Wrap<'a, 'b, X, F>
where
    X: de::EnumAccess<'de> + 'a,
    F: FnMut(Path) + 'b,
{
    type Error = X::Error;
    type Variant = Wrap<'a, 'b, X::Variant, F>;

    fn variant_seed<V>(self, seed: V) -> Result<(V::Value, Self::Variant), X::Error>
    where
        V: DeserializeSeed<'de>,
    {
        let callback = self.callback;
        let path = self.path;
        self.delegate
            .variant_seed(seed)
            .map(move |(v, vis)| (v, Wrap::new(vis, callback, path)))
    }
}

/// Forwarding impl to preserve context.
impl<'a, 'b, 'de, X, F> de::VariantAccess<'de> for Wrap<'a, 'b, X, F>
where
    X: de::VariantAccess<'de>,
    F: FnMut(Path),
{
    type Error = X::Error;

    fn unit_variant(self) -> Result<(), X::Error> {
        self.delegate.unit_variant()
    }

    fn newtype_variant_seed<T>(self, seed: T) -> Result<T::Value, X::Error>
    where
        T: DeserializeSeed<'de>,
    {
        let path = Path::NewtypeVariant { parent: self.path };
        self.delegate
            .newtype_variant_seed(TrackedSeed::new(seed, self.callback, path))
    }

    fn tuple_variant<V>(self, len: usize, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .tuple_variant(len, Wrap::new(visitor, self.callback, self.path))
    }

    fn struct_variant<V>(
        self,
        fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .struct_variant(fields, Wrap::new(visitor, self.callback, self.path))
    }
}

/// Seed that saves the string into the given optional during `visit_str` and
/// `visit_string`.
struct CaptureKey<'a, X> {
    delegate: X,
    key: &'a mut Option<String>,
}

impl<'a, X> CaptureKey<'a, X> {
    fn new(delegate: X, key: &'a mut Option<String>) -> Self {
        CaptureKey { delegate, key }
    }
}

/// Forwarding impl.
impl<'a, 'de, X> DeserializeSeed<'de> for CaptureKey<'a, X>
where
    X: DeserializeSeed<'de>,
{
    type Value = X::Value;

    fn deserialize<D>(self, deserializer: D) -> Result<X::Value, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        self.delegate
            .deserialize(CaptureKey::new(deserializer, self.key))
    }
}

/// Forwarding impl.
impl<'a, 'de, X> de::Deserializer<'de> for CaptureKey<'a, X>
where
    X: de::Deserializer<'de>,
{
    type Error = X::Error;

    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_any(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_bool<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_bool(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_u8<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_u8(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_u16<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_u16(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_u32<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_u32(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_u64<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_u64(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_i8<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_i8(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_i16<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_i16(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_i32<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_i32(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_i64<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_i64(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_f32<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_f32(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_f64<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_f64(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_char<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_char(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_str<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_str(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_string<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_string(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_bytes<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_bytes(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_byte_buf<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_byte_buf(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_option(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_unit<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_unit(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_unit_struct<V>(
        self,
        name: &'static str,
        visitor: V,
    ) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_unit_struct(name, CaptureKey::new(visitor, self.key))
    }

    fn deserialize_newtype_struct<V>(
        self,
        name: &'static str,
        visitor: V,
    ) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_newtype_struct(name, CaptureKey::new(visitor, self.key))
    }

    fn deserialize_seq<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_seq(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_tuple<V>(self, len: usize, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_tuple(len, CaptureKey::new(visitor, self.key))
    }

    fn deserialize_tuple_struct<V>(
        self,
        name: &'static str,
        len: usize,
        visitor: V,
    ) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_tuple_struct(name, len, CaptureKey::new(visitor, self.key))
    }

    fn deserialize_map<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_map(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_struct<V>(
        self,
        name: &'static str,
        fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_struct(name, fields, CaptureKey::new(visitor, self.key))
    }

    fn deserialize_enum<V>(
        self,
        name: &'static str,
        variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_enum(name, variants, CaptureKey::new(visitor, self.key))
    }

    fn deserialize_ignored_any<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_ignored_any(CaptureKey::new(visitor, self.key))
    }

    fn deserialize_identifier<V>(self, visitor: V) -> Result<V::Value, X::Error>
    where
        V: Visitor<'de>,
    {
        self.delegate
            .deserialize_identifier(CaptureKey::new(visitor, self.key))
    }

    fn is_human_readable(&self) -> bool {
        self.delegate.is_human_readable()
    }
}

/// Forwarding impl that also saves the value of integers and strings.
impl<'a, 'de, X> Visitor<'de> for CaptureKey<'a, X>
where
    X: Visitor<'de>,
{
    type Value = X::Value;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        self.delegate.expecting(formatter)
    }

    fn visit_bool<E>(self, v: bool) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_string());
        self.delegate.visit_bool(v)
    }

    fn visit_i8<E>(self, v: i8) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_string());
        self.delegate.visit_i8(v)
    }

    fn visit_i16<E>(self, v: i16) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_string());
        self.delegate.visit_i16(v)
    }

    fn visit_i32<E>(self, v: i32) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_string());
        self.delegate.visit_i32(v)
    }

    fn visit_i64<E>(self, v: i64) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_string());
        self.delegate.visit_i64(v)
    }

    fn visit_u8<E>(self, v: u8) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_string());
        self.delegate.visit_u8(v)
    }

    fn visit_u16<E>(self, v: u16) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_string());
        self.delegate.visit_u16(v)
    }

    fn visit_u32<E>(self, v: u32) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_string());
        self.delegate.visit_u32(v)
    }

    fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_string());
        self.delegate.visit_u64(v)
    }

    fn visit_f32<E>(self, v: f32) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_f32(v)
    }

    fn visit_f64<E>(self, v: f64) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_f64(v)
    }

    fn visit_char<E>(self, v: char) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_char(v)
    }

    fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_owned());
        self.delegate.visit_str(v)
    }

    fn visit_borrowed_str<E>(self, v: &'de str) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.to_owned());
        self.delegate.visit_borrowed_str(v)
    }

    fn visit_string<E>(self, v: String) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        *self.key = Some(v.clone());
        self.delegate.visit_string(v)
    }

    fn visit_unit<E>(self) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_unit()
    }

    fn visit_none<E>(self) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_none()
    }

    fn visit_some<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        self.delegate.visit_some(deserializer)
    }

    fn visit_newtype_struct<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        self.delegate
            .visit_newtype_struct(CaptureKey::new(deserializer, self.key))
    }

    fn visit_seq<V>(self, visitor: V) -> Result<Self::Value, V::Error>
    where
        V: de::SeqAccess<'de>,
    {
        self.delegate.visit_seq(visitor)
    }

    fn visit_map<V>(self, visitor: V) -> Result<Self::Value, V::Error>
    where
        V: de::MapAccess<'de>,
    {
        self.delegate.visit_map(visitor)
    }

    fn visit_enum<V>(self, visitor: V) -> Result<Self::Value, V::Error>
    where
        V: de::EnumAccess<'de>,
    {
        self.delegate.visit_enum(CaptureKey::new(visitor, self.key))
    }

    fn visit_bytes<E>(self, v: &[u8]) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_bytes(v)
    }

    fn visit_borrowed_bytes<E>(self, v: &'de [u8]) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_borrowed_bytes(v)
    }

    fn visit_byte_buf<E>(self, v: Vec<u8>) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        self.delegate.visit_byte_buf(v)
    }
}

impl<'a, 'de, X> de::EnumAccess<'de> for CaptureKey<'a, X>
where
    X: de::EnumAccess<'de>,
{
    type Error = X::Error;
    type Variant = X::Variant;

    fn variant_seed<V>(self, seed: V) -> Result<(V::Value, Self::Variant), X::Error>
    where
        V: DeserializeSeed<'de>,
    {
        self.delegate.variant_seed(CaptureKey::new(seed, self.key))
    }
}

/// Seed used for map values, sequence elements and newtype variants to track
/// their path.
struct TrackedSeed<'a, X, F: 'a> {
    seed: X,
    callback: &'a mut F,
    path: Path<'a>,
}

impl<'a, X, F> TrackedSeed<'a, X, F> {
    fn new(seed: X, callback: &'a mut F, path: Path<'a>) -> Self {
        TrackedSeed {
            seed,
            callback,
            path,
        }
    }
}

impl<'a, 'de, X, F> DeserializeSeed<'de> for TrackedSeed<'a, X, F>
where
    X: DeserializeSeed<'de>,
    F: FnMut(Path),
{
    type Value = X::Value;

    fn deserialize<D>(self, deserializer: D) -> Result<X::Value, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        self.seed.deserialize(Deserializer {
            de: deserializer,
            callback: self.callback,
            path: self.path,
        })
    }
}

/// Seq visitor that tracks the index of its elements.
struct SeqAccess<'a, 'b, X, F: 'b> {
    delegate: X,
    callback: &'b mut F,
    path: &'a Path<'a>,
    index: usize,
}

impl<'a, 'b, X, F> SeqAccess<'a, 'b, X, F> {
    fn new(delegate: X, callback: &'b mut F, path: &'a Path<'a>) -> Self {
        SeqAccess {
            delegate,
            callback,
            path,
            index: 0,
        }
    }
}

/// Forwarding impl to preserve context.
impl<'a, 'b, 'de, X, F> de::SeqAccess<'de> for SeqAccess<'a, 'b, X, F>
where
    X: de::SeqAccess<'de>,
    F: FnMut(Path),
{
    type Error = X::Error;

    fn next_element_seed<T>(&mut self, seed: T) -> Result<Option<T::Value>, X::Error>
    where
        T: DeserializeSeed<'de>,
    {
        let path = Path::Seq {
            parent: self.path,
            index: self.index,
        };
        self.index += 1;
        self.delegate
            .next_element_seed(TrackedSeed::new(seed, self.callback, path))
    }

    fn size_hint(&self) -> Option<usize> {
        self.delegate.size_hint()
    }
}

/// Map visitor that captures the string value of its keys and uses that to
/// track the path to its values.
struct MapAccess<'a, 'b, X, F: 'b> {
    delegate: X,
    callback: &'b mut F,
    path: &'a Path<'a>,
    key: Option<String>,
}

impl<'a, 'b, X, F> MapAccess<'a, 'b, X, F> {
    fn new(delegate: X, callback: &'b mut F, path: &'a Path<'a>) -> Self {
        MapAccess {
            delegate,
            callback,
            path,
            key: None,
        }
    }

    fn key<E>(&mut self) -> Result<String, E>
    where
        E: de::Error,
    {
        self.key.take().ok_or_else(|| E::custom("non-string key"))
    }
}

impl<'a, 'b, 'de, X, F> de::MapAccess<'de> for MapAccess<'a, 'b, X, F>
where
    X: de::MapAccess<'de>,
    F: FnMut(Path),
{
    type Error = X::Error;

    fn next_key_seed<K>(&mut self, seed: K) -> Result<Option<K::Value>, X::Error>
    where
        K: DeserializeSeed<'de>,
    {
        self.delegate
            .next_key_seed(CaptureKey::new(seed, &mut self.key))
    }

    fn next_value_seed<V>(&mut self, seed: V) -> Result<V::Value, X::Error>
    where
        V: DeserializeSeed<'de>,
    {
        let path = Path::Map {
            parent: self.path,
            key: self.key()?,
        };
        self.delegate
            .next_value_seed(TrackedSeed::new(seed, self.callback, path))
    }

    fn size_hint(&self) -> Option<usize> {
        self.delegate.size_hint()
    }
}
