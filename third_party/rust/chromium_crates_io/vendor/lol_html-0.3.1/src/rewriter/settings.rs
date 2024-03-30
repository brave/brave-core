use crate::rewritable_units::{Comment, Doctype, DocumentEnd, Element, EndTag, TextChunk};
use crate::selectors_vm::Selector;
// N.B. `use crate::` will break this because the constructor is not public, only the struct itself
use super::AsciiCompatibleEncoding;
use std::borrow::Cow;
use std::error::Error;

pub(crate) type HandlerResult = Result<(), Box<dyn Error + Send + Sync>>;
pub type DoctypeHandler<'h> = Box<dyn FnMut(&mut Doctype) -> HandlerResult + 'h>;
pub type CommentHandler<'h> = Box<dyn FnMut(&mut Comment) -> HandlerResult + 'h>;
pub type TextHandler<'h> = Box<dyn FnMut(&mut TextChunk) -> HandlerResult + 'h>;
pub type ElementHandler<'h> = Box<dyn FnMut(&mut Element) -> HandlerResult + 'h>;
pub type EndTagHandler<'h> = Box<dyn FnOnce(&mut EndTag) -> HandlerResult + 'h>;
pub type EndHandler<'h> = Box<dyn FnOnce(&mut DocumentEnd) -> HandlerResult + 'h>;

/// Specifies element content handlers associated with a selector.
#[derive(Default)]
pub struct ElementContentHandlers<'h> {
    pub(super) element: Option<ElementHandler<'h>>,
    pub(super) comments: Option<CommentHandler<'h>>,
    pub(super) text: Option<TextHandler<'h>>,
}

impl<'h> ElementContentHandlers<'h> {
    /// Sets a handler for elements matched by a selector.
    #[inline]
    pub fn element(mut self, handler: impl FnMut(&mut Element) -> HandlerResult + 'h) -> Self {
        self.element = Some(Box::new(handler));

        self
    }

    /// Sets a handler for HTML comments in the inner content of elements matched by a selector.
    #[inline]
    pub fn comments(mut self, handler: impl FnMut(&mut Comment) -> HandlerResult + 'h) -> Self {
        self.comments = Some(Box::new(handler));

        self
    }

    /// Sets a handler for text chunks in the inner content of elements matched by a selector.
    #[inline]
    pub fn text(mut self, handler: impl FnMut(&mut TextChunk) -> HandlerResult + 'h) -> Self {
        self.text = Some(Box::new(handler));

        self
    }
}

/// Specifies document-level content handlers.
///
/// Some content can't be captured by CSS selectors as it lays outside of content of any
/// of the HTML elements. Document-level handlers allow capture such a content:
///
/// ```html
/// <!doctype html>
/// <!--
///     I can't be captured with a selector, but I can be
///     captured with a document-level comment handler
/// -->
/// <html>
/// <!-- I can be captured with a selector -->
/// </html>
/// ```
#[derive(Default)]
pub struct DocumentContentHandlers<'h> {
    pub(super) doctype: Option<DoctypeHandler<'h>>,
    pub(super) comments: Option<CommentHandler<'h>>,
    pub(super) text: Option<TextHandler<'h>>,
    pub(super) end: Option<EndHandler<'h>>,
}

impl<'h> DocumentContentHandlers<'h> {
    /// Sets a handler for the [document type declaration].
    ///
    /// [document type declaration]: https://developer.mozilla.org/en-US/docs/Glossary/Doctype
    #[inline]
    pub fn doctype(mut self, handler: impl FnMut(&mut Doctype) -> HandlerResult + 'h) -> Self {
        self.doctype = Some(Box::new(handler));

        self
    }

    /// Sets a handler for all HTML comments present in the input HTML markup.
    #[inline]
    pub fn comments(mut self, handler: impl FnMut(&mut Comment) -> HandlerResult + 'h) -> Self {
        self.comments = Some(Box::new(handler));

        self
    }

    /// Sets a handler for all text chunks present in the input HTML markup.
    #[inline]
    pub fn text(mut self, handler: impl FnMut(&mut TextChunk) -> HandlerResult + 'h) -> Self {
        self.text = Some(Box::new(handler));

        self
    }

    /// Sets a handler for the document end, which is called after the last chunk is processed.
    #[inline]
    pub fn end(mut self, handler: impl FnMut(&mut DocumentEnd) -> HandlerResult + 'h) -> Self {
        self.end = Some(Box::new(handler));

        self
    }
}

