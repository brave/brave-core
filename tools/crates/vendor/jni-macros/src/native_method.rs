//! Procedural macros for generating compile-time type-checked NativeMethod
//! structs
//!
//! This module implements the `native_method!` macro that create `NativeMethod`
//! descriptors with compile-time guarantees that the function pointer matches
//! the JNI signature.

use proc_macro2::TokenStream;
use quote::quote;
use syn::{
    Ident, Result, Token, custom_keyword,
    ext::IdentExt,
    parse::{Parse, ParseStream},
};

use crate::types::{TypeMappings, parse_type};
use crate::types::{generate_type_mapping_checks, sig_type_to_rust_type_core};
use crate::{
    mangle::{create_jni_fn_name, snake_case_to_lower_camel_case},
    types::SigType,
};
use crate::{
    signature::{MethodSignature, parse_method_sig, parse_parameter_with_index},
    types::AbiCheck,
};
use crate::{str::lit_cstr_mutf8, types::JavaClassName};

custom_keyword!(jni);
custom_keyword!(rust_type);
custom_keyword!(java_type);
custom_keyword!(name);
custom_keyword!(sig);
custom_keyword!(type_map);
custom_keyword!(error);
custom_keyword!(error_policy);
custom_keyword!(export);
custom_keyword!(raw);
custom_keyword!(abi_check);
custom_keyword!(catch_unwind);

/// Represents the export behavior for a native method
#[derive(Clone)]
pub enum NativeMethodExport {
    /// Use the default behavior
    #[allow(unused)]
    Default,
    /// Don't export this method (override global setting)
    No,
    /// Export with automatically mangled JNI name
    WithAutoMangle,
    /// Export with explicitly provided name
    WithName(String),
}

/// Input structure for native_method!
struct NativeMethodStructInput {
    #[allow(dead_code)]
    is_static: bool,
    jni_crate: syn::Path,
    rust_type: Option<syn::Path>,
    /// Java class name for export (e.g., "com.example.MyClass")
    java_type: Option<JavaClassName>,
    java_method_name: String,
    method_signature: MethodSignature,
    fn_path: syn::Path,
    type_mappings: TypeMappings,
    /// true if `raw = true` or `raw` qualifier with inline signature
    is_raw_fn: bool,
    /// Only used when is_raw_fn is false
    error_policy: Option<Ident>,
    /// `WithAutoMangle` if `export = true` or `extern` qualifier with inline signature
    export: NativeMethodExport,
    /// ABI check policy, for checking the type of the 'this'/'class' parameter at runtime
    abi_check: AbiCheck,
    /// non-raw wrapper uses `EnvUnowned::with_env` if true, else `with_env_no_catch`
    catch_unwind: bool,
}

