---
name: brave/migrate-prefs-of-removed-features
description:
  'Removing or renaming a feature, model or pref value that users may have
  selected comes with a migration that resets their prefs to a sensible default.'
rubric: correctness
severity: should_fix
---

**Rule.** When a change removes or renames a feature, a model, or a pref key or
value that users may have selected, it MUST add a migration step that finds
profiles holding the removed value and resets the affected prefs to a sensible
default.

**Why.** Otherwise users who had chosen the removed option keep a stale value
that no code understands any more, and see errors or confusing behaviour.

## Examples

Bad:

```cpp
// The model is deleted; users who selected it keep its key in their prefs.
-  {"chat-old-model-1", ...},
```

Good:

```cpp
void MigrateProfilePrefs(PrefService* prefs) {
  const std::string current = prefs->GetString(prefs::kDefaultModelKey);
  static constexpr auto kRemovedModels =
      base::MakeFixedFlatSet<std::string_view>({
          "chat-old-model-1",
          "chat-old-model-2",
      });
  if (kRemovedModels.contains(current)) {
    prefs->ClearPref(prefs::kDefaultModelKey);
  }
}
```
