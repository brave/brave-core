//! Procedural macro for binding Java classes to Rust types
//!
//! This module provides a `bind_java_type!` macro that generates Reference type wrappers
//! and API bindings for Java classes, including constructors, methods, fields, and
//! native method registration.

use proc_macro2::{Span, TokenStream};
use quote::{format_ident, quote};
use syn::{
    Ident, LitBool, LitStr, Result, Token, braced, custom_keyword,
    ext::IdentExt,
    parenthesized,
    parse::{Parse, ParseStream},
};

use crate::{
    mangle::{create_jni_fn_name, snake_case_to_lower_camel_case},
    native_method::{NativeMethodExport, generate_native_method_abi_check},
    signature::{parse_field_sig, sig},
    types::{AbiCheck, generate_type_mapping_checks, sig_type_to_rust_type_core},
};
use crate::{
    signature::parse_method_sig,
    types::{JavaClassName, PrimitiveType, SigType, TypeMappings, parse_type},
};
use crate::{
    signature::{FieldSignature, MethodSignature, parse_parameter_with_index},
    types::path_to_string_no_spaces,
};
use crate::{str::lit_cstr_mutf8, types::ConcreteType};

// Custom keywords
custom_keyword!(jni);
custom_keyword!(rust_type);
custom_keyword!(rust_type_vis);
custom_keyword!(java_type);
custom_keyword!(type_map);
custom_keyword!(api);
custom_keyword!(native_trait);
custom_keyword!(constructors);
custom_keyword!(methods);
custom_keyword!(native_methods);
custom_keyword!(fields);
custom_keyword!(hooks);
custom_keyword!(name);
custom_keyword!(get);
custom_keyword!(set);
custom_keyword!(error);
custom_keyword!(native_methods_error_policy);
custom_keyword!(error_policy);
custom_keyword!(abi_check);
custom_keyword!(native_methods_export);
custom_keyword!(export);
custom_keyword!(native_methods_catch_unwind);
custom_keyword!(catch_unwind);
custom_keyword!(priv_type);
custom_keyword!(is_instance_of);
custom_keyword!(__jni_core);
custom_keyword!(raw);
custom_keyword!(non_null);

/// Represents a visibility modifier
#[derive(Clone)]
enum VisibilitySpec {
    Public,
    PubSelf,
    PubCrate,
    PubSuper,
    PubInPath(syn::Path),
}

impl std::fmt::Debug for VisibilitySpec {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            VisibilitySpec::Public => write!(f, "Public"),
            VisibilitySpec::PubSelf => write!(f, "PubSelf"),
            VisibilitySpec::PubCrate => write!(f, "PubCrate"),
            VisibilitySpec::PubSuper => write!(f, "PubSuper"),
            VisibilitySpec::PubInPath(_) => write!(f, "PubInPath(..)"),
        }
    }
}

