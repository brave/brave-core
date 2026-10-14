---
name: defaults/blink-runtime-features
rubric: defaults
severity: regression
---

**Rule.** Brave's Blink runtime feature pins name features that exist, and keep
them off.

**Why.** Blink runtime features don't show up in a `BASE_FEATURE` diff, and an
unknown name in `EnableFeatureFromString` is silently ignored. With
`copied_from_base_feature_if: "overridden"`, a disabled `base_feature_status`
that nothing overrides leaves a `stable` feature on.

**How to check.** Compare the `name:` lists of
`third_party/blink/renderer/platform/runtime_enabled_features.json5` at both
tags against Brave's `EnableFeatureFromString` strings and the json5 plaster.
Diff the `status:` dicts too
(`git diff -U12 $A $B -- <json5> | grep -E '^[-+]\s+(status|"Android")|name:'`),
and check each json5 pin's `copied_from_base_feature_if`.

## Examples

- `AIClassifierAPI` → `AIDecisionModelAPI`.
- M158: `LanguageDetectionAPI` went stable on Android, and `TranslationAPI` is
  exposed on desktop.
