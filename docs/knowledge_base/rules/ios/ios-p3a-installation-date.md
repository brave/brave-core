---
name: brave/ios-p3a-installation-date
description:
  'Swift code that needs the installation date reads
  Preferences.P3A.installationDate, never Preferences.DAU.installationDate.'
rubric: correctness
severity: should_fix
---

**Rule.** Read the app's installation date from
`Preferences.P3A.installationDate`. Do not use
`Preferences.DAU.installationDate` for it.

**Why.** `Preferences.DAU.installationDate` is reset after 14 days, so code
reading it as the installation date gets `nil` or a wrong date afterwards.

## Examples

Bad:

```swift
guard let installDate = Preferences.DAU.installationDate.value else { return }
```

Good:

```swift
guard let installDate = Preferences.P3A.installationDate.value else { return }
```
