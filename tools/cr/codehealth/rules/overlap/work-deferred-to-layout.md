---
name: overlap/work-deferred-to-layout
rubric: overlap
severity: regression
---

**Rule.** Brave `Layout()` overrides that return early still do the work
upstream moved into `Layout()`.

**Why.** Performance CLs that "mark dirty, flush in `Layout()`" skip Brave
overrides that don't reach upstream's `Layout()`.

**How to check.** For each candidate, check whether a Brave subclass overrides
the view's `Layout()` and returns before calling upstream's.

## Examples

- `BraveTabContainer` and accessible tab indices.
