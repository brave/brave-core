use crate::base::Bytes;
use encoding_rs::Encoding;

/// The type of inserted content.
pub enum ContentType {
    /// HTML content type. The rewriter will insert the content as is.
    Html,
    /// Text content type. The rewriter will HTML-escape the content before insertion:
    ///     - `<` will be replaced with `&lt;`
    ///     - `>` will be replaced with `&gt;`
    ///     - `&` will be replaced with `&amp;`
    Text,
}

#[inline]
pub(super) fn content_to_bytes(
    content: &str,
    content_type: ContentType,
    encoding: &'static Encoding,
    mut output_handler: &mut dyn FnMut(&[u8]),
) {
    let bytes = Bytes::from_str(content, encoding);

    match content_type {
        ContentType::Html => output_handler(&bytes),
        ContentType::Text => bytes.replace_byte3(
            (b'<', b"&lt;"),
            (b'>', b"&gt;"),
            (b'&', b"&amp;"),
            &mut output_handler,
        ),
    }
}

pub struct Mutations {
    pub content_before: Vec<u8>,
    pub replacement: Vec<u8>,
    pub content_after: Vec<u8>,
    pub removed: bool,
    encoding: &'static Encoding,
}

impl Mutations {
    #[inline]
    pub fn new(encoding: &'static Encoding) -> Self {
        Mutations {
            content_before: Vec::default(),
            replacement: Vec::default(),
            content_after: Vec::default(),
            removed: false,
            encoding,
        }
    }

    #[inline]
    pub fn before(&mut self, content: &str, content_type: ContentType) {
        content_to_bytes(content, content_type, self.encoding, &mut |c| {
            self.content_before.extend_from_slice(c);
        });
    }

    #[inline]
    pub fn after(&mut self, content: &str, content_type: ContentType) {
        let mut pos = 0;

        content_to_bytes(content, content_type, self.encoding, &mut |c| {
            self.content_after.splice(pos..pos, c.iter().cloned());

            pos += c.len();
        });
    }

    #[inline]
    pub fn replace(&mut self, content: &str, content_type: ContentType) {
        let mut replacement = Vec::default();

        content_to_bytes(content, content_type, self.encoding, &mut |c| {
            replacement.extend_from_slice(c);
        });

        self.replacement = replacement;
        self.remove();
    }

    #[inline]
    pub fn remove(&mut self) {
        self.removed = true;
    }

    #[inline]
    pub fn removed(&self) -> bool {
        self.removed
    }
}
