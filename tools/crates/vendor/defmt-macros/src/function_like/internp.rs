use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, LitStr};

use crate::construct;

pub(crate) fn expand(args: TokenStream) -> TokenStream {
    let literal = parse_macro_input!(args as LitStr);
    let sym_name = construct::mangled_symbol_name("prim", &literal.value());

    let prefix = Some("prim");
    let section = construct::linker_section(false, prefix, &sym_name);
    let section_for_macos = construct::linker_section(true, prefix, &sym_name);

    let var_addr = if cfg!(feature = "unstable-test") {
        quote!({ defmt::export::fetch_add_string_index() as u16 })
    } else {
        quote!({
            #[cfg_attr(target_os = "macos", link_section = #section_for_macos)]
            #[cfg_attr(not(target_os = "macos"), link_section = #section)]
            #[export_name = #sym_name]
            static S: u8 = 0;
            &S as *const u8 as u16
        })
    };

    quote!({
        defmt::export::make_istr(#var_addr)
    })
    .into()
}
