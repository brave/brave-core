use std::borrow::Cow;
use std::str::FromStr;

use crate::encode::{to_string_repr, StringStyle};
use crate::parser;
use crate::parser::key::is_unquoted_char;
use crate::repr::{Decor, Repr};
use crate::InternalString;

/// Key as part of a Key/Value Pair or a table header.
///
/// # Examples
///
/// ```notrust
/// [dependencies."nom"]
/// version = "5.0"
/// 'literal key' = "nonsense"
/// "basic string key" = 42
/// ```
///
/// There are 3 types of keys:
///
/// 1. Bare keys (`version` and `dependencies`)
///
/// 2. Basic quoted keys (`"basic string key"` and `"nom"`)
///
/// 3. Literal quoted keys (`'literal key'`)
///
/// For details see [toml spec](https://github.com/toml-lang/toml/#keyvalue-pair).
///
/// To parse a key use `FromStr` trait implementation: `"string".parse::<Key>()`.
#[derive(Debug)]
pub struct Key {
    key: InternalString,
    pub(crate) repr: Option<Repr>,
    pub(crate) decor: Decor,
}

impl Key {
    /// Create a new table key
    pub fn new(key: impl Into<InternalString>) -> Self {
        Self {
            key: key.into(),
            repr: None,
            decor: Default::default(),
        }
    }

    /// Parse a TOML key expression
    ///
    /// Unlike `"".parse<Key>()`, this supports dotted keys.
    pub fn parse(repr: &str) -> Result<Vec<Self>, crate::TomlError> {
        Self::try_parse_path(repr)
    }

    pub(crate) fn with_repr_unchecked(mut self, repr: Repr) -> Self {
        self.repr = Some(repr);
        self
    }

    /// While creating the `Key`, add `Decor` to it
    pub fn with_decor(mut self, decor: Decor) -> Self {
        self.decor = decor;
        self
    }

    /// Access a mutable proxy for the `Key`.
    pub fn as_mut(&mut self) -> KeyMut<'_> {
        KeyMut { key: self }
    }

    /// Returns the parsed key value.
    pub fn get(&self) -> &str {
        &self.key
    }

    pub(crate) fn get_internal(&self) -> &InternalString {
        &self.key
    }

    /// Returns key raw representation, if available.
    pub fn as_repr(&self) -> Option<&Repr> {
        self.repr.as_ref()
    }

    /// Returns the default raw representation.
    pub fn default_repr(&self) -> Repr {
        to_key_repr(&self.key)
    }

    /// Returns a raw representation.
    pub fn display_repr(&self) -> Cow<'_, str> {
        self.as_repr()
            .and_then(|r| r.as_raw().as_str())
            .map(Cow::Borrowed)
            .unwrap_or_else(|| {
                Cow::Owned(self.default_repr().as_raw().as_str().unwrap().to_owned())
            })
    }

    /// Returns the surrounding whitespace
    pub fn decor_mut(&mut self) -> &mut Decor {
        &mut self.decor
    }

    /// Returns the surrounding whitespace
    pub fn decor(&self) -> &Decor {
        &self.decor
    }

    /// Returns the location within the original document
    #[cfg(feature = "serde")]
    pub(crate) fn span(&self) -> Option<std::ops::Range<usize>> {
        self.repr.as_ref().and_then(|r| r.span())
    }

    pub(crate) fn despan(&mut self, input: &str) {
        self.decor.despan(input);
        if let Some(repr) = &mut self.repr {
            repr.despan(input)
        }
    }

    /// Auto formats the key.
    pub fn fmt(&mut self) {
        self.repr = Some(to_key_repr(&self.key));
        self.decor.clear();
    }

    fn try_parse_simple(s: &str) -> Result<Key, crate::TomlError> {
        let mut key = parser::parse_key(s)?;
        key.despan(s);
        Ok(key)
    }

    fn try_parse_path(s: &str) -> Result<Vec<Key>, crate::TomlError> {
        let mut keys = parser::parse_key_path(s)?;
        for key in &mut keys {
            key.despan(s);
        }
        Ok(keys)
    }
}

impl Clone for Key {
    #[inline(never)]
    fn clone(&self) -> Self {
        Self {
            key: self.key.clone(),
            repr: self.repr.clone(),
            decor: self.decor.clone(),
        }
    }
}

