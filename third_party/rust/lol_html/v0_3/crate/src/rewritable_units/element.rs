use super::{Attribute, AttributeNameError, ContentType, EndTag, Mutations, StartTag};
use crate::base::Bytes;
use crate::rewriter::{EndTagHandler, HandlerResult};
use encoding_rs::Encoding;
use std::any::Any;
use std::fmt::{self, Debug};
use thiserror::Error;

/// An error that occurs when invalid value is provided for the tag name.
#[derive(Error, Debug, PartialEq, Copy, Clone)]
pub enum TagNameError {
    /// The provided value is empty.
    #[error("Tag name can't be empty.")]
    Empty,

    /// The first character of the provided value is not an ASCII alphabetical character.
    #[error("The first character of the tag name should be an ASCII alphabetical character.")]
    InvalidFirstCharacter,

    /// The provided value contains a character that is forbidden by the HTML grammar in tag names
    /// (e.g. `'>'`).
    #[error("`{0}` character is forbidden in the tag name")]
    ForbiddenCharacter(char),

    /// The provided value contains a character that can't be represented in the document's
    /// [`encoding`].
    ///
    /// [`encoding`]: ../struct.Settings.html#structfield.encoding
    #[error("The tag name contains a character that can't be represented in the document's character encoding.")]
    UnencodableCharacter,
}

/// An error that occurs when invalid value is provided for the tag name.
#[derive(Error, Debug, PartialEq, Copy, Clone)]
pub enum EndTagError {
    /// The tag has no end tag.
    #[error("No end tag.")]
    NoEndTag,
}

/// An HTML element rewritable unit.
///
/// Exposes API for examination and modification of a parsed HTML element.
pub struct Element<'r, 't> {
    start_tag: &'r mut StartTag<'t>,
    end_tag_mutations: Option<Mutations>,
    modified_end_tag_name: Option<Bytes<'static>>,
    end_tag_handler: Option<EndTagHandler<'static>>,
    can_have_content: bool,
    should_remove_content: bool,
    encoding: &'static Encoding,
    user_data: Box<dyn Any>,
}

impl<'r, 't> Element<'r, 't> {
    pub(crate) fn new(start_tag: &'r mut StartTag<'t>, can_have_content: bool) -> Self {
        let encoding = start_tag.encoding();

        Element {
            start_tag,
            end_tag_mutations: None,
            modified_end_tag_name: None,
            end_tag_handler: None,
            can_have_content,
            should_remove_content: false,
            encoding,
            user_data: Box::new(()),
        }
    }

