---
name: brave/docs-best-practices-consistent
description:
  Best practices are consistent with other documentation; a change resolving
  conflicting guidance updates every affected document in the same change.
rubric: correctness
severity: should_fix
---

**Rule.** A best-practice entry MUST NOT contradict other documentation. When a
change adds or changes guidance that conflicts with another document, update the
affected documents in the same change.

**Why.** Contradicting guidance leaves readers, and reviewers, to pick one at
random.

## Examples

Bad:

```text
docs/best-practices/frontend.md: "Use X."   (changed)
docs/best-practices/nala.md:     "Never use X."   (unchanged)
```

Good:

```text
docs/best-practices/frontend.md: "Use X."   (changed)
docs/best-practices/nala.md:     "Use X."   (updated in the same change)
```
