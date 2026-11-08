---
name: brave/new-android-resources-in-brave-res
description:
  New Brave-only Android resources go in a brave-res folder; the upstream
  resource directories hold only resources that intentionally override an
  upstream one.
rubric: design
severity: should_fix
---

**Rule.** A new Brave-specific Android resource (drawable, layout, values file
and so on) MUST be added under a `brave-res` folder (such as
`android/java/brave-res/`). The upstream-mirroring resource directories (such as
`android/java/res/`, listed in `brave_java_resources.gni`) are only for files
that intentionally override an existing upstream resource of the same name.

**Why.** Brave-only resources in the upstream directories make it unclear
whether a file is a Brave addition or an upstream override, which matters at
every Chromium upgrade.

## Examples

Bad:

```
android/java/res/drawable/ic_foo_badge.xml   # no upstream resource of that name
```

Good:

```
android/java/brave-res/drawable/ic_foo_badge.xml
```
