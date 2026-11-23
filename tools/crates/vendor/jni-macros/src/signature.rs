//! Procedural macro for generating JNI signatures at compile time
//!
//! This module provides a `jni_sig!` macro that parses method and field signatures
//! and generates JNI signature string literals.
//!
//! # Method Signatures
//! Method signatures have the form `(parameters) -> return_type` and generate a JNI
//! method descriptor like `"(ILjava/lang/String;)V"`.
//!
//! # Field Signatures
//! Field signatures are just a bare type and generate a JNI field descriptor like
//! `"Ljava/lang/String;"` or `"I"`.
//!
//! The parser automatically detects which type of signature based on the presence of
//! parentheses and a return arrow.

use proc_macro2::{Span, TokenStream};
use quote::quote;
use syn::{
    Ident, Result, Token, custom_keyword, parenthesized,
    parse::{Parse, ParseStream},
    token,
};

use crate::{
    str::lit_cstr_mutf8,
    types::{ConcreteType, JavaClassName, PrimitiveType, SigType, TypeMappings, parse_type},
};

custom_keyword!(sig);

/// Represents a method parameter
#[derive(Debug, Clone)]
pub struct Parameter {
    #[allow(dead_code)]
    pub name: Ident,
    pub ty: SigType,
}

/// Represents a method signature
#[derive(Debug, Clone)]
pub struct MethodSignature {
    pub parameters: Vec<Parameter>,
    pub return_type: SigType,
}

impl MethodSignature {
    /// Convert to JNI method signature string
    pub fn to_jni_signature(&self, type_mappings: &TypeMappings) -> Result<String> {
        let mut result = String::from("(");

        for param in &self.parameters {
            result.push_str(&param.ty.to_jni_descriptor(type_mappings)?);
        }

        result.push(')');
        result.push_str(&self.return_type.to_jni_descriptor(type_mappings)?);

        Ok(result)
    }
}

/// Parse sig = (args) -> ret
pub fn parse_method_sig(
    input: ParseStream,
    type_mappings: &TypeMappings,
) -> Result<MethodSignature> {
    input.parse::<sig>()?;
    input.parse::<Token![=]>()?;

    let args_content;
    parenthesized!(args_content in input);

    let mut parameters = Vec::new();
    let mut param_index = 0;

    while !args_content.is_empty() {
        let param = parse_parameter_with_index(&args_content, param_index, type_mappings)?;
        parameters.push(param);
        param_index += 1;

        if !args_content.is_empty() {
            args_content.parse::<Token![,]>()?;
        }
    }

    let return_type = if input.peek(Token![->]) {
        input.parse::<Token![->]>()?;
        parse_type(input, type_mappings)?
    } else {
        // No return type implies void
        SigType::Alias("void".to_string())
    };

    Ok(MethodSignature {
        parameters,
        return_type,
    })
}

/// Represents a field signature
#[derive(Debug, Clone)]
pub struct FieldSignature {
    pub field_type: SigType,
}

impl FieldSignature {
    /// Convert to JNI field signature string (just the type descriptor)
    pub fn to_jni_signature(&self, type_mappings: &TypeMappings) -> Result<String> {
        self.field_type.to_jni_descriptor(type_mappings)
    }
}

/// Parse sig = Type for fields
pub fn parse_field_sig(input: ParseStream, type_mappings: &TypeMappings) -> Result<FieldSignature> {
    input.parse::<sig>()?;
    input.parse::<Token![=]>()?;
    let field_type = parse_type(input, type_mappings)?;
    Ok(FieldSignature { field_type })
}

/// Represents either a method or field signature
#[derive(Debug, Clone)]
pub enum Signature {
    Method(MethodSignature),
    Field(FieldSignature),
}

