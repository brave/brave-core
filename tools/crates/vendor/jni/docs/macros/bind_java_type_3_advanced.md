# Advanced Topics

This section covers more advanced features and use cases for the `bind_java_type!` macro.

## Native Method Registration vs Exporting

The JVM can discover native method implementations in two ways:

1. **Exporting with mangled names**: The native method is exported as a symbol with a
   JNI-mangled name (e.g., `Java_com_example_MyType_myMethod__I`). The JVM automatically
   resolves these symbols within a shared library.

2. **Registration via `Env::register_native_methods`**: The native method implementations
   are explicitly registered at runtime by calling `Env::register_native_methods()`, which
   maps Java method signatures to function pointers.

The `bind_java_type!` macro supports both approaches:

### Automatic Registration

The generated `{Type}API::get()` method automatically calls `Env::register_native_methods()`
to register all declared native methods with the JVM. This happens when you first obtain
the API reference for the type.

```rust,ignore
# use jni::sys::jint;
use jni::{Env, bind_java_type};
use jni::refs::LoaderContext;

# bind_java_type! {
#     pub ExampleType => com.example.ExampleType,
#     native_methods { extern fn native_method() -> jint }
# }
# impl ExampleTypeNativeInterface for ExampleTypeAPI {
#     type Error = jni::errors::Error;
#     fn native_method<'local>(_: &mut Env<'local>, _: ExampleType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
# fn initialize(env: &mut Env) -> jni::errors::Result<()> {
# return Ok(()); // Hidden: requires actual Java class
    // Loads class and registers all native methods
    let api = ExampleTypeAPI::get(env, &LoaderContext::default())?;
    Ok(())
}
```

**Note:** Native method registration happens early, potentially before Java static
initializers run. This means native methods are available immediately for use in static
blocks.

### Benefits of Exporting

Exporting native methods (via `extern` qualifier or `export = true`) generates JNI export
symbols that the JVM can resolve when loading a shared library:

**Advantages:**
- Native methods can be resolved before you call `Env::register_native_methods()`
- Useful when Java code calls native methods during early initialization
- Standard JNI discovery mechanism

```rust
# use jni::sys::jint;
use jni::bind_java_type;

bind_java_type! {
    pub MyType => com.example.MyType,
    native_methods {
        // Generates: Java_com_example_MyType_earlyMethod__
        extern fn early_method() -> jint,
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn early_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

### Benefits of Registration

Registration without export symbols is useful in specific scenarios:

**Advantages:**
- Works when your native code is not in a shared library
- Essential for `Env::define_class()` - dynamically loaded classes can still use native methods
- Provides control over when native methods are bound

```rust
# use jni::sys::jint;
use jni::bind_java_type;

