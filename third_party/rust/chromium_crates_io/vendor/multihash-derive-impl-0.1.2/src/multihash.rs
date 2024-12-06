use std::collections::HashSet;
use std::convert::TryFrom;

use crate::utils;
use proc_macro2::{Span, TokenStream};
use quote::{quote, ToTokens};
use syn::parse::{Parse, ParseStream};
use syn::spanned::Spanned;
use synstructure::{Structure, VariantInfo};

mod kw {
    use syn::custom_keyword;

    custom_keyword!(code);
    custom_keyword!(hasher);
    custom_keyword!(mh);
    custom_keyword!(alloc_size);
}

/// Attributes for the enum items.
#[derive(Debug)]
#[allow(clippy::large_enum_variant)]
enum MhAttr {
    Code(utils::Attr<kw::code, syn::Expr>),
    Hasher(utils::Attr<kw::hasher, Box<syn::Type>>),
}

impl Parse for MhAttr {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        if input.peek(kw::code) {
            Ok(MhAttr::Code(input.parse()?))
        } else if input.peek(kw::hasher) {
            Ok(MhAttr::Hasher(input.parse()?))
        } else {
            Err(syn::Error::new(input.span(), "unknown attribute"))
        }
    }
}

/// Attributes of the top-level derive.
#[derive(Debug)]
enum DeriveAttr {
    AllocSize(utils::Attr<kw::alloc_size, syn::LitInt>),
}

impl Parse for DeriveAttr {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        if input.peek(kw::alloc_size) {
            Ok(Self::AllocSize(input.parse()?))
        } else {
            Err(syn::Error::new(input.span(), "unknown attribute"))
        }
    }
}

struct Params {
    code_enum: syn::Ident,
}

#[derive(Debug)]
struct Hash {
    ident: syn::Ident,
    code: syn::Expr,
    hasher: Box<syn::Type>,
}

