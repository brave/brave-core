---
name: brave/reuse-existing-gn-args
description:
  'A new GN arg must not duplicate an arg Chromium or Brave already declares for
  the same setting; reuse the existing one.'
rubric: design
severity: should_fix
---

**Rule.** Before a change declares a new arg in a `declare_args()` block, check
whether Chromium's or Brave's `.gni` files already declare one for the same
setting, and use that one if they do. Do not add a Brave-prefixed twin of an
existing arg, such as `brave_android_keystore_path` when `android_keystore_path`
already exists.

**Why.** Duplicated config arguments add confusion and maintenance burden: two
args for one setting can be given different values, and readers cannot tell
which of them the build honours.

## Examples

Bad:

```gn
declare_args() {
  brave_android_keystore_path = ""
}
```

Good:

```gn
# Use the existing arg from //build/config/android/config.gni.
args += [ "--keystore=" + rebase_path(android_keystore_path, root_build_dir) ]
```
