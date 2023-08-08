use lol_html::*;

define_group!(
    "Parsing",
    [
        ("Tag scanner", Settings::default()),
        (
            "Lexer",
            // NOTE: this switches parser to the lexer mode and doesn't
            // trigger token production for anything, except doctype. So,
            // we can get relatively fair comparison.
            Settings {
                document_content_handlers: vec![doctype!(noop_handler!())],
                ..Settings::default()
            }
        ),
        (
            "Text rewritable unit parsing and decoding",
            // NOTE: this is the biggest bottleneck part of the parser and rewriter.
            // It's not guaranteed that chunks that come over the wire contain decodable
            // sequence of bytes for the given character encoding. So, if there is a text
            // handler in the selector matching scope, we need to slice and decode all
            // incoming chunks to produce correct text chunk rewritable units.
            Settings {
                document_content_handlers: vec![doc_text!(noop_handler!())],
                ..Settings::default()
            }
        )
    ]
);
