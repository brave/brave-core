---
name: brave/swift-format-styles-for-durations
description:
  'Durations are displayed with TimestampFormatStyle or Duration.formatted(),
  not formatted by hand.'
rubric: readability
severity: nit
---

**Rule.** Format a duration for display with `Duration.formatted(...)` or a
`Duration.TimeFormatStyle`/`TimestampFormatStyle`. Do not compute minutes and
seconds by hand and build the string with `String(format: "%02d:%02d", ...)`.

**Why.** The format styles handle hours, rounding and the user's locale; manual
formatting gets those wrong and repeats the arithmetic at each use.

## Examples

Bad:

```swift
let minutes = Int(seconds) / 60
let label = String(format: "%d:%02d", minutes, Int(seconds) % 60)
```

Good:

```swift
let label = Duration.seconds(seconds).formatted(.time(pattern: .minuteSecond))
```
