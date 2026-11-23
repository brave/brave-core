use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use quote::{quote, ToTokens};
use syn::{parse_macro_input, parse_quote};

use crate::{cargo, construct};

use self::input::Input;

mod input;

pub(crate) fn expand(input: TokenStream) -> TokenStream {
    let bitflags_input = TokenStream2::from(input.clone());
    let input = parse_macro_input!(input as Input);

    // Encode package and disambiguator to provide the decoder with all info it needs (even if
    // technically redundant, since it's also stored in the symbol we create).
    let format_string = format!(
        "{{={}:__internal_bitflags_{}@{}@{}@{}}}",
        input.ty().to_token_stream(),
        input.ident(),
        cargo::package_name(),
        construct::crate_local_disambiguator(),
        cargo::crate_name(),
    );
    let format_tag = construct::interned_string(
        &format_string,
        "bitflags",
        false,
        None,
        &parse_quote!(defmt),
    );

    let ident = input.ident();
    let ty = input.ty();
    let flag_statics = codegen_flag_statics(&input);
    quote!(
        const _: () = {
            fn assert<T: defmt::export::UnsignedInt>() {}
            assert::<#ty>;

            #(#flag_statics)*
        };

        defmt::export::bitflags! {
            #bitflags_input
        }

        impl defmt::Format for #ident {
            fn format(&self, f: defmt::Formatter) {
                defmt::unreachable!()
            }

            fn _format_tag() -> defmt::Str {
                #format_tag
            }

            fn _format_data(&self) {
                // There's a method available for every supported bitflags type.
                defmt::export::#ty(&self.bits());
            }
        }

    )
    .into()
}

fn codegen_flag_statics(input: &Input) -> Vec<TokenStream2> {
    input
        .flags()
        .enumerate()
        .map(|(i, flag)| {
            let cfg_attrs = flag.cfg_attrs();
            let var_name = flag.ident();
            let struct_name = input.ident();
            let repr_ty = input.ty();

            let sym_name = construct::mangled_symbol_name(
                "bitflags_value",
                &format!("{}::{i}::{}", input.ident(), flag.ident()),
            );

            quote! {
                #(#cfg_attrs)*
                // These values are decoder metadata and have no runtime reference that would
                // otherwise keep them when no defmt linker script is used.
                // MSVC represents `#[used]` as `/INCLUDE:<symbol>`, but `link.exe` fails to
                // resolve defmt's JSON symbol names when they are forced this way.
                #[cfg_attr(not(target_env = "msvc"), used)]
                #[cfg_attr(target_os = "macos", link_section = ".defmt,end")]
                #[cfg_attr(not(target_os = "macos"), link_section = ".defmt.end")]
                #[export_name = #sym_name]
                static #var_name: u128 = {
                    // NB: It might be tempting to just do `#value as u128` here, but that
                    // causes a value such as `1 << 127` to be evaluated as an `i32`, which
                    // overflows. So we instead coerce (but don't cast) it to the bitflags' raw
                    // type, and then cast that to u128.
                    let coerced_value: #repr_ty = #struct_name::#var_name.bits;
                    coerced_value as u128
                };
            }
        })
        .collect::<Vec<_>>()
}