#[doc(hidden)]
#[macro_export]
macro_rules! __element_content_handler {
    ($selector:expr, $handler_name:ident, $handler:expr) => {
        (
            ::std::borrow::Cow::Owned($selector.parse::<$crate::Selector>().unwrap()),
            $crate::ElementContentHandlers::default().$handler_name($handler),
        )
    };
}

/// A convenience macro to construct a rewriting handler for elements that can be matched by the
/// specified CSS selector.
///
/// # Example
/// ```
/// use lol_html::{rewrite_str, element, RewriteStrSettings};
/// use lol_html::html_content::ContentType;
///
/// let html = rewrite_str(
///     r#"<span id="foo"></span>"#,
///     RewriteStrSettings {
///         element_content_handlers: vec![
///             element!("#foo", |el| {
///                 el.set_inner_content("Hello!", ContentType::Text);
///
///                 Ok(())
///             })
///         ],
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
///
/// assert_eq!(html, r#"<span id="foo">Hello!</span>"#);
/// ```
#[macro_export(local_inner_macros)]
macro_rules! element {
    ($selector:expr, $handler:expr) => {
        __element_content_handler!($selector, element, $handler)
    };
}

/// A convenience macro to construct a rewriting handler for text chunks in the inner content of an
/// element that can be matched by the specified CSS selector.
///
/// # Example
/// ```
/// use lol_html::{rewrite_str, text, RewriteStrSettings};
/// use lol_html::html_content::ContentType;
///
/// let html = rewrite_str(
///     r#"<span>Hello</span>"#,
///     RewriteStrSettings {
///         element_content_handlers: vec![
///             text!("span", |t| {
///                 if t.last_in_text_node() {
///                     t.after(" world", ContentType::Text);
///                 }
///
///                 Ok(())
///             })
///         ],
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
///
/// assert_eq!(html, r#"<span>Hello world</span>"#);
/// ```
#[macro_export(local_inner_macros)]
macro_rules! text {
    ($selector:expr, $handler:expr) => {
        __element_content_handler!($selector, text, $handler)
    };
}

/// A convenience macro to construct a rewriting handler for HTML comments in the inner content of
/// an element that can be matched by the specified CSS selector.
///
/// # Example
/// ```
/// use lol_html::{rewrite_str, comments, RewriteStrSettings};
/// use lol_html::html_content::ContentType;
///
/// let html = rewrite_str(
///     r#"<span><!-- 42 --></span>"#,
///     RewriteStrSettings {
///         element_content_handlers: vec![
///             comments!("span", |c| {
///                 c.set_text("Hello!").unwrap();
///
///                 Ok(())
///             })
///         ],
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
///
/// assert_eq!(html, r#"<span><!--Hello!--></span>"#);
/// ```
#[macro_export(local_inner_macros)]
macro_rules! comments {
    ($selector:expr, $handler:expr) => {
        __element_content_handler!($selector, comments, $handler)
    };
}

#[doc(hidden)]
#[macro_export]
macro_rules! __document_content_handler {
    ($handler_name:ident, $handler:expr) => {
        $crate::DocumentContentHandlers::default().$handler_name($handler)
    };
}

/// A convenience macro to construct a handler for [document type declarations] in the HTML document.
///
/// # Example
/// ```
/// use lol_html::{rewrite_str, doctype, RewriteStrSettings};
/// use lol_html::html_content::ContentType;
///
/// rewrite_str(
///     r#"<!doctype html>"#,
///     RewriteStrSettings {
///         document_content_handlers: vec![
///             doctype!(|d| {
///                 assert_eq!(d.name().unwrap(), "html");
///
///                 Ok(())
///             })
///         ],
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
/// ```
///
/// [document type declarations]: https://developer.mozilla.org/en-US/docs/Glossary/Doctype
#[macro_export(local_inner_macros)]
macro_rules! doctype {
    ($handler:expr) => {
        __document_content_handler!(doctype, $handler)
    };
}

/// A convenience macro to construct a rewriting handler for all text chunks in the HTML document.
///
/// # Example
/// ```
/// use lol_html::{rewrite_str, doc_text, RewriteStrSettings};
/// use lol_html::html_content::ContentType;
///
/// let html = rewrite_str(
///     r#"Hello<span>Hello</span>Hello"#,
///     RewriteStrSettings {
///         document_content_handlers: vec![
///             doc_text!(|t| {
///                 if t.last_in_text_node() {
///                     t.after(" world", ContentType::Text);
///                 }
///
///                 Ok(())
///             })
///         ],
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
///
/// assert_eq!(html, r#"Hello world<span>Hello world</span>Hello world"#);
/// ```
#[macro_export(local_inner_macros)]
macro_rules! doc_text {
    ($handler:expr) => {
        __document_content_handler!(text, $handler)
    };
}

