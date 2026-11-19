---
name: brave/null-content-description-on-decorative-images
description:
  A purely decorative ImageView sets android:contentDescription="@null" (or
  setContentDescription(null)), not a string, an empty string or no attribute.
rubric: correctness
severity: should_fix
---

**Rule.** An `ImageView` that is purely decorative (not interactive, and
conveying nothing beyond nearby text, such as an icon beside a title or an
onboarding illustration) MUST set `android:contentDescription="@null"`, not a
string resource, an empty string or no attribute at all. In Java, the same
applies to `setContentDescription(...)`. Before flagging, confirm the image
really is decorative: clickable images, icon-only buttons, QR codes and status
icons with no accompanying text still need a real description.

**Why.** A description on a decorative image makes TalkBack read noise or repeat
the adjacent label, and adds a needless translatable string. `@null`, unlike an
empty or missing value, states that the image was deliberately left undescribed.

## Examples

Bad:

```xml
<ImageView
    android:src="@drawable/ic_shield_done"
    android:contentDescription="@string/brave_shields_title" />
<TextView
    android:text="@string/brave_shields_title" />
```

Good:

```xml
<ImageView
    android:src="@drawable/ic_shield_done"
    android:contentDescription="@null" />
<TextView
    android:text="@string/brave_shields_title" />
```
