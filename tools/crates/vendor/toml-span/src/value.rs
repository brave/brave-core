//! Contains the [`Value`] and [`ValueInner`] containers into which all toml
//! contents can be deserialized into and either used directly or fed into
//! [`crate::Deserialize`] or your own constructs to deserialize into your own
//! types

use crate::{Error, ErrorKind, Span};
use std::{borrow::Cow, fmt};

/// A deserialized [`ValueInner`] with accompanying [`Span`] information for where
/// it was located in the toml document
pub struct Value<'de> {
    value: Option<ValueInner<'de>>,
    /// The location of the value in the toml document
    pub span: Span,
}

impl<'de> Value<'de> {
    /// Creates a new [`Value`] with an empty [`Span`]
    #[inline]
    pub fn new(value: ValueInner<'de>) -> Self {
        Self::with_span(value, Span::default())
    }

    /// Creates a new [`Value`] with the specified [`Span`]
    #[inline]
    pub fn with_span(value: ValueInner<'de>, span: Span) -> Self {
        Self {
            value: Some(value),
            span,
        }
    }

    /// Takes the inner [`ValueInner`]
    ///
    /// This panics if the inner value has already been taken.
    ///
    /// Typically paired with [`Self::set`]
    #[inline]
    pub fn take(&mut self) -> ValueInner<'de> {
        self.value.take().expect("the value has already been taken")
    }

    /// Sets the inner [`ValueInner`]
    ///
    /// This is typically done when the value is taken with [`Self::take`],
    /// processed, and returned
    #[inline]
    pub fn set(&mut self, value: ValueInner<'de>) {
        self.value = Some(value);
    }

    /// Returns true if the value is a table and is non-empty
    #[inline]
    pub fn has_keys(&self) -> bool {
        self.value.as_ref().is_some_and(|val| {
            if let ValueInner::Table(table) = val {
                !table.is_empty()
            } else {
                false
            }
        })
    }

    /// Returns true if the value is a table and has the specified key
    #[inline]
    pub fn has_key(&self, key: &str) -> bool {
        self.value.as_ref().is_some_and(|val| {
            if let ValueInner::Table(table) = val {
                table.contains_key(key)
            } else {
                false
            }
        })
    }

    /// Takes the value as a string, returning an error with either a default
    /// or user supplied message
    #[inline]
    pub fn take_string(&mut self, msg: Option<&'static str>) -> Result<Cow<'de, str>, Error> {
        match self.take() {
            ValueInner::String(s) => Ok(s),
            other => Err(Error {
                kind: ErrorKind::Wanted {
                    expected: msg.unwrap_or("a string"),
                    found: other.type_str(),
                },
                span: self.span,
                line_info: None,
            }),
        }
    }

    /// Returns a borrowed string if this is a [`ValueInner::String`]
    #[inline]
    pub fn as_str(&self) -> Option<&str> {
        self.value.as_ref().and_then(|v| v.as_str())
    }

    /// Returns a borrowed table if this is a [`ValueInner::Table`]
    #[inline]
    pub fn as_table(&self) -> Option<&Table<'de>> {
        self.value.as_ref().and_then(|v| v.as_table())
    }

    /// Returns a borrowed array if this is a [`ValueInner::Array`]
    #[inline]
    pub fn as_array(&self) -> Option<&Array<'de>> {
        self.value.as_ref().and_then(|v| v.as_array())
    }

    /// Returns an `i64` if this is a [`ValueInner::Integer`]
    #[inline]
    pub fn as_integer(&self) -> Option<i64> {
        self.value.as_ref().and_then(|v| v.as_integer())
    }

    /// Returns an `f64` if this is a [`ValueInner::Float`]
    #[inline]
    pub fn as_float(&self) -> Option<f64> {
        self.value.as_ref().and_then(|v| v.as_float())
    }

    /// Returns a `bool` if this is a [`ValueInner::Boolean`]
    #[inline]
    pub fn as_bool(&self) -> Option<bool> {
        self.value.as_ref().and_then(|v| v.as_bool())
    }

    /// Uses JSON pointer-like syntax to lookup a specific [`Value`]
    ///
    /// The basic format is:
    ///
    /// - The path starts with `/`
    /// - Each segment is separated by a `/`
    /// - Each segment is either a key name, or an integer array index
    ///
    /// ```rust
    /// let data = "[x]\ny = ['z', 'zz']";
    /// let value = toml_span::parse(data).unwrap();
    /// assert_eq!(value.pointer("/x/y/1").unwrap().as_str().unwrap(), "zz");
    /// assert!(value.pointer("/a/b/c").is_none());
    /// ```
    ///
    /// Note that this is JSON pointer**-like** because `/` is not supported in
    /// key names because I don't see the point. If you want this it is easy to
    /// implement.
    pub fn pointer(&self, pointer: &str) -> Option<&Self> {
        if pointer.is_empty() {
            return Some(self);
        } else if !pointer.starts_with('/') {
            return None;
        }

        pointer
            .split('/')
            .skip(1)
            // Don't support / or ~ in key names unless someone actually opens
            // an issue about it
            //.map(|x| x.replace("~1", "/").replace("~0", "~"))
            .try_fold(self, move |target, token| {
                (match &target.value {
                    Some(ValueInner::Table(tab)) => tab.get(token),
                    Some(ValueInner::Array(list)) => parse_index(token).and_then(|x| list.get(x)),
                    _ => None,
                })
                .filter(|v| v.value.is_some())
            })
    }

    /// The `mut` version of [`Self::pointer`]
    pub fn pointer_mut(&mut self, pointer: &'de str) -> Option<&mut Self> {
        if pointer.is_empty() {
            return Some(self);
        } else if !pointer.starts_with('/') {
            return None;
        }

        pointer
            .split('/')
            .skip(1)
            // Don't support / or ~ in key names unless someone actually opens
            // an issue about it
            //.map(|x| x.replace("~1", "/").replace("~0", "~"))
            .try_fold(self, |target, token| {
                (match &mut target.value {
                    Some(ValueInner::Table(tab)) => tab.get_mut(token),
                    Some(ValueInner::Array(list)) => {
                        parse_index(token).and_then(|x| list.get_mut(x))
                    }
                    _ => None,
                })
                .filter(|v| v.value.is_some())
            })
    }
}

