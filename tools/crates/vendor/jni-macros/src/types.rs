use core::panic;
use std::rc::Rc;

use proc_macro2::{Span, TokenStream};
use quote::quote;
use syn::{
    Ident, Result, Token, braced, custom_keyword, parenthesized,
    parse::{Parse, ParseStream},
    token,
};

use crate::str::lit_cstr_mutf8;

custom_keyword!(typealias);

custom_keyword!(Always);
custom_keyword!(UnsafeDebugOnly);
custom_keyword!(UnsafeNever);

/// Format a syn::Path as a string without spaces (for rustdoc links)
pub fn path_to_string_no_spaces(path: &syn::Path) -> String {
    quote!(#path).to_string().replace(" ", "")
}

/// Represents a primitive Java type
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum PrimitiveType {
    Void,
    Boolean,
    Byte,
    Char,
    Short,
    Int,
    Long,
    Float,
    Double,
}

impl PrimitiveType {
    /// Convert primitive type to JNI descriptor character
    pub fn to_jni_descriptor(&self) -> &'static str {
        match self {
            PrimitiveType::Void => "V",
            PrimitiveType::Boolean => "Z",
            PrimitiveType::Byte => "B",
            PrimitiveType::Char => "C",
            PrimitiveType::Short => "S",
            PrimitiveType::Int => "I",
            PrimitiveType::Long => "J",
            PrimitiveType::Float => "F",
            PrimitiveType::Double => "D",
        }
    }
}

/// Represents a Java class name (package + class)
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct JavaClassName {
    /// Package segments (e.g., ["java", "lang"])
    pub package: Vec<String>,
    /// Class name (including optional inner class names separated by '$')
    pub class: String,
}

impl JavaClassName {
    /// Convert to JNI internal form (e.g., "java/lang/String" or "java/lang/Outer$Inner")
    pub fn to_jni_internal(&self) -> String {
        let mut result = String::new();

        // Add package
        for (i, segment) in self.package.iter().enumerate() {
            if i > 0 {
                result.push('/');
            }
            result.push_str(segment);
        }

        // Add class name
        if !self.package.is_empty() {
            result.push('/');
        }
        result.push_str(&self.class);

        result
    }

    /// Convert to Java dotted form (e.g., "java.lang.String" or "java.lang.Outer$Inner")
    pub fn to_java_dotted(&self) -> String {
        let mut result = String::new();

        // Add package
        for (i, segment) in self.package.iter().enumerate() {
            if i > 0 {
                result.push('.');
            }
            result.push_str(segment);
        }

        // Add class name
        if !self.package.is_empty() {
            result.push('.');
        }
        result.push_str(&self.class);

        result
    }

    /// Convert to JNI object descriptor (e.g., "Ljava/lang/String;")
    pub fn to_jni_descriptor(&self) -> String {
        format!("L{};", self.to_jni_internal())
    }
}

impl Parse for JavaClassName {
    /// Parse a Java class name from a ParseStream or string literal
    /// Supports:
    /// - Dotted package.Class syntax (e.g., `java.lang.String`)
    /// - Inner classes with :: separator (e.g., `java.lang.Outer::Inner`)
    /// - String literals with dotted syntax
    /// - Special case for default-package classes starting with `.` (e.g., `.NoPackage`)
    ///
    /// The parser requires at least one dot in the input to differentiate from Rust types,
    /// except for the special default-package case which starts with a `.`
    fn parse(input: ParseStream) -> Result<Self> {
        // Check if it's a string literal
        if input.peek(syn::LitStr) {
            let lit = input.parse::<syn::LitStr>()?;
            let class_str = lit.value();

            // Split by dots
            let parts: Vec<&str> = class_str.split('.').collect();
            if parts.is_empty() || (parts.len() == 1 && !class_str.starts_with('.')) {
                return Err(syn::Error::new(
                    lit.span(),
                    "Java class name in string literal must contain at least one dot (e.g., \"java.lang.String\" or \".NoPackage\" for default package)",
                ));
            }

            let class = parts.last().unwrap().to_string();
            let package: Vec<String> = if class_str.starts_with('.') {
                // Default package case
                Vec::new()
            } else {
                parts[..parts.len() - 1]
                    .iter()
                    .map(|s| s.to_string())
                    .collect()
            };

            return Ok(JavaClassName { package, class });
        }

        // Check for dot-prefixed default package class (e.g., .NoPackage)
        if input.peek(Token![.]) {
            input.parse::<Token![.]>()?;
            let mut class_name = input.parse::<Ident>()?.to_string();

            // Parse inner classes if any and concatenate with $
            while input.peek(Token![::]) {
                input.parse::<Token![::]>()?;
                let inner = input.parse::<Ident>()?;
                class_name.push('$');
                class_name.push_str(&inner.to_string());
            }

            return Ok(JavaClassName {
                package: Vec::new(),
                class: class_name,
            });
        }

        // Check if we have an ident followed by a dot (using lookahead to avoid consuming the ident)
        // This ensures we don't consume a token that might be a primitive type or Rust type
        if input.peek(Ident) && input.peek2(Token![.]) {
            // Parse as ident-based syntax: package.Class
            let mut segments = Vec::new();
            let first = input.parse::<Ident>()?;
            segments.push(first.to_string());

            // Parse dot-separated package/class segments
            while input.peek(Token![.]) {
                input.parse::<Token![.]>()?;
                let segment = input.parse::<Ident>()?;
                segments.push(segment.to_string());
            }

            // Parse inner classes (separated by ::) and concatenate with $
            let mut inner_class_parts = Vec::new();
            while input.peek(Token![::]) {
                input.parse::<Token![::]>()?;
                let inner = input.parse::<Ident>()?;
                inner_class_parts.push(inner.to_string());
            }

            // Build the class name: last segment + any inner classes joined with $
            let mut class = segments.pop().unwrap();
            if !inner_class_parts.is_empty() {
                class.push('$');
                class.push_str(&inner_class_parts.join("$"));
            }

            return Ok(JavaClassName {
                package: segments,
                class,
            });
        }

        // None of the valid patterns matched
        Err(syn::Error::new(
            input.span(),
            "Expected Java class name: either a string literal with dots (e.g., \"java.lang.String\"), a dotted identifier (e.g., java.lang.String), or a dot-prefixed class for default package (e.g., .NoPackage)",
        ))
    }
}
/// Represents a type in the signature
#[derive(Debug, Clone, PartialEq)]
pub enum SigType {
    /// A type alias that must be mapped via TypeMappings
    /// into a Java class name or a primitive type
    Alias(String),
    /// Object type (Java class)
    Object(JavaClassName),
    /// Array type with element type and dimensions
    Array(Box<SigType>, usize),
}

