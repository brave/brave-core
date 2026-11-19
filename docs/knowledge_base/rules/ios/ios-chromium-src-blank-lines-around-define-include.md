---
name: brave/ios-chromium-src-blank-lines-around-define-include
description:
  'In iOS chromium_src overrides, an #include wrapped by #define and #undef is
  separated from them by blank lines.'
rubric: readability
severity: nit
---

**Rule.** Separate the `#include` of the upstream file from the `#define` lines
before it and the `#undef` lines after it with one blank line on each side.

**Why.** The blank lines set the substituted include apart, so the reader sees
at a glance which names are redefined around it.

## Examples

Bad:

```cpp
#define ChromeViewController BraveViewController
#include <ios/chrome/browser/some_file.mm>
#undef ChromeViewController
```

Good:

```cpp
#define ChromeViewController BraveViewController

#include <ios/chrome/browser/some_file.mm>

#undef ChromeViewController
```
