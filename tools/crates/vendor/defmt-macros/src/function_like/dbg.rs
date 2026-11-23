use proc_macro::TokenStream;
use quote::quote;
use syn::parse_macro_input;

use crate::construct;

use self::args::Args;

mod args;

pub(crate) fn expand(args: TokenStream) -> TokenStream {
    let args = parse_macro_input!(args as Args);
    codegen(&args)
}

fn codegen(args: &Args) -> TokenStream {
    let tuple_exprs = args
        .exprs
        .iter()
        .map(|expr| {
            let escaped_expr = construct::escaped_expr_string(expr);
            let format_string = format!("{escaped_expr} = {{}}");

            quote!(match #expr {
                tmp => {
                    defmt::trace!(#format_string, tmp);
                    tmp
                }
            })
        })
        .collect::<Vec<_>>();

    if tuple_exprs.is_empty() {
        // for compatibility with `std::dbg!` we also emit a TRACE log in this case
        quote!(defmt::trace!(""))
    } else {
        quote![ (#(#tuple_exprs),*) ]
    }
    .into()
}