impl NativeMethodStructInput {
    fn parse(input: ParseStream) -> Result<Self> {
        // Try to detect jni crate path using parse_jni_crate_override
        let jni_crate = crate::utils::parse_jni_crate_override(&input)?;

        let mut rust_type_opt = None;
        let mut java_type_opt = None;
        let mut type_mappings = TypeMappings::new(&jni_crate);
        let mut error_policy_ident = None;

        let mut is_static_fn = false;
        let mut is_raw_fn = false;
        let mut export_opt = NativeMethodExport::No;
        let mut abi_check = AbiCheck::default();
        let mut catch_unwind = None;

        let mut java_method_name = None;
        let mut method_signature = None;
        let mut inline_fn_path = None;
        let mut fn_path = None;

        // Parse properties and inline signature
        while !input.is_empty() {
            // Try to peek for <ident> = pattern or <ident> { ... }
            // (anything else is treated as inline signature)
            let fork = input.fork();
            let is_prop = if let Ok(_ident) = fork.call(Ident::parse_any) {
                fork.peek(Token![=]) || fork.peek(syn::token::Brace)
            } else {
                false
            };

            if is_prop {
                // Parse as property: <ident> = <value> or <ident> { ... }
                let lookahead = input.lookahead1();

                if lookahead.peek(rust_type) {
                    // Parse: rust_type = Type
                    input.parse::<rust_type>()?;
                    input.parse::<Token![=]>()?;
                    rust_type_opt = Some(input.parse::<syn::Path>()?);
                } else if lookahead.peek(java_type) {
                    // Parse: java_type = "com.example.MyClass"
                    input.parse::<java_type>()?;
                    input.parse::<Token![=]>()?;
                    let class_name = input.parse::<JavaClassName>()?;
                    java_type_opt = Some(class_name);
                } else if lookahead.peek(name) {
                    // Parse: name = "methodName"
                    input.parse::<name>()?;
                    input.parse::<Token![=]>()?;
                    let lit = input.parse::<syn::LitStr>()?;
                    java_method_name = Some(lit.value());
                } else if lookahead.peek(sig) {
                    // Parse: sig = (args) -> ret
                    method_signature = Some(parse_method_sig(input, &type_mappings)?);
                } else if lookahead.peek(Token![static]) {
                    // Parse: static = true/false
                    input.parse::<Token![static]>()?;
                    input.parse::<Token![=]>()?;
                    let lit = input.parse::<syn::LitBool>()?;
                    is_static_fn = lit.value();
                } else if lookahead.peek(raw) {
                    // Parse: raw = true/false
                    input.parse::<raw>()?;
                    input.parse::<Token![=]>()?;
                    let lit = input.parse::<syn::LitBool>()?;
                    is_raw_fn = lit.value();
                } else if lookahead.peek(export) {
                    // Parse: export = true/false/"name"
                    input.parse::<export>()?;
                    input.parse::<Token![=]>()?;
                    if input.peek(syn::LitStr) {
                        // export = "customName"
                        let name = input.parse::<syn::LitStr>()?.value();
                        export_opt = NativeMethodExport::WithName(name);
                    } else if input.peek(syn::LitBool) {
                        // export = true or export = false
                        let lit_bool = input.parse::<syn::LitBool>()?;
                        export_opt = if lit_bool.value() {
                            NativeMethodExport::WithAutoMangle
                        } else {
                            NativeMethodExport::No
                        };
                    } else {
                        return Err(syn::Error::new(
                            input.span(),
                            "export must be 'true', 'false', or a string literal",
                        ));
                    }
                } else if lookahead.peek(self::abi_check) {
                    // Parse: abi_check = Always | UnsafeNever | UnsafeDebugOnly
                    input.parse::<self::abi_check>()?;
                    input.parse::<Token![=]>()?;
                    abi_check = input.parse::<AbiCheck>()?;
                } else if lookahead.peek(self::catch_unwind) {
                    // Parse: catch_unwind = true/false
                    input.parse::<self::catch_unwind>()?;
                    input.parse::<Token![=]>()?;
                    let lit = input.parse::<syn::LitBool>()?;
                    catch_unwind = Some(lit.value());
                } else if lookahead.peek(Token![fn]) {
                    // Parse: fn = path_to_fn
                    input.parse::<Token![fn]>()?;
                    input.parse::<Token![=]>()?;

                    fn_path = Some(input.parse::<syn::Path>()?);
                } else if lookahead.peek(error_policy) {
                    // Parse: error_policy = ErrorPolicyIdent
                    input.parse::<error_policy>()?;
                    input.parse::<Token![=]>()?;
                    error_policy_ident = Some(input.parse::<Ident>()?);
                } else if lookahead.peek(type_map) {
                    // Parse: type_map = { ... }
                    input.parse::<type_map>()?;
                    type_mappings.parse_mappings(input)?;
                } else {
                    return Err(lookahead.error());
                }
            } else {
                // Parse inline signature: [static] [raw] [extern] fn [Path::][method](args) -> ret

                // Parse qualifiers in any order (static, raw, extern)
                loop {
                    if input.peek(Token![static]) {
                        input.parse::<Token![static]>()?;
                        is_static_fn = true;
                    } else if input.peek(Token![extern]) {
                        input.parse::<Token![extern]>()?;
                        // give export = true | "name" higher precedence
                        if matches!(export_opt, NativeMethodExport::No) {
                            export_opt = NativeMethodExport::WithAutoMangle;
                        }
                    } else if input.peek(raw) {
                        input.parse::<raw>()?;
                        is_raw_fn = true;
                    } else {
                        break;
                    }
                }

                // Require 'fn' keyword for both shorthand and block syntax
                if !input.peek(Token![fn]) {
                    return Err(syn::Error::new(
                        input.span(),
                        "Expected 'fn' keyword before RustType::method_name",
                    ));
                }
                input.parse::<Token![fn]>()?;

                // Try to parse a path followed by parentheses
                let full_path = input.parse::<syn::Path>()?;
                inline_fn_path = Some(full_path.clone());

                // Check if this path has multiple segments (Type::method pattern)
                let segments: Vec<_> = full_path.segments.iter().collect();

                let (type_path_opt, method_name_from_path) = if segments.len() >= 2 {
                    // Split into type path and method name
                    let method_ident = &segments[segments.len() - 1].ident;
                    let method_name_snake = method_ident.to_string();

                    // Build the type path (everything except the last segment)
                    let type_segments = &segments[..segments.len() - 1];
                    let mut type_path = syn::Path {
                        leading_colon: full_path.leading_colon,
                        segments: syn::punctuated::Punctuated::new(),
                    };
                    for seg in type_segments {
                        type_path.segments.push((*seg).clone());
                    }

                    (Some(type_path), method_name_snake)
                } else {
                    // Single segment - just a method name
                    (None, segments[0].ident.to_string())
                };

                // Parse method signature: (args) -> ret
                let args_content;
                syn::parenthesized!(args_content in input);

                let mut parameters = Vec::new();
                let mut param_index = 0;

                while !args_content.is_empty() {
                    let param =
                        parse_parameter_with_index(&args_content, param_index, &type_mappings)?;
                    parameters.push(param);
                    param_index += 1;

                    if !args_content.is_empty() {
                        args_content.parse::<Token![,]>()?;
                    }
                }

                let return_type = if input.peek(Token![->]) {
                    input.parse::<Token![->]>()?;
                    parse_type(input, &type_mappings)?
                } else {
                    // No return type means this is constructor shorthand - use void
                    SigType::Alias("void".to_string())
                };

                method_signature = Some(MethodSignature {
                    parameters,
                    return_type,
                });

                // Set values from parsed signature
                if type_path_opt.is_some() && rust_type_opt.is_none() {
                    rust_type_opt = type_path_opt;
                }

                if java_method_name.is_none() {
                    // Convert snake_case to lowerCamelCase for Java method name
                    java_method_name = Some(snake_case_to_lower_camel_case(&method_name_from_path));
                }

                // fn = <path> takes precedence over the default RustType::method_name path
                if fn_path.is_none() {
                    fn_path = full_path.clone().into();
                } else if fn_path.is_none() {
                    // Default to the full path if no => was given
                    fn_path = Some(full_path.clone());
                }
            }

            // Optional trailing comma
            if !input.is_empty() {
                input.parse::<Token![,]>()?;
            }
        }

        // Validate required properties
        let method_name = java_method_name.ok_or_else(|| {
            syn::Error::new(
                input.span(),
                "Missing method name (use 'name = \"javaCamelCaseName\"' or inline signature like 'fn RustType::snake_case_method_name()')",
            )
        })?;
        let method_signature = method_signature.ok_or_else(|| {
            syn::Error::new(
                input.span(),
                "Missing method signature (use 'sig = (args...) -> ret' or inline signature like 'fn RustType::method_name(args...) -> ret')",
            )
        })?;
        // fn = <path> takes precedence over the default RustType::method_name path from an inline signature
        let fn_path = if let Some(fn_path) = fn_path {
            fn_path
        } else {
            if inline_fn_path.is_none() {
                return Err(syn::Error::new(
                    input.span(),
                    "Missing 'fn' and can't infer a default function path without an inline signature like 'fn RustType::method_name(args..) -> ret'.",
                ));
            }
            inline_fn_path.clone().unwrap()
        };

        // Validate: java_type required if export = true
        if matches!(export_opt, NativeMethodExport::WithAutoMangle) && java_type_opt.is_none() {
            return Err(syn::Error::new(
                input.span(),
                "java_type = \"...\" is required when 'export = true' or 'extern' qualifier is used in signature",
            ));
        }

        // Validate that error / error_policy / catch_unwind are not specified with raw = true
        if is_raw_fn {
            if error_policy_ident.is_some() {
                return Err(syn::Error::new(
                    input.span(),
                    "Cannot specify 'error_policy' when using 'raw = true' or 'raw' in signature - error_policy only applies to wrapped functions",
                ));
            }
            if catch_unwind.is_some() {
                return Err(syn::Error::new(
                    input.span(),
                    "Cannot specify 'catch_unwind' when using 'raw = true' or 'raw' in signature - catch_unwind only applies to wrapped functions",
                ));
            }
        }

        Ok(NativeMethodStructInput {
            is_static: is_static_fn,
            jni_crate,
            rust_type: rust_type_opt,
            java_type: java_type_opt,
            java_method_name: method_name,
            method_signature,
            fn_path,
            type_mappings,
            is_raw_fn,
            error_policy: error_policy_ident,
            export: export_opt,
            abi_check,
            catch_unwind: catch_unwind.unwrap_or(true),
        })
    }
}

