use std::cmp::Ordering;

use crate::ast::*;
use proc_macro2::TokenStream;
use quote::quote;

pub fn gen_encode(ast: &SchemaType, libipld: &syn::Ident) -> TokenStream {
    let (ident, generics, body) = match ast {
        SchemaType::Struct(s) => (&s.name, s.generics.as_ref().unwrap(), gen_encode_struct(s)),
        SchemaType::Union(u) => (&u.name, &u.generics, gen_encode_union(u)),
    };
    let (impl_generics, ty_generics, where_clause) = generics.split_for_impl();
    let trait_name = quote!(#libipld::codec::Encode<#libipld::cbor::DagCborCodec>);

    quote! {
        impl #impl_generics #trait_name for #ident #ty_generics #where_clause {
            fn encode<W: std::io::Write>(
                &self,
                c: #libipld::cbor::DagCborCodec,
                w: &mut W,
            ) -> #libipld::Result<()> {
                use #libipld::codec::Encode;
                use #libipld::cbor::cbor::MajorKind;
                use #libipld::cbor::encode::{write_null, write_u8, write_u64};
                #body
            }
        }
    }
}

pub fn gen_decode(ast: &SchemaType, libipld: &syn::Ident) -> TokenStream {
    let (ident, generics, body) = match ast {
        SchemaType::Struct(s) => (&s.name, s.generics.as_ref().unwrap(), gen_decode_struct(s)),
        SchemaType::Union(u) => (&u.name, &u.generics, gen_decode_union(u)),
    };
    let (impl_generics, ty_generics, where_clause) = generics.split_for_impl();
    let trait_name = quote!(#libipld::codec::Decode<#libipld::cbor::DagCborCodec>);

    quote! {
        impl #impl_generics #trait_name for #ident #ty_generics #where_clause {
            fn decode<R: std::io::Read + std::io::Seek>(
                c: #libipld::cbor::DagCborCodec,
                r: &mut R,
            ) -> #libipld::Result<Self> {
                use #libipld::cbor::cbor::{MajorKind, NULL};
                use #libipld::cbor::decode::{read_uint, read_major};
                use #libipld::cbor::error::{LengthOutOfRange, MissingKey, UnexpectedCode, UnexpectedKey};
                use #libipld::codec::Decode;
                use #libipld::error::Result;
                use std::io::SeekFrom;
                #body
            }
        }
    }
}

