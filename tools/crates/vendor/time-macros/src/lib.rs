#![allow(
    clippy::missing_const_for_fn,
    clippy::std_instead_of_core,
    clippy::std_instead_of_alloc,
    clippy::alloc_instead_of_core,
    reason = "irrelevant for proc macros"
)]
#![allow(
    clippy::missing_docs_in_private_items,
    missing_docs,
    reason = "may be removed eventually"
)]

#[allow(
    unused_macros,
    reason = "may not be used for all feature flag combinations"
)]
macro_rules! bug {
    () => { compile_error!("provide an error message to help fix a possible bug") };
    ($descr:literal $($rest:tt)?) => {
        unreachable!(concat!("internal error: ", $descr) $($rest)?)
    }
}

#[macro_use]
mod quote;

mod date;
mod datetime;
mod error;
#[cfg(any(feature = "formatting", feature = "parsing"))]
mod format_description;
mod helpers;
mod offset;
#[cfg(all(feature = "serde", any(feature = "formatting", feature = "parsing")))]
mod serde_format_description;
mod time;
mod timestamp;
mod to_tokens;
mod utc_datetime;

#[cfg(any(feature = "formatting", feature = "parsing"))]
use std::iter::Peekable;

#[cfg(all(feature = "serde", any(feature = "formatting", feature = "parsing")))]
use proc_macro::Delimiter;
use proc_macro::TokenStream;
#[cfg(any(feature = "formatting", feature = "parsing"))]
use proc_macro::TokenTree;

use self::error::Error;

