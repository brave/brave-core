---
name: brave/swift-logger-not-print
description:
  'Debug print statements are removed before submitting, and logging that stays
  uses Logger instead of print.'
rubric: readability
severity: should_fix
---

**Rule.** Do not submit `print`, `debugPrint`, `dump` or `NSLog` calls. Remove
those added for debugging, and log what should remain with a `Logger`
(`Logger.module.debug(...)`, or the module's own logger).

**Why.** `print` output is unstructured and unfiltered, has no level or privacy
redaction, and debugging leftovers add noise to the console of every build.

## Examples

Bad:

```swift
print("playlist items: \(items)")
```

Good:

```swift
Logger.module.debug("Loaded \(items.count) playlist items")
```
