---
name: brave/tab-handle-over-raw-web-contents
description:
  'Stored tab references use tabs::TabHandle rather than raw WebContents*
  pointers.'
rubric: safety
severity: should_fix
---

**Rule.** When a class stores references to tabs (for example a list of tabs to
act on later), store `tabs::TabHandle`s rather than `content::WebContents*`
pointers, and map a `WebContents` to its handle with
`TabInterface::GetFromWebContents()`. This concerns objects whose lifetime is
independent of the tab: a tab-scoped object such as `TabFeatures` keeps a
`raw_ptr<content::WebContents>` to its own tab.

**Why.** A `TabHandle` is an integer identifier, so it is guaranteed not to
point to a different tab by accident. A raw pointer can dangle once the tab
closes and then be reused by a different allocation, including another tab's
`WebContents`.

## Examples

Bad:

```cpp
std::vector<content::WebContents*> tabs_to_close_;
```

Good:

```cpp
std::vector<tabs::TabHandle> tabs_to_close_;
```