impl VisibilitySpec {
    fn to_tokens(&self) -> TokenStream {
        match self {
            VisibilitySpec::Public => quote! { pub },
            VisibilitySpec::PubSelf => quote! { pub(self) },
            VisibilitySpec::PubCrate => quote! { pub(crate) },
            VisibilitySpec::PubSuper => quote! { pub(super) },
            VisibilitySpec::PubInPath(path) => quote! { pub(in #path) },
        }
    }
    // Get back a visibility that is at most as permissive as `other`
    //
    // This is used to clamp the visibility of the `*API` struct to the visibility of the binding
    // type itself.
    fn clamp_to(&self, other: &VisibilitySpec) -> VisibilitySpec {
        match (self, other) {
            (VisibilitySpec::Public, _) => other.clone(),
            (VisibilitySpec::PubCrate, VisibilitySpec::Public) => VisibilitySpec::PubCrate,
            (VisibilitySpec::PubCrate, _) => other.clone(),
            (VisibilitySpec::PubSuper, VisibilitySpec::Public | VisibilitySpec::PubCrate) => {
                VisibilitySpec::PubSuper
            }
            (VisibilitySpec::PubSuper, _) => other.clone(),
            (
                VisibilitySpec::PubSelf,
                VisibilitySpec::Public | VisibilitySpec::PubCrate | VisibilitySpec::PubSuper,
            ) => VisibilitySpec::PubSelf,
            (VisibilitySpec::PubSelf, _) => other.clone(),
            // XXX: it's a bit hand-wavey deciding how we should clamp `pub(in
            // path)`, but essentially we'll assume that if someone want's their
            // binding to only be visible for a specific module path then their
            // API type should also only be visible for the same path, unless
            // `other` is restricted to `pub(self)`
            (VisibilitySpec::PubInPath(_), VisibilitySpec::PubSelf) => VisibilitySpec::PubSelf,
            (VisibilitySpec::PubInPath(_), _) => self.clone(),
        }
    }
}

/// Represents a constructor definition
#[derive(Clone)]
struct Constructor {
    name: Ident,
    method_signature: MethodSignature,
    attrs: Vec<syn::Attribute>,
}

/// Represents a method definition
#[derive(Clone)]
struct Method {
    visibility: VisibilitySpec,
    java_name: String,
    rust_name: Ident,
    method_signature: MethodSignature,
    attrs: Vec<syn::Attribute>,
    is_static: bool,
    non_null: bool,
}

/// Represents a field definition
#[derive(Clone)]
struct Field {
    java_name: String,
    #[allow(dead_code)]
    rust_name: Ident,
    getter_name: Option<Ident>,
    setter_name: Option<Ident>,
    getter_visibility: VisibilitySpec,
    setter_visibility: VisibilitySpec,
    field_signature: FieldSignature,
    attrs: Vec<syn::Attribute>,
    getter_attrs: Vec<syn::Attribute>,
    setter_attrs: Vec<syn::Attribute>,
    is_static: bool,
    non_null: bool,
}

/// Represents an entry in the is_instance_of list
#[derive(Clone)]
struct IsInstanceOfEntry {
    /// The Rust type alias (e.g., "JObject" or "jni::objects::JObject")
    type_alias: String,
    /// Optional stem name for the generated as_<stem>() method
    stem: Option<String>,
}

/// Represents a native method definition
#[derive(Clone)]
struct NativeMethod {
    java_name: String,
    rust_name: Ident,
    method_signature: MethodSignature,
    error_policy: Option<syn::Path>,
    export: NativeMethodExport,
    attrs: Vec<syn::Attribute>,
    /// Optional direct function binding (bypasses trait implementation)
    native_fn: Option<syn::Path>,
    /// Whether this is a raw JNI function (takes EnvUnowned directly, no with_env wrapper)
    is_raw: bool,
    /// Whether this is a static method
    is_static: bool,
    abi_check: Option<AbiCheck>,
    catch_unwind: Option<bool>,
}

/// The kind of method being parsed (determines validation rules)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum MethodKind {
    Constructor,
    Method,
    NativeMethod,
}

/// Intermediate representation for a parsed method (after validation)
struct ParsedMethod {
    rust_name: Ident,
    java_name: String, // Always set (either explicit or derived from rust_name)
    method_signature: MethodSignature, // Always present after parsing
    visibility: Option<VisibilitySpec>,
    error_policy: Option<syn::Path>,    // Only set for NativeMethod
    export: Option<NativeMethodExport>, // Only set for NativeMethod
    attrs: Vec<syn::Attribute>,
    native_fn: Option<syn::Path>, // Only set for NativeMethod
    is_raw: bool,                 // Only set for NativeMethod
    is_static: bool,              // Set for both Method and NativeMethod
    abi_check: Option<AbiCheck>,  // Only set for NativeMethod
    catch_unwind: Option<bool>,   // Only set for NativeMethod
    non_null: bool,               // Set for Method and NativeMethod (not Constructor)
}

/// Parse an optional visibility specifier
/// Supports: pub, pub(self), pub(crate), pub(super), pub(in path), or priv (which maps to pub(self))
/// Returns None if no visibility specifier is present (defaults to Public for methods / constructors)
fn parse_visibility(input: ParseStream) -> Result<Option<VisibilitySpec>> {
    if input.peek(Token![pub]) {
        input.parse::<Token![pub]>()?;

        // Check for pub(...)
        if input.peek(syn::token::Paren) {
            let content;
            parenthesized!(content in input);

            if content.peek(Token![self]) {
                content.parse::<Token![self]>()?;
                Ok(Some(VisibilitySpec::PubSelf))
            } else if content.peek(Token![crate]) {
                content.parse::<Token![crate]>()?;
                Ok(Some(VisibilitySpec::PubCrate))
            } else if content.peek(Token![super]) {
                content.parse::<Token![super]>()?;
                Ok(Some(VisibilitySpec::PubSuper))
            } else if content.peek(Token![in]) {
                content.parse::<Token![in]>()?;
                let path = content.parse::<syn::Path>()?;
                Ok(Some(VisibilitySpec::PubInPath(path)))
            } else {
                Err(syn::Error::new(
                    content.span(),
                    "expected `self`, `crate`, `super`, or `in <path>` after `pub(`",
                ))
            }
        } else {
            Ok(Some(VisibilitySpec::Public))
        }
    } else if input.peek(Token![priv]) {
        input.parse::<Token![priv]>()?;
        Ok(Some(VisibilitySpec::PubSelf))
    } else {
        Ok(None)
    }
}

/// Parse a name = "value" or name = rust_name pair
fn parse_name_spec(input: ParseStream) -> Result<(String, Ident)> {
    input.parse::<name>()?;
    input.parse::<Token![=]>()?;

    if input.peek(LitStr) {
        let lit = input.parse::<LitStr>()?;
        let java_name = lit.value();
        let rust_name = format_ident!("{}", java_name);
        Ok((java_name, rust_name))
    } else {
        let rust_name = input.parse::<Ident>()?;
        let java_name = rust_name.to_string();
        Ok((java_name, rust_name))
    }
}

/// Unified parser for method blocks (constructors, methods, and native methods)
/// Parses both shorthand syntax: `[vis] [static] [raw] [extern] fn name(params) -> ret`
/// and block syntax: `[vis] [static] [raw] [extern] fn name [=] { ... }`
/// Ensures signature is always present and java_name is derived from rust_name if not explicit
/// Validates based on the method kind
fn parse_method(
    input: ParseStream,
    type_mappings: &TypeMappings,
    kind: MethodKind,
) -> Result<ParsedMethod> {
    // Parse attributes first
    let attrs = input.call(syn::Attribute::parse_outer)?;

    // Parse optional visibility specifier before the method name
    let visibility = parse_visibility(input)?;

    // Parse qualifiers that can appear after visibility and before the fn keyword
    // For shorthand: [vis] [static] [raw] [extern] [non_null] fn name(params) -> ret
    // For block: [vis] [static] [extern] fn name { ... }
    let mut is_static = false;
    let mut is_raw = false;
    let mut is_extern = false;
    let mut is_non_null = false;

    // Parse qualifiers in any order (static, raw, extern, non_null)
    loop {
        if input.peek(Token![static]) {
            input.parse::<Token![static]>()?;
            is_static = true;
        } else if input.peek(Token![extern]) {
            input.parse::<Token![extern]>()?;
            is_extern = true;
        } else if input.peek(raw) {
            input.parse::<raw>()?;
            is_raw = true;
        } else if input.peek(non_null) {
            input.parse::<non_null>()?;
            is_non_null = true;
        } else {
            break;
        }
    }

    // Require 'fn' keyword for both shorthand and block syntax
    if !input.peek(Token![fn]) {
        return Err(syn::Error::new(
            input.span(),
            "Expected 'fn' keyword before method name",
        ));
    }
    input.parse::<Token![fn]>()?;

    let rust_name = input.parse::<Ident>()?;

    let mut java_name = None;
    let mut method_signature = None;
    let mut error_policy = None;
    let mut export = None;
    let mut native_fn = None;
    let mut abi_check = None;
    let mut catch_unwind = None;

    // Check if this is shorthand syntax (parentheses) or block syntax (braces)
    if input.peek(syn::token::Paren) {
        // Shorthand syntax: [vis] [qual..] fn name(params) [-> return_type]
        let sig_content;
        parenthesized!(sig_content in input);

        let mut parameters = Vec::new();
        let mut param_index = 0;
        while !sig_content.is_empty() {
            let param = parse_parameter_with_index(&sig_content, param_index, type_mappings)?;
            parameters.push(param);
            param_index += 1;

            if !sig_content.is_empty() {
                sig_content.parse::<Token![,]>()?;
            }
        }

        // Check if there's a return type (-> Type)
        // Constructors don't have return types, methods do
        let return_type = if input.peek(Token![->]) {
            input.parse::<Token![->]>()?;
            parse_type(input, type_mappings)?
        } else {
            // No return type means this is constructor shorthand - use void
            SigType::Alias("void".to_string())
        };

        method_signature = Some(MethodSignature {
            parameters,
            return_type,
        });
    } else if input.peek(syn::token::Brace) || input.peek(Token![=]) {
        // Block syntax: [vis] [qual..] fn name [=] { name = "javaName", sig = (params) -> ret, ... }

        if input.peek(Token![=]) {
            input.parse::<Token![=]>()?;
        }

        let body_content;
        braced!(body_content in input);

        while !body_content.is_empty() {
            let lookahead = body_content.lookahead1();

            if lookahead.peek(name) {
                let (jname, _) = parse_name_spec(&body_content)?;
                java_name = Some(jname);
            } else if lookahead.peek(sig) {
                method_signature = Some(parse_method_sig(&body_content, type_mappings)?);
            } else if lookahead.peek(Token![fn]) {
                // Parse: fn = path
                body_content.parse::<Token![fn]>()?;
                body_content.parse::<Token![=]>()?;
                let path = body_content.parse::<syn::Path>()?;
                native_fn = Some(path);
            } else if lookahead.peek(Token![static]) {
                body_content.parse::<Token![static]>()?;
                body_content.parse::<Token![=]>()?;
                let lit_bool = body_content.parse::<LitBool>()?;
                is_static = lit_bool.value();
            } else if lookahead.peek(raw) {
                body_content.parse::<raw>()?;
                body_content.parse::<Token![=]>()?;
                let lit_bool = body_content.parse::<LitBool>()?;
                is_raw = lit_bool.value();
            } else if lookahead.peek(self::error_policy) {
                body_content.parse::<error_policy>()?;
                body_content.parse::<Token![=]>()?;
                error_policy = Some(body_content.parse::<syn::Path>()?);
            } else if lookahead.peek(self::export) {
                body_content.parse::<export>()?;
                body_content.parse::<Token![=]>()?;
                if body_content.peek(LitStr) {
                    // export = "customName"
                    let name = body_content.parse::<LitStr>()?.value();
                    export = Some(NativeMethodExport::WithName(name));
                } else if body_content.peek(LitBool) {
                    // export = true or export = false
                    let lit_bool = body_content.parse::<LitBool>()?;
                    export = Some(if lit_bool.value() {
                        NativeMethodExport::WithAutoMangle
                    } else {
                        NativeMethodExport::No
                    });
                } else {
                    return Err(syn::Error::new(
                        body_content.span(),
                        "export must be 'true', 'false', or a string literal",
                    ));
                }
            } else if lookahead.peek(self::abi_check) {
                body_content.parse::<abi_check>()?;
                body_content.parse::<Token![=]>()?;
                abi_check = Some(body_content.parse::<AbiCheck>()?);
            } else if lookahead.peek(self::catch_unwind) {
                body_content.parse::<catch_unwind>()?;
                body_content.parse::<Token![=]>()?;
                let lit_bool = body_content.parse::<LitBool>()?;
                catch_unwind = Some(lit_bool.value());
            } else if lookahead.peek(self::non_null) {
                body_content.parse::<non_null>()?;
                body_content.parse::<Token![=]>()?;
                let lit_bool = body_content.parse::<LitBool>()?;
                is_non_null = lit_bool.value();
            } else {
                return Err(lookahead.error());
            }

            // Require comma between properties, but trailing comma is optional
            if !body_content.is_empty() {
                body_content.parse::<Token![,]>()?;
            }
        }
    } else {
        return Err(syn::Error::new(
            rust_name.span(),
            "Method must be followed by parentheses (shorthand syntax) or braces (block syntax)",
        ));
    }

    // If extern was specified before the name, it means export = true
    if is_extern {
        if export.is_some() {
            return Err(syn::Error::new(
                rust_name.span(),
                "Cannot specify both 'extern' qualifier and 'export' property",
            ));
        }
        export = Some(NativeMethodExport::WithAutoMangle);
    }

    // Require signature - all methods must have one
    let method_signature = method_signature
        .ok_or_else(|| syn::Error::new(rust_name.span(), "Method must have a signature"))?;

    // Track whether java_name was explicitly overridden
    let java_name_overridden = java_name.is_some();

    // Set java_name: use explicit name if provided, otherwise convert from rust_name
    let java_name =
        java_name.unwrap_or_else(|| snake_case_to_lower_camel_case(&rust_name.to_string()));

    // Perform kind-specific validation
    match kind {
        MethodKind::Constructor => {
            // Constructors cannot have explicit java name (it's always "<init>")
            if java_name_overridden {
                return Err(syn::Error::new(
                    rust_name.span(),
                    "Cannot override 'name' for constructors - the Java constructor name is always '<init>'",
                ));
            }

            // Constructors must return void
            if !matches!(
                method_signature.return_type,
                SigType::Alias(ref name) if name == "void"
            ) {
                return Err(syn::Error::new(
                    rust_name.span(),
                    "Constructors must return void - either omit '-> type' or use explicit '-> void' or '-> ()'",
                ));
            }

            // Constructors cannot be static
            if is_static {
                return Err(syn::Error::new(
                    rust_name.span(),
                    "Constructors cannot be static",
                ));
            }

            // Constructors cannot use non_null
            if is_non_null {
                return Err(syn::Error::new(
                    rust_name.span(),
                    "Constructors cannot use 'non_null' - use it on methods that can return null instead",
                ));
            }
        }
        MethodKind::Method => {
            // See !NativeMethod validation below
        }
        MethodKind::NativeMethod => {
            // Validate that raw methods and error_policy are not both specified
            // error_policy only applies when we wrap with with_env
            if is_raw {
                if error_policy.is_some() {
                    return Err(syn::Error::new(
                        rust_name.span(),
                        "Cannot specify 'error_policy' when using 'raw' - error_policy only applies to safe wrappers that use with_env",
                    ));
                }
                if catch_unwind.is_some() {
                    return Err(syn::Error::new(
                        rust_name.span(),
                        "Cannot specify 'catch_unwind' when using 'raw' - catch_unwind only applies to safe wrappers",
                    ));
                }
                if is_non_null {
                    return Err(syn::Error::new(
                        rust_name.span(),
                        "Cannot specify 'non_null' when using 'raw' - non_null only applies to safe wrappers that return Result",
                    ));
                }
            }
        }
    }

    // Common validation for non-native methods
    if kind != MethodKind::NativeMethod {
        // Only native methods can have error_policy types
        if error_policy.is_some() {
            return Err(syn::Error::new(
                rust_name.span(),
                "Only native methods can have 'error_policy'",
            ));
        }

        // Only native methods can have export
        if export.is_some() {
            return Err(syn::Error::new(
                rust_name.span(),
                "Only native methods can have extern qualifier or 'export =' property",
            ));
        }

        // Only native methods can have native_fn
        if native_fn.is_some() {
            return Err(syn::Error::new(
                rust_name.span(),
                "Only native methods can have 'fn'",
            ));
        }

        // Only native methods can have raw
        if is_raw {
            return Err(syn::Error::new(
                rust_name.span(),
                "Only native methods can have 'raw'",
            ));
        }

        if abi_check.is_some() {
            return Err(syn::Error::new(
                rust_name.span(),
                "Only native methods can have 'abi_check'",
            ));
        }

        if catch_unwind.is_some() {
            return Err(syn::Error::new(
                rust_name.span(),
                "Only native methods can have 'catch_unwind'",
            ));
        }
    }

    // Validate non_null usage
    if is_non_null {
        // Check if the return type is a primitive or void
        let is_primitive_or_void = method_signature
            .return_type
            .try_as_primitive(type_mappings)
            .is_some()
            || matches!(
                method_signature.return_type,
                SigType::Alias(ref name) if name == "void"
            );

        if is_primitive_or_void {
            return Err(syn::Error::new(
                rust_name.span(),
                "Cannot use 'non_null' with methods that return primitive types or void - non_null is only valid for object/reference return types",
            ));
        }
    }

    Ok(ParsedMethod {
        rust_name,
        java_name,
        method_signature,
        visibility,
        error_policy,
        export,
        attrs,
        native_fn,
        is_raw,
        is_static,
        abi_check,
        catch_unwind,
        non_null: is_non_null,
    })
}

/// Parse constructors block
fn parse_constructors(
    input: ParseStream,
    type_mappings: &TypeMappings,
) -> Result<Vec<Constructor>> {
    let mut constructors = Vec::new();

    while !input.is_empty() {
        let parsed = parse_method(input, type_mappings, MethodKind::Constructor)?;

        constructors.push(Constructor {
            name: parsed.rust_name,
            method_signature: parsed.method_signature,
            attrs: parsed.attrs,
        });

        if input.peek(Token![,]) {
            input.parse::<Token![,]>()?;
        }
    }

    Ok(constructors)
}

/// Parse methods block
fn parse_methods(input: ParseStream, type_mappings: &TypeMappings) -> Result<Vec<Method>> {
    let mut methods = Vec::new();

    while !input.is_empty() {
        let parsed = parse_method(input, type_mappings, MethodKind::Method)?;

        methods.push(Method {
            visibility: parsed.visibility.unwrap_or(VisibilitySpec::Public),
            java_name: parsed.java_name,
            rust_name: parsed.rust_name,
            method_signature: parsed.method_signature,
            attrs: parsed.attrs,
            is_static: parsed.is_static,
            non_null: parsed.non_null,
        });

        if !input.is_empty() {
            input.parse::<Token![,]>()?;
        }
    }

    Ok(methods)
}

/// Parse native_methods block
fn parse_native_methods(
    input: ParseStream,
    type_mappings: &TypeMappings,
) -> Result<(Vec<NativeMethod>, Vec<Method>)> {
    let mut native_methods = Vec::new();
    let mut methods = Vec::new();

    while !input.is_empty() {
        let parsed = parse_method(input, type_mappings, MethodKind::NativeMethod)?;

        if let Some(visibility) = parsed.visibility {
            methods.push(Method {
                visibility,
                java_name: parsed.java_name.clone(),
                rust_name: parsed.rust_name.clone(),
                method_signature: parsed.method_signature.clone(),
                attrs: parsed.attrs.clone(),
                is_static: parsed.is_static,
                non_null: parsed.non_null,
            });
        }
        native_methods.push(NativeMethod {
            java_name: parsed.java_name,
            rust_name: parsed.rust_name,
            method_signature: parsed.method_signature,
            error_policy: parsed.error_policy,
            export: parsed.export.unwrap_or(NativeMethodExport::Default),
            attrs: parsed.attrs,
            native_fn: parsed.native_fn,
            is_raw: parsed.is_raw,
            is_static: parsed.is_static,
            abi_check: parsed.abi_check,
            catch_unwind: parsed.catch_unwind,
        });

        if !input.is_empty() {
            input.parse::<Token![,]>()?;
        }
    }

    Ok((native_methods, methods))
}

/// Parse fields or static_fields block
fn parse_field(input: ParseStream, type_mappings: &TypeMappings) -> Result<Field> {
    // Parse attributes first (for the field itself)
    let field_attrs = input.call(syn::Attribute::parse_outer)?;

    // Parse optional visibility specifier before the field name
    let field_visibility = parse_visibility(input)?;
    // Use field_visibility for both getter and setter, defaulting to Public
    let mut getter_visibility = field_visibility.clone().unwrap_or(VisibilitySpec::Public);
    let mut setter_visibility = field_visibility.unwrap_or(VisibilitySpec::Public);

    // Parse qualifiers that can appear after visibility and before the field name
    // For shorthand: [vis] [static] [non_null] name: Type
    // For block: [vis] [static] name { ... }
    let mut is_static = false;
    let mut is_non_null = false;

    // Parse qualifiers in any order (static, non_null)
    loop {
        if input.peek(Token![static]) {
            input.parse::<Token![static]>()?;
            is_static = true;
        } else if input.peek(non_null) {
            input.parse::<non_null>()?;
            is_non_null = true;
        } else {
            break;
        }
    }

    let rust_name = input.parse::<Ident>()?;

    let default_getter_name = rust_name.clone();

    // If the rust name has any leading underscores, keep them as a prefix for the setter.
    // A field named `_my_field` would have a setter named `_set_my_field` instead of `set__my_field`

    let rust_name_str = rust_name.to_string();
    let n_underscores = rust_name_str.chars().take_while(|c| *c == '_').count();

    let default_setter_name = format_ident!(
        "{}set_{}",
        "_".repeat(n_underscores),
        &rust_name_str[n_underscores..]
    );

    // Automatically convert rust_name from snake_case to lowerCamelCase
    let mut java_name = snake_case_to_lower_camel_case(&rust_name.to_string());

    // Check if this is shorthand syntax (colon) or block syntax (braces)
    if input.peek(Token![:]) {
        // Shorthand syntax: field_name: Type
        input.parse::<Token![:]>()?;
        let field_type = parse_type(input, type_mappings)?;

        let field_signature = FieldSignature { field_type };

        // Validate non_null usage
        if is_non_null {
            let is_primitive = field_signature
                .field_type
                .try_as_primitive(type_mappings)
                .is_some();
            if is_primitive {
                return Err(syn::Error::new(
                    rust_name.span(),
                    "Cannot use 'non_null' with fields of primitive types - non_null is only valid for object/reference field types",
                ));
            }
        }

        Ok(Field {
            java_name,
            rust_name,
            getter_name: Some(default_getter_name),
            setter_name: Some(default_setter_name),
            getter_visibility,
            setter_visibility,
            field_signature,
            attrs: field_attrs,
            getter_attrs: Vec::new(),
            setter_attrs: Vec::new(),
            is_static,
            non_null: is_non_null,
        })
    } else if input.peek(syn::token::Brace) || input.peek(Token![=]) {
        // Block syntax: field_name [=] { name = "javaName", sig = Type, ... }

        if input.peek(Token![=]) {
            input.parse::<Token![=]>()?;
        }

        let body_content;
        braced!(body_content in input);

        let mut field_signature = None;
        let mut getter_attrs = vec![];
        let mut setter_attrs = vec![];
        let mut getter_override_name = None;
        let mut setter_override_name = None;

        while !body_content.is_empty() {
            let prop_attrs = body_content.call(syn::Attribute::parse_outer)?;

            // Try to parse visibility first (handles pub, pub(...), and priv)
            let vis = parse_visibility(&body_content)?;

            // After parsing visibility (if any), check what follows
            if body_content.peek(get) {
                getter_attrs.extend(prop_attrs);
                if let Some(vis) = vis {
                    getter_visibility = vis;
                }
                body_content.parse::<get>()?;
                body_content.parse::<Token![=]>()?;
                getter_override_name = Some(body_content.parse::<Ident>()?);
            } else if body_content.peek(set) {
                setter_attrs.extend(prop_attrs);
                if let Some(vis) = vis {
                    setter_visibility = vis;
                }
                body_content.parse::<set>()?;
                body_content.parse::<Token![=]>()?;
                setter_override_name = Some(body_content.parse::<Ident>()?);
            } else if vis.is_some() {
                // We parsed a visibility but it's not followed by get/set
                return Err(syn::Error::new(
                    body_content.span(),
                    "Visibility specifier must be followed by 'get' or 'set'",
                ));
            } else if !prop_attrs.is_empty() {
                // We have buffered attributes but didn't find get/set
                return Err(syn::Error::new(
                    body_content.span(),
                    "Field property attributes must be followed by 'get' or 'set'",
                ));
            } else {
                // No visibility, no attributes, not get/set - check for other properties
                let lookahead = body_content.lookahead1();

                if lookahead.peek(name) {
                    let (jname, _) = parse_name_spec(&body_content)?;
                    java_name = jname;
                } else if lookahead.peek(sig) {
                    field_signature = Some(parse_field_sig(&body_content, type_mappings)?);
                } else if lookahead.peek(Token![static]) {
                    body_content.parse::<Token![static]>()?;
                    body_content.parse::<Token![=]>()?;
                    let lit_bool = body_content.parse::<LitBool>()?;
                    is_static = lit_bool.value();
                } else if lookahead.peek(non_null) {
                    body_content.parse::<non_null>()?;
                    body_content.parse::<Token![=]>()?;
                    let lit_bool = body_content.parse::<LitBool>()?;
                    is_non_null = lit_bool.value();
                } else {
                    return Err(lookahead.error());
                }
            }

            if !body_content.is_empty() {
                body_content.parse::<Token![,]>()?;
            }
        }

        let field_signature = field_signature
            .ok_or_else(|| syn::Error::new(rust_name.span(), "Field must have a 'sig' property"))?;

        // Explicitly overriding the getter/setter name but not the other should result in
        // us only generating the specified one.
        let (getter_name, setter_name) =
            if getter_override_name.is_some() || setter_override_name.is_some() {
                (getter_override_name, setter_override_name)
            } else {
                (Some(default_getter_name), Some(default_setter_name))
            };

        // Validate non_null usage
        if is_non_null {
            let is_primitive = field_signature
                .field_type
                .try_as_primitive(type_mappings)
                .is_some();
            if is_primitive {
                return Err(syn::Error::new(
                    rust_name.span(),
                    "Cannot use 'non_null' with fields of primitive types - non_null is only valid for object/reference field types",
                ));
            }
        }

        Ok(Field {
            java_name,
            rust_name,
            getter_name,
            setter_name,
            getter_visibility,
            setter_visibility,
            field_signature,
            attrs: field_attrs,
            getter_attrs,
            setter_attrs,
            is_static,
            non_null: is_non_null,
        })
    } else {
        Err(syn::Error::new(
            rust_name.span(),
            "Field must be followed by colon (shorthand syntax) or braces (block syntax)",
        ))
    }
}

/// Parse methods or static_methods block
fn parse_fields(input: ParseStream, type_mappings: &TypeMappings) -> Result<Vec<Field>> {
    let mut fields = Vec::new();

    while !input.is_empty() {
        let field = parse_field(input, type_mappings)?;

        fields.push(field);

        if !input.is_empty() {
            input.parse::<Token![,]>()?;
        }
    }

    Ok(fields)
}

/// The main input structure for bind_java_type!
struct BindClassInput {
    type_name: Ident,
    type_visibility: Option<VisibilitySpec>,
    type_attrs: Vec<syn::Attribute>,
    java_class: JavaClassName,
    api_name: Option<Ident>,
    priv_type: Option<Ident>,
    native_trait_name: Option<Ident>,
    is_instance_of: Vec<IsInstanceOfEntry>,
    type_mappings: TypeMappings,
    constructors: Vec<Constructor>,
    methods: Vec<Method>,
    fields: Vec<Field>,
    native_methods: Vec<NativeMethod>,
    load_class_closure: Option<TokenStream>,
    init_priv_closure: Option<TokenStream>,
    native_methods_export: bool,
    native_methods_catch_unwind: bool,
    abi_check: AbiCheck,
    native_methods_error_policy: Option<syn::Path>,
    jni_core: bool,
    jni_crate: syn::Path,
    sys_type: Option<Ident>,
}

impl Parse for BindClassInput {
    fn parse(input: ParseStream) -> Result<Self> {
        let jni_crate = crate::utils::parse_jni_crate_override(&input)?;

        let mut type_mappings = TypeMappings::new(&jni_crate);

        // Format:
        //
        // ```
        // #[attributes]
        // key = value,
        // #[attributes]
        // key = { ... },
        // #[attributes]
        // key { ... },
        // #[attributes]
        // RustType => java.Type,
        // ```

        // Initialize all possible properties
        let mut type_visibility = None;
        let mut type_name_opt = None;
        let mut type_attrs = Vec::new();
        let mut java_class_opt = None;
        let mut api_name = None;
        let mut priv_type = None;
        let mut native_trait_name = None;
        let mut is_instance_of = Vec::new();
        let mut constructors = Vec::new();
        let mut methods = Vec::new();
        let mut fields = Vec::new();
        let mut native_methods = Vec::new();
        let mut load_class_closure = None;
        let mut init_priv_closure = None;
        let mut native_methods_export = true;
        let mut native_methods_catch_unwind = true;
        let mut abi_check = AbiCheck::default();
        let mut native_methods_error_policy = None;
        let mut jni_core = false;
        let mut sys_type = None;

        while !input.is_empty() {
            let mut prop_attrs = input.call(syn::Attribute::parse_outer)?;

            // Try to peek for <ident> = pattern or <ident> { ... }
            // (anything else is treated as `[vis] RustType => java.Type` shorthand)
            let fork = input.fork();
            let is_prop = if let Ok(_ident) = fork.call(Ident::parse_any) {
                // Note: we need to rule out `=>` before checking for `=` otherwise we could split the `=>` token
                // and misinterpret the shorthand syntax as a property
                !fork.peek(Token![=>]) && (fork.peek(Token![=]) || fork.peek(syn::token::Brace))
            } else {
                false
            };

            if is_prop {
                let lookahead = input.lookahead1();

                if lookahead.peek(rust_type) {
                    let prop_ident: Ident = input.parse()?;
                    if type_name_opt.is_some() {
                        return Err(syn::Error::new(
                            prop_ident.span(),
                            "Rust type name already specified",
                        ));
                    }
                    input.parse::<Token![=]>()?;

                    if let Some(vis) = parse_visibility(input)? {
                        type_visibility = Some(vis);
                    }

                    type_name_opt = Some(input.parse()?);
                    type_attrs = std::mem::take(&mut prop_attrs);
                } else if lookahead.peek(rust_type_vis) {
                    let prop_ident: Ident = input.parse()?;
                    if type_visibility.is_some() {
                        return Err(syn::Error::new(
                            prop_ident.span(),
                            "Type visibility already specified",
                        ));
                    }
                    input.parse::<Token![=]>()?;
                    let Some(vis) = parse_visibility(input)? else {
                        return Err(syn::Error::new(
                            prop_ident.span(),
                            "Expected visibility specifier after 'type_visibility ='",
                        ));
                    };
                    type_visibility = Some(vis);
                } else if lookahead.peek(java_type) {
                    let prop_ident: Ident = input.parse()?;
                    if java_class_opt.is_some() {
                        return Err(syn::Error::new(
                            prop_ident.span(),
                            "Java class name already specified",
                        ));
                    }
                    input.parse::<Token![=]>()?;
                    java_class_opt = Some(input.parse()?);
                } else if lookahead.peek(api) {
                    let _ = input.parse::<Ident>()?;
                    input.parse::<Token![=]>()?;
                    api_name = Some(input.parse()?);
                } else if lookahead.peek(self::priv_type) {
                    let _ = input.parse::<Ident>()?;
                    input.parse::<Token![=]>()?;
                    priv_type = Some(input.parse()?);
                } else if lookahead.peek(native_trait) {
                    let _ = input.parse::<Ident>()?;
                    input.parse::<Token![=]>()?;
                    native_trait_name = Some(input.parse()?);
                } else if lookahead.peek(self::is_instance_of) {
                    let _ = input.parse::<Ident>()?;
                    // Optional '=' before block
                    if input.peek(Token![=]) {
                        input.parse::<Token![=]>()?;
                    }

                    let is_instance_content;
                    braced!(is_instance_content in input);

                    while !is_instance_content.is_empty() {
                        // Try to parse stem = Type or stem: Type or just Type
                        let stem: Option<String>;
                        let type_path: syn::Path;

                        // Check if we have a simple identifier (not a path) followed by = or :
                        // We need to distinguish between:
                        //   - "stem = Path" or "stem: Path" (has stem)
                        //   - "path::to::Type" (no stem, just a path)
                        // To do this, check if the next token after an identifier is = or : (not ::)
                        let has_stem = is_instance_content.peek(Ident)
                            && !is_instance_content.peek2(Token![::])
                            && (is_instance_content.peek2(Token![=])
                                || is_instance_content.peek2(Token![:]));

                        if has_stem {
                            let stem_ident: Ident = is_instance_content.parse()?;
                            if is_instance_content.peek(Token![=]) {
                                is_instance_content.parse::<Token![=]>()?;
                            } else {
                                is_instance_content.parse::<Token![:]>()?;
                            }
                            type_path = is_instance_content.parse()?;
                            stem = Some(stem_ident.to_string());
                        } else {
                            // Just a bare type path
                            type_path = is_instance_content.parse()?;
                            stem = None;
                        }

                        let type_path_str = quote!(#type_path).to_string().replace(" ", "");

                        // Validate that JObject is not explicitly specified
                        if type_path_str == "JObject" || type_path_str == "jni::objects::JObject" {
                            return Err(syn::Error::new_spanned(
                                &type_path,
                                "JObject should not be explicitly specified in is_instance_of - all types are already instances of JObject",
                            ));
                        }

                        is_instance_of.push(IsInstanceOfEntry {
                            type_alias: type_path_str,
                            stem,
                        });

                        // Require comma between entries, but trailing comma is optional
                        if !is_instance_content.is_empty() {
                            is_instance_content.parse::<Token![,]>()?;
                        }
                    }
                } else if lookahead.peek(self::type_map) {
                    let _ = input.parse::<Ident>()?;
                    type_mappings.parse_mappings(input)?;
                } else if lookahead.peek(self::hooks) {
                    let _ = input.parse::<Ident>()?;
                    // Optional '=' before block
                    if input.peek(Token![=]) {
                        input.parse::<Token![=]>()?;
                    }

                    let hooks_content;
                    braced!(hooks_content in input);

                    while !hooks_content.is_empty() {
                        let hooks_key = hooks_content.parse::<Ident>()?;
                        hooks_content.parse::<Token![=]>()?;

                        // Parse closure as an expression until we hit a comma
                        let closure_expr: syn::Expr = hooks_content.parse()?;
                        let closure_tokens = quote! { #closure_expr };

                        match hooks_key.to_string().as_str() {
                            "load_class" => {
                                load_class_closure = Some(closure_tokens);
                            }
                            "init_priv" => {
                                init_priv_closure = Some(closure_tokens);
                            }
                            _ => {
                                return Err(syn::Error::new(
                                    hooks_key.span(),
                                    format!("Unknown hooks property: {}", hooks_key),
                                ));
                            }
                        }

                        // Require comma between entries, but trailing comma is optional
                        if !hooks_content.is_empty() {
                            hooks_content.parse::<Token![,]>()?;
                        }
                    }
                } else if lookahead.peek(self::constructors) {
                    let _ = input.parse::<Ident>()?;
                    // Optional '=' before block
                    if input.peek(Token![=]) {
                        input.parse::<Token![=]>()?;
                    }

                    let constructors_content;
                    braced!(constructors_content in input);
                    constructors = parse_constructors(&constructors_content, &type_mappings)?;
                } else if lookahead.peek(self::methods) {
                    let _ = input.parse::<Ident>()?;
                    // Optional '=' before block
                    if input.peek(Token![=]) {
                        input.parse::<Token![=]>()?;
                    }

                    let methods_content;
                    braced!(methods_content in input);

                    methods.extend(parse_methods(&methods_content, &type_mappings)?);
                } else if lookahead.peek(self::fields) {
                    let _ = input.parse::<Ident>()?;
                    // Optional '=' before block
                    if input.peek(Token![=]) {
                        input.parse::<Token![=]>()?;
                    }

                    let fields_content;
                    braced!(fields_content in input);

                    fields.extend(parse_fields(&fields_content, &type_mappings)?);
                } else if lookahead.peek(self::native_methods) {
                    let _ = input.parse::<Ident>()?;
                    // Optional '=' before block
                    if input.peek(Token![=]) {
                        input.parse::<Token![=]>()?;
                    }

                    let native_methods_content;
                    braced!(native_methods_content in input);

                    // A native method block can produce a NativeMethod implementation to register /
                    // export, as well as a public Method API if any visibility is specified
                    let (block_native_methods, block_methods) =
                        parse_native_methods(&native_methods_content, &type_mappings)?;
                    native_methods.extend(block_native_methods);
                    methods.extend(block_methods);
                } else if lookahead.peek(self::native_methods_export) {
                    let _ = input.parse::<Ident>()?;
                    input.parse::<Token![=]>()?;
                    let value: LitBool = input.parse()?;
                    native_methods_export = value.value();
                } else if lookahead.peek(self::native_methods_error_policy) {
                    let _ = input.parse::<Ident>()?;
                    input.parse::<Token![=]>()?;
                    native_methods_error_policy = Some(input.parse::<syn::Path>()?);
                } else if lookahead.peek(self::abi_check) {
                    let _ = input.parse::<Ident>()?;
                    input.parse::<Token![=]>()?;
                    abi_check = input.parse::<AbiCheck>()?;
                } else if lookahead.peek(self::native_methods_catch_unwind) {
                    let _ = input.parse::<Ident>()?;
                    input.parse::<Token![=]>()?;
                    let value: LitBool = input.parse()?;
                    native_methods_catch_unwind = value.value();
                } else {
                    // Private or invalid properties that shouldn't show in in a lookahead1 error
                    // as a suggested property

                    let property_name: Ident = input.parse()?;
                    let property_str = property_name.to_string();

                    match property_str.as_str() {
                        "jni" => {
                            // jni can only be the first property
                            return Err(syn::Error::new(
                                property_name.span(),
                                "jni property must be the first property if specified",
                            ));
                        }
                        "__jni_core" => {
                            input.parse::<Token![=]>()?;
                            let value: LitBool = input.parse()?;
                            jni_core = value.value();
                        }
                        "__sys_type" => {
                            input.parse::<Token![=]>()?;
                            sys_type = Some(input.parse()?);
                        }
                        _ => {
                            return Err(lookahead.error());
                        }
                    }
                }
            } else {
                // Expect shorthand syntax: [vis] RustType => java.Type
                type_visibility = parse_visibility(input)?;

                let rust_type: Ident = input.parse()?;
                input.parse::<Token![=>]>()?;
                let java_type: JavaClassName = input.parse()?;

                if type_name_opt.is_some() || java_class_opt.is_some() {
                    return Err(syn::Error::new(
                        input.span(),
                        "The Rust type and Java class can only be specified once",
                    ));
                }
                type_name_opt = Some(rust_type);
                type_attrs = std::mem::take(&mut prop_attrs);
                java_class_opt = Some(java_type);
            }

            // As soon as we know the type name and java class, we can can register
            // a Self type mapping
            if let (Some(type_name), Some(java_class)) = (&type_name_opt, &java_class_opt) {
                let type_path_str = type_name.to_string();
                // A Self type mapping already exists (e.g. from a type_map block or builtin type)
                // then we let that take precedence. This avoids conflicting with shared type_maps
                // in a project that may also use fully qualified Rust type names that will look
                // like a type mismatch if we call insert_ref_type again here.
                #[allow(clippy::collapsible_if)]
                if !type_mappings.is_core_java_type(java_class)
                    && type_mappings.map_alias(&type_path_str).is_none()
                {
                    // Note: if the type mapping already exists (e.g. from a type_map block or builtin type)
                    // this will validate that the type matches the existing mapping
                    if let Err(err) = type_mappings.insert_ref_type(
                        &type_path_str,
                        &type_path_str,
                        java_class.clone(),
                        false, // implied because the type isn't already mapped
                    ) {
                        return Err(syn::Error::new(
                            type_name.span(),
                            format!("Failed to register Self type mapping: {}", err),
                        ));
                    }
                }
            }

            // Require comma after each property, except trailing comma is optional
            if !input.is_empty() {
                input.parse::<Token![,]>()?;
            }
        }

        // Validate required properties
        let type_name = type_name_opt.ok_or_else(|| {
            syn::Error::new(
                input.span(),
                "Missing required property: rust_type = RustTypeName",
            )
        })?;

        let java_class = java_class_opt.ok_or_else(|| {
            syn::Error::new(
                input.span(),
                "Missing required property: java_type = java.class.Name",
            )
        })?;

        Ok(BindClassInput {
            type_name,
            type_visibility,
            type_attrs,
            java_class,
            api_name,
            priv_type,
            native_trait_name,
            is_instance_of,
            type_mappings,
            constructors,
            methods,
            fields,
            native_methods,
            load_class_closure,
            init_priv_closure,
            native_methods_export,
            native_methods_catch_unwind,
            abi_check,
            native_methods_error_policy,
            jni_core,
            jni_crate,
            sys_type,
        })
    }
}

/// Generate code for the bind_java_type macro
pub fn bind_java_type_impl(input: TokenStream) -> Result<TokenStream> {
    let input: BindClassInput = syn::parse2(input)?;

    let type_name = &input.type_name;
    let java_class = &input.java_class;
    let java_class_internal = java_class.to_jni_internal();
    let java_class_dotted = java_class.to_java_dotted();
    let jni = &input.jni_crate;

    // Check if this is a core Java type that cannot be bound without __jni_core = true
    if input.type_mappings.is_core_java_type(java_class) && !input.jni_core {
        return Err(syn::Error::new(
            Span::call_site(),
            format!(
                "Cannot create binding for core Java type '{}'. Core types (java.lang.Object, java.lang.Class, java.lang.Throwable, and java.lang.String) are already provided by the jni crate and cannot be re-bound.",
                java_class_dotted
            ),
        ));
    }

    // Generate API struct name
    let api_name = input
        .api_name
        .unwrap_or_else(|| format_ident!("{}API", type_name));

    let type_visibility = input.type_visibility.unwrap_or(VisibilitySpec::PubSelf);

    // Generate the type struct
    let type_struct = generate_type_struct(
        type_name,
        &type_visibility,
        &input.type_attrs,
        &java_class_dotted,
        jni,
    );

    // Generate constructor method ID fields and initialization code
    let (constructor_method_id_fields, constructor_method_id_inits) =
        generate_constructor_method_ids(&input.constructors, &input.type_mappings, jni)?;

    // Generate method ID fields and initialization code for methods
    let (method_id_fields, method_id_inits) =
        generate_method_ids(&input.methods, &input.type_mappings, jni)?;

    // Generate field ID fields and initialization code for fields
    let (field_id_fields, field_id_inits) =
        generate_field_ids(&input.fields, &input.type_mappings, jni)?;

    // Combine all ID fields and inits
    let all_id_fields = [
        constructor_method_id_fields,
        method_id_fields,
        field_id_fields,
    ]
    .concat();

    let all_id_inits = [constructor_method_id_inits, method_id_inits, field_id_inits].concat();

    // Generate the API struct
    let api_struct = generate_api_struct(
        &api_name,
        &type_visibility,
        input.priv_type.as_ref(),
        &all_id_fields,
        jni,
    );

    // Generate native methods support - get registration code for API::get()
    let (native_trait_and_wrappers, native_registration_code) = generate_native_methods_code(
        type_name,
        &api_name,
        &type_visibility,
        &java_class_internal,
        &java_class_dotted,
        &input.native_methods,
        &input.native_trait_name,
        &input.type_mappings,
        input.native_methods_export,
        input.native_methods_catch_unwind,
        input.abi_check,
        &input.native_methods_error_policy,
        jni,
    )?;

    // Generate the API::get() method with native registration integrated
    let api_get_method = generate_api_get_method(
        &api_name,
        type_name,
        &java_class_dotted,
        input.load_class_closure.as_ref(),
        input.init_priv_closure.as_ref(),
        input.priv_type.as_ref(),
        &all_id_inits,
        &native_registration_code,
        &input.is_instance_of,
        &input.type_mappings,
        input.abi_check,
        jni,
    )?;

    // Generate Reference trait implementation
    let reference_impl = generate_reference_impl(type_name, &api_name, &java_class_dotted, jni);

    // Generate base methods (from_raw, null, into_raw)
    let base_methods = generate_base_methods(type_name, input.sys_type.as_ref(), jni);

    // Generate is_instance_of methods and From impls
    let is_instance_of_code =
        generate_is_instance_of_code(type_name, &input.is_instance_of, &input.type_mappings, jni)?;

    // Generate constructor implementations
    let constructors_code = generate_constructors(
        type_name,
        &api_name,
        &input.constructors,
        &input.type_mappings,
        jni,
    )?;

    // Generate method implementations
    let methods_impl = generate_methods(&api_name, &input.methods, &input.type_mappings, jni)?;

    // Generate field implementations
    let fields_impl = generate_fields(&api_name, &input.fields, &input.type_mappings, jni);

    // Wrap instance methods and fields in impl<'local> block
    let instance_impl = quote! {
        impl<'local> #type_name<'local> {
            #methods_impl
            #fields_impl
        }
    };

    Ok(quote! {
        #type_struct
        #api_struct
        #api_get_method
        #reference_impl
        #base_methods
        #is_instance_of_code
        #constructors_code
        #instance_impl
        #native_trait_and_wrappers
    })
}

/// Generate the type struct definition
fn generate_type_struct(
    type_name: &Ident,
    type_visibility: &VisibilitySpec,
    type_attrs: &[syn::Attribute],
    java_class: &str,
    jni: &syn::Path,
) -> TokenStream {
    // Check if there's a doc attribute in type_attrs
    let has_doc_attr = type_attrs.iter().any(|attr| attr.path().is_ident("doc"));

    let jni_path_str = path_to_string_no_spaces(jni);

    // Generate default doc comment if no doc attribute provided
    let doc_attr = if has_doc_attr {
        quote! {}
    } else {
        quote! {
            #[doc = concat!(
                r#"A `"#, #java_class, r#"` reference, tied to a JNI local reference frame.

See the [`JObject`] documentation for more information about object references,
how to cast them, and local reference frame lifetimes.

[`JObject`]: "#, #jni_path_str, r#"::objects::JObject
"#
            )]
        }
    };

