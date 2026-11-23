# Core Properties Reference

This section documents the core properties that define the basic binding structure.

## `rust_type` (required)

The name of the Rust wrapper type to generate.

```rust
# use jni::bind_java_type;
bind_java_type! {
    rust_type = MyType,
    rust_type_vis = pub,
    java_type = com.example.MyClass,
}
```

Normally it's recommended to use the shorthand syntax for the `rust_type`, `rust_type_vis`, and `java_type`:

```rust
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
}
```

## `rust_type_vis` (optional)

The visibility of the generated Rust wrapper type. Defaults to private.

Normally it's recommended to use the shorthand syntax for the `rust_type`, `rust_type_vis`, and `java_type`,
as shown in the `rust_type` section above.

## `java_type` (required)

The fully-qualified Java class name. Can be specified as dot-separated identifiers or as a string literal.

Normally it's recommended to use the shorthand syntax for the `rust_type`, `rust_type_vis`, and `java_type`,
as shown in the `rust_type` section above.

**Syntax options:**

- Standard classes: `java.lang.String`, `com.example.MyClass`
- Inner classes: `com.example.Outer::Inner` or `"com.example.Outer$Inner"`
- Default package: `.MyClass` or `".MyClass"`

```rust
# use jni::bind_java_type;
// Using identifiers
bind_java_type! {
    pub MyType => com.example.MyClass,
}

// Using string literal (useful for inner classes)
bind_java_type! {
    pub MyInner => "com.example.Outer$Inner",
}

// Default package class
bind_java_type! {
    pub MyDefaultPkg => .DefaultPackageClass,
}
```

See the [`jni_sig!`] macro documentation for complete details on Java type syntax.

## `type_map`

Defines mappings between Rust types and Java classes, allowing you to use custom types in signatures.
Multiple `type_map` blocks can be specified and will be merged.

```rust,ignore
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    type_map = {
        // Map Rust type to Java class
        CustomType => com.example.CustomClass,

        // Type alias for readability
        typealias MyCustom => crate::CustomType,

        // Unsafe primitive mapping (for handles, etc.)
        unsafe HandleType => long,
    },
}
```

**Mapping types:**

1. **Reference type mappings**: `RustType => java.class.Name`
   - Maps a Rust [Reference] type to a Java class
   - Used for method/field signatures

2. **Type aliases**: `typealias Alias => path::to::Type`
   - Creates a shorter name for convenience
   - Useful in wrapper macros

3. **Unsafe primitive mappings**: `unsafe RustType => javaPrimitive`
   - Maps a Rust type to a Java primitive (for handles, pointers, etc.)
   - Macro validates size/alignment at compile time
   - Requires `From<RustType> for jlong` (or appropriate primitive) implementation

See the [`jni_sig!`] macro documentation for complete details on type mappings.

## `is_instance_of`

Declares that this type can be safely cast to other [Reference] types. The macro generates
`From` trait implementations and validates the relationships at runtime.

```rust,ignore
# use jni::bind_java_type;
# bind_java_type! { BaseClass => com.example.Base }
bind_java_type! {
    pub MyType => com.example.MyClass,
    is_instance_of = {
        // With stem: generates as_base() method + From traits
        base: BaseClass,

        // Without stem: generates From traits only
        JThrowable,
    },
}
```

**Syntax:**

- `stem: Type` - Generates `as_stem() -> Type<'local>` method and `From<MyType> for Type` implementations
- `Type` - Generates only `From<MyType> for Type` implementations

The generated `as_*()` methods perform a runtime `IsInstanceOf` check.

# Field Blocks Reference (`fields`)

This section documents the `fields` block for defining field bindings.

## Field Block Overview

The `fields` block defines bindings for instance and static fields, generating getter and setter methods.

**Shorthand syntax:**
```text
[visibility] [static] [non_null] name: type
```

**Block syntax:**
```text
[visibility] [static] name {
    property = value,
    ...
}
```

**Common qualifiers:**

