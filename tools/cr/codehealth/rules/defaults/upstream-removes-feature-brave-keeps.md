---
name: defaults/upstream-removes-feature-brave-keeps
rubric: defaults
severity: regression
---

**Rule.** Clean-up code that upstream adds when it removes a feature doesn't
delete data or prefs that Brave still uses.

**Why.** Removal campaigns add startup code that deletes the feature's database,
files or prefs. When Brave keeps the feature, that code silently wipes Brave's
data on every start.

**How to check.** The candidates add deletion or pref clearing, or say
"residual" or "deprecated" in a deletion. Match the paths and pref names they
delete against Brave's users of the feature (`git grep -n '<file or pref>'`).

## Examples

- M159, 8133.1: `DeleteHistoryEmbeddingsDatabase()` deletes the database Brave's
  history embeddings service still uses.
