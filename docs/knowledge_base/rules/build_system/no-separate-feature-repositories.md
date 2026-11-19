---
name: brave/no-separate-feature-repositories
description:
  'Code for a Brave feature is kept in brave-core, not in a new separate
  repository pulled in through DEPS.'
rubric: design
severity: should_fix
---

**Rule.** Do not add a `DEPS` entry for a new repository that holds code for a
Brave feature. Keep that code within brave-core, normally as a component under
`components/`.

**Why.** Separate repositories are harder to manage, code review and maintain:
changes need coordinated rolls, the feature's code is reviewed outside
brave-core's presubmits and owners, and its history is split from the code using
it.

## Examples

Bad:

```python
"vendor/brave-foo-feature": "https://github.com/brave/brave-foo-feature.git@0123456789abcdef0123456789abcdef01234567",
```

Good:

```text
components/brave_foo/   # the feature's code, built by brave-core's BUILD.gn
```
