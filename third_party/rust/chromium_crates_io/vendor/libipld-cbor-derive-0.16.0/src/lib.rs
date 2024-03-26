use proc_macro2::{Span, TokenStream};
use proc_macro_crate::{crate_name, FoundCrate};
use quote::quote;
use synstructure::{decl_derive, Structure};

decl_derive!([DagCbor, attributes(ipld)] => dag_cbor_derive);

mod ast;
mod attr;
mod gen;
mod parse;

fn dag_cbor_derive(s: Structure) -> TokenStream {
    let libipld = match use_crate("libipld") {
        Ok(ident) => ident,
        Err(error) => return error,
    };
    let ast = parse::parse(&s);
    let encode = gen::gen_encode(&ast, &libipld);
    let decode = gen::gen_decode(&ast, &libipld);
    quote! {
        #encode
        #decode
    }
}

/// Get the name of a crate based on its original name.
///
/// This works even if the crate was renamed in the `Cargo.toml` file. If the crate is not a
/// dependency, it will lead to a compile-time error.
fn use_crate(name: &str) -> Result<syn::Ident, TokenStream> {
    match crate_name(name) {
        Ok(FoundCrate::Name(n)) => Ok(syn::Ident::new(&n, Span::call_site())),
        Ok(FoundCrate::Itself) => Ok(syn::Ident::new("crate", Span::call_site())),
        Err(err) => Err(syn::Error::new(Span::call_site(), err).to_compile_error()),
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn test() {
        let t = trybuild::TestCases::new();
        t.pass("examples/basic.rs");
        t.pass("examples/name_attr.rs");
        t.pass("examples/repr_attr.rs");
    }
}
