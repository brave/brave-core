---
name: brave/platform-browser-test-for-cross-platform-tests
description:
  'A browser test meant to run on both desktop and Android derives from
  PlatformBrowserTest, not InProcessBrowserTest.'
rubric: testing
severity: should_fix
---

**Rule.** When a browser test should run on desktop and Android, derive its
fixture from `PlatformBrowserTest`, including
`chrome/test/base/android/android_browser_test.h` on Android and
`chrome/test/base/in_process_browser_test.h` elsewhere. Do not derive it from
`InProcessBrowserTest`, which only exists on desktop.

**Why.** `InProcessBrowserTest` is not available on Android, so a test built on
it cannot run there, and the Android coverage the test was meant to give is
lost.

## Examples

Bad:

```cpp
class MyBrowserTest : public InProcessBrowserTest {};
```

Good:

```cpp
#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

class MyBrowserTest : public PlatformBrowserTest {};
```
