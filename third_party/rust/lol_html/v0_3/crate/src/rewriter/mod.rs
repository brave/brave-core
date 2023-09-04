mod handlers_dispatcher;
mod rewrite_controller;

#[macro_use]
mod settings;

use self::handlers_dispatcher::ContentHandlersDispatcher;
use self::rewrite_controller::*;
use crate::memory::MemoryLimitExceededError;
use crate::memory::MemoryLimiter;
use crate::parser::ParsingAmbiguityError;
use crate::selectors_vm::{self, SelectorMatchingVm};
use crate::transform_stream::*;
use encoding_rs::Encoding;
use std::error::Error as StdError;
use std::fmt::{self, Debug};
use std::rc::Rc;
use thiserror::Error;

pub use self::settings::*;

/// This is an encoding known to be ASCII-compatible.
///
/// Non-ASCII-compatible encodings (`UTF-16LE`, `UTF-16BE`, `ISO-2022-JP` and
/// `replacement`) are not supported by lol_html.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct AsciiCompatibleEncoding(&'static Encoding);

impl AsciiCompatibleEncoding {
    /// Returns `Some` if `Encoding` is ascii-compatible, or `None` otherwise.
    pub fn new(encoding: &'static Encoding) -> Option<Self> {
        if encoding.is_ascii_compatible() {
            Some(Self(encoding))
        } else {
            None
        }
    }
}

impl From<AsciiCompatibleEncoding> for &'static Encoding {
    fn from(ascii_enc: AsciiCompatibleEncoding) -> &'static Encoding {
        ascii_enc.0
    }
}

impl std::convert::TryFrom<&'static Encoding> for AsciiCompatibleEncoding {
    type Error = ();

    fn try_from(enc: &'static Encoding) -> Result<Self, ()> {
        Self::new(enc).ok_or(())
    }
}

/// A compound error type that can be returned by [`write`] and [`end`] methods of the rewriter.
///
/// # Note
/// This error is unrecoverable. The rewriter instance will panic on attempt to use it after such an
/// error.
///
/// [`write`]: ../struct.HtmlRewriter.html#method.write
/// [`end`]: ../struct.HtmlRewriter.html#method.end
#[derive(Error, Debug)]
pub enum RewritingError {
    /// See [`MemoryLimitExceededError`].
    ///
    /// [`MemoryLimitExceededError`]: struct.MemoryLimitExceededError.html
    #[error("{0}")]
    MemoryLimitExceeded(MemoryLimitExceededError),

    /// See [`ParsingAmbiguityError`].
    ///
    /// [`ParsingAmbiguityError`]: struct.ParsingAmbiguityError.html
    #[error("{0}")]
    ParsingAmbiguity(ParsingAmbiguityError),

    /// An error that was propagated from one of the content handlers.
    #[error("{0}")]
    ContentHandlerError(Box<dyn StdError + Send + Sync>),
}

/// A streaming HTML rewriter.
///
/// # Example
/// ```
/// use lol_html::{element, HtmlRewriter, Settings};
///
/// let mut output = vec![];
///
/// {
///     let mut rewriter = HtmlRewriter::new(
///         Settings {
///             element_content_handlers: vec![
///                 // Rewrite insecure hyperlinks
///                 element!("a[href]", |el| {
///                     let href = el
///                         .get_attribute("href")
///                         .unwrap()
///                         .replace("http:", "https:");
///
///                     el.set_attribute("href", &href).unwrap();
///
///                     Ok(())
///                 })
///             ],
///             ..Settings::default()
///         },
///         |c: &[u8]| output.extend_from_slice(c)
///     );
///
///     rewriter.write(b"<div><a href=").unwrap();
///     rewriter.write(b"http://example.com>").unwrap();
///     rewriter.write(b"</a></div>").unwrap();
///     rewriter.end().unwrap();
/// }
///
/// assert_eq!(
///     String::from_utf8(output).unwrap(),
///     r#"<div><a href="https://example.com"></a></div>"#
/// );
/// ```
pub struct HtmlRewriter<'h, O: OutputSink> {
    stream: TransformStream<HtmlRewriteController<'h>, O>,
    poisoned: bool,
}

