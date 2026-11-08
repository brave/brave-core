---
name: brave/keep-changes-inside-brave-core
description:
  Before a change modifies Chromium with a new chromium_src override, patch or
  plaster, it must rule out doing the work entirely in brave-core with Brave's
  own code and APIs.
rubric: design
severity: should_fix
---

**Rule.** When a change adds a new `chromium_src/` override, `patches/` patch or
`rewrite/` plaster substitution, check that the feature or fix could not be
written entirely in brave-core instead: in Brave's own classes, subclasses,
observers and services under `src/brave/`, through the APIs upstream already
exposes. Modify Chromium only when that is not possible, and then prefer a
chromium_src override over a patch
([brave/prefer-chromium-src-override-over-patch]).

**Why.** Every change to Chromium's code has to be carried across each Chromium
rebase, where it can conflict, and the speed of those rebases matters to Brave.
Code that lives only in brave-core uses upstream's public interfaces and does
not conflict.

## Examples

Bad:

```diff
// patches/chrome-browser-ui-tabs-tab_strip_model.cc.patch
+  brave::RecordTabOpened(profile_);
```

Good:

```cpp
// brave/browser/ui/tabs/brave_tab_strip_model_observer.cc
void BraveTabStripModelObserver::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (change.type() == TabStripModelChange::kInserted) {
    brave::RecordTabOpened(profile_);
  }
}
```
