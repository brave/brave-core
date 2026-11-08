---
name: brave/remove-dead-code-with-removed-features
description:
  'Removing a feature flag, model or script also removes all the code, strings
  and constants only it used.'
rubric: change_scope
severity: should_fix
---

**Rule.** A change that removes a feature flag, a model or a script MUST also
remove what only it used: helper and utility functions, constants, prefs no
longer read, and UI strings left orphaned in `.grd`/`.grdp` files.

**Why.** Code and strings left behind compile, get translated and confuse
readers indefinitely, as nothing marks them as unused.

## Examples

Bad:

```cpp
-BASE_FEATURE(kBraveOldThing, base::FEATURE_DISABLED_BY_DEFAULT);
 // Still present, now called by nothing:
 bool IsOldThingSupported(const GURL& url) { ... }
```

Good:

```cpp
-BASE_FEATURE(kBraveOldThing, base::FEATURE_DISABLED_BY_DEFAULT);
-bool IsOldThingSupported(const GURL& url) { ... }
```
