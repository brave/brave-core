---
name: brave/feature-flag-checks-at-entry-points
description:
  'Feature flag state is checked at entry points, not with CHECK/DCHECK in
  private helpers called only from them; public APIs with many callers keep
  their own checks.'
rubric: readability
severity: should_fix
---

**Rule.** Check whether a feature is enabled at the entry points that lead to
the feature's code. Do not add `CHECK()`/`DCHECK()` of the feature's state to
private helpers that only those entry points call; give such a helper a comment
such as "Only called when kFoo is enabled" instead. A public function that any
code can call independently is an entry point itself and keeps its own guard.

**Why.** This is the pattern upstream follows. Repeated checks in every helper
add noise without adding protection, as the entry point has already decided.

## Examples

Bad:

```cpp
void TabStripModel::SetCustomTitle(...) {
  // Only called from OnTabContextMenuAction(), which checks the feature.
  CHECK(base::FeatureList::IsEnabled(kRenamingTabs));
}
```

Good:

```cpp
void OnTabContextMenuAction(int action) {
  if (!base::FeatureList::IsEnabled(kRenamingTabs)) {
    return;
  }
  model->SetCustomTitle(...);  // Only called when kRenamingTabs is enabled.
}

// A public API callable from anywhere keeps its own CHECK.
bool StoragePartitionUtils::IsContainersStoragePartition(...) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  ...
}
```