    fn tag_name_bytes_from_str(&self, name: &str) -> Result<Bytes<'static>, TagNameError> {
        match name.chars().next() {
            Some(ch) if !ch.is_ascii_alphabetic() => Err(TagNameError::InvalidFirstCharacter),
            Some(_) => {
                if let Some(ch) = name
                    .chars()
                    .find(|&ch| matches!(ch, ' ' | '\n' | '\r' | '\t' | '\x0C' | '/' | '>'))
                {
                    Err(TagNameError::ForbiddenCharacter(ch))
                } else {
                    // NOTE: if character can't be represented in the given
                    // encoding then encoding_rs replaces it with a numeric
                    // character reference. Character references are not
                    // supported in tag names, so we need to bail.
                    match Bytes::from_str_without_replacements(name, self.encoding) {
                        Ok(name) => Ok(name.into_owned()),
                        Err(_) => Err(TagNameError::UnencodableCharacter),
                    }
                }
            }
            None => Err(TagNameError::Empty),
        }
    }

    #[inline]
    fn remove_content(&mut self) {
        self.start_tag.mutations.content_after.clear();
        self.end_tag_mutations_mut().content_before.clear();
        self.should_remove_content = true;
    }

    #[inline]
    fn end_tag_mutations_mut(&mut self) -> &mut Mutations {
        let encoding = self.encoding;

        self.end_tag_mutations
            .get_or_insert_with(|| Mutations::new(encoding))
    }

    /// Returns the tag name of the element.
    #[inline]
    pub fn tag_name(&self) -> String {
        self.start_tag.name()
    }

    /// Sets the tag name of the element.
    #[inline]
    pub fn set_tag_name(&mut self, name: &str) -> Result<(), TagNameError> {
        let name = self.tag_name_bytes_from_str(name)?;

        if self.can_have_content {
            self.modified_end_tag_name = Some(name.clone());
        }

        self.start_tag.set_name(name);

        Ok(())
    }

    /// Returns the [namespace URI] of the element.
    ///
    /// [namespace URI]: https://developer.mozilla.org/en-US/docs/Web/API/Element/namespaceURI
    #[inline]
    pub fn namespace_uri(&self) -> &'static str {
        self.start_tag.namespace_uri()
    }

    /// Returns an immutable collection of element's attributes.
    #[inline]
    pub fn attributes(&self) -> &[Attribute<'t>] {
        self.start_tag.attributes()
    }

    /// Returns the value of an attribute with the `name`.
    ///
    /// Returns `None` if the element doesn't have an attribute with the `name`.
    #[inline]
    pub fn get_attribute(&self, name: &str) -> Option<String> {
        let name = name.to_ascii_lowercase();

        self.attributes().iter().find_map(|attr| {
            if attr.name() == name {
                Some(attr.value())
            } else {
                None
            }
        })
    }

    /// Returns `true` if the element has an attribute with `name`.
    #[inline]
    pub fn has_attribute(&self, name: &str) -> bool {
        let name = name.to_ascii_lowercase();

        self.attributes().iter().any(|attr| attr.name() == name)
    }

    /// Sets `value` of element's attribute with `name`.
    ///
    /// If element doesn't have an attribute with the `name`, method adds new attribute
    /// to the element with `name` and `value`.
    #[inline]
    pub fn set_attribute(&mut self, name: &str, value: &str) -> Result<(), AttributeNameError> {
        self.start_tag.set_attribute(name, value)
    }

    /// Removes an attribute with the `name` if it is present.
    #[inline]
    pub fn remove_attribute(&mut self, name: &str) {
        self.start_tag.remove_attribute(name);
    }

    /// Inserts `content` before the element.
    ///
    /// Consequent calls to the method append `content` to the previously inserted content.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, element, RewriteStrSettings};
    /// use lol_html::html_content::ContentType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div id="foo"></div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             element!("#foo", |el| {
    ///                 el.before("<bar>", ContentType::Html);
    ///                 el.before("<qux>", ContentType::Html);
    ///                 el.before("<quz>", ContentType::Text);
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<bar><qux>&lt;quz&gt;<div id="foo"></div>"#);
    /// ```
    #[inline]
    pub fn before(&mut self, content: &str, content_type: ContentType) {
        self.start_tag.mutations.before(content, content_type);
    }

    /// Inserts `content` after the element.
    ///
    /// Consequent calls to the method prepend `content` to the previously inserted content.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, element, RewriteStrSettings};
    /// use lol_html::html_content::ContentType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div id="foo"></div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             element!("#foo", |el| {
    ///                 el.after("<bar>", ContentType::Html);
    ///                 el.after("<qux>", ContentType::Html);
    ///                 el.after("<quz>", ContentType::Text);
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<div id="foo"></div>&lt;quz&gt;<qux><bar>"#);
    /// ```
    #[inline]
    pub fn after(&mut self, content: &str, content_type: ContentType) {
        if self.can_have_content {
            self.end_tag_mutations_mut().after(content, content_type);
        } else {
            self.start_tag.mutations.after(content, content_type);
        }
    }

    /// Prepends `content` to the element's inner content, i.e. inserts content right after
    /// the element's start tag.
    ///
    /// Consequent calls to the method prepend `content` to the previously inserted content.
    /// A call to the method doesn't make any effect if the element is an [empty element].
    ///
    /// [empty element]: https://developer.mozilla.org/en-US/docs/Glossary/Empty_element
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, element, RewriteStrSettings};
    /// use lol_html::html_content::{ContentType, Element};
    ///
    /// let handler = |el: &mut Element| {
    ///     el.prepend("<bar>", ContentType::Html);
    ///     el.prepend("<qux>", ContentType::Html);
    ///     el.prepend("<quz>", ContentType::Text);
    ///
    ///     Ok(())
    /// };
    ///
    /// let html = rewrite_str(
    ///     r#"<div id="foo"><!-- content --></div><img>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             element!("#foo", handler),
    ///             element!("img", handler),
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<div id="foo">&lt;quz&gt;<qux><bar><!-- content --></div><img>"#);
    /// ```
    #[inline]
    pub fn prepend(&mut self, content: &str, content_type: ContentType) {
        if self.can_have_content {
            self.start_tag.mutations.after(content, content_type);
        }
    }

    /// Appends `content` to the element's inner content, i.e. inserts content right before
    /// the element's end tag.
    ///
    /// Consequent calls to the method append `content` to the previously inserted content.
    /// A call to the method doesn't make any effect if the element is an [empty element].
    ///
    /// [empty element]: https://developer.mozilla.org/en-US/docs/Glossary/Empty_element
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, element, RewriteStrSettings};
    /// use lol_html::html_content::{ContentType, Element};
    ///
    /// let handler = |el: &mut Element| {
    ///     el.append("<bar>", ContentType::Html);
    ///     el.append("<qux>", ContentType::Html);
    ///     el.append("<quz>", ContentType::Text);
    ///
    ///     Ok(())
    /// };
    ///
    /// let html = rewrite_str(
    ///     r#"<div id="foo"><!-- content --></div><img>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             element!("#foo", handler),
    ///             element!("img", handler),
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<div id="foo"><!-- content --><bar><qux>&lt;quz&gt;</div><img>"#);
    /// ```
    #[inline]
    pub fn append(&mut self, content: &str, content_type: ContentType) {
        if self.can_have_content {
            self.end_tag_mutations_mut().before(content, content_type);
        }
    }

    /// Replaces inner content of the element with `content`.
    ///
    /// Consequent calls to the method overwrite previously inserted content.
    /// A call to the method doesn't make any effect if the element is an [empty element].
    ///
    /// [empty element]: https://developer.mozilla.org/en-US/docs/Glossary/Empty_element
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, element, RewriteStrSettings};
    /// use lol_html::html_content::{ContentType, Element};
    ///
    /// let handler = |el: &mut Element| {
    ///     el.append("<!-- only one -->", ContentType::Html);
    ///     el.set_inner_content("<!-- will -->", ContentType::Html);
    ///     el.set_inner_content("<!-- survive -->", ContentType::Html);
    ///
    ///     Ok(())
    /// };
    ///
    /// let html = rewrite_str(
    ///     r#"<div id="foo"><!-- content --></div><img>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             element!("#foo", handler),
    ///             element!("img", handler),
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<div id="foo"><!-- survive --></div><img>"#);
    /// ```
    #[inline]
    pub fn set_inner_content(&mut self, content: &str, content_type: ContentType) {
        if self.can_have_content {
            self.remove_content();
            self.start_tag.mutations.after(content, content_type);
        }
    }

    /// Replaces the element and its inner content with `content`.
    ///
    /// Consequent calls to the method overwrite previously inserted content.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, element, RewriteStrSettings};
    /// use lol_html::html_content::ContentType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div id="foo"></div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             element!("#foo", |el| {
    ///                 el.replace("<span></span>", ContentType::Html);
    ///                 el.replace("Hello", ContentType::Text);
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"Hello"#);
    /// ```
    #[inline]
    pub fn replace(&mut self, content: &str, content_type: ContentType) {
        self.start_tag.mutations.replace(content, content_type);

        if self.can_have_content {
            self.remove_content();
            self.end_tag_mutations_mut().remove();
        }
    }

    /// Removes the element and its inner content.
    #[inline]
    pub fn remove(&mut self) {
        self.start_tag.mutations.remove();

        if self.can_have_content {
            self.remove_content();
            self.end_tag_mutations_mut().remove();
        }
    }

    /// Removes the element, but keeps its content. I.e. remove start and end tags of the element.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, element, RewriteStrSettings};
    ///
    /// let html = rewrite_str(
    ///     r#"<div><span><!-- 42 --></span></div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             element!("div", |el| {
    ///                 el.remove_and_keep_content();
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<span><!-- 42 --></span>"#);
    /// ```
    #[inline]
    pub fn remove_and_keep_content(&mut self) {
        self.start_tag.mutations.remove();

        if self.can_have_content {
            self.end_tag_mutations_mut().remove();
        }
    }

    /// Returns `true` if the element has been removed or replaced with some content.
    #[inline]
    pub fn removed(&self) -> bool {
        self.start_tag.mutations.removed()
    }

    #[inline]
    pub(crate) fn should_remove_content(&self) -> bool {
        self.should_remove_content
    }

    /// Sets a handler to run when the end tag is reached.
    ///
    /// Subsequent calls to the method on the same element replace the previous handler.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::html_content::ContentType;
    /// use lol_html::{element, rewrite_str, text, RewriteStrSettings};
    /// let buffer = std::rc::Rc::new(std::cell::RefCell::new(String::new()));
    /// let html = rewrite_str(
    ///     "<span>Short</span><span><b>13</b> characters</span>",
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             element!("span", |el| {
    ///                 // Truncate string for each new span.
    ///                 buffer.borrow_mut().clear();
    ///                 let buffer = buffer.clone();
    ///                 el.on_end_tag(move |end| {
    ///                     let s = buffer.borrow();
    ///                     if s.len() == 13 {
    ///                         // add text before the end tag
    ///                         end.before("!", ContentType::Text);
    ///                     } else {
    ///                         // replace the end tag with an uppercase version
    ///                         end.remove();
    ///                         let name = end.name().to_uppercase();
    ///                         end.after(&format!("</{}>", name), ContentType::Html);
    ///                     }
    ///                     Ok(())
    ///                 })?;
    ///                 Ok(())
    ///             }),
    ///             text!("span", |t| {
    ///                 // Save the text contents for the end tag handler.
    ///                 buffer.borrow_mut().push_str(t.as_str());
    ///                 Ok(())
    ///             }),
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     },
    /// )
    /// .unwrap();
    ///
    /// assert_eq!(html, "<span>Short</SPAN><span><b>13</b> characters!</span>");
    /// ```
    pub fn on_end_tag(
        &mut self,
        handler: impl FnOnce(&mut EndTag) -> HandlerResult + 'static,
    ) -> Result<(), EndTagError> {
        if self.can_have_content {
            self.end_tag_handler = Some(Box::new(handler));
            Ok(())
        } else {
            Err(EndTagError::NoEndTag)
        }
    }

    pub(crate) fn into_end_tag_handler(self) -> Option<EndTagHandler<'static>> {
        let end_tag_mutations = self.end_tag_mutations;
        let modified_end_tag_name = self.modified_end_tag_name;
        let end_tag_handler = self.end_tag_handler;

        if end_tag_mutations.is_some()
            || modified_end_tag_name.is_some()
            || end_tag_handler.is_some()
        {
            Some(Box::new(move |end_tag: &mut EndTag| {
                if let Some(name) = modified_end_tag_name {
                    end_tag.set_name(name);
                }

                if let Some(mutations) = end_tag_mutations {
                    end_tag.mutations = mutations;
                }

                if let Some(handler) = end_tag_handler {
                    handler(end_tag)
                } else {
                    Ok(())
                }
            }))
        } else {
            None
        }
    }
}

