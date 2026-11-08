---
name: brave/prefer-material-components
description:
  'A hand-rolled view that reimplements an existing Material component is
  flagged as a nit only, asking why Material was not used; Brave subclasses of
  Material components are never flagged.'
rubric: design
severity: nit
---

**Rule.** When a Material component exists for a UI element, use it instead of a
custom version: `MaterialSwitch` over custom toggles, `MaterialButton` over a
styled `Button`, `BottomSheetDialogFragment` over hand-rolled sheets,
`MaterialAlertDialogBuilder` over custom modal views, and `Snackbar`, `Chip`,
`TextInputLayout` and `MaterialCardView`. When a Brave subclass of the Material
component exists in the module, callers use it rather than the raw Material
class.

Flag a violation as `nit:` only, NEVER as a blocker, and phrase it as a question
about why Material was not used. Before flagging, verify that the class does NOT
extend any `com.google.android.material.*` component anywhere in its hierarchy,
not just in the file in the diff. NEVER flag a Brave subclass of a Material
component (such as `BraveSettingsSwitch extends MaterialSwitch`) as a custom
reimplementation.

**Why.** Material components carry built-in motion, accessibility and theming
that hand-rolled `View` subclasses almost always miss. A Brave subclass is how
Brave adds theming or telemetry on top of Material, and the author of a custom
view may have a constraint that just is not documented.

## Examples

Bad:

```java
// nit: Why not MaterialSwitch?
public class BraveToggleView extends FrameLayout {
    private final View mThumb;
    private final View mTrack;
}
```

Good:

```java
public class BraveSettingsSwitch extends MaterialSwitch {
    // Brave theming on top of Material.
}
```
