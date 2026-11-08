---
name: brave/no-intermediate-nala-color-ids
description:
  C++ UI uses a Nala color token such as nala::kColorContainerBackground
  directly at the call site, instead of a new kColor* id whose color mixer entry
  only maps to it.
rubric: design
severity: should_fix
---

**Rule.** Do NOT add a Brave color id (an `E_CPONLY(kColorFoo)` entry in
`brave_color_id.h`) whose color mixer entry just maps it to a Nala token
(`mixer[kColorFoo] = {nala::kColorBar};`). Use the Nala token directly where the
color is used: `views::CreateThemedSolidBackground(nala::kColorBar)`. Mapping an
existing upstream color id to a Nala token, so that upstream UI follows Nala, is
not an intermediate token.

**Why.** The extra id obscures which token the UI actually uses, and is one more
mapping to update when the design system changes.

## Examples

Bad:

```cpp
// In brave_color_id.h:
E_CPONLY(kColorSidebarPanelBackground)

// In brave_color_mixer.cc:
mixer[kColorSidebarPanelBackground] = {nala::kColorContainerBackground};

// In side_panel_view.cc:
SetBackground(views::CreateThemedSolidBackground(kColorSidebarPanelBackground));
```

Good:

```cpp
// In side_panel_view.cc:
SetBackground(
    views::CreateThemedSolidBackground(nala::kColorContainerBackground));
```
