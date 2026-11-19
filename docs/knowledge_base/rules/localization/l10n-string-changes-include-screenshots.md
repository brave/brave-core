---
name: brave/l10n-string-changes-include-screenshots
description:
  A pull request that adds or modifies user-facing strings includes screenshots
  of the affected UI in its description.
rubric: change_scope
severity: should_fix
---

**Rule.** When a change adds or modifies user-facing strings, its PR description
MUST include screenshots of the rendered UI that shows them, not of the GRD/GRDP
diff. Put them in the PR description body, not in inline code comments.

**Why.** String reviewers check strings in context, for truncation,
capitalization consistency, layout and terminology, which the diff alone does
not show.

## Examples

Bad:

```text
PR description: "Adds strings for the new Shields panel."
```

Good:

```text
PR description: "Adds strings for the new Shields panel." followed by
screenshots of the panel in light and dark mode.
```
