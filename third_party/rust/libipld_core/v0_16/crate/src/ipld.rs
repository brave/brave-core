//! Ipld representation.
use alloc::{
    borrow::ToOwned,
    boxed::Box,
    collections::BTreeMap,
    string::{String, ToString},
    vec,
    vec::Vec,
};
use core::fmt;

use crate::cid::Cid;
use crate::error::TypeError;

/// Ipld
#[derive(Clone, PartialEq)]
pub enum Ipld {
    /// Represents the absence of a value or the value undefined.
    Null,
    /// Represents a boolean value.
    Bool(bool),
    /// Represents an integer.
    Integer(i128),
    /// Represents a floating point value.
    Float(f64),
    /// Represents an UTF-8 string.
    String(String),
    /// Represents a sequence of bytes.
    Bytes(Vec<u8>),
    /// Represents a list.
    List(Vec<Ipld>),
    /// Represents a map of strings.
    Map(BTreeMap<String, Ipld>),
    /// Represents a map of integers.
    Link(Cid),
}

impl fmt::Debug for Ipld {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        if f.alternate() {
            match self {
                Self::Null => write!(f, "Null"),
                Self::Bool(b) => write!(f, "Bool({:?})", b),
                Self::Integer(i) => write!(f, "Integer({:?})", i),
                Self::Float(i) => write!(f, "Float({:?})", i),
                Self::String(s) => write!(f, "String({:?})", s),
                Self::Bytes(b) => write!(f, "Bytes({:?})", b),
                Self::List(l) => write!(f, "List({:#?})", l),
                Self::Map(m) => write!(f, "Map({:#?})", m),
                Self::Link(cid) => write!(f, "Link({})", cid),
            }
        } else {
            match self {
                Self::Null => write!(f, "null"),
                Self::Bool(b) => write!(f, "{:?}", b),
                Self::Integer(i) => write!(f, "{:?}", i),
                Self::Float(i) => write!(f, "{:?}", i),
                Self::String(s) => write!(f, "{:?}", s),
                Self::Bytes(b) => write!(f, "{:?}", b),
                Self::List(l) => write!(f, "{:?}", l),
                Self::Map(m) => write!(f, "{:?}", m),
                Self::Link(cid) => write!(f, "{}", cid),
            }
        }
    }
}

/// An index into ipld
pub enum IpldIndex<'a> {
    /// An index into an ipld list.
    List(usize),
    /// An owned index into an ipld map.
    Map(String),
    /// An index into an ipld map.
    MapRef(&'a str),
}

impl<'a> From<usize> for IpldIndex<'a> {
    fn from(index: usize) -> Self {
        Self::List(index)
    }
}

impl<'a> From<String> for IpldIndex<'a> {
    fn from(key: String) -> Self {
        Self::Map(key)
    }
}

impl<'a> From<&'a str> for IpldIndex<'a> {
    fn from(key: &'a str) -> Self {
        Self::MapRef(key)
    }
}

impl Ipld {
    /// Destructs an ipld list or map
    pub fn take<'a, T: Into<IpldIndex<'a>>>(mut self, index: T) -> Result<Self, TypeError> {
        let index = index.into();
        let ipld = match &mut self {
            Ipld::List(ref mut l) => match index {
                IpldIndex::List(i) => Some(i),
                IpldIndex::Map(ref key) => key.parse().ok(),
                IpldIndex::MapRef(key) => key.parse().ok(),
            }
            .map(|i| {
                if i < l.len() {
                    Some(l.swap_remove(i))
                } else {
                    None
                }
            }),
            Ipld::Map(ref mut m) => match index {
                IpldIndex::Map(ref key) => Some(m.remove(key)),
                IpldIndex::MapRef(key) => Some(m.remove(key)),
                IpldIndex::List(i) => Some(m.remove(&i.to_string())),
            },
            _ => None,
        };
        ipld.unwrap_or_default()
            .ok_or_else(|| TypeError::new(index, self))
    }

    /// Indexes into an ipld list or map.
    pub fn get<'a, T: Into<IpldIndex<'a>>>(&self, index: T) -> Result<&Self, TypeError> {
        let index = index.into();
        let ipld = match self {
            Ipld::List(l) => match index {
                IpldIndex::List(i) => Some(i),
                IpldIndex::Map(ref key) => key.parse().ok(),
                IpldIndex::MapRef(key) => key.parse().ok(),
            }
            .map(|i| l.get(i)),
            Ipld::Map(m) => match index {
                IpldIndex::Map(ref key) => Some(m.get(key)),
                IpldIndex::MapRef(key) => Some(m.get(key)),
                IpldIndex::List(i) => Some(m.get(&i.to_string())),
            },
            _ => None,
        };
        ipld.unwrap_or_default()
            .ok_or_else(|| TypeError::new(index, self))
    }

