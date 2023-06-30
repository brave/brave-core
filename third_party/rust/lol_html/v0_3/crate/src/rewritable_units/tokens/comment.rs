use super::{Mutations, Token};
use crate::base::Bytes;
use encoding_rs::Encoding;
use std::any::Any;
use std::fmt::{self, Debug};
use thiserror::Error;

/// An error that occurs when invalid value is provided for the HTML comment text.
#[derive(Error, Debug, PartialEq, Copy, Clone)]
pub enum CommentTextError {
    /// The provided value contains the `-->` character sequence that preemptively closes the comment.
    #[error("Comment text shouldn't contain comment closing sequence (`-->`).")]
    CommentClosingSequence,

    /// The provided value contains a character that can't be represented in the document's [`encoding`].
    ///
    /// [`encoding`]: ../struct.Settings.html#structfield.encoding
    #[error("Comment text contains a character that can't be represented in the document's character encoding.")]
    UnencodableCharacter,
}

/// An HTML comment rewritable unit.
///
/// Exposes API for examination and modification of a parsed HTML comment.
pub struct Comment<'i> {
    text: Bytes<'i>,
    raw: Option<Bytes<'i>>,
    encoding: &'static Encoding,
    mutations: Mutations,
    user_data: Box<dyn Any>,
}

impl<'i> Comment<'i> {
    pub(super) fn new_token(
        text: Bytes<'i>,
        raw: Bytes<'i>,
        encoding: &'static Encoding,
    ) -> Token<'i> {
        Token::Comment(Comment {
            text,
            raw: Some(raw),
            encoding,
            mutations: Mutations::new(encoding),
            user_data: Box::new(()),
        })
    }

    /// Returns the text of the comment.
    #[inline]
    pub fn text(&self) -> String {
        self.text.as_string(self.encoding)
    }

    /// Sets the text of the comment.
    #[inline]
    pub fn set_text(&mut self, text: &str) -> Result<(), CommentTextError> {
        if text.contains("-->") {
            Err(CommentTextError::CommentClosingSequence)
        } else {
            // NOTE: if character can't be represented in the given
            // encoding then encoding_rs replaces it with a numeric
            // character reference. Character references are not
            // supported in comments, so we need to bail.
            match Bytes::from_str_without_replacements(text, self.encoding) {
                Ok(text) => {
                    self.text = text.into_owned();
                    self.raw = None;

                    Ok(())
                }
                Err(_) => Err(CommentTextError::UnencodableCharacter),
            }
        }
    }

    /// Inserts `content` before the comment.
    ///
    /// Consequent calls to the method append `content` to the previously inserted content.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, comments, RewriteStrSettings};
    /// use lol_html::html_content::ContentType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div><!-- foo --></div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             comments!("div", |c| {
    ///                 c.before("<!-- 42 -->", ContentType::Html);
    ///                 c.before("bar", ContentType::Text);
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<div><!-- 42 -->bar<!-- foo --></div>"#);
    /// ```
    #[inline]
    pub fn before(&mut self, content: &str, content_type: crate::rewritable_units::ContentType) {
        self.mutations.before(content, content_type);
    }

    /// Inserts `content` after the comment.
    ///
    /// Consequent calls to the method prepend `content` to the previously inserted content.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, comments, RewriteStrSettings};
    /// use lol_html::html_content::ContentType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div><!-- foo --></div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             comments!("div", |c| {
    ///                 c.after("Bar", ContentType::Text);
    ///                 c.after("Qux", ContentType::Text);
    ///
    ///                 Ok(())
    ///             })
    ///         ],
    ///         ..RewriteStrSettings::default()
    ///     }
    /// ).unwrap();
    ///
    /// assert_eq!(html, r#"<div><!-- foo -->QuxBar</div>"#);
    /// ```
    #[inline]
    pub fn after(&mut self, content: &str, content_type: crate::rewritable_units::ContentType) {
        self.mutations.after(content, content_type);
    }

    /// Replaces the comment with the `content`.
    ///
    /// Consequent calls to the method overwrite previous replacement content.
    ///
    /// # Example
    ///
    /// ```
    /// use lol_html::{rewrite_str, comments, RewriteStrSettings};
    /// use lol_html::html_content::ContentType;
    ///
    /// let html = rewrite_str(
    ///     r#"<div><!-- foo --></div>"#,
    ///     RewriteStrSettings {
    ///         element_content_handlers: vec![
    ///             comments!("div", |c| {
    ///                 c.replace("Bar", ContentType::Text);
    ///                 c.replace("Qux", ContentType::Text);
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

    /// Removes the comment.
    #[inline]
    pub fn remove(&mut self) {
        self.mutations.remove();
    }

    /// Returns `true` if the comment has been replaced or removed.
    #[inline]
    pub fn removed(&self) -> bool {
        self.mutations.removed()
    }

    #[inline]
    fn raw(&self) -> Option<&Bytes> {
        self.raw.as_ref()
    }

    #[inline]
    fn serialize_from_parts(&self, output_handler: &mut dyn FnMut(&[u8])) {
        output_handler(b"<!--");
        output_handler(&self.text);
        output_handler(b"-->");
    }
}

