use crate::base::Bytes;
use crate::errors::RewritingError;
use crate::html::escape_double_quotes_only;
use crate::parser::AttributeBuffer;
use crate::rewritable_units::Serialize;
use encoding_rs::Encoding;
use lazycell::LazyCell;
use std::fmt::{self, Debug};
use std::ops::Deref;
use thiserror::Error;

/// An error that occurs when invalid value is provided for the attribute name.
#[derive(Error, Debug, Eq, PartialEq, Copy, Clone)]
pub enum AttributeNameError {
    /// The provided value is empty.
    #[error("Attribute name can't be empty.")]
    Empty,

    /// The provided value contains a character that is forbidden by the HTML grammar in attribute
    /// names (e.g. `'='`).
    #[error("`{0}` character is forbidden in the attribute name")]
    ForbiddenCharacter(char),

    /// The provided value contains a character that can't be represented in the document's
    /// [`encoding`].
    ///
    /// [`encoding`]: ../struct.Settings.html#structfield.encoding
    #[error("The attribute name contains a character that can't be represented in the document's character encoding.")]
    UnencodableCharacter,
}

/// An attribute of an [`Element`].
///
/// This is an immutable representation of an attribute. To modify element's attributes use
/// approriate [`Element`]'s methods.
///
/// [`Element`]: struct.Element.html
pub struct Attribute<'i> {
    name: Bytes<'i>,
    value: Bytes<'i>,
    raw: Option<Bytes<'i>>,
    encoding: &'static Encoding,
}

impl<'i> Attribute<'i> {
    #[inline]
    #[must_use]
    const fn new(
        name: Bytes<'i>,
        value: Bytes<'i>,
        raw: Bytes<'i>,
        encoding: &'static Encoding,
    ) -> Self {
        Attribute {
            name,
            value,
            raw: Some(raw),
            encoding,
        }
    }

    #[inline]
    fn name_from_str(
        name: &str,
        encoding: &'static Encoding,
    ) -> Result<Bytes<'static>, AttributeNameError> {
        if name.is_empty() {
            Err(AttributeNameError::Empty)
        } else if let Some(ch) = name.as_bytes().iter().copied().find(|&ch| {
            matches!(
                ch,
                b' ' | b'\n' | b'\r' | b'\t' | b'\x0C' | b'/' | b'>' | b'='
            )
        }) {
            Err(AttributeNameError::ForbiddenCharacter(ch as char))
        } else {
            // NOTE: if character can't be represented in the given
            // encoding then encoding_rs replaces it with a numeric
            // character reference. Character references are not
            // supported in attribute names, so we need to bail.
            Bytes::from_str_without_replacements(name, encoding)
                .map_err(|_| AttributeNameError::UnencodableCharacter)
                .map(Bytes::into_owned)
        }
    }

    #[inline]
    fn try_from(
        name: &str,
        value: &str,
        encoding: &'static Encoding,
    ) -> Result<Self, AttributeNameError> {
        Ok(Attribute {
            name: Attribute::name_from_str(name, encoding)?,
            value: Bytes::from_str(value, encoding).into_owned(),
            raw: None,
            encoding,
        })
    }

    /// Returns the name of the attribute.
    #[inline]
    #[must_use]
    pub fn name(&self) -> String {
        self.name.as_lowercase_string(self.encoding)
    }

    /// Returns the name of the attribute, preserving its case.
    #[inline]
    #[must_use]
    pub fn name_preserve_case(&self) -> String {
        self.name.as_string(self.encoding)
    }

    /// Returns the value of the attribute.
    #[inline]
    #[must_use]
    pub fn value(&self) -> String {
        self.value.as_string(self.encoding)
    }

    #[inline]
    fn set_value(&mut self, value: &str) {
        self.value = Bytes::from_str(value, self.encoding).into_owned();
        self.raw = None;
    }
}

impl Serialize for &Attribute<'_> {
    #[inline]
    fn into_bytes(self, output_handler: &mut dyn FnMut(&[u8])) -> Result<(), RewritingError> {
        if let Some(raw) = self.raw.as_ref() {
            output_handler(raw);
        } else {
            output_handler(&self.name);
            output_handler(b"=\"");
            escape_double_quotes_only(&self.value, output_handler);
            output_handler(b"\"");
        }
        Ok(())
    }
}

impl Debug for Attribute<'_> {
    #[cold]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Attribute")
            .field("name", &self.name())
            .field("value", &self.value())
            .finish()
    }
}

pub(crate) struct Attributes<'i> {
    input: &'i Bytes<'i>,
    attribute_buffer: &'i AttributeBuffer,
    items: LazyCell<Vec<Attribute<'i>>>,
    encoding: &'static Encoding,
}

impl<'i> Attributes<'i> {
    #[inline]
    #[must_use]
    pub(super) fn new(
        input: &'i Bytes<'i>,
        attribute_buffer: &'i AttributeBuffer,
        encoding: &'static Encoding,
    ) -> Self {
        Attributes {
            input,
            attribute_buffer,
            items: LazyCell::default(),
            encoding,
        }
    }

    pub fn set_attribute(
        &mut self,
        name: &str,
        value: &str,
        encoding: &'static Encoding,
    ) -> Result<(), AttributeNameError> {
        let name = name.to_ascii_lowercase();
        let items = self.as_mut_vec();

        match items.iter_mut().find(|attr| attr.name() == name.as_str()) {
            Some(attr) => attr.set_value(value),
            None => {
                items.push(Attribute::try_from(&name, value, encoding)?);
            }
        }

        Ok(())
    }

    pub fn remove_attribute(&mut self, name: &str) -> bool {
        let name = name.to_ascii_lowercase();
        let items = self.as_mut_vec();
        let mut i = 0;

        while i < items.len() {
            if items[i].name() == name.as_str() {
                items.remove(i);
                return true;
            }

            i += 1;
        }

        false
    }

    fn init_items(&self) -> Vec<Attribute<'i>> {
        self.attribute_buffer
            .iter()
            .map(|a| {
                Attribute::new(
                    self.input.slice(a.name),
                    self.input.slice(a.value),
                    self.input.slice(a.raw_range),
                    self.encoding,
                )
            })
            .collect()
    }

    #[inline]
    fn as_mut_vec(&mut self) -> &mut Vec<Attribute<'i>> {
        // NOTE: we can't use borrow_mut_with here as we'll need
        // because `self` is a mutable reference and we'll have
        // two mutable references by passing it to the initializer
        // closure.
        if !self.items.filled() {
            self.items
                .fill(self.init_items())
                .expect("Cell should be empty at this point");
        }

        self.items
            .borrow_mut()
            .expect("Items should be initialized")
    }

    #[cfg(test)]
    pub(crate) const fn raw_attributes(&self) -> (&'i Bytes<'i>, &'i AttributeBuffer) {
        (self.input, self.attribute_buffer)
    }
}

impl<'i> Deref for Attributes<'i> {
    type Target = [Attribute<'i>];

    #[inline]
    fn deref(&self) -> &[Attribute<'i>] {
        self.items.borrow_with(|| self.init_items())
    }
}

impl Serialize for &Attributes<'_> {
    #[inline]
    fn into_bytes(self, output_handler: &mut dyn FnMut(&[u8])) -> Result<(), RewritingError> {
        if !self.is_empty() {
            let last = self.len() - 1;

            for (idx, attr) in self.iter().enumerate() {
                attr.into_bytes(output_handler)?;

                if idx != last {
                    output_handler(b" ");
                }
            }
        }
        Ok(())
    }
}
