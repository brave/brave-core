---
name: brave/ios-gn-args-for-platform-feature-defaults
description:
  'Platform-specific feature defaults for iOS are set with GN build arguments,
  not with #if blocks in C++ source.'
rubric: design
severity: should_fix
---

**Rule.** When a feature's default differs on iOS, set it from a GN build
argument (exposed to C++ through a buildflag) instead of writing
`#if BUILDFLAG(IS_IOS)` around the feature's default state in C++.

**Why.** A build argument can be changed per build without editing source, and
keeps platform `#if` clutter out of the feature definitions.

## Examples

Bad:

```cpp
BASE_FEATURE(kBraveFoo,
#if BUILDFLAG(IS_IOS)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);
```

Good:

```gn
declare_args() {
  enable_brave_foo_by_default = !is_ios
}

buildflag_header("buildflags") {
  header = "buildflags.h"
  flags = [ "ENABLE_BRAVE_FOO_BY_DEFAULT=$enable_brave_foo_by_default" ]
}
```

```cpp
BASE_FEATURE(kBraveFoo,
             BUILDFLAG(ENABLE_BRAVE_FOO_BY_DEFAULT)
                 ? base::FEATURE_ENABLED_BY_DEFAULT
                 : base::FEATURE_DISABLED_BY_DEFAULT);
```
