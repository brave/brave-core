---
name: brave/baseline-color-resources-over-java-color-patches
description:
  Android color and theming fixes follow upstream by using baseline XML color
  resources for non-dynamic color states, rather than patching Java code to set
  colors programmatically.
rubric: design
severity: should_fix
---

**Rule.** To fix an Android color or theming issue, follow upstream's approach:
define baseline colors (XML color resources) for the non-dynamic color states,
rather than patching Java code to set colors programmatically (`setTextColor()`,
`setBackgroundColor()` and the like). The fix MUST be verified with the Dynamic
Colors flag both enabled and disabled.

**Why.** Baseline color resources are how upstream's theming system supplies
colors when dynamic colors are off, so overriding them is easier to maintain
than Java patches that fight that system. Dynamic colors replace many of these
colors, so a fix checked in only one mode is often broken in the other.

## Examples

Bad:

```diff
+        mDialogView.setBackgroundColor(
+                context.getColor(R.color.toolbar_background_primary_dark));
```

Good:

```xml
<!-- android/java/res/values/brave_colors.xml: the non-dynamic state's color. -->
<color name="dialog_bg_color_dark_baseline">@color/toolbar_background_primary_dark</color>
```
