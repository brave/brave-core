---
name: brave/share-identical-assets-with-ios
description:
  An asset file identical on Android and iOS, such as a Lottie animation JSON,
  is one shared copy that both reference, not two duplicates.
rubric: design
severity: should_fix
---

**Rule.** When an asset added for Android is identical to one iOS uses (or the
reverse), such as a Lottie animation JSON, both platforms MUST reference a
single shared copy instead of each keeping its own duplicate.

**Why.** Duplicated assets drift: a later change made to one copy is missed in
the other. With one shared copy, a change is made once.

## Examples

Bad:

```
browser/foo/android/java/res/raw/foo_animation.json
ios/brave-ios/Sources/Foo/LottieAssets/foo_animation.json  # the same file
```

Good:

```
# One copy, which the Android resources and the iOS bundle both reference.
components/foo/resources/foo_animation.json
```
