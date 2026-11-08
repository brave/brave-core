---
name: brave/no-positional-references-in-comments
description:
  'Comments refer to other code by name, never as "above" or "below".'
rubric: readability
severity: nit
---

**Rule.** Do not point to other code with "above" or "below" (or "the
previous/next test") in a comment, in source files, test filter files, or
patches alike. Name the function, test, constant or entry referred to.

**Why.** Code gets inserted and moved between the comment and what it refers to,
and the reference then silently points at the wrong thing.

## Examples

Bad:

```cpp
// Same root cause as the test above
-BrowserTest.SomeOtherTest
```

Good:

```cpp
// Same root cause as BrowserTest.FirstTest (kPromptForDownload override)
-BrowserTest.SomeOtherTest
```
