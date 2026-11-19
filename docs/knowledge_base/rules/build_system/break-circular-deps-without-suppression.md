---
name: brave/break-circular-deps-without-suppression
description:
  'Circular dependencies are broken with interface/impl, dependency inversion or
  templated subclasses; allow_circular_includes_from and sources.gni are a last
  resort, and check_includes = false is never used.'
rubric: design
severity: blocking
---

**Rule.** Break a circular dependency with the interface/impl pattern (public
headers in the direct dependency, the implementation pulled into a higher-level
target such as `//brave/browser`), dependency inversion, or the
templated-subclass technique. Only fall back to
`brave_chrome_browser_ui_allow_circular_includes_from` or `sources.gni` as a
temporary last resort. NEVER set `check_includes = false` to suppress circular
include errors.

**Why.** Suppressing the error keeps the cycle and turns off the include
checking that would catch the next one; the fallbacks keep the cycle too, which
is why they are only temporary.

## Examples

Bad:

```gn
source_set("history") {
  check_includes = false
  sources = [ "brave_history_embeddings_page_handler.cc" ]
}
```

Good:

```gn
# brave/browser/ui/webui/history/BUILD.gn: the interface, with no cycle.
source_set("history") {
  sources = [ "history_embeddings_delegate.h" ]
}

# brave/browser/BUILD.gn: the implementation, in a higher-level target.
source_set("browser_process") {
  sources = [ "history_embeddings_delegate_impl.cc" ]
  deps = [ "//brave/browser/ui/webui/history" ]
}
```