/// A convenience macro to construct a rewriting handler for all HTML comments in the HTML document.
///
/// # Example
/// ```
/// use lol_html::{rewrite_str, doc_comments, RewriteStrSettings};
/// use lol_html::html_content::ContentType;
///
/// let html = rewrite_str(
///     r#"<!-- 42 --><span><!-- 42 --></span><!-- 42 -->"#,
///     RewriteStrSettings {
///         document_content_handlers: vec![
///             doc_comments!(|c| {
///                 c.set_text("Hello!").unwrap();
///
///                 Ok(())
///             })
///         ],
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
///
/// assert_eq!(html, r#"<!--Hello!--><span><!--Hello!--></span><!--Hello!-->"#);
/// ```
#[macro_export(local_inner_macros)]
macro_rules! doc_comments {
    ($handler:expr) => {
        __document_content_handler!(comments, $handler)
    };
}

/// A convenience macro to construct a rewriting handler for the end of the document.
///
/// This handler will only be called after the rewriter has finished processing the final chunk.
///
/// # Example
/// ```
/// use lol_html::{rewrite_str, element, end, RewriteStrSettings};
/// use lol_html::html_content::ContentType;
///
/// let html = rewrite_str(
///     r#"<span>foo</span>"#,
///     RewriteStrSettings {
///         element_content_handlers: vec![
///             element!("span", |el| {
///                 el.append("bar", ContentType::Text);
///
///                 Ok(())
///             })
///         ],
///         document_content_handlers: vec![
///             end!(|end| {
///                 end.append("<div>baz</div>", ContentType::Html);
///
///                 Ok(())
///             })
///         ],
///         ..RewriteStrSettings::default()
///     }
/// ).unwrap();
///
/// assert_eq!(html, r#"<span>foobar</span><div>baz</div>"#);
/// ```
#[macro_export(local_inner_macros)]
macro_rules! end {
    ($handler:expr) => {
        __document_content_handler!(end, $handler)
    };
}

/// Specifies the memory settings for [`HtmlRewriter`].
///
/// [`HtmlRewriter`]: struct.HtmlRewriter.html
// NOTE: exposed in C API as well, thus repr(C).
#[repr(C)]
pub struct MemorySettings {
    /// Specifies the number of bytes that should be preallocated on [`HtmlRewriter`] instantiation
    /// for the internal parsing buffer.
    ///
    /// In some cases (e.g. when rewriter encounters a start tag represented by two or more input
    /// chunks) the rewriter needs to buffer input content.
    ///
    /// Internal parsing buffer is used in such cases. Reallocations and, thus, performance
    /// degradation can be avoided by preallocating the buffer ahead of time. As a drawback of
    /// this approach, every instance of the rewriter will consume the preallocated amount of
    /// memory.
    ///
    /// It's up to the user to adjust the limit according to their environment limitations.
    ///
    /// ### Default
    ///
    /// `1024` bytes when constructed with `MemorySettings::default()`.
    ///
    /// [`HtmlRewriter`]: struct.HtmlRewriter.html
    pub preallocated_parsing_buffer_size: usize,

    /// Sets a hard limit in bytes on memory consumption of a [`HtmlRewriter`] instance.
    ///
    /// Rewriter's [`write`] and [`end`] methods will error if this limit is exceeded.
    ///
    /// Note, that value doesn't reflect the exact threshold after which the rewriter will bailout.
    /// It is impossible to account for all the memory consumed without a significant performance
    /// penalty. So, instead, we try to provide the best approximation by measuring the memory
    /// consumed by internal buffers that grow depending on the input.
    ///
    /// ### Default
    ///
    /// [`std::usize::MAX`] when constructed with `MemorySettings::default()`.
    ///
    /// [`HtmlRewriter`]: struct.HtmlRewriter.html
    /// [`std::usize::MAX`]: https://doc.rust-lang.org/std/usize/constant.MAX.html
    /// [`write`]: struct.HtmlRewriter.html#method.write
    /// [`end`]: struct.HtmlRewriter.html#method.end
    pub max_allowed_memory_usage: usize,
}

impl Default for MemorySettings {
    #[inline]
    fn default() -> Self {
        MemorySettings {
            preallocated_parsing_buffer_size: 1024,
            max_allowed_memory_usage: std::usize::MAX,
        }
    }
}

