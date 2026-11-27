use std::{
    collections::hash_map::DefaultHasher,
    hash::{Hash as _, Hasher as _},
};

use defmt_parser::Warning;
use proc_macro::Span;
use proc_macro2::{Ident as Ident2, Span as Span2, TokenStream as TokenStream2};
use quote::{format_ident, quote, quote_spanned};
use syn::{parse_quote, Expr, Ident, LitStr};

pub(crate) use symbol::mangled as mangled_symbol_name;

mod symbol;

pub(crate) fn crate_local_disambiguator() -> u64 {
    // We want a deterministic, but unique-per-macro-invocation identifier. For that we
    // hash the call site `Span`'s debug representation, which contains a counter that
    // should disambiguate macro invocations within a crate.
    hash(&format!("{:?}", Span::call_site()))
}

pub(crate) fn escaped_expr_string(expr: &Expr) -> String {
    quote!(#expr)
        .to_string()
        .replace('{', "{{")
        .replace('}', "}}")
}

pub(crate) fn interned_string(
    string: &str,
    tag: &str,
    is_log_statement: bool,
    prefix: Option<&str>,
    defmt_path: &syn::Path,
) -> TokenStream2 {
    // NOTE we rely on this variable name when extracting file location information from the DWARF
    // without it we have no other mean to differentiate static variables produced by `info!` vs
    // produced by `intern!` (or `internp`)
    let var_name = if is_log_statement {
        format_ident!("DEFMT_LOG_STATEMENT")
    } else {
        format_ident!("S")
    };

    let var_addr = if cfg!(feature = "unstable-test") {
        quote!({ #defmt_path::export::fetch_add_string_index() })
    } else {
        let var_item = static_variable(&var_name, string, tag, prefix);
        quote!({
            #var_item
            &#var_name as *const u8 as u16
        })
    };

    quote!({
        #defmt_path::export::make_istr(#var_addr)
    })
}

/// Reports `warnings` at `span`: stable proc macros have no diagnostic of their own, so each
/// one rides on a deprecated constant the expansion reads.
pub(crate) fn format_string_warnings(warnings: &[Warning], span: Span2) -> TokenStream2 {
    let statements = warnings.iter().map(|warning| {
        let note = warning.to_string();
        quote_spanned!(span=>
            {
                #[deprecated(note = #note)]
                const DEFMT_FORMAT_STRING: () = ();
                let _ = DEFMT_FORMAT_STRING;
            }
        )
    });

    quote!(#(#statements)*)
}

/// work around restrictions on length and allowed characters imposed by macos linker
/// returns (note the comma character for macos):
///   under macos: ".defmt," + 16 character hex digest of symbol's hash
///   otherwise:   ".defmt." + prefix + symbol
pub(crate) fn linker_section(for_macos: bool, prefix: Option<&str>, symbol: &str) -> String {
    let mut sub_section = if let Some(prefix) = prefix {
        format!(".{prefix}.{symbol}")
    } else {
        format!(".{symbol}")
    };

    if for_macos {
        sub_section = format!(",{:x}", hash(&sub_section));
    }

    format!(".defmt{sub_section}")
}

pub(crate) fn static_variable(
    name: &Ident2,
    data: &str,
    tag: &str,
    prefix: Option<&str>,
) -> TokenStream2 {
    let sym_name = mangled_symbol_name(tag, data);
    let section = linker_section(false, prefix, &sym_name);
    let section_for_macos = linker_section(true, prefix, &sym_name);

    quote!(
        #[cfg_attr(target_os = "macos", link_section = #section_for_macos)]
        #[cfg_attr(not(target_os = "macos"), link_section = #section)]
        #[export_name = #sym_name]
        static #name: u8 = 0;
    )
}

pub(crate) fn string_literal(content: &str) -> LitStr {
    LitStr::new(content, Span2::call_site())
}

pub(crate) fn variable(name: &str) -> Expr {
    let ident = Ident::new(name, Span2::call_site());
    parse_quote!(#ident)
}

fn hash(string: &str) -> u64 {
    let mut hasher = DefaultHasher::new();
    string.hash(&mut hasher);
    hasher.finish()
}
