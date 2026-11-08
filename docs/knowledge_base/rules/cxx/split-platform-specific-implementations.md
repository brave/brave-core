---
name: brave/split-platform-specific-implementations
description:
  'A method whose implementation is entirely different on one platform gets a
  separate platform file such as my_class_android.cc, not #if BUILDFLAG(IS_*)
  blocks in the main file.'
rubric: readability
severity: should_fix
---

**Rule.** When a method's body is completely different on one platform, put that
platform's implementation in its own file named with the platform suffix
(`my_class_android.cc`, `my_class_win.cc`, `my_class_mac.mm`), selected in
`BUILD.gn`, instead of wrapping whole bodies in `#if BUILDFLAG(IS_*)` blocks in
the shared file. Small platform differences inside otherwise shared logic stay
inline.

**Why.** Files full of platform blocks are hard to read, and each platform's
code is interleaved with code it never compiles. A separate file shows each
implementation whole.

## Examples

Bad:

```cpp
// my_class.cc
void MyClass::Show() {
#if BUILDFLAG(IS_ANDROID)
  // 40 lines of Android-only code.
#else
  // 40 lines of desktop code.
#endif
}
```

Good:

```cpp
// my_class_android.cc
void MyClass::Show() {
  // Android implementation.
}

// my_class_desktop.cc
void MyClass::Show() {
  // Desktop implementation.
}
```
