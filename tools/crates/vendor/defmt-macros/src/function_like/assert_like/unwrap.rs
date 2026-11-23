use defmt_parser::Level;
use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, punctuated::Punctuated};

use crate::{construct, function_like::log};

pub(crate) fn expand(args: TokenStream) -> TokenStream {
    let args = parse_macro_input!(args as super::Args);

    let condition = args.condition;
    let (format_string, formatting_args) = if let Some(log_args) = args.log_args {
        let format_string = format!("panicked at '{}'", log_args.format_string.value());
        (format_string, log_args.formatting_args)
    } else {
        let format_string = format!(
            "panicked at 'unwrap failed: {}'\nerror: `{{:?}}`",
            construct::escaped_expr_string(&condition)
        );

        let mut formatting_args = Punctuated::new();
        formatting_args.push(construct::variable("_unwrap_err"));

        (format_string, Some(formatting_args))
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
        match defmt::export::into_result(#condition) {
            ::core::result::Result::Ok(res) => res,
            ::core::result::Result::Err(_unwrap_err) => {
                #log_stmt;
                defmt::export::panic()
            }
        }
    )
    .into()
}
