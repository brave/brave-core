---
name: brave/feature-java-in-feature-android-directories
description:
  New feature-specific Java files go in the feature's browser/<feature>/android/
  directory, not the legacy android/java/org/chromium/chrome/browser/ tree,
  except subclasses of upstream classes from deprecated locations.
rubric: design
severity: should_fix
---

**Rule.** By default, a new feature-specific Java file goes in the feature's
Android directory: `browser/<feature>/android/` in Brave (paths relative to
`brave-core`), not `android/java/org/chromium/chrome/browser/...`; in Chromium,
`chrome/browser/<feature>/android/` over the deprecated
`chrome/android/java/src/org/chromium/chrome/browser/...` layout. Listing in
`brave_java_sources` does not by itself decide a file's location.

Exceptions, NEVER to be flagged:

- A new file that subclasses an upstream class in a deprecated location, placed
  in the corresponding deprecated Brave location (such as
  `android/java/org/chromium/chrome/browser/app/bookmarks/BraveBookmarkActivity.java`
  for upstream `BookmarkActivity`).
- `chrome/browser/android/`, which holds common browser-support code that
  feature-local Android code can depend on: it is not deprecated, though it is
  not the default home for feature-specific Java files either.

**Why.** Keeping Android code in the feature's directory keeps it with the
feature it implements, under that feature's own build target, instead of growing
the legacy tree.

## Examples

Bad:

```
android/java/org/chromium/chrome/browser/my_feature/MyFeatureController.java
```

Good:

```
browser/my_feature/android/java/src/org/chromium/chrome/browser/my_feature/MyFeatureController.java
```