impl Signature {
    /// Convert to JNI signature string
    pub fn to_jni_signature(&self, type_mappings: &TypeMappings) -> Result<String> {
        match self {
            Signature::Method(sig) => sig.to_jni_signature(type_mappings),
            Signature::Field(sig) => sig.to_jni_signature(type_mappings),
        }
    }
}

/// Parse a parameter with an index for fallback naming: `name: type` or just `type`
/// If only a type is provided, generates a fallback name like "arg0", "arg1", etc.
pub fn parse_parameter_with_index(
    input: ParseStream,
    index: usize,
    type_mappings: &TypeMappings,
) -> Result<Parameter> {
    if input.peek(Ident) && input.peek2(Token![:]) {
        // Named parameter
        let name = input.parse::<Ident>()?;
        input.parse::<Token![:]>()?;
        let ty = parse_type(input, type_mappings)?;
        Ok(Parameter { name, ty })
    } else {
        // Unnamed parameter - generate fallback name
        let ty = parse_type(input, type_mappings)?;
        let name = Ident::new(&format!("arg{}", index), Span::call_site());
        Ok(Parameter { name, ty })
    }
}

/// Represents the full macro input with named properties
struct SignatureInput {
    signature: Signature,
    type_mappings: TypeMappings,
}

impl Parse for SignatureInput {
    fn parse(input: ParseStream) -> Result<Self> {
        // Parse properties in the form: prop = value, prop = value, ...
        // If an argument is unnamed, it's assumed to be the signature.
        // This allows properties to be specified in any order and makes it easy
        // for wrapper macros to inject properties like `jni = foo,` without
        // needing to parse the syntax or worry about positioning.

        let mut signature: Option<Signature> = None;

        let jni_path = crate::utils::parse_jni_crate_override(&input)?;

        // Initialize TypeMappings unconditionally now that we have jni_crate
        let mut type_mappings = TypeMappings::new(&jni_path);

        // Parse remaining properties/arguments
        while !input.is_empty() {
            // Check if this looks like a named property ('<ident> = <value>' or '<ident> { ... }')
            let is_named_property = {
                let fork = input.fork();
                if fork.peek(Ident) {
                    if let Ok(_ident) = fork.parse::<Ident>() {
                        fork.peek(Token![=]) || fork.peek(syn::token::Brace)
                    } else {
                        false
                    }
                } else {
                    false
                }
            };

            if is_named_property {
                // Parse named property
                let prop_name = input.parse::<Ident>()?;
                let prop_str = prop_name.to_string();

                match prop_str.as_str() {
                    "jni" => {
                        // jni can only be the first property
                        return Err(syn::Error::new(
                            prop_name.span(),
                            "jni property must be the first property if specified",
                        ));
                    }
                    "sig" => {
                        input.parse::<Token![=]>()?;
                        if signature.is_some() {
                            return Err(syn::Error::new(
                                prop_name.span(),
                                "signature specified multiple times (either as unnamed argument or with 'sig =')",
                            ));
                        }
                        signature = Some(parse_signature(input, &type_mappings)?);
                    }
                    "type_map" => {
                        type_mappings.parse_mappings(input)?;
                    }
                    _ => {
                        return Err(syn::Error::new(
                            prop_name.span(),
                            format!(
                                "unknown property '{}'. Valid properties are: sig, type_map, jni",
                                prop_str
                            ),
                        ));
                    }
                }
            } else {
                // Unnamed argument - assume it's the signature
                if signature.is_some() {
                    return Err(syn::Error::new(
                        input.span(),
                        "signature specified multiple times (either as unnamed argument or with 'sig =')",
                    ));
                }
                signature = Some(parse_signature(input, &type_mappings)?);
            }

            if !input.is_empty() {
                input.parse::<Token![,]>()?;
            }
        }

        // Signature is required
        let signature = signature.ok_or_else(|| {
            syn::Error::new(
                Span::call_site(),
                "signature is required (either as unnamed argument or with 'sig =')",
            )
        })?;

        Ok(SignatureInput {
            signature,
            type_mappings,
        })
    }
}

