---
name: brave/list-public-headers-in-public
description:
  'A GN target that exposes only some of its headers lists them in public =
  [...], keeping the rest internal.'
rubric: design
severity: should_fix
---

**Rule.** When a target's consumers should include only some of its headers,
list those headers in `public = [ ... ]`. Without a `public` list every header
in `sources` is public. This applies especially to `static_library` and
`source_set` targets holding both a public API and private implementation files.

**Why.** GN enforces that dependents only `#include` the headers listed in
`public`, so internal helpers cannot be coupled to by accident and the target's
API is what it declares.

## Examples

Bad:

```gn
source_set("core") {
  sources = [
    "internal_helper.cc",
    "internal_helper.h",
    "local_ai_service.cc",
    "local_ai_service.h",
  ]
}
```

Good:

```gn
source_set("core") {
  public = [ "local_ai_service.h" ]
  sources = [
    "internal_helper.cc",
    "internal_helper.h",
    "local_ai_service.cc",
    "local_ai_service.h",
  ]
}
```
