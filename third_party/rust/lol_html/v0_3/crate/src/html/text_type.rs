use cfg_if::cfg_if;

/// A type of parsed text.
///
/// Parsing context adds certain limitations for the textual content. E.g., it's unsafe to
/// rewrite text inside `<script>` element with a string that contains `"</script>"` substring as
/// this will preemptively close the `<script>` element, possibly introducing an XSS attack vector.
/// As other example, some parsing contexts don't allow [HTML entities] in text. Thus, rewriting
/// content of a `<style>` element with text that contains HTML entities may cause a CSS parsing
/// error in a browser, because entities won't be decoded by a browser in this context.
///
/// Text type provides users of the rewriter with a capability to assess the context in which text
/// parsing is hapenning and make informed decision about preprocessing of the textual content
/// replacement.
///
/// The names of the text types are taken from the [HTML parsing specification].
///
/// [HTML entities]: https://developer.mozilla.org/en-US/docs/Glossary/Entity
/// [HTML parsing specification]: https://html.spec.whatwg.org/multipage/parsing.html
#[derive(Copy, Clone, PartialEq, Debug)]
pub enum TextType {
    /// Text inside a `<plaintext>` element.
    PlainText,
    /// Text inside `<title>` and `<textarea>` elements.
    RCData,
    /// Text inside `<style>`, `<xmp>`, `<iframe>`, `<noembed>`, `<noframes>` and
    /// `<noscript>` elements.
    RawText,
    /// Text inside a `<script>` element.
    ScriptData,
    /// Regular text.
    Data,
    /// Text inside a [CDATA section].
    ///
    /// [CDATA section]: https://developer.mozilla.org/en-US/docs/Web/API/CDATASection
    CDataSection,
}

impl TextType {
    #[inline]
    pub fn allows_html_entities(self) -> bool {
        self == TextType::Data || self == TextType::RCData
    }
}

cfg_if! {
    if #[cfg(feature = "integration_test")] {
        impl TextType {
            pub fn should_replace_unsafe_null_in_text(self) -> bool {
                self != TextType::Data && self != TextType::CDataSection
            }
        }

        impl<'s> From<&'s str> for TextType {
            fn from(text_type: &'s str) -> Self {
                match text_type {
                    "Data state" => TextType::Data,
                    "PLAINTEXT state" => TextType::PlainText,
                    "RCDATA state" => TextType::RCData,
                    "RAWTEXT state" => TextType::RawText,
                    "Script data state" => TextType::ScriptData,
                    "CDATA section state" => TextType::CDataSection,
                    _ => panic!("Unknown text type"),
                }
            }
        }
    }
}