    let type_visibility = type_visibility.to_tokens();

    quote! {
        #(#type_attrs)*
        #doc_attr
        #[repr(transparent)]
        #[derive(Debug, Default)]
        #type_visibility struct #type_name<'local>(#jni::objects::JObject<'local>);

        impl<'local> ::core::convert::AsRef<#type_name<'local>> for #type_name<'local> {
            #[inline]
            fn as_ref(&self) -> &#type_name<'local> {
                self
            }
        }

        impl<'local> ::core::convert::AsRef<#jni::objects::JObject<'local>> for #type_name<'local> {
            #[inline]
            fn as_ref(&self) -> &#jni::objects::JObject<'local> {
                self
            }
        }

        impl<'local> ::core::ops::Deref for #type_name<'local> {
            type Target = #jni::objects::JObject<'local>;

            #[inline]
            fn deref(&self) -> &Self::Target {
                &self.0
            }
        }

        impl<'local> ::core::convert::From<#type_name<'local>> for #jni::objects::JObject<'local> {
            #[inline]
            fn from(other: #type_name<'local>) -> #jni::objects::JObject<'local> {
                other.0
            }
        }
    }
}

/// Generate the API struct definition
fn generate_api_struct(
    api_name: &Ident,
    type_visibility: &VisibilitySpec,
    priv_type: Option<&Ident>,
    method_id_fields: &[TokenStream],
    jni: &syn::Path,
) -> TokenStream {
    let priv_field = if let Some(priv_ty) = priv_type {
        quote! { private: #priv_ty, }
    } else {
        quote! {}
    };

    let api_visibility = VisibilitySpec::PubCrate
        .clamp_to(type_visibility)
        .to_tokens();
    quote! {
        #[allow(non_snake_case)]
        #api_visibility struct #api_name {
            class: #jni::refs::Global<#jni::objects::JClass<'static>>,
            #priv_field
            #(#method_id_fields)*
        }

        unsafe impl ::core::marker::Send for #api_name {}
        unsafe impl ::core::marker::Sync for #api_name {}
    }
}

