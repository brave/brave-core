use std::fmt::Write as _;

use proc_macro2::TokenStream as TokenStream2;
use quote::{format_ident, quote};
use syn::{parse_quote, Field, Fields, Index, Type, WherePredicate};

use crate::consts;

pub(crate) fn codegen(
    fields: &Fields,
    format_string: &mut String,
    patterns: &mut Vec<TokenStream2>,
    defmt_path: &syn::Path,
) -> syn::Result<(Vec<TokenStream2>, Vec<WherePredicate>)> {
    let (fields, fields_are_named) = match fields {
        Fields::Named(named) => (&named.named, true),
        Fields::Unit => return Ok((vec![], vec![])),
        Fields::Unnamed(unnamed) => (&unnamed.unnamed, false),
    };

    if fields.is_empty() {
        return Ok((vec![], vec![]));
    }

    if fields_are_named {
        format_string.push_str(" {{ ");
    } else {
        format_string.push('(');
    }

    let mut stmts = vec![];
    let mut where_predicates = vec![];
    let mut is_first = true;
    for (index, field) in fields.iter().enumerate() {
        if is_first {
            is_first = false;
        } else {
            format_string.push_str(", ");
        }

        let format_opt = get_defmt_format_option(field)?;
        // Find out if the field type is natively supported by defmt. `ty` will be None if not.
        let ty = as_native_type(&field.ty);
        // `field_ty` will be the field's type if it is not natively supported by defmt
        let field_ty = if ty.is_none() { Some(&field.ty) } else { None };
        // Get the field format specifier. Either the native specifier or '?'.
        let ty = ty.unwrap_or_else(|| consts::TYPE_FORMAT.to_string());
        let ident = field
            .ident
            .clone()
            .unwrap_or_else(|| format_ident!("arg{}", index));
        // Find the required trait bounds for the field and add the formatting statement depending on the field type and the formatting options
        let bound: Option<syn::Path> = if let Some(FormatOption::Debug2Format) = format_opt {
            stmts.push(quote!(#defmt_path::export::fmt(&#defmt_path::Debug2Format(&#ident))));
            field_ty.map(|_| parse_quote!(::core::fmt::Debug))
        } else if let Some(FormatOption::Display2Format) = format_opt {
            stmts.push(quote!(#defmt_path::export::fmt(&#defmt_path::Display2Format(&#ident))));
            field_ty.map(|_| parse_quote!(::core::fmt::Display))
        } else if ty == consts::TYPE_FORMAT {
            stmts.push(quote!(#defmt_path::export::fmt(#ident)));
            field_ty.map(|_| parse_quote!(#defmt_path::Format))
        } else {
            let method = format_ident!("{}", ty);
            stmts.push(quote!(#defmt_path::export::#method(#ident)));
            field_ty.map(|_| parse_quote!(#defmt_path::Format))
        };
        if let Some(bound) = bound {
            where_predicates.push(parse_quote!(#field_ty: #bound));
        }

        if field.ident.is_some() {
            // Named field.
            write!(format_string, "{ident}: {{={ty}:?}}").ok();

            patterns.push(quote!( #ident ));
        } else {
            // Unnamed (tuple) field.
            write!(format_string, "{{={ty}}}").ok();

            let index = Index::from(index);
            patterns.push(quote!( #index: #ident ));
        }
    }

    if fields_are_named {
        format_string.push_str(" }}");
    } else {
        format_string.push(')');
    }

    Ok((stmts, where_predicates))
}

#[derive(Copy, Clone, Eq, PartialEq, Debug)]
enum FormatOption {
    Debug2Format,
    Display2Format,
}

/// If the field has a valid defmt attribute (e.g. `#[defmt(Debug2Format)]`), returns `Ok(Some(FormatOption))`.
/// Returns `Err` if we can't parse a valid defmt attribute.
/// Returns `Ok(None)` if there are no `defmt` attributes on the field.
fn get_defmt_format_option(field: &Field) -> syn::Result<Option<FormatOption>> {
    let mut format_option = None;

    for attr in &field.attrs {
        if attr.path().is_ident("defmt") {
            if format_option.is_some() {
                return Err(syn::Error::new_spanned(
                    field,
                    "multiple `defmt` attributes not supported",
                ));
            }

            let mut parsed_format = None;

            attr.parse_nested_meta(|meta| {
                // #[defmt(Debug2Format)]
                if meta.path.is_ident("Debug2Format") {
                    parsed_format = Some(FormatOption::Debug2Format);
                    return Ok(());
                }

                // #[defmt(Display2Format)]
                if meta.path.is_ident("Display2Format") {
                    parsed_format = Some(FormatOption::Display2Format);
                    return Ok(());
                }

                Err(meta.error("expected `Debug2Format` or `Display2Format`"))
            })?;

            if parsed_format.is_none() {
                return Err(syn::Error::new_spanned(
                    &attr.meta,
                    "expected 1 attribute argument",
                ));
            }

            format_option = parsed_format;
        }
    }

    Ok(format_option)
}

/// Returns `Some` if `ty` refers to a builtin Rust type that has native support from defmt and does
/// not have to go through the `Format` trait.
///
/// This should return `Some` for all types that can be used as `{=TYPE}`.
///
/// Note: This is technically incorrect, since builtin types can be shadowed. However the efficiency
/// gains are too big to pass up, so we expect user code to not do that.
fn as_native_type(ty: &Type) -> Option<String> {
    match ty {
        Type::Path(path) => {
            let ident = path.path.get_ident()?;
            let ty_name = ident.to_string();

            match &*ty_name {
                "u8" | "u16" | "u32" | "u64" | "u128" | "usize" | "i8" | "i16" | "i32" | "i64"
                | "i128" | "isize" | "f32" | "f64" | "bool" | "str" => Some(ty_name),
                _ => None,
            }
        }
        Type::Reference(ty_ref) => as_native_type(&ty_ref.elem),
        _ => None,
    }
}