/// Parse a signature (method or field) from input
fn parse_signature(input: ParseStream, type_mappings: &TypeMappings) -> Result<Signature> {
    // If the input starts with a string literal, it could either be a raw JNI signature
    // or a Java class name for a field signature.
    //
    // If the string literal contains dots (.), we'll treat it as a Java class name for a field signature.
    if input.peek(syn::LitStr) || input.peek(syn::LitCStr) {
        // Parse the string literal
        let raw_sig: String = if input.peek(syn::LitCStr) {
            let lit = input.parse::<syn::LitCStr>()?;
            let cstr = lit.value();
            cstr.to_str()
                .map_err(|e| {
                    syn::Error::new(
                        lit.span(),
                        format!("CStr literal must contain valid UTF-8: {}", e),
                    )
                })?
                .to_string()
        } else {
            let lit = input.parse::<syn::LitStr>()?;
            lit.value()
        };

        let is_raw_jni_sig = !raw_sig.contains('.');

        if is_raw_jni_sig {
            // Parse as raw JNI signature
            parse_raw_jni_signature(&raw_sig)
        } else {
            // It contains a dot, so treat it as a Java class name for a field signature
            // Parse it as a JavaClassName
            let parts: Vec<&str> = raw_sig.split('.').collect();
            if parts.is_empty() {
                return Err(syn::Error::new(
                    input.span(),
                    format!("Empty Java class name in string literal: '{}'", raw_sig),
                ));
            }

            let class = parts.last().unwrap().to_string();
            let package: Vec<String> = parts[..parts.len() - 1]
                .iter()
                .map(|s| s.to_string())
                .collect();

            let java_class = JavaClassName { package, class };
            Ok(Signature::Field(FieldSignature {
                field_type: SigType::Object(java_class),
            }))
        }
    } else if input.peek(token::Paren) {
        // Method signature: (args) [-> ret]
        let args_content;
        parenthesized!(args_content in input);

        let mut parameters = Vec::new();

        if !args_content.is_empty() {
            let mut param_index = 0;
            loop {
                let param = parse_parameter_with_index(&args_content, param_index, type_mappings)?;
                parameters.push(param);
                param_index += 1;

                if args_content.is_empty() {
                    break;
                }
                args_content.parse::<Token![,]>()?;
                if args_content.is_empty() {
                    break;
                }
            }
        }

        let return_type = if input.peek(Token![->]) {
            input.parse::<Token![->]>()?;

            parse_type(input, type_mappings)?
        } else {
            // Default return type is void
            SigType::Alias("void".to_string())
        };

        Ok(Signature::Method(MethodSignature {
            parameters,
            return_type,
        }))
    } else {
        // Field signature: just a type
        let field_type = parse_type(input, type_mappings)?;
        Ok(Signature::Field(FieldSignature { field_type }))
    }
}

/// Parse a raw JNI signature string into a Signature
/// This parses strings like "(ILjava/lang/String;)V" or "[I" or "Ljava/lang/String;"
fn parse_raw_jni_signature(sig: &str) -> Result<Signature> {
    // Check if it's a method signature (starts with '(')
    if sig.starts_with('(') {
        parse_raw_jni_method_signature(sig)
    } else {
        parse_raw_jni_field_signature(sig)
    }
}

