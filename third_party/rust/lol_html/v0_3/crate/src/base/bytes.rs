use super::Range;
use encoding_rs::{Encoding, WINDOWS_1252};
use memchr::{memchr, memchr3};
use std::borrow::Cow;
use std::fmt::{self, Debug};
use std::ops::Deref;
use std::str;

/// An error used to indicate that an encoded string has replacements and can't be converted losslessly.
#[derive(Debug, Default, PartialEq, Eq, PartialOrd, Ord)]
pub struct HasReplacementsError;

/// A thin wrapper around either byte slice or owned bytes with some handy APIs attached
#[derive(Clone, PartialEq, Eq, Hash)]
pub struct Bytes<'b>(Cow<'b, [u8]>);

impl<'b> Bytes<'b> {
    #[inline]
    pub fn from_str(string: &'b str, encoding: &'static Encoding) -> Self {
        encoding.encode(string).0.into()
    }

    /// Same as `Bytes::from_str(&string).into_owned()`, but avoids copying in the common case where
    /// the output and input encodings are the same.
    pub fn from_string(string: String, encoding: &'static Encoding) -> Bytes<'static> {
        Bytes(Cow::Owned(match encoding.encode(&string).0 {
            Cow::Owned(bytes) => bytes,
            Cow::Borrowed(_) => string.into_bytes(),
        }))
    }

    #[inline]
    pub fn from_str_without_replacements(
        string: &'b str,
        encoding: &'static Encoding,
    ) -> Result<Self, HasReplacementsError> {
        let (res, _, has_replacements) = encoding.encode(string);

        if has_replacements {
            Err(HasReplacementsError)
        } else {
            Ok(res.into())
        }
    }

    #[inline]
    pub fn as_string(&self, encoding: &'static Encoding) -> String {
        encoding.decode(self).0.into_owned()
    }

    #[inline]
    pub fn as_lowercase_string(&self, encoding: &'static Encoding) -> String {
        encoding.decode(self).0.to_ascii_lowercase()
    }

    #[inline]
    pub fn into_owned(self) -> Bytes<'static> {
        Bytes(Cow::Owned(self.0.into_owned()))
    }

    #[inline]
    pub fn slice(&self, range: Range) -> Bytes {
        self.0[range.start..range.end].into()
    }

    #[inline]
    pub fn opt_slice(&self, range: Option<Range>) -> Option<Bytes> {
        range.map(|range| self.slice(range))
    }

    pub(crate) fn as_debug_string(&self) -> String {
        // NOTE: use WINDOWS_1252 (superset of ASCII) encoding here as
        // the most safe variant since we don't know which actual encoding
        // has been used for bytes.
        self.as_string(WINDOWS_1252)
    }
}

macro_rules! impl_replace_byte {
    ($self:tt, $output_handler:ident, $impls:ident) => {
        let mut tail: &[u8] = $self;

        loop {
            match $impls!(@find tail) {
                Some(pos) => {
                    let replacement = $impls!(@get_replacement tail, pos);
                    let chunk = &tail[..pos];

                    if !chunk.is_empty() {
                        $output_handler(chunk);
                    }

                    $output_handler(&replacement);
                    tail = &tail[pos + 1..];
                }
                None => {
                    if !tail.is_empty() {
                        $output_handler(&tail);
                    }
                    break;
                }
            }
        }
    };
}

impl<'b> Bytes<'b> {
    #[inline]
    pub fn replace_byte(&self, (needle, repl): (u8, &[u8]), output_handler: &mut dyn FnMut(&[u8])) {
        macro_rules! impls {
            (@find $tail:ident) => {
                memchr(needle, $tail)
            };

            (@get_replacement $tail:ident, $pos:ident) => {
                repl
            };
        }

        impl_replace_byte!(self, output_handler, impls);
    }

    #[inline]
    pub fn replace_byte3(
        &self,
        (needle1, repl1): (u8, &[u8]),
        (needle2, repl2): (u8, &[u8]),
        (needle3, repl3): (u8, &[u8]),
        output_handler: &mut dyn FnMut(&[u8]),
    ) {
        macro_rules! impls {
            (@find $tail:ident) => {
                memchr3(needle1, needle2, needle3, $tail)
            };

            (@get_replacement $tail:ident, $pos:ident) => {{
                let matched = $tail[$pos];

                if matched == needle1 {
                    repl1
                } else if matched == needle2 {
                    repl2
                } else {
                    repl3
                }
            }};
        }

        impl_replace_byte!(self, output_handler, impls);
    }
}

impl<'b> From<Cow<'b, [u8]>> for Bytes<'b> {
    #[inline]
    fn from(bytes: Cow<'b, [u8]>) -> Self {
        Bytes(bytes)
    }
}

impl<'b> From<&'b [u8]> for Bytes<'b> {
    #[inline]
    fn from(bytes: &'b [u8]) -> Self {
        Bytes(bytes.into())
    }
}

impl Debug for Bytes<'_> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "`{}`", self.as_debug_string())
    }
}

impl Deref for Bytes<'_> {
    type Target = [u8];

    fn deref(&self) -> &[u8] {
        &*self.0
    }
}
