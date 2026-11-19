---
name: brave/docs-minimal-fresh-documentation
description:
  'Documentation is kept small, accurate and current: no docs that will not be
  maintained, and no sections nobody reads or updates.'
rubric: readability
severity: nit
---

**Rule.** Prefer a small amount of accurate, current documentation to extensive
documentation in poor condition. Do not write documentation that will not be
maintained, remove sections that nobody reads or updates, and focus on the
documentation that saves developers the most time.

**Why.** Documentation is only worth what it costs to keep accurate; large,
unmaintained docs go stale and mislead.

## Examples

Bad:

```markdown
## Appendix: Full History of the Rewards Protocol (2019–2021)

(Several pages describing designs that no longer exist.)
```

Good:

```markdown
## Protocol

The current protocol is described in `rewards_protocol.md`.
```
