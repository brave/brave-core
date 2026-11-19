---
name: brave/platform-scopers-for-os-handles
description:
  'Windows HANDLEs and Core Foundation objects are held in
  base::win::ScopedHandle and base::apple::ScopedCFTypeRef<T>, not managed by
  hand.'
rubric: safety
severity: should_fix
---

**Rule.** Hold a Windows `HANDLE` in a `base::win::ScopedHandle` and a Core
Foundation object in a `base::apple::ScopedCFTypeRef<T>`, instead of calling
`::CloseHandle()` or `CFRelease()` by hand. The Core Foundation scoper is
spelled `base::apple::ScopedCFTypeRef`; `base::mac::ScopedCFTypeRef` no longer
exists.

**Why.** The scopers release on destruction like any other smart pointer, so the
handle is released on every path. A hand-managed handle leaks on every early
return.

## Examples

Bad:

```cpp
HANDLE file = ::CreateFile(...);
if (!IsValid(file)) {
  return false;
}
::CloseHandle(file);
```

Good:

```cpp
base::win::ScopedHandle file(::CreateFile(...));
if (!file.IsValid()) {
  return false;
}
```