/// Generate the API::get() method
#[allow(clippy::too_many_arguments)]
fn generate_api_get_method(
    api_name: &Ident,
    type_name: &Ident,
    java_class: &str,
    load_class_closure: Option<&TokenStream>,
    init_priv_closure: Option<&TokenStream>,
    priv_type: Option<&Ident>,
    method_id_inits: &[TokenStream],
    native_registration_code: &TokenStream,
    is_instance_of: &[IsInstanceOfEntry],
    type_mappings: &TypeMappings,
    abi_check: AbiCheck,
    jni: &syn::Path,
) -> Result<TokenStream> {
    // Generate wrapper function names to avoid conflicts
    let load_class_wrapper_name = format_ident!(
        "__{}_load_class_wrapper",
        type_name.to_string().to_lowercase()
    );
    let init_priv_wrapper_name = format_ident!(
        "__{}_init_priv_wrapper",
        type_name.to_string().to_lowercase()
    );

    // Generate wrapper functions (outside the impl block)
    let load_class_wrapper = if load_class_closure.is_some() {
        quote! {
            #[allow(unused, non_snake_case)]
            fn #load_class_wrapper_name<'env_local, F>(
                env: &mut #jni::Env<'env_local>,
                loader: &#jni::refs::LoaderContext,
                initialize: bool,
                load_class: F,
            ) -> #jni::errors::Result<#jni::objects::JClass<'env_local>>
            where
                F: FnOnce(
                    &mut #jni::Env<'env_local>,
                    &#jni::refs::LoaderContext,
                    bool,
                ) -> #jni::errors::Result<#jni::objects::JClass<'env_local>>,
            {
                load_class(env, loader, initialize)
            }
        }
    } else {
        quote! {}
    };

    #[allow(clippy::collapsible_if)]
    let init_priv_wrapper = if let Some(priv_type) = priv_type {
        if init_priv_closure.is_some() {
            quote! {
                #[allow(unused, non_snake_case)]
                fn #init_priv_wrapper_name<'env_local, F>(
                    env: &mut #jni::Env<'env_local>,
                    loader: &#jni::refs::LoaderContext,
                    class: &#jni::objects::JClass,
                    init_priv: F,
                ) -> #jni::errors::Result<#priv_type>
                where
                    F: FnOnce(
                        &mut #jni::Env<'env_local>,
                        &#jni::refs::LoaderContext,
                        &#jni::objects::JClass,
                    ) -> #jni::errors::Result<#priv_type>,
                    #priv_type: Send + Sync
                {
                    init_priv(env, loader, class)
                }
            }
        } else {
            quote! {}
        }
    } else {
        quote! {}
    };

    // Generate the class loading code
    let load_class_code = if let Some(closure) = load_class_closure {
        quote! {
            #load_class_wrapper_name(env, loader, false, #closure)?
        }
    } else {
        quote! {
            loader.load_class_for_type::<#type_name>(env, false)?
        }
    };

    // Generate the priv initialization code
    let init_priv_code = if let Some(closure) = init_priv_closure {
        quote! {
            let private = #init_priv_wrapper_name(env, loader, class, #closure)?;
        }
    } else {
        quote! {}
    };

    // Generate IsAssignable checks for all is_instance_of types
    let is_instance_of_checks = generate_is_instance_of_assignable_checks(
        type_name,
        java_class,
        is_instance_of,
        type_mappings,
        jni,
    )?;

    // Generate type mapping assertions
    let type_mapping_runtime_checks = if abi_check.requires_abi_check() {
        generate_type_mapping_checks(type_mappings, jni)
    } else {
        quote! {}
    };

    let api_construction = if priv_type.is_some() {
        quote! {
            // All of the CStr literals have been validated at compile time and
            // since they have been encoded as MUTF-8 they can be safely cast as
            // a JNIStr without a runtime check.
            unsafe {
                Self {
                    class: env.new_global_ref(class)?,
                    private,
                    #(#method_id_inits)*
                }
            }
        }
    } else {
        quote! {
            // All of the CStr literals have been validated at compile time and
            // since they have been encoded as MUTF-8 they can be safely cast as
            // a JNIStr without a runtime check.
            unsafe {
                Self {
                    class: env.new_global_ref(class)?,
                    #(#method_id_inits)*
                }
            }
        }
    };

    Ok(quote! {
        #load_class_wrapper
        #init_priv_wrapper

        impl #api_name {
            pub fn get(
                env: &#jni::Env,
                loader: &#jni::refs::LoaderContext,
            ) -> #jni::errors::Result<&'static Self> {
                static API: ::std::sync::OnceLock<#api_name> = ::std::sync::OnceLock::new();

                // Fast path: already initialized
                if let Some(api) = API.get() {
                    return Ok(api);
                }

                // Slow path: Lookup class

                // The general pattern here is to avoid holding any lock while
                // performing class lookups and API initialization in case we need
                // to be re-entrant (e.g. due to class initializers that call back
                // into Rust).
                //
                // This matters for types where we lookup method IDs and field IDs
                // which may trigger class initialization, and especially for types
                // that register native methods that may need to be registered
                // before the class can be initialized and then called during class
                // initialization.

                // NB: the purpose of the `OnceLock` here is to amortize the cost of
                // class lookups and API initialization over multiple uses, so we aren't
                // really concerned about a small amount of redundant work if multiple
                // threads race here.

                let api = env.with_local_frame(4, |env| -> #jni::errors::Result<#api_name> {
                    // We cache the class early to avoid repeat lookups just in case
                    // `::get()` is re-entered by the same thread during class
                    // initialization.
                    //
                    // After `API` is set this `CLASS` cache won't be used again.
                    static CLASS: ::std::sync::OnceLock<#jni::objects::Global<#jni::objects::JClass>> = ::std::sync::OnceLock::new();

                    let class = if let Some(class) = CLASS.get() {
                        class
                    } else {
                        let class: #jni::objects::JClass = {
                            #load_class_code
                        };
                        let global_class = env.new_global_ref(&class)?;
                        let _ = CLASS.set(global_class);
                        CLASS.get().unwrap()
                    };
                    let class: &#jni::objects::JClass = class.as_ref();

                    // Register native methods ASAP, before anything could
                    // trigger class initialization that might itself need to
                    // call native methods.
                    #native_registration_code

                    #init_priv_code

                    // Assert that all declared is_instance_of types are valid
                    #is_instance_of_checks

                    // Assert that all type mappings are correct
                    #type_mapping_runtime_checks

                    let api = #api_construction;

                    Ok(api)
                })?;
                let _ = API.set(api);
                Ok(API.get().unwrap())
            }
        }
    })
}