impl_user_data!(Element<'_, '_>);

impl Debug for Element<'_, '_> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("Element")
            .field("tag_name", &self.tag_name())
            .field("attributes", &self.attributes())
            .finish()
    }
}

#[cfg(test)]
mod tests {
    use crate::errors::*;
    use crate::html_content::*;
    use crate::rewritable_units::test_utils::*;
    use crate::*;
    use encoding_rs::{Encoding, EUC_JP, UTF_8};

    fn rewrite_element(
        html: &[u8],
        encoding: &'static Encoding,
        selector: &str,
        mut handler: impl FnMut(&mut Element),
    ) -> String {
        let mut handler_called = false;

        let output = rewrite_html(
            html,
            encoding,
            vec![
                element!(selector, |el| {
                    handler_called = true;
                    handler(el);
                    Ok(())
                }),
                element!("inner-remove-me", |el| {
                    el.before("[before: should be removed]", ContentType::Text);
                    el.after("[after: should be removed]", ContentType::Text);
                    el.append("[append: should be removed]", ContentType::Text);
                    el.before("[before: should be removed]", ContentType::Text);
                    Ok(())
                }),
            ],
            vec![],
        );

        assert!(handler_called, "Handler not called.");

        output
    }

    #[test]
    fn empty_tag_name() {
        rewrite_element(b"<div>", UTF_8, "div", |el| {
            let err = el.set_tag_name("").unwrap_err();

            assert_eq!(err, TagNameError::Empty);
        });
    }

