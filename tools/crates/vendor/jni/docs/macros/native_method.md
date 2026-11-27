Create a compile-time type-checked `NativeMethod` for registering native methods with the JVM.

This macro generates a [`NativeMethod`] struct with compile-time guarantees that the Rust
function matches the JNI signature. It can optionally:
- Wrap implementations with panic safety (`catch_unwind`) and error handling
- Generate JNI export symbols for automatic JVM resolution
- Perform runtime ABI checks to ensure static/instance methods are registered correctly

This macro provides strong type safety for implementing individual native methods.

[`NativeMethod`]: https://docs.rs/jni/latest/jni/struct.NativeMethod.html

# Quick Example

```rust
# use jni::{Env, native_method, objects::JObject, sys::jint};
// Instance method with default settings
const ADD_METHOD: jni::NativeMethod = native_method! {
    java_type = "com.example.MyClass",
    extern fn native_add(a: jint, b: jint) -> jint,
};
// Will export `Java_com_example_MyClass_nativeAdd__II` symbol and
// `ADD_METHOD` can be passed to `Env::register_native_methods`

fn native_add<'local>(
    _env: &mut Env<'local>,
    _this: JObject<'local>,
    a: jint,
    b: jint,
) -> Result<jint, jni::errors::Error> {
    Ok(a + b)
}
```

# Syntax Overview

The macro supports both property-based and shorthand syntax, which can be combined:

```ignore
native_method! {
    [java_type = "com.example.MyClass",]  // For exports (required with `extern` or `export = true`)
    [rust_type = CustomType,]             // Type for 'this' (default: JObject)
    [static] [raw] [extern] fn [RustType::]method_name(args) -> ret, // Shorthand signature
    [fn = implementation_fn,]             // Function path (default: RustType::method_name from shorthand)
    [... other properties ...]
}
```

# Generated Code

The macro generates a `const` block containing:
1. A type-checked wrapper function
2. An optional runtime type-check for the second parameter (to distinguish static vs instance
   methods)
3. An optional export function with a mangled JNI name
4. A `NativeMethod` struct created via `NativeMethod::from_raw_parts`

For non-raw methods with the default settings:

```ignore
const {
    // Generated wrapper with panic safety and error handling
    extern "system" fn __native_method_wrapper<'local>(
        mut unowned_env: EnvUnowned<'local>,
        this: JObject<'local>,
        a: jint,
        b: jint,
    ) -> jint {
        // One-time ABI check: validates that 'this' is NOT a Class (i.e., instance method)
        static _ABI_CHECK: ::std::sync::Once = ::std::sync::Once::new();
        _ABI_CHECK.call_once(|| {
            // ... check that second parameter is not java.lang.Class ...
        });

        unowned_env
            .with_env(|env| {
                // Call your implementation
                native_add(env, this, a, b)
            })
            .resolve::<ThrowRuntimeExAndDefault>()
    }

    unsafe {
        NativeMethod::from_raw_parts(
            jni_str!("nativeAdd"),
            jni_str!("(II)I"),
            __native_method_wrapper as *mut c_void,
        )
    }
}
```

With `export = true` or `extern` qualifier, an additional export function is generated:

```ignore
#[export_name = "Java_com_example_MyClass_nativeAdd__II"]
pub extern "system" fn __native_method_export<'local>(...) -> jint {
    __native_method_wrapper(...)
}
```

The export_name is mangled according to the JNI Specification, under "Design" ->
"Resolving Native Method Names"

<https://docs.oracle.com/en/java/javase/11/docs/specs/jni/design.html#resolving-native-method-names>

# Property Reference

## `java_type` - Java Class Name

**Required** when using `export = true` or the `extern` qualifier.

The fully-qualified Java class name containing this native method.

```ignore
java_type = "com.example.MyClass"
```

Can also be specified as dot-separated identifiers:
```ignore
java_type = com.example.MyClass
```

