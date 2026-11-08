---
name: brave/no-runtime-feature-flags-in-gn
description:
  'GN deps and sources are gated only by build flags, never by a runtime
  base::Feature; with no build flag for the feature, they are included
  unconditionally.'
rubric: correctness
severity: blocking
---

**Rule.** Guard GN `deps`, `sources` and `#include` blocks only with build flags
set from GN args (`if (enable_local_ai)`, `#if BUILDFLAG(ENABLE_LOCAL_AI)`).
NEVER write a runtime feature flag (`kLocalAIModels`,
`base::FeatureList::IsEnabled(...)`) as a GN condition. When the feature has no
build flag yet, do not invent one: include the deps unconditionally and let the
runtime feature check handle enablement.

**Why.** Runtime feature flags (`base::FeatureList`) and compile-time build
flags (`BUILDFLAG()`) are completely different mechanisms. A `base::Feature`
does not exist in GN, and a value checked at run time cannot decide what the
build compiled.

## Examples

Bad:

```gn
if (kLocalAIModels) {  # Does not exist in GN!
  deps += [ "//brave/components/local_ai/resources" ]
}
```

Good:

```gn
if (enable_local_ai) {
  deps += [ "//brave/components/local_ai/resources" ]
}
```