    #[test]
    fn forbidden_characters_in_tag_name() {
        rewrite_element(b"<div>", UTF_8, "div", |el| {
            for &ch in &[' ', '\n', '\r', '\t', '\x0C', '/', '>'] {
                let err = el.set_tag_name(&format!("foo{}bar", ch)).unwrap_err();

                assert_eq!(err, TagNameError::ForbiddenCharacter(ch));
            }
        });
    }

    #[test]
    fn encoding_unmappable_chars_in_tag_name() {
        rewrite_element(b"<div>", EUC_JP, "div", |el| {
            let err = el.set_tag_name("foo\u{00F8}bar").unwrap_err();

            assert_eq!(err, TagNameError::UnencodableCharacter);
        });
    }

    #[test]
    fn invalid_first_char_of_tag_name() {
        rewrite_element(b"<div>", UTF_8, "div", |el| {
            let err = el.set_tag_name("1foo").unwrap_err();

            assert_eq!(err, TagNameError::InvalidFirstCharacter);
        });
    }

    #[test]
    fn namespace_uri() {
        rewrite_element(b"<script></script>", UTF_8, "script", |el| {
            assert_eq!(el.namespace_uri(), "http://www.w3.org/1999/xhtml");
        });

        rewrite_element(b"<svg><script></script></svg>", UTF_8, "script", |el| {
            assert_eq!(el.namespace_uri(), "http://www.w3.org/2000/svg");
        });

        rewrite_element(
            b"<svg><foreignObject><script></script></foreignObject></svg>",
            UTF_8,
            "script",
            |el| {
                assert_eq!(el.namespace_uri(), "http://www.w3.org/1999/xhtml");
            },
        );

        rewrite_element(b"<math><script></script></math>", UTF_8, "script", |el| {
            assert_eq!(el.namespace_uri(), "http://www.w3.org/1998/Math/MathML");
        });
    }

