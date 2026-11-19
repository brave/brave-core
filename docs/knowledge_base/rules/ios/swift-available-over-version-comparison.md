---
name: brave/swift-available-over-version-comparison
description:
  'Swift code checks API availability with #available, never by comparing the OS
  version by hand.'
rubric: correctness
severity: should_fix
---

**Rule.** Guard an API that needs a newer iOS with `if #available(iOS N, *)` (or
`guard #available`). Do not compare
`ProcessInfo.processInfo.operatingSystemVersion`,
`UIDevice.current.systemVersion` or similar values by hand to decide whether to
call it.

**Why.** The compiler enforces `#available`: it lets the guarded code call the
newer API, and warns when a raised minimum deployment target makes the check
dead. A manual comparison is invisible to the compiler, so it neither unlocks
the API nor flags the check once it is obsolete.

## Examples

Bad:

```swift
if ProcessInfo.processInfo.operatingSystemVersion.majorVersion >= 17 {
  useNewAPI()
}
```

Good:

```swift
if #available(iOS 17, *) {
  useNewAPI()
}
```