/// Parse a raw JNI method signature string
fn parse_raw_jni_method_signature(sig: &str) -> Result<Signature> {
    if !sig.starts_with('(') {
        return Err(syn::Error::new(
            Span::call_site(),
            format!("Method signature must start with '(': '{}'", sig),
        ));
    }

    // Find the closing ')'
    let close_paren = sig.find(')').ok_or_else(|| {
        syn::Error::new(
            Span::call_site(),
            format!("Method signature missing closing ')': '{}'", sig),
        )
    })?;

    // Parse arguments
    let args_str = &sig[1..close_paren];
    let mut args = Vec::new();
    let mut i = 0;
    let args_bytes = args_str.as_bytes();

    while i < args_bytes.len() {
        let (java_type, consumed) = parse_raw_jni_type(&args_str[i..])?;
        args.push(java_type);
        i += consumed;
    }

    // Parse return type
    let ret_str = &sig[close_paren + 1..];
    if ret_str.is_empty() {
        return Err(syn::Error::new(
            Span::call_site(),
            format!("Method signature missing return type: '{}'", sig),
        ));
    }

    let (ret, consumed) = parse_raw_jni_type(ret_str)?;
    if consumed != ret_str.len() {
        return Err(syn::Error::new(
            Span::call_site(),
            format!(
                "Trailing input: '{}' while parsing '{}'",
                &ret_str[consumed..],
                sig
            ),
        ));
    }

    // Create dummy parameters with generated names
    let parameters = args
        .into_iter()
        .enumerate()
        .map(|(i, ty)| Parameter {
            name: Ident::new(&format!("arg{}", i), Span::call_site()),
            ty,
        })
        .collect();

    Ok(Signature::Method(MethodSignature {
        parameters,
        return_type: ret,
    }))
}

/// Parse a raw JNI field signature string
fn parse_raw_jni_field_signature(sig: &str) -> Result<Signature> {
    let (field_type, consumed) = parse_raw_jni_type(sig)?;
    if consumed != sig.len() {
        return Err(syn::Error::new(
            Span::call_site(),
            format!(
                "Trailing input: '{}' while parsing '{}'",
                &sig[consumed..],
                sig
            ),
        ));
    }

    // Validate that void cannot be used as a field type
    if matches!(field_type, SigType::Alias(ref name) if name == "void") {
        return Err(syn::Error::new(
            Span::call_site(),
            format!(
                "void cannot be used as a field type in signature: '{}'",
                sig
            ),
        ));
    }

    Ok(Signature::Field(FieldSignature { field_type }))
}

/// Parse a single JNI type descriptor from a string
/// Returns the JavaType and the number of bytes consumed
fn parse_raw_jni_type(s: &str) -> Result<(SigType, usize)> {
    let bytes = s.as_bytes();
    if bytes.is_empty() {
        return Err(syn::Error::new(
            Span::call_site(),
            "Expected type descriptor but found empty string",
        ));
    }

    match bytes[0] as char {
        'Z' => Ok((SigType::Alias("jboolean".to_string()), 1)),
        'B' => Ok((SigType::Alias("jbyte".to_string()), 1)),
        'C' => Ok((SigType::Alias("jchar".to_string()), 1)),
        'S' => Ok((SigType::Alias("jshort".to_string()), 1)),
        'I' => Ok((SigType::Alias("jint".to_string()), 1)),
        'J' => Ok((SigType::Alias("jlong".to_string()), 1)),
        'F' => Ok((SigType::Alias("jfloat".to_string()), 1)),
        'D' => Ok((SigType::Alias("jdouble".to_string()), 1)),
        'V' => Ok((SigType::Alias("void".to_string()), 1)),
        '[' => {
            // Array type - parse element type recursively
            let (elem_type, elem_consumed) = parse_raw_jni_type(&s[1..])?;

            // Validate that void cannot be used as an array element type
            if matches!(elem_type, SigType::Alias(ref name) if name == "void") {
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!(
                        "void cannot be used as an array element type in signature: '{}'",
                        s
                    ),
                ));
            }

            Ok((SigType::Array(Box::new(elem_type), 1), 1 + elem_consumed))
        }
        'L' => {
            // Object type - find the semicolon
            let end = s.find(';').ok_or_else(|| {
                syn::Error::new(
                    Span::call_site(),
                    format!("Object type missing closing ';': '{}'", s),
                )
            })?;

            let class_name = &s[1..end];

            // Validate class name - should not be empty and should not start/end with '/'
            if class_name.is_empty() {
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!("Empty class name in object type: '{}'", s),
                ));
            }
            if class_name.starts_with('/') {
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!("Class name cannot start with '/': '{}'", s),
                ));
            }
            if class_name.ends_with('/') {
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!("Class name cannot end with '/': '{}'", s),
                ));
            }

            // Validate that name segments are non-empty
            if class_name.contains("//") {
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!("Class name contains empty segment: '{}'", s),
                ));
            }

            // Parse the class name (could be "java/lang/String" or "java/lang/Outer$Inner")
            // Split by '/' for package segments
            let parts: Vec<&str> = class_name.split('/').collect();

            // The last part is the class name (may contain '$' for inner classes)
            let class = parts.last().unwrap().to_string();

            // Everything before the last part is the package
            let package: Vec<String> = parts[..parts.len() - 1]
                .iter()
                .map(|s| s.to_string())
                .collect();

            let java_class = JavaClassName { package, class };

            Ok((SigType::Object(java_class), end + 1))
        }
        c => Err(syn::Error::new(
            Span::call_site(),
            format!("Invalid type descriptor: '{}'", c),
        )),
    }
}

