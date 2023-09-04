use super::{Mutations, Token};
use crate::base::Bytes;
use crate::html::TextType;
use encoding_rs::Encoding;
use std::any::Any;
use std::borrow::Cow;
use std::fmt::{self, Debug};

/// An HTML text node chunk.
///
/// Since the rewriter operates on a streaming input with minimal internal buffering, HTML
/// text node can be represented by multiple text chunks. The size of a chunk depends on multiple
/// parameters, such as decoding buffer size and input chunk size.
///
/// It is up to a user of the rewriter to buffer content of chunks to get the whole text node
/// content where desired. The last chunk in a text node can be determined by calling
/// [`last_in_text_node`] method of the chunk.
///
/// Note that in the sequence `"<span>red-<b>or</b>-blue</span>"` the `span` element contains three text
/// nodes: `"red-"`, `"or"`, and `"-blue"`. Each of these can produce multiple text chunks and each will
/// produce one text chunk where [`last_in_text_node`] returns `true`. The last chunk in a text
/// node can have empty textual content. To perform an action once on the text contents of an
/// element, see [`Element::on_end_tag`][crate::rewritable_units::Element::on_end_tag].
///
/// # Example
/// ```
/// use lol_html::{HtmlRewriter, Settings, text};
///
/// let mut greeting = String::new();
///
/// {
///     let mut rewriter = HtmlRewriter::new(
///         Settings {
///            element_content_handlers: vec![
///                text!("div", |t| {
///                  greeting += t.as_str();
///
///                  if t.last_in_text_node() {
///                         greeting += "!";
///                     }
///
///                     Ok(())
///                 })
///             ],
///             ..Settings::default()
///         },
///         |_:&[u8]| {}
///     );
///
///     rewriter.write(b"<div>He").unwrap();
///     rewriter.write(b"llo w").unwrap();
///     rewriter.write(b"orld</div>").unwrap();
///     rewriter.end().unwrap();
/// }
///
/// assert_eq!(greeting, "Hello world!");
/// ```
///
/// [`last_in_text_node`]: #method.last_in_text_node
pub struct TextChunk<'i> {
    text: Cow<'i, str>,
    text_type: TextType,
    last_in_text_node: bool,
    encoding: &'static Encoding,
    mutations: Mutations,
    user_data: Box<dyn Any>,
}

impl<'i> TextChunk<'i> {
    pub(super) fn new_token(
        text: &'i str,
        text_type: TextType,
        last_in_text_node: bool,
        encoding: &'static Encoding,
    ) -> Token<'i> {
        Token::TextChunk(TextChunk {
            text: text.into(),
            text_type,
            last_in_text_node,
            encoding,
            mutations: Mutations::new(encoding),
            user_data: Box::new(()),
        })
    }

    /// Returns the textual content of the chunk.
    #[inline]
    pub fn as_str(&self) -> &str {
        &*self.text
    }

    /// Returns the type of the text in the chunk.
    ///
    /// The type of the text depends on the surrounding context of the text. E.g. regular visible
    /// text and text inside a `<script>` element will have different types. Refer to [`TextType`]
    /// for more information about possible text types.
    ///
    /// [`TextType`]: enum.TextType.html
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, text, RewriteStrSettings};
    /// use lol_html::html_content::TextType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div>Hello</div><script>"use strict";</script>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             text!("div", |t| {
    ///                 assert_eq!(t.text_type(), TextType::Data);
    ///
    ///                 Ok(())
    ///             }),
    ///             text!("script", |t| {
    ///                 assert_eq!(t.text_type(), TextType::ScriptData);
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    /// ```
    #[inline]
    pub fn text_type(&self) -> TextType {
        self.text_type
    }

    /// Returns `true` if the chunk is last in a HTML text node.
    ///
    /// Note that last chunk can have empty textual content.
    #[inline]
    pub fn last_in_text_node(&self) -> bool {
        self.last_in_text_node
    }

    /// Inserts `content` before the text chunk.
    ///
    /// Consequent calls to the method append `content` to the previously inserted content.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, text, RewriteStrSettings};
    /// use lol_html::html_content::ContentType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div>world</div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             text!("div", |t| {
    ///                 if !t.last_in_text_node(){
    ///                     t.before("<!-- 42 -->", ContentType::Html);
    ///                     t.before("Hello ", ContentType::Text);
    ///                 }
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<div><!-- 42 -->Hello world</div>"#);
    /// ```
    #[inline]
    pub fn before(&mut self, content: &str, content_type: crate::rewritable_units::ContentType) {
        self.mutations.before(content, content_type);
    }

    /// Inserts `content` after the text chunk.
    ///
    /// Consequent calls to the method prepend `content` to the previously inserted content.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, text, RewriteStrSettings};
    /// use lol_html::html_content::ContentType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div>Foo</div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             text!("div", |t| {
    ///                 if t.last_in_text_node(){
    ///                     t.after("Bar", ContentType::Text);
    ///                     t.after("Qux", ContentType::Text);
    ///                 }
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<div>FooQuxBar</div>"#);
    /// ```
    #[inline]
    pub fn after(&mut self, content: &str, content_type: crate::rewritable_units::ContentType) {
        self.mutations.after(content, content_type);
    }

    /// Replaces the text chunk with the `content`.
    ///
    /// Consequent calls to the method overwrite previous replacement content.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, text, RewriteStrSettings};
    /// use lol_html::html_content::ContentType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div>Foo</div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             text!("div", |t| {
    ///                 if !t.last_in_text_node(){
    ///                     t.replace("Bar", ContentType::Text);
    ///                     t.replace("Qux", ContentType::Text);
    ///                 }
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<div>Qux</div>"#);
    /// ```
    #[inline]
    pub fn replace(&mut self, content: &str, content_type: crate::rewritable_units::ContentType) {
        self.mutations.replace(content, content_type);
    }

    /// Removes the text chunk.
    #[inline]
    pub fn remove(&mut self) {
        self.mutations.remove();
    }

    /// Returns `true` if the text chunk has been replaced or removed.
    #[inline]
    pub fn removed(&self) -> bool {
        self.mutations.removed()
    }

    #[inline]
    fn raw(&self) -> Option<&Bytes> {
        None
    }

    #[inline]
    fn serialize_from_parts(&self, output_handler: &mut dyn FnMut(&[u8])) {
        if !self.text.is_empty() {
            output_handler(&Bytes::from_str(&self.text, self.encoding));
        }
    }
}

