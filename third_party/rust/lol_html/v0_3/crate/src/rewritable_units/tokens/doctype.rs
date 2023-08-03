use crate::base::Bytes;
use crate::rewritable_units::{Serialize, Token};
use encoding_rs::Encoding;
use std::any::Any;
use std::fmt::{self, Debug};

/// A [document type declaration] preamble.
///
/// Note that unlike other HTML content, `Doctype` can't be modified and should be used only for
/// the examination purposes.
///
/// # Example
/// ```
/// use lol_html::{rewrite_str, doctype, RewriteStrSettings};
///
/// rewrite_str(
///     r#"<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "DTD/xhtml1-transitional.dtd""#,
///     RewriteStrSettings {
///         document_content_handlers: vec![
///             doctype!(|d| {
///                 assert_eq!(d.name(), Some("html".into()));
///                 assert_eq!(d.public_id(), Some("-//W3C//DTD XHTML 1.0 Transitional//EN".into()));
///                 assert_eq!(d.system_id(), Some("DTD/xhtml1-transitional.dtd".into()));
///
///                 Ok(())
///             })
///         ],
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
/// ```
///
/// [document type declaration]: https://developer.mozilla.org/en-US/docs/Glossary/Doctype
pub struct Doctype<'i> {
    name: Option<Bytes<'i>>,
    public_id: Option<Bytes<'i>>,
    system_id: Option<Bytes<'i>>,
    force_quirks: bool,
    raw: Bytes<'i>,
    encoding: &'static Encoding,
    user_data: Box<dyn Any>,
}

impl<'i> Doctype<'i> {
    pub(super) fn new_token(
        name: Option<Bytes<'i>>,
        public_id: Option<Bytes<'i>>,
        system_id: Option<Bytes<'i>>,
        force_quirks: bool,
        raw: Bytes<'i>,
        encoding: &'static Encoding,
    ) -> Token<'i> {
        Token::Doctype(Doctype {
            name,
            public_id,
            system_id,
            force_quirks,
            raw,
            encoding,
            user_data: Box::new(()),
        })
    }

    /// The name of the doctype.
    #[inline]
    pub fn name(&self) -> Option<String> {
        self.name
            .as_ref()
            .map(|n| n.as_lowercase_string(self.encoding))
    }

    /// The public identifier of the doctype.
    #[inline]
    pub fn public_id(&self) -> Option<String> {
        self.public_id.as_ref().map(|i| i.as_string(self.encoding))
    }

    /// The system identifier of the doctype.
    #[inline]
    pub fn system_id(&self) -> Option<String> {
        self.system_id.as_ref().map(|i| i.as_string(self.encoding))
    }

    #[inline]
    #[cfg(feature = "integration_test")]
    pub fn force_quirks(&self) -> bool {
        self.force_quirks
    }
}

impl_user_data!(Doctype<'_>);

impl Serialize for Doctype<'_> {
    #[inline]
    fn to_bytes(&self, output_handler: &mut dyn FnMut(&[u8])) {
        output_handler(&self.raw);
    }
}

impl Debug for Doctype<'_> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("Doctype")
            .field("name", &self.name())
            .field("public_id", &self.public_id())
            .field("system_id", &self.system_id())
            .field("force_quirks", &self.force_quirks)
            .finish()
    }
}

#[cfg(test)]
mod tests {
    use crate::html_content::*;
    use crate::rewritable_units::test_utils::*;
    use crate::*;
    use encoding_rs::{Encoding, UTF_8};

    fn rewrite_doctype(
        html: &[u8],
        encoding: &'static Encoding,
        mut handler: impl FnMut(&mut Doctype),
    ) -> String {
        let mut handler_called = false;

        let output = rewrite_html(
            html,
            encoding,
            vec![],
            vec![doctype!(|c| {
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
        rewrite_doctype(b"<!doctype>", UTF_8, |d| {
            d.set_user_data(42usize);

            assert_eq!(*d.user_data().downcast_ref::<usize>().unwrap(), 42usize);

            *d.user_data_mut().downcast_mut::<usize>().unwrap() = 1337usize;

            assert_eq!(*d.user_data().downcast_ref::<usize>().unwrap(), 1337usize);
        });
    }

    #[test]
    fn serialization() {
        for (html, enc) in encoded(r#"<!DOCTYPE html SYSTEM "Ĥey">"#) {
            let output = rewrite_doctype(&html, enc, |_| {});

            assert_eq!(output, r#"<!DOCTYPE html SYSTEM "Ĥey">"#);
        }
    }
}