See the 'Java Object Types' section in the [`jni_sig!`] documentation for details on how to
specify Java types, including inner classes and default-package classes.

## `rust_type` - Custom Type for 'this' Parameter

For instance methods, specifies the Rust type for the `this` parameter. Defaults to `JObject`.

```ignore
rust_type = MyCustomType
```

This type must implement `jni::refs::Reference`.

## Shorthand Signature

The shorthand syntax allows specifying method details in a function-like form:

```ignore
[static] [raw] [extern] fn [RustType::]method_name(args) -> ret
```

Where:
- `static` - Static method (receives `class: JClass` instead of `this`)
- `raw` - No panic safety wrapper, receives `EnvUnowned`, returns value directly (not `Result`)
- `extern` - Generate JNI export symbol (requires `java_type`)
- `RustType::` - If present, sets `rust_type = RustType` and defaults `fn =
  RustType::method_name`
- `method_name` - Converted from snake_case to lowerCamelCase for the Java method name

and the `args` and `ret` specify the method signature using the syntax from [`jni_sig!`].

Example:
```rust
# use jni::{Env, native_method, sys::jint};
# struct MyType<'a>(std::marker::PhantomData<&'a ()>);
const METHOD: jni::NativeMethod = native_method! {
    static fn MyType::compute_sum(a: jint, b: jint) -> jint,
};

impl MyType<'_> {
    fn compute_sum<'local>(
        _env: &mut Env<'local>,
        _class: jni::objects::JClass<'local>,
        a: jint,
        b: jint,
    ) -> Result<jint, jni::errors::Error> {
        Ok(a + b)
    }
}
```

## `fn` - Implementation Function Path

Path to the Rust function implementing this native method. Defaults to `RustType::method_name`
or `method_name` if a shorthand signature is given.

```ignore
fn = my_module::my_implementation
```

## `name` - Java Method Name

The Java method name as a string literal. Defaults to the `method_name` name converted from
`snake_case` to `lowerCamelCase` if a shorthand signature is given.

```ignore
name = "customMethodName"
```

## `sig` / Method Signature

Typically the signature will come from the shorthand syntax, but it can also be specified
explicitly via the `sig` property.

The method signature using the syntax from [`jni_sig!`].

```ignore
sig = (param1: jint, param2: JString) -> jboolean
// or shorthand (part of function-like syntax):
fn my_method(param1: jint, param2: JString) -> jboolean
```

## `type_map` - Type Mappings

Optional type mappings for custom Rust types. See [`jni_sig!`] for full syntax.

```ignore
type_map = {
    CustomType => com.example.CustomClass,
    unsafe HandleType => long,
}
```

## `static` - Static Method Flag

Indicates a static method. The second parameter will be `class: JClass` instead of a `this`
object.

```ignore
static = true
// or as qualifier:
static fn my_method() -> jint
```

## `raw` - Raw Function Flag

When `raw = true` or the `raw` qualifier is used:
- Function receives `EnvUnowned<'local>` (not `&mut Env<'local>`)
- Function returns the value directly (not `Result`)
- No panic safety wrapper (`catch_unwind`)
- No automatic error handling

```ignore
raw = true
// or as qualifier:
raw fn my_method(value: jint) -> jint
```

Raw function signature:
```ignore
fn my_raw_method<'local>(
    env: EnvUnowned<'local>,
    this: JObject<'local>,
    value: jint,
) -> jint {
    value * 2
}
```

## `export` - JNI Export Symbol

Controls whether a JNI export symbol is generated:
- `true` - Generate auto-mangled JNI export name (e.g., `Java_com_example_Class_method__II`)
- `false` - Don't generate export
- `"CustomName"` - Use custom export name

Specifying the `extern` qualifier is equivalent to `export = true`.

**Note:** `java_type` must be provided when `export = true` or the `extern` qualifier is used.

```ignore
export = true
// or as qualifier:
extern fn my_method() -> jint
// or with custom name:
export = "Java_custom_Name"
```

