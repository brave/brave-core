---
name: overlap/visibility-dependent-lookups
rubric: overlap
severity: regression
---

**Rule.** Upstream element-tracker lookups still find what Brave hides.

**Why.** Element-tracker lookups only find visible views, and Brave usually
hides upstream views rather than removing them, so a new lookup of a view Brave
hides finds nothing.

**How to check.** For each candidate's new lookup, check whether Brave hides the
element it looks up.
