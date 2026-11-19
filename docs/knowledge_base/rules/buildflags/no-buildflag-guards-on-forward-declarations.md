---
name: brave/no-buildflag-guards-on-forward-declarations
description:
  'Forward declarations are never wrapped in #if BUILDFLAG(...) guards; only
  #includes and actual usages are.'
rubric: readability
severity: should_fix
---

**Rule.** Do not wrap a forward declaration (`class HistoryTool;`) in a
`#if BUILDFLAG(...)` guard, and NEVER ask for one. Guard the `#include`
directives and the actual usages instead.

**Why.** Forward declarations are harmless: they do not pull in dependencies and
cost nothing at compile time if unused. A guard around one only adds
preprocessor noise.

## Examples

Bad:

```cpp
#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT)
class HistoryTool;
#endif
```

Good:

```cpp
class HistoryTool;
```
