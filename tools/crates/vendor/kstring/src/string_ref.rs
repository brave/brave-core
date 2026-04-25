use std::fmt;

use crate::KStringBase;
use crate::KStringCowBase;

type StdString = std::string::String;
type BoxedStr = Box<str>;

/// A reference to a UTF-8 encoded, immutable string.
#[derive(Copy, Clone)]
#[repr(transparent)]
pub struct KStringRef<'s> {
    pub(crate) inner: KStringRefInner<'s>,
}

#[derive(Copy, Clone, Debug)]
pub(crate) enum KStringRefInner<'s> {
    Borrowed(&'s str),
    Singleton(&'static str),
}

impl<'s> KStringRef<'s> {
    /// Create a new empty `KStringBase`.
    #[inline]
    #[must_use]
    pub const fn new() -> Self {
        Self::from_static("")
    }

    /// Create a reference to a `'static` data.
    #[inline]
    #[must_use]
    pub const fn from_static(other: &'static str) -> Self {
        Self {
            inner: KStringRefInner::Singleton(other),
        }
    }

    /// Create a reference to a borrowed data.
    #[inline]
    #[must_use]
    pub fn from_ref(other: &'s str) -> Self {
        Self {
            inner: KStringRefInner::Borrowed(other),
        }
    }

    /// Clone the data into an owned-type.
    #[inline]
    #[must_use]
    #[allow(clippy::wrong_self_convention)]
    pub fn to_owned<B: crate::backend::HeapStr>(&self) -> KStringBase<B> {
        self.inner.to_owned()
    }

    /// Extracts a string slice containing the entire `KStringRef`.
    #[inline]
    #[must_use]
    pub fn as_str(&self) -> &str {
        self.inner.as_str()
    }

    /// Convert to a mutable string type, cloning the data if necessary.
    #[inline]
    #[must_use]
    pub fn into_mut(self) -> StdString {
        self.inner.into_mut()
    }
}

impl<'s> KStringRefInner<'s> {
    #[inline]
    #[allow(clippy::wrong_self_convention)]
    fn to_owned<B: crate::backend::HeapStr>(&self) -> KStringBase<B> {
        match self {
            Self::Borrowed(s) => KStringBase::from_ref(s),
            Self::Singleton(s) => KStringBase::from_static(s),
        }
    }

    #[inline]
    fn as_str(&self) -> &str {
        match self {
            Self::Borrowed(s) => s,
            Self::Singleton(s) => s,
        }
    }

    #[inline]
    fn into_mut(self) -> StdString {
        self.as_str().to_owned()
    }
}

impl<'s> std::ops::Deref for KStringRef<'s> {
    type Target = str;

    #[inline]
    fn deref(&self) -> &str {
        self.as_str()
    }
}

impl<'s> Eq for KStringRef<'s> {}

impl<'s> PartialEq<KStringRef<'s>> for KStringRef<'s> {
    #[inline]
    fn eq(&self, other: &KStringRef<'s>) -> bool {
        PartialEq::eq(self.as_str(), other.as_str())
    }
}

impl<'s> PartialEq<str> for KStringRef<'s> {
    #[inline]
    fn eq(&self, other: &str) -> bool {
        PartialEq::eq(self.as_str(), other)
    }
}

impl<'s> PartialEq<&'s str> for KStringRef<'s> {
    #[inline]
    fn eq(&self, other: &&str) -> bool {
        PartialEq::eq(self.as_str(), *other)
    }
}

impl<'s> PartialEq<String> for KStringRef<'s> {
    #[inline]
    fn eq(&self, other: &StdString) -> bool {
        PartialEq::eq(self.as_str(), other.as_str())
    }
}

impl<'s> Ord for KStringRef<'s> {
    #[inline]
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.as_str().cmp(other.as_str())
    }
}

impl<'s> PartialOrd for KStringRef<'s> {
    #[inline]
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl<'s> std::hash::Hash for KStringRef<'s> {
    #[inline]
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.as_str().hash(state);
    }
}

impl<'s> fmt::Debug for KStringRef<'s> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&self.inner, f)
    }
}

impl<'s> fmt::Display for KStringRef<'s> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Display::fmt(self.as_str(), f)
    }
}

impl<'s> AsRef<str> for KStringRef<'s> {
    #[inline]
    fn as_ref(&self) -> &str {
        self.as_str()
    }
}

impl<'s> AsRef<[u8]> for KStringRef<'s> {
    #[inline]
    fn as_ref(&self) -> &[u8] {
        self.as_bytes()
    }
}

impl<'s> AsRef<std::ffi::OsStr> for KStringRef<'s> {
    #[inline]
    fn as_ref(&self) -> &std::ffi::OsStr {
        (**self).as_ref()
    }
}

impl<'s> AsRef<std::path::Path> for KStringRef<'s> {
    #[inline]
    fn as_ref(&self) -> &std::path::Path {
        std::path::Path::new(self)
    }
}

impl<'s> std::borrow::Borrow<str> for KStringRef<'s> {
    #[inline]
    fn borrow(&self) -> &str {
        self.as_str()
    }
}

impl<'s> Default for KStringRef<'s> {
    #[inline]
    fn default() -> Self {
        Self::new()
    }
}

impl<'s, B: crate::backend::HeapStr> From<&'s KStringBase<B>> for KStringRef<'s> {
    #[inline]
    fn from(other: &'s KStringBase<B>) -> Self {
        other.as_ref()
    }
}

impl<'s, B: crate::backend::HeapStr> From<&'s KStringCowBase<'s, B>> for KStringRef<'s> {
    #[inline]
    fn from(other: &'s KStringCowBase<'s, B>) -> Self {
        other.as_ref()
    }
}

impl<'s> From<&'s StdString> for KStringRef<'s> {
    #[inline]
    fn from(other: &'s StdString) -> Self {
        KStringRef::from_ref(other.as_str())
    }
}

impl<'s> From<&'s BoxedStr> for KStringRef<'s> {
    #[inline]
    fn from(other: &'s BoxedStr) -> Self {
        Self::from_ref(other)
    }
}

impl<'s> From<&'s str> for KStringRef<'s> {
    #[inline]
    fn from(other: &'s str) -> Self {
        KStringRef::from_ref(other)
    }
}

#[cfg(feature = "serde")]
impl<'s> serde::Serialize for KStringRef<'s> {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl<'de: 's, 's> serde::Deserialize<'de> for KStringRef<'s> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let s: &'s str = serde::Deserialize::deserialize(deserializer)?;
        let s = KStringRef::from_ref(s);
        Ok(s)
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_size() {
        println!("KStringRef: {}", std::mem::size_of::<KStringRef<'static>>());
    }
}
