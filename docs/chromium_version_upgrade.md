# Upgrading `brave-core` to a newer version of Chromium

This document describes the basics and the tooling available to have
`brave-core` upstream tag selection chromium, also known as a *version bump*.

## Understanding the basics

In order to be able to depend on, and make changes to vanilla Chromium,
`brave-core` has to pick a tag of Chromium to checkout and build against. This
tag is set in [`package`](../package.json) and saved as `tag`.

```json
  "config": {
    "projects": {
      "chrome": {
        "dir": "src",
        "tag": "134.0.6998.95",
        "repository": {
          "url": "https://github.com/brave/chromium"
        }
      },
```

### Common version bump changes

It is the current practice, when changing upstream tag, to produce these
changes documenting specific steps documenting the steps involved in a upstream
tag update.

| Commit name | Purpose |
| --- | --- |
| `Upgrade` | A change with the upgrading of the chromium tag in `package.json` |
| `Conflict-resolved patches` | Patches that may have failed to apply, and required manual conflict resolution, once `npm run init` was run on the new tag |
| `Update patches` | Regenerated versions of all patches that applied cleanly. |
| `Updated strings` | Translation strings may have had updates once regenerated under the new Chromium tag. |

There may be many other changes necessary to get Brave to build with the tag
being picked, but these four changes are a common idiom when doing doing a
version bump.

## Doing a version upgrade with `npm run` tools

All version bumps are done with *🚀Brockit!*, however this section goes over
the basic underlying steps of how one can produce a bump only using `npm run`
tools.

It is important to have a good understanding of the steps involved in order to
be able to understand what exactly the automation on top is doing.

### The `package.json` update change

The first step to start the bump would be updating `package.json`, and then
committing it to the branch.
```shell
$ git checkout -b cr121 origin/master  # branch name is cr+Major number

# package.json is saved
$ git add package.json

# From is the tag that is on origin/master, while to is the version being
# committed now in `package.json`
$ git commit -m "Update from Chromium 119.7049.17 to Chromium 120.0.7050.40."
```

### Dealing with patch failure and conflict resolution

Depending what changed since the last time a tag was picked for `brave-core`,
there could be any number of patches that can't apply anymore, for a variety of
reason, be it due to merge conflicts, or because the source file being patched
is no longer valid.

This step has to do with checking out the tag, applying patches, and in case of
failure, dealing with conflict resolution.
```shell
# Checks out the new tag, and tries to apply all patches
$ npm run init

# npm run apply patches (not needed as init already calls it for us)
```

At this point, if no failure occurs, there's nothing else to be done, and one
should carry on to the next step. However, when failure occurs this means that
that we are expected to manually fix the failed patches.

A failed run usually produces a failure report listing the files that failed to
apply.

```
$ npm run init
# ... For the sake of explanation, just the failures

2 failed patches:
chrome/browser/ui/views/frame/browser_view_layout.cc
  - Patch applied because: The target file was modified since the patch was last applied.
  - Error - Program git exited with error code 1.
error: patch failed: chrome/browser/ui/views/frame/browser_view_layout.cc:72
error: chrome/browser/ui/views/frame/browser_view_layout.cc: patch does not apply

----------------------------------------------------------------------------------------------------------
chrome/browser/ui/webui/settings/site_settings_helper.cc
  - Patch applied because: The target file was modified since the patch was last applied.
  - Error - Program git exited with error code 1.
error: patch failed: chrome/browser/ui/webui/settings/site_settings_helper.cc:248
error: chrome/browser/ui/webui/settings/site_settings_helper.cc: patch does not apply

```

With this failure report, the user can manually attempt to apply these failed
patches with `--3way`, which allows for conflict resolution. The name of the
patch file can be deduced from the name of the file reported in the failure
report above.

chrome/browser/ui/views/frame/browser_view_layout.cc
chrome/browser/ui/webui/settings/site_settings_helper.cc

brave/patches/chrome-browser-ui-views-frame-browser_view_layout.cc.patch
brave/patches/chrome-browser-ui-webui-settings-site_settings_helper.cc.patch

| Commit name | Purpose |
| --- | --- |
| `chrome/browser/ui/views/frame/browser_view_layout.cc` | `brave/patches/chrome-browser-ui-views-frame-browser_view_layout.cc.patch` |
| `chrome/browser/ui/webui/settings/site_settings_helper.cc` | `brave/patches/chrome-browser-ui-webui-settings-site_settings_helper.cc.patch` |

Therefore these patches should be applied as:
```shell
# Applying patches in `chromium/src`
$ git -C ../ apply --3way --ignore-space-change --ignore-whitespace \
      brave/patches/chrome-browser-ui-views-frame-browser_view_layout.cc.patch \
      brave/patches/chrome-browser-ui-webui-settings-site_settings_helper.cc.patch

# Resetting state to avoid issues when trying to regenerate the patches
$ git -C ../ reset HEAD
```

The output to `git apply` will be usually something as the following.
```
Applied patch to 'chrome/browser/ui/views/frame/browser_view_layout.cc' with conflicts.
U chrome/browser/ui/views/frame/browser_view_layout.cc
Applied patch to 'chrome/browser/ui/webui/settings/site_settings_helper.cc' with conflicts.
U chrome/browser/ui/webui/settings/site_settings_helper.cc
```

This means these files have now conflicts waiting for resolution before
proceeding.

> [!WARNING]
> There may be broken patches for repositories other than `chromium/src`. In
> those cases, it is necessary to run both `reset` and `apply` from for the
> failed patches of those repositories too.

#### Patches for deleted sources and broken patches

In some cases patches cannot be applied anymore either because the file that
was being patched is removed/renamed, or because there's something wrong with
the patch. In these cases it is important to commit the patch removal as its
own change, outside the `Conflict-resolved patches` change, as it makes it
easier for reviewers to understand the reason why some patch is being removed
or drastically changed.

#### `npm run update_patches` and committing conflict resolution

Once all merge conflicts are resolved, update all patches, and commit the
the conflict-resolved change to your branch.

```shell
$ npm run update_patches
$ git add patches/chrome-browser-ui-views-frame-browser_view_layout.cc.patch
$ git add patches/chrome-browser-ui-webui-settings-site_settings_helper.cc.patch
$ git commit -m " Conflict-resolved patches from Chromium 119.7049.17 to Chromium 120.0.7050.40."
```

### Updating all patches left by `npm run update_patches`

At this stage, if you have not updated all patches yet, then have do so.
```shell
$ npm run update_patches
```

With all patches that had problems out of the way, and committed as part of
`Conflict-resolved patches`, or in some other change, it is time to commit all
remaining changed paths as part of `Update patches`.
```shell
git add -u *.patch
$ git commit -m " Update patches from Chromium 119.7049.17 to Chromium 120.0.7050.40."

# Run init again to make sure all patches are green.
$ npm run init
```

### Regenerating l18n strings

This last step consists of regenerating all string files that are replicated in
`brave-core` from chromium.

```shell
$ npm run chromium_rebase_l10n
$ git add *.grdp *.grd *.xtb
$ git commit -m " Updated strings for Chromium 120.0.7050.40."
```

This step concludes the process of management of the infrastructure to pick out
a Chromium version, and the branch is ready to run a build, to correct the
build failures that may now be occurring.

## Using *🚀Brockit!* for version management

TBD.
