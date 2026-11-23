use proc_macro2::TokenStream as TokenStream2;
use quote::{quote, ToTokens};
use syn::{
    parse_quote, punctuated::Punctuated, DataStruct, Ident, ImplGenerics, Token, TypeGenerics,
    WhereClause, WherePredicate,
};

pub(crate) use enum_data::encode as encode_enum_data;

use crate::construct;

mod enum_data;
mod fields;

pub(crate) struct EncodeData {
    pub(crate) format_tag: TokenStream2,
    pub(crate) stmts: Vec<TokenStream2>,
    pub(crate) where_predicates: Vec<WherePredicate>,
}

pub(crate) fn encode_struct_data(
    ident: &Ident,
    data: &DataStruct,
    defmt_path: &syn::Path,
) -> syn::Result<EncodeData> {
    let mut format_string = ident.to_string();
    let mut stmts = vec![];
    let mut field_patterns = vec![];

    let (encode_fields_stmts, where_predicates) = fields::codegen(
        &data.fields,
        &mut format_string,
        &mut field_patterns,
        defmt_path,
    )?;

    stmts.push(quote!(match self {
        Self { #(#field_patterns),* } => {
            #(#encode_fields_stmts;)*
        }
    }));

    let format_tag = construct::interned_string(&format_string, "derived", false, None, defmt_path);
    Ok(EncodeData {
        format_tag,
        stmts,
        where_predicates,
    })
}

pub(crate) struct Generics<'a> {
    pub(crate) impl_generics: ImplGenerics<'a>,
    pub(crate) type_generics: TypeGenerics<'a>,
    pub(crate) where_clause: WhereClause,
}

impl<'a> Generics<'a> {
    pub(crate) fn codegen(
        generics: &'a mut syn::Generics,
        where_predicates: Vec<WherePredicate>,
    ) -> Self {
        let mut where_clause = generics.make_where_clause().clone();
        let (impl_generics, type_generics, _) = generics.split_for_impl();

        // Extend where-clause with `Format` bounds for all field types.
        where_clause.predicates.extend(where_predicates);

        Self {
            impl_generics,
            type_generics,
            where_clause,
        }
    }
}

pub(crate) struct DefmtAttr {
    pub(crate) transparent: bool,
    pub(crate) defmt_path: syn::Path,
    pub(crate) where_clause: Option<WhereClause>,
}

impl DefmtAttr {
    pub(crate) fn from_attrs(attrs: &[syn::Attribute]) -> syn::Result<Self> {
        let (transparent, defmt_path, bound) = attrs
            .iter()
            .filter(|attr| attr.path().is_ident("defmt"))
            .try_fold(
                (false, None, None),
                |(mut transparent, mut defmt_path, mut bound), attr| -> syn::Result<_> {
                    let options = attr.meta.require_list()?;
                    options.parse_nested_meta(|meta| {
                        if meta.path.is_ident("transparent") {
                            transparent = true;
                        } else if meta.path.is_ident("crate") {
                            meta.input.parse::<syn::Token![=]>()?;
                            defmt_path = Some(meta.input.parse::<syn::Path>()?);
                        } else if meta.path.is_ident("bound") {
                            let content;
                            let _paren = ::syn::parenthesized!(content in meta.input);
                            bound =
                                Some(Punctuated::<WherePredicate, Token![,]>::parse_terminated(
                                    &content,
                                )?);
                        } else {
                            let path = meta.path.to_token_stream().to_string().replace(' ', "");
                            return Err(
                                meta.error(format_args!("unknown defmt attribute `{path}`"))
                            );
                        }
                        Ok(())
                    })?;

                    Ok((transparent, defmt_path, bound))
                },
            )?;

        let defmt_path = defmt_path.unwrap_or_else(|| parse_quote! { defmt });
        let where_clause = bound.map(|bound| parse_quote! { where #bound });
        Ok(Self {
            transparent,
            defmt_path,
            where_clause,
        })
    }
}
