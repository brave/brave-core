---
name: brave/docs-update-with-code
description:
  A change that alters documented code updates that documentation (READMEs,
  docs, comments) in the same change, not in a later one.
rubric: change_scope
severity: should_fix
---

**Rule.** When a change modifies an API or behavior that documentation describes
(a README, a page under `docs/`, or the comments on the declarations), it MUST
update that documentation in the same change. Do not defer documentation to a
follow-up change.

**Why.** Follow-up documentation changes tend to be forgotten or lose context.
Updating docs in the same change keeps them accurate, and shows reviewers what
the change means.

## Examples

Bad:

```text
CL 1: Refactor RewardsService API
CL 2: (never created) Update RewardsService docs
```

Good:

```text
CL 1: Refactor RewardsService API + update README and method docs
```
