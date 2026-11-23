use syn::{Ident, Token};

/// The default name for the `jni` crate
///
/// Note: This used to be resolved using `proc_macro_crate` but we later decided
/// to remove the dependency (it brings in numerous sub dependencies and adds
/// complexity that's not strictly necessary if the `jni=<path>` can be used to
/// specify the path explicitly). It should be very rare to need a non-default
/// path.
///
/// We now assume that the name will either be overridden explicitly or else we
/// use the default 'jni' name
fn jni_crate_default() -> syn::Path {
    syn::parse_quote!(::jni)
}

pub fn parse_jni_crate_override(input: &syn::parse::ParseStream) -> Result<syn::Path, syn::Error> {
    let mut jni_path: Option<syn::Path> = None;

    // First check for a special-case `jni = path` property for overriding the jni crate path
    if !input.is_empty() {
        let lookahead = input.lookahead1();
        if lookahead.peek(Ident) {
            // Peek at the first property name
            let fork = input.fork();
            let first_property: Ident = fork.parse()?;

            #[allow(clippy::cmp_owned)]
            if first_property.to_string() == "jni" {
                // Check if it's followed by '='
                if fork.peek(Token![=]) {
                    // Parse the jni property
                    let _property_name: Ident = input.parse()?;
                    input.parse::<Token![=]>()?;
                    jni_path = Some(input.parse()?);

                    // Skip comma after jni property
                    if input.peek(Token![,]) {
                        input.parse::<Token![,]>()?;
                    }
                }
            }
        }
    }

    Ok(jni_path.unwrap_or_else(jni_crate_default))
}