    #[test]
    fn empty_attr_name() {
        rewrite_element(b"<div>", UTF_8, "div", |el| {
            let err = el.set_attribute("", "").unwrap_err();

            assert_eq!(err, AttributeNameError::Empty);
        });
    }

    #[test]
    fn forbidden_characters_in_attr_name() {
        rewrite_element(b"<div>", UTF_8, "div", |el| {
            for &ch in &[' ', '\n', '\r', '\t', '\x0C', '/', '>', '='] {
                let err = el.set_attribute(&format!("foo{}bar", ch), "").unwrap_err();

                assert_eq!(err, AttributeNameError::ForbiddenCharacter(ch));
            }
        });
    }

    #[test]
    fn encoding_unmappable_character_in_attr_name() {
        rewrite_element(b"<div>", EUC_JP, "div", |el| {
            let err = el.set_attribute("foo\u{00F8}bar", "").unwrap_err();

            assert_eq!(err, AttributeNameError::UnencodableCharacter);
        });
    }

    #[test]
    fn tag_name_getter_and_setter() {
        for (html, enc) in encoded("<FooǼ><div><span></span></div></FooǼ>") {
            let output = rewrite_element(&html, enc, "fooǼ", |el| {
                assert_eq!(el.tag_name(), "fooǼ", "Encoding: {}", enc.name());

                el.set_tag_name("BaZǽ").unwrap();

                assert_eq!(el.tag_name(), "bazǽ", "Encoding: {}", enc.name());
            });

            assert_eq!(output, "<BaZǽ><div><span></span></div></BaZǽ>");
        }
    }

    #[test]
    fn attribute_list() {
        for (html, enc) in encoded("<Foo Fooα1=Barβ1 Fooγ2=Barδ2>") {
            rewrite_element(&html, enc, "foo", |el| {
                assert_eq!(el.attributes().len(), 2, "Encoding: {}", enc.name());
                assert_eq!(
                    el.attributes()[0].name(),
                    "fooα1",
                    "Encoding: {}",
                    enc.name()
                );
                assert_eq!(
                    el.attributes()[1].name(),
                    "fooγ2",
                    "Encoding: {}",
                    enc.name()
                );

                assert_eq!(
                    el.attributes()[0].value(),
                    "Barβ1",
                    "Encoding: {}",
                    enc.name()
                );

                assert_eq!(
                    el.attributes()[1].value(),
                    "Barδ2",
                    "Encoding: {}",
                    enc.name()
                );
            });
        }
    }

    #[test]
    fn get_attrs() {
        for (html, enc) in encoded("<Foo Fooα1=Barβ1 Fooγ2=Barδ2>") {
            rewrite_element(&html, enc, "foo", |el| {
                assert_eq!(
                    el.get_attribute("fOoα1").unwrap(),
                    "Barβ1",
                    "Encoding: {}",
                    enc.name()
                );

                assert_eq!(
                    el.get_attribute("Fooα1").unwrap(),
                    "Barβ1",
                    "Encoding: {}",
                    enc.name()
                );

                assert_eq!(
                    el.get_attribute("FOOγ2").unwrap(),
                    "Barδ2",
                    "Encoding: {}",
                    enc.name()
                );

                assert_eq!(
                    el.get_attribute("fooγ2").unwrap(),
                    "Barδ2",
                    "Encoding: {}",
                    enc.name()
                );

                assert_eq!(el.get_attribute("foo3"), None, "Encoding: {}", enc.name());
            });
        }
    }

    #[test]
    fn has_attr() {
        for (html, enc) in encoded("<Foo FooѦ1=Bar1 FooѤ2=Bar2>") {
            rewrite_element(&html, enc, "foo", |el| {
                assert!(el.has_attribute("FOoѦ1"), "Encoding: {}", enc.name());
                assert!(el.has_attribute("fooѦ1"), "Encoding: {}", enc.name());
                assert!(el.has_attribute("FOOѤ2"), "Encoding: {}", enc.name());
                assert!(!el.has_attribute("foo3"), "Encoding: {}", enc.name());
            });
        }
    }

    #[test]
    fn set_attr() {
        for (html, enc) in encoded("<div҈>") {
            rewrite_element(&html, enc, "div҈", |el| {
                el.set_attribute("FooѴ", "҈Bar1҈").unwrap();

                assert_eq!(
                    el.get_attribute("fooѴ").unwrap(),
                    "҈Bar1҈",
                    "Encoding: {}",
                    enc.name()
                );

                el.set_attribute("fOOѴ", "Bar2").unwrap();

                assert_eq!(
                    el.get_attribute("fooѴ").unwrap(),
                    "Bar2",
                    "Encoding: {}",
                    enc.name()
                );
            });
        }
    }

