---
name: brave/no-raw-ptr-for-plugin-exempt-pointers
description:
  'Function pointers, ObjC pointers and const char*/const wchar_t* fields
  pointing to string literals need neither raw_ptr nor an exclusion annotation.'
rubric: safety
severity: should_fix
---

**Rule.** NEVER ask for `raw_ptr<T>` or a `RAW_PTR_EXCLUSION` annotation on a
field whose type the Clang plugin exempts: a function pointer, an Objective-C
object pointer, or a `const char*` / `const wchar_t*` that points to a string
literal. When such a `const char*` field may point to a heap-allocated string,
it should be a `raw_ptr<const char>`.

**Why.** The plugin does not require `raw_ptr` for these types, and string
literals live in static memory that cannot dangle. A field that may point into
the heap has the same use-after-free risk as any other pointer.

## Examples

Fine as is, do not flag:

```cpp
struct FeatureEntry {
  const char* name;  // always a string literal
  void (*handler)(int);
};
```

Bad:

```cpp
class Parser {
  const char* current_token_;  // points into a heap-allocated std::string
};
```

Good:

```cpp
class Parser {
  raw_ptr<const char> current_token_;
};
```
