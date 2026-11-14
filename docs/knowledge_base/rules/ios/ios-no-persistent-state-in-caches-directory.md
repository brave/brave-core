---
name: brave/ios-no-persistent-state-in-caches-directory
description:
  'State that must survive app restarts is never stored in the iOS caches
  directory, which the system can clear at any time.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER write data that must persist (settings, downloaded content the
user expects to keep, databases, migration markers) under the caches directory
(`FileManager.default.urls(for: .cachesDirectory, ...)`, `URL.cachesDirectory`,
`NSCachesDirectory`). Store it under Application Support or Documents, and keep
the caches directory for data that can be recreated.

**Why.** iOS deletes the caches directory's contents whenever it needs space,
without telling the app, so state kept there disappears between launches.

## Examples

Bad:

```swift
let url = URL.cachesDirectory.appending(path: "playlist.sqlite")
```

Good:

```swift
let url = URL.applicationSupportDirectory.appending(path: "playlist.sqlite")
```
