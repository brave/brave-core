use proc_macro2::Span;
use proc_macro_crate::{crate_name, FoundCrate};
use syn::parse::{Parse, ParseStream};
use syn::punctuated::Punctuated;
use syn::Error;

pub fn use_crate(name: &str) -> Result<syn::Ident, Error> {
    match crate_name(name) {
        Ok(FoundCrate::Name(krate)) => Ok(syn::Ident::new(&krate, Span::call_site())),
        Ok(FoundCrate::Itself) => Ok(syn::Ident::new("crate", Span::call_site())),
        Err(err) => Err(Error::new(Span::call_site(), err)),
    }
}

#[derive(Debug)]
pub(crate) struct Attrs<A> {
    // The information is part of the parsed AST, we preserve it even if it isn't used yet.
    #[allow(dead_code)]
    pub ident: syn::Ident,
    // The information is part of the parsed AST, we preserve it even if it isn't used yet.
    #[allow(dead_code)]
    pub paren: syn::token::Paren,
    pub attrs: Punctuated<A, syn::token::Comma>,
}

impl<A: Parse> Parse for Attrs<A> {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        // Maybe check if ident == "mh"
        let ident = input.parse()?;
        let content;
        let paren = syn::parenthesized!(content in input);
        let attrs = content.parse_terminated(A::parse, syn::token::Comma)?;
        Ok(Self {
            ident,
            paren,
            attrs,
        })
    }
}

#[derive(Debug)]
pub(crate) struct Attr<K, V> {
    // The information is part of the parsed AST, we preserve it even if it isn't used yet.
    #[allow(dead_code)]
    pub key: K,
    // The information is part of the parsed AST, we preserve it even if it isn't used yet.
    #[allow(dead_code)]
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
