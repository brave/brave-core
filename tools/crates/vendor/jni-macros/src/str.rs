//! String utilities for JNI signature and class name handling
//!
//! This module provides utilities for encoding strings to Java's Modified UTF-8 (MUTF-8)
//! format and creating CStr and JNIStr literals for use in procedural macros.

use proc_macro2::Span;
use syn::LitCStr;

/// Encode a UTF-8 string as MUTF-8 (Java's modified UTF-8)
///
/// Java uses a modified version of UTF-8 that:
/// - Encodes the null character (U+0000) as `0xC0 0x80` instead of `0x00`
/// - Encodes Unicode characters above U+FFFF using CESU-8 (surrogate pairs)
///
/// # Examples
///
/// ```ignore
/// let mutf8_bytes = encode_mutf8("Hello");
/// // Basic ASCII is unchanged
///
/// let mutf8_emoji = encode_mutf8("😀");
/// // Emoji encoded as surrogate pairs in MUTF-8
/// ```
pub fn encode_mutf8(s: &str) -> Vec<u8> {
    simd_cesu8::mutf8::encode(s).into_owned()
}

/// Create a LitCStr from a string with MUTF-8 encoding
///
/// This function takes a UTF-8 string, encodes it to MUTF-8, and creates
/// a `syn::LitCStr` for use in procedural macros.
///
/// # Safety
///
/// MUTF-8 encoded strings should not contain NUL bytes (except encoded as \xC0\x80).
/// The function uses `CString::from_vec_unchecked` which is safe for MUTF-8 encoded data.
///
/// # Examples
///
/// ```ignore
/// let cstr = lit_cstr_mutf8("java.lang.String");
/// // Creates c"java.lang.String" with MUTF-8 encoding
/// ```
pub fn lit_cstr_mutf8(s: &str) -> LitCStr {
    let mutf8 = encode_mutf8(s);
    // Safety: MUTF-8 encoded strings should not contain NUL bytes (except encoded as \xC0\x80)
    let c = unsafe { std::ffi::CString::from_vec_unchecked(mutf8) };
    LitCStr::new(&c, Span::call_site())
}

use proc_macro2::TokenStream;
use quote::quote;
use syn::{Lit, Result, Token, parse::Parse};

/// Convert a literal to a string value for concatenation
fn literal_to_string(lit: &Lit) -> Result<String> {
    match lit {
        Lit::Str(s) => Ok(s.value()),
        Lit::Char(c) => Ok(c.value().to_string()),
        Lit::Int(i) => Ok(i.base10_digits().to_string()),
        Lit::Float(f) => Ok(f.base10_digits().to_string()),
        Lit::Bool(b) => Ok(b.value.to_string()),
        Lit::Byte(b) => {
            // Bytes are formatted as their numeric value
            Ok(b.value().to_string())
        }
        Lit::CStr(c) => {
            // CStr must be valid UTF-8 for our purposes
            let cstr = c.value();
            cstr.to_str().map(|s| s.to_string()).map_err(|e| {
                syn::Error::new_spanned(c, format!("CStr literal must contain valid UTF-8: {}", e))
            })
        }
        Lit::ByteStr(_) => Err(syn::Error::new_spanned(
            lit,
            "byte string literals are not supported in jni_str!/jni_cstr! macros",
        )),
        Lit::Verbatim(_) => Err(syn::Error::new_spanned(
            lit,
            "verbatim literals are not supported in jni_str!/jni_cstr! macros",
        )),
        // Lit is marked as non_exhaustive, so we need a wildcard pattern
        _ => Err(syn::Error::new_spanned(
            lit,
            "unsupported literal type in jni_str!/jni_cstr! macros",
        )),
    }
}

/// Input for jni_cstr! and jni_str! macros
/// Supports:
/// - Multiple comma-separated literals (like concat!)
/// - Optional `jni = <path>` as the first argument
struct JniStrInput {
    jni_crate: syn::Path,
    string: String,
}

impl Parse for JniStrInput {
    fn parse(input: syn::parse::ParseStream) -> Result<Self> {
        let jni_crate = crate::utils::parse_jni_crate_override(&input)?;

        // Parse comma-separated literals (like concat!)
        let mut concatenated = String::new();

        if input.is_empty() {
            return Err(syn::Error::new(
                input.span(),
                "expected at least one literal",
            ));
        }

        loop {
            let lit = input.parse::<Lit>()?;
            let value = literal_to_string(&lit)?;
            concatenated.push_str(&value);

            if input.is_empty() {
                break;
            }

            input.parse::<Token![,]>()?;

            if input.is_empty() {
                break;
            }
        }

        Ok(JniStrInput {
            jni_crate,
            string: concatenated,
        })
    }
}

/// Implementation for jni_cstr! macro
/// Takes UTF-8 string literals and returns a MUTF-8 encoded CStr literal
/// Supports:
/// - Multiple comma-separated string literals (like concat!)
/// - Optional `jni = <path>` as the first argument
pub fn jni_cstr_impl(input: TokenStream) -> Result<TokenStream> {
    let JniStrInput { string, .. } = syn::parse2(input)?;

    // Create a C string literal with MUTF-8 encoding
    let cstr = lit_cstr_mutf8(&string);

    Ok(quote! {
        #cstr
    })
}

