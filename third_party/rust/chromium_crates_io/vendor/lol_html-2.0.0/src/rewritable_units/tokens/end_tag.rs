use super::{Mutations, Token};
use crate::base::Bytes;
use crate::rewritable_units::ContentType;
use encoding_rs::Encoding;
use std::fmt::{self, Debug};

/// An HTML end tag rewritable unit.
///
/// Exposes API for examination and modification of a parsed HTML end tag.
pub struct EndTag<'i> {
    name: Bytes<'i>,
    raw: Option<Bytes<'i>>,
    encoding: &'static Encoding,
    pub(crate) mutations: Mutations,
}

impl<'i> EndTag<'i> {
    pub(super) fn new_token(
        name: Bytes<'i>,
        raw: Bytes<'i>,
        encoding: &'static Encoding,
    ) -> Token<'i> {
        Token::EndTag(EndTag {
            name,
            raw: Some(raw),
            encoding,
            mutations: Mutations::new(encoding),
        })
    }

    /// Returns the name of the tag.
    #[inline]
    pub fn name(&self) -> String {
        self.name.as_lowercase_string(self.encoding)
    }

    /// Returns the name of the tag, preserving its case.
    #[inline]
    pub fn name_preserve_case(&self) -> String {
        self.name.as_string(self.encoding)
    }

    /// Sets the name of the tag.
    #[inline]
    pub fn set_name(&mut self, name: Bytes<'static>) {
        self.name = name;
        self.raw = None;
    }

    /// Sets the name of the tag by encoding the given string.
    #[inline]
    pub fn set_name_str(&mut self, name: String) {
        self.set_name(Bytes::from_string(name, self.encoding))
    }

    /// Inserts `content` before the end tag.
    ///
    /// Consequent calls to the method append `content` to the previously inserted content.
    #[inline]
    pub fn before(&mut self, content: &str, content_type: ContentType) {
        self.mutations.before(content, content_type);
    }

    /// Inserts `content` after the end tag.
    ///
    /// Consequent calls to the method prepend `content` to the previously inserted content.
    #[inline]
    pub fn after(&mut self, content: &str, content_type: ContentType) {
        self.mutations.after(content, content_type);
    }

    /// Replaces the end tag with `content`.
    ///
    /// Consequent calls to the method overwrite previous replacement content.
    #[inline]
    pub fn replace(&mut self, content: &str, content_type: ContentType) {
        self.mutations.replace(content, content_type);
    }

    /// Removes the end tag.
    #[inline]
    pub fn remove(&mut self) {
        self.mutations.remove();
    }

    #[inline]
    fn raw(&self) -> Option<&Bytes> {
        self.raw.as_ref()
    }

    #[inline]
    fn serialize_from_parts(&self, output_handler: &mut dyn FnMut(&[u8])) {
        output_handler(b"</");
        output_handler(&self.name);
        output_handler(b">");
    }
}

impl_serialize!(EndTag);

impl Debug for EndTag<'_> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("EndTag")
            .field("name", &self.name())
            .finish()
    }
}
