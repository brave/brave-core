---
name: defaults/superseded-flags-and-or-predicates
rubric: defaults
severity: regression
---

**Rule.** Brave's feature overrides still gate the behaviour: the feature wasn't
superseded or split (`kFoo` → `kFooV2`, new params), and the predicates that
read it don't OR in other features left on.

**Why.** An override of a superseded flag pins nothing. A predicate that ORs a
pinned flag with others turns on through any of them.

**How to check.** The candidates remove features Brave names, or add `||` around
their checks. Find the successor of each removed one; grep each pinned flag's
`Is…Enabled()` helpers for `||`, and check every other operand's default. An
existing `||` turns on through a flip, not an edit, so also read the
`feature-flips` candidates for operands of predicates Brave pins.
`base/compile_overridden_features.inc` isn't a pin: check that its names are
upstream features.

## Examples

- M158, 8127.2: `IsContextualTasksUIEnabled()` turned on through
  `kContextualTasksSidePanel` past the `kContextualTasks` pin.
- M158: `ReadIsSubjectToUniversalOptOutCapability` in
  `base/compile_overridden_features.inc` isn't an upstream feature.
