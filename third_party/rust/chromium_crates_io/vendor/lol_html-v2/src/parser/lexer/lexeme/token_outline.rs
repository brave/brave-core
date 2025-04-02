use crate::base::{Align, Range};
use crate::html::{LocalNameHash, Namespace, TextType};
use crate::parser::AttributeBuffer;

#[derive(Debug, Default, Copy, Clone)]
pub(crate) struct AttributeOutline {
    pub name: Range,
    pub value: Range,
    pub raw_range: Range,
}

impl Align for AttributeOutline {
    #[inline]
    fn align(&mut self, offset: usize) {
        self.name.align(offset);
        self.value.align(offset);
        self.raw_range.align(offset);
    }
}

#[derive(Debug)]
pub(crate) enum TagTokenOutline {
    StartTag {
        name: Range,
        name_hash: LocalNameHash,
        ns: Namespace,
        attributes: AttributeBuffer,
        self_closing: bool,
    },

    EndTag {
        name: Range,
        name_hash: LocalNameHash,
    },
}

#[derive(Debug)]
pub(crate) enum NonTagContentTokenOutline {
    Text(TextType),
    Comment(Range),

    Doctype {
        name: Option<Range>,
        public_id: Option<Range>,
        system_id: Option<Range>,
        force_quirks: bool,
    },

    Eof,
}

impl Align for TagTokenOutline {
    #[inline]
    fn align(&mut self, offset: usize) {
        match self {
            Self::StartTag {
                name, attributes, ..
            } => {
                name.align(offset);
                attributes.as_mut_slice().align(offset);
            }
            Self::EndTag { name, .. } => name.align(offset),
        }
    }
}

impl Align for NonTagContentTokenOutline {
    #[inline]
    fn align(&mut self, offset: usize) {
        match self {
            Self::Comment(text) => text.align(offset),
            Self::Doctype {
                name,
                public_id,
                system_id,
                ..
            } => {
                name.align(offset);
                public_id.align(offset);
                system_id.align(offset);
            }
            _ => (),
        }
    }
}
