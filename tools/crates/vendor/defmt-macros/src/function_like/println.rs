use defmt_parser::ParserMode;
use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use quote::quote;
use syn::{parse_macro_input, parse_quote};

use crate::construct;
use crate::function_like::log::{Args, Codegen};

pub(crate) fn expand(args: TokenStream) -> TokenStream {
    let parsed = parse_macro_input!(args as Args);
    expand_parsed(parsed)
        .unwrap_or_else(syn::Error::into_compile_error)
        .into()
}

pub(crate) fn expand_parsed(args: Args) -> syn::Result<TokenStream2> {
    let format_string = args.format_string.value();
    let (fragments, warnings) = match defmt_parser::parse_with_warnings(&format_string, ParserMode::Strict) {
        Ok(parsed) => parsed,
        Err(e @ defmt_parser::Error::UnknownDisplayHint(_)) => return Err(syn::Error::new(
            args.format_string.span(),
            format!(
                "{e}\n  = help: {help}",
                help = "`defmt` uses a slightly different syntax than regular formatting in Rust. See https://defmt.ferrous-systems.com/macros.html for more details."
            )
        )),
        Err(e) => return Err(syn::Error::new(args.format_string.span(), format!("{}", e))), // No extra help
    };
    let warnings = construct::format_string_warnings(&warnings, args.format_string.span());

    let formatting_exprs = args
        .formatting_args
        .map(|punctuated| punctuated.into_iter().collect::<Vec<_>>())
        .unwrap_or_default();

    let Codegen { patterns, exprs } = Codegen::new(
        &fragments,
        formatting_exprs.len(),
        args.format_string.span(),
    )?;

    let header =
        construct::interned_string(&format_string, "println", true, None, &parse_quote!(defmt));
    let content = if exprs.is_empty() {
        quote!(
            defmt::export::acquire_header_and_release(&#header);
        )
    } else {
        quote!(
            // safety: will be released a few lines further down
            unsafe { defmt::export::acquire_and_header(&#header); };
            #(#exprs;)*
            // safety: acquire() was called a few lines above
            unsafe { defmt::export::release() }
        )
    };
    Ok(quote!({
        #warnings
        match (#(&(#formatting_exprs)),*) {
            (#(#patterns),*) => {
                #content
            }
        }
    }))
}
