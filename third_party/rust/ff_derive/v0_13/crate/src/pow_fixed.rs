//! Fixed-exponent variable-base exponentiation using addition chains.

use addchain::{build_addition_chain, Step};
use num_bigint::BigUint;
use quote::quote;
use syn::Ident;

/// Returns t{n} as an ident.
fn get_temp(n: usize) -> Ident {
    Ident::new(&format!("t{}", n), proc_macro2::Span::call_site())
}

pub(crate) fn generate(
    base: &proc_macro2::TokenStream,
    exponent: BigUint,
) -> proc_macro2::TokenStream {
    let steps = build_addition_chain(exponent);

    let mut gen = proc_macro2::TokenStream::new();

    // First entry in chain is one, i.e. the base.
    let start = get_temp(0);
    gen.extend(quote! {
        let #start = #base;
    });

    let mut tmps = vec![start];
    for (i, step) in steps.into_iter().enumerate() {
        let out = get_temp(i + 1);

        gen.extend(match step {
            Step::Double { index } => {
                let val = &tmps[index];
                quote! {
                    let #out = #val.square();
                }
            }
            Step::Add { left, right } => {
                let left = &tmps[left];
                let right = &tmps[right];
                quote! {
                    let #out = #left * #right;
                }
            }
        });

        tmps.push(out.clone());
    }

    let end = tmps.last().expect("have last");
    gen.extend(quote! {
        #end
    });

    gen
}
