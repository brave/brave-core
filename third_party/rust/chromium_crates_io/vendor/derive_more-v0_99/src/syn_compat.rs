//! This module contains things adapted from syn 1.x
//! to preserve compatibility.

use quote::ToTokens;
use syn::ext::IdentExt as _;
use syn::parse::{Parse, ParseStream, Parser as _};
use syn::punctuated::Punctuated;
use syn::spanned::Spanned;
use syn::token::Paren;
use syn::{
    parenthesized, token, Attribute, Ident, Lit, LitBool, MacroDelimiter, Meta,
    MetaNameValue, Path, PathSegment, Result, Token,
};

pub(crate) trait AttributeExt {
    fn parse_meta(&self) -> Result<ParsedMeta>;
}

impl AttributeExt for Attribute {
    fn parse_meta(&self) -> Result<ParsedMeta> {
        parse_nested_meta(self.meta.clone())
    }
}

/// [`Meta`] but more like the version from syn 1.x in two important ways:
/// * The nested metas in a list are already parsed
/// * Paths are allowed to contain keywords.
#[derive(Clone)]
pub(crate) enum ParsedMeta {
    Path(Path),
    List(ParsedMetaList),
    NameValue(MetaNameValue),
}

impl Parse for ParsedMeta {
    fn parse(input: ParseStream) -> Result<Self> {
        let path = input.call(parse_meta_path)?;
        parse_meta_after_path(path, input)
    }
}

impl ParsedMeta {
    pub(crate) fn path(&self) -> &Path {
        match self {
            ParsedMeta::Path(path) => path,
            ParsedMeta::List(meta) => &meta.path,
            ParsedMeta::NameValue(meta) => &meta.path,
        }
    }
}

impl ToTokens for ParsedMeta {
    fn to_tokens(&self, tokens: &mut proc_macro2::TokenStream) {
        match self {
            ParsedMeta::Path(p) => p.to_tokens(tokens),
            ParsedMeta::List(l) => l.to_tokens(tokens),
            ParsedMeta::NameValue(n) => n.to_tokens(tokens),
        }
    }
}

#[derive(Clone)]
pub(crate) struct ParsedMetaList {
    pub path: Path,
    pub paren_token: Paren,
    pub nested: Punctuated<NestedMeta, Token![,]>,
}

impl ToTokens for ParsedMetaList {
    fn to_tokens(&self, tokens: &mut proc_macro2::TokenStream) {
        self.path.to_tokens(tokens);
        self.paren_token.surround(tokens, |tokens| {
            self.nested.to_tokens(tokens);
        });
    }
}

#[derive(Clone)]
pub(crate) enum NestedMeta {
    Meta(ParsedMeta),
    Lit(Lit),
}

impl Parse for NestedMeta {
    fn parse(input: ParseStream) -> Result<Self> {
        if input.peek(Lit) && !(input.peek(LitBool) && input.peek2(Token![=])) {
            input.parse().map(NestedMeta::Lit)
        } else if input.peek(Ident::peek_any)
            || input.peek(Token![::]) && input.peek3(Ident::peek_any)
        {
            input.parse().map(NestedMeta::Meta)
        } else {
            Err(input.error("expected identifier or literal"))
        }
    }
}

impl ToTokens for NestedMeta {
    fn to_tokens(&self, tokens: &mut proc_macro2::TokenStream) {
        match self {
            NestedMeta::Meta(meta) => meta.to_tokens(tokens),
            NestedMeta::Lit(lit) => lit.to_tokens(tokens),
        }
    }
}

fn parse_nested_meta(meta: Meta) -> Result<ParsedMeta> {
    match meta {
        Meta::Path(path) => Ok(ParsedMeta::Path(path)),
        Meta::NameValue(name_value) => Ok(ParsedMeta::NameValue(name_value)),
        Meta::List(list) => {
            let MacroDelimiter::Paren(paren_token) = list.delimiter else {
                return Err(syn::Error::new(
                    list.delimiter.span().span(),
                    "Expected paren",
                ));
            };
            Ok(ParsedMeta::List(ParsedMetaList {
                path: list.path,
                paren_token,
                nested: Punctuated::parse_terminated.parse2(list.tokens)?,
            }))
        }
    }
}

// Like Path::parse_mod_style but accepts keywords in the path.
fn parse_meta_path(input: ParseStream) -> Result<Path> {
    Ok(Path {
        leading_colon: input.parse()?,
        segments: {
            let mut segments = Punctuated::new();
            while input.peek(Ident::peek_any) {
                let ident = Ident::parse_any(input)?;
                segments.push_value(PathSegment::from(ident));
                if !input.peek(Token![::]) {
                    break;
                }
                let punct = input.parse()?;
                segments.push_punct(punct);
            }
            if segments.is_empty() {
                return Err(input.error("expected path"));
            } else if segments.trailing_punct() {
                return Err(input.error("expected path segment"));
            }
            segments
        },
    })
}

pub(crate) fn parse_meta_after_path(
    path: Path,
    input: ParseStream,
) -> Result<ParsedMeta> {
    if input.peek(token::Paren) {
        parse_meta_list_after_path(path, input).map(ParsedMeta::List)
    } else if input.peek(Token![=]) {
        parse_meta_name_value_after_path(path, input).map(ParsedMeta::NameValue)
    } else {
        Ok(ParsedMeta::Path(path))
    }
}

fn parse_meta_list_after_path(
    path: Path,
    input: ParseStream,
) -> Result<ParsedMetaList> {
    let content;
    Ok(ParsedMetaList {
        path,
        paren_token: parenthesized!(content in input),
        nested: content.parse_terminated(NestedMeta::parse, Token![,])?,
    })
}

fn parse_meta_name_value_after_path(
    path: Path,
    input: ParseStream,
) -> Result<MetaNameValue> {
    Ok(MetaNameValue {
        path,
        eq_token: input.parse()?,
        value: input.parse()?,
    })
}
