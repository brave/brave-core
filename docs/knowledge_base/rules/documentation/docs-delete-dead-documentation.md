---
name: brave/docs-delete-dead-documentation
description:
  Documentation and comments that no longer match the code are updated or
  deleted, defaulting to removal when in doubt.
rubric: readability
severity: should_fix
---

**Rule.** When documentation or a comment no longer matches the code, update it
or delete it; do not leave it in place. Remove content that is certainly wrong,
and when in doubt, remove rather than keep: an absent doc is better than a wrong
one.

**Why.** Obsolete documentation misleads developers and erodes their trust in
the rest of it.

## Examples

Bad:

```cpp
// This method fetches data from the legacy XML endpoint.
void FetchData() {
  // Actually uses JSON now...
}
```

Good:

```cpp
// Fetches user data from the JSON API.
void FetchData() { ... }
```
