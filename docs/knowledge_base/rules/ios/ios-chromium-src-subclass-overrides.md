---
name: brave/ios-chromium-src-subclass-overrides
description:
  'chromium_src overrides of iOS Chromium classes substitute a Brave subclass
  with #define/#include/#undef instead of duplicating code.'
rubric: design
severity: should_fix
---

**Rule.** To change the behaviour of an iOS Chromium class from `chromium_src`,
write a Brave subclass and substitute it with `#define Chrome... Brave...`,
`#include <ios/...>` of the upstream file, and `#undef`. Do not copy the
upstream implementation into the override and edit the copy.

**Why.** A subclass holds only Brave's changes and keeps inheriting upstream's
code, so rebases bring upstream fixes along; a copy diverges silently from
upstream.

## Examples

Bad:

```cpp
// chromium_src/ios/chrome/browser/some_file.mm
// ...the whole upstream implementation, copied and modified.
```

Good:

```cpp
// chromium_src/ios/chrome/browser/some_file.mm
#define ChromeViewController BraveViewController

#include <ios/chrome/browser/some_file.mm>

#undef ChromeViewController
```
