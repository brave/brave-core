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
Diff the names against Brave's lists. A new trial on a feature Brave pins off,
or on a sibling feature that doesn't depend on it, reaches the API past the pin:
its trial name goes in the blocklist.

## Examples

- M160, 8183.1: `JSSelfProfilingInDedicatedWorker` on
  `ProfilerAPIForDedicatedWorker`, beside Brave's disabled `ProfilerAPI`
  ([e0209e4e51a4](https://chromium.googlesource.com/chromium/src/+/e0209e4e51a47d42b4376aa8c43d9b866fdc29bf)).