/// Generate IsAssignable checks for all declared is_instance_of types
fn generate_is_instance_of_assignable_checks(
    type_name: &Ident,
    java_class: &str,
    is_instance_of: &[IsInstanceOfEntry],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<TokenStream> {
    let checks: Vec<_> = is_instance_of
        .iter()
        .map(|entry| {
            let to_rust_type_alias = &entry.type_alias;

            match type_mappings.map_alias(to_rust_type_alias) {
                Some(ConcreteType::Primitive { .. }) => {
                    Err(syn::Error::new(
                        Span::call_site(),
                        format!("Primitive type '{}' cannot be used in is_instance_of", to_rust_type_alias)
                    ))
                },
                Some(ConcreteType::Object { name: to_java_class, reference_type: to_rust_type }) => {
                    let to_type_path_str = &to_rust_type.path();

                    let to_java_class_str = to_java_class.to_java_dotted();

                    let to_type_path: syn::Path = syn::parse_str(to_type_path_str).map_err(|_| {
                        syn::Error::new(
                            Span::call_site(),
                            format!("Invalid Rust type path: {}", to_type_path_str),
                        )
                    })?;

                    Ok(quote! {
                        {
                            let is_instance_of_class = <#to_type_path as #jni::refs::Reference>::lookup_class(env, loader)?;
                            let is_instance_of_class: &#jni::objects::JClass = unsafe { is_instance_of_class.as_ref() };

                            // IsAssignableFrom is not exception safe, so we can't go ahead with the JNI call if
                            // there's currently a pending exception.
                            if env.exception_check() {
                                return Err(#jni::errors::Error::JavaException);
                            }
                            // Call IsAssignableFrom to check if is_instance_of_class is assignable from our class
                            let is_assignable = unsafe {
                                use #jni::refs::Reference as _;
                                let env_ptr: *mut #jni::sys::JNIEnv = env.get_raw();
                                let interface: *const #jni::sys::JNINativeInterface_ = *env_ptr;
                                ((*interface).v1_1.IsAssignableFrom)(
                                    env_ptr,
                                    class.as_raw(), // From
                                    is_instance_of_class.as_raw(), // To
                                )
                            };

                            assert!(
                                is_assignable,
                                "{}({}) is not a subtype of {}({}) and is not a valid 'is_instance_of' type for {}",
                                #to_type_path_str,
                                #to_java_class_str,
                                stringify!(#type_name),
                                #java_class,
                                stringify!(#type_name)
                            );
                        }
                    })
                }
                None => {
                    Err(syn::Error::new(
                        Span::call_site(),
                        format!("Type '{}' used in is_instance_of is not defined in types mapping", to_rust_type_alias)
                    ))
                }
            }


        })
        .collect::<Result<Vec<_>>>()?;

    if checks.is_empty() {
        Ok(quote! {})
    } else {
        Ok(quote! {
            #(#checks)*
        })
    }
}

/// Generate Reference trait implementation
fn generate_reference_impl(
    type_name: &Ident,
    api_name: &Ident,
    java_class: &str,
    jni: &syn::Path,
) -> TokenStream {
    // Create a CStr literal for the Java class name
    let java_class_cstr = lit_cstr_mutf8(java_class);

    quote! {
        unsafe impl<'local> #jni::refs::Reference for #type_name<'local> {
            type Kind<'env> = #type_name<'env>;
            type GlobalKind = #type_name<'static>;

            #[inline]
            fn as_raw(&self) -> #jni::sys::jobject {
                self.0.as_raw()
            }

            #[inline]
            fn class_name() -> ::std::borrow::Cow<'static, #jni::strings::JNIStr> {
                // Safety: we have compile-time encoded the name of the Java class as MUTF8
                // and therefore know it's safe to cast as a JNIStr
                unsafe {
                    ::std::borrow::Cow::Borrowed(#jni::strings::JNIStr::from_cstr_unchecked(#java_class_cstr))
                }
            }

            #[inline]
            fn lookup_class<'caller>(
                env: &#jni::Env<'_>,
                loader_context: &#jni::refs::LoaderContext,
            ) -> #jni::errors::Result<
                impl ::std::ops::Deref<Target = #jni::refs::Global<#jni::objects::JClass<'static>>> + 'caller
            > {
                let api = #api_name::get(env, loader_context)?;
                Ok(&api.class)
            }

            #[inline]
            unsafe fn kind_from_raw<'env>(local_ref: #jni::sys::jobject) -> Self::Kind<'env> {
                unsafe { #type_name(#jni::objects::JObject::kind_from_raw(local_ref)) }
            }

            #[inline]
            unsafe fn global_kind_from_raw(global_ref: #jni::sys::jobject) -> Self::GlobalKind {
                unsafe { #type_name(#jni::objects::JObject::global_kind_from_raw(global_ref)) }
            }
        }
    }
}

/// Generate base methods (from_raw, null, into_raw)
fn generate_base_methods(
    type_name: &Ident,
    sys_type: Option<&Ident>,
    jni: &syn::Path,
) -> TokenStream {
    // Use the provided sys_type or default to jobject
    let sys_type_ident = sys_type
        .map(|ident| quote! { #ident })
        .unwrap_or_else(|| quote! { jobject });

    // Generate the full path to the sys type
    let sys_type_path = quote! { #jni::sys::#sys_type_ident };

    let jni_path_str = path_to_string_no_spaces(jni);

    quote! {
        impl<'local> #type_name<'local> {
            #[doc = concat!(
                r#"Creates a [`"#, stringify!(#type_name), r#"`] that wraps the given `raw` [jobject]

# Safety

- `raw` must be a valid raw JNI local reference (or `null`).
- `raw` must be an instance of the correct Java class.
- There must not be any other owning [Reference] wrapper for the same reference.
- The local reference must belong to the current thread and not outlive the
  JNI stack frame associated with the [Env] `'local` lifetime.

[jobject]: "#, #jni_path_str, r#"::sys::jobject
[Reference]: "#, #jni_path_str, r#"::refs::Reference
[Env]: "#, #jni_path_str, r#"::Env
"#
            )]
            #[inline]
            pub unsafe fn from_raw<'env_inner>(
                env: &#jni::Env<'env_inner>,
                raw: #sys_type_path,
            ) -> #type_name<'env_inner> {
                unsafe { #type_name(#jni::objects::JObject::from_raw(env, raw as #jni::sys::jobject)) }
            }

            #[doc = concat!(
                r#"Creates a new null reference.

Null references are always valid and do not belong to a local reference frame. Therefore,
the returned [`"#, stringify!(#type_name), r#"`] always has the `'static` lifetime."#
            )]
            #[inline]
            pub const fn null() -> #type_name<'static> {
                #type_name(#jni::objects::JObject::null())
            }

            #[doc = r" Unwrap to the raw jni type."]
            #[inline]
            pub fn into_raw(self) -> #sys_type_path {
                self.0.into_raw() as #sys_type_path
            }

            #[doc = concat!(
                r#"Cast a local reference to a [`"#, stringify!(#type_name), r#"`]

This will do a runtime (`IsInstanceOf`) check that the object is an instance of the correct class.

Also see these other options for casting local or global references to a [`"#, stringify!(#type_name), r#"`]:
- [Env::as_cast]("#, #jni_path_str, r#"::Env::as_cast)
- [Env::new_cast_local_ref]("#, #jni_path_str, r#"::Env::new_cast_local_ref)
- [Env::cast_local]("#, #jni_path_str, r#"::Env::cast_local)
- [Env::new_cast_global_ref]("#, #jni_path_str, r#"::Env::new_cast_global_ref)
- [Env::cast_global]("#, #jni_path_str, r#"::Env::cast_global)

# Errors

Returns [Error::WrongObjectType] if the `IsInstanceOf` check fails.

[Error::WrongObjectType]: "#, #jni_path_str, r#"::errors::Error::WrongObjectType
"#
            )]
            #[inline]
            pub fn cast_local<'any_local>(
                env: &mut #jni::Env<'_>,
                obj: impl #jni::refs::Reference
                    + ::core::convert::Into<#jni::objects::JObject<'any_local>>
                    + ::core::convert::AsRef<#jni::objects::JObject<'any_local>>,
            ) -> #jni::errors::Result<#type_name<'any_local>> {
                env.cast_local::<#type_name>(obj)
            }
        }
    }
}

/// Generate is_instance_of methods and From trait implementations
fn generate_is_instance_of_code(
    type_name: &Ident,
    is_instance_of: &[IsInstanceOfEntry],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<TokenStream> {
    let mut is_instance_of_methods = Vec::new();
    let mut trait_impls = Vec::new();

    for entry in is_instance_of {
        let type_alias = &entry.type_alias;

        match type_mappings.map_alias(type_alias) {
            Some(ConcreteType::Primitive { .. }) => {
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!(
                        "Primitive type '{}' cannot be used in is_instance_of",
                        type_alias
                    ),
                ));
            }
            Some(ConcreteType::Object {
                reference_type: rust_type,
                ..
            }) => {
                let type_path_str = rust_type.path();

                let type_path: syn::Path = syn::parse_str(type_path_str).map_err(|_| {
                    syn::Error::new(
                        Span::call_site(),
                        format!("Invalid Rust type path: {}", type_path_str),
                    )
                })?;

                if let Some(stem) = &entry.stem {
                    let method_name = format_ident!("as_{}", stem);

                    is_instance_of_methods.push(quote! {
                        #[doc = concat!(
                        r#"Casts this `"#, stringify!(#type_name), r#"` to a `"#, #type_path_str, r#"`

This does not require a runtime type check since any `"#, stringify!(#type_name), r#"` is also a `"#, #type_path_str, r#"`"#)]
                        pub fn #method_name(&self) -> #jni::refs::Cast<'local, '_, #type_path<'local>> {
                            unsafe { #jni::refs::Cast::<#type_path>::new_unchecked(self) }
                        }
                    });
                }

                // Only generate From impl if the type is not JObject
                if type_alias != "JObject" && type_alias != "jni::objects::JObject" {
                    trait_impls.push(quote! {
                        impl<'local> ::core::convert::From<#type_name<'local>> for #type_path<'local> {
                            fn from(value: #type_name<'local>) -> #type_path<'local> {
                                let raw = value.into_raw();
                                unsafe { <#type_path as #jni::refs::Reference>::kind_from_raw(raw) }
                            }
                        }

                        // Assuming #type_name<'local> is a transparent wrapper around JObject
                        // (asserted) we can implement `AsRef` by transmuting `&self` to
                        // `&#type_path<'local>`.
                        impl<'local> ::core::convert::AsRef<#type_path<'local>> for #type_name<'local> {
                            fn as_ref(&self) -> &#type_path<'local> {
                                const fn assert_is_instance_of_type_is_ffi_safe<T: #jni::refs::TransparentReference>() {}
                                const _: () = assert_is_instance_of_type_is_ffi_safe::<#type_path<'_>>();
                                unsafe { std::mem::transmute(self) }
                            }
                        }
                    });
                }
            }

            None => {
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!(
                        "Type '{}' used in is_instance_of is not defined in types mapping",
                        type_alias
                    ),
                ));
            }
        }
    }

    Ok(quote! {
        impl<'local> #type_name<'local> {
            #(#is_instance_of_methods)*
        }

        #(#trait_impls)*
    })
}

/// Extract cfg attributes from a list of attributes
fn extract_cfg_attrs(attrs: &[syn::Attribute]) -> Vec<syn::Attribute> {
    attrs
        .iter()
        .filter(|attr| attr.path().is_ident("cfg"))
        .cloned()
        .collect()
}

/// Intermediate representation for ID lookups (method IDs or field IDs)
struct IdLookup {
    /// The field name in the API struct (e.g., "new_method_id", "get_value_field_id")
    field_name: Ident,
    /// The field type (JMethodID or JFieldID)
    field_type: TokenStream,
    /// The Java name to look up (e.g., "<init>", "getValue", "myField")
    java_name: String,
    /// The JNI signature string
    signature: String,
    /// The function to call for lookup (e.g., "get_method_id", "get_static_field_id")
    lookup_fn: Ident,
    /// The cfg attributes to apply to this ID (e.g., #[cfg(feature = "foo")])
    cfg_attrs: Vec<syn::Attribute>,
}

/// Generate field and initialization code from IdLookup descriptors
fn generate_id_fields_and_inits(
    lookups: Vec<IdLookup>,
    jni: &syn::Path,
) -> (Vec<TokenStream>, Vec<TokenStream>) {
    let mut fields = Vec::new();
    let mut inits = Vec::new();

    for lookup in lookups {
        let field_name = lookup.field_name;
        let field_type = lookup.field_type;
        let java_name = &lookup.java_name;
        let signature = &lookup.signature;
        let lookup_fn = lookup.lookup_fn;
        let cfg_attrs = &lookup.cfg_attrs;

        // Create CStr literals for both the name and signature
        let name_cstr = lit_cstr_mutf8(java_name);

        // Add field to API struct with cfg guards
        fields.push(quote! {
            #(#cfg_attrs)*
            #field_name: #field_type,
        });

        // Add initialization with cfg guards
        inits.push(quote! {
            #(#cfg_attrs)*
            #field_name: env.#lookup_fn(class, #jni::strings::JNIStr::from_cstr_unchecked(#name_cstr), #jni::jni_sig!(jni=#jni, #signature))?,
        });
    }

    (fields, inits)
}

