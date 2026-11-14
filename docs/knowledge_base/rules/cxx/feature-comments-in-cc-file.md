---
name: brave/feature-comments-in-cc-file
description:
  'The comment explaining a base::Feature sits on its BASE_FEATURE definition in
  the .cc file, not on its declaration in the header.'
rubric: readability
severity: nit
---

**Rule.** Put the comment describing what a feature enables above its
`BASE_FEATURE(...)` definition in the `.cc` file. Leave the
`BASE_DECLARE_FEATURE(...)` in the header without it, and do not duplicate the
comment in both files.

**Why.** It is Brave's convention, the one `chrome/common/chrome_features.h`
also states: the description lives next to the definition and its default state,
and one copy cannot drift from another.

## Examples

Bad:

```cpp
// my_features.h
// Enables the new tab workaround for flash prevention.
BASE_DECLARE_FEATURE(kBraveWorkaroundNewWindowFlash);
```

Good:

```cpp
// my_features.h
BASE_DECLARE_FEATURE(kBraveWorkaroundNewWindowFlash);

// my_features.cc
// Enables the new tab workaround for flash prevention.
BASE_FEATURE(kBraveWorkaroundNewWindowFlash, base::FEATURE_ENABLED_BY_DEFAULT);
```
