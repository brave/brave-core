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

#[cfg(test)]
pub(crate) fn assert_proc_macro(
    result: proc_macro2::TokenStream,
    expected: proc_macro2::TokenStream,
) {
    let result = result.to_string();
    let expected = expected.to_string();
    pretty_assertions::assert_eq!(result, expected);
}