macro_rules! guarded {
    ($self:ident, $expr:expr) => {{
        assert!(
            !$self.poisoned,
            "Attempt to use the HtmlRewriter after a fatal error."
        );

        let res = $expr;

        if res.is_err() {
            $self.poisoned = true;
        }

        res
    }};
}

impl<'h, O: OutputSink> HtmlRewriter<'h, O> {
    /// Constructs a new rewriter with the provided `settings` that writes
    /// the output to the `output_sink`.
    ///
    /// # Note
    ///
    /// For the convenience the [`OutputSink`] trait is implemented for closures.
    ///
    /// [`OutputSink`]: trait.OutputSink.html
    pub fn new<'s>(settings: Settings<'h, 's>, output_sink: O) -> Self {
        let encoding = settings.encoding;
        let mut selectors_ast = selectors_vm::Ast::default();
        let mut dispatcher = ContentHandlersDispatcher::default();
        let has_selectors = !settings.element_content_handlers.is_empty();

        for (selector, handlers) in settings.element_content_handlers {
            let locator = dispatcher.add_selector_associated_handlers(handlers);

            selectors_ast.add_selector(&selector, locator);
        }

        for handlers in settings.document_content_handlers {
            dispatcher.add_document_content_handlers(handlers);
        }

        let memory_limiter =
            MemoryLimiter::new_shared(settings.memory_settings.max_allowed_memory_usage);

        let selector_matching_vm = if has_selectors {
            Some(SelectorMatchingVm::new(
                selectors_ast,
                encoding.into(),
                Rc::clone(&memory_limiter),
                settings.enable_esi_tags,
            ))
        } else {
            None
        };

        let controller = HtmlRewriteController::new(dispatcher, selector_matching_vm);

        let stream = TransformStream::new(TransformStreamSettings {
            transform_controller: controller,
            output_sink,
            preallocated_parsing_buffer_size: settings
                .memory_settings
                .preallocated_parsing_buffer_size,
            memory_limiter,
            encoding: encoding.into(),
            strict: settings.strict,
        });

        HtmlRewriter {
            stream,
            poisoned: false,
        }
    }

    /// Writes a chunk of input data to the rewriter.
    ///
    /// # Panics
    ///  * If previous invocation of the method returned a [`RewritingError`]
    ///    (these errors are unrecovarable).
    ///
    /// [`RewritingError`]: errors/enum.RewritingError.html
    /// [`end`]: struct.HtmlRewriter.html#method.end
    #[inline]
    pub fn write(&mut self, data: &[u8]) -> Result<(), RewritingError> {
        guarded!(self, self.stream.write(data))
    }

    /// Finalizes the rewriting process.
    ///
    /// Should be called once the last chunk of the input is written.
    ///
    /// # Panics
    ///  * If previous invocation of [`write`] returned a [`RewritingError`] (these errors
    ///    are unrecovarable).
    ///
    /// [`RewritingError`]: errors/enum.RewritingError.html
    /// [`write`]: struct.HtmlRewriter.html#method.write
    #[inline]
    pub fn end(mut self) -> Result<(), RewritingError> {
        guarded!(self, self.stream.end())
    }
}

// NOTE: this opaque Debug implementation is required to make
// `.unwrap()` and `.expect()` methods available on Result
// returned by the `HtmlRewriterBuilder.build()` method.
impl<O: OutputSink> Debug for HtmlRewriter<'_, O> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "HtmlRewriter")
    }
}

