---
name: brave/grd-no-edits-to-upstream-copies
description:
  Strings are never added to or modified in the upstream Chromium GRD/GRDP
  files, which rebases replace; customizations go in Brave's GRD/GRDP files with
  a chromium_src IDS_ override.
rubric: correctness
severity: blocking
---

**Rule.** NEVER hand-edit Brave's copies of upstream GRD/GRDP files (such as
`app/generated_resources.grd`, `app/settings_strings.grdp` or
`components/browser_ui/strings/android/browser_ui_strings.grd`), nor patch
upstream GRD/GRDP files. Only `pnpm run chromium_rebase_l10n` changes the
copies. To customize an upstream string, add a new string to the matching Brave
file (`app/brave_generated_resources.grd`, `app/brave_settings_strings.grdp`,
`browser/ui/android/strings/android_brave_strings.grd`), and add a
`chromium_src` override that `#undef`s the upstream `IDS_` constant and
`#define`s it to the Brave one.

**Why.** These files are replaced on every Chromium version bump, so a hand edit
is silently lost on the next rebase.

## Examples

Bad:

```xml
<!-- app/settings_strings.grdp -->
<message name="IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS" desc="...">
  Passwords you save in Brave appear here
</message>
```

Good:

```cpp
// chromium_src/chrome/browser/ui/webui/password_manager/password_manager_ui.cc
#undef IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS
#define IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS \
  IDS_BRAVE_PASSWORD_MANAGER_UI_EMPTY_STATE_SIGNEDOUT_USERS
```
