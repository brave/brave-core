---
name: brave/ui-describe-visuals-in-text
description:
  Images and other visuals in the UI come with descriptive text, so that no
  meaning relies solely on images, colors or animations.
rubric: design
severity: should_fix
---

**Rule.** Give every meaningful image, icon or visual descriptive text (alt
text, an accessible name, or a visible label), and do not convey information
only through an image, a color or an animation.

**Why.** Screen reader users get nothing from an undescribed visual, and
color-only cues are lost to color-blind users.

## Examples

Bad:

```tsx
<img src={shieldIcon} />
<span className={isBlocked ? 'red' : 'green'} />
```

Good:

```tsx
<img src={shieldIcon} alt={getLocale(S.SHIELDS_ICON_DESCRIPTION)} />
<span>{getLocale(isBlocked ? S.SHIELDS_BLOCKED : S.SHIELDS_ALLOWED)}</span>
```