## `error_policy` - Error Handling Policy

For non-raw methods, specifies how to convert `Result` errors to JNI exceptions. Default is
`ThrowRuntimeExAndDefault`.

Built-in policies:
- `jni::errors::ThrowRuntimeExAndDefault` - Throws `RuntimeException`, returns default value
- `jni::errors::LogErrorAndDefault` - Logs error, returns default value

Or implement your own policy by implementing the `jni::errors::ErrorPolicy` trait.

```ignore
error_policy = jni::errors::LogErrorAndDefault
```

## `catch_unwind` - Panic Safety

For non-raw methods, controls whether panics are caught and converted to Java exceptions.
Default is `true`.

- `true` - Use `EnvUnowned::with_env` (catches panics)
- `false` - Use `EnvUnowned::with_env_no_catch` (panics will abort when crossing FFI boundary)

```ignore
catch_unwind = false
```

**Note:** Not applicable to raw methods (which never have panic safety).

## `abi_check` - Runtime ABI Validation

Controls runtime validation that the method is registered correctly as static/instance.

Values:
- `Always` - Always check (default)
- `UnsafeNever` - Never check (unsafe micro-optimization, for production if needed)
- `UnsafeDebugOnly` - Check only in debug builds (unsafe micro-optimization, for production if
  needed)

```ignore
abi_check = Always
```

The check validates that the second parameter (`this` for instance, `class` for static) matches
how Java called the method. This is performed once per method via `std::sync::Once`.

Check failures for non-raw methods will throw an error that will be mapped via the specified
error handling policy. For raw methods, a panic will occur, which will abort at the FFI
boundary.

## `jni` - Override JNI Crate Path

Override the path to the `jni` crate. Must be the first property if provided.

```ignore
jni = ::my_jni_crate
```

# Function Signature Requirements

## Non-raw (Default)

Instance method:
```ignore
fn<'local>(
    env: &mut Env<'local>,
    this: RustType<'local>,  // Or JObject<'local>
    param1: jint,
    param2: JString<'local>,
    ...
) -> Result<ReturnType, E>
where E: Into<jni::errors::Error>
```

Static method:
```ignore
fn<'local>(
    env: &mut Env<'local>,
    class: JClass<'local>,
    param1: jint,
    ...
) -> Result<ReturnType, E>
where E: Into<jni::errors::Error>
```

## Raw

Instance method:
```ignore
fn<'local>(
    env: EnvUnowned<'local>,
    this: RustType<'local>,  // Or JObject<'local>
    param1: jint,
    ...
) -> ReturnType
```

Static method:
```ignore
fn<'local>(
    env: EnvUnowned<'local>,
    class: JClass<'local>,
    param1: jint,
    ...
) -> ReturnType
```

# Complete Examples

## Basic Static Method

```
# use jni::{Env, native_method, objects::JClass, sys::jint};
const METHOD: jni::NativeMethod = native_method! {
    static fn native_compute(value: jint) -> jint,
};

fn native_compute<'local>(
    _env: &mut Env<'local>,
    _class: JClass<'local>,
    value: jint,
) -> Result<jint, jni::errors::Error> {
    Ok(value * 100)
}
```

## Instance Method with Custom Type

```
# use jni::{Env, native_method, sys::jint};
# struct Calculator<'a>(std::marker::PhantomData<&'a ()>);
const METHOD: jni::NativeMethod = native_method! {
    fn Calculator::multiply(a: jint, b: jint) -> jint,
#   abi_check = UnsafeNever, // because Calculator isn't a real Reference type
};

impl Calculator<'_> {
    fn multiply<'local>(
        _env: &mut Env<'local>,
        _this: Calculator<'local>,
        a: jint,
        b: jint,
    ) -> Result<jint, jni::errors::Error> {
        Ok(a * b)
    }
}
```

## Exported Method with Type Mapping

