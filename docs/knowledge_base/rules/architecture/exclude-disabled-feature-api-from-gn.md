---
name: brave/exclude-disabled-feature-api-from-gn
description:
  'When a buildflag disables a feature, its whole API target is left out of the
  build, not built with empty stub implementations.'
rubric: design
severity: should_fix
---

**Rule.** When a feature can be disabled with a buildflag, define its API
targets inside `if (<feature>_enabled) { ... }` in GN, so that they are not
built at all when it is off. Do not build the API unconditionally with its
implementation reduced to empty stubs under `#if !BUILDFLAG(...)`.

**Why.** Stub implementations keep code that does nothing compiling and
callable, which hides callers that should not exist in that configuration;
leaving the target out makes such callers fail to build.

## Examples

Bad:

```gn
source_set("wallet_api") {
  sources = [ "wallet_api.cc" ]  # has empty stubs when disabled
}
```

Good:

```gn
if (brave_wallet_enabled) {
  source_set("wallet_api") {
    sources = [ "wallet_api.cc" ]
  }
}
```
