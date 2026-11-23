//! JNI-compatible method signature generator for Rust libraries.
//!
//! This crate was designed for use with the [`jni`](https://crates.io/crates/jni) crate, which
//! exposes JNI-compatible type bindings. Although it's possible to use `jni` without `jni_fn`, the
//! procedural macro defined here will make it easier to write the method signatures correctly.
//!
//! See the `jni_fn` attribute macro documentation below for more info and usage examples.

#![deny(missing_docs)]
#![deny(unsafe_code)]

use proc_macro2::TokenStream;
use quote::ToTokens;
use syn::parse_quote;
use syn::spanned::Spanned;

/// Deals exclusively with `proc_macro2::TokenStream` instead of `proc_macro::TokenStream`,
/// allowing it and all interior functionality to be unit tested.
pub fn jni_mangle2(attr: TokenStream, item: TokenStream) -> TokenStream {
    let attr_span = attr.span();
    let item_span = item.span();

    let mut function: syn::ItemFn = match syn::parse2(item) {
        Ok(f) => f,
        Err(_e) => {
            return syn::Error::new(
                item_span,
                "The `jni_mangle` attribute can only be applied to `fn` items",
            )
            .to_compile_error();
        }
    };

    // Parse the attribute arguments
    let args: syn::punctuated::Punctuated<syn::LitStr, syn::Token![,]> =
        match syn::parse::Parser::parse2(syn::punctuated::Punctuated::parse_terminated, attr) {
            Ok(args) => args,
            Err(_e) => {
                return syn::Error::new(
                    attr_span,
                    "The `jni_mangle` attribute must have string literal arguments",
                )
                .to_compile_error();
            }
        };

    if args.is_empty() || args.len() > 3 {
        return syn::Error::new(
            attr_span,
            "The `jni_mangle` attribute must have 1-3 string literal arguments",
        )
        .to_compile_error();
    }

    let namespace = args[0].value();

    if !valid_namespace(&namespace) {
        return syn::Error::new(
            attr_span,
            "Invalid package namespace supplied to `jni_mangle` attribute",
        )
        .to_compile_error();
    }

    let orig_fn_name = function.sig.ident.to_string();

    // Parse optional method name and signature
    let (method_name, signature) = match args.len() {
        1 => {
            // Just namespace - derive lowerCamelCase from Rust function name
            (snake_case_to_lower_camel_case(&orig_fn_name), None)
        }
        2 => {
            // Namespace + either method name or signature
            let second_arg = args[1].value();
            if second_arg.contains('(') {
                // It's a signature - derive method name from Rust function name
                (
                    snake_case_to_lower_camel_case(&orig_fn_name),
                    Some(second_arg),
                )
            } else {
                // It's a method name
                (second_arg, None)
            }
        }
        3 => {
            // Namespace + method name + signature
            (args[1].value(), Some(args[2].value()))
        }
        _ => unreachable!(),
    };

    let mangled_jni_name = create_jni_fn_name(&namespace, &method_name, signature.as_deref());

    // Specify the name of the exported function symbol
    //
    // Note: we don't change `function.sig.ident` and use `#[no_mangle]` so that the function can also
    // continue to be referenced by its Rust name within the crate.
    if cfg!(has_unsafe_attr) {
        // Add attributes for Rust 1.82+
        function
            .attrs
            .extend([parse_quote!(#[unsafe(export_name = #mangled_jni_name)])]);
    } else {
        // Add attributes for older Rust versions
        function
            .attrs
            .extend([parse_quote!(#[export_name = #mangled_jni_name])]);
    }

    function
        .attrs
        .extend([parse_quote!(#[allow(non_snake_case)])]);

    // Check ABI - must be "system" or unspecified
    if let Some(ref abi) = function.sig.abi {
        if let Some(ref name) = abi.name {
            if name.value() != "system" {
                return syn::Error::new(
                    name.span(),
                    format!(
                        "`jni_mangle` attributed functions must use `extern \"system\"` ABI, found `extern \"{}\"`",
                        name.value()
                    ),
                )
                .to_compile_error();
            }
            // ABI is already "system", keep it as is
        } else {
            // extern with no explicit ABI string - set to "system"
            function.sig.abi = Some(syn::Abi {
                extern_token: abi.extern_token,
                name: Some(syn::LitStr::new("system", function.sig.ident.span())),
            });
        }
    } else {
        // No ABI specified - set to "system"
        function.sig.abi = Some(syn::Abi {
            extern_token: Default::default(),
            name: Some(syn::LitStr::new("system", function.sig.ident.span())),
        });
    }

    if !matches!(function.vis, syn::Visibility::Public(_)) {
        return syn::Error::new(
            function.vis.span(),
            "`jni_mangle` attributed functions must have public visibility (`pub`)",
        )
        .to_compile_error();
    }

    function.into_token_stream()
}

/// Ensures that `namespace` appears roughly like a valid package name.
///
/// A package name is a '.'-separated identifier list.
///
/// Identifiers are described in section 3.8 of the Java language specification, although some
/// JVM-compatible languages have slightly different restrictions on what is considered a valid
/// identifier. This function attempts to catch obviously incorrect strings.
///
/// Please submit an issue report or patch to make this more permissive if it's required for
/// valid JVM code! Otherwise, making it more restrictive is appreciated as long as it's confirmed
/// to work with multiple JVM-compatible languages.
fn valid_namespace(namespace: &str) -> bool {
    /// These shouldn't occur _anywhere_ in the package name.
    const FORBIDDEN_CHARS: &[char] = &[
        ' ', ',', ':', ';', '|', '\\', '/', '!', '@', '#', '%', '^', '&', '*', '(', ')', '{', '}',
        '[', ']', '-', '`', '~', '\t', '\n', '\r',
    ];

    for c in FORBIDDEN_CHARS {
        if namespace.contains(*c) {
            return false;
        }
    }

    // Check for leading or trailing dots
    if namespace.starts_with('.') || namespace.ends_with('.') {
        return false;
    }

    // Check for consecutive dots (which would create empty identifiers)
    if namespace.contains("..") {
        return false;
    }

    fn is_valid_ident(ident: &str) -> bool {
        /// These shouldn't occur as the first character of an identifier.
        const FORBIDDEN_START_CHARS: &[char] = &['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'];

        if ident.is_empty() {
            return false;
        }

        for c in FORBIDDEN_START_CHARS {
            if ident.starts_with(*c) {
                return false;
            }
        }

        true
    }

    for ident in namespace.split('.') {
        if !is_valid_ident(ident) {
            return false;
        }
    }

    true
}

/// Converts a snake_case identifier to lowerCamelCase.
/// This transformation is idempotent - if the input is already in lowerCamelCase, it returns unchanged.
/// If the input contains any uppercase letters, it's returned unchanged to preserve intentional casing.
/// Leading underscores are preserved except for one underscore that is removed.
/// Trailing underscores are preserved.
///
/// When capitalizing segments after underscores, the first non-numeric character is capitalized.
/// This ensures that segments with numeric prefixes are properly capitalized.
///
/// Examples:
/// - "say_hello" -> "sayHello"
/// - "get_user_name" -> "getUserName"
/// - "_private_method" -> "privateMethod" (one leading underscore removed)
/// - "__dunder__" -> "_dunder__" (one leading underscore removed)
/// - "___priv" -> "__priv" (one leading underscore removed)
/// - "trailing_" -> "trailing_"
/// - "sayHello" -> "sayHello" (unchanged)
/// - "getUserName" -> "getUserName" (unchanged)
/// - "Foo_Bar" -> "Foo_Bar" (unchanged - contains uppercase)
/// - "XMLParser" -> "XMLParser" (unchanged - contains uppercase)
/// - "init" -> "init" (unchanged - no underscores)
/// - "test_αλφα" -> "testΑλφα" (Unicode-aware)
/// - "array_2d_foo" -> "array2DFoo" (capitalizes first char after digits)
/// - "test_3d" -> "test3D" (capitalizes first char after digits)
pub fn snake_case_to_lower_camel_case(s: &str) -> String {
    // If the string contains any uppercase letters, assume it's intentionally cased
    // and return it unchanged
    if s.chars().any(|c| c.is_uppercase()) {
        return s.to_string();
    }

    // Find leading underscores
    let leading_underscores = s.chars().take_while(|&c| c == '_').count();

    // Find trailing underscores
    let trailing_underscores = s.chars().rev().take_while(|&c| c == '_').count();

    // If the entire string is underscores, return as-is
    if leading_underscores + trailing_underscores >= s.len() {
        return s.to_string();
    }

    // Extract the middle part (without leading/trailing underscores)
    let middle = &s[leading_underscores..s.len() - trailing_underscores];

    // Convert the middle part
    let mut result = String::new();
    let mut capitalize_next = false;

    for c in middle.chars() {
        if c == '_' {
            capitalize_next = true;
        } else if capitalize_next {
            // If this is a digit, just add it and keep looking for the first non-digit to capitalize
            if c.is_ascii_digit() {
                result.push(c);
            } else {
                // Use Unicode-aware uppercase conversion on the first non-digit character
                for upper_c in c.to_uppercase() {
                    result.push(upper_c);
                }
                capitalize_next = false;
            }
        } else {
            result.push(c);
        }
    }

    // Reconstruct with leading and trailing underscores
    // Remove one leading underscore (if any exist)
    let adjusted_leading_underscores = if leading_underscores > 0 {
        leading_underscores - 1
    } else {
        0
    };

    let mut final_result = String::with_capacity(s.len());
    for _ in 0..adjusted_leading_underscores {
        final_result.push('_');
    }
    final_result.push_str(&result);
    for _ in 0..trailing_underscores {
        final_result.push('_');
    }

    final_result
}

/// Creates a JNI-compatible function name from the given namespace, function name, and optional signature.
/// This does _not_ transform the provided function name into `snakeCase` if it's not already; but
/// `#[allow(non_snake_case)]` should be added to prevent errors.
///
/// Any underscores in the original namespace or function name need to be replaced by "_1", and
/// then dot separators need to be turned into underscores.
///
/// For signatures (if provided), only the argument types (between parentheses) are encoded:
/// - '_' -> "_1"
/// - ';' -> "_2"
/// - '[' -> "_3"
/// - '/' -> "_"
/// - Non-ASCII characters (including '$') -> "_0xxxx" where xxxx is the lowercase hex Unicode codepoint
///
/// The return type is ignored, and parentheses are not included in the mangled name.
pub fn create_jni_fn_name(namespace: &str, fn_name: &str, signature: Option<&str>) -> String {
    fn mangle_identifier(s: &str) -> String {
        let mut result = String::new();
        for c in s.chars() {
            match c {
                '_' => result.push_str("_1"),
                '.' => result.push('_'),
                // Handle ASCII alphanumeric and safe characters
                _ if c.is_ascii_alphanumeric() => result.push(c),
                // Everything else (including '$' and non-ASCII) gets encoded
                _ => result.push_str(&format!("_0{:04x}", c as u32)),
            }
        }
        result
    }

    fn mangle_signature_args(s: &str) -> String {
        let mut result = String::new();
        for c in s.chars() {
            match c {
                '_' => result.push_str("_1"),
                ';' => result.push_str("_2"),
                '[' => result.push_str("_3"),
                '/' => result.push('_'),
                _ if c.is_ascii_alphanumeric() => result.push(c),
                _ => {
                    // Non-ASCII character or other special chars - encode as _0xxxx
                    result.push_str(&format!("_0{:04x}", c as u32));
                }
            }
        }
        result
    }

    let namespace_underscored = mangle_identifier(namespace);
    let fn_name_underscored = mangle_identifier(fn_name);

    let mut result = format!("Java_{}_{}", namespace_underscored, fn_name_underscored);

    if let Some(sig) = signature {
        // Extract only the argument types (between parentheses), ignoring return type
        #[allow(clippy::collapsible_if)]
        if let Some(start) = sig.find('(') {
            if let Some(end) = sig.find(')') {
                let args = &sig[start + 1..end];
                // Always add __ when signature is provided (indicates overloaded method)
                result.push_str("__");
                if !args.is_empty() {
                    result.push_str(&mangle_signature_args(args));
                }
            }
        }
    }

    result
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_create_jni_fn_name() {
        // Basic namespace and function name tests
        assert_eq!(
            create_jni_fn_name("com.example.Foo", "init", None),
            "Java_com_example_Foo_init"
        );
        assert_eq!(
            create_jni_fn_name("com.example.Bar", "closeIt", None),
            "Java_com_example_Bar_closeIt"
        );
        assert_eq!(
            create_jni_fn_name("com.example.Bar", "close_it", None),
            "Java_com_example_Bar_close_1it"
        );
        assert_eq!(
            create_jni_fn_name(
                "org.signal.client.internal.Native",
                "IdentityKeyPair_Deserialize",
                None
            ),
            "Java_org_signal_client_internal_Native_IdentityKeyPair_1Deserialize"
        );
        assert_eq!(
            create_jni_fn_name("a.b.c.Test$", "show", None),
            "Java_a_b_c_Test_00024_show"
        );

        // Tests with signatures - only argument types are encoded, no parens or return type
        assert_eq!(
            create_jni_fn_name("com.example.Foo", "method", Some("(I)Z")),
            "Java_com_example_Foo_method__I"
        );
        assert_eq!(
            create_jni_fn_name("com.example.Bar", "test", Some("(Ljava/lang/String;)V")),
            "Java_com_example_Bar_test__Ljava_lang_String_2"
        );
        assert_eq!(
            create_jni_fn_name("a.b.Test", "arrayMethod", Some("([I)[Ljava/lang/Object;")),
            "Java_a_b_Test_arrayMethod___3I"
        );
        assert_eq!(
            create_jni_fn_name(
                "com.example.Test",
                "complex_method",
                Some("([[Ljava/lang/String;I)[[I")
            ),
            "Java_com_example_Test_complex_1method___3_3Ljava_lang_String_2I"
        );
        // Test with no arguments (empty parentheses) - should still have __ suffix
        assert_eq!(
            create_jni_fn_name("com.example.Foo", "noArgs", Some("()V")),
            "Java_com_example_Foo_noArgs__"
        );
    }

    #[test]
    fn test_valid_namespace() {
        // Valid namespaces
        assert!(valid_namespace("com.example.Foo"));
        assert!(valid_namespace("com.antonok.kb"));
        assert!(valid_namespace("org.signal.client.internal.Native"));
        assert!(valid_namespace("net.under_score"));
        assert!(valid_namespace("a.b.c.Test$"));

        // Invalid namespaces - spaces and special characters
        assert!(!valid_namespace("com example Foo"));
        assert!(!valid_namespace(" com.example.Foo"));
        assert!(!valid_namespace("com.example.Foo "));
        assert!(!valid_namespace("com.example.1Foo"));

        // Invalid namespaces - leading dots
        assert!(!valid_namespace(".com.example.Foo"));
        assert!(!valid_namespace("."));

        // Invalid namespaces - trailing dots
        assert!(!valid_namespace("com.example.Foo."));

        // Invalid namespaces - consecutive dots
        assert!(!valid_namespace("com..example.Foo"));
        assert!(!valid_namespace("com...example.Foo"));
        assert!(!valid_namespace("com.example..Foo"));
    }

    #[test]
    fn test_code_generation() {
        let attr = quote::quote! {
            "com.example.Bar"
        };
        let source = quote::quote! {
            pub fn close_it(env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        // Note: close_it becomes closeIt (lowerCamelCase)
        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Bar_closeIt")]
                    #[allow(non_snake_case)]
                    pub extern "system" fn close_it(env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                        unimplemented!()
                    }
                }
            )
        );
    }

    #[test]
    fn test_unsafe_fn() {
        let attr = quote::quote! {
            "com.example.Bar"
        };
        let source = quote::quote! {
            pub unsafe fn close_it(env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        // Note: close_it becomes closeIt (lowerCamelCase)
        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Bar_closeIt")]
                    #[allow(non_snake_case)]
                    pub unsafe extern "system" fn close_it(env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                        unimplemented!()
                    }
                }
            )
        );
    }

    #[test]
    fn test_non_function() {
        let attr = quote::quote! { "com.example.Foo" };
        let source = quote::quote! {
            enum NotAFunction {
                Variant1,
                Variant2(u8),
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    ::core::compile_error! { "The `jni_mangle` attribute can only be applied to `fn` items" }
                }
            )
        );
    }

    #[test]
    fn test_empty_attribute() {
        let attr = quote::quote! {};
        let source = quote::quote! {
            pub fn close_it(env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    ::core::compile_error! { "The `jni_mangle` attribute must have 1-3 string literal arguments" }
                }
            )
        );
    }

    #[test]
    fn test_invalid_namespace() {
        let attr = quote::quote! { "." };
        let source = quote::quote! {
            pub fn close_it(env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    ::core::compile_error! { "Invalid package namespace supplied to `jni_mangle` attribute" }
                }
            )
        );
    }

    #[test]
    fn test_wrong_abi_generates_error() {
        let attr = quote::quote! { "com.example.Foo" };
        let source = quote::quote! {
            pub extern "C" fn closeIt(env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        // Should generate an error for non-system ABI
        let expanded_str = format!("{}", expanded);
        assert!(expanded_str.contains("compile_error"));
        assert!(expanded_str.contains("must use `extern \\\"system\\\"` ABI"));
        assert!(expanded_str.contains("found `extern \\\"C\\\"`"));
    }

    #[test]
    fn test_system_abi_is_preserved() {
        let attr = quote::quote! { "com.example.Foo" };
        let source = quote::quote! {
            pub extern "system" fn closeIt(env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        // The "system" ABI should be preserved
        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Foo_closeIt")]
                    #[allow(non_snake_case)]
                    pub extern "system" fn closeIt (env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                        unimplemented!()
                    }
                }
            )
        );
    }

    #[test]
    fn test_wrong_visibility() {
        let attr = quote::quote! { "com.example.Foo" };
        let source = quote::quote! {
            fn close_it(env: JNIEnv, _: JClass, filename: JString) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    ::core::compile_error! { "`jni_mangle` attributed functions must have public visibility (`pub`)" }
                }
            )
        );
    }

    #[test]
    fn test_with_method_name() {
        let attr = quote::quote! {
            "com.example.Bar", "customMethod"
        };
        let source = quote::quote! {
            pub fn rust_function(env: JNIEnv, _: JClass) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Bar_customMethod")]
                    #[allow(non_snake_case)]
                    pub extern "system" fn rust_function (env: JNIEnv, _: JClass) -> jboolean {
                        unimplemented!()
                    }
                }
            )
        );
    }

    #[test]
    fn test_with_signature_only() {
        let attr = quote::quote! {
            "com.example.Bar", "(I)Z"
        };
        let source = quote::quote! {
            pub fn boolMethod(env: JNIEnv, _: JClass) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Bar_boolMethod__I")]
                    #[allow(non_snake_case)]
                    pub extern "system" fn boolMethod (env: JNIEnv, _: JClass) -> jboolean {
                        unimplemented!()
                    }
                }
            )
        );
    }

    #[test]
    fn test_with_method_name_and_signature() {
        let attr = quote::quote! {
            "com.example.Bar", "testMethod", "(Ljava/lang/String;)V"
        };
        let source = quote::quote! {
            pub fn rust_func(env: JNIEnv, _: JClass) {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Bar_testMethod__Ljava_lang_String_2")]
                    #[allow(non_snake_case)]
                    pub extern "system" fn rust_func (env: JNIEnv, _: JClass) {
                        unimplemented!()
                    }
                }
            )
        );
    }

    #[test]
    fn test_snake_case_to_lower_camel_case() {
        // Basic conversions
        assert_eq!(snake_case_to_lower_camel_case("say_hello"), "sayHello");
        assert_eq!(
            snake_case_to_lower_camel_case("get_user_name"),
            "getUserName"
        );
        assert_eq!(snake_case_to_lower_camel_case("init"), "init");
        assert_eq!(snake_case_to_lower_camel_case("close_it"), "closeIt");

        // Idempotent - already lowerCamelCase
        assert_eq!(snake_case_to_lower_camel_case("sayHello"), "sayHello");
        assert_eq!(snake_case_to_lower_camel_case("getUserName"), "getUserName");
        assert_eq!(snake_case_to_lower_camel_case("closeIt"), "closeIt");

        // Mixed case - preserved unchanged
        assert_eq!(snake_case_to_lower_camel_case("Foo_Bar"), "Foo_Bar");
        assert_eq!(snake_case_to_lower_camel_case("XMLParser"), "XMLParser");
        assert_eq!(snake_case_to_lower_camel_case("IOError"), "IOError");
        assert_eq!(snake_case_to_lower_camel_case("HTML_Parser"), "HTML_Parser");

        // Unicode support
        assert_eq!(snake_case_to_lower_camel_case("café_résumé"), "caféRésumé");
        assert_eq!(snake_case_to_lower_camel_case("ß_test"), "ßTest");
        assert_eq!(snake_case_to_lower_camel_case("test_αλφα"), "testΑλφα");
        assert_eq!(
            snake_case_to_lower_camel_case("method_привет"),
            "methodПривет"
        );

        // Leading underscores - one is removed
        assert_eq!(
            snake_case_to_lower_camel_case("_private_method"),
            "privateMethod"
        );
        assert_eq!(snake_case_to_lower_camel_case("__dunder__"), "_dunder__");
        assert_eq!(snake_case_to_lower_camel_case("_leading"), "leading");
        assert_eq!(
            snake_case_to_lower_camel_case("__leading_multiple"),
            "_leadingMultiple"
        );
        assert_eq!(snake_case_to_lower_camel_case("___priv"), "__priv");
        assert_eq!(snake_case_to_lower_camel_case("_priv_name"), "privName");
        assert_eq!(snake_case_to_lower_camel_case("__priv_name"), "_privName");

        // Trailing underscores preserved
        assert_eq!(snake_case_to_lower_camel_case("trailing_"), "trailing_");
        assert_eq!(
            snake_case_to_lower_camel_case("trailing_multiple__"),
            "trailingMultiple__"
        );
        assert_eq!(snake_case_to_lower_camel_case("_a_"), "a_");
        assert_eq!(snake_case_to_lower_camel_case("_foo_bar_"), "fooBar_");

        // All underscores
        assert_eq!(snake_case_to_lower_camel_case("___"), "___");

        // Edge cases
        assert_eq!(snake_case_to_lower_camel_case("a_b_c"), "aBC");

        // Numeric prefixes - capitalize first non-digit character
        assert_eq!(snake_case_to_lower_camel_case("array_2d_foo"), "array2DFoo");
        assert_eq!(snake_case_to_lower_camel_case("test_3d"), "test3D");
        assert_eq!(snake_case_to_lower_camel_case("foo_123bar"), "foo123Bar");
        assert_eq!(snake_case_to_lower_camel_case("get_2d_array"), "get2DArray");
        assert_eq!(snake_case_to_lower_camel_case("test_42"), "test42");
        assert_eq!(snake_case_to_lower_camel_case("a_1_b"), "a1B");
    }

    #[test]
    fn test_snake_case_function_name_conversion() {
        // Test that snake_case function names are automatically converted to lowerCamelCase
        let attr = quote::quote! {
            "com.example.Bar"
        };
        let source = quote::quote! {
            pub fn say_hello_world(env: JNIEnv, _: JClass) {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Bar_sayHelloWorld")]
                    #[allow(non_snake_case)]
                    pub extern "system" fn say_hello_world (env: JNIEnv, _: JClass) {
                        unimplemented!()
                    }
                }
            )
        );
    }

    #[test]
    fn test_camel_case_function_name_unchanged() {
        // Test that lowerCamelCase function names remain unchanged (idempotent)
        let attr = quote::quote! {
            "com.example.Bar"
        };
        let source = quote::quote! {
            pub fn sayHelloWorld(env: JNIEnv, _: JClass) {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Bar_sayHelloWorld")]
                    #[allow(non_snake_case)]
                    pub extern "system" fn sayHelloWorld (env: JNIEnv, _: JClass) {
                        unimplemented!()
                    }
                }
            )
        );
    }

    #[test]
    fn test_snake_case_with_signature() {
        // Test that snake_case function names are converted even when signature is present
        let attr = quote::quote! {
            "com.example.Bar", "(I)Z"
        };
        let source = quote::quote! {
            pub fn check_valid(env: JNIEnv, _: JClass) -> jboolean {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Bar_checkValid__I")]
                    #[allow(non_snake_case)]
                    pub extern "system" fn check_valid (env: JNIEnv, _: JClass) -> jboolean {
                        unimplemented!()
                    }
                }
            )
        );
    }

    #[test]
    fn test_signature_with_no_args() {
        // Test that signatures with no arguments still add __ suffix (for overloaded methods)
        let attr = quote::quote! {
            "com.example.Bar", "()V"
        };
        let source = quote::quote! {
            pub fn noArgs(env: JNIEnv, _: JClass) {
                unimplemented!()
            }
        };

        let expanded = jni_mangle2(attr, source);

        // Should have __ even with no arguments (indicates overloaded method)
        assert_eq!(
            format!("{}", expanded),
            format!(
                "{}",
                quote::quote! {
                    #[unsafe(export_name = "Java_com_example_Bar_noArgs__")]
                    #[allow(non_snake_case)]
                    pub extern "system" fn noArgs (env: JNIEnv, _: JClass) {
                        unimplemented!()
                    }
                }
            )
        );
    }
}
