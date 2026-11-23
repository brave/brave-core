mod bind_java_type;
mod mangle;
mod native_method;
mod signature;
mod str;
mod types;
mod utils;

// Note: This crate is marked with doctest = false and documentation is owned
// by the jni crate. See ../../jni/docs/macros/jni_str.md file
#[proc_macro]
pub fn jni_str(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    str::jni_str_impl(input.into())
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

/// Converts UTF-8 string literals to a MUTF-8 encoded CStr literal.
///
/// This macro is equivalent to [`jni_str!`] but returns a `&CStr` instead of a `&'static JNIStr`.
///
/// See the [`jni_str!`] macro documentation for detailed syntax and examples.
#[proc_macro]
pub fn jni_cstr(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    str::jni_cstr_impl(input.into())
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

// Note: This crate is marked with doctest = false and documentation is owned
// by the jni crate. See ../../jni/docs/macros/jni_sig.md file
#[proc_macro]
pub fn jni_sig(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    signature::jni_sig_impl(input.into())
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

/// Parses a JNI method or field signature at compile time and returns a `&str` literal.
///
/// This macro is similar to `jni_sig!` but returns a plain UTF-8 string literal instead
/// of a `MethodSignature` or `FieldSignature` struct.
///
/// See the `jni_sig!` macro documentation for detailed syntax and examples.
///
/// # Examples
///
/// ```ignore
/// const SIG: &str = jni_sig_str!((a: jint, b: jboolean) -> void);
/// // Result: "(IZ)V"
///
/// const FIELD_SIG: &str = jni_sig_str!(java.lang.String);
/// // Result: "Ljava/lang/String;"
/// ```
#[proc_macro]
pub fn jni_sig_str(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    signature::jni_sig_str_impl(input.into())
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

/// Parses a JNI method or field signature at compile time and returns a CStr literal.
///
/// This macro is similar to `jni_sig!` but returns a C string literal (e.g., `c"(IZ)V"`)
/// with MUTF-8 encoding instead of a `MethodSignature` or `FieldSignature` struct.
///
/// The output is encoded using Java's modified UTF-8 (MUTF-8) format via `simd_cesu8::mutf8::encode`.
///
/// See the `jni_sig!` macro documentation for detailed syntax and examples.
///
/// # Examples
///
/// ```ignore
/// const SIG: &CStr = jni_sig_cstr!((a: jint, b: jboolean) -> void);
/// // Result: c"(IZ)V" (MUTF-8 encoded)
///
/// const FIELD_SIG: &CStr = jni_sig_cstr!(java.lang.String);
/// // Result: c"Ljava/lang/String;" (MUTF-8 encoded)
/// ```
#[proc_macro]
pub fn jni_sig_cstr(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    signature::jni_sig_cstr_impl(input.into())
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

/// Parses a JNI method or field signature at compile time and returns a `&'static JNIStr`.
///
/// This macro is similar to `jni_sig!` but returns a `&'static JNIStr` with MUTF-8 encoding
/// instead of a `MethodSignature` or `FieldSignature` struct.
///
/// The output is encoded using Java's modified UTF-8 (MUTF-8) format via `simd_cesu8::mutf8::encode`
/// and wrapped in a `JNIStr` via `jni::strings::JNIStr::from_cstr_unchecked()`.
///
/// See the `jni_sig!` macro documentation for detailed syntax and examples.
///
/// # Examples
///
/// ```ignore
/// const SIG: &JNIStr = jni_sig_jstr!((a: jint, b: jboolean) -> void);
/// // Result: &'static JNIStr for "(IZ)V" (MUTF-8 encoded)
///
/// const FIELD_SIG: &JNIStr = jni_sig_jstr!(java.lang.String);
/// // Result: &'static JNIStr for "Ljava/lang/String;" (MUTF-8 encoded)
/// ```
#[proc_macro]
pub fn jni_sig_jstr(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    signature::jni_sig_jstr_impl(input.into())
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

// Note: This crate is marked with doctest = false and documentation is owned
// by the jni crate. See ../../jni/docs/macros/jni_mangle.md file
#[proc_macro_attribute]
pub fn jni_mangle(
    attr: proc_macro::TokenStream,
    item: proc_macro::TokenStream,
) -> proc_macro::TokenStream {
    mangle::jni_mangle2(attr.into(), item.into()).into()
}

// Note: This crate is marked with doctest = false and documentation is owned
// by the jni crate. See ../../jni/docs/macros/native_method.md file
#[proc_macro]
pub fn native_method(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    native_method::native_method_impl(input.into())
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}

// Note: This crate is marked with doctest = false and documentation is owned
// by the jni crate. See ../../jni/docs/macros/bind_java_type_*.md files
#[proc_macro]
pub fn bind_java_type(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    bind_java_type::bind_java_type_impl(input.into())
        .unwrap_or_else(|e| e.to_compile_error())
        .into()
}