    #[test]
    fn remove_attr() {
        for (html, enc) in encoded("<Foo Foo1இ=Bar1 Foo2இ=Bar2>") {
            rewrite_element(&html, enc, "foo", |el| {
                el.remove_attribute("Unknown");

                assert_eq!(el.attributes().len(), 2, "Encoding: {}", enc.name());

                el.remove_attribute("Foo1இ");

                assert_eq!(el.attributes().len(), 1, "Encoding: {}", enc.name());
                assert_eq!(el.get_attribute("foo1இ"), None, "Encoding: {}", enc.name());

                el.remove_attribute("FoO2இ");

                assert!(el.attributes().is_empty(), "Encoding: {}", enc.name());
                assert_eq!(el.get_attribute("foo2இ"), None, "Encoding: {}", enc.name());
            });
        }
    }

    #[test]
    fn insert_content_before() {
        for (html, enc) in encoded("<div><span>ĥi</span></div>") {
            let output = rewrite_element(&html, enc, "span", |el| {
                el.before("<imgĤ>", ContentType::Html);
                el.before("<imgĤ>", ContentType::Text);
            });

            assert_eq!(output, "<div><imgĤ>&lt;imgĤ&gt;<span>ĥi</span></div>");
        }
    }

    #[test]
    fn prepend_content() {
        for (html, enc) in encoded("<div><span>ĥi</span></div>") {
            let output = rewrite_element(&html, enc, "span", |el| {
                el.prepend("<imgĤ>", ContentType::Html);
                el.prepend("<imgĤ>", ContentType::Text);
            });

            assert_eq!(output, "<div><span>&lt;imgĤ&gt;<imgĤ>ĥi</span></div>");
        }
    }

    #[test]
    fn append_content() {
        for (html, enc) in encoded("<div><span>ĥi</span></div>") {
            let output = rewrite_element(&html, enc, "span", |el| {
                el.append("<imgĤ>", ContentType::Html);
                el.append("<imgĤ>", ContentType::Text);
            });

            assert_eq!(output, "<div><span>ĥi<imgĤ>&lt;imgĤ&gt;</span></div>");
        }
    }

    #[test]
    fn insert_content_after() {
        for (html, enc) in encoded("<div><span>ĥi</span></div>") {
            let output = rewrite_element(&html, enc, "span", |el| {
                el.after("<imgĤ>", ContentType::Html);
                el.after("<imgĤ>", ContentType::Text);
            });

            assert_eq!(output, "<div><span>ĥi</span>&lt;imgĤ&gt;<imgĤ></div>");
        }
    }

    #[test]
    fn set_content_after() {
        for (html, enc) in
            encoded("<div><span>Hi<inner-remove-me>RemoveŴ</inner-remove-me></span></div>")
        {
            let output = rewrite_element(&html, enc, "span", |el| {
                el.prepend("<prepended>", ContentType::Html);
                el.append("<appended>", ContentType::Html);
                el.set_inner_content("<imgŵ>", ContentType::Html);
                el.set_inner_content("<imgŵ>", ContentType::Text);
            });

            assert_eq!(output, "<div><span>&lt;imgŵ&gt;</span></div>");

            let output = rewrite_element(&html, enc, "span", |el| {
                el.prepend("<prepended>", ContentType::Html);
                el.append("<appended>", ContentType::Html);
                el.set_inner_content("<imgŵ>", ContentType::Text);
                el.set_inner_content("<imgŵ>", ContentType::Html);
            });

            assert_eq!(output, "<div><span><imgŵ></span></div>");
        }
    }

    #[test]
    fn replace() {
        for (html, enc) in
            encoded("<div><span>Hi<inner-remove-me>Remove㘗</inner-remove-me></span></div>")
        {
            let output = rewrite_element(&html, enc, "span", |el| {
                el.prepend("<prepended>", ContentType::Html);
                el.append("<appended>", ContentType::Html);
                el.replace("<img㘘>", ContentType::Html);
                el.replace("<img㘘>", ContentType::Text);

                assert!(el.removed());
            });

            assert_eq!(output, "<div>&lt;img㘘&gt;</div>");

            let output = rewrite_element(&html, enc, "span", |el| {
                el.prepend("<prepended>", ContentType::Html);
                el.append("<appended>", ContentType::Html);
                el.replace("<img㘘>", ContentType::Text);
                el.replace("<img㘘>", ContentType::Html);

                assert!(el.removed());
            });

            assert_eq!(output, "<div><img㘘></div>");
        }
    }