impl Hash {
    fn code_into_u64(&self, params: &Params) -> TokenStream {
        let ident = &self.ident;
        let code_enum = &params.code_enum;
        let code = &self.code;
        quote!(#code_enum::#ident => #code)
    }

    fn code_from_u64(&self) -> TokenStream {
        let ident = &self.ident;
        let code = &self.code;
        quote!(#code => Ok(Self::#ident))
    }

    fn code_digest(&self) -> TokenStream {
        let ident = &self.ident;
        let hasher = &self.hasher;
        let code = &self.code;
        quote!(Self::#ident => {
            let mut hasher = #hasher::default();
            hasher.update(input);
            Multihash::wrap(#code, hasher.finalize()).unwrap()
        })
    }
}

impl<'a> TryFrom<&'a VariantInfo<'a>> for Hash {
    type Error = syn::Error;
    fn try_from(bi: &'a VariantInfo<'a>) -> Result<Self, syn::Error> {
        let mut code = None;
        let mut hasher = None;
        for attr in bi.ast().attrs {
            let attr: Result<utils::Attrs<MhAttr>, _> = syn::parse2(attr.meta.to_token_stream());
            if let Ok(attr) = attr {
                for attr in attr.attrs {
                    match attr {
                        MhAttr::Code(attr) => code = Some(attr.value),
                        MhAttr::Hasher(attr) => hasher = Some(attr.value),
                    }
                }
            }
        }

        let ident = bi.ast().ident.clone();
        let code = code.ok_or_else(|| -> syn::Error {
            let msg = "Missing code attribute: e.g. #[mh(code = multihash::SHA3_256)]";
            #[cfg(test)]
            panic!("{}", msg);
            #[cfg(not(test))]
            syn::Error::new(bi.ast().ident.span(), msg)
        })?;
        let hasher = hasher.ok_or_else(|| -> syn::Error {
            let msg = "Missing hasher attribute: e.g. #[mh(hasher = multihash::Sha2_256)]";
            #[cfg(test)]
            panic!("{}", msg);
            #[cfg(not(test))]
            syn::Error::new(bi.ast().ident.span(), msg)
        })?;
        Ok(Self {
            ident,
            code,
            hasher,
        })
    }
}

/// Parse top-level enum [#mh()] attributes.
///
/// Returns the `alloc_size` and whether errors regarding to `alloc_size` should be reported or not.
fn parse_code_enum_attrs(ast: &syn::DeriveInput) -> syn::Result<syn::LitInt> {
    let mut alloc_size = None;

    for attr in &ast.attrs {
        let derive_attrs: Result<utils::Attrs<DeriveAttr>, _> =
            syn::parse2(attr.meta.to_token_stream());

        if let Ok(derive_attrs) = derive_attrs {
            for derive_attr in derive_attrs.attrs {
                match derive_attr {
                    DeriveAttr::AllocSize(alloc_size_attr) => {
                        alloc_size = Some(alloc_size_attr.value)
                    }
                }
            }
        }
    }
    alloc_size.ok_or_else(|| -> syn::Error {
        let msg = "enum is missing `alloc_size` attribute: e.g. #[mh(alloc_size = 64)]";
        #[cfg(test)]
        panic!("{}", msg);
        #[cfg(not(test))]
        syn::Error::new(ast.span(), msg)
    })
}

/// Return an error if the same code is used several times.
///
/// This only checks for string equality, though this should still catch most errors caused by
/// copy and pasting.
fn check_error_code_duplicates(hashes: &[Hash]) -> Result<(), syn::Error> {
    // Use a temporary store to determine whether a certain value is unique or not
    let mut uniq = HashSet::new();

    let mut errors = hashes.iter().filter_map(|hash| -> Option<syn::Error> {
        let code = &hash.code;
        // It's a duplicate
        if uniq.insert(code) {
            return None;
        }

        let already_defined = uniq.get(code).unwrap();
        let line = already_defined.to_token_stream().span().start().line;

        let msg = format!(
            "the #mh(code) attribute `{}` is defined multiple times, previous definition at line {}",
            quote!(#code), line
        );

        #[cfg(test)]
        panic!("{}", msg);
        #[cfg(not(test))]
        Some(syn::Error::new(hash.code.span(), msg))
    });
    if let Some(mut error) = errors.next() {
        error.extend(errors);
        Err(error)
    } else {
        Ok(())
    }
}

pub fn multihash(s: Structure) -> TokenStream {
    match multihash_inner(s) {
        Ok(ts) => ts,
        Err(e) => e.to_compile_error(),
    }
}
fn multihash_inner(s: Structure) -> syn::Result<TokenStream> {
    let mh_crate =
        utils::use_crate("multihash-derive").map_err(|e| syn::Error::new(Span::call_site(), e))?;
    let code_enum = &s.ast().ident;
    let alloc_size = parse_code_enum_attrs(s.ast())?;
    let hashes: Vec<_> = s
        .variants()
        .iter()
        .map(Hash::try_from)
        .collect::<Result<_, _>>()?;

    check_error_code_duplicates(&hashes)?;

    let params = Params {
        code_enum: code_enum.clone(),
    };

    let code_into_u64 = hashes.iter().map(|h| h.code_into_u64(&params));
    let code_from_u64 = hashes.iter().map(|h| h.code_from_u64());
    let code_digest = hashes.iter().map(|h| h.code_digest());

    Ok(quote! {
        /// A Multihash with the same allocated size as the Multihashes produces by this derive.
        pub type Multihash = #mh_crate::Multihash<#alloc_size>;

        impl #mh_crate::MultihashDigest<#alloc_size> for #code_enum {
            fn digest(&self, input: &[u8]) -> Multihash {
                use #mh_crate::Hasher;
                match self {
                    #(#code_digest,)*
                    _ => unreachable!(),
                }
            }

            fn wrap(&self, digest: &[u8]) -> Result<Multihash, #mh_crate::Error> {
                Multihash::wrap((*self).into(), digest)
            }
        }

        impl From<#code_enum> for u64 {
            fn from(code: #code_enum) -> Self {
                match code {
                    #(#code_into_u64,)*
                    _ => unreachable!(),
                }
            }
        }

        impl core::convert::TryFrom<u64> for #code_enum {
            type Error = #mh_crate::UnsupportedCode;

            fn try_from(code: u64) -> Result<Self, Self::Error> {
                match code {
                    #(#code_from_u64,)*
                    _ => Err(#mh_crate::UnsupportedCode(code))
                }
            }
        }
    })
}
