use codegen::DefmtAttr;
use proc_macro::TokenStream;
use proc_macro2::Span;
use quote::quote;
use syn::{
    parse_macro_input, parse_quote, spanned::Spanned, Arm, Data, DeriveInput, Generics, Ident,
    WhereClause,
};

mod codegen;

pub(crate) fn expand(input: TokenStream) -> TokenStream {
    let DeriveInput {
        attrs,
        vis: _,
        ident,
        mut generics,
        data,
    } = parse_macro_input!(input as DeriveInput);

    let DefmtAttr {
        transparent,
        defmt_path,
        where_clause: custom_where_clause,
    } = match DefmtAttr::from_attrs(&attrs) {
        Ok(maybe_attr) => maybe_attr,
        Err(err) => return err.into_compile_error().into(),
    };

    if transparent {
        return match expand_transparent(ident, data, generics, defmt_path, custom_where_clause) {
            Ok(attr) => attr,
            Err(err) => err.into_compile_error().into(),
        };
    }

    let encode_data = match &data {
        Data::Enum(data) => codegen::encode_enum_data(&ident, data, &defmt_path),
        Data::Struct(data) => codegen::encode_struct_data(&ident, data, &defmt_path),
        Data::Union(_) => {
            return syn::Error::new(
                Span::call_site(),
                "`#[derive(Format)]` does not support unions",
            )
            .into_compile_error()
            .into()
        }
    };

    let codegen::EncodeData {
        format_tag,
        stmts,
        where_predicates,
    } = match encode_data {
        Ok(data) => data,
        Err(e) => return e.into_compile_error().into(),
    };

    let codegen::Generics {
        impl_generics,
        type_generics,
        where_clause,
    } = codegen::Generics::codegen(&mut generics, where_predicates);

    let where_clause = custom_where_clause.unwrap_or(where_clause);
    quote!(
        #[automatically_derived]
        impl #impl_generics #defmt_path::Format for #ident #type_generics #where_clause {
            fn format(&self, f: #defmt_path::Formatter) {
                use #defmt_path as defmt;
                #defmt_path::unreachable!()
            }

            fn _format_tag() -> #defmt_path::Str {
                #format_tag
            }

            fn _format_data(&self) {
                #(#stmts)*
            }
        }
    )
    .into()
}

pub(crate) fn expand_transparent(
    ident: Ident,
    data: Data,
    mut generics: Generics,
    defmt_path: syn::Path,
    custom_where_clause: Option<WhereClause>,
) -> syn::Result<TokenStream> {
    let mut where_clause = generics.make_where_clause().clone();
    let (impl_generics, ty_generics, ..) = generics.split_for_impl();

    let mut member_types: Vec<syn::Type> = vec![];
    let body = match data {
        Data::Enum(data) => {
            let mut match_arms = vec![];
            for v in data.variants {
                let mut fields = v.fields.iter();
                let field = fields.next();
                let one_or_less = fields.next().is_none();
                let Some(field) = field.filter(|_| one_or_less) else {
                    return Err(syn::Error::new(
                        v.fields.span(),
                        format!(
                            "Transparent format can only be applied \
                            when all variants have exactly one field (got {})",
                            v.fields.len(),
                        ),
                    ));
                };

                member_types.push(field.ty.clone());
                let field = field.ident.clone().map_or_else(
                    || {
                        syn::Member::Unnamed(syn::Index {
                            index: 0,
                            span: Span::call_site(),
                        })
                    },
                    syn::Member::Named,
                );
                let variant_name = &v.ident;
                let match_arm: Arm = parse_quote! {
                    Self::#variant_name{ #field: inner } => inner.format(f)
                };
                match_arms.push(match_arm)
            }
            quote! {
                match &self {
                    #( #match_arms, )*
                }
            }
        }
        Data::Struct(data) => {
            if data.fields.len() > 1 {
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!(
                        "Transparent format can only be applied to structs with one field (got {})",
                        data.fields.len()
                    ),
                ));
            }
            let mut fields = data.fields.iter();
            let field = fields.next();
            let Some(field) = field else {
                return Err(syn::Error::new(
                    Span::call_site(),
                    "Transparent format can only be applied to structs with one field (got 0)",
                ));
            };

            member_types.push(field.ty.clone());
            let field = field.ident.clone().map_or_else(
                || {
                    syn::Member::Unnamed(syn::Index {
                        index: 0,
                        span: Span::call_site(),
                    })
                },
                syn::Member::Named,
            );
            quote! {
                self.#field.format(f);
            }
        }
        Data::Union(_) => {
            return Err(syn::Error::new(
                ident.span(),
                "`#[derive(Format)]` does not support unions",
            ))
        }
    };

    let generic_bounds: Vec<syn::WherePredicate> = member_types
        .iter()
        .map(|ty| parse_quote! { #ty: #defmt_path::Format })
        .collect();
    where_clause.predicates.extend(generic_bounds);

    let where_clause = custom_where_clause.unwrap_or(where_clause);
    let quoted = quote! {
        impl #impl_generics #defmt_path::Format for #ident #ty_generics #where_clause {
            fn format(&self, f: #defmt_path::Formatter) {
                #body
            }
        }
    };
    Ok(quoted.into())
}