impl Parse for NativeMethodStructInput {
    fn parse(input: ParseStream) -> Result<Self> {
        Self::parse(input)
    }
}

pub fn generate_native_method_abi_check(
    jni: &syn::Path,
    method_name: &str,
    need_abi_check: AbiCheck,
    is_raw: bool,
    is_static: bool,
    check_type_mappings: Option<&TypeMappings>,
) -> TokenStream {
    if !need_abi_check.requires_abi_check() {
        return quote! {};
    }

    // Check at runtime that the method was correctly registered as static or instance
    // We assume this can't change dynamically so we use Once to only check once
    let abi_check_ensure_env = if is_raw {
        quote! {
            let mut _guard = unsafe {
                #jni::AttachGuard::from_unowned(unowned_env.as_raw())
            };
            let env: &mut #jni::Env<'local> = _guard.borrow_env_mut();
        }
    } else {
        quote! {}
    };
    let abi_assertion = if is_static {
        quote! {
            let is_class_reciever = env.is_instance_of(&class, lang_class_class).expect("Failed to check 2nd arg type for native method ABI check");
            assert!(is_class_reciever, "Native method '{}' was registered as static but called as instance method", stringify!(#method_name));
        }
    } else {
        quote! {
            let is_class_reciever = env.is_instance_of(&this, lang_class_class).expect("Failed to check 2nd arg type for native method ABI check");
            assert!(!is_class_reciever, "Native method '{}' was registered as instance but called as static method", stringify!(#method_name));
        }
    };

    let type_mapping_runtime_checks = if let Some(type_mappings) = check_type_mappings {
        generate_type_mapping_checks(type_mappings, jni)
    } else {
        quote! {}
    };

    quote! {
        // Use atomic fetch_update to guard the ABI check. The closure runs without holding
        // any lock, so multiple threads may race and perform the check concurrently on first
        // invocation. This is acceptable since the check should always succeed or fail consistently.
        //
        // Critically: we only set the flag to true if the checks succeed. If a check fails and
        // panics, the flag remains false, forcing subsequent threads to retry the check (in case
        // the program doesn't abort).
        //
        // This approach avoids locks that could cause deadlocks if the check triggers class
        // initialization which calls back into native methods.
        static _ABI_CHECK_DONE: ::std::sync::atomic::AtomicBool = ::std::sync::atomic::AtomicBool::new(false);

        // The closure returns Some(true) only after the check succeeds.
        // If the check panics, the flag stays false and other attempts to call this method will
        // retry the check (in case the program doesn't abort).
        let _ = _ABI_CHECK_DONE.fetch_update(
            ::std::sync::atomic::Ordering::AcqRel,   // set ordering
            ::std::sync::atomic::Ordering::Acquire,   // fetch ordering
            |done| {
                if done {
                    // The checks have already been completed successfully
                    None
                } else {
                    // Perform the ABI check (runs outside any lock)
                    #abi_check_ensure_env

                    // Note: The `jni` crate has a MSRV that lets us assume it's safe to panic and unwind
                    // up to a JNI boundary and then abort. Therefore we can use expect + assertions here.
                    let lang_class_class = <#jni::objects::JClass as #jni::refs::Reference>::lookup_class(
                        env,
                        &#jni::refs::LoaderContext::None,
                    ).expect("Failed to lookup Java class for java.lang.class, for native method ABI check");
                    let lang_class_class: &#jni::objects::JClass = &lang_class_class;
                    #abi_assertion
                    #type_mapping_runtime_checks

                    // Check succeeded - set flag to true
                    Some(true)
                }
            }
        );
        // If fetch_update returned Ok, we successfully set the flag (check passed)
        // If it returned Err, another thread beat us to it (also check passed)
        // If the check failed, we panicked and never reached here
    }
}

/// Generate the NativeMethod struct creation
pub fn native_method_impl(input: TokenStream) -> Result<TokenStream> {
    let input: NativeMethodStructInput = syn::parse2(input)?;

    let is_static = input.is_static;
    let jni = &input.jni_crate;
    let java_method_name = &input.java_method_name;
    let fn_path = &input.fn_path;
    let type_mappings = &input.type_mappings;

    // Generate JNI signature
    let jni_signature = input
        .method_signature
        .to_jni_signature(type_mappings)
        .map_err(|e| {
            syn::Error::new(
                proc_macro2::Span::call_site(),
                format!("Failed to generate JNI signature: {}", e),
            )
        })?;

    // Create CStr literals for name and signature
    let name_cstr = lit_cstr_mutf8(java_method_name);
    let sig_cstr = lit_cstr_mutf8(&jni_signature);

    // Generate the type-checked wrapper function
    let lifetime = quote! { 'local };

    // Build parameter list - differs for raw vs wrapped functions
    // For raw functions: extern "system" signature with EnvUnowned<'local> (no &mut)
    // For wrapped functions: the user's function takes &mut Env<'local>
    let mut raw_params = Vec::new();
    let mut user_fn_params = Vec::new();

    if input.is_raw_fn {
        // Raw function: extern "system" fn(unowned_env: EnvUnowned<'local>, ...)
        raw_params.push(quote! { unowned_env: #jni::EnvUnowned<#lifetime> });
        user_fn_params.push(quote! { unowned_env: #jni::EnvUnowned<#lifetime> });
    } else {
        // Wrapped function: extern "system" wrapper, but user's fn takes &mut Env<'local>
        raw_params.push(quote! { mut unowned_env: #jni::EnvUnowned<#lifetime> });
        user_fn_params.push(quote! { env: &mut #jni::Env<#lifetime> });
    }

    // Add this/class parameter
    if is_static {
        raw_params.push(quote! { class: #jni::objects::JClass<#lifetime> });
        user_fn_params.push(quote! { class: #jni::objects::JClass<#lifetime> });
    } else {
        let this_type = if let Some(rust_type) = &input.rust_type {
            quote! { #rust_type<#lifetime> }
        } else {
            quote! { #jni::objects::JObject<#lifetime> }
        };
        raw_params.push(quote! { this: #this_type });
        user_fn_params.push(quote! { this: #this_type });
    }

    // Add method parameters
    for param in &input.method_signature.parameters {
        let param_name = &param.name;
        let rust_type = sig_type_to_rust_type_core(&param.ty, &lifetime, type_mappings, jni);
        raw_params.push(quote! { #param_name: #rust_type });
        user_fn_params.push(quote! { #param_name: #rust_type });
    }

    // Get return type
    let return_type = sig_type_to_rust_type_core(
        &input.method_signature.return_type,
        &lifetime,
        type_mappings,
        jni,
    );

    // Build argument list for calling the user's function
    let mut call_args = Vec::new();
    if is_static {
        call_args.push(quote! { class });
    } else {
        call_args.push(quote! { this });
    }
    for param in &input.method_signature.parameters {
        let param_name = &param.name;
        call_args.push(quote! { #param_name });
    }

    // Note: we unconditionally generate a wrapper, even for raw functions without
    // an ABI check, so we can be sure we register an `extern "system"` ABI function
    // pointer.

    let abi_check = generate_native_method_abi_check(
        jni,
        java_method_name,
        input.abi_check,
        input.is_raw_fn,
        input.is_static,
        Some(type_mappings),
    );

    let (wrapper_path, native_method_block) = if input.is_raw_fn {
        // For raw functions with ABI check: create wrapper that does ABI check
        // The wrapper has extern "system" signature with raw_params
        // The user's function is called with user_fn_params (EnvUnowned)
        let wrapper_ident =
            syn::Ident::new("__native_method_wrapper", proc_macro2::Span::call_site());
        let wrapper_path: syn::Path = syn::parse_quote! { #wrapper_ident };

        let block = quote! {
            // ABI-checking wrapper function that converts from extern "system" to Rust calling convention
            extern "system" fn #wrapper_ident<#lifetime>(
                #(#raw_params),*
            ) -> #return_type {
                #abi_check

                // Call the user's raw function
                #fn_path(unowned_env, #(#call_args),*)
            }

            // Safety: The wrapper function is type-checked at compile time to match
            // the signature specified in the macro. The function pointer is valid
            // and points to __native_method_abi_check_wrapper.
            unsafe {
                #jni::NativeMethod::from_raw_parts(
                    #jni::strings::JNIStr::from_cstr_unchecked(#name_cstr),
                    #jni::strings::JNIStr::from_cstr_unchecked(#sig_cstr),
                    #wrapper_ident as *mut ::std::ffi::c_void,
                )
            }
        };
        (wrapper_path, block)
    } else {
        let with_env_api = if input.catch_unwind {
            quote! { with_env }
        } else {
            quote! { with_env_no_catch }
        };
        // For safe wrapped functions: create wrapper that calls with_env and resolve
        // The wrapper has extern "system" signature with raw_params
        // The user's function is called with user_fn_params (env: &mut Env)
        // Type checking happens naturally when calling the user function
        let error_policy = input
            .error_policy
            .as_ref()
            .map(|p| quote! { #p })
            .unwrap_or_else(|| quote! { #jni::errors::ThrowRuntimeExAndDefault });

        let wrapper_ident =
            syn::Ident::new("__native_method_wrapper", proc_macro2::Span::call_site());
        let wrapper_path: syn::Path = syn::parse_quote! { #wrapper_ident };

        let block = quote! {
            // Type-checked wrapper function that converts from extern "system" to Rust calling convention
            // and handles error resolution
            extern "system" fn #wrapper_ident<#lifetime>(
                #(#raw_params),*
            ) -> #return_type {
                unowned_env
                    .#with_env_api(|env| {
                        #abi_check

                        #fn_path(env, #(#call_args),*)
                    })
                    .resolve::<#error_policy>()
            }

            // Safety: The wrapper function is type-checked at compile time to match
            // the signature specified in the macro. The function pointer is valid
            // and points to __native_method_wrapper.
            unsafe {
                #jni::NativeMethod::from_raw_parts(
                    #jni::strings::JNIStr::from_cstr_unchecked(#name_cstr),
                    #jni::strings::JNIStr::from_cstr_unchecked(#sig_cstr),
                    #wrapper_ident as *mut ::std::ffi::c_void,
                )
            }
        };
        (wrapper_path, block)
    };

    // Generate export wrapper if requested
    if !matches!(input.export, NativeMethodExport::No) {
        let java_class = input.java_type.as_ref().unwrap(); // Validated earlier
        let java_class_dotted = java_class.to_java_dotted();

        // Generate the mangled JNI function name
        let mangled_name = if let NativeMethodExport::WithName(export_name) = input.export {
            export_name.clone()
        } else {
            create_jni_fn_name(&java_class_dotted, java_method_name, Some(&jni_signature))
        };

        // Use export_name attribute - check if has_unsafe_attr is set
        let export_name_attr = if cfg!(has_unsafe_attr) {
            quote! { #[unsafe(export_name = #mangled_name)] }
        } else {
            quote! { #[export_name = #mangled_name] }
        };

        // Build call arguments for the wrapper (just pass through all params)
        let mut export_call_args = vec![quote! { unowned_env }];
        if is_static {
            export_call_args.push(quote! { class });
        } else {
            export_call_args.push(quote! { this });
        }
        for param in &input.method_signature.parameters {
            let param_name = &param.name;
            export_call_args.push(quote! { #param_name });
        }

        // The export wrapper calls the wrapper function (for wrapped) or user function (for raw)
        Ok(quote! {
            const {
                #[doc(hidden)]
                #export_name_attr
                pub extern "system" fn __native_method_export<#lifetime>(
                    #(#raw_params),*
                ) -> #return_type {
                    #wrapper_path(#(#export_call_args),*)
                }

                #native_method_block
            }
        })
    } else {
        Ok(quote! {
            const {
                #native_method_block
            }
        })
    }
}