/// Generate constructor method ID fields and initialization code
fn generate_constructor_method_ids(
    constructors: &[Constructor],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<(Vec<TokenStream>, Vec<TokenStream>)> {
    let mut lookups = Vec::new();

    for constructor in constructors {
        let name = &constructor.name;
        let method_id_field = format_ident!("{}_method_id", name);

        // Use the MethodSignature to generate JNI signature
        let jni_sig_str = constructor
            .method_signature
            .to_jni_signature(type_mappings)
            .map_err(|e| {
                syn::Error::new(
                    name.span(),
                    format!(
                        "Failed to generate JNI signature for constructor '{}': {}",
                        name, e
                    ),
                )
            })?;

        let cfg_attrs = extract_cfg_attrs(&constructor.attrs);

        lookups.push(IdLookup {
            field_name: method_id_field,
            field_type: quote! { #jni::ids::JMethodID },
            java_name: "<init>".to_string(),
            signature: jni_sig_str,
            lookup_fn: format_ident!("get_method_id"),
            cfg_attrs,
        });
    }

    Ok(generate_id_fields_and_inits(lookups, jni))
}

/// Convert a JavaType to a Rust native method trait argument type with lifetime
/// This variant does NOT use AsRef - it takes reference types by value
fn sig_type_to_rust_native_trait_arg_type(
    sig_type: &SigType,
    lifetime: &TokenStream,
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> TokenStream {
    // For native trait methods, use the concrete type directly without AsRef
    sig_type_to_rust_type_core(sig_type, lifetime, type_mappings, jni)
}

/// Convert a JavaType to a Rust method argument type with lifetime
/// This variant uses impl AsRef for object and array types
fn sig_type_to_rust_arg_type(
    sig_type: &SigType,
    lifetime: &TokenStream,
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> TokenStream {
    // For primitives and void, return the type as-is (no AsRef needed)
    #[allow(clippy::collapsible_if)]
    if let SigType::Alias(alias_name) = sig_type {
        if let Some(ConcreteType::Primitive { .. }) = type_mappings.map_alias(alias_name) {
            return sig_type_to_rust_type_core(sig_type, lifetime, type_mappings, jni);
        }
    }

    // For objects, arrays, and rust references, wrap in impl AsRef
    let concrete_type = sig_type_to_rust_type_core(sig_type, lifetime, type_mappings, jni);
    quote! { impl AsRef<#concrete_type> }
}

/// Convert a JavaType to a Rust method return type with lifetime
fn sig_type_to_rust_return_type(
    sig_type: &SigType,
    lifetime: &TokenStream,
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> TokenStream {
    // Return types are identical to the core type representation
    sig_type_to_rust_type_core(sig_type, lifetime, type_mappings, jni)
}

/// Generate constructor implementations
fn generate_constructors(
    type_name: &Ident,
    api_name: &Ident,
    constructors: &[Constructor],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<TokenStream> {
    let mut constructor_impls = Vec::new();

    for constructor in constructors {
        let name = &constructor.name;
        let method_id_field = format_ident!("{}_method_id", name);

        // Generate JNI call arguments data
        let args =
            generate_jni_call_args(&constructor.method_signature.parameters, type_mappings, jni);

        let lifetimes = &args.lifetimes;
        let decls = &args.decls;
        let jvalue_conversions = &args.jvalue_conversions;

        // Add lifetime declarations for the function
        let lifetime_decls = if lifetimes.is_empty() {
            quote! { <'env_local> }
        } else {
            quote! { <'env_local, #(#lifetimes),*> }
        };

        // Generate explicit type annotation for jni_args array to handle empty case
        let jni_args_type = if jvalue_conversions.is_empty() {
            quote! { : [#jni::sys::jvalue; 0] }
        } else {
            quote! {}
        };

        // Apply attributes to the constructor
        let attrs = &constructor.attrs;

        constructor_impls.push(quote! {
            #(#attrs)*
            pub fn #name #lifetime_decls (
                env: &mut #jni::Env<'env_local>,
                #(#decls),*
            ) -> #jni::errors::Result<#type_name<'env_local>> {
                let api = #api_name::get(env, &#jni::refs::LoaderContext::None)?;
                let jni_args #jni_args_type = [#(#jvalue_conversions),*];

                unsafe {
                    use #jni::refs::Reference as _;

                    // NewObjectA is not exception safe, so we can't go ahead with the JNI call if
                    // there's currently a pending exception.
                    if env.exception_check() {
                        return Err(#jni::errors::Error::JavaException);
                    }

                    // Since we're returning a new local reference we need to assert that the
                    // lifetime we have is associated with the top local reference frame
                    env.assert_top();

                    let class: &#jni::objects::JClass = api.class.as_ref();

                    // Call NewObjectA
                    let env_ptr: *mut #jni::sys::JNIEnv = env.get_raw();
                    let interface: *const #jni::sys::JNINativeInterface_ = *env_ptr;
                    let ret_obj: #jni::sys::jobject = ((*interface).v1_1.NewObjectA)(
                        env_ptr,
                        class.as_raw(),
                        api.#method_id_field.into_raw(),
                        jni_args.as_ptr()
                    );

                    // Check for exception
                    if env.exception_check() {
                        return Err(#jni::errors::Error::JavaException);
                    }

                    Ok(#type_name::from_raw(env, ret_obj))
                }
            }
        });
    }

    if constructor_impls.is_empty() {
        Ok(quote! {})
    } else {
        Ok(quote! {
            impl #type_name<'static> {
                #(#constructor_impls)*
            }
        })
    }
}

/// Generate method ID fields and initialization code for methods
fn generate_method_ids(
    methods: &[Method],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<(Vec<TokenStream>, Vec<TokenStream>)> {
    let mut lookups = Vec::new();

    for method in methods {
        let rust_name = &method.rust_name;
        let java_name = &method.java_name;
        let method_id_field = format_ident!("{}_method_id", rust_name);
        let is_static = method.is_static;

        // Use the MethodSignature to generate JNI signature
        let jni_sig_str = method
            .method_signature
            .to_jni_signature(type_mappings)
            .map_err(|e| {
                syn::Error::new(
                    rust_name.span(),
                    format!(
                        "Failed to generate JNI signature for method '{}': {}",
                        rust_name, e
                    ),
                )
            })?;

        let lookup_fn = if is_static {
            format_ident!("get_static_method_id")
        } else {
            format_ident!("get_method_id")
        };

        let field_type = if is_static {
            quote! { #jni::ids::JStaticMethodID }
        } else {
            quote! { #jni::ids::JMethodID }
        };

        let cfg_attrs = extract_cfg_attrs(&method.attrs);

        lookups.push(IdLookup {
            field_name: method_id_field,
            field_type,
            java_name: java_name.clone(),
            signature: jni_sig_str,
            lookup_fn,
            cfg_attrs,
        });
    }

    Ok(generate_id_fields_and_inits(lookups, jni))
}

/// Generate field ID fields and initialization code for fields
fn generate_field_ids(
    fields: &[Field],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<(Vec<TokenStream>, Vec<TokenStream>)> {
    let mut lookups = Vec::new();

    for field in fields {
        let rust_name = &field.rust_name;
        let java_name = &field.java_name;
        let field_id_field = format_ident!("{}_field_id", rust_name);

        // Use the FieldSignature to generate JNI signature
        let field_sig = field
            .field_signature
            .to_jni_signature(type_mappings)
            .map_err(|e| {
                syn::Error::new(
                    rust_name.span(),
                    format!(
                        "Failed to generate JNI signature for field '{}': {}",
                        rust_name, e
                    ),
                )
            })?;

        let lookup_fn = if field.is_static {
            format_ident!("get_static_field_id")
        } else {
            format_ident!("get_field_id")
        };

        let field_type = if field.is_static {
            quote! { #jni::ids::JStaticFieldID }
        } else {
            quote! { #jni::ids::JFieldID }
        };

        let cfg_attrs = extract_cfg_attrs(&field.attrs);

        lookups.push(IdLookup {
            field_name: field_id_field,
            field_type,
            java_name: java_name.clone(),
            signature: field_sig,
            lookup_fn,
            cfg_attrs,
        });
    }

    Ok(generate_id_fields_and_inits(lookups, jni))
}

/// Helper struct to hold processed JNI call arguments data
struct JniCallArgs {
    lifetimes: Vec<TokenStream>,
    jvalue_conversions: Vec<TokenStream>,
    decls: Vec<TokenStream>,
}

/// Generate JNI call arguments processing data for a method/constructor
/// This consolidates the common logic for processing parameters into jvalue arrays
fn generate_jni_call_args(
    parameters: &[crate::signature::Parameter],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> JniCallArgs {
    // Generate unique lifetime for each argument
    let lifetimes: Vec<_> = (0..parameters.len())
        .map(|i| {
            let lt_name = format!("local_{}", i);
            let lt = syn::Lifetime::new(&format!("'{}", lt_name), Span::call_site());
            quote! { #lt }
        })
        .collect();

    // Use parameter names from the signature
    let names: Vec<_> = parameters.iter().map(|param| param.name.clone()).collect();

    let types: Vec<_> = parameters
        .iter()
        .zip(lifetimes.iter())
        .map(|(param, lt)| sig_type_to_rust_arg_type(&param.ty, lt, type_mappings, jni))
        .collect();

    // Generate jvalue conversion for each argument
    let jvalue_conversions: Vec<_> = parameters
        .iter()
        .zip(names.iter())
        .map(|(param, name)| {
            if let Some(primitive) = param.ty.try_as_primitive(type_mappings) {
                let converter = match primitive {
                    PrimitiveType::Boolean => quote! { #jni::objects::JValue::Bool },
                    PrimitiveType::Byte => quote! { #jni::objects::JValue::Byte },
                    PrimitiveType::Char => quote! { #jni::objects::JValue::Char },
                    PrimitiveType::Short => quote! { #jni::objects::JValue::Short },
                    PrimitiveType::Int => quote! { #jni::objects::JValue::Int },
                    PrimitiveType::Long => quote! { #jni::objects::JValue::Long },
                    PrimitiveType::Float => quote! { #jni::objects::JValue::Float },
                    PrimitiveType::Double => quote! { #jni::objects::JValue::Double },
                    PrimitiveType::Void => quote! { #jni::objects::JValue::Void },
                };
                // Use .into() to cover custom primitive type wrappers
                quote! { #converter((#name).into()).as_jni() }
            } else {
                // For objects and arrays, use as_ref() to get JObject
                quote! { #jni::objects::JValue::Object(#name.as_ref()).as_jni() }
            }
        })
        .collect();

    let decls: Vec<_> = names
        .iter()
        .zip(types.iter())
        .map(|(name, ty)| quote! { #name: #ty })
        .collect();

    JniCallArgs {
        lifetimes,
        jvalue_conversions,
        decls,
    }
}

/// Generate JNI call code for a given return type
#[allow(clippy::too_many_arguments)]
fn generate_jni_call_for_return_type(
    debug_name: &str,
    return_type: &SigType,
    is_static: bool,
    jni: &syn::Path,
    this_or_class: &TokenStream,
    method_id: &TokenStream,
    jni_args: &TokenStream,
    type_mappings: &TypeMappings,
) -> TokenStream {
    let call_prefix = if is_static { "CallStatic" } else { "Call" };

    // Determine the JNI function name and return value handling based on the return type
    let (top_check, call_fn, return_tokens) = if let Some(prim) =
        return_type.try_as_primitive(type_mappings)
    {
        match prim {
            PrimitiveType::Void => {
                let call_fn = format_ident!("{}VoidMethodA", call_prefix);
                (quote! {}, call_fn, quote! { () })
            }
            PrimitiveType::Boolean => {
                let call_fn = format_ident!("{}BooleanMethodA", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Byte => {
                let call_fn = format_ident!("{}ByteMethodA", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Char => {
                let call_fn = format_ident!("{}CharMethodA", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Short => {
                let call_fn = format_ident!("{}ShortMethodA", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Int => {
                let call_fn = format_ident!("{}IntMethodA", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Long => {
                let call_fn = format_ident!("{}LongMethodA", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Float => {
                let call_fn = format_ident!("{}FloatMethodA", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Double => {
                let call_fn = format_ident!("{}DoubleMethodA", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
        }
    } else {
        // For objects, arrays, and rust references, we call CallObjectMethodA
        let call_fn = format_ident!("{}ObjectMethodA", call_prefix);
        let return_type_tokens =
            sig_type_to_rust_return_type(return_type, &quote! { 'env_local }, type_mappings, jni);
        (
            quote! { env.assert_top(); },
            call_fn,
            quote! { unsafe { <#return_type_tokens>::from_raw(env, ret) } },
        )
    };

    // The JNI spec asserts that `Call<Type>MethodA` _must_ not be called with a `null` object reference
    // Note: we assume that the class will never be null when calling static methods
    let null_check = if !is_static {
        let error_msg = format!("Attempted to call '{}' with a null object", debug_name);
        quote! {
            if #this_or_class.is_null() {
                return Err(#jni::errors::Error::NullPtr(#error_msg));
            }
        }
    } else {
        quote! {}
    };

    // Generate the JNI call
    quote! {
        {
            // None of the call functions are exception safe, so we can't go ahead with the JNI call
            // if there's currently a pending exception.
            if env.exception_check() {
                return Err(#jni::errors::Error::JavaException);
            }

            #null_check

            #top_check

            let ret = unsafe {
                use #jni::refs::Reference as _;
                let env_ptr: *mut #jni::sys::JNIEnv = env.get_raw();
                let interface: *const #jni::sys::JNINativeInterface_ = *env_ptr;
                ((*interface).v1_1.#call_fn)(
                    env_ptr,
                    #this_or_class.as_raw(),
                    #method_id,
                    #jni_args.as_ptr()
                )
            };

            if env.exception_check() {
                return Err(#jni::errors::Error::JavaException);
            }

            Ok(#return_tokens)
        }
    }
}

/// Generate method implementations (instance or static methods)
fn generate_methods(
    api_name: &Ident,
    methods: &[Method],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<TokenStream> {
    let mut method_impls = Vec::new();

    for method in methods {
        let rust_name = &method.rust_name;
        let method_id_field = format_ident!("{}_method_id", rust_name);
        let visibility = method.visibility.to_tokens();
        let is_static = method.is_static;

        // Generate JNI call arguments data
        let args = generate_jni_call_args(&method.method_signature.parameters, type_mappings, jni);

        let lifetimes = &args.lifetimes;
        let decls = &args.decls;
        let jvalue_conversions = &args.jvalue_conversions;

        // Determine environment type based on return type (primitives use &Env, objects use &mut Env)
        let env_type = if method
            .method_signature
            .return_type
            .try_as_primitive(type_mappings)
            .is_some()
        {
            quote! { &#jni::Env<'env_local> }
        } else {
            quote! { &mut #jni::Env<'env_local> }
        };

        // Generate return type
        let return_type = sig_type_to_rust_return_type(
            &method.method_signature.return_type,
            &quote! { 'env_local },
            type_mappings,
            jni,
        );

        // Add lifetime declarations for the function
        let lifetime_decls = if lifetimes.is_empty() {
            quote! { <'env_local> }
        } else {
            quote! { <'env_local, #(#lifetimes),*> }
        };

        let self_param = if is_static {
            quote! {}
        } else {
            quote! { &self, }
        };

        // Generate explicit type annotation for jni_args array to handle empty case
        let jni_args_type = if jvalue_conversions.is_empty() {
            quote! { : [#jni::sys::jvalue; 0] }
        } else {
            quote! {}
        };

        // Generate the this_or_class expression
        let this_or_class = if is_static {
            quote! { class }
        } else {
            quote! { self }
        };

        // Generate the method body with JNI call
        let jni_call = generate_jni_call_for_return_type(
            &method.java_name,
            &method.method_signature.return_type,
            is_static,
            jni,
            &this_or_class,
            &quote! { api.#method_id_field.into_raw() },
            &quote! { jni_args },
            type_mappings,
        );

        let class_def = if is_static {
            quote! {
                use #jni::refs::Reference as _;
                let class: &#jni::objects::JClass = api.class.as_ref();
            }
        } else {
            quote! {}
        };

        // Apply attributes to the method
        let attrs = &method.attrs;

        // Generate null check if non_null is true
        let null_check_and_return = if method.non_null {
            quote! {
                #jni_call.and_then(|__result| {
                    use #jni::refs::Reference as _;
                    if __result.is_null() {
                        Err(#jni::errors::Error::NullPtr("Null Object"))
                    } else {
                        Ok(__result)
                    }
                })
            }
        } else {
            quote! {
                #jni_call
            }
        };

        method_impls.push(quote! {
            #(#attrs)*
            #visibility fn #rust_name #lifetime_decls (
                #self_param
                env: #env_type,
                #(#decls),*
            ) -> #jni::errors::Result<#return_type> {
                let api = #api_name::get(env, &#jni::refs::LoaderContext::None)?;
                let jni_args #jni_args_type = [#(#jvalue_conversions),*];

                #class_def
                #null_check_and_return
            }
        });
    }

    // Return just the method implementations without the impl block wrapper
    // The impl block will be created by the caller
    Ok(quote! {
        #(#method_impls)*
    })
}

/// Generate JNI get field call code for a given field type
fn generate_jni_get_field_call(
    debug_name: &str,
    field_type: &SigType,
    is_static: bool,
    jni: &syn::Path,
    this_or_class: &TokenStream,
    field_id: &TokenStream,
    type_mappings: &TypeMappings,
) -> TokenStream {
    let call_prefix = if is_static { "GetStatic" } else { "Get" };

    // Determine the JNI function name and return value handling based on the field type
    let (top_check, call_fn, return_tokens) = if let Some(prim) =
        field_type.try_as_primitive(type_mappings)
    {
        match prim {
            PrimitiveType::Void => {
                // Void fields don't make sense but we handle them for completeness
                return quote! { Ok(()) };
            }
            PrimitiveType::Boolean => {
                let call_fn = format_ident!("{}BooleanField", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Byte => {
                let call_fn = format_ident!("{}ByteField", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Char => {
                let call_fn = format_ident!("{}CharField", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Short => {
                let call_fn = format_ident!("{}ShortField", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Int => {
                let call_fn = format_ident!("{}IntField", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Long => {
                let call_fn = format_ident!("{}LongField", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Float => {
                let call_fn = format_ident!("{}FloatField", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
            PrimitiveType::Double => {
                let call_fn = format_ident!("{}DoubleField", call_prefix);
                (quote! {}, call_fn, quote! { ret })
            }
        }
    } else {
        // For objects, arrays, and rust references, we call GetObjectField
        let call_fn = format_ident!("{}ObjectField", call_prefix);
        let return_type_tokens =
            sig_type_to_rust_return_type(field_type, &quote! { 'env_local }, type_mappings, jni);
        (
            quote! { env.assert_top(); },
            call_fn,
            quote! { unsafe { <#return_type_tokens>::from_raw(env, ret) } },
        )
    };

    // The JNI spec asserts that `Get<Type>Field` _must_ not be called with a `null` object reference
    // Note: we assume that the class will never be null when getting static fields
    let null_check = if !is_static {
        let error_msg = format!("Attempted to set '{}' field with a null object", debug_name);
        quote! {
            if #this_or_class.is_null() {
                return Err(#jni::errors::Error::NullPtr(#error_msg));
            }
        }
    } else {
        quote! {}
    };

    quote! {
        {
            // None of these getter functions are exception safe, so we can't go ahead with the JNI
            // call if there's currently a pending exception.
            if env.exception_check() {
                return Err(#jni::errors::Error::JavaException);
            }

            #null_check

            #top_check

            let ret = unsafe {
                use #jni::refs::Reference as _;
                let env_ptr: *mut #jni::sys::JNIEnv = env.get_raw();
                let interface: *const #jni::sys::JNINativeInterface_ = *env_ptr;
                ((*interface).v1_1.#call_fn)(
                    env_ptr,
                    #this_or_class.as_raw(),
                    #field_id
                )
            };

            if env.exception_check() {
                return Err(#jni::errors::Error::JavaException);
            }

            Ok(#return_tokens)
        }
    }
}

/// Generate JNI set field call code for a given field type
#[allow(clippy::too_many_arguments)]
fn generate_jni_set_field_call(
    debug_name: &str,
    field_type: &SigType,
    is_static: bool,
    jni: &syn::Path,
    this_or_class: &TokenStream,
    field_id: &TokenStream,
    val: &TokenStream,
    type_mappings: &TypeMappings,
) -> TokenStream {
    let call_prefix = if is_static { "SetStatic" } else { "Set" };

    // Determine the JNI function name and value cast type based on the field type
    let (call_fn, val_cast) = if let Some(prim) = field_type.try_as_primitive(type_mappings) {
        match prim {
            PrimitiveType::Void => {
                // Void fields don't make sense but we handle them for completeness
                return quote! { Ok(()) };
            }
            PrimitiveType::Boolean => {
                let call_fn = format_ident!("{}BooleanField", call_prefix);
                (call_fn, quote! { #val as #jni::sys::jboolean })
            }
            PrimitiveType::Byte => {
                let call_fn = format_ident!("{}ByteField", call_prefix);
                (call_fn, quote! { #val as #jni::sys::jbyte })
            }
            PrimitiveType::Char => {
                let call_fn = format_ident!("{}CharField", call_prefix);
                (call_fn, quote! { #val as #jni::sys::jchar })
            }
            PrimitiveType::Short => {
                let call_fn = format_ident!("{}ShortField", call_prefix);
                (call_fn, quote! { #val as #jni::sys::jshort })
            }
            PrimitiveType::Int => {
                let call_fn = format_ident!("{}IntField", call_prefix);
                (call_fn, quote! { #val as #jni::sys::jint })
            }
            PrimitiveType::Long => {
                let call_fn = format_ident!("{}LongField", call_prefix);
                (call_fn, quote! { #val as #jni::sys::jlong })
            }
            PrimitiveType::Float => {
                let call_fn = format_ident!("{}FloatField", call_prefix);
                (call_fn, quote! { #val as #jni::sys::jfloat })
            }
            PrimitiveType::Double => {
                let call_fn = format_ident!("{}DoubleField", call_prefix);
                (call_fn, quote! { #val as #jni::sys::jdouble })
            }
        }
    } else {
        // For objects, arrays, and rust references, we call SetObjectField
        let call_fn = format_ident!("{}ObjectField", call_prefix);
        (call_fn, quote! { #val.as_ref().as_raw() })
    };

    // The JNI spec asserts that `Set<Type>Field` _must_ not be called with a `null` object reference
    // Note: we assume that the class will never be null when setting static fields
    let null_check = if !is_static {
        let error_msg = format!("Attempted to set '{}' field with a null object", debug_name);
        quote! {
            if #this_or_class.is_null() {
                return Err(#jni::errors::Error::NullPtr(#error_msg));
            }
        }
    } else {
        quote! {}
    };

    // Generate the JNI field setter call
    quote! {
        {
            // None of these setter functions are exception safe, so we can't go ahead with the JNI
            // call if there's currently a pending exception.
            if env.exception_check() {
                return Err(#jni::errors::Error::JavaException);
            }

            #null_check

            unsafe {
                use #jni::refs::Reference as _;
                let env_ptr: *mut #jni::sys::JNIEnv = env.get_raw();
                let interface: *const #jni::sys::JNINativeInterface_ = *env_ptr;
                ((*interface).v1_1.#call_fn)(
                    env_ptr,
                    #this_or_class.as_raw(),
                    #field_id,
                    #val_cast
                );
            }

            if env.exception_check() {
                return Err(#jni::errors::Error::JavaException);
            }

            Ok(())
        }
    }
}

/// Generate field getter/setter implementations (without any impl block wrapper)
fn generate_fields(
    api_name: &Ident,
    fields: &[Field],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> TokenStream {
    let mut field_impls = Vec::new();

    for field in fields {
        let rust_name = &field.rust_name;
        let field_id_field = format_ident!("{}_field_id", rust_name);

        // Determine environment type based on field type (primitives use &Env, objects use &mut Env)
        let get_env_type = if field
            .field_signature
            .field_type
            .try_as_primitive(type_mappings)
            .is_some()
        {
            quote! { &#jni::Env<'env_local> }
        } else {
            quote! { &mut #jni::Env<'env_local> }
        };

        // Setters always use &Env (non-mut)
        let set_env_type = quote! { &#jni::Env<'env_local> };

        // Generate return type
        let return_type = sig_type_to_rust_return_type(
            &field.field_signature.field_type,
            &quote! { 'env_local },
            type_mappings,
            jni,
        );

        // Generate the field type lifetime if needed
        let field_lifetime = if field
            .field_signature
            .field_type
            .try_as_primitive(type_mappings)
            .is_some()
        {
            quote! {}
        } else {
            quote! { 'local_field }
        };

        let field_lifetime_decl = if field_lifetime.is_empty() {
            quote! { <'env_local> }
        } else {
            quote! { <'env_local, #field_lifetime> }
        };

        // Generate argument type for setter
        let arg_type = sig_type_to_rust_arg_type(
            &field.field_signature.field_type,
            &field_lifetime,
            type_mappings,
            jni,
        );

        let self_param = if field.is_static {
            quote! {}
        } else {
            quote! { &self, }
        };

        // Generate the this_or_class expression
        let this_or_class = if field.is_static {
            quote! { class }
        } else {
            quote! { self }
        };

        let class_def = if field.is_static {
            quote! {
                use #jni::refs::Reference as _;
                let class: &#jni::objects::JClass = api.class.as_ref();
            }
        } else {
            quote! {}
        };

        if let Some(getter_name) = &field.getter_name {
            let getter_visibility = field.getter_visibility.to_tokens();

            // Always start with cfg attributes from the field itself
            let field_cfg_attrs = extract_cfg_attrs(&field.attrs);

            // Determine which attributes to apply to getter
            // If getter_attrs are explicitly set, use those (plus field cfg)
            // Otherwise, use the field's attrs
            let mut getter_attributes = if !field.getter_attrs.is_empty() {
                // Merge: field cfg attrs + explicit getter attrs
                let mut attrs = field_cfg_attrs.clone();
                attrs.extend(field.getter_attrs.clone());
                attrs
            } else {
                field.attrs.clone()
            };

            if !getter_attributes
                .iter()
                .any(|attr| attr.path().is_ident("doc"))
            {
                // If no doc attribute, add a default one
                let java_name = &field.java_name;
                let doc_attr: syn::Attribute = syn::parse_quote! {
                    #[doc = concat!("Gets the `", #java_name, "` field.")]
                };
                getter_attributes.insert(0, doc_attr);
            }

            // Generate getter
            let get_call = generate_jni_get_field_call(
                &field.java_name,
                &field.field_signature.field_type,
                field.is_static,
                jni,
                &this_or_class,
                &quote! { api.#field_id_field.into_raw() },
                type_mappings,
            );

            // Generate null check if non_null is true
            let get_null_check_and_return = if field.non_null {
                quote! {
                    #get_call.and_then(|__result| {
                        use #jni::refs::Reference as _;
                        if __result.is_null() {
                            Err(#jni::errors::Error::NullPtr("Null Object"))
                        } else {
                            Ok(__result)
                        }
                    })
                }
            } else {
                quote! {
                    #get_call
                }
            };

            field_impls.push(quote! {
                #(#getter_attributes)*
                #getter_visibility fn #getter_name<'env_local>(
                    #self_param
                    env: #get_env_type
                ) -> #jni::errors::Result<#return_type> {
                    let api = #api_name::get(env, &#jni::refs::LoaderContext::None)?;
                    #class_def
                    #get_null_check_and_return
                }
            });
        }

        if let Some(setter_name) = &field.setter_name {
            let setter_visibility = field.setter_visibility.to_tokens();

            // Always start with cfg attributes from the field itself
            let field_cfg_attrs = extract_cfg_attrs(&field.attrs);

            // Determine which attributes to apply to setter
            // If setter_attrs are explicitly set, use those (plus field cfg)
            // Otherwise, use the field's attrs (excluding doc)
            let mut setter_attributes: Vec<syn::Attribute> = if !field.setter_attrs.is_empty() {
                // Merge: field cfg attrs + explicit setter attrs
                let mut attrs = field_cfg_attrs.clone();
                attrs.extend(field.setter_attrs.clone());
                attrs
            } else {
                field
                    .attrs
                    .iter()
                    .filter(|attr| !attr.path().is_ident("doc"))
                    .cloned()
                    .collect()
            };

            if !setter_attributes
                .iter()
                .any(|attr| attr.path().is_ident("doc"))
            {
                // If no doc attribute, add a default one
                let java_name = &field.java_name;
                let getter = &field.getter_name;
                let doc_attr: syn::Attribute = syn::parse_quote! {
                    #[doc = concat!("Sets the `", #java_name, "` field.\n\nSee [`Self::", stringify!(#getter), "`] for more details.")]
                };
                setter_attributes.insert(0, doc_attr);
            }

            // Generate setter
            let set_call = generate_jni_set_field_call(
                &field.java_name,
                &field.field_signature.field_type,
                field.is_static,
                jni,
                &this_or_class,
                &quote! { api.#field_id_field.into_raw() },
                &quote! { val },
                type_mappings,
            );

            // Generate null check if non_null is true
            let set_null_check_and_call = if field.non_null {
                quote! {
                    use #jni::refs::Reference as _;
                    if val.as_ref().is_null() {
                        return Err(#jni::errors::Error::NullPtr("Null Object"));
                    }
                    #set_call
                }
            } else {
                quote! {
                    #set_call
                }
            };

            field_impls.push(quote! {
                #(#setter_attributes)*
                #setter_visibility fn #setter_name #field_lifetime_decl (
                    #self_param
                    env: #set_env_type,
                    val: #arg_type
                ) -> #jni::errors::Result<()> {
                    let api = #api_name::get(env, &#jni::refs::LoaderContext::None)?;
                    #class_def
                    #set_null_check_and_call
                }
            });
        }
    }

    // Return just the field implementations without any impl block wrapper
    quote! {
        #(#field_impls)*
    }
}

/// Generate all native method related code (trait, wrappers, registration)
/// Returns: (trait_and_impl_struct, registration_code)
#[allow(clippy::too_many_arguments)]
fn generate_native_methods_code(
    type_name: &Ident,
    api_name: &Ident,
    type_visibility: &VisibilitySpec,
    java_class_internal: &str,
    java_class_dotted: &str,
    native_methods: &[NativeMethod],
    native_trait_name: &Option<Ident>,
    type_mappings: &TypeMappings,
    native_methods_export: bool,
    native_methods_catch_unwind: bool,
    default_abi_check: AbiCheck,
    native_methods_error_policy: &Option<syn::Path>,
    jni: &syn::Path,
) -> Result<(TokenStream, TokenStream)> {
    // If there are no native methods, return empty
    if native_methods.is_empty() {
        return Ok((quote! {}, quote! {}));
    }

    // Generate default names if not provided
    let default_trait_name = format_ident!("{}NativeInterface", type_name);

    let trait_name = native_trait_name.as_ref().unwrap_or(&default_trait_name);

    // Generate the native trait
    let native_trait = generate_native_trait(
        trait_name,
        type_name,
        type_visibility,
        native_methods,
        type_mappings,
        jni,
    );

    // Generate wrapper methods (to be added to impl API block)
    let wrapper_methods = generate_native_wrappers(
        type_name,
        api_name,
        java_class_internal,
        native_methods,
        trait_name,
        type_mappings,
        native_methods_catch_unwind,
        default_abi_check,
        native_methods_error_policy,
        jni,
    );

    // Wrap the wrappers in an impl block for the API struct
    let wrapper_impl = quote! {
        impl #api_name {
            #wrapper_methods
        }
    };

    // Generate registration code (to be added to API::get())
    let registration_code =
        generate_native_registration_code(api_name, native_methods, type_mappings, jni)?;

    // Generate exported native method functions for methods marked with export = true
    let native_exports = generate_native_exports(
        type_name,
        api_name,
        java_class_dotted,
        native_methods,
        native_methods_export,
        type_mappings,
        jni,
    )?;

    // Combine trait, impl struct, wrapper impl, and exports for top-level output
    let trait_and_impl = quote! {
        #native_trait
        #wrapper_impl
        #native_exports
    };

    Ok((trait_and_impl, registration_code))
}

/// Generate the native methods trait
fn generate_native_trait(
    trait_name: &Ident,
    type_name: &Ident,
    type_visibility: &VisibilitySpec,
    native_methods: &[NativeMethod],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> TokenStream {
    let mut method_sigs = Vec::new();

    // Generate instance native method signatures
    for method in native_methods {
        // Skip methods with direct function bindings - they bypass the trait
        if method.native_fn.is_some() {
            continue;
        }

        let rust_name = &method.rust_name;
        let lifetime = quote! { 'local };

        let mut params = Vec::new();

        // Raw methods take EnvUnowned directly, regular methods take &mut Env
        if method.is_raw {
            params.push(quote! { unowned_env: #jni::EnvUnowned<'local> });
        } else {
            params.push(quote! { env: &mut #jni::Env<'local> });
        }

        if method.is_static {
            params.push(quote! { class: #jni::objects::JClass<'local> });
        } else {
            params.push(quote! { this: #type_name<'local> });
        }

        for param in &method.method_signature.parameters {
            let param_name = &param.name;
            // Use the native trait arg type (without AsRef)
            let rust_type =
                sig_type_to_rust_native_trait_arg_type(&param.ty, &lifetime, type_mappings, jni);
            params.push(quote! { #param_name: #rust_type });
        }

        let return_type = sig_type_to_rust_return_type(
            &method.method_signature.return_type,
            &lifetime,
            type_mappings,
            jni,
        );

        // Only emit cfg attributes on trait methods (not doc or other attributes)
        let cfg_attrs = extract_cfg_attrs(&method.attrs);

        // Raw methods return the value directly, regular methods return Result
        if method.is_raw {
            method_sigs.push(quote! {
                #(#cfg_attrs)*
                fn #rust_name<'local>(#(#params),*) -> #return_type;
            });
        } else {
            method_sigs.push(quote! {
                #(#cfg_attrs)*
                fn #rust_name<'local>(#(#params),*) -> ::std::result::Result<#return_type, Self::Error>;
            });
        }
    }

    // If there are no methods that need trait implementations, don't generate the trait
    if method_sigs.is_empty() {
        return quote! {};
    }

    let trait_visibility = VisibilitySpec::PubCrate
        .clamp_to(type_visibility)
        .to_tokens();
    quote! {
        /// Native methods trait for user implementation
        #trait_visibility trait #trait_name {
            type Error: From<#jni::errors::Error>;
            #(#method_sigs)*
        }
    }
}

/// Generate wrapper functions for native methods as API struct methods
#[allow(clippy::too_many_arguments)]
fn generate_native_wrappers(
    type_name: &Ident,
    api_name: &Ident,
    _java_class_internal: &str,
    native_methods: &[NativeMethod],
    trait_name: &Ident,
    type_mappings: &TypeMappings,
    default_native_methods_catch_unwind: bool,
    default_abi_check: AbiCheck,
    default_error_policy: &Option<syn::Path>,
    jni: &syn::Path,
) -> TokenStream {
    let mut wrappers = Vec::new();

    for method in native_methods {
        let wrapper = generate_single_native_wrapper(
            type_name,
            method,
            trait_name,
            api_name,
            type_mappings,
            default_native_methods_catch_unwind,
            default_abi_check,
            default_error_policy,
            jni,
        );
        wrappers.push(wrapper);
    }

    quote! {
        #(#wrappers)*
    }
}

/// Generate a single native method wrapper as an API struct method
#[allow(clippy::too_many_arguments)]
fn generate_single_native_wrapper(
    type_name: &Ident,
    method: &NativeMethod,
    trait_name: &Ident,
    api_name: &Ident,
    type_mappings: &TypeMappings,
    default_native_methods_catch_unwind: bool,
    default_abi_check: AbiCheck,
    default_error_policy: &Option<syn::Path>,
    jni: &syn::Path,
) -> TokenStream {
    let rust_name = &method.rust_name;
    let wrapper_name = format_ident!("{}_native_method", rust_name);

    let lifetime = quote! { 'local };

    // Build parameter list for extern "system" fn
    let mut params = Vec::new();
    params.push(quote! { mut unowned_env: #jni::EnvUnowned<#lifetime> });

    if method.is_static {
        params.push(quote! { class: #jni::objects::JClass<#lifetime> });
    } else {
        params.push(quote! { this: #type_name<#lifetime> });
    }

    // Add method parameters using parameter names from signature
    // Use native trait arg type (without AsRef) to match the trait signature
    for param in &method.method_signature.parameters {
        let param_name = &param.name;
        let rust_type =
            sig_type_to_rust_native_trait_arg_type(&param.ty, &lifetime, type_mappings, jni);
        params.push(quote! { #param_name: #rust_type });
    }

    // Get return type
    let return_type = sig_type_to_rust_return_type(
        &method.method_signature.return_type,
        &lifetime,
        type_mappings,
        jni,
    );

    let need_abi_check = method.abi_check.unwrap_or(default_abi_check);
    let abi_check = generate_native_method_abi_check(
        jni,
        &method.java_name,
        need_abi_check,
        method.is_raw,
        method.is_static,
        None, // type mappings are checked in the TypeAPI::get() method
    );

    let with_env_api = if method
        .catch_unwind
        .unwrap_or(default_native_methods_catch_unwind)
    {
        quote! { with_env }
    } else {
        quote! { with_env_no_catch }
    };

    // Generate the wrapper body based on native_fn and is_raw combination
    let wrapper_body = match (&method.native_fn, method.is_raw) {
        // Direct raw function (fn = raw path)
        (Some(raw_fn), true) => {
            // Call the function directly with unowned env - no with_env wrapper
            let mut call_args = vec![quote! { unowned_env }];
            if !method.is_static {
                call_args.push(quote! { this });
            } else {
                call_args.push(quote! { class });
            }
            for param in &method.method_signature.parameters {
                let param_name = &param.name;
                call_args.push(quote! { #param_name });
            }

            quote! {
                #abi_check

                #raw_fn(#(#call_args),*)
            }
        }
        // Direct safe function (fn = path)
        (Some(safe_fn), false) => {
            // Wrap with EnvUnowned::with_env and error handling but skip trait
            let mut call_args = vec![quote! { env }];
            if !method.is_static {
                call_args.push(quote! { this });
            } else {
                call_args.push(quote! { class });
            }
            for param in &method.method_signature.parameters {
                let param_name = &param.name;
                call_args.push(quote! { #param_name });
            }

            // Get error policy with fallback chain:
            // 1. method.error_policy if specified
            // 2. default_error_policy if specified
            // 3. jni::errors::ThrowRuntimeExAndDefault
            let error_policy = method
                .error_policy
                .as_ref()
                .or(default_error_policy.as_ref())
                .map(|p| quote! { #p })
                .unwrap_or_else(|| quote! { #jni::errors::ThrowRuntimeExAndDefault });

            quote! {
                unowned_env
                    .#with_env_api(|env| {
                        #abi_check

                        #safe_fn(#(#call_args),*)
                    })
                    .resolve::<#error_policy>()
            }
        }
        // Trait-based raw method (raw = true, no fn)
        (None, true) => {
            // Call trait method directly without with_env wrapper
            let mut call_args = vec![quote! { unowned_env }];
            if !method.is_static {
                call_args.push(quote! { this });
            } else {
                call_args.push(quote! { class });
            }
            for param in &method.method_signature.parameters {
                let param_name = &param.name;
                call_args.push(quote! { #param_name });
            }

            quote! {
                #abi_check

                <#api_name as #trait_name>::#rust_name(#(#call_args),*)
            }
        }
        // Trait-based safe method (default: no fn, raw = false)
        (None, false) => {
            // Use trait method with with_env wrapper and error handling
            let mut call_args = vec![quote! { env }];
            if !method.is_static {
                call_args.push(quote! { this });
            } else {
                call_args.push(quote! { class });
            }
            for param in &method.method_signature.parameters {
                let param_name = &param.name;
                call_args.push(quote! { #param_name });
            }

            // Get error policy with fallback chain:
            // 1. method.error_policy if specified
            // 2. default_error_policy if specified
            // 3. jni::errors::ThrowRuntimeExAndDefault
            let error_policy = method
                .error_policy
                .as_ref()
                .or(default_error_policy.as_ref())
                .map(|p| quote! { #p })
                .unwrap_or_else(|| quote! { #jni::errors::ThrowRuntimeExAndDefault });

            quote! {
                unowned_env
                    .#with_env_api(|env| {
                        #abi_check

                        <#api_name as #trait_name>::#rust_name(#(#call_args),*)
                    })
                    .resolve::<#error_policy>()
            }
        }
    };

    // Extract cfg attributes to apply to the wrapper function
    let cfg_attrs = extract_cfg_attrs(&method.attrs);

    quote! {
        #(#cfg_attrs)*
        #[allow(unused)]
        extern "system" fn #wrapper_name<#lifetime>(
            #(#params),*
        ) -> #return_type {
            #wrapper_body
        }
    }
}

/// Generate the native methods registration code for API::get()
fn generate_native_registration_code(
    _api_name: &Ident,
    native_methods: &[NativeMethod],
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<TokenStream> {
    // If no native methods, return empty
    if native_methods.is_empty() {
        return Ok(quote! {});
    }

    let mut native_method_descriptors = Vec::new();

    for method in native_methods {
        let java_name = &method.java_name;
        let rust_name = &method.rust_name;

        // Use MethodSignature to generate JNI signature
        let jni_signature = method
            .method_signature
            .to_jni_signature(type_mappings)
            .map_err(|e| {
                syn::Error::new(
                    rust_name.span(),
                    format!(
                        "Failed to generate JNI signature for native method '{}': {}",
                        rust_name, e
                    ),
                )
            })?;

        // Create CStr literals for name and signature
        let name_cstr = lit_cstr_mutf8(java_name);
        let sig_cstr = lit_cstr_mutf8(&jni_signature);

        // Always use the generated wrapper function (even for raw_fn)
        // This ensures type checking and allows exports to work
        let wrapper_name = format_ident!("{}_native_method", rust_name);
        let fn_ptr = quote! { Self::#wrapper_name as *mut ::std::ffi::c_void };

        // Extract cfg attributes to guard the registration descriptor
        let cfg_attrs = extract_cfg_attrs(&method.attrs);

        native_method_descriptors.push(quote! {
            #(#cfg_attrs)*
            #jni::NativeMethod::from_raw_parts(
                #jni::strings::JNIStr::from_cstr_unchecked(#name_cstr),
                #jni::strings::JNIStr::from_cstr_unchecked(#sig_cstr),
                #fn_ptr,
            )
        });
    }

    Ok(quote! {
        {
            // Safety: All of the name and signature CStr literals have been validate
            // at compile time and encoded as MUTF-8 - therefore they can be safely
            // cast to a JNIStr unchecked at runtime.
            unsafe {
                let native_methods = &[
                    #(#native_method_descriptors),*
                ];
                env.register_native_methods(class, native_methods)?;
            }
        }
    })
}

/// Generate exported native method functions for methods marked with export = true
#[allow(clippy::too_many_arguments)]
fn generate_native_exports(
    type_name: &Ident,
    api_name: &Ident,
    java_class_dotted: &str,
    native_methods: &[NativeMethod],
    global_native_methods_export: bool,
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<TokenStream> {
    let mut exports = Vec::new();

    for method in native_methods {
        // Determine if this method should be exported
        let should_export = match &method.export {
            NativeMethodExport::Default => global_native_methods_export,
            NativeMethodExport::No => false,
            NativeMethodExport::WithAutoMangle | NativeMethodExport::WithName(_) => true,
        };

        if !should_export {
            continue;
        }

        // Determine the export name (None = auto-mangle)
        let export_name = match &method.export {
            NativeMethodExport::WithName(name) => Some(name.clone()),
            _ => None,
        };

        let export = generate_single_native_export(
            type_name,
            api_name,
            java_class_dotted,
            method,
            export_name,
            type_mappings,
            jni,
        )?;
        exports.push(export);
    }

    Ok(quote! {
        #(#exports)*
    })
}

/// Generate a single exported native method function
#[allow(clippy::too_many_arguments)]
fn generate_single_native_export(
    type_name: &Ident,
    api_name: &Ident,
    java_class_dotted: &str,
    method: &NativeMethod,
    export_name: Option<String>,
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> Result<TokenStream> {
    let rust_name = &method.rust_name;
    let wrapper_name = format_ident!("{}_native_method", rust_name);

    // Use custom export name if provided, otherwise use java_name
    let java_method_name = export_name.as_ref().unwrap_or(&method.java_name);

    // Use MethodSignature to generate JNI signature for mangling
    let jni_signature = method
        .method_signature
        .to_jni_signature(type_mappings)
        .map_err(|e| {
            syn::Error::new(
                rust_name.span(),
                format!(
                    "Failed to generate JNI signature for exported native method '{}': {}",
                    rust_name, e
                ),
            )
        })?;

    // Generate the mangled JNI function name
    let mangled_name = if let Some(export_name) = export_name {
        export_name.clone()
    } else {
        create_jni_fn_name(java_class_dotted, java_method_name, Some(&jni_signature))
    };
    let mangled_ident = format_ident!("{}", mangled_name);

    let lifetime = quote! { 'local };

    // Build parameter list for export function (same as wrapper)
    let mut params = Vec::new();
    params.push(quote! { mut unowned_env: #jni::EnvUnowned<#lifetime> });

    if method.is_static {
        params.push(quote! { class: #jni::objects::JClass<#lifetime> });
    } else {
        params.push(quote! { this: #type_name<#lifetime> });
    }

    // Add method parameters
    for param in &method.method_signature.parameters {
        let param_name = &param.name;
        let rust_type =
            sig_type_to_rust_native_trait_arg_type(&param.ty, &lifetime, type_mappings, jni);
        params.push(quote! { #param_name: #rust_type });
    }

    // Build forwarding call arguments
    let mut call_args = vec![quote! { unowned_env }];
    if !method.is_static {
        call_args.push(quote! { this });
    } else {
        call_args.push(quote! { class });
    }
    for param in &method.method_signature.parameters {
        let param_name = &param.name;
        call_args.push(quote! { #param_name });
    }

    // Get return type
    let return_type = sig_type_to_rust_return_type(
        &method.method_signature.return_type,
        &lifetime,
        type_mappings,
        jni,
    );

    let no_mangle_attr = if cfg!(has_unsafe_attr) {
        quote! { #[unsafe(no_mangle)] }
    } else {
        quote! { #[no_mangle] }
    };

    let cfg_attrs = extract_cfg_attrs(&method.attrs);

    Ok(quote! {
        #(#cfg_attrs)*
        #[doc(hidden)]
        #no_mangle_attr
        #[allow(non_snake_case)]
        pub unsafe extern "system" fn #mangled_ident<#lifetime>(
            #(#params),*
        ) -> #return_type {
            #api_name::#wrapper_name(#(#call_args),*)
        }
    })
}