```
# use jni::{Env, native_method, objects::JString, sys::jint};
# struct MyHandle(*const u8);
# impl From<MyHandle> for jni::sys::jlong { fn from(h: MyHandle) -> jni::sys::jlong { h.0 as jni::sys::jlong } }
# struct MyType<'a>(std::marker::PhantomData<&'a ()>);
const METHOD: jni::NativeMethod = native_method! {
    java_type = "com.example.MyClass",
    type_map = {
        unsafe MyHandle => long,
    },
    extern fn MyType::process(handle: MyHandle) -> JString,
#   abi_check = UnsafeNever, // because MyType isn't a real Reference type
};

impl MyType<'_> {
    fn process<'local>(
        env: &mut Env<'local>,
        _this: MyType<'local>,
        handle: MyHandle,
    ) -> Result<JString<'local>, jni::errors::Error> {
        JString::from_str(env, "processed")
    }
}
```

## Raw Method (No Wrapping)

```
# use jni::{EnvUnowned, native_method, objects::JObject, sys::jint};
const METHOD: jni::NativeMethod = native_method! {
    raw fn fast_compute(value: jint) -> jint,
};

fn fast_compute<'local>(
    _env: EnvUnowned<'local>,
    _this: JObject<'local>,
    value: jint,
) -> jint {
    value * 2
}
```

## Array of Methods for Registration

```
# use jni::{Env, EnvUnowned, NativeMethod, native_method};
# use jni::objects::{JClass, JObject, JString};
# use jni::sys::jint;
const METHODS: &[NativeMethod] = &[
    native_method! {
        fn add(a: jint, b: jint) -> jint,
    },
    native_method! {
        fn greet(name: JString) -> JString,
    },
    native_method! {
        static fn get_version() -> jint,
    },
    native_method! {
        raw fn fast_path(value: jint) -> jint,
    },
];

fn add<'local>(
    _env: &mut Env<'local>, _this: JObject<'local>, a: jint, b: jint
) -> Result<jint, jni::errors::Error> { Ok(a + b) }

fn greet<'local>(
    env: &mut Env<'local>, _this: JObject<'local>, name: JString<'local>
) -> Result<JString<'local>, jni::errors::Error> {
    JString::from_str(env, &format!("Hello, {}", name.try_to_string(env)?))
}

fn get_version<'local>(
    _env: &mut Env<'local>, _class: JClass<'local>
) -> Result<jint, jni::errors::Error> { Ok(1) }

fn fast_path<'local>(
    _env: EnvUnowned<'local>, _this: JObject<'local>, value: jint
) -> jint { value }

fn register_native_methods<'local>(
    env: &mut Env<'local>,
    class: JClass<'local>,
) -> Result<(), jni::errors::Error> {
    unsafe { env.register_native_methods(class, METHODS) }
}
```

# Type Safety

The macro ensures compile-time type safety by:
- Generating an `extern "system"` wrapper that has the correct ABI for registration with the
  associated JNI signature
- Type-checking arguments when calling your implementation function
- Rejecting mismatches between the JNI signature and Rust types

**Important:** The macro cannot determine if a method is `static` or instance at compile time.
You must specify `static` correctly to ensure the second parameter type (`JClass` vs `JObject`)
matches. The `abi_check` property (enabled by default) adds runtime validation to catch
registration errors.

# Wrapper Macros

You can create wrapper macros to inject common configuration:

```
# extern crate jni as jni2;
macro_rules! my_native_method {
    ($($tt:tt)*) => {
        ::jni2::native_method! {
            jni = ::jni2,
            type_map = {
                // Common type mappings
            },
            $($tt)*
        }
    };
}
```

# See Also

- [`NativeMethod`] - The struct created by this macro
- [`jni_sig!`] - Signature syntax reference
- [`jni_mangle`] - Lower-level attribute macro for exports

[`NativeMethod`]: https://docs.rs/jni/latest/jni/struct.NativeMethod.html