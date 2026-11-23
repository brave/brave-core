Export a Rust function with a JNI-compatible, mangled method name.

This adds an appropriate `#[export_name = "..."]` attribute and `extern
"system"` ABI to the function, to allow it to be resolved by a JVM when
calling an associated native method.

This attribute takes one to three string literal arguments:
1. Package namespace (required)
2. Method name (optional)
3. JNI signature (optional)

If two arguments are given, the second is inferred to be a method name if it
doesn't contain '(', otherwise it's treated as a signature.

The name is mangled according to the JNI Specification, under "Design" ->
"Resolving Native Method Names"

<https://docs.oracle.com/en/java/javase/11/docs/specs/jni/design.html#resolving-native-method-names>

## Method Name Generation

If no method name is provided, the Rust function name is converted from
`snake_case` to `lowerCamelCase`.

If the Rust function name is not entirely lowercase with underscores (i.e.
it contains any uppercase letters), the name is used directly without
transformation.

## `snake_case` to `lowerCamelCase` Conversion Rules

If the input contains any uppercase letters, it's returned unchanged to
preserve intentional casing.

Leading underscores are preserved except for one underscore that is removed.

Trailing underscores are preserved.

When capitalizing segments after underscores, the first non-numeric
character is capitalized. This ensures that segments with numeric prefixes
are properly capitalized.

Examples:
- `"say_hello"` -> `"sayHello"`
- `"get_user_name"` -> `"getUserName"`
- `"_private_method"` -> `"privateMethod"` (one leading underscore removed)
- `"__dunder__"` -> `"_dunder__"` (one leading underscore removed)
- `"___priv"` -> `"__priv"` (one leading underscore removed)
- `"trailing_"` -> `"trailing_"`
- `"sayHello"` -> `"sayHello"` (unchanged)
- `"getUserName"` -> `"getUserName"` (unchanged)
- `"Foo_Bar"` -> `"Foo_Bar"` (unchanged - contains uppercase)
- `"XMLParser"` -> `"XMLParser"` (unchanged - contains uppercase)
- `"init"` -> `"init"` (unchanged - no underscores)
- `"test_αλφα"` -> `"testΑλφα"` (Unicode-aware)
- `"array_2d_foo"` -> `"array2DFoo"` (capitalizes first char after digits)
- `"test_3d"` -> `"test3D"` (capitalizes first char after digits)

## ABI Handling

The macro requires the ABI to be `extern "system"` (required for JNI).
- If no ABI is specified, it will automatically be set to `extern "system"`
- If `extern "system"` is already specified, it will be preserved
- If any other ABI (e.g., `extern "C"`) is specified, a compile error will
  be generated

## Examples

Basic usage with just namespace (function name converted to lowerCamelCase):
```
# use jni::{ EnvUnowned, objects::{ JObject, JString } };
# use jni_macros::jni_mangle;

// Rust function in snake_case
#[jni_mangle("com.example.RustBindings")]
pub fn say_hello<'local>(mut env: EnvUnowned<'local>, _: JObject<'local>, name: JString<'local>) -> JString<'local> {
    // ...
#     unimplemented!()
}
// Generates: Java_com_example_RustBindings_sayHello
```

Or already in lowerCamelCase (idempotent):
```
# use jni::{ EnvUnowned, objects::{ JObject, JString } };
# use jni_macros::jni_mangle;
#[allow(non_snake_case)]
#[jni_mangle("com.example.RustBindings")]
pub fn sayHello<'local>(mut env: EnvUnowned<'local>, _: JObject<'local>, name: JString<'local>) -> JString<'local> {
    // ...
#     unimplemented!()
}
// Generates: Java_com_example_RustBindings_sayHello
```

The `sayHello` function will automatically be expanded to have the correct
ABI specification and the appropriate JNI-compatible name, i.e. in this case
- `Java_com_example_RustBindings_sayHello`.

Then it can be accessed by, for example, Kotlin code as follows:
```kotlin
package com.example.RustBindings

class RustBindings {
    private external fun sayHello(name: String): String

    fun greetWorld() {
        println(sayHello("world"))
    }
}
```

With custom method name:
```
# use jni::{ EnvUnowned, objects::JObject };
# use jni_macros::jni_mangle;
#[jni_mangle("com.example.RustBindings", "customMethodName")]
pub fn some_rust_function<'local>(env: EnvUnowned<'local>, _: JObject<'local>) { }
// Generates: Java_com_example_RustBindings_customMethodName
```

With signature only (overloaded method):
```
# use jni::{ EnvUnowned, objects::JObject };
# use jni_macros::jni_mangle;
#[jni_mangle("com.example.RustBindings", "(I)Z")]
pub fn boolean_method<'local>(env: EnvUnowned<'local>, _: JObject<'local>) { }
// Generates: Java_com_example_RustBindings_booleanMethod__I
// Note: Only argument types are encoded (I), return type (Z) is ignored
```

With method name and signature:
```
# use jni::{ EnvUnowned, objects::JObject };
# use jni_macros::jni_mangle;
#[jni_mangle("com.example.RustBindings", "customName", "(Ljava/lang/String;)V")]
pub fn another_function<'local>(env: EnvUnowned<'local>, _: JObject<'local>) { }
// Generates: Java_com_example_RustBindings_customName__Ljava_lang_String_2
// Note: Only argument types are encoded, return type (V) is ignored
```

Pre-existing "system" ABI is preserved:
```
# use jni::{ EnvUnowned, objects::JObject };
# use jni_macros::jni_mangle;
#[jni_mangle("com.example.RustBindings")]
pub extern "system" fn my_function<'local>(env: EnvUnowned<'local>, _: JObject<'local>) { }
// The ABI will be set to "system" but you can also set it explicitly
```