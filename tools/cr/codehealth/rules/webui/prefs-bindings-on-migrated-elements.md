---
name: webui/prefs-bindings-on-migrated-elements
rubric: webui
severity: regression
---

**Rule.** Brave's `{{prefs.…}}` bindings, and the `prefs` Brave passes down to
its children, sit only in elements that still have the `prefs` property or
`PrefsMixin`, including elements that stay Polymer.

**Why.** Upstream's PrefService migration takes `prefs` away element by element.
A Brave binding inside a migrated element reads nothing, and a toggle two levels
down breaks without an error. The fix is `pref-key`.

**How to check.** The candidates lose or gain `PrefsMixin` or a
`prefs="{{prefs}}"` binding:
`git diff $A $B -- chrome/browser/resources | grep -E '^[-+].*(PrefsMixin|prefs="\{\{prefs\}\}")'`.
For each element, check Brave's `{{prefs.…}}` bindings inside it, and the
`prefs` it passes down to Brave children. Also grep Brave's templates for
`{{prefs.` every lift: master commits keep adding the old binding after the
branch migrated an element.

## Examples

- The email aliases toggle broke two levels down.
- M157: `settings-privacy-page` stayed Polymer, and Brave's personalization
  toggles lost their prefs.
- M158, 8117.1: the on-device speech model toggle, added on master with the old
  binding.
