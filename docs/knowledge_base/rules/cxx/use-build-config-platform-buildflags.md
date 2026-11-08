---
name: brave/use-build-config-platform-buildflags
description:
  'Platform conditions use BUILDFLAG(IS_*) from build/build_config.h, never
  compiler macros such as WIN32, __APPLE__ or __linux__, nor defined(OS_*).'
rubric: correctness
severity: should_fix
---

**Rule.** Test the platform with the `BUILDFLAG(IS_*)` macros of
`build/build_config.h` (`BUILDFLAG(IS_WIN)`, `BUILDFLAG(IS_ANDROID)`,
`BUILDFLAG(IS_MAC)`, ...), and include that header. NEVER test compiler-provided
macros such as `WIN32`, `_WIN32`, `__APPLE__` or `__linux__`, and never
`defined(OS_*)`, which no longer exists.

**Why.** Compiler macros do not match Chromium's platform definitions (Linux
macros are also defined on ChromeOS and Android), and `OS_*` macros are no
longer defined, so a test of them is always false.

## Examples

Bad:

```cpp
#if defined(WIN32)
#if defined(OS_ANDROID)
```

Good:

```cpp
#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
```