- **`visibility`** - `pub`, `priv`, `pub(crate)`, etc. (defaults to `pub`)
- **`static`** - Static field (class-level field)
- **`non_null`** - Validate that field is never null (shorthand implies `non_null = true`)

**Example:**

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
# use jni::objects::JString;
bind_java_type! {
    pub MyType => com.example.MyClass,
    fields {
        // Shorthand: instance fields
        value: jint,
        name: JString,

        // Static field
        static counter: jint,

        // Array field
        values: jint[],

        // With custom visibility
        pub public_field: jint,
        priv private_field: jint,
        pub(crate) crate_field: jint,

        // Block syntax: customized field
        internal_value {
            sig = jint,
            name = "internalValue",
            pub get = get_internal,
            priv set = update_internal,
        },
    },
}
```

**Generated methods:**

- Getter: `pub fn name(&self, env: &mut Env<'local>) -> Result<Type>`
- Setter: `pub fn set_name(&self, env: &mut Env<'local>, value: impl AsRef<Type>) -> Result<()>`

## `sig` - Field Type

The `sig` property defines the field's type signature. In shorthand syntax, this is specified
after the colon. In block syntax, it's explicitly specified with the `sig` property.

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
# use jni::objects::JString;
bind_java_type! {
    pub MyType => com.example.MyClass,
    fields {
        // Shorthand
        value: jint,
        name: JString,

        // Block syntax
        custom {
            sig = jint,
        },
    },
}
```

See the [`jni_sig!`] macro documentation for complete details on type signature syntax.

## `name` - Java Field Name Override

By default, the Rust field name is used as the Java field name. Use the `name` property to
specify a different Java field name.

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
# bind_java_type! { pub MyType => com.example.MyClass,
# fields {
rust_name {
    sig = jint,
    name = "javaFieldName",
}
# }}
```

This is useful when the Java field name conflicts with Rust keywords or conventions.

## `get` and `set` - Custom Accessor Names and Visibility

The `get` and `set` properties customize the generated getter and setter method names, and
can specify different visibility for each accessor.

**Syntax:**
```text
[visibility] get = method_name
[visibility] set = method_name
```

Omit `set` to create a read-only field binding.

```rust,ignore
# use jni::sys::jint;
# use jni::bind_java_type;
# bind_java_type! { pub MyType => com.example.MyClass,
# fields {
// Read-only field (no setter)
static CONSTANT {
    sig = jint,
    get = CONSTANT,
}

// Custom names with different visibility
internal_value {
    sig = jint,
    /// Gets the internal value (public)
    pub get = get_internal,
    /// Updates the internal value (private)
    priv set = update_internal,
}

// Public getter, crate-visible setter
config_value {
    sig = jint,
    /// Retrieves the configuration value
    pub get = config_value,
    /// Sets the configuration value (crate-internal)
    pub(crate) set = set_config_value,
}
# }}
```

## Documentation Comments

Documentation comments can be added to fields and will be applied to the generated getter
and setter methods:

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
# use jni::objects::JString;
bind_java_type! {
    pub MyType => com.example.MyClass,
    fields {
        /// This documentation appears on the getter
        value: jint,

        // With explicit getter/setter docs
        name {
            sig = JString,
            /// Gets the name
            get = name,
            /// Sets the name
            set = set_name,
        },
    },
}
```

## `non_null` - Null Validation for Fields

The `non_null` property validates that field getters return non-null values and field setters
do not accept null values, treating `null` as an error.

**Syntax:**
```text
non_null = true | false   // In block syntax (default: false)
non_null field_name: Type // In shorthand syntax (implies non_null = true)
```

**Behavior:**
- If `non_null = true` and the field getter returns `null`, it returns `Err(Error::NullPtr("Null Object"))`
- If `non_null = true` and you try to set the field to `null`, it returns `Err(Error::NullPtr("Null Object"))`
- `Error::JavaException` has higher precedence - Java exceptions are always returned first

```rust,ignore
# use jni::bind_java_type;
# use jni::objects::JString;
# bind_java_type! { pub MyType => com.example.MyClass,
# fields {
// Shorthand syntax - non_null qualifier
non_null required_name: JString,

// Block syntax - explicit property
optional_name {
    sig = JString,
    non_null = false,  // Default - null is allowed
},

validated_name {
    sig = JString,
    non_null = true,   // Null causes Error::NullPtr
},
# }}
```

**Restrictions:**

- Cannot be used with fields of primitive types

This is useful when Java fields are not expected to be `null` normally and it's logically an error
if they are but the implementation may not throw an exception in this case.

# Method Blocks Common Reference

This section documents properties and syntax common to all method blocks.

There are three method blocks available for defining different types of methods:

- [`constructors`](#constructor-blocks-reference-constructors) - Defines constructor bindings that call Java `<init>` methods.
- [`methods`](#method-blocks-reference-methods) - Defines instance and static method bindings.
- [`native_methods`](#native-method-blocks-reference-native_methods) - Defines native methods implemented in Rust and called from Java.

## Method Block Syntax

All three method blocks support two syntax forms with common qualifiers:

**Shorthand syntax:**
```text
[visibility] [static] [raw] [extern] [non_null] fn name(args...) -> return_type
```

**Block syntax:**
```text
[visibility] [static] [raw] [extern] fn name {
    property = value,
    ...
}
```

**Common qualifiers:**

- **`visibility`** - `pub`, `priv`, `pub(crate)`, etc. (defaults to `pub` in `methods` and `constructors` blocks, no default for `native_methods`)
- **`static`** - Static method (for `methods` and `native_methods` only)
- **`raw`** - Raw JNI method (for `native_methods` only)
- **`extern`** - Export JNI symbol (for `native_methods` only, equivalent to `export = true`)
- **`non_null`** - Validate that method returns non-null (for `methods` and `native_methods` only, shorthand implies `non_null = true`)

**Example:**

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
# use jni::objects::JString;
bind_java_type! {
    pub MyType => com.example.MyClass,
    constructors {
        fn new(),
        fn with_value(value: jint),
    },
    methods {
        // Shorthand: instance method
        fn get_value() -> jint,

        // Shorthand: static method with visibility
        pub static fn create_default() -> jint,
    },
    native_methods {
        // Shorthand: exported native method
        extern fn native_add(a: jint, b: jint) -> jint,

        // Block: customized native method
        pub static extern fn native_custom {
            sig = (value: jint) -> jint,
            error_policy = jni::errors::LogErrorAndDefault,
        },
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn native_add<'local>(_env: &mut jni::Env<'local>, _obj: MyType<'local>, a: jint, b: jint) -> Result<jint, jni::errors::Error> { Ok(a + b) }
#     fn native_custom<'local>(_env: &mut jni::Env<'local>, _class: jni::objects::JClass<'local>, value: jint) -> Result<jint, jni::errors::Error> { Ok(value) }
# }
```

## `sig` - Method Signature

The `sig` property defines the method's type signature. In shorthand syntax, this is the parameter
list and return type. In block syntax, it's explicitly specified.

**Syntax:**
```text
sig = (param1: Type1, param2: Type2, ...) [-> ReturnType]
```

Omitting the return type is the same as specifying `-> void` or `-> ()`.

For constructors the return type must always be void.

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
# use jni::objects::JString;
bind_java_type! {
    pub MyType => com.example.MyClass,
    constructors {
        fn new(),
        fn with_value(value: jint),
    },
    methods {
        fn get_value() -> jint,
        fn set_name(name: JString),
    },
}
```

See the [`jni_sig!`] macro documentation for complete details on type signature syntax.

## `name` - Java Method Name Override

By default, the Rust method name is used as the Java method name. Use the `name` property to
specify a different Java method name.

```rust
# use jni::bind_java_type;
# bind_java_type! { pub MyType => com.example.MyClass,
# methods {
fn rust_name {
    sig = (),
    name = "javaMethodName",
}
# }}
```

This is useful when the Java method name conflicts with Rust keywords or conventions.

## `non_null` - Null Validation for Methods

The `non_null` property validates that methods return non-null object references, treating `null` as
an error even when no Java exception was thrown.

**Syntax:**

```text
non_null = true | false  // In block syntax (default: false)
non_null fn name(...)     // In shorthand syntax (implies non_null = true)
```

**Behavior:**

- If `non_null = true` and the method returns `null`, it returns `Err(Error::NullPtr("Null Object"))`
- `Error::JavaException` has higher precedence - Java exceptions are always returned first

```rust,no_run
# use jni::bind_java_type;
# use jni::objects::JString;
# bind_java_type! { pub MyType => com.example.MyClass,
# methods {
// Shorthand syntax - non_null qualifier
non_null fn get_required_name() -> JString,

// Block syntax - explicit property
fn get_optional_name {
    sig = () -> JString,
    non_null = false,  // Default - null is allowed
},

fn get_validated_name {
    sig = () -> JString,
    non_null = true,   // Null causes Error::NullPtr
},
# }}
```

**Restrictions:**

- Cannot be used with constructors
- Cannot be used with methods that return primitive types or void
- For native methods: only affects bindings with a visibility specifier (callable from Rust), not the implementation

This is useful when Java APIs are not expected to return `null` normally and it's logically an error
if they do, but they also may not throw an exception in this case.

**Note for native methods:** The `non_null` property only affects the generated Rust API for calling
the native method (when a visibility specifier is present). It does not affect the implementation of
the native method itself.

# Constructor Blocks Reference (`constructors`)

For an overview of method block syntax, see [Method Blocks Common Reference](#method-blocks-common-reference).

Defines constructor bindings that call the Java class's `<init>` methods.

```rust
# use jni::bind_java_type;
# use jni::objects::JString;
bind_java_type! {
    pub MyType => com.example.MyClass,
    constructors {
        fn new(),
        fn with_value(value: jint),
        fn with_name_and_value(name: JString, value: jint),
    },
}
```

Generated constructors have the signature:
```rust,ignore
pub fn name(env: &mut Env<'local>, ...) -> jni::errors::Result<MyType<'local>>
```

Constructors do not support the `static` qualifier.

# Method Blocks Reference (`methods`)

For an overview of method block syntax, see [Method Blocks Common Reference](#method-blocks-common-reference).

Defines bindings for instance and static methods.

```rust,ignore
# use jni::sys::jint;
# use jni::bind_java_type;
# use jni::objects::JString;
bind_java_type! {
    pub MyType => com.example.MyClass,
    methods {
        // Instance methods
        fn get_value() -> jint,
        fn set_name(name: JString),

        // Static methods
        static fn create_default() -> MyType,
        static fn get_constant() -> jint,
    },
}
```

**Generated method signatures:**

Instance methods:
```rust,ignore
pub fn name(&self, env: &mut Env<'local>, ...) -> jni::errors::Result<ReturnType>
```

Static methods:
```rust,ignore
pub fn name(env: &mut Env<'local>, ...) -> jni::errors::Result<ReturnType>
```

# Native Method Blocks Reference (`native_methods`)

For an overview of method block syntax, see [Method Blocks Common Reference](#method-blocks-common-reference).

Defines native methods that are implemented in Rust and called from Java.

```rust,ignore
# use jni::sys::jint;
# use jni::bind_java_type;
# use jni::objects::JString;
bind_java_type! {
    pub MyType => com.example.MyClass,
    native_methods {
        // Exported instance method (implemented via trait)
        extern fn native_add(a: jint, b: jint) -> jint,

        // Static native method
        static extern fn native_greet(name: JString) -> JString,

        // Raw method (receives EnvUnowned directly)
        raw extern fn native_raw(value: jint) -> jint,
    },
}
```

**Additional qualifiers for native_methods:**

- `extern` - Generates JNI export symbol (equivalent to `export = true`)
- `raw` - Receives `EnvUnowned` directly, no automatic error handling or panic catching

**Implementation methods:**

1. **Via trait** (default): Implement the generated `{Type}NativeInterface` trait
2. **Direct function**: Provide `fn = path::to::function` to bypass the trait

## Visibility - Making Native Methods Callable from Rust

Native methods can optionally include a visibility specifier (`pub`, `pub(crate)`, etc.) to generate
a Rust API for calling the native method from Rust code. Without a visibility specifier, the method
will only have a Rust implementation that can be called from Java or dynamically via JNI.

**With visibility** - Generates both implementation and callable API:
```rust
# use jni::sys::jint;
# use jni::bind_java_type;
# bind_java_type! { pub MyType => com.example.MyClass,
# native_methods {
pub extern fn native_method(value: jint) -> jint,
# }}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn native_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>, _: jint) -> Result<jint, Self::Error> { Ok(0) }
# }
```

This generates:
- Trait method for implementation: `fn native_method(...) -> Result<jint, Error>`
- Callable Rust API: `pub fn native_method(&self, env: &mut Env, value: jint) -> Result<jint>`
- JNI export symbol (due to `extern`)

**Without visibility** - Implementation only:
```rust
# use jni::sys::jint;
# use jni::bind_java_type;
# bind_java_type! { pub MyType => com.example.MyClass,
# native_methods {
extern fn native_method(value: jint) -> jint,
# }}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn native_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>, _: jint) -> Result<jint, Self::Error> { Ok(0) }
# }
```

This generates:
- Trait method for implementation: `fn native_method(...) -> Result<jint, Error>`
- JNI export symbol (due to `extern`)
- **No** callable Rust API (can only be called from Java)

This distinction is important because some properties like `non_null` only affect the callable Rust API,
not the implementation.

## `fn` - Direct Function Implementation

Instead of implementing the native methods trait, you can provide a direct function implementation.

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
# use jni::Env;
# bind_java_type! { pub MyType => com.example.MyClass,
# native_methods {
fn native_method {
    sig = (value: jint) -> jint,
    fn = my_implementation_function,
}
# }}
# fn my_implementation_function<'local>(_env: &mut Env<'local>, _this: MyType<'local>, value: jint) -> Result<jint, jni::errors::Error> { Ok(value) }
```

The function must have the appropriate signature for the method type (instance/static/raw).

## `export` - JNI Symbol Export Control

Controls whether a JNI export symbol is generated for this native method. Defaults to the
value of `native_methods_export` (which defaults to `true`).

**Values:**

- `true` - Generate JNI export symbol with standard name (equivalent to `extern` qualifier)
- `false` - No export symbol (method registration only)
- `"CustomName"` - Export with exact symbol name (e.g., `"Java_com_example_myMethod"`)

The `extern` qualifier in shorthand syntax is equivalent to `export = true`.

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    native_methods_export = false,  // Don't export by default (otherwise export = true / extern are redundant)
    native_methods {
        // Exported with standard name (extern is shorthand for export = true)
        extern fn public_native() -> jint,

        // Equivalent using block syntax
        fn also_public {
            sig = () -> jint,
            export = true,
        },

        // Custom symbol name
        fn custom_symbol {
            sig = () -> jint,
            export = "Java_com_example_CustomName",
        },

        // Not exported (registration only)
        fn internal_native {
            sig = () -> jint,
            export = false,
        },
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn public_native<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
#     fn also_public<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
#     fn custom_symbol<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
#     fn internal_native<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

## `error_policy` - Error Handling Policy

Specifies how errors from native methods are handled. When a native method returns
`Result<T, E>`, the error policy can convert errors to JNI exceptions and default values.

The default policy is `jni::errors::ThrowRuntimeExAndDefault`, which throws a `RuntimeException`
for errors and returns a default value.

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    native_methods {
        fn native_method {
            sig = (value: jint) -> jint,
            error_policy = jni::errors::LogErrorAndDefault,
        },
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn native_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>, value: jint) -> Result<jint, Self::Error> { Ok(value) }
# }
```

See the [Advanced Topics](bind_java_type_advanced.md#native-method-error-handling) chapter
for details on error policies and custom implementations.

## `catch_unwind` - Panic Safety

Controls whether the native method wrapper catches Rust panics. Defaults to `true`.

`catch_unwind` only applies to non-raw methods and it's an error to set it for a raw method.

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    native_methods {
        fn native_method {
            sig = (value: jint) -> jint,
            catch_unwind = false,  // Panics will abort the process
        },
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn native_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>, value: jint) -> Result<jint, Self::Error> { Ok(value) }
# }
```

**Warning:** Without `catch_unwind`, any panic will abort the process when unwinding reaches
the FFI boundary.

## Trait Implementation

By default, native methods are implemented via a generated trait. For example:

```rust
# use jni::{Env, bind_java_type};
# use jni::objects::JString;
# use jni::sys::jint;
# bind_java_type! {
#     pub MyType => com.example.MyClass,
#     native_methods {
#         extern fn native_add(a: jint, b: jint) -> jint,
#     }
# }
impl MyTypeNativeInterface for MyTypeAPI {
    type Error = jni::errors::Error;

    fn native_add<'local>(
        env: &mut Env<'local>,
        this: MyType<'local>,
        a: jint,
        b: jint,
    ) -> Result<jint, Self::Error> {
        Ok(a + b)
    }
}
```

See the [Native Methods](bind_java_type_examples.md#native-methods) section in the Examples
chapter for complete details.

# Advanced Properties Reference

This section documents advanced configuration properties for specialized use cases.

## `api`

Custom name for the generated API struct. Default is `{Type}API`.

```rust,ignore
# use jni::bind_java_type;
bind_java_type! {
    rust_type = MyType,
    java_type = com.example.MyClass,
    api = MyCustomAPIStruct,
}
```

## `native_trait`

Custom name for the generated native methods trait. Default is `{Type}NativeInterface`.

```rust,ignore
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    native_trait = MyCustomNativeTrait,
    native_methods {
        extern fn native_method() -> jint,
    },
}
```

## `native_methods_export`

Controls whether native methods generate JNI export symbols by default. Default is `true`.
Individual methods can override with `export = true/false` or the `extern` qualifier.

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    native_methods_export = false,  // Don't export by default
    native_methods {
        // Not exported
        fn internal_native() -> jint,

        // Explicitly exported with block syntax
        fn public_native {
            sig = () -> jint,
            export = true,
        },

        // Explicitly exported with extern qualifier (shorthand)
        extern fn another_public_native(value: jint) -> jint,
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn internal_native<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
#     fn public_native<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
#     fn another_public_native<'local>(_: &mut jni::Env<'local>, _: MyType<'local>, _: jint) -> Result<jint, Self::Error> { Ok(0) }
# }
```

## `native_methods_error_policy`

Sets a global default error handling policy for all non-raw native methods. When a native
method returns `Result<T, E>`, this policy determines how errors are converted to JNI
exceptions and default values.

**Built-in policies:**

- `jni::errors::ThrowRuntimeExAndDefault` - Throws `RuntimeException`, returns default value (default)
- `jni::errors::LogErrorAndDefault` - Logs error, returns default value

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    native_methods_error_policy = jni::errors::LogErrorAndDefault,
    native_methods {
        extern fn native_method() -> jint,  // Uses global policy

        fn custom_policy {
            sig = () -> jint,
            error_policy = jni::errors::ThrowRuntimeExAndDefault,  // Override
        },
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn native_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
#     fn custom_policy<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

Individual methods can override the global policy with their own `error_policy` property.

## `native_methods_catch_unwind`

Controls whether native methods catch Rust panics by default. Defaults to `true`.
Individual methods can override with `catch_unwind = true/false`.

```rust
# use jni::sys::jint;
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    native_methods_catch_unwind = false,  // Don't catch panics by default
    native_methods {
        // No panic catching (unsafe!)
        extern fn native_method() -> jint,

        // Explicitly enable panic catching
        fn safe_method {
            sig = () -> jint,
            catch_unwind = true,
        },
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn native_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
#     fn safe_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

**Warning:** Without `catch_unwind`, any panic in a native method will abort the process when
unwinding reaches the `extern "system"` FFI boundary.

## `abi_check`

Controls validation of type mappings at compile-time and runtime. Defaults to `Always`
(checks enabled in all builds).

**Validation performed:**

1. **Compile-time ABI checks** - Validates size/alignment of unsafe primitive type mappings
2. **Runtime type_map Reference checks** - Validates that Reference types in `type_map` correspond to their declared Java classes
3. **Runtime static/instance checks for native methods** - Validates whether the second, receiver parameter is a `JClass` (for static methods) or not (for instance methods)

**Note:** The runtime checks are either performed once during API initialization or the once when a native method is first called, depending on the check type.

**Available values:**

- `Always` - Enable checks in all builds (default)
- `UnsafeDebugOnly` - Enable checks in debug builds only
- `UnsafeNever` - Disable checks entirely (not recommended)

```rust
# use jni::bind_java_type;
# #[repr(transparent)] #[derive(Copy, Clone)] struct Handle(*const u8);
# bind_java_type! { CustomType => com.example.CustomClass }
bind_java_type! {
    pub MyType => com.example.MyClass,
    abi_check = Always,  // Enable checks in all builds
    type_map = {
        // Compile-time: validates Handle has the size and alignment of a jlong
        unsafe Handle => long,

        // Runtime: validates CustomType is actually com.example.CustomClass
        CustomType => com.example.CustomClass,
    },
}
# impl From<Handle> for jni::sys::jlong { fn from(h: Handle) -> Self { h.0 as jni::sys::jlong } }
```

Individual native methods can also override the global setting. See the
[ABI Safety Checks](bind_java_type_advanced.md#abi-safety-checks) section in the Advanced
chapter for details.

## `jni`

Override the path to the `jni` crate. Must be specified first if provided.

```rust,ignore
# use jni::bind_java_type;
bind_java_type! {
    jni = ::my_jni_crate,
    rust_type = MyType,
    java_type = com.example.MyClass,
}
```

## `priv_type` and `init_priv` Hook

For advanced use cases, you can inject custom data into the API struct.

```rust
# use jni::bind_java_type;
# struct MyCustomData;
# impl MyCustomData { fn new() -> Self { MyCustomData } }
# unsafe impl Send for MyCustomData {}
# unsafe impl Sync for MyCustomData {}
bind_java_type! {
    pub MyType => com.example.MyClass,
    priv_type = MyCustomData,
    hooks = {
        init_priv = |_env, _class, _load_context| {
            Ok(MyCustomData::new())
        },
    },
}
```

The `priv_type` must implement `Send + Sync` and is stored as a `private` field in the API struct.
The `init_priv` hook must be an inline closure (not a function name) that receives `env: &mut Env`,
`class: &GlobalRef`, and `load_context: &LoaderContext`. It must return
`Result<PrivType, jni::errors::Error>` and is called during API initialization to create the
private data.

See the [Private Data](bind_java_type_advanced.md#private-data) section in the Advanced chapter
for complete details.

## `load_class` Hook

Override the default class loading behavior with a custom class loader.

```rust,ignore
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    hooks = {
        load_class = |env, load_context, initialize| {
            // Custom loading logic here
            // Use the default loader:
            load_context.load_class_for_type::<MyType>(env, initialize)

            // Or custom implementation:
            // let class = env.find_class("com/example/MyClass")?;
            // env.new_global_ref(&class)
        },
    },
}
```

The `load_class` hook must be an inline closure (not a function name) that receives
`env: &mut Env`, `load_context: &LoaderContext`, and `initialize: bool`. It must return
`Result<GlobalRef, jni::errors::Error>`. This allows custom class loading behavior such as using
specific class loaders, loading from non-standard locations, or applying bytecode transformations.

See the [Custom Class Loading](bind_java_type_advanced.md#custom-class-loading) section
in the Advanced chapter for complete details.

[Reference]: https://docs.rs/jni/latest/jni/refs/trait.Reference.html
[`jni_sig!`]: macro.jni_sig.html
