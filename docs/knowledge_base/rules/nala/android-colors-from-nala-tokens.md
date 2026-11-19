---
name: brave/android-colors-from-nala-tokens
description:
  Android code uses existing Nala color tokens for its colors, instead of
  defining new color resources or hardcoding color values.
rubric: design
severity: should_fix
---

**Rule.** When a change needs a color on Android, reference an existing Nala
color token (`@color/button_background`, `@color/text_primary`, or
`R.color.text_primary` from Java). Do NOT define a new color value: no new
`<color>` resource holding a hex value in `res/values/colors.xml` or a similar
file, and no hex, `@android:color/*` or `Color.parseColor()` value in its place.
Where a resource is needed, it points at a Nala token. When flagging a
violation, tag `@brave/nala-token-reviewers`.

**Why.** New color definitions bypass the design system: they do not follow its
light and dark themes and drift from the same color on other platforms.

## Examples

Bad:

```xml
<!-- res/values/colors.xml -->
<color name="my_custom_blue">#1A73E8</color>
```

Good:

```xml
<!-- res/layout/my_feature_view.xml -->
<TextView
    android:id="@+id/title"
    android:textColor="@color/text_primary" />
```
