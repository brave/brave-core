---
name: brave/include-guards-not-pragma-once
description:
  'Every header has a standard #include guard; #pragma once is not allowed.'
rubric: correctness
severity: should_fix
---

**Rule.** Protect every header with an `#ifndef`/`#define`/`#endif` include
guard. NEVER use `#pragma once`.

**Why.** Chromium's style guide requires guards: `#pragma once` was historically
unsupported on some platforms and is no faster than guards.

## Examples

Bad:

```cpp
#pragma once
```

Good:

```cpp
#ifndef BRAVE_COMPONENTS_FOO_BAR_H_
#define BRAVE_COMPONENTS_FOO_BAR_H_

...

#endif  // BRAVE_COMPONENTS_FOO_BAR_H_
```