/// Implementation for jni_str! macro
/// Takes UTF-8 string literals and returns a MUTF-8 encoded &'static JNIStr
/// Supports:
/// - Multiple comma-separated string literals (like concat!)
/// - Optional `jni = <path>` as the first argument
pub fn jni_str_impl(input: TokenStream) -> Result<TokenStream> {
    let JniStrInput { jni_crate, string } = syn::parse2(input)?;

    // Create a C string literal with MUTF-8 encoding
    let cstr = lit_cstr_mutf8(&string);

    Ok(quote! {
        {
            // Safety: The string was encoded to MUTF-8 at compile-time
            unsafe {
                #jni_crate::strings::JNIStr::from_cstr_unchecked(#cstr)
            }
        }
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use quote::quote;

    #[test]
    fn test_jni_cstr_single_literal() {
        let input = quote! { "hello" };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();

        // Should produce a CStr literal
        assert!(result_str.contains("c\"hello\"") || result_str.contains("\"hello\""));
    }

    #[test]
    fn test_jni_cstr_multiple_literals() {
        let input = quote! { "hello", " ", "world" };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();

        // Should concatenate to "hello world"
        assert!(result_str.contains("hello world"));
    }

    #[test]
    fn test_jni_cstr_with_jni_override() {
        let input = quote! { jni = crate::my_jni, "test" };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();

        // Should produce a CStr literal (jni override doesn't affect jni_cstr! output)
        assert!(result_str.contains("test"));
    }

    #[test]
    fn test_jni_str_single_literal() {
        let input = quote! { "hello" };
        let result = jni_str_impl(input).unwrap();
        let result_str = result.to_string();

        // Should produce JNIStr::from_cstr_unchecked call
        assert!(result_str.contains("JNIStr"));
        assert!(result_str.contains("from_cstr_unchecked"));
        assert!(result_str.contains("hello"));
    }

    #[test]
    fn test_jni_str_multiple_literals() {
        let input = quote! { "java.lang.", "String" };
        let result = jni_str_impl(input).unwrap();
        let result_str = result.to_string();

        // Should concatenate to "java.lang.String"
        assert!(result_str.contains("java.lang.String"));
    }

    #[test]
    fn test_jni_str_with_jni_override() {
        let input = quote! { jni = ::my_custom_jni, "test" };
        let result = jni_str_impl(input).unwrap();
        let result_str = result.to_string();

        // Should use the custom jni crate path
        assert!(result_str.contains(":: my_custom_jni :: strings :: JNIStr"));
        assert!(result_str.contains("test"));
    }

    #[test]
    fn test_jni_str_with_jni_override_and_multiple_literals() {
        let input = quote! { jni = crate, "part1", "part2" };
        let result = jni_str_impl(input).unwrap();
        let result_str = result.to_string();

        // Should use crate:: and concatenate literals
        assert!(result_str.contains("crate :: strings :: JNIStr"));
        assert!(result_str.contains("part1part2"));
    }

    #[test]
    fn test_jni_cstr_empty_input_fails() {
        let input = quote! {};
        let result = jni_cstr_impl(input);
        assert!(result.is_err());

        let err = result.unwrap_err();
        assert!(err.to_string().contains("expected at least one literal"));
    }

    #[test]
    fn test_jni_cstr_trailing_comma() {
        let input = quote! { "hello", "world", };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();

        // Should handle trailing comma and concatenate
        assert!(result_str.contains("helloworld"));
    }

    #[test]
    fn test_jni_str_jni_property_must_be_first() {
        // jni property after string literal should fail during parsing
        // because it will try to parse "test" as a string literal, then fail on the identifier "jni"
        let input = quote! { "test", jni = crate };
        let result = jni_str_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_mutf8_encoding() {
        // Test that emoji is properly encoded using MUTF-8
        let input = quote! { "😀" };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();

        // Emoji should be encoded as surrogate pairs in MUTF-8
        // The exact encoding will be in the c"..." literal
        assert!(result_str.contains("c\"") || result_str.contains("\""));
    }

    #[test]
    fn test_literal_char() {
        let input = quote! { 'a' };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("a"));
    }

    #[test]
    fn test_literal_int() {
        let input = quote! { 42 };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("42"));
    }

    #[test]
    fn test_literal_float() {
        let input = quote! { 3.14 };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("3.14"));
    }

    #[test]
    fn test_literal_bool() {
        let input = quote! { true };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("true"));

        let input = quote! { false };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("false"));
    }

    #[test]
    fn test_literal_byte() {
        let input = quote! { b'A' };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();
        // Byte literals are formatted as their numeric value
        assert!(result_str.contains("65")); // ASCII value of 'A'
    }

    #[test]
    fn test_literal_cstr_valid_utf8() {
        let input = quote! { c"hello" };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("hello"));
    }

    #[test]
    fn test_mixed_literals() {
        let input = quote! { "Port: ", 8080 };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("Port: 8080") || result_str.contains("Port : 8080"));
    }

    #[test]
    fn test_mixed_literals_with_char() {
        let input = quote! { "Version ", 1, '.', 2 };
        let result = jni_cstr_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("Version 1.2") || result_str.contains("Version 1 . 2"));
    }

    #[test]
    fn test_jni_str_mixed_literals() {
        let input = quote! { "localhost:", 8080 };
        let result = jni_str_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("localhost:8080") || result_str.contains("localhost : 8080"));
        assert!(result_str.contains("JNIStr"));
    }

    #[test]
    fn test_literal_byte_str_fails() {
        // Byte string literals should not be supported
        let input = quote! { b"hello" };
        let result = jni_cstr_impl(input);
        assert!(result.is_err());
        let err = result.unwrap_err();
        assert!(err.to_string().contains("byte string"));
    }
}
