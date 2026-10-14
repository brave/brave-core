---
name: webui/removed-ancestor-prefs
rubric: webui
severity: regression
---

**Rule.** When upstream removes an element or property from an ancestor, every
Brave consumer reached through it moves to the replacement.

**Why.** Brave reaches upstream's prefs through the ancestors that held them.
Removing `<settings-prefs>`, or `prefs` on `settings-ui`/`settings-main`,
empties every Brave page below them at once.

**How to check.** For each candidate commit, grep Brave for every consumer
reached through what it removed: injected `prefs="{{prefs}}"` bindings,
`PrefsMixin`, `getPrefsElement()`, `querySelector('settings-prefs')` and
`CrSettingsPrefs`, including `chromium_src` search code that walks shadow roots.

## Examples

- M157, 8093.0: every Brave settings page lost its prefs, and the Brave search
  card stopped rendering.
