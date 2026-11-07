---
name: android_ios/ios-providers-and-tab-helpers
rubric: android_ios
severity: regression
---

**Rule.** Brave's copies of upstream's iOS wiring keep up with it: the provider
list in `ios/browser/providers/BUILD.gn`, the tab helpers `tab_helper_util.mm`
creates, the Swift policy deciders, and full-function replacements in
`chromium_src/ios`.

**Why.** Each is a copy of an upstream list or function, so it misses what
upstream adds: a new provider, a hardening done in a tab helper or decider, a
security fix to the replaced function.

**How to check.** For each candidate commit: a new provider goes into Brave's
list; a hardening in a tab helper or policy decider needs Brave's
`tab_helper_util.mm` to create that helper, or Brave's Swift decider to do the
same; a fix to a function `chromium_src/ios` replaces needs porting.

## Examples

- The WebSocket block for local pages, missed by a full-function replacement.
- M158: `NewTabPageTabHelper` now blocks web-initiated `about://newtab`, but
  Brave never creates it.