impl SigType {
    /// Convert to JNI descriptor
    pub fn to_jni_descriptor(&self, type_mappings: &TypeMappings) -> Result<String> {
        match self {
            SigType::Alias(name) => match type_mappings.map_alias(name) {
                Some(ConcreteType::Primitive { primitive, .. }) => {
                    Ok(primitive.to_jni_descriptor().to_string())
                }
                Some(ConcreteType::Object {
                    name: java_class, ..
                }) => Ok(java_class.to_jni_descriptor()),
                None => Err(syn::Error::new(
                    Span::call_site(),
                    format!("Unknown type '{}'", name),
                )),
            },
            SigType::Object(class) => Ok(class.to_jni_descriptor()),
            SigType::Array(elem, dims) => {
                let elem_desc = elem.to_jni_descriptor(type_mappings)?;
                let mut result = String::new();
                for _ in 0..*dims {
                    result.push('[');
                }
                result.push_str(&elem_desc);
                Ok(result)
            }
        }
    }

    pub fn try_as_primitive(&self, type_mappings: &TypeMappings) -> Option<PrimitiveType> {
        match self {
            SigType::Alias(name) => match type_mappings.map_alias(name) {
                Some(ConcreteType::Primitive { primitive, .. }) => Some(primitive),
                _ => None,
            },
            _ => None,
        }
    }
}

/// Parse a type (can be primitive, object, rust reference, or array)
pub fn parse_type(input: ParseStream, type_mappings: &TypeMappings) -> Result<SigType> {
    // Check for leading reference operator (ignore it)
    if input.peek(Token![&]) {
        input.parse::<Token![&]>()?;
    }

    // Check for array syntax [...]
    if input.peek(token::Bracket) {
        let content;
        let bracket_span = input.span();
        syn::bracketed!(content in input);
        let elem_type = parse_type(&content, type_mappings)?;

        // Validate that the element type can be used in an array
        validate_array_element_type(&elem_type, type_mappings, bracket_span)?;

        // Count array dimensions
        let mut dims = 1;
        while input.peek(token::Bracket) {
            let _content;
            syn::bracketed!(_content in input);
            // Nested array - increment dimensions
            dims += 1;
        }

        return Ok(SigType::Array(Box::new(elem_type), dims));
    }

    // Check for unit type ()
    if input.peek(token::Paren) {
        let content;
        parenthesized!(content in input);
        if content.is_empty() {
            return Ok(SigType::Alias("void".to_string()));
        } else {
            return Err(syn::Error::new(
                content.span(),
                "Expected empty parentheses for void type",
            ));
        }
    }

    // Try to parse as Java class name first (handles string literals, .NoPackage, and package.Class)
    // This is done speculatively - if it fails, we'll try other interpretations
    let base_type = if let Ok(java_class) = input.parse::<JavaClassName>() {
        SigType::Object(java_class)
    } else {
        // Not a Java class name, so it must be a TypeMappings type (could be
        // primitive lie jint/i32 or Reference like
        // JString/jni::objects::JString)
        let path = input.parse::<syn::Path>()?;
        let path_str = path_to_string_no_spaces(&path);

        SigType::Alias(path_str)
    };

    // Check for suffix array syntax (applies to all base types)
    let mut dims = 0;
    let mut first_bracket_span = None;
    while input.peek(token::Bracket) {
        let bracket_span = input.span();
        if first_bracket_span.is_none() {
            first_bracket_span = Some(bracket_span);
        }
        let content;
        syn::bracketed!(content in input);
        if !content.is_empty() {
            return Err(syn::Error::new(
                content.span(),
                "Expected empty brackets for array suffix syntax",
            ));
        }
        dims += 1;
    }

    // If we have array dimensions, validate and wrap in Array type
    if dims > 0 {
        // Validate that the element type can be used in an array (void cannot)
        validate_array_element_type(&base_type, type_mappings, first_bracket_span.unwrap())?;
        Ok(SigType::Array(Box::new(base_type), dims))
    } else {
        Ok(base_type)
    }
}

#[derive(Clone, Debug, Eq)]
enum RustTypeTarget {
    /// A primitive type that maps to a `jni::sys` type
    Primitive {
        /// Is this a standard primitive type or a user-defined mapping?
        is_builtin: bool,
        /// The primitive type
        primitive: PrimitiveType,
        /// The name or full path for the primitive type (e.g., "i32" or "jni::sys::jint")
        ///
        /// If the jni crate is renamed, this path will reflect the renamed crate path.
        ///
        /// If this represents a Rust primitive type, such as `i32`, then this will match
        /// that type name.
        path: String,
    },
    Reference {
        /// Is this a built-in `jni` crate Reference type?
        is_builtin: bool,
        /// The full path for a `jni` `Reference` type (e.g., "jni::objects::JString")
        ///
        /// If the jni crate is renamed, this path will reflect the renamed crate path.
        path: String,
    },
}

