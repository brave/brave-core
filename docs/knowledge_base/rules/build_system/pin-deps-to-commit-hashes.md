---
name: brave/pin-deps-to-commit-hashes
description:
  'A git dependency added or updated in a DEPS file must pin a commit hash,
  never a branch name or a tag.'
rubric: correctness
severity: blocking
---

**Rule.** Every git dependency in a `DEPS` file (a `url` written
`<repository>@<revision>`, or the revision variable it expands) MUST pin a
commit hash. NEVER pin a branch name (`@main`, `@refs/heads/...`) or a tag
(`@v1.2.3`, `@refs/tags/...`). CIPD packages (`"dep_type": "cipd"`) are not git
dependencies, and this rule says nothing about their `version`.

**Why.** Commit hashes are immutable, so every checkout of a brave-core revision
fetches the same code and builds are reproducible. A branch moves, and a tag can
be moved, so the same `DEPS` can fetch different code over time.

## Examples

Bad:

```python
"vendor/omaha": {
  "url": "https://github.com/brave/omaha.git@main",
  "condition": "checkout_win",
},
```

Good:

```python
"vendor/omaha": {
  "url": "https://github.com/brave/omaha.git@846b535b45365499c9097059502aaae2ed69fc85",
  "condition": "checkout_win",
},
```
