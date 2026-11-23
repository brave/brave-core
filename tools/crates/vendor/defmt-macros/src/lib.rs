//! INTERNAL; DO NOT USE. Please use the `defmt` crate to access the functionality implemented here

#![doc(html_logo_url = "https://knurling.ferrous-systems.com/knurling_logo_light_text.svg")]

use defmt_parser::Level;
use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use quote::quote;

mod attributes;
mod cargo;
mod construct;
mod consts;
mod derives;
mod function_like;
mod items;

// NOTE some proc-macro functions have an `_` (underscore) suffix. This is intentional.
// If left unsuffixed the procedural macros would shadow macros defined in `std` (like `assert`)
// within the context of this entire crate, which leads to lots of confusion.

/* # Attributes */
#[proc_macro_attribute]
pub fn global_logger(args: TokenStream, item: TokenStream) -> TokenStream {
    attributes::global_logger::expand(args, item)
}

#[proc_macro_attribute]
pub fn panic_handler(args: TokenStream, item: TokenStream) -> TokenStream {
    attributes::panic_handler::expand(args, item)
}

/* # Derives */
#[proc_macro_derive(Format, attributes(defmt))]
pub fn format(input: TokenStream) -> TokenStream {
    derives::format::expand(input)
}

/* # Function-like */
#[proc_macro]
pub fn assert_(args: TokenStream) -> TokenStream {
    function_like::assert_like::assert::expand(args)
}

#[proc_macro]
pub fn assert_eq_(args: TokenStream) -> TokenStream {
    function_like::assert_binop::eq(args)
}

#[proc_macro]
pub fn assert_ne_(args: TokenStream) -> TokenStream {
    function_like::assert_binop::ne(args)
}

/* ## `debug_` variants */
// NOTE these `debug_*` macros can be written using `macro_rules!` (that'd be simpler) but that
// results in an incorrect source code location being reported: the location of the `macro_rules!`
// statement is reported. Using a proc-macro results in the call site being reported, which is what
// we want
#[proc_macro]
pub fn debug_assert_(input: TokenStream) -> TokenStream {
    let assert = TokenStream2::from(assert_(input));
    quote!(if cfg!(debug_assertions) {
        #assert
    })
    .into()
}

#[proc_macro]
pub fn debug_assert_eq_(input: TokenStream) -> TokenStream {
    let assert = TokenStream2::from(assert_eq_(input));
    quote!(if cfg!(debug_assertions) {
        #assert
    })
    .into()
}

#[proc_macro]
pub fn debug_assert_ne_(input: TokenStream) -> TokenStream {
    let assert = TokenStream2::from(assert_ne_(input));
    quote!(if cfg!(debug_assertions) {
        #assert
    })
    .into()
}
/* ## end of `debug_` variants */

#[proc_macro]
pub fn dbg(args: TokenStream) -> TokenStream {
    function_like::dbg::expand(args)
}

#[proc_macro]
pub fn intern(args: TokenStream) -> TokenStream {
    function_like::intern::expand(args)
}

#[proc_macro]
pub fn internp(args: TokenStream) -> TokenStream {
    function_like::internp::expand(args)
}

#[proc_macro]
pub fn println(args: TokenStream) -> TokenStream {
    function_like::println::expand(args)
}

/* ## Logging macros */

#[proc_macro]
pub fn trace(args: TokenStream) -> TokenStream {
    function_like::log::expand(Level::Trace, args)
}

#[proc_macro]
pub fn debug(args: TokenStream) -> TokenStream {
    function_like::log::expand(Level::Debug, args)
}

#[proc_macro]
pub fn info(args: TokenStream) -> TokenStream {
    function_like::log::expand(Level::Info, args)
}

#[proc_macro]
pub fn warn(args: TokenStream) -> TokenStream {
    function_like::log::expand(Level::Warn, args)
}

#[proc_macro]
pub fn error(args: TokenStream) -> TokenStream {
    function_like::log::expand(Level::Error, args)
}
/* ## end of logging macros */

#[proc_macro]
pub fn panic_(args: TokenStream) -> TokenStream {
    function_like::panic_like::expand(args, "panicked at 'explicit panic'", |format_string| {
        format!("panicked at '{format_string}'")
    })
}

#[proc_macro]
pub fn todo_(args: TokenStream) -> TokenStream {
    function_like::panic_like::expand(args, "panicked at 'not yet implemented'", |format_string| {
        format!("panicked at 'not yet implemented: {format_string}'")
    })
}

#[proc_macro]
pub fn unreachable_(args: TokenStream) -> TokenStream {
    function_like::panic_like::expand(
        args,
        "panicked at 'internal error: entered unreachable code'",
        |format_string| {
            format!(
                "panicked at 'internal error: entered unreachable code: {}'",
                format_string
            )
        },
    )
}

#[proc_macro]
pub fn unwrap(args: TokenStream) -> TokenStream {
    function_like::assert_like::unwrap::expand(args)
}

#[proc_macro]
pub fn write(args: TokenStream) -> TokenStream {
    function_like::write::expand(args)
}

/* # Items */
#[proc_macro]
pub fn bitflags(ts: TokenStream) -> TokenStream {
    items::bitflags::expand(ts)
}

#[proc_macro]
pub fn timestamp(args: TokenStream) -> TokenStream {
    items::timestamp::expand(args)
}