macro_rules! impl_macros {
    ($($name:ident)*) => {$(
        #[proc_macro]
        pub fn $name(input: TokenStream) -> TokenStream {
            use crate::to_tokens::ToTokenStream;

            let mut iter = input.into_iter().peekable();
            match $name::parse(&mut iter) {
                Ok(value) => match iter.peek() {
                    Some(tree) => Error::UnexpectedToken { tree: tree.clone() }.to_compile_error(),
                    None => quote_! { const { #S(value.into_token_stream()) } },
                },
                Err(err) => err.to_compile_error(),
            }
        }
    )*};
}

impl_macros![date datetime utc_datetime offset time timestamp];

#[cfg(any(feature = "formatting", feature = "parsing"))]
type PeekableTokenStreamIter = Peekable<proc_macro::token_stream::IntoIter>;

#[cfg(any(feature = "formatting", feature = "parsing"))]
#[derive(Clone, Copy)]
enum FormatDescriptionVersion {
    V1,
    V2,
    V3,
}

#[cfg(any(feature = "formatting", feature = "parsing"))]
impl FormatDescriptionVersion {
    fn is_v1(self) -> bool {
        match self {
            Self::V1 => true,
            Self::V2 | Self::V3 => false,
        }
    }

    fn is_at_most_v2(self) -> bool {
        match self {
            Self::V1 | Self::V2 => true,
            Self::V3 => false,
        }
    }

    fn is_at_least_v2(self) -> bool {
        match self {
            Self::V1 => false,
            Self::V2 | Self::V3 => true,
        }
    }

    fn is_at_least_v3(self) -> bool {
        match self {
            Self::V1 | Self::V2 => false,
            Self::V3 => true,
        }
    }
}

#[cfg(any(feature = "formatting", feature = "parsing"))]
fn parse_format_description_version<const NO_EQUALS_IS_MOD_NAME: bool>(
    iter: &mut PeekableTokenStreamIter,
) -> Result<Option<FormatDescriptionVersion>, Error> {
    let end_of_input_err = || {
        if NO_EQUALS_IS_MOD_NAME {
            Error::UnexpectedEndOfInput
        } else {
            Error::ExpectedString {
                span_start: None,
                span_end: None,
            }
        }
    };
    let version_ident = match iter.peek().ok_or_else(end_of_input_err)? {
        version @ TokenTree::Ident(ident) if ident.to_string() == "version" => {
            let version_ident = version.clone();
            iter.next(); // consume `version`
            version_ident
        }
        _ => return Ok(None),
    };

    match iter.peek() {
        Some(TokenTree::Punct(punct)) if punct.as_char() == '=' => iter.next(),
        _ if NO_EQUALS_IS_MOD_NAME => {
            // Push the `version` ident to the front of the iterator.
            *iter = std::iter::once(version_ident)
                .chain(iter.clone())
                .collect::<TokenStream>()
                .into_iter()
                .peekable();
            return Ok(None);
        }
        Some(token) => {
            return Err(Error::Custom {
                message: "expected `=`".into(),
                span_start: Some(token.span()),
                span_end: Some(token.span()),
            });
        }
        None => {
            return Err(Error::Custom {
                message: "expected `=`".into(),
                span_start: None,
                span_end: None,
            });
        }
    };
    let version_literal = match iter.next() {
        Some(TokenTree::Literal(literal)) => literal,
        Some(token) => {
            return Err(Error::Custom {
                message: "expected 1, 2, or 3".into(),
                span_start: Some(token.span()),
                span_end: Some(token.span()),
            });
        }
        None => {
            return Err(Error::Custom {
                message: "expected 1, 2, or 3".into(),
                span_start: None,
                span_end: None,
            });
        }
    };
    let version = match version_literal.to_string().as_str() {
        "1" => FormatDescriptionVersion::V1,
        "2" => FormatDescriptionVersion::V2,
        "3" => FormatDescriptionVersion::V3,
        _ => {
            return Err(Error::Custom {
                message: "invalid format description version".into(),
                span_start: Some(version_literal.span()),
                span_end: Some(version_literal.span()),
            });
        }
    };
    helpers::consume_punct(',', iter)?;

    Ok(Some(version))
}

#[cfg(all(feature = "serde", any(feature = "formatting", feature = "parsing")))]
fn parse_visibility(iter: &mut PeekableTokenStreamIter) -> Result<TokenStream, Error> {
    let mut visibility = match iter.peek().ok_or(Error::UnexpectedEndOfInput)? {
        pub_ident @ TokenTree::Ident(ident) if ident.to_string() == "pub" => {
            let visibility = quote_! { #(pub_ident.clone()) };
            iter.next(); // consume `pub`
            visibility
        }
        _ => return Ok(quote_! {}),
    };

    match iter.peek().ok_or(Error::UnexpectedEndOfInput)? {
        group @ TokenTree::Group(path) if path.delimiter() == Delimiter::Parenthesis => {
            visibility.extend(std::iter::once(group.clone()));
            iter.next(); // consume parentheses and path
        }
        _ => {}
    }

    Ok(visibility)
}

#[cfg(any(feature = "formatting", feature = "parsing"))]
fn expand_format_description(item: format_description::public::OwnedFormatItem) -> TokenStream {
    match item.version {
        FormatDescriptionVersion::V1 | FormatDescriptionVersion::V2 => match &item.inner {
            format_description::public::OwnedFormatItemInner::Compound(items) => {
                let items = items
                    .iter()
                    .map(|inner_item| {
                        quote_! {
                            #S(format_description::public::OwnedFormatItem {
                                version: item.version,
                                inner: inner_item.clone(),
                            }),
                        }
                    })
                    .collect::<TokenStream>();
                quote_! {
                    const {
                        use ::time::format_description::{*, modifier::*};
                        &[#S(items)] as StaticFormatDescription
                    }
                }
            }
            _ => quote_! {
                const {
                    use ::time::format_description::{*, modifier::*};
                    &[#S(item)] as StaticFormatDescription
                }
            },
        },
        FormatDescriptionVersion::V3 => quote_! {
            const {
                use ::time::format_description::__private::*;
                use ::time::format_description::modifier::*;
                #S(item).into_opaque()
            }
        },
    }
}

#[cfg(any(feature = "formatting", feature = "parsing"))]
#[proc_macro]
pub fn format_description(input: TokenStream) -> TokenStream {
    (|| {
        let mut input = input.into_iter().peekable();
        let version = parse_format_description_version::<false>(&mut input)?
            .unwrap_or(FormatDescriptionVersion::V1);
        let (span, string) = helpers::get_string_literal(version.is_at_most_v2(), input)?;
        let items = format_description::parse_with_version(version, &string, span)?;

        Ok(expand_format_description(items))
    })()
    .unwrap_or_else(|err: Error| err.to_compile_error())
}

#[cfg(all(feature = "serde", any(feature = "formatting", feature = "parsing")))]
#[proc_macro]
pub fn serde_format_description(input: TokenStream) -> TokenStream {
    (|| {
        let mut tokens = input.into_iter().peekable();

        // First, the optional format description version.
        let maybe_version = parse_format_description_version::<true>(&mut tokens)?;

        // Then, the visibility of the module.
        let visibility = parse_visibility(&mut tokens)?;

        // If no explicit version was provided, allow `mod` (possibly after visibility) to imply v3
        // syntax: `mod foo [Type] = ...`. Otherwise fall back to v1 for backwards compatibility.
        let version = if let Some(version) = maybe_version {
            version
        } else if maybe_version.is_none()
            && let Some(TokenTree::Ident(ident)) = tokens.peek()
            && ident.to_string() == "mod"
        {
            FormatDescriptionVersion::V3
        } else {
            FormatDescriptionVersion::V1
        };

        let (mod_name, ty) = match version {
            FormatDescriptionVersion::V1 | FormatDescriptionVersion::V2 => {
                // Next, an identifier (the desired module name)
                let mod_name = match tokens.next() {
                    Some(TokenTree::Ident(ident)) => Ok(ident),
                    Some(tree) => Err(Error::UnexpectedToken { tree }),
                    None => Err(Error::UnexpectedEndOfInput),
                }?;

                // Followed by a comma
                helpers::consume_punct(',', &mut tokens)?;

                // Then, the type to create serde serializers for (e.g., `OffsetDateTime`).
                let ty = match tokens.next() {
                    Some(tree @ TokenTree::Ident(_)) => Ok(tree.into()),
                    Some(tree) => Err(Error::UnexpectedToken { tree }),
                    None => Err(Error::UnexpectedEndOfInput),
                }?;

                // Another comma
                helpers::consume_punct(',', &mut tokens)?;

                (mod_name, ty)
            }
            FormatDescriptionVersion::V3 => {
                // Next, the `mod` token.
                match tokens.next() {
                    Some(TokenTree::Ident(ident)) if ident.to_string() == "mod" => {}
                    Some(tree) => {
                        return Err(Error::Custom {
                            message: "expected `mod`".into(),
                            span_start: Some(tree.span().start()),
                            span_end: Some(tree.span().end()),
                        });
                    }
                    None => return Err(Error::UnexpectedEndOfInput),
                };

                // Next, an identifier (the desired module name)
                let mod_name = match tokens.next() {
                    Some(TokenTree::Ident(ident)) => Ok(ident),
                    Some(tree) => Err(Error::UnexpectedToken { tree }),
                    None => Err(Error::UnexpectedEndOfInput),
                }?;

                // Followed by the type to create implementations for, placed in brackets.
                let ty = match tokens.next() {
                    Some(TokenTree::Group(group)) if group.delimiter() == Delimiter::Bracket => {
                        Ok(group.stream())
                    }
                    Some(tree) => Err(Error::UnexpectedToken { tree }),
                    None => Err(Error::UnexpectedEndOfInput),
                }?;

                // Then an equals sign
                helpers::consume_punct('=', &mut tokens)?;

                (mod_name, ty)
            }
        };

        // We now have two options. The user can either provide a format description as a
        // string or they can provide a path to a format description. If the
        // latter, all remaining tokens are assumed to be part of the path.
        let (format, format_description_display) = match tokens.peek() {
            // string literal
            Some(TokenTree::Literal(_)) => {
                let (span, format_string) =
                    helpers::get_string_literal(version.is_at_most_v2(), tokens)?;
                let items = format_description::parse_with_version(version, &format_string, span)?;
                let items = expand_format_description(items);

                (items, String::from_utf8_lossy(&format_string).into_owned())
            }
            // path
            Some(_) => {
                let tokens = tokens.collect::<TokenStream>();
                let tokens_string = tokens.to_string();
                (tokens, tokens_string)
            }
            None => return Err(Error::UnexpectedEndOfInput),
        };

        Ok(serde_format_description::build(
            version,
            visibility,
            mod_name,
            ty,
            format,
            format_description_display,
        ))
    })()
    .unwrap_or_else(|err: Error| err.to_compile_error_standalone())
}
