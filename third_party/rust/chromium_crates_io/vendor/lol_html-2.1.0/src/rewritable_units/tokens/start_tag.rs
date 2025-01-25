use super::{Attribute, AttributeNameError, Attributes};
use super::{Mutations, Serialize, Token};
use crate::base::Bytes;
use crate::errors::RewritingError;
use crate::html::Namespace;
use crate::html_content::{ContentType, StreamingHandler};
use crate::rewritable_units::StringChunk;
use encoding_rs::Encoding;
use std::fmt::{self, Debug};

/// An HTML start tag rewritable unit.
///
/// Exposes API for examination and modification of a parsed HTML start tag.
pub struct StartTag<'i> {
    name: Bytes<'i>,
    attributes: Attributes<'i>,
    ns: Namespace,
    self_closing: bool,
    raw: Option<Bytes<'i>>,
    encoding: &'static Encoding,
    pub(crate) mutations: Mutations,
}

impl<'i> StartTag<'i> {
    #[inline]
    #[must_use]
    pub(super) fn new_token(
        name: Bytes<'i>,
        attributes: Attributes<'i>,
        ns: Namespace,
        self_closing: bool,
        raw: Bytes<'i>,
        encoding: &'static Encoding,
    ) -> Token<'i> {
        Token::StartTag(StartTag {
            name,
            attributes,
            ns,
            self_closing,
            raw: Some(raw),
            encoding,
            mutations: Mutations::new(),
        })
    }

    #[inline]
    #[doc(hidden)]
    pub const fn encoding(&self) -> &'static Encoding {
        self.encoding
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

    /// Returns the [namespace URI] of the tag's element.
    ///
    /// [namespace URI]: https://developer.mozilla.org/en-US/docs/Web/API/Element/namespaceURI
    #[inline]
    pub fn namespace_uri(&self) -> &'static str {
        self.ns.uri()
    }

    /// Returns an immutable collection of tag's attributes.
    #[inline]
    pub fn attributes(&self) -> &[Attribute<'i>] {
        &self.attributes
    }

    /// Sets `value` of tag's attribute with `name`.
    ///
    /// If tag doesn't have an attribute with the `name`, method adds a new attribute
    /// to the tag with `name` and `value`.
    #[inline]
    pub fn set_attribute(&mut self, name: &str, value: &str) -> Result<(), AttributeNameError> {
        self.attributes.set_attribute(name, value, self.encoding)?;
        self.raw = None;

        Ok(())
    }

    /// Removes an attribute with the `name` if it is present.
    #[inline]
    pub fn remove_attribute(&mut self, name: &str) {
        if self.attributes.remove_attribute(name) {
            self.raw = None;
        }
    }

    /// Whether the tag is explicitly self-closing, e.g. `<foo />`.
    #[inline]
    pub fn self_closing(&self) -> bool {
        self.self_closing
    }

    /// Inserts `content` before the start tag.
    ///
    /// Consequent calls to the method append `content` to the previously inserted content.
    #[inline]
    pub fn before(&mut self, content: &str, content_type: ContentType) {
        self.mutations
            .mutate()
            .content_before
            .push_back(StringChunk::from_str(content, content_type));
    }

    /// Inserts `content` after the start tag.
    ///
    /// Consequent calls to the method prepend `content` to the previously inserted content.
    #[inline]
    pub fn after(&mut self, content: &str, content_type: ContentType) {
        self.mutations
            .mutate()
            .content_after
            .push_front(StringChunk::from_str(content, content_type));
    }

    /// Replaces the start tag with `content`.
    ///
    /// Consequent calls to the method overwrite previous replacement content.
    #[inline]
    pub fn replace(&mut self, content: &str, content_type: ContentType) {
        self.mutations
            .mutate()
            .replace(StringChunk::from_str(content, content_type));
    }

    /// Inserts content from a [`StreamingHandler`] before the start tag.
    ///
    /// Consequent calls to the method append to the previously inserted content.
    ///
    /// Use the [`streaming!`] macro to make a `StreamingHandler` from a closure.
    pub fn streaming_before(&mut self, string_writer: Box<dyn StreamingHandler + Send>) {
        self.mutations
            .mutate()
            .content_before
            .push_back(StringChunk::stream(string_writer));
    }

    /// Inserts content from a [`StreamingHandler`] after the start tag.
    ///
    /// Consequent calls to the method prepend to the previously inserted content.
    ///
    /// Use the [`streaming!`] macro to make a `StreamingHandler` from a closure.
    pub fn streaming_after(&mut self, string_writer: Box<dyn StreamingHandler + Send>) {
        self.mutations
            .mutate()
            .content_after
            .push_front(StringChunk::stream(string_writer));
    }

    /// Replaces the start tag with the content from a [`StreamingHandler`].
    ///
    /// Consequent calls to the method overwrite previous replacement content.
    ///
    /// Use the [`streaming!`] macro to make a `StreamingHandler` from a closure.
    pub fn streaming_replace(&mut self, string_writer: Box<dyn StreamingHandler + Send>) {
        self.mutations
            .mutate()
            .replace(StringChunk::stream(string_writer));
    }

    /// Removes the start tag.
    #[inline]
    pub fn remove(&mut self) {
        self.mutations.mutate().remove();
    }

    fn serialize_self(&self, output_handler: &mut dyn FnMut(&[u8])) -> Result<(), RewritingError> {
        if let Some(raw) = &self.raw {
            output_handler(raw);
            return Ok(());
        }

        output_handler(b"<");
        output_handler(&self.name);

        if !self.attributes.is_empty() {
            output_handler(b" ");

            self.attributes.into_bytes(output_handler)?;

            // NOTE: attributes can be modified the way that
            // last attribute has an unquoted value. We always
            // add extra space before the `/`, because otherwise
            // it will be treated as a part of such an unquoted
            // attribute value.
            if self.self_closing {
                output_handler(b" ");
            }
        }

        if self.self_closing {
            output_handler(b"/>");
        } else {
            output_handler(b">");
        }
        Ok(())
    }

    #[cfg(test)]
    pub(crate) const fn raw_attributes(
        &self,
    ) -> (&'i Bytes<'i>, &'i crate::parser::AttributeBuffer) {
        self.attributes.raw_attributes()
    }
}

impl_serialize!(StartTag);

impl Debug for StartTag<'_> {
    #[cold]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("StartTag")
            .field("name", &self.name())
            .field("attributes", &self.attributes())
            .field("self_closing", &self.self_closing)
            .finish()
    }
}
