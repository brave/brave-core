---
name: brave/raw-ptr-or-raw-ref-for-pointer-fields
description:
  'Non-owning class and struct fields are raw_ptr<T> or const raw_ref<T>, not T*
  or T&, whenever possible.'
rubric: safety
severity: should_fix
---

**Rule.** Write non-owning class and struct fields as `raw_ptr<T>` (the default,
for fields that can be null or reassigned) or `const raw_ref<T>` (for fields
that must never be null), not as `T*` or `T&`. This applies to fields only:
parameters, locals and return types stay `T*` / `T&`. The exceptions are pointer
types the Clang plugin exempts (`brave/no-raw-ptr-for-plugin-exempt-pointers`),
the cases that allow `RAW_PTR_EXCLUSION`
(`brave/raw-ptr-exclusion-only-for-listed-cases`) and memory outside
PartitionAlloc (`brave/no-raw-ptr-outside-partition-alloc`).

**Why.** Both types detect a use-after-free of the pointee instead of silently
operating on freed memory, and they document the lifetime contract: whenever the
field holds a value, the memory it points to is expected to be alive.

## Examples

Bad:

```cpp
class TabFeatures {
  content::WebContents* web_contents_;
  BrowserWindowInterface& window_;
};
```

Good:

```cpp
class TabFeatures {
  raw_ptr<content::WebContents> web_contents_;
  const raw_ref<BrowserWindowInterface> window_;
};
```
