---
name: brave/should-prefix-for-bool-queries
description:
  'A method answering whether something should happen is named Should*, not Get*
  for a bool query.'
rubric: readability
severity: nit
---

**Rule.** Name a method that returns whether something should happen
`Should*()`, and do not prefix a bool query with `Get`: `ShouldShowX()`, never
`GetShouldShowX()` or `GetShowX()`.

**Why.** A `Get*` name suggests a stored value is returned; `Should*` reads as
the question the caller asks, and keeps queries named alike across the code
base.

## Examples

Bad:

```cpp
bool GetShouldShowBrandedWallpaper();
```

Good:

```cpp
bool ShouldShowBrandedWallpaper();
```