/// Specifies settings for [`HtmlRewriter`].
///
/// [`HtmlRewriter`]: struct.HtmlRewriter.html
pub struct Settings<'h, 's> {
    /// Specifies CSS selectors and rewriting handlers for elements and their inner content.
    ///
    /// ### Hint
    ///
    /// [`element`], [`comments`] and [`text`] convenience macros can be used to construct a
    /// `(Selector, ElementContentHandlers)` tuple.
    ///
    /// ### Example
    /// ```
    /// use std::borrow::Cow;
    /// use lol_html::{ElementContentHandlers, Settings};
    ///
    /// let settings = Settings {
    ///     element_content_handlers: vec! [
    ///         (
    ///             Cow::Owned("div[foo]".parse().unwrap()),
    ///             ElementContentHandlers::default().element(|el| {
    ///                 // ...
    ///
    ///                 Ok(())
    ///             })
    ///         ),
    ///         (
    ///             Cow::Owned("body".parse().unwrap()),
    ///             ElementContentHandlers::default().comments(|c| {
    ///                 // ...
    ///
    ///                 Ok(())
    ///             })
    ///         )
    ///     ],
    ///     ..Settings::default()
    /// };
    /// ```
    ///
    /// [`element`]: macro.element.html
    /// [`comments`]: macro.comments.html
    /// [`text`]: macro.text.html
    pub element_content_handlers: Vec<(Cow<'s, Selector>, ElementContentHandlers<'h>)>,

    /// Specifies rewriting handlers for the content without associating it to a particular
    /// CSS selector.
    ///
    /// Refer to [`DocumentContentHandlers`] documentation for more information.
    ///
    /// ### Hint
    /// [`doctype`], [`doc_comments`] and [`doc_text`] convenience macros can be used to construct
    /// items of this vector.
    ///
    /// [`DocumentContentHandlers`]: struct.DocumentContentHandlers.html
    /// [`doctype`]: macro.doctype.html
    /// [`doc_comments`]: macro.doc_comments.html
    /// [`doc_text`]: macro.doc_text.html
    pub document_content_handlers: Vec<DocumentContentHandlers<'h>>,

    /// Specifies the [character encoding] for the input and the output of the rewriter.
    ///
    /// Can be a [label] for any of the web-compatible encodings with an exception for `UTF-16LE`,
    /// `UTF-16BE`, `ISO-2022-JP` and `replacement` (these non-ASCII-compatible encodings
    /// are not supported).
    ///
    /// [character encoding]: https://developer.mozilla.org/en-US/docs/Glossary/character_encoding
    /// [label]: https://encoding.spec.whatwg.org/#names-and-labels
    ///
    /// ### Default
    ///
    /// `"utf-8"` when constructed with `Settings::default()`.
    pub encoding: AsciiCompatibleEncoding,

    /// Specifies the memory settings.
    pub memory_settings: MemorySettings,

    /// If set to `true` the rewriter bails out if it encounters markup that drives the HTML parser
    /// into ambigious state.
    ///
    /// Since the rewriter operates on a token stream and doesn't have access to a full
    /// DOM-tree, there are certain rare cases of non-conforming HTML markup which can't be
    /// guaranteed to be parsed correctly without an ability to backtrace the tree.
    ///
    /// Therefore, due to security considerations, sometimes it's preferable to abort the
    /// rewriting process in case of such uncertainty.
    ///
    /// One of the simplest examples of such markup is the following:
    ///
    /// ```html
    /// ...
    /// <select><xmp><script>"use strict";</script></select>
    /// ...
    /// ```
    ///
    /// The `<xmp>` element is not allowed inside the `<select>` element, so in a browser the start
    /// tag for `<xmp>` will be ignored and following `<script>` element will be parsed and executed.
    ///
    /// On the other hand, the `<select>` element itself can be also ignored depending on the
    /// context in which it was parsed. In this case, the `<xmp>` element will not be ignored
    /// and the `<script>` element along with its content will be parsed as a simple text inside
    /// it.
    ///
    /// So, in this case the parser needs an ability to backtrace the DOM-tree to figure out the
    /// correct parsing context.
    ///
    /// ### Default
    ///
    /// `true` when constructed with `Settings::default()`.
    pub strict: bool,

    pub enable_esi_tags: bool,
}

impl Default for Settings<'_, '_> {
    #[inline]
    fn default() -> Self {
        Settings {
            element_content_handlers: vec![],
            document_content_handlers: vec![],
            encoding: AsciiCompatibleEncoding(encoding_rs::UTF_8),
            memory_settings: MemorySettings::default(),
            strict: true,
            enable_esi_tags: false,
        }
    }
}