impl_serialize!(Comment);
impl_user_data!(Comment<'_>);

impl Debug for Comment<'_> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("Comment")
            .field("text", &self.text())
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

    fn rewrite_comment(
        html: &[u8],
        encoding: &'static Encoding,
        mut handler: impl FnMut(&mut Comment),
    ) -> String {
        let mut handler_called = false;

        let output = rewrite_html(
            html,
            encoding,
            vec![],
            vec![doc_comments!(|c| {
                handler_called = true;
                handler(c);
                Ok(())
            })],
        );

        assert!(handler_called);

        output
    }

    #[test]
    fn comment_closing_sequence_in_text() {
        rewrite_comment(b"<!-- foo -->", UTF_8, |c| {
            let err = c.set_text("foo -- bar --> baz").unwrap_err();

            assert_eq!(err, CommentTextError::CommentClosingSequence);
        });
    }

    #[test]
    fn encoding_unmappable_chars_in_text() {
        rewrite_comment(b"<!-- foo -->", EUC_JP, |c| {
            let err = c.set_text("foo\u{00F8}bar").unwrap_err();

            assert_eq!(err, CommentTextError::UnencodableCharacter);
        });
    }

    #[test]
    fn user_data() {
        rewrite_comment(b"<!-- foo -->", UTF_8, |c| {
            c.set_user_data(42usize);

            assert_eq!(*c.user_data().downcast_ref::<usize>().unwrap(), 42usize);

            *c.user_data_mut().downcast_mut::<usize>().unwrap() = 1337usize;

            assert_eq!(*c.user_data().downcast_ref::<usize>().unwrap(), 1337usize);
        });
    }

    mod serialization {
        use super::*;

        const HTML: &str = "<!-- fooé -- bar -->";

        macro_rules! test {
            ($handler:expr, $expected:expr) => {
                for (html, enc) in encoded(HTML) {
                    assert_eq!(rewrite_comment(&html, enc, $handler), $expected);
                }
            };
        }

        #[test]
        fn parsed() {
            test!(|_| {}, "<!-- fooé -- bar -->");
        }

        #[test]
        fn modified_text() {
            test!(
                |c| {
                    c.set_text("42é <!-").unwrap();
                },
                "<!--42é <!--->"
            );
        }

        #[test]
        fn with_prepends_and_appends() {
            test!(
                |c| {
                    c.before("<span>", ContentType::Text);
                    c.before("<div>Hey</div>", ContentType::Html);
                    c.before("<foo>", ContentType::Html);
                    c.after("</foo>", ContentType::Html);
                    c.after("<!-- 42é -->", ContentType::Html);
                    c.after("<foo & bar>", ContentType::Text);
                },
                concat!(
                    "&lt;span&gt;<div>Hey</div><foo><!-- fooé -- bar -->",
                    "&lt;foo &amp; bar&gt;<!-- 42é --></foo>",
                )
            );
        }

        #[test]
        fn removed() {
            test!(
                |c| {
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
