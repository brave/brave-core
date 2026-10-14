---
name: defaults/other-feature-lists
rubric: defaults
severity: regression
---

**Rule.** Brave's other feature lists follow upstream: the origin-trial
blocklist in
`chromium_src/third_party/blink/common/origin_trials/origin_trials.cc` and the
Android `GlicEnabling.java` list.

**Why.** Each is a list of upstream names, which misses new trials and entry
points, and silently keeps renamed ones.

**How to check.** The candidates change origin trial names in
`runtime_enabled_features.json5` or the trial code, and `GlicEnabling.java`.
Diff the names against Brave's lists.