// Ignore the 'is_builtin' field for equality and hashing

impl PartialEq for RustTypeTarget {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (
                RustTypeTarget::Primitive {
                    primitive: p1,
                    path: path1,
                    ..
                },
                RustTypeTarget::Primitive {
                    primitive: p2,
                    path: path2,
                    ..
                },
            ) => p1 == p2 && path1 == path2,
            (
                RustTypeTarget::Reference { path: path1, .. },
                RustTypeTarget::Reference { path: path2, .. },
            ) => path1 == path2,
            _ => false,
        }
    }
}
impl std::hash::Hash for RustTypeTarget {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        match &self {
            RustTypeTarget::Primitive {
                primitive, path, ..
            } => {
                primitive.hash(state);
                path.hash(state);
            }
            RustTypeTarget::Reference { path, .. } => {
                path.hash(state);
            }
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub struct RustType {
    target: RustTypeTarget,
}

impl RustType {
    pub fn primitive(&self) -> Option<PrimitiveType> {
        if let RustTypeTarget::Primitive { primitive, .. } = &self.target {
            Some(primitive.clone())
        } else {
            None
        }
    }
    pub fn is_builtin(&self) -> bool {
        match self.target {
            RustTypeTarget::Primitive { is_builtin, .. } => is_builtin,
            RustTypeTarget::Reference { is_builtin, .. } => is_builtin,
        }
    }
    pub fn path(&self) -> &str {
        match &self.target {
            RustTypeTarget::Primitive { path, .. } => path.as_str(),
            RustTypeTarget::Reference { path, .. } => path.as_str(),
        }
    }
}

pub enum ConcreteType {
    Primitive {
        primitive: PrimitiveType,
        /// The full path for the primitive type (e.g., "jni::sys::jint")
        ///
        /// If the jni crate is renamed, this path will reflect the renamed
        /// crate path.
        ///
        /// If the lookup was done using a Rust type alias such as `i32` then
        /// this will correspond to that alias.
        ///
        /// If the lookup was done using a Java type alias such as `int`, then
        /// this will correspond to the canonical `jni::sys` type path.
        path: String,
        /// Whether this is a built-in primitive type mapping or a user-defined one
        is_builtin: bool,
    },
    Object {
        name: Rc<JavaClassName>,
        reference_type: Rc<RustType>,
    },
}

/// Type mappings from Rust Reference types to Java class names and aliases for
/// primitive types
///
/// Firstly, the type mappings start with a set of N:1 mapping from Rust type
/// aliases to RustTypes that either represent a `Primitive` type or a
/// `Reference` type.
///
/// E.g. `"jint"` => `{ Primitive(Int), jni::sys::jint }`, `"i32"` => `{
///     Primitive(Int), i32 }`, `"JString"` =>
///     `Reference("jni::objects::JString")`
///
/// ## Reference Types
///
///   A Reference type is represented by a full Rust type path (e.g.,
///   "jni::objects::JString") (such as "JString" => "jni::objects::JString")
///
///   Additionally, for the Reference types there are also:
///
/// - A N:1 mapping from canonical Rust type paths to Java class names (such as
///   "jni::objects::JString" => "java.lang.String")
///
///   Note: that multiple Rust type paths can map to the same Java class name,
///   such as if a third-party crate provides it's own bindings for a java.lang
///   type that has a built-in binding in the jni crate. (like
///   JList/java.util.List)
///
///   Note: As a restriction, certain "core" java.lang classes cannot be
///   remapped, including java.lang.Object, java.lang.Class, java.lang.String,
///   java.lang.Throwable.
///
/// - A 1:1 mapping from Java class names to canonical Rust type paths (such as
///   "java.lang.String" => "jni::objects::JString")
///
///   Note: When there are multiple Rust type paths that map to the same Java
///   class name, then the most-recently added mapping takes precedence in the
///   reverse lookup so that built-in (non-core) types can be overridden by
///   user-defined types.
///
/// ## Primitive Types
///
/// A primitive type is represented by the `PrimitiveType` enum, and a full path
/// to the `jni::sys` primitive type (e.g., "jni::sys::jint") or the name of an
/// equivalent Rust primitive type (e.g., "i32").
///
/// Each JNI primitive typically has two primitive RustTypes:
/// - The `jni::sys` type (e.g., "jni::sys::jint")
/// - The Rust primitive type (e.g., "i32")
///
/// and then one alias for the Java type name (e.g., "int") that points to the
/// `jni::sys` type.
///
/// A `type_map` may add additional aliases for primitive types,
///
/// There are no other mappings for primitive types beyond the alias -> RustType
pub struct TypeMappings {
    /// Maps Rust type names to canonical Rust type paths
    ///
    /// For example, built-in types have multiple aliases, so signatures can
    /// use types like "JString" or "jni::objects::JString" for the same type.
    ///
    /// Note: Even if the jni crate is renamed, the full-path alias for built-in
    /// types will still have a `jni::` prefix, and it's only the
    /// `RustType::path` that will have the renamed crate path.
    alias_to_rust: std::collections::HashMap<String, Rc<RustType>>,
    /// Set of all known Rust types
    rust_types: std::collections::HashSet<Rc<RustType>>,
    /// Maps `RustType::path`s to Java class names
    ///
    /// Only includes the canonical Rust type paths, so make sure to resolve
    /// aliases like "JString" before looking up in this map.
    rust_to_java: std::collections::HashMap<Rc<RustType>, Rc<JavaClassName>>,
    /// Reverse mapping from Java class names to the canonical RustType path
    java_to_rust: std::collections::HashMap<Rc<JavaClassName>, Rc<RustType>>,
    /// Set of core Java types that can not be mapped to non-jni-crate types
    ///
    /// Although some types, like java.util.List, can be remapped to third-party
    /// bindings, we don't allow alternative mappings for core java.lang types
    /// like java.lang.Object, java.lang.Class, java.lang.String.
    ///
    /// In addition to the types listed here, we also consider any array type
    /// (any descriptor that starts with '[') to be a core type that cannot be
    /// remapped.
    core_java: std::collections::HashSet<Rc<JavaClassName>>,
    /// The path to the jni crate (e.g., `jni` or `::jni` or `crate::jni`)
    jni_crate: syn::Path,
}

impl TypeMappings {
    /// Create a new TypeMappings with default JNI type mappings
    ///
    /// # Arguments
    /// * `jni_crate` - The path to the jni crate (e.g., `jni` or `::jni` or `crate::jni`)
    pub fn new(jni_crate: &syn::Path) -> Self {
        let mut type_mappings = Self {
            alias_to_rust: std::collections::HashMap::new(),
            rust_types: std::collections::HashSet::new(),
            rust_to_java: std::collections::HashMap::new(),
            java_to_rust: std::collections::HashMap::new(),
            core_java: std::collections::HashSet::new(),
            jni_crate: jni_crate.clone(),
        };

        // Helper function to parse a Java class name from a dotted string
        let parse_java_class_dotted = |class_str: &str| -> JavaClassName {
            let parts: Vec<&str> = class_str.split('.').collect();
            if parts.is_empty() {
                panic!("Invalid Java class name: {}", class_str);
            }

            let class = parts.last().unwrap().to_string();
            let package = parts[..parts.len() - 1]
                .iter()
                .map(|s| s.to_string())
                .collect();

            JavaClassName { package, class }
        };

        // Convert the jni crate path to a string
        let jni_crate_str = path_to_string_no_spaces(jni_crate);

        // Add default type mappings for built-in jni crate types
        let builtins = [
            (
                "JByteBuffer",
                "java.nio.ByteBuffer",
                "objects::JByteBuffer",
                false,
            ),
            (
                "JCharSequence",
                "java.lang.CharSequence",
                "objects::JCharSequence",
                false,
            ),
            (
                "JClassLoader",
                "java.lang.ClassLoader",
                "objects::JClassLoader",
                false,
            ),
            ("JClass", "java.lang.Class", "objects::JClass", true),
            (
                "JCollection",
                "java.util.Collection",
                "objects::JCollection",
                false,
            ),
            (
                "JIterator",
                "java.util.Iterator",
                "objects::JIterator",
                false,
            ),
            ("JList", "java.util.List", "objects::JList", false),
            ("JMap", "java.util.Map", "objects::JMap", false),
            (
                "JMapEntry",
                "java.util.Map$Entry",
                "objects::JMapEntry",
                false,
            ),
            ("JObject", "java.lang.Object", "objects::JObject", true),
            ("JSet", "java.util.Set", "objects::JSet", false),
            (
                "JStackTraceElement",
                "java.lang.StackTraceElement",
                "objects::JStackTraceElement",
                false,
            ),
            ("JString", "java.lang.String", "objects::JString", true),
            ("JThread", "java.lang.Thread", "objects::JThread", false),
            (
                "JThrowable",
                "java.lang.Throwable",
                "objects::JThrowable",
                true,
            ),
        ];

        for (simple_name, java_class_str, module_path, is_core) in builtins {
            let java_class = Rc::new(parse_java_class_dotted(java_class_str));

            let jni_full_path = format!("jni::{}", module_path);
            let real_full_path = format!("{}::{}", jni_crate_str, module_path);

            type_mappings
                .insert_ref_type(&jni_full_path, &real_full_path, (*java_class).clone(), true)
                .expect("Failed to insert built-in jni crate type mapping");
            type_mappings
                .insert_alias(simple_name, &jni_full_path)
                .expect("Failed to insert built-in jni crate type alias");
            if is_core {
                type_mappings.core_java.insert(java_class.clone());
            }
        }

        // Insert Void as a special case
        type_mappings
            .insert_prim_type("void", PrimitiveType::Void, "()", true)
            .expect("Failed to insert 'void' primitive type mapping");

        let jni_sys_prim_types = [
            ("jboolean", "sys::jboolean", PrimitiveType::Boolean),
            ("jbyte", "sys::jbyte", PrimitiveType::Byte),
            ("jchar", "sys::jchar", PrimitiveType::Char),
            ("jshort", "sys::jshort", PrimitiveType::Short),
            ("jint", "sys::jint", PrimitiveType::Int),
            ("jlong", "sys::jlong", PrimitiveType::Long),
            ("jfloat", "sys::jfloat", PrimitiveType::Float),
            ("jdouble", "sys::jdouble", PrimitiveType::Double),
        ];

        for (rust_name, path, prim_type) in jni_sys_prim_types {
            let full_path = format!("{}::{}", jni_crate_str, path);
            type_mappings
                .insert_prim_type(rust_name, prim_type, &full_path, true)
                .expect("Failed to insert jni::sys primitive type mapping");
        }

        let rust_prim_types = [
            ("bool", PrimitiveType::Boolean),
            ("i8", PrimitiveType::Byte),
            ("i16", PrimitiveType::Short),
            ("i32", PrimitiveType::Int),
            ("i64", PrimitiveType::Long),
            ("f32", PrimitiveType::Float),
            ("f64", PrimitiveType::Double),
        ];

        for (rust_name, prim_type) in rust_prim_types {
            type_mappings
                .insert_prim_type(rust_name, prim_type, rust_name, true)
                .expect("Failed to insert Rust primitive type mapping");
        }

        let prim_aliases = [
            ("boolean", "jboolean"),
            ("byte", "jbyte"),
            ("char", "jchar"),
            ("short", "jshort"),
            ("int", "jint"),
            ("long", "jlong"),
            ("float", "jfloat"),
            ("double", "jdouble"),
        ];

        for (alias, target) in prim_aliases {
            type_mappings
                .insert_alias(alias, target)
                .expect("Failed to insert primitive type alias");
        }

        type_mappings
    }

