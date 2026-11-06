---
name: webui/lit-siblings-of-patched-mixins
rubric: webui
severity: regression
---

**Rule.** A new `*_lit.ts` copy of a Polymer mixin or file Brave patches carries
Brave's patch too.

**Why.** Elements that migrate to the Lit copy lose what Brave's patch added to
the Polymer one, without any error.

**How to check.** Compare the range's new Lit files
(`git diff --name-status --diff-filter=A $A $B -- 'chrome/browser/resources/**/*_lit.ts'`)
against `patches/chrome-browser-resources-*`, and against the `chromium_src`
overrides of their Polymer twins. Elements that move onto an existing Lit twin
import its `*MixinLit`: check that twin the same way, once per twin.

## Examples

- `SiteSettingsMixinLit` and `braveCookieType`.
- M160, 8168.0: `site-entry` moves onto `SiteSettingsMixinLit`, which still
  lacks `braveCookieType`.
