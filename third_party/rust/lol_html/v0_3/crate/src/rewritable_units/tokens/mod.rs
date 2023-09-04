mod attributes;
mod capturer;

use super::Mutations;

pub(super) use self::attributes::Attributes;
pub use self::attributes::{Attribute, AttributeNameError};
pub use self::capturer::*;

pub trait Serialize {
    fn to_bytes(&self, output_handler: &mut dyn FnMut(&[u8]));
}

macro_rules! impl_serialize {
    ($Token:ident) => {
        impl crate::rewritable_units::Serialize for $Token<'_> {
            #[inline]
            fn to_bytes(&self, output_handler: &mut dyn FnMut(&[u8])) {
                let Mutations {
                    content_before,
                    replacement,
                    content_after,
                    removed,
                    ..
                } = &self.mutations;

                if !content_before.is_empty() {
                    output_handler(content_before);
                }

                if !removed {
                    match self.raw() {
                        Some(raw) => output_handler(raw),
                        None => self.serialize_from_parts(output_handler),
                    }
                } else if !replacement.is_empty() {
                    output_handler(replacement);
                }

                if !content_after.is_empty() {
                    output_handler(content_after);
                }
            }
        }
    };
}

mod comment;
mod doctype;
mod end_tag;
mod start_tag;
mod text_chunk;

pub use self::comment::{Comment, CommentTextError};
pub use self::doctype::Doctype;
pub use self::end_tag::EndTag;
pub use self::start_tag::StartTag;
pub use self::text_chunk::TextChunk;

#[derive(Debug)]
pub enum Token<'i> {
    TextChunk(TextChunk<'i>),
    Comment(Comment<'i>),
    StartTag(StartTag<'i>),
    EndTag(EndTag<'i>),
    Doctype(Doctype<'i>),
}

impl Serialize for Token<'_> {
    #[inline]
    fn to_bytes(&self, output_handler: &mut dyn FnMut(&[u8])) {
        match self {
            Token::TextChunk(t) => t.to_bytes(output_handler),
            Token::Comment(t) => t.to_bytes(output_handler),
            Token::StartTag(t) => t.to_bytes(output_handler),
            Token::EndTag(t) => t.to_bytes(output_handler),
            Token::Doctype(t) => t.to_bytes(output_handler),
        }
    }
}
