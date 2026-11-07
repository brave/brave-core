---
name: defaults/color-pins
rubric: defaults
severity: regression
---

**Rule.** Brave's color pins survive upstream's mixers.

**Why.** Brave's mixers run early, so a color ID that upstream newly assigns in
a later mixer overrides Brave's pin. A new ID defaulted to another
(`mixer[kNew] = {kOld}`) resolves through `GetLastNonPostprocessingMixer()`, so
it misses Brave's `postprocessing_mixer[kOld]`. A new branch in
`ThemeService::GetColorProviderKey()` ahead of the `GetIsGrayscale()` check
skips Brave's override.

**How to check.** The candidates are added `mixer[k…]` lines on IDs Brave
assigns, under `chrome/browser/ui/color`, `ui/color`, `ui/gtk`, `ui/qt` and
`chrome/browser/themes`, and changes to `GetColorProviderKey()`. Compare the
order in which the two mixers run.

## Examples

- The Linux native mixer re-tints the location icon chip.
- M158: the hovered inactive tab foreground.
- M158, 8113.1: the GTK accent.