    #[test]
    fn remove() {
        for (html, enc) in
            encoded("<div><span㗵>Hi<inner-remove-me>Remove</inner-remove-me></span㗵></div>")
        {
            let output = rewrite_element(&html, enc, "span㗵", |el| {
                el.prepend("<prepended>", ContentType::Html);
                el.append("<appended>", ContentType::Html);
                el.remove();

                assert!(el.removed());
            });

            assert_eq!(output, "<div></div>");
        }
    }

    #[test]
    fn remove_with_unfinished_end_tag() {
        for (html, enc) in encoded("<div><span㚴>Heello</span㚴  ") {
            let output = rewrite_element(&html, enc, "span㚴", |el| {
                el.remove();

                assert!(el.removed());
            });

            assert_eq!(output, "<div>");
        }
    }

    #[test]
    fn remove_and_keep_content() {
        for (html, enc) in encoded("<div><spanЫ>Hi</spanЫ></div>") {
            let output = rewrite_element(&html, enc, "spanЫ", |el| {
                el.prepend("<prepended>", ContentType::Html);
                el.append("<appended>", ContentType::Html);
                el.remove_and_keep_content();

                assert!(el.removed());
            });

            assert_eq!(output, "<div><prepended>Hi<appended></div>");
        }
    }

    #[test]
    fn multiple_consequent_removes() {
        let output = rewrite_html(
            b"<div><span>42</span></div><h1>Hello</h1><h2>Hello2</h2>",
            UTF_8,
            vec![
                element!("div", |el| {
                    el.replace("hey & ya", ContentType::Html);
                    Ok(())
                }),
                element!("h1", |el| {
                    el.remove();
                    Ok(())
                }),
                element!("h2", |el| {
                    el.remove_and_keep_content();
                    Ok(())
                }),
            ],
            vec![],
        );

        assert_eq!(output, "hey & yaHello2");
    }

    #[test]
    fn void_element() {
        let output = rewrite_element(b"<img><span>Hi</span></img>", UTF_8, "img", |el| {
            el.after("<!--after-->", ContentType::Html);
            el.append("<!--append-->", ContentType::Html);
            el.prepend("<!--prepend-->", ContentType::Html);
            el.set_inner_content("<!--set_inner_content-->", ContentType::Html);
            el.set_tag_name("img-foo").unwrap();
        });

        assert_eq!(output, "<img-foo><!--after--><span>Hi</span></img>");
    }

    #[test]
    fn self_closing_element() {
        let output = rewrite_element(b"<svg><foo/>Hi</foo></svg>", UTF_8, "foo", |el| {
            el.after("<!--after-->", ContentType::Html);
            el.set_tag_name("bar").unwrap();
        });

        assert_eq!(output, "<svg><bar/><!--after-->Hi</foo></svg>");
    }

    #[test]
    fn user_data() {
        rewrite_element(b"<div><span>Hi</span></div>", UTF_8, "span", |el| {
            el.set_user_data(42usize);

            assert_eq!(*el.user_data().downcast_ref::<usize>().unwrap(), 42usize);

            *el.user_data_mut().downcast_mut::<usize>().unwrap() = 1337usize;

            assert_eq!(*el.user_data().downcast_ref::<usize>().unwrap(), 1337usize);
        });
    }

    mod serialization {
        use super::*;

        const HTML: &str = r#"<a a1='foo " baré " baz' / a2="foo ' bar ' baz" a3=foo/bar a4></a>"#;
        const SELECTOR: &str = "a";

        macro_rules! test {
            ($handler:expr, $expected:expr) => {
                for (html, enc) in encoded(HTML) {
                    assert_eq!(rewrite_element(&html, enc, SELECTOR, $handler), $expected);
                }
            };
        }

        #[test]
        fn parsed() {
            test!(
                |_| {},
                r#"<a a1='foo " baré " baz' / a2="foo ' bar ' baz" a3=foo/bar a4></a>"#
            );
        }

        #[test]
        fn modified_name() {
            test!(
                |el| {
                    el.set_tag_name("div").unwrap();
                },
                r#"<div a1='foo " baré " baz' a2="foo ' bar ' baz" a3=foo/bar a4></div>"#
            );
        }

        #[test]
        fn modified_single_quoted_attr() {
            test!(
                |el| {
                    el.set_attribute("a2", "foo ' bar ' baz42").unwrap();
                },
                r#"<a a1='foo " baré " baz' a2="foo ' bar ' baz42" a3=foo/bar a4></a>"#
            );
        }

        #[test]
        fn modified_double_quoted_attr() {
            test!(
                |el| {
                    el.set_attribute("a2", "foo ' bar ' baz42").unwrap();
                },
                r#"<a a1='foo " baré " baz' a2="foo ' bar ' baz42" a3=foo/bar a4></a>"#
            );
        }

