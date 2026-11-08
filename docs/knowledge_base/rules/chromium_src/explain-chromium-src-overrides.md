---
name: brave/explain-chromium-src-overrides
description:
  'A chromium_src override carries comments saying why it is needed, per
  function where that suffices, but no doc comments are requested inside #define
  macro bodies.'
rubric: readability
severity: should_fix
---

**Rule.** A new `chromium_src/` override, or a new part of one, MUST say in a
comment why the override is needed, wherever that is not evident from the code:
what upstream lacks, and what Brave changes. Comments on the overriding
functions are enough; NEVER also ask for a file-level comment when they explain
the purpose. NEVER ask for doc comments on the method declarations a `#define`
injects into an upstream class: any documentation belongs in the header that
defines the macro, outside the macro body, or in the implementation file.

**Why.** An override is read out of the context of the upstream file it changes,
often during a rebase that broke it, and its purpose is not always evident from
its code. Comments cannot practically be placed inside a macro body.

## Examples

Bad:

```cpp
// chromium_src/chrome/browser/ui/views/tabs/tab_view.cc
void TabView::BuildContextMenu() {
  ...
}
```

Good:

```cpp
// chromium_src/chrome/browser/ui/views/tabs/tab_view.cc

// Add Brave-specific tab context menu items. The upstream class doesn't
// support extensibility here, so we replace the menu construction logic.
void TabView::BuildContextMenu() {
  ...
}
```