/// Convert a JavaType to a TokenStream that constructs jni::signature::JavaType
fn java_type_to_tokens(ty: &SigType, type_mappings: &TypeMappings) -> Result<TokenStream> {
    let jni = type_mappings.jni_crate();

    match ty {
        SigType::Alias(name) => match type_mappings.map_alias(name) {
            Some(ConcreteType::Object { .. }) => Ok(quote! {
                #jni::signature::JavaType::Object
            }),
            Some(ConcreteType::Primitive { primitive, .. }) => {
                let variant = match primitive {
                    PrimitiveType::Boolean => quote! { Boolean },
                    PrimitiveType::Byte => quote! { Byte },
                    PrimitiveType::Char => quote! { Char },
                    PrimitiveType::Short => quote! { Short },
                    PrimitiveType::Int => quote! { Int },
                    PrimitiveType::Long => quote! { Long },
                    PrimitiveType::Float => quote! { Float },
                    PrimitiveType::Double => quote! { Double },
                    PrimitiveType::Void => quote! { Void },
                };
                Ok(quote! {
                    #jni::signature::JavaType::Primitive(#jni::signature::Primitive::#variant)
                })
            }
            None => Err(syn::Error::new(
                Span::call_site(),
                format!("Unknown type '{}'", name),
            )),
        },
        SigType::Object(_) => Ok(quote! {
            #jni::signature::JavaType::Object
        }),
        SigType::Array(_, _) => {
            // For arrays, jni::signature::JavaType just uses the Array variant
            // The dimensionality is encoded in the signature string
            Ok(quote! {
                #jni::signature::JavaType::Array
            })
        }
    }
}

/// The actual procedural macro implementation for jni_sig!
/// Returns a jni::signature::{MethodSignature, FieldSignature}
pub fn jni_sig_impl(input: TokenStream) -> Result<TokenStream> {
    let SignatureInput {
        signature,
        type_mappings,
    } = syn::parse2(input)?;

    // Get the jni crate path
    let jni = type_mappings.jni_crate();

    // Generate the JNI signature string
    let jni_sig = signature.to_jni_signature(&type_mappings)?;

    // Create a C string literal for the signature with MUTF-8 encoding
    let sig_cstr = lit_cstr_mutf8(&jni_sig);

    // Generate the appropriate jni::signature type
    match signature {
        Signature::Method(method_sig) => {
            // Generate the arguments array
            let args_tokens: Vec<TokenStream> = method_sig
                .parameters
                .iter()
                .map(|param| java_type_to_tokens(&param.ty, &type_mappings))
                .collect::<Result<Vec<_>>>()?;

            // Generate the return type
            let ret_token = java_type_to_tokens(&method_sig.return_type, &type_mappings)?;

            Ok(quote! {
                {
                    // Safety: The signature was parsed and validated at compile-time
                    unsafe {
                        #jni::signature::MethodSignature::from_raw_parts(
                            #jni::strings::JNIStr::from_cstr_unchecked(#sig_cstr),
                            &[#(#args_tokens),*],
                            #ret_token
                        )
                    }
                }
            })
        }
        Signature::Field(field_sig) => {
            // Generate the field type
            let ty_token = java_type_to_tokens(&field_sig.field_type, &type_mappings)?;

            Ok(quote! {
                {
                    // Safety: The signature was parsed and validated at compile-time
                    unsafe {
                        #jni::signature::FieldSignature::from_raw_parts(
                            #jni::strings::JNIStr::from_cstr_unchecked(#sig_cstr),
                            #ty_token
                        )
                    }
                }
            })
        }
    }
}

