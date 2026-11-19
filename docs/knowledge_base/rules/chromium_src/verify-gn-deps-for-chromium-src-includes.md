---
name: brave/verify-gn-deps-for-chromium-src-includes
description:
  A header newly included by a chromium_src override must be one the upstream
  target compiling it has a GN dependency on, verified with gn check on the
  upstream file.
rubric: correctness
severity: should_fix
---

**Rule.** When a `chromium_src/` override gains an `#include`, check that the
upstream target the overridden file belongs to depends on the target providing
that header. Verify it by adding the same include to the upstream file
temporarily and running `gn check`; when that fails, the dependency is missing,
and the include needs either the dependency or another approach, such as the
forward-declared hook of [brave/no-brave-deps-in-chromium-src].

**Why.** The override is compiled as part of the upstream target, which may not
depend on the header's target, and `gn check` does not check `chromium_src/`
files, so the missing dependency goes unnoticed until it breaks a build
configuration or a link.

## Examples

Bad:

```cpp
// chromium_src/chrome/browser/ui/foo/foo_view.cc
// //chrome/browser/ui/foo has no dependency on //components/bar.
#include "components/bar/bar_service.h"
```

Good:

```bash
# Add the same #include to chrome/browser/ui/foo/foo_view.cc, then:
gn check out/Default
# A failure means the upstream target is missing the dependency.
```
