---
name: brave/explicit-enum-values-around-conditional-members
description:
  'When an enum has members compiled out behind a build flag, every member gets
  an explicit numeric value so the others do not shift between builds.'
rubric: correctness
severity: should_fix
---

**Rule.** If any enumerator of an enum sits inside `#if BUILDFLAG(...)` (or
another preprocessor conditional), assign an explicit value to every enumerator
of that enum, including those inside the conditional, so that each keeps the
same number whatever the build configuration.

**Why.** Implicit values count up from the previous enumerator, so compiling one
out renumbers everything after it. Values that are persisted in prefs, recorded
in histograms, sent over IPC or shared between components then mean different
members in different builds.

## Examples

Bad:

```cpp
enum class SidebarItem {
  kBookmarks,
#if BUILDFLAG(ENABLE_BRAVE_TALK)
  kTalk,
#endif
  kHistory,  // value changes depending on build flag!
};
```

Good:

```cpp
enum class SidebarItem {
  kBookmarks = 0,
#if BUILDFLAG(ENABLE_BRAVE_TALK)
  kTalk = 1,
#endif
  kHistory = 2,
};
```
