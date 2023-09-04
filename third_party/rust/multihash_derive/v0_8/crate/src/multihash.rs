use std::collections::HashSet;

use crate::utils;
use proc_macro2::{Span, TokenStream};
use quote::quote;
#[cfg(not(test))]
use quote::ToTokens;
use syn::parse::{Parse, ParseStream};
#[cfg(not(test))]
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

impl<'a> From<&'a VariantInfo<'a>> for Hash {
    fn from(bi: &'a VariantInfo<'a>) -> Self {
        let mut code = None;
        let mut hasher = None;
        for attr in bi.ast().attrs {
            let attr: Result<utils::Attrs<MhAttr>, _> = syn::parse2(attr.tokens.clone());
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
        let code = code.unwrap_or_else(|| {
            let msg = "Missing code attribute: e.g. #[mh(code = multihash::SHA3_256)]";
            #[cfg(test)]
            panic!("{}", msg);
            #[cfg(not(test))]
            proc_macro_error::abort!(ident, msg);
        });
        let hasher = hasher.unwrap_or_else(|| {
            let msg = "Missing hasher attribute: e.g. #[mh(hasher = multihash::Sha2_256)]";
            #[cfg(test)]
            panic!("{}", msg);
            #[cfg(not(test))]
            proc_macro_error::abort!(ident, msg);
        });
        Self {
            ident,
            code,
            hasher,
        }
    }
}