    // Lookup the canonical RustType for a Reference type path
    fn rust_reference_type(&mut self, path: &str, is_builtin: bool) -> Rc<RustType> {
        let candidate = RustType {
            target: RustTypeTarget::Reference {
                path: path.to_string(),
                is_builtin,
            },
        };
        if let Some(existing) = self.rust_types.get(&candidate) {
            // It would be a bug if a builtin type resolved to a non-builtin existing type
            if is_builtin {
                assert!(
                    existing.is_builtin(),
                    "Inconsistent is_builtin flag for existing Rust reference type '{}'",
                    path
                );
            }
            return existing.clone();
        }
        let rust_type = Rc::new(candidate);
        self.rust_types.insert(rust_type.clone());
        rust_type
    }

    // Lookup the canonical RustType for a Primitive type path
    fn rust_prim_type(
        &mut self,
        primitive: PrimitiveType,
        path: String,
        is_builtin: bool,
    ) -> Rc<RustType> {
        let candidate = RustType {
            target: RustTypeTarget::Primitive {
                primitive,
                path,
                is_builtin,
            },
        };
        if let Some(existing) = self.rust_types.get(&candidate) {
            // It would be a bug if a builtin type resolved to a non-builtin existing type
            if is_builtin {
                assert!(
                    existing.is_builtin(),
                    "Inconsistent is_builtin flag for existing Rust primitive type '{}'",
                    candidate.path()
                );
            }
            return existing.clone();
        }
        let rust_type = Rc::new(candidate);
        self.rust_types.insert(rust_type.clone());
        rust_type
    }

