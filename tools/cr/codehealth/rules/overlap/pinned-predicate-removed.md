---
name: overlap/pinned-predicate-removed
rubric: overlap
severity: regression
---

**Rule.** The predicates and flags Brave pins still gate the code Brave meant
them to.

**Why.** "Remove X from Y" and flag clean-ups hard-code the launched branch
exactly where Brave forced the other one, which re-enables the feature. A Brave
kill switch on a high-level helper likewise misses callers that upstream
switches to a lower-level one.

**How to check.** The candidates remove a predicate or feature Brave names. Run
`git log -S'<predicate>'` for each predicate Brave pins in `rewrite/`, and check
what the launched branch does where Brave pinned the other one.

## Examples

- `IsReplaceSyncPromosWithSignInPromosEnabled` stopped gating the avatar "Sign
  in" pill.
