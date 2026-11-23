use proc_macro::TokenStream;
use proc_macro2::Span;
use quote::quote;
use syn::{parse_macro_input, Fields, ItemStruct};

pub(crate) fn expand(args: TokenStream, item: TokenStream) -> TokenStream {
    let strukt = parse_macro_input!(item as ItemStruct);

    if !args.is_empty() {
        return syn::Error::new(
            Span::call_site(),
            "`#[global_logger]` attribute takes no arguments",
        )
        .into_compile_error()
        .into();
    }

    if let Err(err) = validate(&strukt) {
        return err.into_compile_error().into();
    }

    codegen(&strukt)
}

fn validate(strukt: &ItemStruct) -> syn::Result<()> {
    let is_unit_struct = matches!(strukt.fields, Fields::Unit);

    if !strukt.generics.params.is_empty()
        || strukt.generics.where_clause.is_some()
        || !is_unit_struct
    {
        Err(syn::Error::new(
            strukt.ident.span(),
            "struct must be a non-generic unit struct (e.g. `struct S;`)",
        ))
    } else {
        Ok(())
    }
}

fn codegen(strukt: &ItemStruct) -> TokenStream {
    let attrs = &strukt.attrs;
    let ident = &strukt.ident;
    let vis = &strukt.vis;

    quote!(
        #(#attrs)*
        #vis struct #ident;

        #[inline(never)]
        #[no_mangle]
        unsafe fn _defmt_acquire()  {
            <#ident as defmt::Logger>::acquire()
        }

        #[inline(never)]
        #[no_mangle]
        unsafe fn _defmt_flush()  {
            <#ident as defmt::Logger>::flush()
        }

        #[inline(never)]
        #[no_mangle]
        unsafe fn _defmt_release()  {
            <#ident as defmt::Logger>::release()
        }

        #[inline(never)]
        #[no_mangle]
        unsafe fn _defmt_write(bytes: &[u8])  {
            <#ident as defmt::Logger>::write(bytes)
        }
    )
    .into()
}
