---
name: brave/no-cached-web-contents-in-tab-features
description:
  "A tab feature never keeps the WebContents it was created with, as an
  observer, pointer or WeakPtr; it calls the tab's GetContents() when needed, or
  derives from ContentsObservingTabFeature."
rubric: correctness
severity: blocking
---

**Rule.** A tab feature MUST NOT bind itself to the `WebContents` its tab has
when the feature is created: no `content::WebContentsObserver` initialized with
`tab.GetContents()`, no `raw_ptr` / `raw_ref` / `WeakPtr` to that `WebContents`.
Hold `raw_ref<tabs::TabInterface>` and call `tab_->GetContents()` at the point
of use; to observe the contents, derive from
`tabs::ContentsObservingTabFeature`, or subscribe with
`TabInterface::RegisterWillDiscardContents()`. Follow this on Android too,
although Android does not replace a tab's contents.

**Why.** On desktop, discarding a tab replaces its `WebContents` while the tab
feature lives on: an observer of the old contents stops receiving events, a
`WeakPtr` to it becomes null, and a raw pointer to it dangles.

## Examples

Bad:

```cpp
class FooTabFeature : public content::WebContentsObserver {
 public:
  explicit FooTabFeature(tabs::TabInterface& tab)
      : WebContentsObserver(tab.GetContents()) {}
};
```

Good:

```cpp
class FooTabFeature : public tabs::ContentsObservingTabFeature {
 public:
  explicit FooTabFeature(tabs::TabInterface& tab)
      : ContentsObservingTabFeature(tab) {}
};
```