/// Parse top-level enum [#mh()] attributes.
///
/// Returns the `alloc_size` and whether errors regarding to `alloc_size` should be reported or not.
fn parse_code_enum_attrs(ast: &syn::DeriveInput) -> syn::LitInt {
    let mut alloc_size = None;

    for attr in &ast.attrs {
        let derive_attrs: Result<utils::Attrs<DeriveAttr>, _> = syn::parse2(attr.tokens.clone());
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
    match alloc_size {
        Some(alloc_size) => alloc_size,
        None => {
            let msg = "enum is missing `alloc_size` attribute: e.g. #[mh(alloc_size = 64)]";
            #[cfg(test)]
            panic!("{}", msg);
            #[cfg(not(test))]
            proc_macro_error::abort!(&ast.ident, msg);
        }
    }
}

/// Return an error if the same code is used several times.
///
/// This only checks for string equality, though this should still catch most errors caused by
/// copy and pasting.
fn error_code_duplicates(hashes: &[Hash]) {
    // Use a temporary store to determine whether a certain value is unique or not
    let mut uniq = HashSet::new();

    hashes.iter().for_each(|hash| {
        let code = &hash.code;
        let msg = format!(
            "the #mh(code) attribute `{}` is defined multiple times",
            quote!(#code)
        );

        // It's a duplicate
        if !uniq.insert(code) {
            #[cfg(test)]
            panic!("{}", msg);
            #[cfg(not(test))]
            {
                let already_defined = uniq.get(code).unwrap();
                let line = already_defined.to_token_stream().span().start().line;
                proc_macro_error::emit_error!(
                    &hash.code, msg;
                    note = "previous definition of `{}` at line {}", quote!(#code), line;
                );
            }
        }
    });
}

/// An error that contains a span in order to produce nice error messages.
#[derive(Debug)]
struct ParseError(Span);

pub fn multihash(s: Structure) -> TokenStream {
    let mh_crate = match utils::use_crate("multihash") {
        Ok(ident) => ident,
        Err(e) => {
            let err = syn::Error::new(Span::call_site(), e).to_compile_error();
            return quote!(#err);
        }
    };
    let code_enum = &s.ast().ident;
    let alloc_size = parse_code_enum_attrs(s.ast());
    let hashes: Vec<_> = s.variants().iter().map(Hash::from).collect();

    error_code_duplicates(&hashes);

    let params = Params {
        code_enum: code_enum.clone(),
    };

    let code_into_u64 = hashes.iter().map(|h| h.code_into_u64(&params));
    let code_from_u64 = hashes.iter().map(|h| h.code_from_u64());
    let code_digest = hashes.iter().map(|h| h.code_digest());

    quote! {
        /// A Multihash with the same allocated size as the Multihashes produces by this derive.
        pub type Multihash = #mh_crate::MultihashGeneric<#alloc_size>;

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
            type Error = #mh_crate::Error;

            fn try_from(code: u64) -> Result<Self, Self::Error> {
                match code {
                    #(#code_from_u64,)*
                    _ => Err(#mh_crate::Error::UnsupportedCode(code))
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_multihash_derive() {
        let input = quote! {
           #[derive(Clone, Multihash)]
           #[mh(alloc_size = 32)]
           pub enum Code {
               #[mh(code = multihash::IDENTITY, hasher = multihash::Identity256)]
               Identity256,
               /// Multihash array for hash function.
               #[mh(code = 0x38b64f, hasher = multihash::Strobe256)]
               Strobe256,
            }
        };
        let expected = quote! {
            /// A Multihash with the same allocated size as the Multihashes produces by this derive.
            pub type Multihash = multihash::MultihashGeneric<32>;

            impl multihash::MultihashDigest<32> for Code {
               fn digest(&self, input: &[u8]) -> Multihash {
                   use multihash::Hasher;
                   match self {
                       Self::Identity256 => {
                           let mut hasher = multihash::Identity256::default();
                           hasher.update(input);
                           Multihash::wrap(multihash::IDENTITY, hasher.finalize()).unwrap()
                       },
                       Self::Strobe256 => {
                           let mut hasher = multihash::Strobe256::default();
                           hasher.update(input);
                           Multihash::wrap(0x38b64f, hasher.finalize()).unwrap()
                       },
                       _ => unreachable!(),
                   }
               }

               fn wrap(&self, digest: &[u8]) -> Result<Multihash, multihash::Error> {
                   Multihash::wrap((*self).into(), digest)
               }
            }

            impl From<Code> for u64 {
                fn from(code: Code) -> Self {
                    match code {
                        Code::Identity256 => multihash::IDENTITY,
                        Code::Strobe256 => 0x38b64f,
                       _ => unreachable!(),
                    }
                }
            }

            impl core::convert::TryFrom<u64> for Code {
                type Error = multihash::Error;

                fn try_from(code: u64) -> Result<Self, Self::Error> {
                    match code {
                        multihash::IDENTITY => Ok(Self::Identity256),
                        0x38b64f => Ok(Self::Strobe256),
                        _ => Err(multihash::Error::UnsupportedCode(code))
                    }
                }
            }
        };
        let derive_input = syn::parse2(input).unwrap();
        let s = Structure::new(&derive_input);
        let result = multihash(s);
        utils::assert_proc_macro(result, expected);
    }

    #[test]
    #[should_panic(
        expected = "the #mh(code) attribute `multihash :: SHA2_256` is defined multiple times"
    )]
    fn test_multihash_error_code_duplicates() {
        let input = quote! {
           #[derive(Clone, Multihash)]
           #[mh(alloc_size = 64)]
           pub enum Multihash {
               #[mh(code = multihash::SHA2_256, hasher = multihash::Sha2_256)]
               Identity256,
               #[mh(code = multihash::SHA2_256, hasher = multihash::Sha2_256)]
               Identity256,
            }
        };
        let derive_input = syn::parse2(input).unwrap();
        let s = Structure::new(&derive_input);
        multihash(s);
    }

    #[test]
    #[should_panic(expected = "the #mh(code) attribute `0x14` is defined multiple times")]
    fn test_multihash_error_code_duplicates_numbers() {
        let input = quote! {
           #[derive(Clone, Multihash)]
           #[mh(alloc_size = 32)]
           pub enum Code {
               #[mh(code = 0x14, hasher = multihash::Sha2_256)]
               Identity256,
               #[mh(code = 0x14, hasher = multihash::Sha2_256)]
               Identity256,
            }
        };
        let derive_input = syn::parse2(input).unwrap();
        let s = Structure::new(&derive_input);
        multihash(s);
    }
}