impl std::ops::Deref for Key {
    type Target = str;

    fn deref(&self) -> &Self::Target {
        self.get()
    }
}

impl std::hash::Hash for Key {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.get().hash(state);
    }
}

impl Ord for Key {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.get().cmp(other.get())
    }
}

impl PartialOrd for Key {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Eq for Key {}

impl PartialEq for Key {
    #[inline]
    fn eq(&self, other: &Key) -> bool {
        PartialEq::eq(self.get(), other.get())
    }
}

impl PartialEq<str> for Key {
    #[inline]
    fn eq(&self, other: &str) -> bool {
        PartialEq::eq(self.get(), other)
    }
}

impl<'s> PartialEq<&'s str> for Key {
    #[inline]
    fn eq(&self, other: &&str) -> bool {
        PartialEq::eq(self.get(), *other)
    }
}

impl PartialEq<String> for Key {
    #[inline]
    fn eq(&self, other: &String) -> bool {
        PartialEq::eq(self.get(), other.as_str())
    }
}

impl std::fmt::Display for Key {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        crate::encode::Encode::encode(self, f, None, ("", ""))
    }
}

impl FromStr for Key {
    type Err = crate::TomlError;

    /// Tries to parse a key from a &str,
    /// if fails, tries as basic quoted key (surrounds with "")
    /// and then literal quoted key (surrounds with '')
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Key::try_parse_simple(s)
    }
}

fn to_key_repr(key: &str) -> Repr {
    if key.as_bytes().iter().copied().all(is_unquoted_char) && !key.is_empty() {
        Repr::new_unchecked(key)
    } else {
        to_string_repr(key, Some(StringStyle::OnelineSingle), Some(false))
    }
}

impl<'b> From<&'b str> for Key {
    fn from(s: &'b str) -> Self {
        Key::new(s)
    }
}

impl<'b> From<&'b String> for Key {
    fn from(s: &'b String) -> Self {
        Key::new(s)
    }
}

impl From<String> for Key {
    fn from(s: String) -> Self {
        Key::new(s)
    }
}

impl From<InternalString> for Key {
    fn from(s: InternalString) -> Self {
        Key::new(s)
    }
}

#[doc(hidden)]
impl From<Key> for InternalString {
    fn from(key: Key) -> InternalString {
        key.key
    }
}

/// A mutable reference to a `Key`
#[derive(Debug, Eq, PartialEq, PartialOrd, Ord, Hash)]
pub struct KeyMut<'k> {
    key: &'k mut Key,
}

impl<'k> KeyMut<'k> {
    /// Returns the parsed key value.
    pub fn get(&self) -> &str {
        self.key.get()
    }

    /// Returns the raw representation, if available.
    pub fn as_repr(&self) -> Option<&Repr> {
        self.key.as_repr()
    }

    /// Returns the default raw representation.
    pub fn default_repr(&self) -> Repr {
        self.key.default_repr()
    }

    /// Returns a raw representation.
    pub fn display_repr(&self) -> Cow<str> {
        self.key.display_repr()
    }

    /// Returns the surrounding whitespace
    pub fn decor_mut(&mut self) -> &mut Decor {
        self.key.decor_mut()
    }

    /// Returns the surrounding whitespace
    pub fn decor(&self) -> &Decor {
        self.key.decor()
    }

    /// Auto formats the key.
    pub fn fmt(&mut self) {
        self.key.fmt()
    }
}

impl<'k> std::ops::Deref for KeyMut<'k> {
    type Target = str;

    fn deref(&self) -> &Self::Target {
        self.get()
    }
}

impl<'s> PartialEq<str> for KeyMut<'s> {
    #[inline]
    fn eq(&self, other: &str) -> bool {
        PartialEq::eq(self.get(), other)
    }
}

impl<'s> PartialEq<&'s str> for KeyMut<'s> {
    #[inline]
    fn eq(&self, other: &&str) -> bool {
        PartialEq::eq(self.get(), *other)
    }
}

impl<'s> PartialEq<String> for KeyMut<'s> {
    #[inline]
    fn eq(&self, other: &String) -> bool {
        PartialEq::eq(self.get(), other.as_str())
    }
}

impl<'k> std::fmt::Display for KeyMut<'k> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        std::fmt::Display::fmt(&self.key, f)
    }
}
