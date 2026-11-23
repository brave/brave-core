mod component;
pub(super) mod modifier;
mod optimizing;

use proc_macro::TokenStream;

pub(crate) use self::component::Component;
use crate::FormatDescriptionVersion;
use crate::to_tokens::ToTokenStream;

#[derive(Clone)]
pub(crate) struct OwnedFormatItem {
    pub(crate) version: FormatDescriptionVersion,
    pub(crate) inner: OwnedFormatItemInner,
}

#[derive(Clone)]
pub(crate) enum OwnedFormatItemInner {
    Literal(Vec<u8>),
    StringLiteral(String),
    Component(Component),
    Compound(Vec<Self>),
    Optional { format: bool, item: Box<Self> },
    First(Vec<Self>),
}

impl ToTokenStream for OwnedFormatItem {
    fn append_to(self, ts: &mut TokenStream) {
        match self.version {
            FormatDescriptionVersion::V1 | FormatDescriptionVersion::V2 => match self.inner {
                OwnedFormatItemInner::Literal(bytes) => quote_append! { ts
                    BorrowedFormatItem::Literal(#(Literal::byte_string(bytes.as_ref())))
                },
                OwnedFormatItemInner::StringLiteral(string) => quote_append! { ts
                    BorrowedFormatItem::StringLiteral(#(string.as_ref()))
                },
                OwnedFormatItemInner::Component(component) => quote_append! { ts
                    BorrowedFormatItem::Component { 0: #S(component) }
                },
                OwnedFormatItemInner::Compound(items) => {
                    let items = items
                        .into_iter()
                        .map(|item| {
                            quote_! { #S(Self { version: self.version, inner: item }), }
                        })
                        .collect::<TokenStream>();
                    quote_append! { ts
                        BorrowedFormatItem::Compound { 0: &[#S(items)] }
                    }
                }
                OwnedFormatItemInner::Optional { format, item } => {
                    if !format {
                        bug!("v1 and v2 format descriptions must format optional items")
                    }
                    quote_append! { ts
                        BorrowedFormatItem::Optional {
                            0: &#S(Self { version: self.version, inner: *item })
                        }
                    }
                }
                OwnedFormatItemInner::First(items) => {
                    let items = items
                        .into_iter()
                        .map(|item| {
                            quote_! { #S(Self { version: self.version, inner: item }), }
                        })
                        .collect::<TokenStream>();
                    quote_append! { ts
                        BorrowedFormatItem::First { 0: &[#S(items)] }
                    }
                }
            },
            FormatDescriptionVersion::V3 => match self.inner {
                OwnedFormatItemInner::Literal(_) => {
                    bug!("v3 format descriptions should never have non-UTF8 literals")
                }
                OwnedFormatItemInner::StringLiteral(string) => quote_append! { ts
                    FormatDescriptionV3Inner::BorrowedLiteral(#(string.as_ref()))
                },
                OwnedFormatItemInner::Component(component) => {
                    // v3 format descriptions have components directly on the item, not as a
                    // sub-enum. Swap out the name of the enum that is being constructed.
                    let tokens = component.into_token_stream();
                    let mut iter = tokens.into_iter().peekable();
                    if let Some(first) = iter.peek_mut() {
                        *first = proc_macro::TokenTree::Ident(proc_macro::Ident::new(
                            "FormatDescriptionV3Inner",
                            proc_macro::Span::mixed_site(),
                        ));
                    } else {
                        bug!("component should have at least one token")
                    }
                    ts.extend(iter);
                }
                OwnedFormatItemInner::Compound(items) => {
                    let items = items
                        .into_iter()
                        .map(|item| {
                            quote_! { #S(Self { version: self.version, inner: item }), }
                        })
                        .collect::<TokenStream>();
                    quote_append! { ts
                        FormatDescriptionV3Inner::BorrowedCompound(const { &[#S(items)] })
                    }
                }
                OwnedFormatItemInner::Optional { format, item } => {
                    quote_append! { ts
                        FormatDescriptionV3Inner::BorrowedOptional {
                            format: #S(format),
                            item: &#S(Self { version: self.version, inner: *item })
                        }
                    }
                }
                OwnedFormatItemInner::First(items) => {
                    let items = items
                        .into_iter()
                        .map(|item| {
                            quote_! { #S(Self { version: self.version, inner: item }), }
                        })
                        .collect::<TokenStream>();
                    quote_append! { ts
                        FormatDescriptionV3Inner::BorrowedFirst(const { &[#S(items)] })
                    }
                }
            },
        }
    }
}
