use std::borrow::Cow;

use ::bstr::{BStr, BString};

/// Provide a borrowed byte-string view of common string and byte containers.
///
/// Unlike [`AsRef`], this conversion deliberately bridges representations such
/// as [`String`] and [`Vec<u8>`] to [`BStr`] without allocating.
pub trait AsBStr {
    /// Return this value as a borrowed byte string.
    fn as_bstr(&self) -> &BStr;
}

/// Provide an optional borrowed byte-string view of common string and byte containers.
pub trait AsBStrOpt {
    /// Return this value as a borrowed byte string, or `None` if it is absent.
    fn as_bstr_opt(&self) -> Option<&BStr>;
}

impl<T: AsBStr + ?Sized> AsBStrOpt for T {
    fn as_bstr_opt(&self) -> Option<&BStr> {
        Some(self.as_bstr())
    }
}

impl AsBStrOpt for Option<&BStr> {
    fn as_bstr_opt(&self) -> Option<&BStr> {
        *self
    }
}

impl<T: AsBStr + ?Sized> AsBStr for &T {
    fn as_bstr(&self) -> &BStr {
        T::as_bstr(self)
    }
}

impl<T: AsBStr + ?Sized> AsBStr for &mut T {
    fn as_bstr(&self) -> &BStr {
        T::as_bstr(self)
    }
}

impl AsBStr for BStr {
    fn as_bstr(&self) -> &BStr {
        self
    }
}

impl AsBStr for BString {
    fn as_bstr(&self) -> &BStr {
        BStr::new(self.as_slice())
    }
}

impl AsBStr for str {
    fn as_bstr(&self) -> &BStr {
        BStr::new(self)
    }
}

impl AsBStr for String {
    fn as_bstr(&self) -> &BStr {
        BStr::new(self)
    }
}

impl AsBStr for [u8] {
    fn as_bstr(&self) -> &BStr {
        BStr::new(self)
    }
}

impl AsBStr for Vec<u8> {
    fn as_bstr(&self) -> &BStr {
        BStr::new(self)
    }
}

impl<const N: usize> AsBStr for [u8; N] {
    fn as_bstr(&self) -> &BStr {
        BStr::new(self)
    }
}

impl<T> AsBStr for Cow<'_, T>
where
    T: AsBStr + ToOwned + ?Sized,
{
    fn as_bstr(&self) -> &BStr {
        self.as_ref().as_bstr()
    }
}
