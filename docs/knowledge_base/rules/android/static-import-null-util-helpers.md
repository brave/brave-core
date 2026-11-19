---
name: brave/static-import-null-util-helpers
description:
  assumeNonNull() and assertNonNull() are always statically imported from
  org.chromium.build.NullUtil, never called as NullUtil.assumeNonNull().
rubric: readability
severity: should_fix
---

**Rule.** Always bring `assumeNonNull` and `assertNonNull` in with
`import static org.chromium.build.NullUtil.assumeNonNull;` (and
`...NullUtil.assertNonNull;`) and call them unqualified. Do not import
`NullUtil` and write `NullUtil.assumeNonNull(...)`.

**Why.** These helpers are used inline in expressions, often several times in a
method; the class qualifier adds noise, and one import form keeps the code
consistent with Chromium's.

## Examples

Bad:

```java
import org.chromium.build.NullUtil;

NullUtil.assumeNonNull(mNullableThing);
```

Good:

```java
import static org.chromium.build.NullUtil.assumeNonNull;

assumeNonNull(mNullableThing);
```