        #[test]
        fn modified_unquoted_attr() {
            test!(
                |el| {
                    el.set_attribute("a3", "foo/bar42").unwrap();
                },
                r#"<a a1='foo " baré " baz' a2="foo ' bar ' baz" a3="foo/bar42" a4></a>"#
            );
        }

        #[test]
        fn set_value_for_attr_without_value() {
            test!(
                |el| {
                    el.set_attribute("a4", "42").unwrap();
                },
                r#"<a a1='foo " baré " baz' a2="foo ' bar ' baz" a3=foo/bar a4="42"></a>"#
            );
        }

        #[test]
        fn add_attr() {
            test!(
                |el| {
                    el.set_attribute("a5", r#"42'"42"#).unwrap();
                },
                r#"<a a1='foo " baré " baz' a2="foo ' bar ' baz" a3=foo/bar a4 a5="42'&quot;42"></a>"#
            );
        }

        #[test]
        fn self_closing_flag() {
            // NOTE: we should add space between valueless attr and self-closing slash
            // during serialization. Otherwise, it will be interpreted as a part of the
            // attribute name.
            let mut output = rewrite_element(b"<img a1=42 a2 />", UTF_8, "img", |el| {
                el.set_attribute("a1", "foo").unwrap();
            });

            assert_eq!(output, r#"<img a1="foo" a2 />"#);

            // NOTE: but we shouldn't add space if there are no attributes.
            output = rewrite_element(b"<img a1 />", UTF_8, "img", |el| {
                el.remove_attribute("a1");
            });

            assert_eq!(output, r#"<img/>"#);
        }

        #[test]
        fn remove_non_existent_attr() {
            test!(
                |el| {
                    el.remove_attribute("a5");
                },
                r#"<a a1='foo " baré " baz' / a2="foo ' bar ' baz" a3=foo/bar a4></a>"#
            );
        }

        #[test]
        fn without_attrs() {
            test!(
                |el| {
                    for name in &["a1", "a2", "a3", "a4"] {
                        el.remove_attribute(name);
                    }
                },
                "<a></a>"
            );
        }

        #[test]
        fn with_before_and_prepend() {
            test!(
                |el| {
                    el.before("<span>", ContentType::Text);
                    el.before("<div>Hey</div>", ContentType::Html);
                    el.before("<foo>", ContentType::Html);
                    el.prepend("</foo>", ContentType::Html);
                    el.prepend("<!-- 42 -->", ContentType::Html);
                    el.prepend("<foo & bar>", ContentType::Text);
                },
                concat!(
                    "&lt;span&gt;<div>Hey</div><foo>",
                    r#"<a a1='foo " baré " baz' / a2="foo ' bar ' baz" a3=foo/bar a4>"#,
                    "&lt;foo &amp; bar&gt;<!-- 42 --></foo>",
                    "</a>"
                )
            );
        }

        #[test]
        fn with_after_and_append() {
            test!(
                |el| {
                    el.append("<span>", ContentType::Text);
                    el.append("<div>Hey</div>", ContentType::Html);
                    el.append("<foo>", ContentType::Html);
                    el.after("</foo>", ContentType::Html);
                    el.after("<!-- 42 -->", ContentType::Html);
                    el.after("<foo & bar>", ContentType::Text);
                },
                concat!(
                    r#"<a a1='foo " baré " baz' / a2="foo ' bar ' baz" a3=foo/bar a4>"#,
                    "&lt;span&gt;<div>Hey</div><foo>",
                    "</a>",
                    "&lt;foo &amp; bar&gt;<!-- 42 --></foo>",
                )
            );
        }

        #[test]
        fn removed() {
            test!(
                |el| {
                    assert!(!el.removed());

                    el.remove();

                    assert!(el.removed());

                    el.before("<before>", ContentType::Html);
                    el.after("<after>", ContentType::Html);
                },
                "<before><after>"
            );
        }

        #[test]
        fn replaced_with_text() {
            test!(
                |el| {
                    el.before("<before>", ContentType::Html);
                    el.after("<after>", ContentType::Html);

                    assert!(!el.removed());

                    el.replace("<div></div>", ContentType::Html);
                    el.replace("<!--42-->", ContentType::Html);
                    el.replace("<foo & bar>", ContentType::Text);

                    assert!(el.removed());
                },
                "<before>&lt;foo &amp; bar&gt;<after>"
            );
        }

        #[test]
        fn replaced_with_html() {
            test!(
                |el| {
                    el.before("<before>", ContentType::Html);
                    el.after("<after>", ContentType::Html);

                    assert!(!el.removed());

                    el.replace("<div></div>", ContentType::Html);
                    el.replace("<!--42-->", ContentType::Html);
                    el.replace("<foo & bar>", ContentType::Html);

                    assert!(el.removed());
                },
                "<before><foo & bar><after>"
            );
        }
    }
}
