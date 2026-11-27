Parses a JNI method or field signature at compile time.

This macro parses method and field signatures with syntax like `(arg0: JString, arg1: jint) ->
JString` and generates a [MethodSignature] or [FieldSignature] struct to represent the corresponding
JNI signature. These structs embed the raw signature paired with type information which can be used
for JNI method calls and field accesses without needing to parse the signature at runtime.

This macro can also parse raw JNI signature strings like `"(Ljava/lang/String;I)Z"` in order to
validate them at compile time but it's recommended to use the structured syntax for better
readability.

**Note:** The signature and `type_map` syntax supported by this macro is also used by the
[`bind_java_type`] and [`native_method`] macros.

[MethodSignature]: https://docs.rs/jni/latest/jni/signature/struct.MethodSignature.html
[FieldSignature]: https://docs.rs/jni/latest/jni/signature/struct.FieldSignature.html

# Syntax

The macro accepts named properties separated by commas:
```ignore
jni_sig!(
    [jni = <path>],
    [type_map = { ... }],
    [sig =] <signature>,
)
```
The parser automatically detects whether it's a method signature (has parentheses) or a field
signature (a single, bare type).

## Properties

- `jni = <path>` - Optionally override the jni crate path (default: auto-detected via
  `proc_macro_crate`, must come first if given)
- `type_map = { RustType => java.lang.ClassName, ... }` - Optional type mappings for Rust types
- `sig = <signature>` - The signature ('`sig =`' prefix is optional for the signature)

The `type_map` property can be provided multiple times and mappings are merged.

The design allows for a `macro_rules` wrapper to inject `jni =` or `type_map =` properties,
without needing to parse anything else.

# Type Syntax

Note: this syntax for signature types is also used by the [`bind_java_type`] and
[`native_method`] macros.

## Primitive Types
- Java primitives: `jboolean`, `jbyte`, `jchar`, `jshort`, `jint`, `jlong`, `jfloat`, `jdouble`
- Aliases: `boolean`/`bool`, `byte`/`i8`, `char`, `short`/`i16`, `int`/`i32`, `long`/`i64`,
  `float`/`f32`, `double`/`f64`
- Void: `void` or `()` or elided return type defaults to `void`

## Java Object Types
- Fully qualified: `java.lang.String`, `java.util.List` or as string literal: `"java.util.List"`
- With inner classes: `java.lang.Outer::Inner` or as string literal: `"java.lang.Outer$Inner"`
- Default package: `.ClassName` or as string literal: `".ClassName"`

_(Notice that Java object types _always_ contain at least one `.` dot)_

## Rust Reference Types
- Single identifier or path: `JString`, `JObject`, `jni::objects::JString`, `RustType`,
  `custom::RustType`

## Array Types
- Prefix syntax: `[jint]`, `[[java.lang.String]]`, `[RustType]`
- Suffix syntax: `jint[]`, `java.lang.String[][]`, `RustType[]`

### Built-in Types
- Types like `JObject`, `JClass`, `JString` etc from the `jni` crate can be used without a
  `type_map`
- Built-in types can also be referenced like `jni::objects::JString`
- Java types like `java.lang.Class` are automatically mapped to built-in types like `JClass`

### Core Types
- The core types `java.lang.Object`, `java.lang.Class`, `java.lang.String` and
  `java.lang.Throwable` can not be mapped to custom types.
- Other built-in types, such as `JList` (`java.util.List`) can be overridden by mapping them to
  a different type via a `type_map`

## Type Mappings via `type_map` Block

A `type_map` block:
- Maps Rust [Reference] type names to Java class names for use in method/field signatures.
- Maps Java class names to Rust types (primarily for use with the [`bind_java_type`] and
  [`native_method`] macros)
- Allows the definition of type aliases for more ergonomic / readable signatures.

Multiple `type_map` blocks will be merged, so that wrapper macros may forward-declare common
type mappings to avoid repetition.

A `type_map` supports three types of mappings:

### Reference Type Mappings

Map Rust [Reference] types to Java classes like `RustType => java.type.Name`:

```ignore
type_map = {
    CustomType => com.example.CustomClass,
    AnotherType => "com.example.AnotherClass",
    InnerType => com.example.Outer::Inner,
    AnotherInnerType => "com.example.Outer$AnotherInner",
    my_crate::MyType => com.example.MyType,
}
```

The right-side Java type uses the syntax for Java Object Types described above.

### Unsafe Primitive Type Mappings

Map Rust types to Java primitive types using the `unsafe` keyword. This is particularly useful
for Rust types that transparently wrap a pointer (e.g., handles) that need to be passed to Java
as a `long`:

```ignore
type_map = {
    unsafe MyHandle => long,
    unsafe MyBoxedPointer => long,
    unsafe MyRawFd => int,
}
```

These mappings are marked `unsafe` because macros like [`bind_java_type`] and [`native_method`]
cannot verify type safety between the Rust type and Java primitive type - apart from checking
the size and alignment.

### Type Aliases

Creates aliases for existing type mappings using the `typealias` keyword. This can improve
readability in signatures before defining full type bindings:

