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

use cid::Cid;

/// Error when accessing IPLD List or Map elements.
#[derive(Clone, Debug)]
#[non_exhaustive]
pub enum IndexError {
    /// Error when key cannot be parsed into an integer.
    ParseInteger(String),
    /// Error when the input wasn't an IPLD List or Map.
    WrongKind(IpldKind),
}

impl fmt::Display for IndexError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::ParseInteger(key) => write!(f, "cannot parse key into integer: {}", key),
            Self::WrongKind(kind) => write!(f, "expected IPLD List or Map but found: {:?}", kind),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for IndexError {}

/// Ipld
#[derive(Clone)]
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

/// NaN floats are forbidden in the IPLD Data Model, but we do not enforce it. So in case such a
/// value is introduced accidentally, make sure that it still compares as equal. This allows us
/// to implement `Eq` for `Ipld`.
impl PartialEq for Ipld {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (Self::Null, Self::Null) => true,
            (Self::Bool(self_value), Self::Bool(other_value)) => self_value == other_value,
            (Self::Integer(self_value), Self::Integer(other_value)) => self_value == other_value,
            (Self::Float(self_value), Self::Float(other_value)) => {
                // Treat two NaNs as being equal.
                self_value == other_value || self_value.is_nan() && other_value.is_nan()
            }
            (Self::String(self_value), Self::String(other_value)) => self_value == other_value,
            (Self::Bytes(self_value), Self::Bytes(other_value)) => self_value == other_value,
            (Self::List(self_value), Self::List(other_value)) => self_value == other_value,
            (Self::Map(self_value), Self::Map(other_value)) => self_value == other_value,
            (Self::Link(self_value), Self::Link(other_value)) => self_value == other_value,
            _ => false,
        }
    }
}

impl Eq for Ipld {}

/// IPLD Kind information without the actual value.
///
/// Sometimes it's useful to know the kind of an Ipld object without the actual value, e.g. for
/// error reporting. Those kinds can be a unity-only enum.
#[derive(Clone, Debug)]
pub enum IpldKind {
    /// Null type.
    Null,
    /// Boolean type.
    Bool,
    /// Integer type.
    Integer,
    /// Float type.
    Float,
    /// String type.
    String,
    /// Bytes type.
    Bytes,
    /// List type.
    List,
    /// Map type.
    Map,
    /// Link type.
    Link,
}

/// An index into IPLD.
///
/// It's used for accessing IPLD List and Map elements.
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

impl<'a> TryFrom<IpldIndex<'a>> for usize {
    type Error = IndexError;

    fn try_from(index: IpldIndex<'a>) -> Result<Self, Self::Error> {
        let parsed = match index {
            IpldIndex::List(i) => i,
            IpldIndex::Map(ref key) => key
                .parse()
                .map_err(|_| IndexError::ParseInteger(key.to_string()))?,
            IpldIndex::MapRef(key) => key
                .parse()
                .map_err(|_| IndexError::ParseInteger(key.to_string()))?,
        };
        Ok(parsed)
    }
}

impl<'a> From<IpldIndex<'a>> for String {
    fn from(index: IpldIndex<'a>) -> Self {
        match index {
            IpldIndex::Map(ref key) => key.to_string(),
            IpldIndex::MapRef(key) => key.to_string(),
            IpldIndex::List(i) => i.to_string(),
        }
    }
}

impl Ipld {
    /// Convert from an [`Ipld`] object into its kind without any associated values.
    ///
    /// This is intentionally not implemented via `From<Ipld>` to prevent accidental conversions by
    /// making it more explicit.
    pub fn kind(&self) -> IpldKind {
        match self {
            Ipld::Null => IpldKind::Null,
            Ipld::Bool(_) => IpldKind::Bool,
            Ipld::Integer(_) => IpldKind::Integer,
            Ipld::Float(_) => IpldKind::Float,
            Ipld::String(_) => IpldKind::String,
            Ipld::Bytes(_) => IpldKind::Bytes,
            Ipld::List(_) => IpldKind::List,
            Ipld::Map(_) => IpldKind::Map,
            Ipld::Link(_) => IpldKind::Link,
        }
    }

    /// Destructs an ipld list or map
    pub fn take<'a, T: Into<IpldIndex<'a>>>(
        mut self,
        index: T,
    ) -> Result<Option<Self>, IndexError> {
        let index = index.into();
        match &mut self {
            Ipld::List(ref mut list) => {
                let parsed_index = usize::try_from(index)?;
                if parsed_index < list.len() {
                    Ok(Some(list.swap_remove(parsed_index)))
                } else {
                    Ok(None)
                }
            }
            Ipld::Map(ref mut map) => {
                let key = String::from(index);
                Ok(map.remove(&key))
            }
            other => Err(IndexError::WrongKind(other.kind())),
        }
    }

    /// Indexes into an ipld list or map.
    pub fn get<'a, T: Into<IpldIndex<'a>>>(&self, index: T) -> Result<Option<&Self>, IndexError> {
        let index = index.into();
        match self {
            Ipld::List(list) => {
                let parsed_index = usize::try_from(index)?;
                Ok(list.get(parsed_index))
            }
            Ipld::Map(map) => {
                let key = String::from(index);
                Ok(map.get(&key))
            }
            other => Err(IndexError::WrongKind(other.kind())),
        }
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
        let cid =
            Cid::try_from("bafkreie74tgmnxqwojhtumgh5dzfj46gi4mynlfr7dmm7duwzyvnpw7h7m").unwrap();
        assert_eq!(Ipld::Link(cid), Ipld::from(cid));
    }

    #[test]
    fn test_take() {
        let ipld = Ipld::List(vec![Ipld::Integer(0), Ipld::Integer(1), Ipld::Integer(2)]);
        assert_eq!(ipld.clone().take(0).unwrap(), Some(Ipld::Integer(0)));
        assert_eq!(ipld.clone().take(1).unwrap(), Some(Ipld::Integer(1)));
        assert_eq!(ipld.take(2).unwrap(), Some(Ipld::Integer(2)));

        let mut map = BTreeMap::new();
        map.insert("a".to_string(), Ipld::Integer(0));
        map.insert("b".to_string(), Ipld::Integer(1));
        map.insert("c".to_string(), Ipld::Integer(2));
        let ipld = Ipld::Map(map);
        assert_eq!(ipld.take("a").unwrap(), Some(Ipld::Integer(0)));
    }

    #[test]
    fn test_get() {
        let ipld = Ipld::List(vec![Ipld::Integer(0), Ipld::Integer(1), Ipld::Integer(2)]);
        assert_eq!(ipld.get(0).unwrap(), Some(&Ipld::Integer(0)));
        assert_eq!(ipld.get(1).unwrap(), Some(&Ipld::Integer(1)));
        assert_eq!(ipld.get(2).unwrap(), Some(&Ipld::Integer(2)));

        let mut map = BTreeMap::new();
        map.insert("a".to_string(), Ipld::Integer(0));
        map.insert("b".to_string(), Ipld::Integer(1));
        map.insert("c".to_string(), Ipld::Integer(2));
        let ipld = Ipld::Map(map);
        assert_eq!(ipld.get("a").unwrap(), Some(&Ipld::Integer(0)));
    }

    // NaN floats are forbidden in the IPLD Data Model, but still make sure they are treated as
    // equal in case they accidentally end up there.
    #[test]
    fn test_partial_eq_nan() {
        let invalid_ipld = Ipld::Float(f64::NAN);
        assert_eq!(invalid_ipld, invalid_ipld);
    }
}