    /// Insert a mapping from a Rust `Reference` type path to a Java class
    ///
    /// If there is already a a Rust type mapping to the same Java class, the reverse
    /// mapping from Java class to Rust type path will be updated to point to the new
    /// Rust type path (so that user-defined types can override built-in types).
    ///
    /// Note:
    /// - User code cannot create mappings for core Java classes.
    /// - User code cannot _change_ the mapping for an existing Rust type path.
    /// - User code cannot currently create multiple alias for a single Rust type path.
    ///
    /// Returns an error if:
    /// - Attempting to map a core Java class
    /// - Attempting to change the mapping for an existing Rust type path
    pub fn insert_ref_type(
        &mut self,
        alias: &str,
        path: &str,
        java_class: JavaClassName,
        is_builtin: bool,
    ) -> Result<()> {
        let java_class = if let Some((java_class, _)) = self.java_to_rust.get_key_value(&java_class)
        {
            java_class.clone()
        } else {
            Rc::new(java_class)
        };

        // Check if this is a core Java class that cannot be remapped
        if self.core_java.contains(&java_class) {
            return Err(syn::Error::new(
                Span::call_site(),
                format!(
                    "Cannot create type mapping for '{}' -> '{}': '{}' is a core Java type. Core types (java.lang.Object, java.lang.Class, java.lang.Throwable and java.lang.String) are automatically mapped to jni crate types and cannot be remapped.",
                    path,
                    java_class.to_java_dotted(),
                    java_class.to_java_dotted()
                ),
            ));
        }

        let rust_type = self.rust_reference_type(path, is_builtin);

        // Check if this alias already has a mapping to a different Rust type
        if let Some(existing) = self.alias_to_rust.get(alias) {
            if existing != &rust_type {
                let existing_java_mapping = self.rust_to_java.get(existing).unwrap();
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!(
                        "Cannot change existing Rust type mapping for Rust type name '{alias}': already mapped to Java class '{}'/'{}', cannot remap to '{}'/'{}'. Only one mapping per Rust type path is allowed.",
                        existing_java_mapping.to_java_dotted(),
                        existing.path(),
                        java_class.to_java_dotted(),
                        path
                    ),
                ));
            }

            return Ok(());
        }

        self.alias_to_rust
            .insert(alias.to_string(), rust_type.clone());

        // Insert the mappings
        self.rust_to_java
            .insert(rust_type.clone(), java_class.clone());
        self.java_to_rust.insert(java_class, rust_type);

        Ok(())
    }

    pub fn insert_prim_type(
        &mut self,
        alias: &str,
        primitive: PrimitiveType,
        path: &str,
        is_builtin: bool,
    ) -> Result<()> {
        let rust_type = self.rust_prim_type(primitive, path.to_string(), is_builtin);

        if let Some(existing) = self.alias_to_rust.get(alias) {
            if existing != &rust_type {
                return Err(syn::Error::new(
                    Span::call_site(),
                    format!(
                        "Cannot change existing Rust type mapping for Rust type name '{}': already mapped to a different primitive type. Only one mapping per Rust type name is allowed.",
                        alias
                    ),
                ));
            }
            return Ok(());
        }

        self.alias_to_rust
            .insert(alias.to_string(), rust_type.clone());

        Ok(())
    }

    pub fn insert_alias(&mut self, from: &str, to: &str) -> Result<()> {
        if let Some(rust_type) = self.alias_to_rust.get(to) {
            if let Some(existing) = self.alias_to_rust.get(from) {
                if existing != rust_type {
                    return Err(syn::Error::new(
                        Span::call_site(),
                        format!(
                            "Cannot change mapping for typealias '{}' to '{}': already mapped to a different type ({}). Only one mapping per Rust type name is allowed.",
                            from,
                            to,
                            existing.path()
                        ),
                    ));
                }
                return Ok(());
            }

            self.alias_to_rust
                .insert(from.to_string(), rust_type.clone());
            Ok(())
        } else {
            Err(syn::Error::new(
                Span::call_site(),
                format!(
                    "Cannot create alias from '{}' to '{}': target type not found",
                    from, to
                ),
            ))
        }
    }

