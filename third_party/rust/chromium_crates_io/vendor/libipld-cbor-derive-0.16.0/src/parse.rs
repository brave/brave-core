use crate::ast::*;
use crate::attr::{Attrs, DeriveAttr, FieldAttr};
use quote::quote;
use syn::parse::Parse;
use syn::spanned::Spanned;
use synstructure::{BindingInfo, Structure, VariantInfo};

pub fn parse(s: &Structure) -> SchemaType {
    match &s.ast().data {
        syn::Data::Struct(_) => SchemaType::Struct(parse_struct(
            &s.variants()[0],
            Some(s.ast().generics.clone()),
        )),
        syn::Data::Enum(_) => SchemaType::Union(parse_union(s)),
        syn::Data::Union(_) => unimplemented!(),
    }
}

fn parse_attrs<T: Parse>(ast: &[syn::Attribute]) -> Vec<T> {
    let mut derive_attrs = Vec::with_capacity(ast.len());
    for attr in ast {
        let attrs: Result<Attrs<T>, _> = syn::parse2(attr.tokens.clone());
        if let Ok(attrs) = attrs {
            for attr in attrs.attrs {
                derive_attrs.push(attr);
            }
        }
    }
    derive_attrs
}

fn parse_struct_repr(ast: &[syn::Attribute]) -> Option<StructRepr> {
    let attrs = parse_attrs::<DeriveAttr>(ast);
    let mut repr = None;
    for DeriveAttr::Repr(attr) in attrs {
        repr = Some(match attr.value.value().as_str() {
            "map" => StructRepr::Map,
            "tuple" => StructRepr::Tuple,
            "value" => StructRepr::Value,
            "null" => StructRepr::Null,
            repr => panic!("unknown struct representation {}", repr),
        })
    }
    repr
}

fn parse_union_repr(ast: &[syn::Attribute]) -> UnionRepr {
    let attrs = parse_attrs::<DeriveAttr>(ast);
    let mut repr = None;
    for DeriveAttr::Repr(attr) in attrs {
        repr = Some(match attr.value.value().as_str() {
            "keyed" => UnionRepr::Keyed,
            "kinded" => UnionRepr::Kinded,
            "string" => UnionRepr::String,
            "int" => UnionRepr::Int,
            "int-tuple" => UnionRepr::IntTuple,
            repr => panic!("unknown enum representation {}", repr),
        })
    }
    repr.unwrap_or(UnionRepr::Keyed)
}

