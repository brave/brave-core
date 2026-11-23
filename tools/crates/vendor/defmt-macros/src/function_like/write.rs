use defmt_parser::ParserMode;
use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, parse_quote};

use crate::{construct, function_like::log};

use self::args::Args;

mod args;

pub(crate) fn expand(args: TokenStream) -> TokenStream {
    let Args {
        formatter,
        log_args,
        ..
    } = parse_macro_input!(args as Args);

    let format_string = log_args.format_string.value();
    let (fragments, warnings) =
        match defmt_parser::parse_with_warnings(&format_string, ParserMode::Strict) {
            Ok(parsed) => parsed,
            Err(e) => {
                return syn::Error::new(log_args.format_string.span(), format!("{}", e))
                    .into_compile_error()
                    .into()
            }
        };
    let warnings = construct::format_string_warnings(&warnings, log_args.format_string.span());

    let formatting_exprs: Vec<_> = log_args
        .formatting_args
        .map(|punctuated| punctuated.into_iter().collect())
        .unwrap_or_default();

    let log::Codegen { patterns, exprs } = match log::Codegen::new(
        &fragments,
        formatting_exprs.len(),
        log_args.format_string.span(),
    ) {
        Ok(val) => val,
        Err(err) => return err.into_compile_error().into(),
    };

    let format_tag =
        construct::interned_string(&format_string, "write", false, None, &parse_quote!(defmt));
    quote!({
        #warnings
        let _typecheck_formatter: defmt::Formatter<'_> = #formatter;
        match (#(&(#formatting_exprs)),*) {
            (#(#patterns),*) => {
                defmt::export::istr(&#format_tag);
                #(#exprs;)*
            }
        }
    })
    .into()
}