fn parse_index(s: &str) -> Option<usize> {
    if s.starts_with('+') || (s.starts_with('0') && s.len() != 1) {
        return None;
    }
    s.parse().ok()
}

impl<'de> AsRef<ValueInner<'de>> for Value<'de> {
    fn as_ref(&self) -> &ValueInner<'de> {
        self.value
            .as_ref()
            .expect("the value has already been taken")
    }
}

impl fmt::Debug for Value<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:?}", self.value)
    }
}

/// A toml table key
#[derive(Clone)]
pub struct Key<'de> {
    /// The key itself, in most cases it will be borrowed, but may be owned
    /// if escape characters are present in the original source
    pub name: Cow<'de, str>,
    /// The span for the key in the original document
    pub span: Span,
}

impl std::borrow::Borrow<str> for Key<'_> {
    fn borrow(&self) -> &str {
        self.name.as_ref()
    }
}

impl fmt::Debug for Key<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.name)
    }
}

impl fmt::Display for Key<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.name)
    }
}

impl Ord for Key<'_> {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.name.cmp(&other.name)
    }
}

impl PartialOrd for Key<'_> {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl PartialEq for Key<'_> {
    fn eq(&self, other: &Self) -> bool {
        self.name.eq(&other.name)
    }
}

impl Eq for Key<'_> {}

/// A toml table, always represented as a sorted map.
///
/// The original key ordering can be obtained by ordering the keys by their span
pub type Table<'de> = std::collections::BTreeMap<Key<'de>, Value<'de>>;
/// A toml array
pub type Array<'de> = Vec<Value<'de>>;

/// The core value types that toml can deserialize to
///
/// Note that this library does not support datetime values that are part of the
/// toml spec since I have no need of them, but could be added
#[derive(Debug)]
pub enum ValueInner<'de> {
    /// A string.
    ///
    /// This will be borrowed from the original toml source unless it contains
    /// escape characters
    String(Cow<'de, str>),
    /// An integer
    Integer(i64),
    /// A float
    Float(f64),
    /// A boolean
    Boolean(bool),
    /// An array
    Array(Array<'de>),
    /// A table
    Table(Table<'de>),
}

impl<'de> ValueInner<'de> {
    /// Gets the type of the value as a string
    pub fn type_str(&self) -> &'static str {
        match self {
            Self::String(..) => "string",
            Self::Integer(..) => "integer",
            Self::Float(..) => "float",
            Self::Boolean(..) => "boolean",
            Self::Array(..) => "array",
            Self::Table(..) => "table",
        }
    }

    /// Returns a borrowed string if this is a [`Self::String`]
    #[inline]
    pub fn as_str(&self) -> Option<&str> {
        if let Self::String(s) = self {
            Some(s.as_ref())
        } else {
            None
        }
    }

    /// Returns a borrowed table if this is a [`Self::Table`]
    #[inline]
    pub fn as_table(&self) -> Option<&Table<'de>> {
        if let ValueInner::Table(t) = self {
            Some(t)
        } else {
            None
        }
    }

    /// Returns a borrowed array if this is a [`Self::Array`]
    #[inline]
    pub fn as_array(&self) -> Option<&Array<'de>> {
        if let ValueInner::Array(a) = self {
            Some(a)
        } else {
            None
        }
    }

    /// Returns an `i64` if this is a [`Self::Integer`]
    #[inline]
    pub fn as_integer(&self) -> Option<i64> {
        if let ValueInner::Integer(i) = self {
            Some(*i)
        } else {
            None
        }
    }

    /// Returns an `f64` if this is a [`Self::Float`]
    #[inline]
    pub fn as_float(&self) -> Option<f64> {
        if let ValueInner::Float(f) = self {
            Some(*f)
        } else {
            None
        }
    }

    /// Returns a `bool` if this is a [`Self::Boolean`]
    #[inline]
    pub fn as_bool(&self) -> Option<bool> {
        if let ValueInner::Boolean(b) = self {
            Some(*b)
        } else {
            None
        }
    }
}