/// Implementation for jni_sig_str! macro
/// Returns a &str string literal
pub fn jni_sig_str_impl(input: TokenStream) -> Result<TokenStream> {
    let SignatureInput {
        signature,
        type_mappings,
    } = syn::parse2(input)?;

    // Generate the JNI signature string
    let jni_sig = signature.to_jni_signature(&type_mappings)?;

    Ok(quote! {
        #jni_sig
    })
}

/// Implementation for jni_sig_cstr! macro
/// Returns a CStr literal (c"...") with MUTF-8 encoding
pub fn jni_sig_cstr_impl(input: TokenStream) -> Result<TokenStream> {
    let SignatureInput {
        signature,
        type_mappings,
    } = syn::parse2(input)?;

    // Generate the JNI signature string
    let jni_sig = signature.to_jni_signature(&type_mappings)?;

    // Create a C string literal with MUTF-8 encoding
    let sig_cstr = lit_cstr_mutf8(&jni_sig);

    Ok(quote! {
        #sig_cstr
    })
}

/// Implementation for jni_sig_jstr! macro
/// Returns a &'static JNIStr with MUTF-8 encoding
pub fn jni_sig_jstr_impl(input: TokenStream) -> Result<TokenStream> {
    let SignatureInput {
        signature,
        type_mappings,
    } = syn::parse2(input)?;

    // Get the jni crate path
    let jni = type_mappings.jni_crate();

    // Generate the JNI signature string
    let jni_sig = signature.to_jni_signature(&type_mappings)?;

    // Create a C string literal with MUTF-8 encoding
    let sig_cstr = lit_cstr_mutf8(&jni_sig);

    Ok(quote! {
        {
            // Safety: The signature was parsed and validated at compile-time
            unsafe {
                #jni::strings::JNIStr::from_cstr_unchecked(#sig_cstr)
            }
        }
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use quote::quote;

    // Test parsing error: single identifier without dot should fail
    #[test]
    fn test_single_ident_fails_as_java_class() {
        // This should be parsed as a Rust type reference, not a Java class
        let input = quote! {
            String
        };

        let result = jni_sig_impl(input);
        // Should fail because "String" without a dot is treated as a Rust type
        // and there's no type mapping for it
        assert!(result.is_err());
    }

    // Test that slash-separated package names fail with helpful error
    // (Users should use dot-separated Java format, not JNI internal format)
    #[test]
    fn test_slash_separated_package_fails() {
        // This should fail because we expect Java dotted format (java.lang.String)
        // not JNI internal format (java/lang/String)
        let input = quote! {
            java/lang/String
        };

        let result = jni_sig_impl(input);
        // The parser will treat this as a division operation and fail to parse
        assert!(result.is_err());

        // Verify the error is about parsing, not about finding the type
        let err = result.unwrap_err();
        let err_msg = err.to_string();
        // The error should be a parse error, not a "type not found" error
        assert!(!err_msg.contains("No type mapping found"));
    }

    // Test that string literals don't support slash separators either
    #[test]
    fn test_string_literal_with_slashes_in_type_mapping() {
        // Even in string literals, we expect Java dotted format
        let input = quote! {
            MyCustomType,
            type_map = {
                MyCustomType => "java/lang/String",
            }
        };

        // This should fail during parsing of the type mapping
        // String literals with slashes should be rejected
        let result = jni_sig_impl(input);
        assert!(result.is_err());

        // The error should mention that dots are required
        let err = result.unwrap_err();
        let err_msg = err.to_string();
        assert!(err_msg.contains("must contain at least one dot"));
    }

    // Test that the parser requires at least one dot to differentiate from Rust types
    #[test]
    fn test_no_dots_fails_for_java_class() {
        // Without dots, this should be treated as a Rust type reference
        let input = quote! {
            (arg: SomeType) -> void
        };

        let result = jni_sig_impl(input);
        // Should fail with "Unknown type" after attempting to resolve SomeType
        // via type mappings
        assert!(result.is_err());
        let err = result.unwrap_err();
        let err_msg = err.to_string();
        assert!(err_msg.contains("Unknown type"));
    }

    // Test that string literal in JavaClassName::parse requires dots
    #[test]
    fn test_string_literal_without_dots_fails() {
        let input = quote! { "String" };
        let result = syn::parse2::<JavaClassName>(input);

        assert!(result.is_err());
        let err = result.unwrap_err();
        let err_msg = err.to_string();
        assert!(err_msg.contains("must contain at least one dot"));
    }

    // Test that JavaClassName::parse rejects single identifiers without consuming them
    #[test]
    fn test_parse_java_class_name_rejects_single_ident() {
        // A single identifier without a dot should fail to parse as a Java class name
        let input = quote! { SomeType };
        let result = syn::parse2::<JavaClassName>(input);

        assert!(result.is_err());
        let err = result.unwrap_err();
        let err_msg = err.to_string();
        assert!(err_msg.contains("Expected Java class name"));
    }

    // ============================================================================
    // Raw JNI signature tests
    // ============================================================================

    #[test]
    fn test_raw_jni_field_primitives() {
        // Test all primitive types
        let primitives = [
            ("Z", "Z", "Boolean"),
            ("B", "B", "Byte"),
            ("C", "C", "Char"),
            ("S", "S", "Short"),
            ("I", "I", "Int"),
            ("J", "J", "Long"),
            ("F", "F", "Float"),
            ("D", "D", "Double"),
        ];

        for (input_sig, expected_sig, expected_type) in primitives {
            let input = quote! { #input_sig };
            let result = jni_sig_impl(input).unwrap();
            let result_str = result.to_string();
            assert!(
                result_str.contains(&format!("\"{}\"", expected_sig)),
                "Expected signature '{}' for input '{}'",
                expected_sig,
                input_sig
            );
            assert!(
                result_str.contains(&format!("Primitive :: {}", expected_type)),
                "Expected primitive type '{}' for input '{}'",
                expected_type,
                input_sig
            );
            assert!(
                result_str.contains("FieldSignature"),
                "Expected FieldSignature output for input '{}'",
                input_sig
            );
        }
    }

    #[test]
    fn test_raw_jni_errors_empty_string() {
        let input = quote! { "" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_invalid_primitive() {
        let input = quote! { "A" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_void_field() {
        let input = quote! { "V" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_void_array_in_method_arguments() {
        // Void arrays are invalid in method arguments
        let input = quote! { "([V)V" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());

        let input = quote! { "(I[V)V" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());

        let input = quote! { "([VI)V" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_void_array_as_return_type() {
        // Void arrays are invalid as return types
        let input = quote! { "()[V" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());

        let input = quote! { "(I)[V" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_void_array_field() {
        // Void arrays are invalid as field types
        let input = quote! { "[V" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_incomplete_method() {
        let input = quote! { "(I" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_method_no_return() {
        let input = quote! { "(I)" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_trailing_input() {
        let input = quote! { "II" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_invalid_in_method() {
        let input = quote! { "(Invalid)I" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_object_no_semicolon() {
        let input = quote! { "Ljava/lang/String" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_object_empty_name() {
        let input = quote! { "L;" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_errors_leading_slash() {
        let input = quote! { "L/java/lang/String;" };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_raw_jni_string_with_dots_as_field() {
        // String literal with dots should be treated as Java class name for field
        let input = quote! { "java.lang.String" };
        let result = jni_sig_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("\"Ljava/lang/String;\""));
        assert!(result_str.contains("FieldSignature :: from_raw_parts"));
    }

    #[test]
    fn test_raw_jni_inner_classes() {
        // Test inner classes with $ separator
        let input = quote! { "Ljava/lang/Outer$Inner;" };
        let result = jni_sig_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("\"Ljava/lang/Outer$Inner;\""));
    }

    #[test]
    fn test_raw_jni_method_with_inner_class() {
        let input = quote! { "(Ljava/lang/Outer$Inner;)V" };
        let result = jni_sig_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("\"(Ljava/lang/Outer$Inner;)V\""));
    }

    #[test]
    fn test_raw_jni_default_package_class() {
        // Default package class (no package)
        let input = quote! { "LNoPackage;" };
        let result = jni_sig_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("\"LNoPackage;\""));
    }

    #[test]
    fn test_raw_jni_multidimensional_array() {
        let input = quote! { "[[I" };
        let result = jni_sig_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("\"[[I\""));
    }

    #[test]
    fn test_raw_jni_method_returns_array() {
        let input = quote! { "()[[I" };
        let result = jni_sig_impl(input).unwrap();
        let result_str = result.to_string();
        assert!(result_str.contains("\"()[[I\""));
    }

    #[test]
    fn test_non_raw_void_array_prefix_syntax() {
        // Test void array with prefix syntax [void]
        let input = quote! { [void] };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_non_raw_void_array_suffix_syntax() {
        // Test void array with suffix syntax void[]
        let input = quote! { void[] };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_non_raw_void_array_in_method_args() {
        // Test void array in method arguments
        let input = quote! { (arg: [void]) -> void };
        let result = jni_sig_impl(input);
        assert!(result.is_err());

        let input = quote! { (arg: void[]) -> void };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_non_raw_void_array_as_return_type() {
        // Test void array as return type
        let input = quote! { () -> [void] };
        let result = jni_sig_impl(input);
        assert!(result.is_err());

        let input = quote! { () -> void[] };
        let result = jni_sig_impl(input);
        assert!(result.is_err());
    }

    #[test]
    fn test_cstr_literal_invalid_utf8() {
        // CStr literals can contain byte escapes that create invalid UTF-8 sequences
        // For example, \xFF is not valid UTF-8
        // This tests that our code properly rejects such literals with a clear error message

        // Create a CStr literal with invalid UTF-8 using byte escapes
        // \xFF is not a valid UTF-8 byte sequence
        let input = quote! { c"(\xFF)V" };
        let result = jni_sig_impl(input);

        // Should fail with a UTF-8 validation error
        assert!(
            result.is_err(),
            "CStr literal with invalid UTF-8 should be rejected"
        );

        let err = result.unwrap_err();
        let err_msg = err.to_string();
        assert!(
            err_msg.contains("CStr literal must contain valid UTF-8"),
            "Error message should mention UTF-8 validation, got: {}",
            err_msg
        );

        // Also test that valid UTF-8 in CStr literals works correctly
        let input = quote! { c"(I)V" };
        let result = jni_sig_impl(input);
        assert!(
            result.is_ok(),
            "Valid UTF-8 CStr literal should parse successfully"
        );

        let result_str = result.unwrap().to_string();
        assert!(
            result_str.contains("\"(I)V\""),
            "Should contain the signature string"
        );
    }
}
