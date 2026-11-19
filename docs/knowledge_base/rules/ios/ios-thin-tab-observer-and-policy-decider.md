---
name: brave/ios-thin-tab-observer-and-policy-decider
description:
  'BVC+TabObserver.swift and BVC+TabPolicyDecider.swift only delegate;
  feature-specific logic belongs in Tab helpers.'
rubric: design
severity: should_fix
---

**Rule.** In `BVC+TabObserver.swift` and `BVC+TabPolicyDecider.swift`, add only
thin delegation: forward the tab event or policy decision to the feature's `Tab`
helper. Do not add feature-specific conditions, state or business logic to these
files.

**Why.** Every tab event of every feature passes through these two files; logic
added there couples unrelated features in one place, and is harder to find and
test than in the feature's own helper.

## Examples

Bad:

```swift
func tab(_ tab: some TabState, didFinishNavigation navigation: WKNavigation) {
  if Preferences.Playlist.enablePlaylistURLBarButton.value,
    let url = tab.visibleURL, url.host == "www.youtube.com"
  {
    // ... Playlist-specific detection and UI updates.
  }
}
```

Good:

```swift
func tab(_ tab: some TabState, didFinishNavigation navigation: WKNavigation) {
  tab.playlistTabHelper?.didFinishNavigation()
}
```
