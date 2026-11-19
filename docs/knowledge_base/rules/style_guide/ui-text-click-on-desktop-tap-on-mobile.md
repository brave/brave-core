---
name: brave/ui-text-click-on-desktop-tap-on-mobile
description: User-facing text says "click" on desktop and "tap" in mobile apps.
rubric: readability
severity: should_fix
---

**Rule.** Say "click" in desktop strings and "tap" in Android and iOS strings.

**Why.** Mobile users tap; telling them to click reads as a desktop product
ported carelessly.

## Examples

Bad:

```xml
<message name="IDS_PLAYLIST_ANDROID_HINT" desc="Hint in the Android playlist screen">
  Click a video to play it.
</message>
```

Good:

```xml
<message name="IDS_PLAYLIST_ANDROID_HINT" desc="Hint in the Android playlist screen">
  Tap a video to play it.
</message>
```
