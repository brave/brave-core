---
name: brave/link-upstream-issue-in-workarounds
description:
  'A workaround for an upstream Chromium bug links the upstream issue in a
  comment, so that it can be removed when the fix lands.'
rubric: readability
severity: should_fix
---

**Rule.** Code that works around an upstream Chromium bug MUST carry a comment
linking the upstream issue (`https://crbug.com/<id>`) and saying what the
workaround does and when it can be removed. The author also files the details on
the upstream issue so that upstream can fix it.

**Why.** Without the link, nobody can tell when the workaround has become
unnecessary, and it stays in the code base for good.

## Examples

Bad:

```cpp
// Upstream doesn't handle a null X here.
if (!x) {
  return;
}
```

Good:

```cpp
// Workaround for https://crbug.com/123456 - upstream doesn't handle
// the case where X is null. Remove when the upstream fix lands.
if (!x) {
  return;
}
```