/// Rewrites given `html` string with the provided `settings`.
///
/// # Example
///
/// ```
/// use lol_html::{rewrite_str, element, RewriteStrSettings};
///
/// let element_content_handlers = vec![
///     // Rewrite insecure hyperlinks
///     element!("a[href]", |el| {
///         let href = el
///             .get_attribute("href")
///             .unwrap()
///             .replace("http:", "https:");
///
///          el.set_attribute("href", &href).unwrap();
///
///          Ok(())
///     })
/// ];
/// let output = rewrite_str(
///     r#"<div><a href="http://example.com"></a></div>"#,
///     RewriteStrSettings {
///         element_content_handlers,
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
///
/// assert_eq!(output, r#"<div><a href="https://example.com"></a></div>"#);
/// ```
pub fn rewrite_str<'h, 's>(
    html: &str,
    settings: impl Into<Settings<'h, 's>>,
) -> Result<String, RewritingError> {
    let mut output = vec![];

    let mut rewriter = HtmlRewriter::new(settings.into(), |c: &[u8]| {
        output.extend_from_slice(c);
    });

    rewriter.write(html.as_bytes())?;
    rewriter.end()?;

    // NOTE: it's ok to unwrap here as we guarantee encoding validity of the output
    Ok(String::from_utf8(output).unwrap())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::html_content::ContentType;
    use crate::test_utils::{Output, ASCII_COMPATIBLE_ENCODINGS, NON_ASCII_COMPATIBLE_ENCODINGS};
    use encoding_rs::Encoding;
    use std::cell::RefCell;
    use std::convert::TryInto;
    use std::rc::Rc;

    fn write_chunks<O: OutputSink>(
        mut rewriter: HtmlRewriter<O>,
        encoding: &'static Encoding,
        chunks: &[&str],
    ) {
        for chunk in chunks {
            let (chunk, _, _) = encoding.encode(chunk);

            rewriter.write(&*chunk).unwrap();
        }

        rewriter.end().unwrap();
    }

    #[test]
    fn rewrite_html_str() {
        let res = rewrite_str(
            "<!-- 42 --><div><!--hi--></div>",
            RewriteStrSettings {
                element_content_handlers: vec![
                    element!("div", |el| {
                        el.set_tag_name("span").unwrap();
                        Ok(())
                    }),
                    comments!("div", |c| {
                        c.set_text("hello").unwrap();
                        Ok(())
                    }),
                ],
                ..RewriteStrSettings::default()
            },
        )
        .unwrap();

        assert_eq!(res, "<!-- 42 --><span><!--hello--></span>");
    }

    #[test]
    fn rewrite_arbitrary_settings() {
        let res = rewrite_str("<span>Some text</span>", Settings::default()).unwrap();
        assert_eq!(res, "<span>Some text</span>");
    }

    #[test]
    fn non_ascii_compatible_encoding() {
        for encoding in &NON_ASCII_COMPATIBLE_ENCODINGS {
            assert_eq!(AsciiCompatibleEncoding::new(encoding), None);
        }
    }

    #[test]
    fn doctype_info() {
        for &enc in ASCII_COMPATIBLE_ENCODINGS.iter() {
            let mut doctypes = Vec::default();

            {
                let rewriter = HtmlRewriter::new(
                    Settings {
                        document_content_handlers: vec![doctype!(|d| {
                            doctypes.push((d.name(), d.public_id(), d.system_id()));
                            Ok(())
                        })],
                        // NOTE: unwrap() here is intentional; it also tests `Ascii::new`.
                        encoding: enc.try_into().unwrap(),
                        ..Settings::default()
                    },
                    |_: &[u8]| {},
                );

                write_chunks(
                    rewriter,
                    enc,
                    &[
                        "<!doctype html1>",
                        "<!-- test --><div>",
                        r#"<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "#,
                        r#""http://www.w3.org/TR/html4/strict.dtd">"#,
                        "</div><!DoCtYPe ",
                    ],
                );
            }

            assert_eq!(
                doctypes,
                &[
                    (Some("html1".into()), None, None),
                    (
                        Some("html".into()),
                        Some("-//W3C//DTD HTML 4.01//EN".into()),
                        Some("http://www.w3.org/TR/html4/strict.dtd".into())
                    ),
                    (None, None, None),
                ]
            );
        }
    }

    #[test]
    fn rewrite_start_tags() {
        for &enc in ASCII_COMPATIBLE_ENCODINGS.iter() {
            let actual: String = {
                let mut output = Output::new(enc);

                let rewriter = HtmlRewriter::new(
                    Settings {
                        element_content_handlers: vec![element!("*", |el| {
                            el.set_attribute("foo", "bar").unwrap();
                            el.prepend("<test></test>", ContentType::Html);
                            Ok(())
                        })],
                        encoding: enc.try_into().unwrap(),
                        ..Settings::default()
                    },
                    |c: &[u8]| output.push(c),
                );

                write_chunks(
                    rewriter,
                    enc,
                    &[
                        "<!doctype html>\n",
                        "<html>\n",
                        "   <head></head>\n",
                        "   <body>\n",
                        "       <div>Test</div>\n",
                        "   </body>\n",
                        "</html>",
                    ],
                );

                output.into()
            };

            assert_eq!(
                actual,
                concat!(
                    "<!doctype html>\n",
                    "<html foo=\"bar\"><test></test>\n",
                    "   <head foo=\"bar\"><test></test></head>\n",
                    "   <body foo=\"bar\"><test></test>\n",
                    "       <div foo=\"bar\"><test></test>Test</div>\n",
                    "   </body>\n",
                    "</html>",
                )
            );
        }
    }

    #[test]
    fn rewrite_document_content() {
        for &enc in ASCII_COMPATIBLE_ENCODINGS.iter() {
            let actual: String = {
                let mut output = Output::new(enc);

                let rewriter = HtmlRewriter::new(
                    Settings {
                        element_content_handlers: vec![],
                        document_content_handlers: vec![
                            doc_comments!(|c| {
                                c.set_text(&(c.text() + "1337")).unwrap();
                                Ok(())
                            }),
                            doc_text!(|c| {
                                if c.last_in_text_node() {
                                    c.after("BAZ", ContentType::Text);
                                }

                                Ok(())
                            }),
                        ],
                        encoding: enc.try_into().unwrap(),
                        ..Settings::default()
                    },
                    |c: &[u8]| output.push(c),
                );

                write_chunks(
                    rewriter,
                    enc,
                    &[
                        "<!doctype html>\n",
                        "<!-- hey -->\n",
                        "<html>\n",
                        "   <head><!-- aloha --></head>\n",
                        "   <body>\n",
                        "       <div>Test</div>\n",
                        "   </body>\n",
                        "   <!-- bonjour -->\n",
                        "</html>Pshhh",
                    ],
                );

                output.into()
            };

            assert_eq!(
                actual,
                concat!(
                    "<!doctype html>\nBAZ",
                    "<!-- hey 1337-->\nBAZ",
                    "<html>\n",
                    "   BAZ<head><!-- aloha 1337--></head>\n",
                    "   BAZ<body>\n",
                    "       BAZ<div>TestBAZ</div>\n",
                    "   BAZ</body>\n",
                    "   BAZ<!-- bonjour 1337-->\nBAZ",
                    "</html>PshhhBAZ",
                )
            );
        }
    }

    #[test]
    fn handler_invocation_order() {
        let handlers_executed = Rc::new(RefCell::new(Vec::default()));

        macro_rules! create_handlers {
            ($sel:expr, $idx:expr) => {
                element!($sel, {
                    let handlers_executed = Rc::clone(&handlers_executed);

                    move |_| {
                        handlers_executed.borrow_mut().push($idx);
                        Ok(())
                    }
                })
            };
        }

        let _res = rewrite_str(
            "<div><span foo></span></div>",
            RewriteStrSettings {
                element_content_handlers: vec![
                    create_handlers!("div span", 0),
                    create_handlers!("div > span", 1),
                    create_handlers!("span", 2),
                    create_handlers!("[foo]", 3),
                    create_handlers!("div span[foo]", 4),
                ],
                ..RewriteStrSettings::default()
            },
        )
        .unwrap();

        assert_eq!(*handlers_executed.borrow(), vec![0, 1, 2, 3, 4]);
    }

    #[test]
    fn write_esi_tags() {
        let res = rewrite_str(
            "<span><esi:include src=a></span>",
            RewriteStrSettings {
                element_content_handlers: vec![element!("esi\\:include", |el| {
                    el.replace("?", ContentType::Text);
                    Ok(())
                })],
                enable_esi_tags: true,
                ..RewriteStrSettings::default()
            },
        )
        .unwrap();

        assert_eq!(res, "<span>?</span>");
    }

    mod fatal_errors {
        use super::*;
        use crate::errors::MemoryLimitExceededError;

        fn create_rewriter<O: OutputSink>(
            max_allowed_memory_usage: usize,
            output_sink: O,
        ) -> HtmlRewriter<'static, O> {
            HtmlRewriter::new(
                Settings {
                    element_content_handlers: vec![element!("*", |_| Ok(()))],
                    memory_settings: MemorySettings {
                        max_allowed_memory_usage,
                        preallocated_parsing_buffer_size: 0,
                    },
                    ..Settings::default()
                },
                output_sink,
            )
        }

        #[test]
        fn buffer_capacity_limit() {
            const MAX: usize = 100;

            let mut rewriter = create_rewriter(MAX, |_: &[u8]| {});

            // Use two chunks for the stream to force the usage of the buffer and
            // make sure to overflow it.
            let chunk_1 = format!("<img alt=\"{}", "l".repeat(MAX / 2));
            let chunk_2 = format!("{}\" />", "r".repeat(MAX / 2));

            rewriter.write(chunk_1.as_bytes()).unwrap();

            let write_err = rewriter.write(chunk_2.as_bytes()).unwrap_err();

            match write_err {
                RewritingError::MemoryLimitExceeded(e) => assert_eq!(e, MemoryLimitExceededError),
                _ => panic!("{}", write_err),
            }
        }

        #[test]
        #[should_panic(expected = "Attempt to use the HtmlRewriter after a fatal error.")]
        fn poisoning_after_fatal_error() {
            const MAX: usize = 10;

            let mut rewriter = create_rewriter(MAX, |_: &[u8]| {});
            let chunk = format!("<img alt=\"{}", "l".repeat(MAX));

            rewriter.write(chunk.as_bytes()).unwrap_err();
            rewriter.end().unwrap_err();
        }

        #[test]
        fn content_handler_error_propagation() {
            fn assert_err(
                element_handlers: ElementContentHandlers,
                document_handlers: DocumentContentHandlers,
                expected_err: &'static str,
            ) {
                use std::borrow::Cow;

                let mut rewriter = HtmlRewriter::new(
                    Settings {
                        element_content_handlers: vec![(
                            Cow::Owned("*".parse().unwrap()),
                            element_handlers,
                        )],
                        document_content_handlers: vec![document_handlers],
                        ..Settings::default()
                    },
                    |_: &[u8]| {},
                );

                let chunks = [
                    "<!--doc comment--> Doc text",
                    "<div><!--el comment-->El text</div>",
                ];

                let mut err = None;

                for chunk in chunks.iter() {
                    match rewriter.write(chunk.as_bytes()) {
                        Ok(_) => (),
                        Err(e) => {
                            err = Some(e);
                            break;
                        }
                    }
                }

                if err.is_none() {
                    match rewriter.end() {
                        Ok(_) => (),
                        Err(e) => err = Some(e),
                    }
                }

                let err = format!("{}", err.expect("Error expected"));

                assert_eq!(err, expected_err);
            }

            assert_err(
                ElementContentHandlers::default(),
                doc_comments!(|_| Err("Error in doc comment handler".into())),
                "Error in doc comment handler",
            );

            assert_err(
                ElementContentHandlers::default(),
                doc_text!(|_| Err("Error in doc text handler".into())),
                "Error in doc text handler",
            );

            assert_err(
                ElementContentHandlers::default(),
                doc_text!(|_| Err("Error in doctype handler".into())),
                "Error in doctype handler",
            );

            assert_err(
                ElementContentHandlers::default()
                    .element(|_| Err("Error in element handler".into())),
                DocumentContentHandlers::default(),
                "Error in element handler",
            );

            assert_err(
                ElementContentHandlers::default()
                    .comments(|_| Err("Error in element comment handler".into())),
                DocumentContentHandlers::default(),
                "Error in element comment handler",
            );

            assert_err(
                ElementContentHandlers::default()
                    .text(|_| Err("Error in element text handler".into())),
                DocumentContentHandlers::default(),
                "Error in element text handler",
            );
        }
    }
}
