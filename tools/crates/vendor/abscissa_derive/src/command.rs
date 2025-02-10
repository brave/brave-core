//! Custom derive support for `abscissa_core::command::Command`.

use proc_macro2::TokenStream;
use quote::quote;
use synstructure::Structure;

/// Custom derive for `abscissa_core::command::Command`
pub fn derive_command(s: Structure<'_>) -> TokenStream {
    let subcommand_usage = quote!();

    s.gen_impl(quote! {
        #[allow(unknown_lints)]
        #[allow(non_local_definitions)]
        gen impl Command for @Self {
            #[doc = "Name of this program as a string"]
            fn name() -> &'static str {
                env!("CARGO_PKG_NAME")
            }

            #[doc = "Description of this program"]
            fn description() -> &'static str {
                env!("CARGO_PKG_DESCRIPTION").trim()
            }

            #[doc = "Authors of this program"]
            fn authors() -> &'static str {
                env!("CARGO_PKG_AUTHORS")
            }

            #subcommand_usage
        }
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use synstructure::test_derive;

    #[test]
    fn derive_command_on_struct() {
        test_derive! {
            derive_command {
                struct MyCommand {}
            }
            expands to {
                #[allow(non_upper_case_globals)]
                const _DERIVE_Command_FOR_MyCommand: () = {
                    #[allow(unknown_lints)]
                    #[allow(non_local_definitions)]
                    impl Command for MyCommand {
                        #[doc = "Name of this program as a string"]
                        fn name() -> & 'static str {
                            env!("CARGO_PKG_NAME")
                        }

                        #[doc = "Description of this program"]
                        fn description () -> & 'static str {
                            env!("CARGO_PKG_DESCRIPTION" ).trim()
                        }

                        #[doc = "Authors of this program"]
                        fn authors() -> & 'static str {
                            env!("CARGO_PKG_AUTHORS")
                        }
                    }
                };
            }
            no_build // tests the code compiles are in the `abscissa` crate
        }
    }

    #[test]
    fn derive_command_on_enum() {
        test_derive! {
            derive_command {
                enum MyCommand {
                    Foo(A),
                    Bar(B),
                    Baz(C),
                }
            }
            expands to {
                #[allow(non_upper_case_globals)]
                const _DERIVE_Command_FOR_MyCommand: () = {
                    #[allow(unknown_lints)]
                    #[allow(non_local_definitions)]
                    impl Command for MyCommand {
                        #[doc = "Name of this program as a string"]
                        fn name() -> & 'static str {
                            env!("CARGO_PKG_NAME")
                        }

                        #[doc = "Description of this program"]
                        fn description () -> & 'static str {
                            env!("CARGO_PKG_DESCRIPTION" ).trim()
                        }

                        #[doc = "Authors of this program"]
                        fn authors() -> & 'static str {
                            env!("CARGO_PKG_AUTHORS")
                        }
                    }
                };
            }
            no_build // tests the code compiles are in the `abscissa` crate
        }
    }
}
