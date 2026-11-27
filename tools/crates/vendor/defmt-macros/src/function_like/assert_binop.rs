use defmt_parser::Level;
use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use quote::quote;
use syn::{parse_macro_input, punctuated::Punctuated};

use crate::{construct, function_like::log};

use self::args::Args;

mod args;

pub(crate) fn eq(args: TokenStream) -> TokenStream {
    let parsed_args = parse_macro_input!(args as Args);
    expand(parsed_args, BinOp::Eq)
        .unwrap_or_else(syn::Error::into_compile_error)
        .into()
}

pub(crate) fn ne(args: TokenStream) -> TokenStream {
    let parsed_args = parse_macro_input!(args as Args);
    expand(parsed_args, BinOp::Ne)
        .unwrap_or_else(syn::Error::into_compile_error)
        .into()
}

fn expand(args: Args, binop: BinOp) -> syn::Result<TokenStream2> {
    let left = args.left;
    let right = args.right;

    let mut formatting_args = Punctuated::new();

    let extra_string = if let Some(log_args) = args.log_args {
        if let Some(args) = log_args.formatting_args {
            formatting_args.extend(args);
        }
        format!(": {}", log_args.format_string.value())
    } else {
        String::new()
    };

    let vals = match binop {
        BinOp::Eq => &["left_val", "right_val"][..],
        BinOp::Ne => &["left_val"][..],
    };

    for val in vals {
        formatting_args.push(construct::variable(val));
    }

    let panic_msg = match binop {
        BinOp::Eq => format!(
            "panicked at 'assertion failed: `(left == right)`{}'
 left: `{{:?}}`
right: `{{:?}}`",
            extra_string
        ),
        BinOp::Ne => format!(
            "panicked at 'assertion failed: `(left != right)`{}'
left/right: `{{:?}}`",
            extra_string
        ),
    };

    let log_args = log::Args {
        format_string: construct::string_literal(&panic_msg),
        formatting_args: Some(formatting_args),
    };
    let log_stmt = log::expand_parsed(Level::Error, log_args)?;

    let mut cond = quote!(*left_val == *right_val);
    if binop == BinOp::Eq {
        cond = quote!(!(#cond));
    }

    let quoted = quote!(
        // evaluate arguments first
        match (&(#left), &(#right)) {
            (left_val, right_val) => {
                // following `core::assert_eq!`
                if #cond {
                    #log_stmt;
                    defmt::export::panic()
                }
            }
        }
    );
    Ok(quoted)
}

#[derive(PartialEq)]
enum BinOp {
    Eq,
    Ne,
}
