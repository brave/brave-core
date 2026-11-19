---
name: brave/comment-non-obvious-nullptr-assignments
description:
  A nullptr assignment in a chromium_src override whose purpose is not obvious,
  such as preventing a dangling pointer, carries a comment explaining it.
rubric: readability
severity: nit
---

**Rule.** When a `chromium_src/` override sets a pointer to `nullptr` and the
reason is not evident from the code next to it, the assignment MUST have a
comment saying why, in particular when it prevents a dangling pointer (for
example, a reset after the object it points to is shut down).

**Why.** Code in an override is read out of the context of the upstream file it
changes, so the lifetime reasoning behind a reset is rarely visible where it
happens.

## Examples

Bad:

```cpp
provider_ = nullptr;
```

Good:

```cpp
// Reset to prevent dangling pointer after BrowserContext shutdown.
provider_ = nullptr;
```