    /// Parse `type_map` block with Reference + primitive mappings and aliases
    ///
    /// This is a shared parser used by both bind_java_type! and jni_sig! macros. Parses a
    /// braced block containing type mappings from Rust type paths to Java class names.
    ///
    /// # Syntax
    ///
    /// A type_map block can be comprised of:
    /// - Reference type mappings
    /// - Primitive type mappings (unsafe)
    /// - Type aliases
    ///
    /// In all cases:
    /// - The type separator is `=>`
    /// - Mappings are separated by commas
    /// - Trailing comma is optional
    ///
    /// ## Reference Type Mappings
    ///
    /// Each Reference type mapping has the form: `RustPath => java.lang.Class`
    /// - RustPath can be a simple identifier (e.g., `JString`) or a path (e.g.,
    ///   `jni::objects::JString`)
    ///
    /// ## Primitive Type Mappings
    ///
    /// Primitive type mappings are considered `unsafe`, but can be especially useful for mapping
    /// wrapped pointers types to a Java `long` type (e.g. to associate native handles with Java
    /// objects).
    ///
    /// Each Primitive type mapping has the form: `unsafe RustPrimitive => java_primitive`
    ///
    /// ## Aliases
    ///
    /// Type aliases allow you to create a new name for an existing type mapping. This can be useful
    /// for making signatures more readable (e.g. before you have a defined a real type binding).
    ///
    /// An alias mapping has the form: `alias NewTypeName => ExistingTypeName`
    ///
    /// Aliases for array types are not supported.
    ///
    /// # Example
    /// ```ignore
    /// type_map = {
    ///     MyType => java.lang.MyType,
    ///     custom::MyOtherType => com.example.MyOtherType,
    ///     unsafe MyBoxHandle => long,
    ///     typealias MyTypeAlias => MyType,
    ///     typealias MyOtherTypeAlias => JObject,
    /// }
    /// ```
    pub fn parse_mappings(&mut self, input: ParseStream) -> Result<()> {
        // Optional '=' before block
        if input.peek(Token![=]) {
            input.parse::<Token![=]>()?;
        }

        let mappings_content;
        braced!(mappings_content in input);

        while !mappings_content.is_empty() {
            if mappings_content.peek(typealias) {
                // Parse alias mapping
                mappings_content.parse::<typealias>()?;

                // Parse new alias name
                let new_alias: syn::Path = mappings_content.parse()?;

                // Parse separator
                mappings_content.parse::<Token![=>]>()?;

                // Parse existing type name
                let existing_type: syn::Path = mappings_content.parse()?;

                // Insert alias
                let new_alias_str = path_to_string_no_spaces(&new_alias);
                let existing_type_str = path_to_string_no_spaces(&existing_type);
                self.insert_alias(&new_alias_str, &existing_type_str)?;

                // Require comma between entries, but trailing comma is optional
                if !mappings_content.is_empty() {
                    mappings_content.parse::<Token![,]>()?;
                }
            } else {
                let is_prim_mapping = if mappings_content.peek(Token![unsafe]) {
                    mappings_content.parse::<Token![unsafe]>()?;
                    true
                } else {
                    false
                };

                // Parse Rust path
                let rust_path: syn::Path = mappings_content.parse()?;

                // Convert path to string without spaces
                let rust_path_str = path_to_string_no_spaces(&rust_path);

                // Parse separator
                mappings_content.parse::<Token![=>]>()?;

                if is_prim_mapping {
                    // Parse primitive type name
                    let prim_type_ident: Ident = mappings_content.parse()?;
                    let prim_type_name = prim_type_ident.to_string();

                    // Validate type name by checking that it's a "built-in" primitive type name
                    match self.map_alias(&prim_type_name) {
                        Some(ConcreteType::Primitive {
                            primitive,
                            is_builtin,
                            ..
                        }) if is_builtin => {
                            self.insert_prim_type(
                                &rust_path_str,
                                primitive,
                                &rust_path_str,
                                false,
                            )?;
                        }
                        _ => {
                            return Err(syn::Error::new(
                                prim_type_ident.span(),
                                format!(
                                    "Invalid primitive type name '{}' for unsafe primitive mapping. Must be one of the built-in primitive type names (e.g., 'int', 'long', 'boolean', etc.)",
                                    prim_type_name
                                ),
                            ));
                        }
                    }
                } else {
                    // Parse Java class name
                    let java_class_mapping: JavaClassName = mappings_content.parse()?;

                    // Insert mapping
                    self.insert_ref_type(
                        &rust_path_str,
                        &rust_path_str,
                        java_class_mapping,
                        false,
                    )?;
                }

                // Require comma between entries, but trailing comma is optional
                if !mappings_content.is_empty() {
                    mappings_content.parse::<Token![,]>()?;
                }
            }
        }

        Ok(())
    }

    /// Get the Java class name and canonical Rust type for a Rust type alias
    pub fn map_alias(&self, alias: &str) -> Option<ConcreteType> {
        if let Some(rust_type) = self.alias_to_rust.get(alias) {
            match &rust_type.target {
                RustTypeTarget::Primitive {
                    primitive,
                    path,
                    is_builtin,
                } => Some(ConcreteType::Primitive {
                    primitive: primitive.clone(),
                    path: path.clone(),
                    is_builtin: *is_builtin,
                }),
                RustTypeTarget::Reference { .. } => {
                    let java_class = self.rust_to_java.get(rust_type).expect(
                        "TypeMappings invariant violated: rust_type missing in rust_to_java map",
                    );
                    Some(ConcreteType::Object {
                        name: java_class.clone(),
                        reference_type: rust_type.clone(),
                    })
                }
            }
        } else {
            None
        }
    }

