---
name: brave/maybe-create-for-web-contents
description:
  'A WebContents helper that must also attach to non-tab WebContents, and is
  skipped for some (incognito, feature off), is attached by a static
  MaybeCreateForWebContents() holding the guards.'
rubric: design
severity: should_fix
---

**Rule.** For a `WebContentsUserData` helper that must be attached to the
`WebContents` itself, because it also runs on WebContents that are not tabs, and
that is not wanted on every one of them (skipped for incognito, or when its
feature is off), give it a static `MaybeCreateForWebContents()` that checks
those conditions and only then calls `CreateForWebContents()`. Do not create it
unconditionally and check the conditions inside it. A tab-scoped feature is not
such a helper: it belongs in `BraveTabFeatures`, with the guards in
`BraveTabFeatures::Init()` (see `brave/own-tab-features-in-brave-tab-features`).

**Why.** A helper attached everywhere costs memory and work on WebContents it
does nothing for, and spreads its "am I enabled" checks through its code.

## Examples

Bad:

```cpp
FooHelper::CreateForWebContents(web_contents);  // checks kFoo internally
```

Good:

```cpp
// static
void FooHelper::MaybeCreateForWebContents(content::WebContents* web_contents) {
  auto* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (!profile->IsRegularProfile() || !base::FeatureList::IsEnabled(kFoo)) {
    return;
  }
  FooHelper::CreateForWebContents(web_contents);
}
```