impl<'h, 's> From<RewriteStrSettings<'h, 's>> for Settings<'h, 's> {
    #[inline]
    fn from(settings: RewriteStrSettings<'h, 's>) -> Self {
        Settings {
            element_content_handlers: settings.element_content_handlers,
            document_content_handlers: settings.document_content_handlers,
            strict: settings.strict,
            enable_esi_tags: settings.enable_esi_tags,
            ..Settings::default()
        }
    }
}

/// Specifies settings for the [`rewrite_str`] function.
///
/// [`rewrite_str`]: fn.rewrite_str.html
pub struct RewriteStrSettings<'h, 's> {
    /// Specifies CSS selectors and rewriting handlers for elements and their inner content.
    ///
    /// ### Hint
    ///
    /// [`element`], [`comments`] and [`text`] convenience macros can be used to construct a
    /// `(Selector, ElementContentHandlers)` tuple.
    ///
    /// ### Example
    /// ```
    /// use std::borrow::Cow;
    /// use lol_html::{ElementContentHandlers, RewriteStrSettings};
    ///
    /// let settings = RewriteStrSettings {
    ///     element_content_handlers: vec! [
    ///         (
    ///             Cow::Owned("div[foo]".parse().unwrap()),
    ///             ElementContentHandlers::default().element(|el| {
    ///                 // ...
    ///
    ///                 Ok(())
    ///             })
    ///         ),
    ///         (
    ///             Cow::Owned("div[foo]".parse().unwrap()),
    ///             ElementContentHandlers::default().comments(|c| {
    ///                 // ...
    ///
    ///                 Ok(())
    ///             })
    ///         )
    ///     ],
    ///     ..RewriteStrSettings::default()
    /// };
    /// ```
    ///
    /// [`element`]: macro.element.html
    /// [`comments`]: macro.comments.html
    /// [`text`]: macro.text.html
    pub element_content_handlers: Vec<(Cow<'s, Selector>, ElementContentHandlers<'h>)>,

    /// Specifies rewriting handlers for the content without associating it to a particular
    /// CSS selector.
    ///
    /// Refer to [`DocumentContentHandlers`] documentation for more information.
    ///
    /// ### Hint
    /// [`doctype`], [`doc_comments`] and [`doc_text`] convenience macros can be used to construct
    /// items of this vector.
    ///
    /// [`DocumentContentHandlers`]: struct.DocumentContentHandlers.html
    /// [`doctype`]: macro.doctype.html
    /// [`doc_comments`]: macro.doc_comments.html
    /// [`doc_text`]: macro.doc_text.html
    pub document_content_handlers: Vec<DocumentContentHandlers<'h>>,

    /// If set to `true` the rewriter bails out if it encounters markup that drives the HTML parser
    /// into ambigious state.
    ///
    /// Since the rewriter operates on a token stream and doesn't have access to a full
    /// DOM-tree, there are certain rare cases of non-conforming HTML markup which can't be
    /// guaranteed to be parsed correctly without an ability to backtrace the tree.
    ///
    /// Therefore, due to security considerations, sometimes it's preferable to abort the
    /// rewriting process in case of such uncertainty.
    ///
    /// One of the simplest examples of such markup is the following:
    ///
    /// ```html
    /// ...
    /// <select><xmp><script>"use strict";</script></select>
    /// ...
    /// ```
    ///
    /// The `<xmp>` element is not allowed inside the `<select>` element, so in a browser the start
    /// tag for `<xmp>` will be ignored and following `<script>` element will be parsed and executed.
    ///
    /// On the other hand, the `<select>` element itself can be also ignored depending on the
    /// context in which it was parsed. In this case, the `<xmp>` element will not be ignored
    /// and the `<script>` element along with its content will be parsed as a simple text inside
    /// it.
    ///
    /// So, in this case the parser needs an ability to backtrace the DOM-tree to figure out the
    /// correct parsing context.
    ///
    /// ### Default
    ///
    /// `true` when constructed with `Settings::default()`.
    pub strict: bool,

    pub enable_esi_tags: bool,
}

impl Default for RewriteStrSettings<'_, '_> {
    #[inline]
    fn default() -> Self {
        RewriteStrSettings {
            element_content_handlers: vec![],
            document_content_handlers: vec![],
            strict: true,
            enable_esi_tags: true,
        }
    }
}
