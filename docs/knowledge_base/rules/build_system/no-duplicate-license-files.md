---
name: brave/no-duplicate-license-files
description:
  'A change must never add a copy of a license file Chromium or another project
  already has; it references the existing file or adds a special case instead.'
rubric: change_scope
severity: blocking
---

**Rule.** NEVER add a copy of a license file (`LICENSE`, `COPYING`, a license
text) that Chromium or another project already ships. Point to the existing file
instead, for example from the `License File:` of a `README.chromium`, or add a
special case to the license tooling (the `special_cases` of
`script/brave_license_helper.py`).

**Why.** A copied license drifts from the original when that project updates it,
and the credits page then shows two notices for one project, one of them stale.

## Examples

Bad:

```text
third_party/foo/LICENSE   # copied from //third_party/foo/src/LICENSE
```

Good:

```python
special_cases.update({
    'third_party/foo': {
        'Name': 'foo',
        'License File': ['/third_party/foo/src/LICENSE'],
    },
})
```
