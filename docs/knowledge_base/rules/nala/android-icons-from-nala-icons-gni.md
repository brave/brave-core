---
name: brave/android-icons-from-nala-icons-gni
description:
  Android code gets a new icon by adding the Leo icon to android/nala/icons.gni,
  never by creating a new drawable resource.
rubric: design
severity: should_fix
---

**Rule.** To use a new icon on Android, add the Leo icon's entry to `nala_icons`
in `android/nala/icons.gni` (`ic_<name>.xml`, the Leo name with its dashes
written as underscores). Do NOT create a new icon drawable under a
`res/drawable*/` directory. When flagging a violation, tag
`@brave/nala-token-reviewers`.

**Why.** Leo is the canonical icon source across all platforms. An icon added
outside it is inconsistent with the rest of the UI, duplicates icons Leo already
has, and is one more asset to maintain by hand.

## Examples

Bad:

```
# A new drawable added directly.
android/java/res/drawable/ic_my_new_icon.xml
```

Good:

```gn
# android/nala/icons.gni
nala_icons = [
  ...
  "ic_my_new_icon.xml",
  ...
]
```