    /// Get the canonical Rust type path for a Java class name
    pub fn map_java_class_to_rust_type(&self, java_class: &JavaClassName) -> Option<&RustType> {
        self.java_to_rust.get(java_class).map(|s| s.as_ref())
    }

    /// Get an iterator over all Java class names
    #[allow(unused)]
    pub fn java_classes(&self) -> impl Iterator<Item = &Rc<JavaClassName>> {
        self.java_to_rust.keys()
    }

    /// Get an iterator over all Rust types
    pub fn rust_types(&self) -> impl Iterator<Item = &Rc<RustType>> {
        self.rust_types.iter()
    }

    /// Get an iterator over rust_path -> java_class mappings
    pub fn rust_to_java_iter(&self) -> impl Iterator<Item = (&Rc<RustType>, &Rc<JavaClassName>)> {
        self.rust_to_java.iter()
    }

    /// Get the jni crate path
    pub fn jni_crate(&self) -> &syn::Path {
        &self.jni_crate
    }

    /// Check if a Java class is a core type that cannot normally be bound
    pub fn is_core_java_type(&self, java_class: &JavaClassName) -> bool {
        self.core_java.contains(java_class)
    }
}

/// Helper function to validate that a type can be used as an array element
/// Returns an error if the element type is void
fn validate_array_element_type(
    elem_type: &SigType,
    type_mappings: &TypeMappings,
    span: Span,
) -> Result<()> {
    if let SigType::Alias(name) = elem_type {
        match type_mappings.map_alias(name) {
            Some(ConcreteType::Primitive {
                primitive: PrimitiveType::Void,
                ..
            }) => {
                return Err(syn::Error::new(
                    span,
                    "void cannot be used as an array element type",
                ));
            }
            Some(_) => {}
            None => {
                return Err(syn::Error::new(span, format!("Unknown type '{}'", name)));
            }
        }
    }

    Ok(())
}

