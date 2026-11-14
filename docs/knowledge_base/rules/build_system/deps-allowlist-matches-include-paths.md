---
name: brave/deps-allowlist-matches-include-paths
description:
  'A path added to a DEPS include allowlist must exactly match the #include
  paths of the files using it.'
rubric: correctness
severity: blocking
---

**Rule.** Each `+path` added to `include_rules` or `specific_include_rules` in a
`DEPS` file MUST match the `#include` paths of the source files it is for,
directory by directory (`core/` is not `common/`).

**Why.** A mismatched entry allows a path nobody includes, so the DEPS check
does not validate the file that is actually included, and the entry the include
needed is still missing.

## Examples

Bad:

```python
include_rules = [
  "+brave/components/foo/common",  # the code includes foo/core/foo_utils.h
]
```

Good:

```python
include_rules = [
  "+brave/components/foo/core",
]
```
