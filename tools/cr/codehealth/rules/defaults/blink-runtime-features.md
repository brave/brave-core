---
name: defaults/blink-runtime-features
rubric: defaults
severity: regression
---

**Rule.** Brave's Blink runtime feature pins name features that exist, keep them
off, and go through the json5 plaster where they can.

**Why.** Blink runtime features don't show up in a `BASE_FEATURE` diff. A json5
pin (`set_blink_runtime_enabled_feature_state`) marks its entry
`brave_base_feature_overridden`, so the build checks it: it needs an `X()` in
`base/compile_overridden_features.inc`, and its `RUNTIME_FLAG` in
`runtime_enabled_features_unittest.cc` stops building when upstream removes the
flag. An `EnableFeatureFromString` name is silently ignored when upstream
removes or renames the feature.

**How to check.** `blink_runtime_names.py` lists the `EnableFeatureFromString`
strings whose name the range removed from
`third_party/blink/renderer/platform/runtime_enabled_features.json5`: drop each,
or follow the feature's new name. The build checks the json5 plaster's names.
Diff the `status:` dicts too
(`git diff -U12 $A $B -- <json5> | grep -E '^[-+]\s+(status|"Android")|name:'`).
A string whose entry has a `base::Feature` (no `base_feature: "none"`) should be
a json5 pin instead, and a string for a feature the plaster already pins is
dead.

## Examples

- `AIClassifierAPI` → `AIDecisionModelAPI`.
- M158: `LanguageDetectionAPI` went stable on Android, and `TranslationAPI` is
  exposed on desktop.
- M159, 8135.0: `FledgeMultiBid`, removed by
  [2ae93f9b43b9](https://chromium.googlesource.com/chromium/src/+/2ae93f9b43b9bb8d54e382a9257ef950feaa308c),
  stayed in `brave_content_renderer_client.cc` after its json5 pins were
  dropped.
