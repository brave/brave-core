---
name: brave/own-tab-features-in-brave-tab-features
description:
  'A tab-scoped Brave feature is a plain class owned by BraveTabFeatures and
  created in its Init(), not a WebContentsUserData tab helper attached in
  brave::AttachTabHelpers().'
rubric: design
severity: should_fix
---

**Rule.** Make a new tab-scoped feature a plain class held as a
`std::unique_ptr` member of `BraveTabFeatures` and created, under its feature
guards, in `BraveTabFeatures::Init()`: `browser/ui/tabs/brave_tab_features.cc`
on desktop, and `browser/android/brave_tab_features.cc` where it also runs on
Android. Do not make it a `WebContentsUserData` helper attached in
`brave::AttachTabHelpers()` or `brave::AttachPrivacySensitiveTabHelpers()`;
those remain only for helpers that must also attach to WebContents that are not
tabs (AI chat's associated contents, background contents). A presubmit rejects
`CreateFor*` calls in `brave_tab_features.cc`, and new ones in
`brave_tab_helpers.cc` outside its allowlist.

**Why.** Upstream is moving every desktop tab helper to tab features, and
`TabHelpers::AttachTabHelpers` becomes remove-only on desktop; a tab feature
follows the tab, rather than whichever `WebContents` it currently shows.

## Examples

Bad:

```cpp
// browser/brave_tab_helpers.cc
FooTabHelper::MaybeCreateForWebContents(web_contents);
```

Good:

```cpp
// browser/ui/tabs/brave_tab_features.cc
void BraveTabFeatures::Init(TabInterface& tab, Profile* profile) {
  TabFeatures::Init(tab, profile);
  if (base::FeatureList::IsEnabled(kFoo)) {
    foo_ = std::make_unique<FooTabFeature>(tab);
  }
}
```
