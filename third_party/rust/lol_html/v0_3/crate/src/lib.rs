//! ***LOL HTML*** is a **L**ow **O**utput **L**atency streaming **HTML** rewriter/parser with
//! CSS-selector based API.
//!
//! It is designed to modify HTML on the fly with minimal buffering. It can quickly handle very large
//! documents, and operate in environments with limited memory resources.
//!
//! The crate serves as a back-end for the HTML rewriting functionality of [Cloudflare Workers], but
//! can be used as a standalone library with the convenient API for a wide variety of HTML
//! rewriting/analyzis tasks.
//!
//! The crate provides two main API entry points:
//!
//! * [`HtmlRewriter`] - a streaming HTML rewriter;
//! * [`rewrite_str`] - one-off HTML string rewriting function.
//!
//! [Cloudflare Workers]: https://www.cloudflare.com/en-gb/products/cloudflare-workers/
//! [`HtmlRewriter`]: struct.HtmlRewriter.html
//! [`rewrite_str`]: fn.rewrite_str.html

#[macro_use]
mod base;

#[macro_use]
mod html;

#[macro_use]
mod rewriter;

mod memory;
mod parser;
mod rewritable_units;
mod transform_stream;

use cfg_if::cfg_if;

pub use self::rewriter::{
    rewrite_str, AsciiCompatibleEncoding, DocumentContentHandlers, ElementContentHandlers,
    HtmlRewriter, MemorySettings, RewriteStrSettings, Settings,
};
pub use self::selectors_vm::Selector;
pub use self::transform_stream::OutputSink;

/// The errors that can be produced by the crate's API.
pub mod errors {
    pub use super::memory::MemoryLimitExceededError;
    pub use super::parser::ParsingAmbiguityError;
    pub use super::rewritable_units::{AttributeNameError, CommentTextError, TagNameError};
    pub use super::rewriter::RewritingError;
    pub use super::selectors_vm::SelectorError;
}

/// HTML content descriptors that can be produced and modified by a rewriter.
pub mod html_content {
    pub use super::rewritable_units::{
        Attribute, Comment, ContentType, Doctype, DocumentEnd, Element, EndTag, EndTagError,
        TextChunk, UserData,
    };

    pub use super::html::TextType;
}

#[cfg(any(test, feature = "integration_test"))]
pub mod test_utils {
    use encoding_rs::*;

    pub static ASCII_COMPATIBLE_ENCODINGS: [&Encoding; 36] = [
        BIG5,
        EUC_JP,
        EUC_KR,
        GB18030,
        GBK,
        IBM866,
        ISO_8859_2,
        ISO_8859_3,
        ISO_8859_4,
        ISO_8859_5,
        ISO_8859_6,
        ISO_8859_7,
        ISO_8859_8,
        ISO_8859_8_I,
        ISO_8859_10,
        ISO_8859_13,
        ISO_8859_14,
        ISO_8859_15,
        ISO_8859_16,
        KOI8_R,
        KOI8_U,
        MACINTOSH,
        SHIFT_JIS,
        UTF_8,
        WINDOWS_874,
        WINDOWS_1250,
        WINDOWS_1251,
        WINDOWS_1252,
        WINDOWS_1253,
        WINDOWS_1254,
        WINDOWS_1255,
        WINDOWS_1256,
        WINDOWS_1257,
        WINDOWS_1258,
        X_MAC_CYRILLIC,
        X_USER_DEFINED,
    ];

    pub static NON_ASCII_COMPATIBLE_ENCODINGS: [&encoding_rs::Encoding; 4] =
        [UTF_16BE, UTF_16LE, ISO_2022_JP, REPLACEMENT];

    pub struct Output {
        bytes: Vec<u8>,
        encoding: &'static Encoding,
        finalizing_chunk_received: bool,
    }

    impl Output {
        pub fn new(encoding: &'static Encoding) -> Self {
            Output {
                bytes: Vec::default(),
                encoding,
                finalizing_chunk_received: false,
            }
        }

        pub fn push(&mut self, chunk: &[u8]) {
            if chunk.is_empty() {
                self.finalizing_chunk_received = true;
            } else {
                assert!(
                    !self.finalizing_chunk_received,
                    "Chunk written to the output after the finalizing chunk."
                );

                self.bytes.extend_from_slice(chunk);
            }
        }
    }

    impl From<Output> for String {
        fn from(output: Output) -> String {
            assert!(
                output.finalizing_chunk_received,
                "Finalizing chunk for the output hasn't been received."
            );

            output
                .encoding
                .decode_without_bom_handling(&output.bytes)
                .0
                .into_owned()
        }
    }
}

cfg_if! {
    if #[cfg(feature = "integration_test")] {
        pub mod selectors_vm;

        pub use self::transform_stream::{
            StartTagHandlingResult, TransformController, TransformStream,
            TransformStreamSettings
        };

        pub use self::rewritable_units::{
            EndTag, Serialize, StartTag, Token, TokenCaptureFlags, Mutations
        };

        pub use self::base::Bytes;
        pub use self::memory::MemoryLimiter;
        pub use self::html::{LocalName, LocalNameHash, Tag, Namespace};
    } else {
        mod selectors_vm;
    }
}
