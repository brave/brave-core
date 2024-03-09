use std::any::Any;

pub use self::document_end::*;
pub use self::element::*;
pub use self::mutations::{ContentType, Mutations};
pub use self::tokens::*;

/// Data that can be attached to a rewritable unit by a user and shared between content handler
/// invocations.
///
/// Same rewritable units can be passed to different content handlers if all of them capture the
/// unit. `UserData` trait provides capability to attach arbitrary data to a rewritable unit, so
/// handlers can make decision on how to process the unit based on the information provided by
/// previous handlers.
///
/// # Example
/// ```
/// use lol_html::{rewrite_str, element, RewriteStrSettings};
/// use lol_html::html_content::UserData;
///
/// rewrite_str(
///     r#"<div id="foo"></div>"#,
///     RewriteStrSettings {
///         element_content_handlers: vec![
///             element!("*", |el| {
///                 el.set_user_data("Captured by `*`");
///
///                 Ok(())
///             }),
///             element!("#foo", |el| {
///                 let user_data = el.user_data_mut().downcast_mut::<&'static str>().unwrap();
///
///                 assert_eq!(*user_data, "Captured by `*`");
///
///                 *user_data = "Captured by `#foo`";
///
///                 Ok(())
///             }),
///             element!("div", |el| {
///                 let user_data = el.user_data().downcast_ref::<&'static str>().unwrap();
///
///                 assert_eq!(*user_data, "Captured by `#foo`");
///
///                 Ok(())
///             })
///         ],
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
/// ```
pub trait UserData {
    /// Returns a reference to the attached user data.
    fn user_data(&self) -> &dyn Any;
    /// Returns a mutable reference to the attached user data.
    fn user_data_mut(&mut self) -> &mut dyn Any;
    /// Attaches user data to a rewritable unit.
    fn set_user_data(&mut self, data: impl Any);
}

macro_rules! impl_user_data {
    ($Unit:ident<$($lt:lifetime),+>) => {
        impl crate::rewritable_units::UserData for $Unit<$($lt),+> {
            #[inline]
            fn user_data(&self) -> &dyn Any {
                &*self.user_data
            }

            #[inline]
            fn user_data_mut(&mut self) -> &mut dyn Any {
                &mut *self.user_data
            }

            #[inline]
            fn set_user_data(&mut self, data: impl Any){
                self.user_data = Box::new(data);
            }
        }
    };
}

#[macro_use]
mod mutations;

mod document_end;
mod element;
mod tokens;

#[cfg(test)]
mod test_utils {
    use crate::test_utils::{Output, ASCII_COMPATIBLE_ENCODINGS};
    use crate::*;
    use encoding_rs::Encoding;
    use std::borrow::Cow;

    pub fn encoded(input: &str) -> Vec<(Vec<u8>, &'static Encoding)> {
        ASCII_COMPATIBLE_ENCODINGS
            .iter()
            .filter_map(|enc| {
                let (input, _, has_unmappable_characters) = enc.encode(input);

                // NOTE: there is no character in existence outside of ASCII range
                // that can be represented in all the ASCII-compatible encodings.
                // So, if test cases contains some non-ASCII characters that can't
                // be represented in the given encoding then we just skip it.
                // It is OK to do so, because our intention is not to test the
                // encoding library itself (it is already well tested), but test
                // how our own code works with non-ASCII characters.
                if has_unmappable_characters {
                    None
                } else {
                    Some((input.into_owned(), *enc))
                }
            })
            .collect()
    }

    pub fn rewrite_html<'a>(
        html: &[u8],
        encoding: &'static Encoding,
        element_content_handlers: Vec<(Cow<'a, Selector>, ElementContentHandlers)>,
        document_content_handlers: Vec<DocumentContentHandlers>,
    ) -> String {
        let mut output = Output::new(encoding);

        {
            let mut rewriter = HtmlRewriter::new(
                Settings {
                    element_content_handlers,
                    document_content_handlers,
                    encoding: AsciiCompatibleEncoding::new(encoding).unwrap(),
                    ..Settings::default()
                },
                |c: &[u8]| output.push(c),
            );

            rewriter.write(html).unwrap();
            rewriter.end().unwrap();
        }

        output.into()
    }
}
