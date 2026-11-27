Converts UTF-8 string literals to a MUTF-8 encoded `&'static JNIStr`.

This macro takes one or more literals and encodes them using Java's Modified UTF-8
(MUTF-8) format, returning a `&'static JNIStr`.

Like the `concat!` macro, multiple literals can be provided and will be converted to
strings and concatenated before encoding.

Supported literal types:
- String literals (`"..."`)
- Character literals (`'c'`)
- Integer literals (`42`, `-10`)
- Float literals (`3.14`, `1.0`)
- Boolean literals (`true`, `false`)
- Byte literals (`b'A'` - formatted as numeric value)
- C-string literals (`c"..."` - must be valid UTF-8)

MUTF-8 is Java's variant of UTF-8 that:
- Encodes the null character (U+0000) as `0xC0 0x80` instead of `0x00`
- Encodes Unicode characters above U+FFFF using CESU-8 (surrogate pairs)

This is the most type-safe way to create JNI string literals, as it returns a
`JNIStr` which is directly compatible with the jni crate's API.

# Syntax

```
# use jni::jni_str;
# extern crate jni as jni2;
jni_str!("string literal");
jni_str!("part1", "part2", "part3");  // Concatenates before encoding
jni_str!("value: ", 42);               // Mix different literal types
jni_str!(jni = jni2, "string literal");  // Override jni crate path (must be first)
```

# Examples

```
use jni::{jni_str, strings::JNIStr};

const CLASS_NAME: &JNIStr = jni_str!("java.lang.String");
// Result: &'static JNIStr for "java.lang.String" (MUTF-8 encoded)

const EMOJI_CLASS: &JNIStr = jni_str!("unicode.Type😀");
// Result: &'static JNIStr with emoji encoded as surrogate pair

const PACKAGE_CLASS: &JNIStr = jni_str!("java.lang.", "String");
// Result: &'static JNIStr for "java.lang.String" (concatenated then MUTF-8 encoded)

const PORT: &JNIStr = jni_str!("localhost:", 8080);
// Result: &'static JNIStr for "localhost:8080" (mixed literal types concatenated)
```