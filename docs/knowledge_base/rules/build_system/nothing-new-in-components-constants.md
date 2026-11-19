---
name: brave/nothing-new-in-components-constants
description:
  'Never add constants, headers, targets or new deps to the deprecated
  components/constants; constants go in a common target of the component owning
  them.'
rubric: design
severity: blocking
---

**Rule.** NEVER add a constant, header or target to `components/constants`, and
do not add new `deps` on `//brave/components/constants`. Put the constant in a
`common` target of the component that owns it. When a constant, especially a
URL, is shared across several components with no clear owner, put it in a
`brave_domains` target. Migrating existing uses out of `components/constants` is
welcome.

**Why.** `components/constants` is not a real component: it is a deprecated
legacy dumping ground that should be removed, and everything added there or
depending on it makes that removal harder.

## Examples

Bad:

```cpp
// components/constants/brave_constants.h
inline constexpr char kGate3OAuthUrl[] = "...";
```

Good:

```cpp
// components/brave_rewards/common/constants.h
inline constexpr char kGate3OAuthUrl[] = "...";
```
