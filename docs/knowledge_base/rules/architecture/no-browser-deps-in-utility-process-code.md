---
name: brave/no-browser-deps-in-utility-process-code
description:
  'Utility-process code, under components/services/, must never depend on
  browser-process code; code both processes use lives in a common/ directory.'
rubric: design
severity: blocking
---

**Rule.** Code under `components/services/` (utility process service
implementations) MUST NOT include headers from, or depend on targets in, any
`browser/` directory (`components/.../browser/`, `content/public/browser/`).
When it needs code the browser process also uses, move that code from `browser/`
to the component's `common/` directory and depend on that.

**Why.** The utility process is a separate, sandboxed process; browser-process
code assumes browser-only state and APIs, and depending on it breaks the
multi-process boundary.

## Examples

Bad:

```gn
# In components/services/brave_shields/BUILD.gn
deps = [ "//brave/components/brave_shields/core/browser/adblock" ]
```

Good:

```gn
# In components/services/brave_shields/BUILD.gn
deps = [ "//brave/components/brave_shields/core/common/adblock" ]
```