impl_serialize!(TextChunk);
impl_user_data!(TextChunk<'_>);

impl Debug for TextChunk<'_> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("TextChunk")
            .field("text", &self.as_str())
            .field("last_in_text_node", &self.last_in_text_node())
            .finish()
    }
}

#[cfg(test)]
mod tests {
    use crate::html_content::*;
    use crate::rewritable_units::test_utils::*;
    use crate::*;
    use encoding_rs::{Encoding, UTF_8};

    fn rewrite_text_chunk(
        html: &[u8],
        encoding: &'static Encoding,
        mut handler: impl FnMut(&mut TextChunk),
    ) -> String {
        let mut handler_called = false;

        let output = rewrite_html(
            html,
            encoding,
            vec![],
            vec![doc_text!(|c| {
                handler_called = true;
                handler(c);
                Ok(())
            })],
        );

        assert!(handler_called);

        output
    }

    #[test]
    fn user_data() {
        rewrite_text_chunk(b"foo", UTF_8, |c| {
            c.set_user_data(42usize);

            assert_eq!(*c.user_data().downcast_ref::<usize>().unwrap(), 42usize);

            *c.user_data_mut().downcast_mut::<usize>().unwrap() = 1337usize;

            assert_eq!(*c.user_data().downcast_ref::<usize>().unwrap(), 1337usize);
        });
    }

    mod serialization {
        use super::*;

        const HTML: &str =
            "Lorem ipsum dolor sit amet, cÔnsectetur adipiscing elit, sed do eiusmod tempor \
             incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud \
             exercitation & ullamco laboris nisi ut aliquip ex ea commodo > consequat.";

        macro_rules! test {
            ($handler:expr, $expected:expr) => {
                for (html, enc) in encoded(HTML) {
                    assert_eq!(rewrite_text_chunk(&html, enc, $handler), $expected);
                }
            };
        }

        macro_rules! skip_eof_chunk {
            ($c:ident) => {
                if $c.last_in_text_node() {
                    assert!($c.as_str().is_empty());
                    return;
                }
            };
        }

        #[test]
        fn parsed() {
            test!(|_| {}, HTML);
        }

        #[test]
        fn with_prepends_and_appends() {
            test!(
                |c| {
                    skip_eof_chunk!(c);
                    c.before("<span>", ContentType::Text);
                    c.before("<div>Hey</div>", ContentType::Html);
                    c.before("<foo>", ContentType::Html);
                    c.after("</foo>", ContentType::Html);
                    c.after("<!-- 42 -->", ContentType::Html);
                    c.after("<foo & bar>", ContentType::Text);
                },
                concat!(
                    "&lt;span&gt;<div>Hey</div><foo>",
                    "Lorem ipsum dolor sit amet, cÔnsectetur adipiscing elit, sed do eiusmod \
                     tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim \
                     veniam, quis nostrud exercitation & ullamco laboris nisi ut aliquip \
                     ex ea commodo > consequat.",
                    "&lt;foo &amp; bar&gt;<!-- 42 --></foo>"
                )
            );
        }

        #[test]
        fn removed() {
            test!(
                |c| {
                    skip_eof_chunk!(c);
                    assert!(!c.removed());

                    c.remove();

                    assert!(c.removed());

                    c.before("<before>", ContentType::Html);
                    c.after("<after>", ContentType::Html);
                },
                "<before><after>"
            );
        }

        #[test]
        fn replaced_with_text() {
            test!(
                |c| {
                    skip_eof_chunk!(c);
                    c.before("<before>", ContentType::Html);
                    c.after("<after>", ContentType::Html);

                    assert!(!c.removed());

                    c.replace("<div></div>", ContentType::Html);
                    c.replace("<!--42-->", ContentType::Html);
                    c.replace("<foo & bar>", ContentType::Text);

                    assert!(c.removed());
                },
                "<before>&lt;foo &amp; bar&gt;<after>"
            );
        }

        #[test]
        fn replaced_with_html() {
            test!(
                |c| {
                    skip_eof_chunk!(c);
                    c.before("<before>", ContentType::Html);
                    c.after("<after>", ContentType::Html);

                    assert!(!c.removed());

                    c.replace("<div></div>", ContentType::Html);
                    c.replace("<!--42-->", ContentType::Html);
                    c.replace("<foo & bar>", ContentType::Html);

                    assert!(c.removed());
                },
                "<before><foo & bar><after>"
            );
        }
    }
}
