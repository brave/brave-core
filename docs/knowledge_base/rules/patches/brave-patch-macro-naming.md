---
name: brave/brave-patch-macro-naming
description:
  A hook macro a patch inserts into upstream code is named BRAVE_ followed by
  the upstream original method name in upper snake case.
rubric: readability
severity: nit
---

**Rule.** Name a hook macro that a patch inserts, and that a `chromium_src/`
override defines, `BRAVE_` followed by the name of the upstream method it sits
in, in upper snake case (`BRAVE_ALL_CAPS_ORIGINAL_METHOD_NAME`), qualified by
its class where that is needed to tell it apart. NEVER use generic names such as
`BRAVE_HOOK` or `BRAVE_EXTRA_CODE`.

**Why.** The name says where in upstream the hook is, so a reader of the
override can find the code it changes, and a reader of the patch can find the
override.

## Examples

Bad:

```cpp
#define BRAVE_EXTRA_CODE \
  if (ShouldUseBraveVerticalTabs()) { ... }
```

Good:

```cpp
#define BRAVE_TAB_DRAG_CONTEXT_IMPL_CALCULATE_INSERTION_INDEX \
  if (ShouldUseBraveVerticalTabs()) { ... }
```