```ignore
type_map = {
    MyType => com.example.MyType,
    typealias MyAlias => MyType,
    typealias MyObjectAlias => JObject,
}
```

Note: Aliases for array types are not supported.

# Method Signature Syntax

A method can be given in one of these forms:
- `( [args...] ) -> TYPE`
- `( [args...] )`
- `"RAW_JNI_SIG"`

An argument can be given in these forms:
- `name: TYPE`
- `TYPE`

_(with a `TYPE` as described in the `Type Syntax` section above)_

A `TYPE` may have an optional `&` prefix that is ignored

```
# use jni::{jni_sig, signature::{MethodSignature, JavaType, Primitive}};
const JNI_SIG: MethodSignature =
    jni_sig!((arg1: com.example.Type, arg2: JString, arg3: jint) -> JString);
# fn main() {
assert!(JNI_SIG.sig().to_bytes() == b"(Lcom/example/Type;Ljava/lang/String;I)Ljava/lang/String;");
assert!(JNI_SIG.args().len() == 3);
assert!(JNI_SIG.args()[0] == JavaType::Object);
assert!(JNI_SIG.args()[1] == JavaType::Object);
assert!(JNI_SIG.args()[2] == JavaType::Primitive(Primitive::Int));
assert!(JNI_SIG.ret() == JavaType::Object);
# }
```

Traditional JNI signature syntax is also supported:
```ignore
jni_sig!("(IILjava/lang/String;)V")
```

Explicitly named 'sig' property:
```ignore
jni_sig!(sig = (arg1: Type1, arg2: Type2, ...) -> ReturnType)
```

With type mappings:
```
# use jni::{jni_sig, signature::{MethodSignature, JavaType, Primitive}};
const JNI_SIG: MethodSignature = jni_sig!(
    type_map = {
        CustomType => java.class.Type,
        ReturnType => java.class.ReturnType,
    },
    (arg1: CustomType, arg2: JString, arg3: jint) -> ReturnType,
);
```

# Field Signature Syntax

```ignore
jni_sig!(Type)
```

Traditional JNI signature syntax is also supported:
```ignore
jni_sig!("Ljava/lang/String;")
```

Named:
```ignore
jni_sig!(sig = Type)
```

With type mappings:
```ignore
jni_sig!(
    Type,
    type_map = {
        RustType as java.class.Name,
        ...
    }
)
```

# Examples

## Method Signatures

Basic primitive types:
```ignore
const SIG: MethodSignature = jni_sig!((a: jint, b: jboolean) -> void);
// Result: MethodSignature for "(IZ)V"
```

Java object types:
```ignore
const SIG: MethodSignature = jni_sig!(
    (a: jint, b: java.lang.String) -> java.lang.Object
);
// Result: MethodSignature for "(ILjava/lang/String;)Ljava/lang/Object;"
```

Array types:
```ignore
const SIG: MethodSignature = jni_sig!(
    (a: [jint], b: [java.lang.String]) -> [[jint]]
);
// Result: MethodSignature for "([I[Ljava/lang/String;)[[I"
```

With type mappings:
```ignore
const SIG: MethodSignature = jni_sig!(
    type_map = {
        MyString as java.lang.String,
        MyObject as java.lang.Object,
        MyThrowable as java.lang.Throwable,
    },
    (a: jint, b: MyString, c: [MyObject]) -> MyThrowable,
);
// Result: MethodSignature for "(ILjava/lang/String;[Ljava/lang/Object;)Ljava/lang/Throwable;"
```
Multiple type_maps:
```ignore
const SIG: MethodSignature = jni_sig!(
    jni = ::my_jni,
    type_map = { MyType0 => custom.Type0 },
    type_map = { MyType1 => custom.Type1 },
    sig = (arg0: MyType0, arg1: MyType1) -> JString,
);
```

This makes it possible to write wrapper macros to inject a `type_map` without blocking the use
of `type_map` for additional types.

With named signature property:
```ignore
const SIG: MethodSignature = jni_sig!(
    type_map = { MyType => java.lang.MyType },
    sig = (a: jint) -> void,
);
```

With custom jni crate path:
```ignore
const SIG: MethodSignature = jni_sig!(
    jni = ::my_jni, // must come first!
    (a: jint) -> void,
);
```

## Field Signatures

Primitive field:
```ignore
const SIG: FieldSignature = jni_sig!(jint);
// Result: FieldSignature for "I"
```

Object field:
```ignore
const SIG: FieldSignature = jni_sig!(java.lang.String);
// Result: FieldSignature for "Ljava/lang/String;"
```

Array field:
```ignore
const SIG: FieldSignature = jni_sig!([jint]);
// Result: FieldSignature for "[I"
```

Field with type mapping:
```ignore
const SIG: FieldSignature = jni_sig!(
    type_map = {
        MyType as custom.Type,
    },
    MyType
);
// Result: FieldSignature for "Lcustom/Type;"
```
[Reference]: https://docs.rs/jni/latest/jni/refs/trait.Reference.html