fn parse_struct(v: &VariantInfo, generics: Option<syn::Generics>) -> Struct {
    let repr = parse_struct_repr(v.ast().attrs);
    let mut fields: Vec<_> = v
        .bindings()
        .iter()
        .enumerate()
        .map(|(i, binding)| parse_field(i, binding))
        .collect();
    let repr = repr.unwrap_or_else(|| match &v.ast().fields {
        syn::Fields::Named(_) => StructRepr::Map,
        syn::Fields::Unnamed(_) => StructRepr::Tuple,
        syn::Fields::Unit => StructRepr::Null,
    });
    if repr == StructRepr::Map {
        fields.sort_by(|f1, f2| match (&f1.name, &f2.name) {
            (syn::Member::Named(ident1), syn::Member::Named(ident2)) => {
                ident1.to_string().cmp(&ident2.to_string())
            }
            (syn::Member::Unnamed(index1), syn::Member::Unnamed(index2)) => {
                index1.index.cmp(&index2.index)
            }
            _ => unreachable!(),
        });
    }
    Struct {
        name: v.ast().ident.clone(),
        generics,
        rename: None,
        fields,
        repr,
        pat: TokenStreamEq(v.pat()),
        construct: TokenStreamEq(v.construct(|_, i| {
            let binding = &v.bindings()[i];
            quote!(#binding)
        })),
    }
}

fn parse_union(s: &Structure) -> Union {
    let repr = parse_union_repr(&s.ast().attrs);
    Union {
        name: s.ast().ident.clone(),
        generics: s.ast().generics.clone(),
        variants: s
            .variants()
            .iter()
            .map(|v| {
                let mut s = parse_struct(v, None);
                for attr in parse_attrs::<FieldAttr>(v.ast().attrs) {
                    if let FieldAttr::Rename(attr) = attr {
                        s.rename = Some(attr.value.value());
                    }
                }
                s
            })
            .collect(),
        repr,
    }
}

fn parse_field(i: usize, b: &BindingInfo) -> StructField {
    let mut field = StructField {
        name: match b.ast().ident.as_ref() {
            Some(ident) => syn::Member::Named(ident.clone()),
            None => syn::Member::Unnamed(syn::Index {
                index: i as _,
                span: b.ast().ty.span(),
            }),
        },
        rename: None,
        default: None,
        binding: b.binding.clone(),
    };
    for attr in parse_attrs::<FieldAttr>(&b.ast().attrs) {
        match attr {
            FieldAttr::Rename(attr) => field.rename = Some(attr.value.value()),
            FieldAttr::Default(attr) => field.default = Some(attr.value),
        }
    }
    field
}

#[cfg(test)]
pub mod tests {
    use super::*;
    use proc_macro2::TokenStream;
    use quote::{format_ident, quote};

    macro_rules! format_index {
        ($i:expr) => {
            syn::Index {
                index: $i as _,
                span: proc_macro2::Span::call_site(),
            }
        };
    }

    pub fn ast(ts: TokenStream) -> SchemaType {
        let d = syn::parse2(ts).unwrap();
        let s = Structure::new(&d);
        parse(&s)
    }

    #[test]
    fn test_struct_repr_map() {
        let ast = ast(quote! {
            #[derive(DagCbor)]
            #[ipld(repr = "map")]
            struct Map {
                #[ipld(rename = "other", default = false)]
                field: bool,
            }
        });

        assert_eq!(
            ast,
            SchemaType::Struct(Struct {
                name: format_ident!("Map"),
                generics: Some(Default::default()),
                rename: None,
                fields: vec![StructField {
                    name: syn::Member::Named(format_ident!("field")),
                    rename: Some("other".to_string()),
                    default: Some(syn::parse2(quote!(false)).unwrap()),
                    binding: format_ident!("__binding_0"),
                }],
                repr: StructRepr::Map,
                pat: TokenStreamEq(quote! { Map { field: ref __binding_0, }}),
                construct: TokenStreamEq(quote! { Map { field: __binding_0, }}),
            })
        );
    }

    #[test]
    fn test_struct_repr_tuple() {
        let ast = ast(quote! {
            #[derive(DagCbor)]
            #[ipld(repr = "tuple")]
            struct Tuple(bool);
        });

        assert_eq!(
            ast,
            SchemaType::Struct(Struct {
                name: format_ident!("Tuple"),
                generics: Some(Default::default()),
                rename: None,
                fields: vec![StructField {
                    name: syn::Member::Unnamed(format_index!(0)),
                    rename: None,
                    default: None,
                    binding: format_ident!("__binding_0"),
                }],
                repr: StructRepr::Tuple,
                pat: TokenStreamEq(quote! { Tuple(ref __binding_0,) }),
                construct: TokenStreamEq(quote! { Tuple(__binding_0,) }),
            })
        );
    }

    #[test]
    fn test_struct_repr_default() {
        let ast = ast(quote! {
            #[derive(DagCbor)]
            struct Map;
        });

        assert_eq!(
            ast,
            SchemaType::Struct(Struct {
                name: format_ident!("Map"),
                generics: Some(Default::default()),
                rename: None,
                fields: Default::default(),
                repr: StructRepr::Null,
                pat: TokenStreamEq(quote!(Map)),
                construct: TokenStreamEq(quote!(Map)),
            })
        );
    }

    #[test]
    fn test_union_repr_default() {
        let ast = ast(quote! {
            #[derive(DagCbor)]
            enum Union {
                #[ipld(rename = "unit")]
                Unit,
                Tuple(bool),
                Struct { value: bool },
            }
        });

        assert_eq!(
            ast,
            SchemaType::Union(Union {
                name: format_ident!("Union"),
                generics: Default::default(),
                variants: vec![
                    Struct {
                        name: format_ident!("Unit"),
                        generics: None,
                        rename: Some("unit".into()),
                        fields: vec![],
                        repr: StructRepr::Null,
                        pat: TokenStreamEq(quote!(Union::Unit)),
                        construct: TokenStreamEq(quote!(Union::Unit)),
                    },
                    Struct {
                        name: format_ident!("Tuple"),
                        generics: None,
                        rename: None,
                        fields: vec![StructField {
                            name: syn::Member::Unnamed(format_index!(0)),
                            rename: None,
                            default: None,
                            binding: format_ident!("__binding_0"),
                        }],
                        repr: StructRepr::Tuple,
                        pat: TokenStreamEq(quote! { Union::Tuple(ref __binding_0,) }),
                        construct: TokenStreamEq(quote! { Union::Tuple(__binding_0,) }),
                    },
                    Struct {
                        name: format_ident!("Struct"),
                        generics: None,
                        rename: None,
                        fields: vec![StructField {
                            name: syn::Member::Named(format_ident!("value")),
                            rename: None,
                            default: None,
                            binding: format_ident!("__binding_0"),
                        }],
                        repr: StructRepr::Map,
                        pat: TokenStreamEq(quote! { Union::Struct { value: ref __binding_0, } }),
                        construct: TokenStreamEq(quote! { Union::Struct { value: __binding_0, } }),
                    }
                ],
                repr: UnionRepr::Keyed,
            })
        );
    }

    #[test]
    fn test_enum_repr_string() {
        let ast = ast(quote! {
            #[derive(DagCbor)]
            #[ipld(repr = "string")]
            enum Enum {
                #[ipld(rename = "test")]
                Variant,
            }
        });

        assert_eq!(
            ast,
            SchemaType::Union(Union {
                name: format_ident!("Enum"),
                generics: Default::default(),
                variants: vec![Struct {
                    name: format_ident!("Variant"),
                    generics: None,
                    rename: Some("test".into()),
                    fields: vec![],
                    repr: StructRepr::Null,
                    pat: TokenStreamEq(quote!(Enum::Variant)),
                    construct: TokenStreamEq(quote!(Enum::Variant)),
                }],
                repr: UnionRepr::String,
            })
        );
    }
}
