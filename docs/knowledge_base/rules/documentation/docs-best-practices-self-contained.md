---
name: brave/docs-best-practices-self-contained
description:
  'Entries in docs/best-practices/ are self-contained: they state the rule,
  rationale, caveats and examples directly, and never link to PRs, review
  comments, other entries or external documentation.'
rubric: readability
severity: should_fix
---

**Rule.** Each entry in `docs/best-practices/*.md` MUST be self-contained:
include the rule, its rationale, caveats and code examples directly. NEVER add
links to PRs, review comments, other best-practice entries or external
documentation. Distill only the relevant guidance from a source, and NEVER copy
whole source documents.

**Why.** Best-practice entries are read in isolation, by people and by automated
reviewers. A link leaves out what the reader needs; a copied document buries it.

## Examples

Bad:

```markdown
## ✅ Use `base::OnceCallback` for Single-Use Callbacks

See the discussion in https://github.com/brave/brave-core/pull/12345 and
[Chromium's callback docs](https://chromium.googlesource.com/chromium/src/+/main/docs/callback.md).
```

Good:

```markdown
## ✅ Use `base::OnceCallback` for Single-Use Callbacks

**Use `base::OnceCallback` for callbacks that run at most once.** It makes the
single use explicit and lets the callback own its bound arguments.
```
