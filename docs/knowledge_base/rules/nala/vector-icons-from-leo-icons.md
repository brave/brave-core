---
name: brave/vector-icons-from-leo-icons
description:
  C++ browser UI gets a new icon by adding the Leo icon to leo_icons in
  components/vector_icons/BUILD.gn, not by creating a new .icon file.
rubric: design
severity: should_fix
---

**Rule.** To use a new icon in C++ browser UI, add the Leo icon to `leo_icons`
in `components/vector_icons/BUILD.gn` (as `leo_<name>.icon`, generated from
Leo's Skia icons) and use the generated `kLeo<Name>Icon`. Do NOT create a new
`.icon` file, or add one to the `sources` of an `aggregate_vector_icons()`
target. When flagging a violation, tag `@brave/nala-token-reviewers`.

**Why.** Leo is the canonical icon source across all platforms. An icon added
outside it is inconsistent with the rest of the UI, duplicates icons Leo already
has, and is one more asset to maintain by hand.

## Examples

Bad:

```
# A new standalone vector icon file.
components/vector_icons/my_new_icon.icon
```

Good:

```gn
# components/vector_icons/BUILD.gn
leo_icons = [
  ...
  "leo_my_new_icon.icon",
  ...
]
```
