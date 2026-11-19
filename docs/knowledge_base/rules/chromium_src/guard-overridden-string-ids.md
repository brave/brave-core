---
name: brave/guard-overridden-string-ids
description:
  'An override redefining an upstream string ID via #undef/#define first checks
  with #ifndef/#error that the upstream ID still exists.'
rubric: correctness
severity: should_fix
---

**Rule.** Before `#undef IDS_FOO` and `#define IDS_FOO IDS_BRAVE_FOO` in an
override, add `#ifndef IDS_FOO` / `#error "IDS_FOO is not defined"` / `#endif`,
so that the build fails if upstream removes or renames the ID.

**Why.** If upstream removes or renames the ID, the `#undef` and `#define` still
compile, and the override silently stops applying. `check_chromium_src.py`
verifies that overridden symbols are still used in the overridden file, but the
guard is an additional safety net that fails the build itself.

## Examples

Bad:

```cpp
#undef IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS
#define IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS \
  IDS_BRAVE_PASSWORD_MANAGER_UI_EMPTY_STATE
```

Good:

```cpp
#ifndef IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS
#error "IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS is not defined"
#endif
#undef IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS
#define IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS \
  IDS_BRAVE_PASSWORD_MANAGER_UI_EMPTY_STATE
```
