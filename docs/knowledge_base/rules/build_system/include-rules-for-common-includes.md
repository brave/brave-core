---
name: brave/include-rules-for-common-includes
description:
  'In DEPS files, includes allowed for the files of a directory go in
  include_rules; specific_include_rules is reserved for includes only some files
  may use.'
rubric: readability
severity: should_fix
---

**Rule.** Add an include path that the files of a directory generally need to
the `DEPS` file's `include_rules`. Use `specific_include_rules` only for an edge
case: an include that should be allowed in specific files and not the rest. Do
not put commonly needed includes under a catch-all `specific_include_rules`
pattern such as `".*\.cc"`.

**Why.** A `specific_include_rules` entry matching most files is an
`include_rules` entry in disguise: it is harder to read, and it hides the real
exceptions that `specific_include_rules` is meant to call out.

## Examples

Bad:

```python
specific_include_rules = {
  ".*\.cc": [
    "+brave/components/content_settings/core/common",
    "+brave/components/content_settings/core/browser",
  ],
}
```

Good:

```python
include_rules = [
  "+brave/components/content_settings/core/common",
  "+brave/components/content_settings/core/browser",
]
```