    /// Returns an iterator.
    pub fn iter(&self) -> IpldIter<'_> {
        IpldIter {
            stack: vec![Box::new(vec![self].into_iter())],
        }
    }

    /// Returns the references to other blocks.
    pub fn references<E: Extend<Cid>>(&self, set: &mut E) {
        for ipld in self.iter() {
            if let Ipld::Link(cid) = ipld {
                set.extend(core::iter::once(cid.to_owned()));
            }
        }
    }
}

/// Ipld iterator.
pub struct IpldIter<'a> {
    stack: Vec<Box<dyn Iterator<Item = &'a Ipld> + 'a>>,
}

impl<'a> Iterator for IpldIter<'a> {
    type Item = &'a Ipld;

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            if let Some(iter) = self.stack.last_mut() {
                if let Some(ipld) = iter.next() {
                    match ipld {
                        Ipld::List(list) => {
                            self.stack.push(Box::new(list.iter()));
                        }
                        Ipld::Map(map) => {
                            self.stack.push(Box::new(map.values()));
                        }
                        _ => {}
                    }
                    return Some(ipld);
                } else {
                    self.stack.pop();
                }
            } else {
                return None;
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cid::Cid;
    use crate::multihash::{Code, MultihashDigest};

    #[test]
    fn test_ipld_bool_from() {
        assert_eq!(Ipld::Bool(true), Ipld::from(true));
        assert_eq!(Ipld::Bool(false), Ipld::from(false));
    }

    #[test]
    fn test_ipld_integer_from() {
        assert_eq!(Ipld::Integer(1), Ipld::from(1i8));
        assert_eq!(Ipld::Integer(1), Ipld::from(1i16));
        assert_eq!(Ipld::Integer(1), Ipld::from(1i32));
        assert_eq!(Ipld::Integer(1), Ipld::from(1i64));
        assert_eq!(Ipld::Integer(1), Ipld::from(1i128));

        //assert_eq!(Ipld::Integer(1), 1u8.to_ipld().to_owned());
        assert_eq!(Ipld::Integer(1), Ipld::from(1u16));
        assert_eq!(Ipld::Integer(1), Ipld::from(1u32));
        assert_eq!(Ipld::Integer(1), Ipld::from(1u64));
    }

    #[test]
    fn test_ipld_float_from() {
        assert_eq!(Ipld::Float(1.0), Ipld::from(1.0f32));
        assert_eq!(Ipld::Float(1.0), Ipld::from(1.0f64));
    }

    #[test]
    fn test_ipld_string_from() {
        assert_eq!(Ipld::String("a string".into()), Ipld::from("a string"));
        assert_eq!(
            Ipld::String("a string".into()),
            Ipld::from("a string".to_string())
        );
    }

    #[test]
    fn test_ipld_bytes_from() {
        assert_eq!(
            Ipld::Bytes(vec![0, 1, 2, 3]),
            Ipld::from(&[0u8, 1u8, 2u8, 3u8][..])
        );
        assert_eq!(
            Ipld::Bytes(vec![0, 1, 2, 3]),
            Ipld::from(vec![0u8, 1u8, 2u8, 3u8])
        );
    }

    #[test]
    fn test_ipld_link_from() {
        let data = vec![0, 1, 2, 3];
        let hash = Code::Blake3_256.digest(&data);
        let cid = Cid::new_v1(0x55, hash);
        assert_eq!(Ipld::Link(cid), Ipld::from(cid));
    }

    #[test]
    fn test_take() {
        let ipld = Ipld::List(vec![Ipld::Integer(0), Ipld::Integer(1), Ipld::Integer(2)]);
        assert_eq!(ipld.clone().take(0).unwrap(), Ipld::Integer(0));
        assert_eq!(ipld.clone().take(1).unwrap(), Ipld::Integer(1));
        assert_eq!(ipld.take(2).unwrap(), Ipld::Integer(2));

        let mut map = BTreeMap::new();
        map.insert("a".to_string(), Ipld::Integer(0));
        map.insert("b".to_string(), Ipld::Integer(1));
        map.insert("c".to_string(), Ipld::Integer(2));
        let ipld = Ipld::Map(map);
        assert_eq!(ipld.take("a").unwrap(), Ipld::Integer(0));
    }

    #[test]
    fn test_get() {
        let ipld = Ipld::List(vec![Ipld::Integer(0), Ipld::Integer(1), Ipld::Integer(2)]);
        assert_eq!(ipld.get(0).unwrap(), &Ipld::Integer(0));
        assert_eq!(ipld.get(1).unwrap(), &Ipld::Integer(1));
        assert_eq!(ipld.get(2).unwrap(), &Ipld::Integer(2));

        let mut map = BTreeMap::new();
        map.insert("a".to_string(), Ipld::Integer(0));
        map.insert("b".to_string(), Ipld::Integer(1));
        map.insert("c".to_string(), Ipld::Integer(2));
        let ipld = Ipld::Map(map);
        assert_eq!(ipld.get("a").unwrap(), &Ipld::Integer(0));
    }
}