fn rename(name: &syn::Member, rename: Option<&String>) -> TokenStream {
    if let Some(rename) = rename {
        quote!(#rename)
    } else {
        let name = match name {
            syn::Member::Named(ident) => ident.to_string(),
            syn::Member::Unnamed(index) => index.index.to_string(),
        };
        quote!(#name)
    }
}

fn default(binding: &syn::Ident, default: Option<&syn::Expr>, tokens: TokenStream) -> TokenStream {
    if let Some(default) = default {
        quote! {
            if #binding != &#default {
                #tokens
            }
        }
    } else {
        tokens
    }
}

fn gen_encode_match(arms: impl Iterator<Item = TokenStream>) -> TokenStream {
    quote! {
        match *self {
            #(#arms,)*
        }
        Ok(())
    }
}

fn gen_encode_struct(s: &Struct) -> TokenStream {
    let pat = &*s.pat;
    let body = gen_encode_struct_body(s);
    gen_encode_match(std::iter::once(quote!(#pat => { #body })))
}

fn gen_encode_struct_body(s: &Struct) -> TokenStream {
    match s.repr {
        StructRepr::Map => {
            let len = s.fields.len() as u64;
            let dfields = s.fields.iter().filter_map(|field| {
                if let Some(default) = field.default.as_ref() {
                    let binding = &field.binding;
                    Some(quote! {
                        if #binding == &#default {
                            len -= 1;
                        }
                    })
                } else {
                    None
                }
            });
            let mut cbor_order = s
                .fields
                .iter()
                .map(|field| {
                    let key = rename(&field.name, field.rename.as_ref());
                    let binding = &field.binding;
                    let field = default(
                        binding,
                        field.default.as_deref(),
                        quote! {
                            Encode::encode(#key, c, w)?;
                            Encode::encode(#binding, c, w)?;
                        },
                    );
                    (key.to_string(), field)
                })
                .collect::<Vec<(String, _)>>();
            // CBOR RFC-7049 specifies a canonical sort order, where keys are sorted by length
            // first. This was later revised with RFC-8949, but we need to stick to the original
            // order to stay compatible with existing data.
            cbor_order.sort_unstable_by(|(key_a, _), (key_b, _)| {
                match key_a.len().cmp(&key_b.len()) {
                    Ordering::Greater => Ordering::Greater,
                    Ordering::Less => Ordering::Less,
                    Ordering::Equal => key_a.cmp(key_b),
                }
            });
            let fields = cbor_order.iter().map(|(_, field)| field);
            quote! {
                let mut len = #len;
                #(#dfields)*
                write_u64(w, MajorKind::Map, len)?;
                #(#fields)*
            }
        }
        StructRepr::Tuple => {
            let len = s.fields.len() as u64;
            let fields = s.fields.iter().map(|field| {
                let binding = &field.binding;
                quote! {
                    Encode::encode(#binding, c, w)?;
                }
            });
            quote! {
                write_u64(w, MajorKind::Array, #len)?;
                #(#fields)*
            }
        }
        StructRepr::Value => {
            assert_eq!(s.fields.len(), 1);
            let field = &s.fields[0];
            let binding = &field.binding;
            default(
                binding,
                field.default.as_deref(),
                quote! {
                    Encode::encode(#binding, c, w)?;
                },
            )
        }
        StructRepr::Null => {
            assert_eq!(s.fields.len(), 0);
            quote!(write_null(w)?;)
        }
    }
}

#[allow(clippy::needless_collect)]
fn gen_encode_union(u: &Union) -> TokenStream {
    let arms = u
        .variants
        .iter()
        .enumerate()
        .map(|(i, s)| {
            let pat = &*s.pat;
            let key = rename(&syn::Member::Named(s.name.clone()), s.rename.as_ref());
            let value = gen_encode_struct_body(s);
            match u.repr {
                UnionRepr::Keyed => {
                    quote! {
                        #pat => {
                            write_u8(w, MajorKind::Map, 1)?;
                            Encode::encode(#key, c, w)?;
                            #value
                        }
                    }
                }
                UnionRepr::Kinded => {
                    quote!(#pat => { #value })
                }
                UnionRepr::String => {
                    assert_eq!(s.repr, StructRepr::Null);
                    quote!(#pat => Encode::encode(#key, c, w)?)
                }
                UnionRepr::Int => {
                    assert_eq!(s.repr, StructRepr::Null);
                    quote!()
                }
                UnionRepr::IntTuple => {
                    quote! {
                        #pat => {
                            write_u8(w, MajorKind::Array, 2)?;
                            write_u64(w, MajorKind::UnsignedInt, #i as u64)?;
                            #value
                        }
                    }
                }
            }
        })
        .collect::<Vec<_>>();
    if u.repr == UnionRepr::Int {
        quote!(Encode::encode(&(*self as u64), c, w))
    } else {
        gen_encode_match(arms.into_iter())
    }
}

fn gen_decode_struct(s: &Struct) -> TokenStream {
    let len = s.fields.len() as u64;
    let construct = &*s.construct;
    match s.repr {
        StructRepr::Map => {
            let binding: Vec<_> = s.fields.iter().map(|field| &field.binding).collect();
            let key: Vec<_> = s
                .fields
                .iter()
                .map(|field| rename(&field.name, field.rename.as_ref()))
                .collect();
            let fields: Vec<_> = s
                .fields
                .iter()
                .map(|field| {
                    let binding = &field.binding;
                    let key = rename(&field.name, field.rename.as_ref());
                    if let Some(default) = field.default.as_ref() {
                        quote!(let #binding = #binding.unwrap_or(#default);)
                    } else {
                        quote!(let #binding = #binding.ok_or(MissingKey::new::<Self>(#key))?;)
                    }
                })
                .collect();
            quote! {
                let major = read_major(r)?;
                match major.kind() {
                    MajorKind::Map => {
                        let len = read_uint(r, major)?;
                        if len > #len {
                            return Err(LengthOutOfRange::new::<Self>().into());
                        }
                        #(let mut #binding = None;)*
                        for _ in 0..len {
                            let mut key: String = Decode::decode(c, r)?;
                            match key.as_str() {
                                #(#key => { #binding = Some(Decode::decode(c, r)?); })*
                                _ => {
                                    Decode::decode(c, r)?;
                                }
                            }
                        }

                        #(#fields)*

                        return Ok(#construct);
                    }
                    _ => {
                        return Err(UnexpectedCode::new::<Self>(major.into()).into());
                    }
                }
            }
        }
        StructRepr::Tuple => {
            let fields = s.fields.iter().map(|field| {
                let binding = &field.binding;
                quote! {
                    let #binding = Decode::decode(c, r)?;
                }
            });
            quote! {
                let major = read_major(r)?;
                match major.kind() {
                    MajorKind::Array => {
                        let len = read_uint(r, major)?;
                        if len != #len {
                            return Err(LengthOutOfRange::new::<Self>().into());
                        }
                        #(#fields)*
                        return Ok(#construct);
                    }
                    _ => {
                        return Err(UnexpectedCode::new::<Self>(major.into()).into());
                    }
                }
            }
        }
        StructRepr::Value => {
            assert_eq!(s.fields.len(), 1);
            let binding = &s.fields[0].binding;
            quote! {
                let #binding = Decode::decode(c, r)?;
                return Ok(#construct);
            }
        }
        StructRepr::Null => {
            assert_eq!(s.fields.len(), 0);
            quote! {
                let major = read_major(r)?;
                match major {
                    NULL => {
                        return Ok(#construct);
                    }
                    _ => {
                        return Err(UnexpectedCode::new::<Self>(major.into()).into());
                    }
                }
            }
        }
    }
}

fn gen_decode_union(u: &Union) -> TokenStream {
    match u.repr {
        UnionRepr::Keyed => {
            let variants = u.variants.iter().map(|s| {
                let key = rename(&syn::Member::Named(s.name.clone()), s.rename.as_ref());
                let parse = gen_decode_struct(s);
                quote! {
                    if key.as_str() == #key {
                        #parse
                    }
                }
            });
            quote! {
                let major = read_major(r)?;
                if major.kind() != MajorKind::Map {
                    return Err(UnexpectedCode::new::<Self>(major.into()).into());
                } else if read_uint(r, major)? != 1 {
                    return Err(LengthOutOfRange::new::<Self>().into());
                }
                let key: String = Decode::decode(c, r)?;
                #(#variants;)*
                Err(UnexpectedKey::new::<Self>(key).into())
            }
        }
        UnionRepr::Kinded => {
            // TODO: this is wrong. Kinded should be based on the kind, not "if it decodes".
            let variants = u.variants.iter().map(|s| {
                let parse = gen_decode_struct(s);
                quote! {
                    let pos = r.seek(SeekFrom::Current(0))?;
                    let result: Result<Self> = (|| {
                        #parse
                    })();
                    match result {
                        Ok(res) => return Ok(res),
                        Err(err) => {
                            r.seek(SeekFrom::Start(pos))?;
                        }
                    };
                }
            });
            quote! {
                #(#variants;)*
                Err(UnexpectedCode::new::<Self>(read_major(r)?.into()).into())
            }
        }
        UnionRepr::String => {
            let arms = u.variants.iter().map(|v| {
                let pat = &*v.pat;
                let value = rename(&syn::Member::Named(v.name.clone()), v.rename.as_ref());
                quote!(#value => #pat)
            });
            quote! {
                let key: String = Decode::decode(c, r)?;
                let res = match key.as_str() {
                    #(#arms,)*
                    _ => return Err(UnexpectedKey::new::<Self>(key).into()),
                };
                Ok(res)
            }
        }
        UnionRepr::Int => {
            let arms = u.variants.iter().map(|v| {
                let pat = &*v.pat;
                quote!(x if x == #pat as u64 => #pat)
            });
            quote! {
                let key: u64 = Decode::decode(c, r)?;
                let res = match key {
                    #(#arms,)*
                    _ => return Err(UnexpectedKey::new::<Self>(key.to_string()).into()),
                };
                Ok(res)
            }
        }
        UnionRepr::IntTuple => {
            let variants = u.variants.iter().enumerate().map(|(i, s)| {
                let i = i as u64;
                let parse = gen_decode_struct(s);
                quote!(#i => { #parse })
            });
            quote! {
                let major = read_major(r)?;
                if major.kind() != MajorKind::Array {
                    return Err(UnexpectedCode::new::<Self>(major.into()).into());
                }
                if read_uint(r, major)? != 2 {
                    return Err(LengthOutOfRange::new::<Self>().into());
                }
                let ty: u64 = Decode::decode(c, r)?;
                match ty {
                    #(#variants,)*
                    _ => return Err(UnexpectedKey::new::<Self>(ty.to_string()).into()),
                }
            }
        }
    }
}
