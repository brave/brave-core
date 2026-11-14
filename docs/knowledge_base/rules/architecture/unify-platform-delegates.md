---
name: brave/unify-platform-delegates
description:
  'Logic needed on both Android and desktop lives in one shared class, with only
  the platform-specific parts, such as tab access, behind a delegate interface.'
rubric: design
severity: should_fix
---

**Rule.** When functionality runs on both Android and desktop, write its logic
once, and put behind a delegate interface only what differs per platform (such
as enumerating tabs through `BrowserList` or `TabModel`). Do not write an
Android delegate and a desktop delegate that each repeat the same logic.

**Why.** Duplicated platform implementations drift apart: a fix lands in one and
not the other.

## Examples

Bad:

```cpp
class DesktopDelegate { /* same logic with BrowserList */ };
class AndroidDelegate { /* same logic with TabModel */ };
```

Good:

```cpp
class UnifiedDelegate {
  virtual std::vector<TabInfo> GetOpenTabs() = 0;  // platform-specific
  void DoSharedLogic();                            // uses GetOpenTabs()
};
```
