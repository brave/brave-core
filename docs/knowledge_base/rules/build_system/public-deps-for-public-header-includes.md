---
name: brave/public-deps-for-public-header-includes
description:
  "A dependency whose headers a target's public headers include must be in
  public_deps; dependencies used only by .cc files or internal headers stay in
  deps."
rubric: correctness
severity: blocking
---

**Rule.** When a public header of a GN target includes a header of another
target, that target MUST be in `public_deps`, not `deps`. A dependency used only
from `.cc` files, or from headers kept out of the target's `public` list, stays
in `deps`. When a target has no `public` list, every header in its `sources` is
public.

**Why.** `public_deps` gives the target's consumers the transitive include paths
they need to compile its public headers; in `deps` they break or rely on some
other path to the dependency. Putting implementation-only dependencies in
`public_deps` leaks them into every consumer.

## Examples

Bad:

```gn
source_set("my_service") {
  sources = [ "my_service.h", "my_service.cc" ]
  deps = [ "//components/prefs" ]  # prefs is used in my_service.h!
}
```

Good:

```gn
source_set("my_service") {
  sources = [ "my_service.h", "my_service.cc" ]
  public_deps = [ "//components/prefs" ]  # used in header
  deps = [ "//base" ]  # only used in .cc
}
```
