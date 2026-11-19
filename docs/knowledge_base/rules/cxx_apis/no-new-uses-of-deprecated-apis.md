---
name: brave/no-new-uses-of-deprecated-apis
description:
  'Never introduce a new use of an API whose declaration in the current source
  tree carries a deprecation notice; flag it only after reading that notice in
  the header.'
rubric: design
severity: blocking
---

**Rule.** A change MUST NOT add a call to, or a use of, a function, class or
constant that is marked deprecated in its declaration (a `[[deprecated]]`
attribute, a `// DEPRECATED`/"Deprecated:" comment, or a `TODO` to replace its
callers and remove it). Use the replacement the notice names.

Before flagging, the reviewer MUST open the header that declares the API in the
current source tree and confirm that the deprecation notice is there. NEVER flag
an API as deprecated from memory or training data: APIs change across Chromium
upgrades, and what was once deprecated may be fine now, or the reverse.

**Why.** Every new use of a deprecated API is one more caller to migrate before
the API can be deleted, and it may break on the next Chromium rebase when the
API disappears.

## Examples

Bad:

```cpp
uint32_t hash = base::Hash(str);
```

Good:

```cpp
size_t hash = base::FastHash(base::as_byte_span(str));
```