bind_java_type! {
    pub MyType => com.example.MyType,
    native_methods_export = false,  // Disable exports
    native_methods {
        fn registration_only() -> jint,
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn registration_only<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

### Combined Approach

The default behavior exports all methods AND registers them. This provides maximum
compatibility:

```rust
# use jni::sys::jint;
use jni::bind_java_type;

bind_java_type! {
    pub MyType => com.example.MyType,
    // Default: native_methods_export = true
    native_methods {
        extern fn both_exported_and_registered() -> jint,
        // Exports Java_com_example_MyType_bothExportedAndRegistered__
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn both_exported_and_registered<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

## Error Handling Policies

Non-raw native methods that return `Result<T, E>` use error policies to convert errors into JNI
exceptions and default values.

### Built-in Policies

**`ThrowRuntimeExAndDefault`** (default):
- Throws a `java.lang.RuntimeException` with the error message
- Returns the default value for the return type

**`LogErrorAndDefault`**:
- Logs the error (without throwing an exception)
- Returns the default value

### Global Policy

Set a default policy for all native methods in a binding:

```rust
# use jni::sys::jint;
use jni::{Env, bind_java_type};

bind_java_type! {
    pub MyType => com.example.MyType,
    native_methods_error_policy = jni::errors::LogErrorAndDefault,
    native_methods {
        extern fn method1() -> jint,  // Uses global policy
        extern fn method2() -> jint,  // Uses global policy
    },
}

# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn method1<'local>(_: &mut Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
#     fn method2<'local>(_: &mut Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

### Per-Method Policy

Override the global policy for specific methods:

```rust
# use jni::sys::jint;
use jni::{Env, bind_java_type};

bind_java_type! {
    pub MyType => com.example.MyType,
    native_methods_error_policy = jni::errors::LogErrorAndDefault,  // Global default
    native_methods {
        extern fn quiet_method() -> jint,  // Uses LogErrorAndDefault

        fn loud_method {
            sig = () -> jint,
            error_policy = jni::errors::ThrowRuntimeExAndDefault,  // Override
        },
    },
}

# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn quiet_method<'local>(_: &mut Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
#     fn loud_method<'local>(_: &mut Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

### Custom Error Policies

You can implement custom error policies by implementing the `ErrorPolicy` trait. See the
[`ErrorPolicy` documentation](https://docs.rs/jni/latest/jni/errors/trait.ErrorPolicy.html)
for details on creating custom implementations.

The built-in policies (`ThrowRuntimeExAndDefault`, `LogErrorAndDefault`, and
`LogContextErrorAndDefault`) provide examples of different error handling strategies.

### Raw Methods and Error Policies

Raw native methods (`raw`) bypass error handling entirely:

```rust
# use jni::sys::jint;
use jni::{EnvUnowned, bind_java_type};

bind_java_type! {
    pub MyType => com.example.MyType,
    native_methods {
        // No catch_unwind, no error policy for raw methods
        raw extern fn raw_method() -> jint,
    },
}

impl MyTypeNativeInterface for MyTypeAPI {
    type Error = jni::errors::Error;

    fn raw_method<'local>(
        _env: EnvUnowned<'local>,
        _this: MyType<'local>,
    ) -> jint {
        // Direct return, no Result
        42
    }
}
```

## Panic Safety with `catch_unwind`

By default, all non-raw native methods are wrapped with `catch_unwind` to prevent Rust panics
from unwinding across the JNI boundary (which will cause the program to abort).

### Default Behavior

```rust
# use jni::sys::jint;
use jni::bind_java_type;

bind_java_type! {
    pub MyType => com.example.MyType,
    native_methods {
        // Automatically wrapped with catch_unwind
        extern fn safe_method() -> jint,
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn safe_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

The generated wrapper effectively does:

```ignore
fn wrapper<'local>(mut unowned_env: EnvUnowned<'local>, this: MyType<'local>) -> jint {
    let outcome = unowned_env.with_env(|env| {
        // Your trait implementation called here
        // Panics are caught
        MyTypeAPI::safe_method(env, this)
    });
    outcome.resolve::<ErrorPolicy>()
}
```

### Disabling `catch_unwind`

If you want to allow your native method to abort on panic or you plan to handle panics
yourself, you can disable the `catch_unwind` wrapper:

```rust,ignore
# use jni::sys::jint;
use jni::bind_java_type;

bind_java_type! {
    pub MyType => com.example.MyType,
    native_methods {
        fn no_unwind {
            sig = () -> jint,
            catch_unwind = false,  // Disable panic catching
        },
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn no_unwind<'local>(_: &mut jni::Env<'local>, _: MyType<'local>) -> Result<jint, Self::Error> { Ok(0) }
# }
```

**Warning:** If your implementation panics with `catch_unwind = false`, it will unwind up to
the JNI boundary and then abort unless you wrap with `catch_unwind` yourself.

### Raw Methods and `catch_unwind`

Raw methods (`raw`) never use `catch_unwind`:

```rust
# use jni::sys::jint;
use jni::{EnvUnowned, bind_java_type};

bind_java_type! {
    pub MyType => com.example.MyType,
    native_methods {
        // No catch_unwind wrapper
        raw extern fn raw_fast() -> jint,
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn raw_fast<'local>(_: jni::EnvUnowned<'local>, _: MyType<'local>) -> jint { 0 }
# }
```

## Type Checking

The macro performs both compile-time and runtime validation to ensure type safety.

### Compile-Time Checks

When using `unsafe` type mappings (for handles, pointers, etc.), the macro generates
compile-time size and alignment checks:

```rust,ignore
# use jni::sys::jint;
use jni::bind_java_type;

#[repr(transparent)]
#[derive(Copy, Clone)]
struct Handle(*const u8);

impl From<Handle> for jlong {
    fn from(h: Handle) -> Self {
        h.0 as jlong
    }
}

bind_java_type! {
    pub MyType => com.example.MyType,
    type_map = {
        // Generates compile-time size/alignment assertions
        unsafe Handle => long,
    },
    native_methods {
        extern fn process_handle(handle: Handle) -> jint,
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn process_handle<'local>(_: &mut jni::Env<'local>, _: MyType<'local>, _: Handle) -> Result<jint, Self::Error> { Ok(0) }
# }
```

The macro validates:
- `size_of::<Handle>() == size_of::<jlong>()`
- `align_of::<Handle>() == align_of::<jlong>()`

### Runtime Checks

The macro also performs runtime validation during API initialization:

**Reference type validation** (controlled by `abi_check`): For each Reference type in `type_map`,
the macro validates that the Rust type corresponds to the declared Java class using `IsInstanceOf`:

```rust
use jni::bind_java_type;
# use jni::objects::JObject;

bind_java_type! {
    pub MyType => com.example.MyType,
    type_map = {
        // Validates at runtime that CustomType is actually com.example.CustomClass
        CustomType => com.example.CustomClass,
    },
}
# bind_java_type! { CustomType => com.example.CustomClass }
```

**is_instance_of validation** (always enabled): For each type declared in `is_instance_of`, the
macro validates the inheritance relationship using `IsInstanceOf`. These checks cannot currently
be disabled:

```rust,ignore
use jni::bind_java_type;
# bind_java_type! { pub BaseClass => com.example.Base }

bind_java_type! {
    pub MyType => com.example.MyType,
    is_instance_of = {
        // Validates at runtime that MyType is an instance of BaseClass
        base: BaseClass,
    },
}
```

**Native method receiver validation** (controlled by `abi_check`): For native methods, the macro
validates that the second parameter matches the method type:

- Instance methods: Must receive an instance object (not `JClass`)
- Static methods: Must receive `JClass`

```rust
# use jni::sys::jint;
use jni::bind_java_type;

bind_java_type! {
    pub MyType => com.example.MyType,
    native_methods {
        // Validates receiver is an instance
        extern fn instance_method(value: jint) -> jint,

        // Validates receiver is JClass
        static extern fn static_method(value: jint) -> jint,
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn instance_method<'local>(_: &mut jni::Env<'local>, _: MyType<'local>, _: jint) -> Result<jint, Self::Error> { Ok(0) }
#     fn static_method<'local>(_: &mut jni::Env<'local>, _: jni::objects::JClass<'local>, _: jint) -> Result<jint, Self::Error> { Ok(0) }
# }
```

### Disabling Type Checks

The `abi_check` property controls all validation (compile-time and runtime). In rare cases,
you may need to disable the checks:

```rust,ignore
use jni::bind_java_type;

bind_java_type! {
    pub MyType => com.example.MyType,
    abi_check = UnsafeNever,  // Disable all checks globally
    type_map = {
        unsafe Handle => long,
        CustomType => com.example.CustomClass,  // No runtime validation
    },
}
# impl From<Handle> for jni::sys::jlong { fn from(h: Handle) -> Self { h.0 as jni::sys::jlong } }
# bind_java_type! { pub CustomType => com.example.CustomClass }
```

Or per-native-method:

```rust,ignore
# use jni::sys::jint;
use jni::bind_java_type;

bind_java_type! {
    pub MyType => com.example.MyType,
    type_map = {
        unsafe Handle => long,
    },
    native_methods {
        fn no_check {
            sig = (handle: Handle) -> jint,
            abi_check = UnsafeNever,  // Disable checks for this method
        },
    },
}
# impl MyTypeNativeInterface for MyTypeAPI {
#     type Error = jni::errors::Error;
#     fn no_check<'local>(_: &mut jni::Env<'local>, _: MyType<'local>, _: Handle) -> Result<jint, Self::Error> { Ok(0) }
# }
```

**Warning:** Disabling checks removes important safety validations. Only use `UnsafeNever`
when you're certain the types are correct and the performance cost of runtime checks is
unacceptable.

## Custom Hooks and Private Data

For advanced use cases, inject custom data and initialization logic into the API struct:

### Private Data

Store custom data in the API struct using `priv_type`:

```rust,ignore
use jni::bind_java_type;

struct MyContext {
    counter: std::sync::atomic::AtomicUsize,
}

bind_java_type! {
    pub MyType => com.example.MyType,
    priv_type = MyContext,
    hooks = {
        init_priv = |_env, _class, _load_context| {
            Ok(MyContext {
                counter: std::sync::atomic::AtomicUsize::new(0),
            })
        },
    },
}
```

The `priv_type` is stored as a `private` field in the API struct and must implement
`Send + Sync`. The `init_priv` hook must be an inline closure that receives:

- `env: &mut Env<'_>` - JNI environment
- `class: &GlobalRef` - The loaded class
- `load_context: &LoaderContext` - Class loading context

The closure must return `Result<PrivType, jni::errors::Error>`.

### Accessing Private Data

The private data is not directly accessible from outside the generated code. It's primarily
useful for internal macro-generated code or when combined with custom hooks.

### Custom Class Loading

Override the default class loading behavior with a `load_class` hook:

```rust,ignore
use jni::bind_java_type;

bind_java_type! {
    pub MyType => com.example.MyType,
    hooks = {
        load_class = |env, load_context, initialize| {
            // Custom class loading logic
            println!("Loading class: com.example.MyType");

            // Use the default loader for this type
            load_context.load_class_for_type::<MyType>(env, initialize)

            // Or implement custom loading:
            // let class = env.find_class("com/example/MyType")?;
            // if initialize {
            //     env.ensure_local_capacity(1)?;
            //     env.call_method(&class, "someInitMethod", "()V", &[])?;
            // }
            // env.new_global_ref(&class)
        },
    },
}
```

The `load_class` hook must be an inline closure that receives:

- `env: &mut Env<'local>` - JNI environment
- `load_context: &LoaderContext` - Class loading context with helper methods
- `initialize: bool` - Whether to initialize the class

The closure must return `Result<JClass<'local>, jni::errors::Error>`.

## Wrapper Macros

Create custom wrapper macros to encapsulate common configuration across multiple bindings:

```rust,ignore
use jni::bind_java_type;

// Define common types
bind_java_type! { pub UserId => com.example.types.UserId }
bind_java_type! { pub Timestamp => com.example.types.Timestamp }

// Create a wrapper macro
macro_rules! my_bind {
    ($($tokens:tt)*) => {
        bind_java_type! {
            // Inject common configuration
            type_map = {
                UserId => com.example.types.UserId,
                Timestamp => com.example.types.Timestamp,
            },
            native_methods_error_policy = jni::errors::LogErrorAndDefault,

            // Forward the rest
            $($tokens)*
        }
    };
}

// Use the wrapper - no need to repeat type_map or error_policy
my_bind! {
    pub User => com.example.User,
    fields {
        id: UserId,
        created: Timestamp,
    },
}

my_bind! {
    pub Post => com.example.Post,
    fields {
        author_id: UserId,
        posted: Timestamp,
    },
}
```

### Overriding JNI Crate Path

If you've renamed the `jni` dependency, use a wrapper to inject the path:

```rust
macro_rules! my_bind {
    ($($tokens:tt)*) => {
        jni::bind_java_type! {
            jni = ::my_renamed_jni,
            $($tokens)*
        }
    };
}
```
