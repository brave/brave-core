---
name: brave/plaster-patterns-match-specific-context
description:
  A plaster regex matches the method name or other context that makes it a
  single, targeted match; a broad pattern is used only when every occurrence is
  meant to be replaced.
rubric: correctness
severity: should_fix
---

**Rule.** A `re_pattern` (or `pattern`) in a `rewrite/` plaster MUST include the
context, such as the enclosing method's name, that singles out the one
occurrence it means to change. A bare, common fragment such as `return false;`
is only acceptable when the substitution intends to replace every occurrence in
the file, deliberately, as with a constant renamed throughout; NEVER rely on a
broad pattern happening to match only the intended occurrence today.

**Why.** A broad pattern can match a different occurrence after a Chromium
update and rewrite the wrong code. A pattern that names its context is more
maintainable, and it fails instead of misfiring when upstream changes.

## Examples

Bad:

```yaml
re_pattern: 'return false;'
```

Good:

```yaml
re_pattern: 'bool IsFeatureEnabled[\(\)\S\s\{\}]+?(return false);\s+^\}'
```

```yaml
# Every occurrence is meant to change.
re_pattern: 'kOldConstant'
```
