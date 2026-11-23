use std::borrow::Cow;

use defmt_parser::Level;
use proc_macro::TokenStream;
use quote::quote;
use syn::parse_macro_input;

use crate::{construct, function_like::log};

pub(crate) fn expand(
    args: TokenStream,
    zero_args_format_string: &str,
    transform_format_string: impl FnOnce(&str) -> String,
) -> TokenStream {
    let (format_string, formatting_args) = if args.is_empty() {
        // panic!() -> error!("panicked at 'explicit panic'")
        (Cow::from(zero_args_format_string), None)
    } else {
        // panic!("a", b, c) -> error!("panicked at 'a'", b, c)
        let log_args = parse_macro_input!(args as log::Args);
        let format_string = transform_format_string(&log_args.format_string.value());

        (Cow::from(format_string), log_args.formatting_args)
    };

    let format_string = construct::string_literal(&format_string);
    let log_stmt = match log::expand_parsed(
        Level::Error,
        log::Args {
            format_string,
            formatting_args,
        },
    ) {
        Ok(log_stmt) => log_stmt,
        Err(err) => return err.into_compile_error().into(),
    };

    quote!(
        {
            #log_stmt;
            defmt::export::panic()
        }
    )
    .into()
}
