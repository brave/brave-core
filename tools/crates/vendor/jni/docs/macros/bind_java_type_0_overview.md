Binds a Java class to a Rust type with constructors, methods, fields, and native methods.

This macro generates a complete [Reference] type binding for a Java class, providing:

- A Rust wrapper type that implements the [Reference] trait
- An API struct that caches the class reference and all method/field IDs
- Type-safe constructor, method, and field accessors
- A trait-based interface for implementing native methods
- Automatic native method registration with the JVM
- Optional JNI export symbols for native methods

The generated bindings include runtime validation to ensure that type mappings and
class hierarchies are correct.

[Reference]: https://docs.rs/jni/latest/jni/refs/trait.Reference.html

# Quick Start

The simplest form uses shorthand syntax for trivial bindings:

```rust
use jni::bind_java_type;

// Minimal binding - just creates the wrapper type
bind_java_type! { pub MyType => com.example.MyClass }
```

For more complete bindings with methods and fields:

```rust
use jni::bind_java_type;
use jni::Env;
use jni::objects::JString;
use jni::sys::jint;

bind_java_type! {
    rust_type = Counter,
    java_type = com.example.Counter,

    constructors {
        fn new(),
        fn with_initial(value: jint),
    },

    methods {
        fn increment(),
        fn get_value() -> jint,
        static fn get_version() -> JString,
    },

    fields {
        value: jint,
        name: JString,
    },
}

// Use the generated binding
fn example(env: &mut Env) -> jni::errors::Result<()> {
    let counter = Counter::new(env)?;
    counter.increment(env)?;
    let value = counter.get_value(env)?;
    println!("Counter value: {}", value);
    Ok(())
}
```

# Generated Code

The macro generates:

- **`struct {Type}<'local>`** - A [Reference] wrapper type for the Java object
- **`struct {Type}API`** - A singleton API struct that caches the class reference and all method/field IDs
- **`impl Reference for {Type}<'local>`** - Implements the [Reference] trait
- **`trait {Type}NativeInterface`** - A trait for implementing native methods (if any are declared)
- **`impl Deref<Target=JObject<'local>> for {Type}<'local>`** - Deref to `JObject`, base class, for convenience
- **`impl From<{Type}<'local>> for <IsInstanceOfType>`** - Conversions for `is_instance_of` types + `JObject`
- **`impl AsRef<IsInstanceOfType<'local>> for {Type}<'local>`** - Casting to `is_instance_of` types + `JObject` via `AsRef`

## API Initialization

The `{Type}API::get(&mut Env, &LoaderContext)` method:

- Loads and caches the Java class reference (using `LoaderContext` if needed)
- Caches all method IDs and field IDs for fast access
- Validates type mappings and class relationships at runtime
- Registers native methods with the JVM (if any are declared)

This initialization happens lazily on the first call to `get()` and is thread-safe.

# Shorthand vs Block Syntax

The macro supports two forms:

**Shorthand syntax** for simple bindings:
```rust
# use jni::bind_java_type;
bind_java_type! { pub MyType => com.example.MyClass }
```

**Block syntax** for full control:
```rust
# use jni::bind_java_type;
bind_java_type! {
    rust_type = MyType,
    rust_type_vis = pub,
    java_type = com.example.MyClass,
    // ... additional properties
}
```

Both forms can be mixed - you can use shorthand for the type names and add block properties:
```rust
# use jni::bind_java_type;
bind_java_type! {
    pub MyType => com.example.MyClass,
    constructors { fn new() },
}
```

# Method and Field Syntax

Methods and fields also support shorthand and block syntax.

**Shorthand syntax** (recommended when possible):
```rust
# use jni::bind_java_type;
# bind_java_type! { pub MyType => com.example.MyClass,
methods {
    fn add(a: jint, b: jint) -> jint,
    static fn get_version() -> jint,
}
# }
```

**Block syntax** for additional properties:
```rust
# use jni::bind_java_type;
# use jni::objects::JString;
# bind_java_type! { pub MyType => com.example.MyClass,
methods {
    fn my_method {
        sig = (value: jint),
        name = "myJavaMethod",
    },
}
# }
```

See the [Properties Reference](bind_java_type_properties.md) and sections below for complete details
on all available properties and syntax options.

# Type Signatures

Method and field signatures use a Rust-like syntax that maps to JNI types. The full syntax
is documented in the [`jni_sig!`] macro, but here are the key points:

**Primitive types**: `jboolean`, `jbyte`, `jchar`, `jshort`, `jint`, `jlong`, `jfloat`, `jdouble`, `void`

**Reference types**:
- Built-in JNI types: `JObject`, `JString`, `JClass`, etc.
- Java classes: `java.lang.String`, `com.example.MyClass`
- Custom mapped types: `MyCustomType` (requires `type_map`)

**Arrays**:
- Prefix: `[jint]`, `[[JString]]`
- Suffix: `jint[]`, `JString[][]`

For complete details on type syntax, see the [`jni_sig!`] macro documentation.

# Name Conversion

When no explicit Java name is provided for methods or fields, Rust `snake_case` names are
automatically converted to Java `lowerCamelCase`:

- `get_user_name` → `getUserName`
- `my_2d_array` → `my2DArray`
- `_internal_method` → `internalMethod` (only one leading underscore is removed)

Names with existing uppercase letters are preserved as-is (e.g., `MY_CONSTANT` stays `MY_CONSTANT`).

You can always override the Java name explicitly:
```rust
# use jni::bind_java_type;
# bind_java_type! { pub MyType => com.example.MyClass,
methods {
    fn my_method {
        sig = (),
        name = "actualJavaName",
    },
}
# }
```

# == INDEX ==

## Properties Reference

- **[Core Properties](bind_java_type_properties.md#core-properties-reference)** - `rust_type`, `java_type`, `type_map`, `is_instance_of`
- **[Field Blocks](bind_java_type_properties.md#field-blocks-reference-fields)** - Defining field bindings with getters/setters
- **[Method Blocks Common](bind_java_type_properties.md#method-blocks-common-reference)** - Syntax shared across all method blocks
  - **[Constructor Blocks](bind_java_type_properties.md#constructor-blocks-reference-constructors)** - Binding Java constructors
  - **[Method Blocks](bind_java_type_properties.md#method-blocks-reference-methods)** - Instance and static methods
  - **[Native Method Blocks](bind_java_type_properties.md#native-method-blocks-reference-native_methods)** - Implementing native methods in Rust
- **[Advanced Properties](bind_java_type_properties.md#advanced-properties-reference)** - API customization and hooks

## Additional Resources

- **[Examples](bind_java_type_examples.md)** - Practical examples covering common use cases
- **[Advanced Topics](bind_java_type_advanced.md)** - Custom error policies, hooks, and wrapper macros

[`jni_sig!`]: macro.jni_sig.html
