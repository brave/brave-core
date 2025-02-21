use super::{Mutations, Token};
use crate::base::Bytes;
use crate::errors::RewritingError;
use crate::html_content::{ContentType, StreamingHandler};
use crate::rewritable_units::StringChunk;
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
    #[inline]
    #[must_use]
    pub(super) fn new_token(
        name: Bytes<'i>,
        raw: Bytes<'i>,
        encoding: &'static Encoding,
    ) -> Token<'i> {
        Token::EndTag(EndTag {
            name,
            raw: Some(raw),
            encoding,
            mutations: Mutations::new(),
        })
    }

    /// Returns the name of the tag.
    #[inline]
    #[must_use]
    pub fn name(&self) -> String {
        self.name.as_lowercase_string(self.encoding)
    }

    /// Returns the name of the tag, preserving its case.
    #[inline]
    #[must_use]
    pub fn name_preserve_case(&self) -> String {
        self.name.as_string(self.encoding)
    }

    #[inline]
    #[doc(hidden)]
    #[deprecated(note = "use set_name_str")]
    pub fn set_name(&mut self, name: Bytes<'static>) {
        self.set_name_raw(name);
    }

    /// Sets the name of the tag.
    pub(crate) fn set_name_raw(&mut self, name: Bytes<'static>) {
        self.name = name;
        self.raw = None;
    }

    /// Sets the name of the tag by encoding the given string.
    #[inline]
    pub fn set_name_str(&mut self, name: String) {
        self.set_name_raw(Bytes::from_string(name, self.encoding));
    }

    /// Inserts `content` before the end tag.
    ///
    /// Consequent calls to the method append `content` to the previously inserted content.
    #[inline]
    pub fn before(&mut self, content: &str, content_type: ContentType) {
        self.mutations
            .mutate()
            .content_before
            .push_back(StringChunk::from_str(content, content_type));
    }

    /// Inserts `content` after the end tag.
    ///
    /// Consequent calls to the method prepend `content` to the previously inserted content.
    #[inline]
    pub fn after(&mut self, content: &str, content_type: ContentType) {
        self.mutations
            .mutate()
            .content_after
            .push_front(StringChunk::from_str(content, content_type));
    }

    /// Replaces the end tag with `content`.
    ///
    /// Consequent calls to the method overwrite previous replacement content.
    #[inline]
    pub fn replace(&mut self, content: &str, content_type: ContentType) {
        self.mutations
            .mutate()
            .replace(StringChunk::from_str(content, content_type));
    }

    /// Inserts content from a [`StreamingHandler`] before the end tag.
    ///
    /// Consequent calls to the method append to the previously inserted content.
    ///
    /// Use the [`streaming!`] macro to make a `StreamingHandler` from a closure.
    #[inline]
    pub fn streaming_before(&mut self, string_writer: Box<dyn StreamingHandler + Send>) {
        self.mutations
            .mutate()
            .content_before
            .push_back(StringChunk::stream(string_writer));
    }

    /// Inserts content from a [`StreamingHandler`] after the end tag.
    ///
    /// Consequent calls to the method prepend to the previously inserted content.
    ///
    /// Use the [`streaming!`] macro to make a `StreamingHandler` from a closure.
    #[inline]
    pub fn streaming_after(&mut self, string_writer: Box<dyn StreamingHandler + Send>) {
        self.mutations
            .mutate()
            .content_after
            .push_front(StringChunk::stream(string_writer));
    }

    /// Replaces the end tag with content from a [`StreamingHandler`].
    ///
    /// Consequent calls to the method overwrite previous replacement content.
    ///
    /// Use the [`streaming!`] macro to make a `StreamingHandler` from a closure.
    #[inline]
    pub fn streaming_replace(&mut self, string_writer: Box<dyn StreamingHandler + Send>) {
        self.mutations
            .mutate()
            .replace(StringChunk::stream(string_writer));
    }

    /// Removes the end tag.
    #[inline]
    pub fn remove(&mut self) {
        self.mutations.mutate().remove();
    }

    #[inline]
    fn serialize_self(&self, output_handler: &mut dyn FnMut(&[u8])) -> Result<(), RewritingError> {
        if let Some(raw) = &self.raw {
            output_handler(raw);
        } else {
            output_handler(b"</");
            output_handler(&self.name);
            output_handler(b">");
        }
        Ok(())
    }
}

impl_serialize!(EndTag);

impl Debug for EndTag<'_> {
    #[cold]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("EndTag")
            .field("name", &self.name())
            .finish()
    }
}
