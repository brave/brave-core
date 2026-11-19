---
name: brave/no-window-state-in-tab-features
description:
  'A tab-scoped feature never holds a Browser*, BrowserWindowInterface* or other
  browser-window state, since tabs move between windows.'
rubric: safety
severity: blocking
---

**Rule.** A tab-scoped feature MUST NOT store a `Browser*`, a
`BrowserWindowInterface*`, or any other browser-window-level object or state, as
a member, `raw_ptr`, `raw_ref` or `WeakPtr`. Hold the `tabs::TabInterface`, and
ask it for the current window when needed (`tab_->GetBrowserWindowInterface()`).

**Why.** Tabs move between windows (drag-and-drop, "move to new window"), so a
stored window pointer goes stale: it points at the old window, or at a window
that was destroyed, which is a use-after-free.

## Examples

Bad:

```cpp
class FooTabFeature {
  raw_ptr<Browser> browser_;  // Stale when tab moves to another window!
};
```

Good:

```cpp
class FooTabFeature {
  raw_ref<tabs::TabInterface> tab_;
  // To access the current window: tab_->GetBrowserWindowInterface()
};
```
