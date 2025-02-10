use quote::quote;

/// Custom derive for `abscissa_core::runnable::Runnable`
pub fn derive_runnable(s: synstructure::Structure<'_>) -> proc_macro2::TokenStream {
    let body = s.each(|bi| {
        quote! { #bi.run() }
    });

    s.gen_impl(quote! {
        #[allow(unknown_lints)]
        #[allow(non_local_definitions)]
        gen impl Runnable for @Self {
            fn run(&self) {
                match *self { #body }
            }
        }
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use synstructure::test_derive;

    #[test]
    fn derive_runnable_on_enum() {
        test_derive! {
            derive_runnable {
                enum MyRunnable {
                    A(VariantA),
                    B(VariantB),
                    C(VariantC),
                }
            }
            expands to {
                #[allow(non_upper_case_globals)]
                const _DERIVE_Runnable_FOR_MyRunnable: () = {
                    #[allow(unknown_lints)]
                    #[allow(non_local_definitions)]
                    impl Runnable for MyRunnable {
                        fn run(&self) {
                            match *self {
                                MyRunnable::A(ref __binding_0,) => {
                                    { __binding_0.run() }
                                }
                                MyRunnable::B(ref __binding_0,) => {
                                    { __binding_0.run() }
                                }
                                MyRunnable::C(ref __binding_0,) => {
                                    { __binding_0.run() }
                                }
                            }
                        }
                    }
                };
            }
            no_build // tests the code compiles are in the `abscissa` crate
        }
    }
}
