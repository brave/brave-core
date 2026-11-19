---
name: brave/ios-mojom-wrappers-in-ios-common
description:
  'Objective-C mojom wrapper targets are defined in an ios/common directory, not
  under ios/browser.'
rubric: design
severity: should_fix
---

**Rule.** Define a new `ios_objc_mojom_wrappers` target in an `ios/common/`
directory (`components/ai_chat/ios/common/BUILD.gn`), not in an `ios/browser/`
`BUILD.gn`.

**Why.** Mojom wrapper types are shared between the browser and renderer sides;
defining them under `ios/browser/` makes any non-browser user depend on the
browser layer.

## Examples

Bad:

```gn
# components/ai_chat/ios/browser/BUILD.gn
ios_objc_mojom_wrappers("mojom") {
  mojom_target = "//brave/components/ai_chat/core/common/mojom"
}
```

Good:

```gn
# components/ai_chat/ios/common/BUILD.gn
ios_objc_mojom_wrappers("mojom") {
  mojom_target = "//brave/components/ai_chat/core/common/mojom"
}
```
