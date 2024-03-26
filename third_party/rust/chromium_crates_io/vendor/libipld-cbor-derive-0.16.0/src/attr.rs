use syn::parse::{Parse, ParseStream};
use syn::punctuated::Punctuated;

mod kw {
    use syn::custom_keyword;

    custom_keyword!(repr);

    custom_keyword!(rename);
    custom_keyword!(default);
}

#[derive(Debug)]
pub struct Attrs<A> {
    pub paren: syn::token::Paren,
    pub attrs: Punctuated<A, syn::token::Comma>,
}

impl<A: Parse> Parse for Attrs<A> {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let content;
        let paren = syn::parenthesized!(content in input);
        let attrs = content.parse_terminated(A::parse)?;
        Ok(Self { paren, attrs })
    }
}

#[derive(Debug)]
pub struct Attr<K, V> {
    pub key: K,
    pub eq: syn::token::Eq,
    pub value: V,
}

impl<K: Parse, V: Parse> Parse for Attr<K, V> {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        Ok(Self {
            key: input.parse()?,
            eq: input.parse()?,
            value: input.parse()?,
        })
    }
}

#[derive(Debug)]
pub enum DeriveAttr {
    Repr(Attr<kw::repr, syn::LitStr>),
}

impl Parse for DeriveAttr {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        if input.peek(kw::repr) {
            Ok(DeriveAttr::Repr(input.parse()?))
        } else {
            Err(syn::Error::new(input.span(), "unknown attribute"))
        }
    }
}

#[derive(Debug)]
pub enum FieldAttr {
    Rename(Attr<kw::rename, syn::LitStr>),
    Default(Attr<kw::default, Box<syn::Expr>>),
}

impl Parse for FieldAttr {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        if input.peek(kw::rename) {
            Ok(FieldAttr::Rename(input.parse()?))
        } else if input.peek(kw::default) {
            Ok(FieldAttr::Default(input.parse()?))
        } else {
            Err(syn::Error::new(input.span(), "unknown attribute"))
        }
    }
}
