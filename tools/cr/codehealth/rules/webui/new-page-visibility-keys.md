---
name: webui/new-page-visibility-keys
rubric: webui
severity: regression
---

**Rule.** Each key upstream adds to the settings `PageVisibility` has a value in
Brave's `browser/resources/settings/br/page_visibility.ts`, or showing its page
in Brave is intended.

**Why.** Brave's page visibility is a proxy that answers `true` for any key it
doesn't list, so a new upstream section, menu item or route gated on
`pageVisibility.<key>` shows in Brave, even where upstream hides it.

**How to check.** For each key the candidates add to `PageVisibility`, look for
it in `staticProps` in `br/page_visibility.ts`. When it is missing, read what
upstream gates on it (`settings_menu.html.ts`, `settings_main.html.ts`,
`route.ts`), and whether Brave's manglers and routes cope with it shown.

## Examples

- M159, 8146.1: `home`, the Settings Refresh 2026 Home section, menu item and
  people child routes under `/`
  ([e49370626759](https://chromium.googlesource.com/chromium/src/+/e49370626759b8e48c12183b0fe02a5fe3dd99b8)).
