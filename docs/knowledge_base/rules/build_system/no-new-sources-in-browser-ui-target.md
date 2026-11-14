---
name: brave/no-new-sources-in-browser-ui-target
description:
  'New source files are not added to source_set("ui") in
  brave/browser/ui/BUILD.gn; they go in a more specific, lower-level target.'
rubric: design
severity: should_fix
---

**Rule.** Do not add files to the `sources` of `source_set("ui")` in
`brave/browser/ui/BUILD.gn`. Put new files in a more specific, lower-level
target instead, such as a feature target in the `BUILD.gn` of their own
directory (`brave/browser/ui/webui/history/BUILD.gn`).

**Why.** `//chrome/browser/ui` is prone to circular dependencies, and every file
added to Brave's `ui` target adds to that problem; a small target with its own
deps keeps the files out of the cycle.

## Examples

Bad:

```gn
# brave/browser/ui/BUILD.gn
source_set("ui") {
  sources = [
    ...
    "webui/history/brave_history_embeddings_page_handler.cc",
    "webui/history/brave_history_embeddings_page_handler.h",
  ]
}
```

Good:

```gn
# brave/browser/ui/webui/history/BUILD.gn
source_set("history") {
  sources = [
    "brave_history_embeddings_page_handler.cc",
    "brave_history_embeddings_page_handler.h",
  ]
  deps = [ ... ]
}
```