/// Core function to convert a JavaType to a concrete Rust type with lifetime
/// Returns the type WITHOUT any AsRef wrapper
pub fn sig_type_to_rust_type_core(
    java_type: &SigType,
    lifetime: &TokenStream,
    type_mappings: &TypeMappings,
    jni: &syn::Path,
) -> TokenStream {
    match java_type {
        SigType::Alias(alias) => match type_mappings.map_alias(alias) {
            Some(ConcreteType::Primitive {
                primitive, path, ..
            }) => {
                if let PrimitiveType::Void = primitive {
                    quote! { () }
                } else {
                    let path: syn::Path = syn::parse_str(&path)
                        .unwrap_or_else(|_| panic!("Invalid Rust type path: {}", path));

                    quote! { #path  }
                }
            }
            Some(ConcreteType::Object {
                reference_type: rust_type,
                ..
            }) => {
                let path_str = rust_type.path();

                let path: syn::Path = syn::parse_str(path_str)
                    .unwrap_or_else(|_| panic!("Invalid Rust type path: {}", path_str));

                quote! { #path<#lifetime> }
            }
            None => {
                // Should have been validated while parsing
                unreachable!("Unknown type: {}", alias);
            }
        },
        SigType::Object(class_name) => {
            // Check if there's a type mapping for this Java class
            if let Some(rust_type) = type_mappings.map_java_class_to_rust_type(class_name) {
                // Parse the rust path and generate the type
                let path_str = rust_type.path();
                let path: syn::Path = syn::parse_str(path_str)
                    .unwrap_or_else(|_| panic!("Invalid Rust type path: {}", path_str));
                quote! { #path<#lifetime> }
            } else {
                // Use JObject for unmapped class types
                quote! { #jni::objects::JObject<#lifetime> }
            }
        }
        SigType::Array(element_type, dimensions) => {
            // First, resolve the innermost array dimension (the actual array of elements)
            // This will be either:
            // - JPrimitiveArray<prim_type> for primitive arrays
            // - JObjectArray<RustType> for arrays of mapped types
            // - JObjectArray (defaulting to JObjectArray<JObject>) for unmapped object arrays
            let innermost_array = match &**element_type {
                SigType::Alias(alias) => {
                    match type_mappings.map_alias(alias) {
                        Some(ConcreteType::Primitive { primitive, .. }) => {
                            let prim_ident = match primitive {
                                PrimitiveType::Void => {
                                    // This should be unreachable since void array elements are rejected during parsing
                                    unreachable!("void cannot be used as an array element type")
                                }
                                PrimitiveType::Boolean => quote! { jboolean },
                                PrimitiveType::Byte => quote! { jbyte },
                                PrimitiveType::Char => quote! { jchar },
                                PrimitiveType::Short => quote! { jshort },
                                PrimitiveType::Int => quote! { jint },
                                PrimitiveType::Long => quote! { jlong },
                                PrimitiveType::Float => quote! { jfloat },
                                PrimitiveType::Double => quote! { jdouble },
                            };
                            quote! { #jni::objects::JPrimitiveArray<#lifetime, #jni::sys::#prim_ident> }
                        }
                        Some(ConcreteType::Object {
                            reference_type: rust_type,
                            ..
                        }) => {
                            let path_str = rust_type.path();
                            let path: syn::Path = syn::parse_str(path_str)
                                .unwrap_or_else(|_| panic!("Invalid Rust type path: {}", path_str));
                            quote! { #jni::objects::JObjectArray<#lifetime, #path<#lifetime>> }
                        }
                        None => {
                            // Should have been validated while parsing
                            unreachable!("Unknown type: {}", alias);
                        }
                    }
                }
                SigType::Object(class_name) => {
                    // Check for type mapping for array element type
                    if let Some(rust_type) = type_mappings.map_java_class_to_rust_type(class_name) {
                        let path_str = rust_type.path();
                        let path: syn::Path = syn::parse_str(path_str)
                            .unwrap_or_else(|_| panic!("Invalid Rust type path: {}", path_str));
                        quote! { #jni::objects::JObjectArray<#lifetime, #path<#lifetime>> }
                    } else {
                        // Object array with unmapped type
                        quote! { #jni::objects::JObjectArray<#lifetime> }
                    }
                }
                SigType::Array(_inner_element_type, _dimensions) => {
                    // An array of arrays should be represented via `Array(_, dimensions > 1)`, not by nesting
                    // ArgType::Array as the element type.
                    unreachable!(
                        "Multi-dimensional arrays should not be represented by nesting ArgType::Array"
                    )
                }
            };

            // Now wrap with JObjectArray for each additional dimension beyond the first
            let mut result = innermost_array;
            for _ in 0..(*dimensions - 1) {
                result = quote! { #jni::objects::JObjectArray<#lifetime, #result> };
            }
            result
        }
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Default)]
pub enum AbiCheck {
    #[default]
    Always,
    UnsafeDebugOnly,
    UnsafeNever,
}

impl AbiCheck {
    pub fn requires_abi_check(&self) -> bool {
        match self {
            AbiCheck::Always => true,
            AbiCheck::UnsafeDebugOnly => cfg!(debug_assertions),
            AbiCheck::UnsafeNever => false,
        }
    }
}

impl Parse for AbiCheck {
    fn parse(input: ParseStream) -> Result<Self> {
        let lookahead = input.lookahead1();
        if lookahead.peek(Always) {
            input.parse::<Always>()?;
            Ok(AbiCheck::Always)
        } else if lookahead.peek(UnsafeDebugOnly) {
            input.parse::<UnsafeDebugOnly>()?;
            Ok(AbiCheck::UnsafeDebugOnly)
        } else if lookahead.peek(UnsafeNever) {
            input.parse::<UnsafeNever>()?;
            Ok(AbiCheck::UnsafeNever)
        } else {
            Err(lookahead.error())
        }
    }
}

/// Generate assertions that type mappings are correct
///
/// - Generates assertions to verify the Java class names match at runtime.
/// - Generates compile-time assertions to verify primitive type sizes and alignments match.
///
/// Skips checks for the built-in jni crate types that are assumed to be correct.
/// (note that we will still generate a compile-time error if attempting to map
/// a built-in Rust type to a different Java type)
pub fn generate_type_mapping_checks(type_mappings: &TypeMappings, jni: &syn::Path) -> TokenStream {
    let mut type_checks = Vec::new();

    // Generate runtime checks for all type mappings
    for (rust_type, java_class) in type_mappings
        .rust_to_java_iter()
        .filter(|(rust_type, _)| !rust_type.is_builtin())
    {
        let rust_path: syn::Path = syn::parse_str(rust_type.path())
            .unwrap_or_else(|_| panic!("Invalid Rust type path: {}", &rust_type.path()));

        let java_class_dotted = java_class.to_java_dotted();
        let java_class_cstr = lit_cstr_mutf8(&java_class_dotted);

        type_checks.push(quote! {
            {
                // Safety: we have compile-time encoded the name of the Java class as MUTF8
                // and therefore know it's safe to cast as a JNIStr
                let expected_class_name = unsafe {
                    #jni::strings::JNIStr::from_cstr_unchecked(#java_class_cstr)
                };
                let actual_class_name = <#rust_path as #jni::refs::Reference>::class_name();
                assert!(
                    actual_class_name.as_ref() == expected_class_name,
                    "Type mapping mismatch for {}: expected {:?}, got {:?}",
                    stringify!(#rust_path),
                    expected_class_name,
                    actual_class_name
                );
            }
        });
    }

    // Generate compile-time size and alignment checks for primitive type mappings
    for rust_type in type_mappings
        .rust_types()
        .filter(|rt| rt.primitive().is_some())
        .filter(|rt| !rt.is_builtin())
    {
        let rust_path: syn::Path = syn::parse_str(rust_type.path())
            .unwrap_or_else(|_| panic!("Invalid Rust type path: {}", &rust_type.path()));

        let jni_sys_prim = match rust_type.primitive().unwrap() {
            PrimitiveType::Void => {
                unreachable!("Void type should not have a custom type mapping")
            }
            PrimitiveType::Boolean => quote! { jboolean },
            PrimitiveType::Byte => quote! { jbyte },
            PrimitiveType::Char => quote! { jchar },
            PrimitiveType::Short => quote! { jshort },
            PrimitiveType::Int => quote! { jint },
            PrimitiveType::Long => quote! { jlong },
            PrimitiveType::Float => quote! { jfloat },
            PrimitiveType::Double => quote! { jdouble },
        };

        type_checks.push(quote! {
            {
                const _: () =  {
                    assert!(std::mem::size_of::<#jni::sys::#jni_sys_prim>() == std::mem::size_of::<#rust_path>(), concat!("Size mismatch for primitive type ", stringify!(#rust_path)));
                    assert!(std::mem::align_of::<#jni::sys::#jni_sys_prim>() == std::mem::align_of::<#rust_path>(), concat!("Alignment mismatch for primitive type ", stringify!(#rust_path)));
                };
            }
        });
    }

    if type_checks.is_empty() {
        quote! {}
    } else {
        quote! {
            #(#type_checks)*
        }
    }
}
