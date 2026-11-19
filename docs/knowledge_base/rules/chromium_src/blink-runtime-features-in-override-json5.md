---
name: brave/blink-runtime-features-in-override-json5
description:
  Brave's Blink runtime-enabled features are added to
  runtime_enabled_features.override.json5, never by patching
  runtime_enabled_features.json5.
rubric: design
severity: should_fix
---

**Rule.** NEVER add an entry for a Brave-specific Blink runtime-enabled feature
to `third_party/blink/renderer/platform/runtime_enabled_features.json5`, whether
by a patch or by a plaster substitution. Add it to the `data` list of
`runtime_enabled_features.override.json5` in the same directory.

**Why.** The override file exists for downstream additions and upstream never
changes it, so entries there do not conflict. `runtime_enabled_features.json5`
changes upstream constantly, and a patch to it conflicts on rebases.

## Examples

Bad:

```diff
// patches/third_party-blink-renderer-platform-runtime_enabled_features.json5.patch
+    {
+      name: "BraveGlobalPrivacyControl",
+      public: true,
+      base_feature: "none",
+    },
```

Good:

```json5
// third_party/blink/renderer/platform/runtime_enabled_features.override.json5
{
  name: "BraveGlobalPrivacyControl",
  public: true,
  base_feature: "none",
},
```
