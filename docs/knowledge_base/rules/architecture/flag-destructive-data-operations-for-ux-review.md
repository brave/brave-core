---
name: brave/flag-destructive-data-operations-for-ux-review
description:
  'Code that deletes user data, such as clearing preferences or wiping storage,
  is flagged for UX review before it is implemented.'
rubric: design
severity: blocking
---

**Rule.** When a change deletes user data, for example clearing preferences the
user set or wiping a feature's storage, the deletion MUST have been through UX
review. Flag it and ask whether it was reviewed, and how the user is told.

**Why.** Deleting data silently surprises users and can break their trust; UX
review decides whether, when and how the deletion is surfaced.

## Examples

Bad:

```cpp
void FooService::OnFeatureDisabled() {
  prefs_->ClearPref(kFooUserSettings);       // user settings silently lost
  base::DeletePathRecursively(storage_dir_);
}
```

Good:

```cpp
// UX-reviewed: the user confirms in the "Reset Foo data" dialog.
void FooService::ResetUserData() {
  prefs_->ClearPref(kFooUserSettings);
  base::DeletePathRecursively(storage_dir_);
}
```
